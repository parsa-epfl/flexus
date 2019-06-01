// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <deque>
#include <vector>
#include <iterator>
#include <fstream>
#include <string>

#include <core/metaprogram.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <components/CommonQEMU/TraceTracker.hpp>

using namespace boost::multi_index;

#include <core/performance/profile.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

using namespace Flexus;

#include "InclusiveMESI.hpp"
#include "AbstractArray.hpp"
#include "CacheBuffers.hpp"
#include "MissTracker.hpp"

#define DBG_DeclareCategories Cache
#define DBG_SetDefaultOps AddCat(Cache) Set( (CompName) << theName ) Set( (CompIdx) << theNodeId )
#include DBG_Control()

namespace nCache {

namespace Stat = Flexus::Stat;

////////////////////////////////////////////////////////////////////////////////
// InclusiveMESI class

// Register the Implementation so that it can be created automagically
REGISTER_CONTROLLER_IMPLEMENTATION(InclusiveMESI, "InclusiveMESI");


// The automagic creation process calls this function
// The args should be empty, and the params contain a pointer to the controller and the init info.
BaseCacheControllerImpl * InclusiveMESI::createInstance(std::list<std::pair<std::string, std::string> > &args, const ControllerParams & params) {
  bool anAlwaysNAck = false;
  bool aSnoopLRU = false;
  bool anEvictAcksRequired = true;
  bool a2LevelPrivate = false;
  while (!args.empty()) {
    if (strcasecmp(args.front().first.c_str(), "AlwaysNAck") == 0) {
      anAlwaysNAck = boost::lexical_cast<bool>(args.front().second);
    } else if (strcasecmp(args.front().first.c_str(), "snoop_lru") == 0) {
      aSnoopLRU = boost::lexical_cast<bool>(args.front().second);
    } else if (strcasecmp(args.front().first.c_str(), "evict_acks_required") == 0) {
      anEvictAcksRequired = boost::lexical_cast<bool>(args.front().second);
    } else if (strcasecmp(args.front().first.c_str(), "two_level_private") == 0) {
      a2LevelPrivate = boost::lexical_cast<bool>(args.front().second);
    } else {
      DBG_Assert(false, NoDefaultOps() ( << "Unknown argument to InclusiveMESI controller: '" << args.front().first << "'" ));
    }
    args.pop_front();
  }
  DBG_Assert(args.empty(), NoDefaultOps() ( << "Tried to create InclusiveMESI controller with invalid arguments" ));
  return new InclusiveMESI(params.first, params.second, anAlwaysNAck, aSnoopLRU, anEvictAcksRequired, a2LevelPrivate);
}

InclusiveMESI::InclusiveMESI ( CacheController * aController,
                               CacheInitInfo * anInit,
                               bool   aNAckAlways,
                                bool            aSnoopLRU,
                                bool            anEvictAcksRequired,
                                bool            a2LevelPrivate )
  : BaseCacheControllerImpl ( aController, anInit )
  , theEvictBuffer(anInit->theEBSize)
  , theNAckAlways(aNAckAlways)
  , theSnoopLRU(aSnoopLRU)
  , theEvictAcksRequired(anEvictAcksRequired & !a2LevelPrivate)
  , the2LevelPrivate(a2LevelPrivate)
  , thePendingEvicts(0)
  , theEvictThreshold(4) {
  // We need to fix this to make it use a template and our state
  theArray = constructArray<State, State::Invalid>(anInit->theArrayConfiguration, anInit->theName, anInit->theNodeId, anInit->theBlockSize);
}

// Perform lookup, select action and update cache state if necessary
std::tuple<bool, bool, Action> InclusiveMESI::doRequest( MemoryTransport      transport,
    bool                 has_maf_entry,
    TransactionTracker_p waking_tracker) {
  bool is_write = false;
  bool is_prefetchwrite = false;
  bool is_upgrade = false;
  bool is_miss = false;
  bool is_hit = false;
  bool was_prefetched = false;

  Action action(kReplyAndRemoveMAF, transport[TransactionTrackerTag], 1);
  MemoryMessage_p msg = transport[MemoryMessageTag];
  TransactionTracker_p tracker = transport[TransactionTrackerTag];
  MemoryMessage_p request;
  MemoryAddress block_addr = getBlockAddress(msg->address());

  LookupResult_p lookup = (*theArray)[block_addr];

  DBG_( Iface, ( << " Do Request: " << *msg ) );

  // For now, we stall ALL requests if we have an outstanding request for this block
  if (has_maf_entry) {
    DBG_(Trace, ( << " Stalling request while we have a MAF entry, block state is " << lookup->state() << " : " << (*msg) ));
    return std::make_tuple(false, false, Action(kInsertMAF_WaitAddress, tracker));
  }

  // If this is a miss, then look in the Evict buffer
  if (lookup->state() == State::Invalid) {
    EvictBuffer<State>::iterator evictee = theEvictBuffer.find(getBlockAddress(msg->address()));
    bool evictee_write = false;
    if (evictee != theEvictBuffer.end()) {
      // If we're in the process of evicting the block, wait for the evict to complete
      // to avoid any races
      if (evictee->pending()) {
        return std::make_tuple(false, false, Action(kInsertMAF_WaitEvict, tracker));
      }

      // Make sure we can allocate the block, wait on the set if not
      DBG_Assert(theArray->canAllocate(lookup, getBlockAddress(msg->address())));

      State block_state = evictee->state();

      LookupResult_p victim = theArray->allocate(lookup, getBlockAddress(msg->address()));

      // Remove block from the evict buffer
      theEvictBuffer.remove(evictee);

      // Keep state from EB.
      lookup->setState(block_state);

      DBG_(Trace, ( << " replacing EB entry, evicting block in state " << victim->state() << " : " << *msg ));
      if (victim->state() != State::Invalid) {
        evictee_write = evictBlock(victim);
      }
      theTraceTracker.fill(theNodeId, theCacheLevel, getBlockAddress(msg->address()), theCacheLevel, false, evictee_write);
    }
  }

  if (lookup->state() != State::Invalid) {
    is_hit = true;
    was_prefetched = lookup->state().prefetched();
  }

  // Check the Snoop Buffer
  // First, look for any active snoops
  SnoopBuffer::snoop_iter iter = theSnoopBuffer.getActiveEntry(getBlockAddress(msg->address()));

  //bool snoop_inval_pending = false;
  if (iter != theSnoopBuffer.end()) {
// For now, disallow any overlap
// Need to make sure we handle the case where we have a line in Exclusive state, we get a ReadFwd, issue a Downgrade, and get a Read
// request before we get the downgrade reply. We have to make sure that we don't reply with MissReplyWritable
// rather than changed the state of the line initially, we just wait for the snoop to complete first
#if 0
    // Allow overlap of ReadFwd and FetchFwd requests, otherwise wait
    if ( ((iter->message->type() == MemoryMessage::ReadFwd)
          || (iter->message->type() == MemoryMessage::FetchFwd))
         && ((msg->type() == MemoryMessage::ReadReq) || (msg->type() == MemoryMessage::FetchReq))) {
      // Make sure any waiting snoops are also reads or fetches
      SnoopBuffer::snoop_iter end;
      std::tie(iter, end) = theSnoopBuffer.getWaitingEntries(getBlockAddress(msg->address()));
      for (; iter != end; iter++) {
        if ( (iter->message->type() != MemoryMessage::ReadFwd)
             && (iter->message->type() != MemoryMessage::FetchFwd)) {
          snoop_inval_pending = true;
          break;
        }
      }
    } else {
      snoop_inval_pending = true;
    }
    if (snoop_inval_pending) {
#endif
      return std::make_tuple(false, false, Action(kInsertMAF_WaitSnoop, tracker));
//   }
    } else if (theSnoopBuffer.hasEntry(getBlockAddress(msg->address() ) ) ) {
      return std::make_tuple(false, false, Action(kInsertMAF_WaitSnoop, tracker));
    }

    if (msg->type() == MemoryMessage::UpgradeReq && lookup->state() == State::Invalid) {
      // Upgrade must have been over-taken by an invalidate while waiting in the buffer
      // Convert to a WriteReq and continue
      msg->type() = MemoryMessage::WriteReq;
    }

    // handle each operation slightly differently
    switch (msg->type()) {

      case MemoryMessage::NonAllocatingStoreReq:
        is_write = true;

        if (!(lookup->state() == State::Modified) && !(lookup->state() == State::Exclusive)) {
          is_miss = true;
          action.theAction = kSend;
          action.theRequiresData = 0;

          // If we're the L1, we need to store this in a MAF so we reply with the correct size
          if (theCacheLevel == eL1) {
            MemoryMessage_p request(new MemoryMessage(MemoryMessage::NonAllocatingStoreReq, getBlockAddress(msg->address()), msg->pc()));
            request->reqSize() = theBlockSize;
            request->address() = getBlockAddress(msg->address());;
            request->coreIdx() = msg->coreIdx();;
            action.theBackMessage = true;
            action.theBackTransport = transport;
            action.theBackTransport.set(MemoryMessageTag, request);

            action.theAction = kInsertMAF_WaitResponse;
          }
        } else {
          is_hit = true;
          theArray->recordAccess(lookup);
          if (!(lookup->state() == State::Modified)) {
            lookup->setState(State::Modified);
          }
          msg->type() = MemoryMessage::NonAllocatingStoreReply;
        }
        break;

      case MemoryMessage::FetchReq:
        // IF we have the block in Exclusive state we need to probe the DCache to see if it has a modified
        // version. Otherwise, we don't need to probe, because we're an Inclusive Cache
        if (lookup->state() == State::Exclusive) {
          request = new MemoryMessage(MemoryMessage::Probe, getBlockAddress(msg->address()), msg->pc());
          action.theAction = kInsertMAF_WaitProbe;
          action.theRequiresData = 0;
          action.theFrontMessage = true;
          action.theFrontTransport = transport;
          action.theFrontTransport.set(MemoryMessageTag, request);
          request->reqSize() = 0;
          request->coreIdx() = msg->coreIdx();
          // Protect the block so it doesn't get evicted while we're waiting for the probe
          lookup->setProtected(true);
          is_hit = false;
        } else if (lookup->state() == State::Invalid) {
          if (theCacheLevel == eL1I) {
            MemoryMessage_p request(new MemoryMessage(MemoryMessage::FetchReq, getBlockAddress(msg->address()), msg->pc()));
            request->reqSize() = theBlockSize;
            request->address() = getBlockAddress(msg->address());
            request->coreIdx() = msg->coreIdx();
            action.theBackMessage = true;
            action.theBackTransport = transport;
            action.theBackTransport.set(MemoryMessageTag, request);
          }
          action.theAction = kInsertMAF_WaitResponse;
          is_miss = true;
        } else { // In MOS states we record the access (ie. update LRU) and send a reply
          is_hit = true;
          theArray->recordAccess(lookup);
          msg->type() = MemoryMessage::FetchReply;
          // Make sure the reply goes to the right cache
          action.theFrontToDCache = false;
          action.theFrontToICache = true;
        }
        break;

      case MemoryMessage::LoadReq:
        if (lookup->state() == State::Invalid) {

          MemoryMessage_p request(new MemoryMessage(MemoryMessage::ReadReq, getBlockAddress(msg->address()), msg->pc()));
          request->reqSize() = theBlockSize;
          request->coreIdx() = msg->coreIdx();
          request->address() = getBlockAddress(msg->address());
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, request);

          action.theAction = kInsertMAF_WaitResponse;
          is_miss = true;
        } else if (lookup->state() == State::Modified) {
          //lookup->setState(State::Exclusive);
          theArray->recordAccess(lookup);
          msg->type() = MemoryMessage::LoadReply;
          is_hit = true;
        } else { // Owned, Exclusive or Shared, no state change
          theArray->recordAccess(lookup);
          msg->type() = MemoryMessage::LoadReply;
          is_hit = true;
        }
        if (lookup->state().prefetched() && is_hit) {
          if ( tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher) ) {
            prefetchHitsPrefetch++;
            tracker->setFillLevel(eLocalMem);
            tracker->setNetworkTrafficRequired(false);
            tracker->setResponder(theNodeId);
            lookup->setPrefetched(false);
          }
        }
        break;

      case MemoryMessage::ReadReq:
        if (lookup->state() == State::Invalid) {
          action.theAction = kInsertMAF_WaitResponse;
          is_miss = true;
        } else if (lookup->state() == State::Modified) {
          lookup->setState(State::Exclusive);
          theArray->recordAccess(lookup);
          msg->type() = MemoryMessage::MissReplyDirty;
          is_hit = true;
        } else if (lookup->state() == State::Exclusive) {
          theArray->recordAccess(lookup);
          msg->type() = MemoryMessage::MissReplyWritable;
          is_hit = true;
        } else { // Owned or Shared, no state change
          theArray->recordAccess(lookup);
          msg->type() = MemoryMessage::MissReply;
          is_hit = true;
        }
        if (lookup->state().prefetched() && is_hit) {
          if ( tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher) ) {
            prefetchHitsPrefetch++;
            tracker->setFillLevel(eLocalMem);
            tracker->setNetworkTrafficRequired(false);
            tracker->setResponder(theNodeId);
            lookup->setPrefetched(false);
          }
        }
        break;

      case MemoryMessage::WriteReq:

        if (lookup->state() == State::Modified) {
          lookup->setState(State::Exclusive);
          msg->type() = MemoryMessage::MissReplyDirty;
          theArray->recordAccess(lookup);
          is_hit = true;
        } else if (lookup->state() == State::Exclusive) {
          msg->type() = MemoryMessage::MissReplyWritable;
          theArray->recordAccess(lookup);
          is_hit = true;
        } else if (lookup->state() == State::Invalid) {
          action.theAction = kInsertMAF_WaitResponse;
          is_miss = true;
        } else { // Shared or Owned
          request = new MemoryMessage(MemoryMessage::UpgradeReq, getBlockAddress(msg->address()));
          action.theAction = kInsertMAF_WaitResponse;
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, request);
          is_miss = true;
          upgrades++;
          is_upgrade = true;
          is_hit = false;

          // Protect this block so it doesn't get evicted
          lookup->setProtected(true);

          if (lookup->state().prefetched()) {
            theTraceTracker.prefetchHit(theNodeId, theCacheLevel, getBlockAddress(msg->address()), true);
            prefetchHitsButUpgrade++;
            lookup->setPrefetched(false);
          }
        }
        is_write = true;

        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq: {
        MemoryMessage::MemoryMessageType reply_type = MemoryMessage::NumMemoryMessageTypes;

        switch (msg->type()) {
          case MemoryMessage::RMWReq:
            reply_type = MemoryMessage::RMWReply;
            break;
          case MemoryMessage::CmpxReq:
            reply_type = MemoryMessage::CmpxReply;
            break;
          case MemoryMessage::StoreReq:
            reply_type = MemoryMessage::StoreReply;
            break;
          case MemoryMessage::StorePrefetchReq:
            reply_type = MemoryMessage::StorePrefetchReply;
            is_prefetchwrite = true;
            break;
          default:
            DBG_Assert(false, ( << "?!?!? What happened?" ));
            break;
        }

        if (lookup->state() == State::Modified) {
          msg->type() = reply_type;
          theArray->recordAccess(lookup);
          is_hit = true;
        } else if (lookup->state() == State::Exclusive) {
          lookup->setState(State::Modified);
          msg->type() = reply_type;
          theArray->recordAccess(lookup);
          is_hit = true;
        } else if (lookup->state() == State::Invalid) {
          action.theAction = kInsertMAF_WaitResponse;
          request = new MemoryMessage(MemoryMessage::WriteReq, getBlockAddress(msg->address()), msg->pc());
          request->reqSize() = theBlockSize;
          request->coreIdx() = msg->coreIdx();
          request->address() = getBlockAddress(msg->address());;
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, request);
          is_miss = true;
        } else { // Shared or Owned
          request = new MemoryMessage(MemoryMessage::UpgradeReq, getBlockAddress(msg->address()), msg->pc());
          request->coreIdx() = msg->coreIdx();
          action.theAction = kInsertMAF_WaitResponse;
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, request);
          is_miss = true;
          upgrades++;
          is_upgrade = true;
          is_hit = false;

          // Protect this block so it doesn't get evicted
          lookup->setProtected(true);

          if (lookup->state().prefetched()) {
            theTraceTracker.prefetchHit(theNodeId, theCacheLevel, getBlockAddress(msg->address()), true);
            prefetchHitsButUpgrade++;
            lookup->setPrefetched(false);
          }
        }

        is_write = true;

        break;
      }
      case MemoryMessage::UpgradeReq:
        if (lookup->state() == State::Shared) {
          action.theAction = kInsertMAF_WaitResponse;
          is_miss = true;
          is_upgrade = true;
          upgrades++;
          is_hit = false;

          // Protect this block so it doesn't get evicted
          lookup->setProtected(true);

          if (lookup->state().prefetched()) {
            theTraceTracker.prefetchHit(theNodeId, theCacheLevel, getBlockAddress(msg->address()), true);
            prefetchHitsButUpgrade++;
            lookup->setPrefetched(false);
          }
        } else if (lookup->state() == State::Exclusive ) {
          // TODO: find out HOW this happens.
          // Coherence protocol shouldn't allow this, but somehow it's happening.
          DBG_(Dev, ( << " Cache received Upgrade in Exclusive state: " << (*msg) ));
          msg->type() = MemoryMessage::UpgradeReply;
          theArray->recordAccess(lookup);
          is_hit = true;
          is_upgrade = true;
          upgrades++;
        } else {
          DBG_Assert(false, ( << " cache received Upgrade request in state " << lookup->state() << " : " << (*msg) ));
        }
        is_write = true;

        break;

      case MemoryMessage::PrefetchReadNoAllocReq:
        action.theRequiresData = 0;
        if (lookup->state() == State::Invalid) {
          action.theAction = kSend;
          is_miss = true;
        } else {
          msg->type() = MemoryMessage::PrefetchReadRedundant;
        }
        break;

        // Nothing else should reach here
      default:
        DBG_Assert(false, ( << "Got invalid message: " << *msg) );
    }

    if (lookup->state().prefetched() && is_hit) {
      if (is_write) {
        prefetchHitsWrite++;
        theTraceTracker.prefetchHit(theNodeId, theCacheLevel, getBlockAddress(msg->address()), true);
      } else {
        prefetchHitsRead++;
        theTraceTracker.prefetchHit(theNodeId, theCacheLevel, getBlockAddress(msg->address()), false);
      }
      lookup->setPrefetched(false);
    }

    if (is_hit) {
      if (tracker && !tracker->fillLevel()) {
        tracker->setNetworkTrafficRequired(false);
        tracker->setResponder(theNodeId);
        tracker->setFillLevel(theCacheLevel);
      }
    }
    if (is_upgrade) {
      tracker->setFillType(eCoherence);
    }

    switch (action.theAction) {
      case kInsertMAF_WaitResponse:
        action.theRequiresData = 0;
        if (action.theBackMessage == false) {
          action.theBackMessage = true;
          action.theBackTransport = transport;
        }
        theRequestTracker.startRequest(theArray->getSet(msg->address()));
        DBG_(Trace, ( << " starting Request in set " << theArray->getSet(msg->address()) << " for " << *msg ));
        break;

      case kReplyAndRemoveMAF:
        action.theFrontMessage = true;
        action.theFrontTransport = transport;
        break;

      case kSend:
        action.theBackMessage = true;
        action.theBackTransport = transport;
        action.theRequiresData = 0;
        break;

      case kInsertMAF_WaitProbe:
        theRequestTracker.startRequest(theArray->getSet(msg->address()));
        DBG_(Trace, ( << " starting Request in set " << theArray->getSet(msg->address()) << " for " << *msg ));

      default:
        break;
    }

    if (is_miss) {
      // TraceTracker miss
      if (!msg->isPrefetchType()) {
        theTraceTracker.access(theNodeId, theCacheLevel, msg->address(), msg->pc(), false, msg->isWrite(), !is_upgrade, msg->isPriv(), (tracker->logicalTimestamp() ? *tracker->logicalTimestamp() : 0 ));
      }

      misses++;
      if (tracker && tracker->isFetch() && *tracker->isFetch()) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_I++;
        } else {
          misses_user_I++;
        }
      } else if (is_prefetchwrite) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_PrefetchWrite++;
        } else {
          misses_user_D++;
          misses_user_D_PrefetchWrite++;
        }
      } else if (is_write) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_Write++;
        } else {
          misses_user_D++;
          misses_user_D_Write++;
        }
      } else {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_Read++;
        } else {
          misses_user_D++;
          misses_user_D_Read++;
        }
        theReadTracker.startMiss(tracker);
      }
    }

    return std::make_tuple(!is_miss, was_prefetched, action);
  }

  bool InclusiveMESI::evictBlock(LookupResult_p victim) {

    MemoryMessage::MemoryMessageType evict_type = MemoryMessage::EvictClean;

    if (victim->state() == State::Invalid) {
      return false;
    } else if ((victim->state() == State::Modified)) {
      evict_type = MemoryMessage::EvictDirty;
    } else if (victim->state() == State::Exclusive) {
      evict_type = MemoryMessage::EvictWritable;
    }

    // Let the L1-I cache do evictions without sending invalidates to the core
    theEvictBuffer.allocEntry(victim->blockAddress(), evict_type, victim->state(), (theCacheLevel == eL1I) );
    theTraceTracker.eviction(theNodeId, theCacheLevel, victim->blockAddress(), false);

    DBG_(Trace, ( << " Adding " << std::hex << victim->blockAddress() << " to the EvictBuffer." ));

    return (evict_type == MemoryMessage::EvictDirty);
  }

  bool InclusiveMESI::allocateBlock(LookupResult_p result, MemoryAddress aBlockAddress) {

    DBG_Assert(theArray->canAllocate(result, aBlockAddress), ( << " can NOT allocate block " << std::hex << aBlockAddress ));

    LookupResult_p victim = theArray->allocate(result, aBlockAddress);

    // Move the victim to the victim buffer as necessary
    if (victim->state() != State::Invalid) {
      evictBlock(victim);
    }

    return true;
  }

  // Handles a MemoryTransport from the back side
  // Action InclusiveMESI::handleBackMessage(MemoryMessage_p msg, TransactionTracker_p tracker) {
  Action InclusiveMESI::handleBackMessage(MemoryTransport transport) {
    MemoryMessage_p msg = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_( Trace, (  << " Handle BackProcess: " << *msg) );

    accesses++;
    if (tracker && tracker->isFetch() && *tracker->isFetch()) {
      if (tracker->OS() && *tracker->OS()) {
        accesses_system_I++;
      } else {
        accesses_user_I++;
      }
    } else {
      if (tracker && tracker->OS() && *tracker->OS()) {
        accesses_system_D++;
      } else {
        accesses_user_D++;
      }
    }

    // Protocol Messages are just place-holders to track proper latency
    if (msg->type() == MemoryMessage::ProtocolMessage) {
      Action act(kSend, tracker, false);

      act.theBackMessage = true;
      act.theBackTransport = transport;

      return act;
    }

    // Do a cache lookup
    LookupResult_p result = (*theArray)[msg->address()];

    // Look for an outstanding request
    MemoryMessage_p original_miss;
    TransactionTracker_p original_tracker;
    std::tie( original_miss, original_tracker) = theController->getWaitingMAFEntry( msg->address() );
    bool waiting_maf = (original_miss != nullptr);

    State state = result->state();

    EvictBuffer<State>::iterator evictee = theEvictBuffer.end();
    bool from_eb = false;
    if (state == State::Invalid) {
      evictee = theEvictBuffer.find(getBlockAddress(msg->address()));
      if (evictee != theEvictBuffer.end()) {
        from_eb = true;
        state = evictee->state();
        DBG_(Trace, ( << "Found block " << std::hex << getBlockAddress(msg->address()) << " in EB in state: " << state ));
      }
    }

    switch (msg->type()) {
        // ---- First, look at Request Types ---- //

        // A Read or Fetch forwarded from a peer cache.
      case MemoryMessage::ReadFwd:
      case MemoryMessage::FetchFwd:

        if (state == State::Invalid) {
          // We need to send FwdNack to the Directory
          Action act(kSend, tracker, false);

          msg->type() = MemoryMessage::FwdNAck;
          act.theBackMessage = true;
          act.theBackTransport = transport;

          return act;

        } else if (state == State::Shared) {
          Action act(kSend, tracker, true);
          msg->type() = MemoryMessage::FwdReply;
          act.theBackMessage = true;
          act.theBackTransport = transport;

          // Set the fill-level
          tracker->setNetworkTrafficRequired(false);
          tracker->setResponder(theNodeId);
          tracker->setFillLevel(thePeerLevel);
          tracker->setPreviousState(eShared);
          return act;

        } else if (state == State::Exclusive) {

          // If no snoop buffer entry, then we need to send snoops
          // otherwise, we just wait for the other snoop to return
          if (!theSnoopBuffer.hasEntry(msg->address())) {
            theSnoopBuffer.allocEntry(transport);
            Action act(kSend, tracker, false);
            intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Downgrade, msg->address(), msg->pc()));
            // make sure size is 0 so we don't get un-necessary update
            request->reqSize() = 0;
            request->coreIdx() = msg->coreIdx();
            act.theFrontMessage = true;
            act.theFrontTransport = transport;
            act.theFrontTransport.set(MemoryMessageTag, request);
            return act;
          } else {
            theSnoopBuffer.addWaitingEntry(transport);
            return Action(kNoAction, tracker, false);
          }
        } else if (state == State::Modified) {
          if (from_eb) {
            evictee->state() = State::Shared;
            evictee->type() = MemoryMessage::EvictClean;
          } else {
            result->setState(State::Shared);
          }
          msg->type() = MemoryMessage::FwdReplyOwned;
          Action act(kSend, tracker, true);
          act.theBackMessage = true;
          act.theBackTransport = transport;
          tracker->setNetworkTrafficRequired(false);
          tracker->setResponder(theNodeId);
          tracker->setFillLevel(thePeerLevel);
          tracker->setPreviousState(eModified);
          return act;
        }
        break;

      case MemoryMessage::WriteFwd:
        if (state == State::Invalid) {
          if (theNAckAlways) {
            // We need to send FwdNack to the Directory
            Action act(kSend, tracker, false);

            msg->type() = MemoryMessage::FwdNAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;

            return act;
          } else {
            // We just drop it, the directory will see our evict message and know that we dropped this
            return Action(kNoAction, tracker, false);
          }
        } else {
          // Check if we have a waiting upgrade, convert it to a Write if we do
          if (waiting_maf && original_miss->type() == MemoryMessage::UpgradeReq) {
            original_miss->type() = MemoryMessage::WriteReq;
        // Un-protect the block before invalidating it
        result->setProtected(false);
          }
          // L1-I can reply without sending the Invalidate up
          if (theCacheLevel == eL1I) {
            tracker->setNetworkTrafficRequired(true);
            tracker->setResponder(theNodeId);
            tracker->setFillLevel(theCacheLevel);

            if (!from_eb) {
              result->setState(State::Invalid);
              theArray->invalidateBlock(result);
            } else {
              DBG_Assert( evictee != theEvictBuffer.end());

              // if we've already sent an Evict to the next level, don't do anything
              // any new requests will stall until the evict completes
              // and the next level will ensure we don't get any other snoops
              if (!evictee->pending()) {
                // We haven't sent an evict yet, just clear the block from the evict buffer

                // the block is in the evict buffer
                // but it's invalid, so just get rid of the evict buffer entry
                theEvictBuffer.remove(evictee);
                evictee = theEvictBuffer.end();
              }
            }
            Action act(kSend, tracker, true /*Write Fwd needs to contain data*/);
            msg->type() = MemoryMessage::FwdReplyWritable;
            act.theBackMessage = true;
            act.theBackTransport = transport;
            return act;
          }
      // Stop sending Invalidates if the block is being evicted
      if (from_eb && evictee->pending()){
               tracker->setNetworkTrafficRequired(true);
           tracker->setResponder(theNodeId);
           tracker->setFillLevel(theCacheLevel);
               Action act(kSend, tracker, true /*Write Fwd needs to contain data*/);
           if (state == State::Modified)
                   msg->type() = MemoryMessage::FwdReplyDirty;
           else
              msg->type() = MemoryMessage::FwdReplyWritable;
           act.theBackMessage = true;
           act.theBackTransport = transport;
           return act;
       }
          // First, wait for outstanding Read/Fetch downgrade requests
          if (!theSnoopBuffer.hasEntry(msg->address())) {

            SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
            snp->i_snoop_outstanding = (theCacheLevel != eL1);
            // if it's modified it can't be cached in the dcache

            Action act(kSend, tracker, false);
            intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address(), msg->pc()));
            request->reqSize() = 0;
            request->coreIdx() = msg->coreIdx();
            act.theFrontMessage = true;
            act.theFrontTransport = transport;
            act.theFrontTransport.set(MemoryMessageTag, request);
            if (result->state() == State::Modified && theCacheLevel != eL1) {
              act.theFrontToDCache = false;
              snp->d_snoop_outstanding = false;
            } else {
              act.theFrontToDCache = true;
              snp->d_snoop_outstanding = true;
            }
            act.theFrontToICache = (theCacheLevel != eL1);
            return act;
          } else {
            theSnoopBuffer.addWaitingEntry(transport);
            return Action(kNoAction, tracker, false);
          }
        }
        break;

      case MemoryMessage::Invalidate:
        if (state == State::Invalid) {
          if (theNAckAlways) {
            // We need to send FwdNack to the Directory
            Action act(kSend, tracker, false);

            msg->type() = MemoryMessage::InvalidateNAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;

            return act;
          } else {
            // Drop invalidates to avoid duplicate Evict/InvalAck messages
            return Action(kNoAction, tracker, false);
          }
        } else if (((state == State::Exclusive)
                    || (state == State::Modified)) && !the2LevelPrivate) {
          // We should get a WriteFwd request instead of an Invalidate if we're modified or exclusive
          // This is bad
          DBG_Assert(false, ( << "received in Invalidate while in " << state << " state : " << (*msg) ));
        } else {
          // state == Shared or Owner
          // If we have an outstanind upgrade, change it to a write
          if (waiting_maf && original_miss->type() == MemoryMessage::UpgradeReq) {
            original_miss->type() = MemoryMessage::WriteReq;
        result->setProtected(false);
          }
          // Make sure we don't have an outstanding read/fetch because that shouldn't be possibl
          DBG_Assert(!waiting_maf || original_miss->isWrite(), ( << "Received invalidate for block in " << state << " state while waiting for: " << *original_miss ));

          // L1-I can reply without sending the Invalidate up
          if (theCacheLevel == eL1I) {
            if (!tracker->fillType()) {
              tracker->setFillType(eCoherence);
            }
            if (!from_eb) {
              result->setState(State::Invalid);
              theArray->invalidateBlock(result);
            } else {
              DBG_Assert( evictee != theEvictBuffer.end());

              // if we've already sent an Evict to the next level, don't do anything
              // any new requests will stall until the evict completes
              // and the next level will ensure we don't get any other snoops
              if (!evictee->pending()) {
                // We haven't sent an evict yet, just clear the block from the evict buffer

                // the block is in the evict buffer
                // but it's invalid, so just get rid of the evict buffer entry
                theEvictBuffer.remove(evictee);
                evictee = theEvictBuffer.end();
              }
            }
            Action act(kSend, tracker, false);
            msg->type() = MemoryMessage::InvalidateAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;
            return act;
          }


          if (from_eb && evictee->pending()) {
            Action act(kSend, tracker, false);
            msg->type() = MemoryMessage::InvalidateAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;
            return act;
          } else if (from_eb) {
            // This avoids a possible race condition if we send snoops for something in the EB
            // and then decide to do the eviction before those snoops return.
            // If the EB entry is evictable, then just send an Ack and remove the EB entry.
            if (evictee->evictable()) {
              // Need to remove evict buffer entry
              theEvictBuffer.remove(evictee);

          Action act(kSend, tracker, false);
          msg->type() = MemoryMessage::InvalidateAck;
          act.theBackMessage = true;
          act.theBackTransport = transport;
          return act;
        }
          }

          // First, look for outstanding snoops, and wait for them to complete
          // if no snoops, send out some invalidates
          if (!theSnoopBuffer.hasEntry(msg->address())) {

            SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
            snp->i_snoop_outstanding = (theCacheLevel != eL1);
            snp->d_snoop_outstanding = true;

            Action act(kSend, tracker, false);
            intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address(), msg->pc()));
            request->reqSize() = 0;
            request->coreIdx() = msg->coreIdx();
            act.theFrontMessage = true;
            act.theFrontTransport = transport;
            act.theFrontTransport.set(MemoryMessageTag, request);
            act.theFrontToDCache = true;
            act.theFrontToICache = (theCacheLevel != eL1);
            return act;

          } else {
            theSnoopBuffer.addWaitingEntry(transport);
            return Action(kNoAction, tracker, false);
          }
        }
      case MemoryMessage::BackInvalidate:
        if (state == State::Invalid) {
          if (theNAckAlways) {
            // We need to send FwdNack to the Directory
            Action act(kSend, tracker, false);

            msg->type() = MemoryMessage::InvalidateNAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;

            return act;
          } else {
            DBG_(Trace, ( << "Received BackInval for block in invalid state. Droping backinval: " << *msg ));
            return Action(kNoAction, tracker, false);
          }
        } else {
          // If there's a request in the MAF then it's an upgrade
          // The Other request has been ordered before our upgrade
          // so we change the upgrade to a write request

          // It could also be a Store, Store Prefetch, RMW, or CmpExch request
          if (waiting_maf) {
            if (original_miss->type() == MemoryMessage::UpgradeReq) {
              DBG_Assert(state != State::Modified, ( << "Expected MAF to be Upgrade for block NOT in modified state, instead, block is " << state << " and maf is for: " << *original_miss ));
              original_miss->type() = MemoryMessage::WriteReq;
              // Un-protect the block before invalidating it
          result->setProtected(false);
            } else {
              DBG_Assert((original_miss->type() == MemoryMessage::StoreReq) || (original_miss->type() == MemoryMessage::StorePrefetchReq) || (original_miss->type() == MemoryMessage::CmpxReq) || (original_miss->type() == MemoryMessage::RMWReq), ( << "Expected MAF to be Store/StorePrefetch/Cmpx/RMW, but instead found " << *original_miss));
              DBG_Assert(state != State::Modified, ( << "Expected MAF to be Store for block NOT in modified state, instead, block is " << state << " and maf is for: " << *original_miss ));
            }
          }

          // L1-I can reply without sending the Invalidate up
          if (theCacheLevel == eL1I) {
            if (!tracker->fillType()) {
              tracker->setFillType(eCoherence);
            }
            if (!from_eb) {
              result->setState(State::Invalid);
              theArray->invalidateBlock(result);
            } else {
              DBG_Assert( evictee != theEvictBuffer.end());

              // if we've already sent an Evict to the next level, don't do anything
              // any new requests will stall until the evict completes
              // and the next level will ensure we don't get any other snoops
              if (!evictee->pending()) {
                // We haven't sent an evict yet, just clear the block from the evict buffer

                // the block is in the evict buffer
                // but it's invalid, so just get rid of the evict buffer entry
                theEvictBuffer.remove(evictee);
                evictee = theEvictBuffer.end();
              }
            }
            Action act(kSend, tracker, false);
            msg->type() = MemoryMessage::InvalidateAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;
            return act;
          }


          if (from_eb && evictee->pending()) {
            Action act(kSend, tracker, false);
            msg->type() = MemoryMessage::InvalidateAck;
            act.theBackMessage = true;
            act.theBackTransport = transport;
            return act;
          }


          // First, look for outstanding snoops, and wait for them to complete
          if (!theSnoopBuffer.hasEntry(msg->address())) {

            SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
            snp->i_snoop_outstanding = (theCacheLevel != eL1);
            snp->d_snoop_outstanding = (theCacheLevel == eL1) || (state != State::Modified);

            Action act(kSend, tracker, false);
            intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address(), msg->pc()));
            request->reqSize() = 0;
            request->coreIdx() = msg->coreIdx();
            act.theFrontMessage = true;
            act.theFrontTransport = transport;
            act.theFrontTransport.set(MemoryMessageTag, request);
            act.theFrontToICache = (theCacheLevel != eL1);
            act.theFrontToDCache = (theCacheLevel == eL1) || (state != State::Modified);
            return act;
          } else {
            theSnoopBuffer.addWaitingEntry(transport);
            return Action(kNoAction, tracker, false);
          }
        }

        // Evict act
      case MemoryMessage::EvictAck: {
        // Make sure we found the block in the evict buffer
        DBG_Assert(from_eb);

        // Need to remove the block from the evict buffer and wake any requests waiting on the evict
        theEvictBuffer.remove(evictee);

        Action act(kNoAction, tracker, false);
        act.theWakeEvicts = true;

        thePendingEvicts--;

        return act;
      }

      // ---- Reply Types are all pretty similar, we'll handle most of them separately below ---- //
      case MemoryMessage::FwdReply:
      case MemoryMessage::FwdReplyOwned:
      case MemoryMessage::FwdReplyWritable:
      case MemoryMessage::FwdReplyDirty:
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvalidateNAck:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::MissNotifyData:
      case MemoryMessage::FetchReply:
      case MemoryMessage::UpgradeReply:
      case MemoryMessage::MissReply:
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::MissReplyDirty:
        break;

        // Prefetch replies are just sent up the hierarchy
        // This will really mess with our inclusion if they get inserted into L1s, but ignore this for now
      case MemoryMessage::PrefetchReadReply:
      case MemoryMessage::PrefetchWritableReply:
      case MemoryMessage::PrefetchReadRedundant: {
        Action act(kSend, tracker, false);
        act.theFrontMessage = true;
        act.theFrontTransport = transport;
        return act;
      }
      case MemoryMessage::NonAllocatingStoreReply: {
        if (theCacheLevel != eL1) {
          Action act(kSend, tracker, false);
          act.theFrontMessage = true;
          act.theFrontTransport = transport;
          if (msg->ackRequired()) {
            intrusive_ptr<MemoryMessage> ack(new MemoryMessage(MemoryMessage::NASAck, msg->address(), msg->pc()));
            act.theBackMessage = true;
            act.theBackTransport = transport;
            act.theBackTransport.set(MemoryMessageTag, ack);
          }
          return act;
        }
        theRequestTracker.endRequest(theArray->getSet(msg->address()));
        DBG_(Trace, ( << " ending Request in set " << theArray->getSet(msg->address()) << " for " << *msg ));
        break;
      }

      // MissNotify -> tells us how many invalidates we should wait for
      case MemoryMessage::MissNotify: {
        DBG_Assert( waiting_maf, ( << " received miss notify with no outstanding request : " << (*msg) ));
        DBG_Assert( original_miss->isWrite(), ( << " received MissNotify while wrong type of message was waiting. orig msg : " << (*msg) << ", reply : " << (*msg) ));

        MissAddressFile::maf_iter maf_entry = theController->getWaitingMAFEntryIter(msg->address());
        maf_entry->outstanding_msgs += msg->outstandingMsgs();

        // It's possible we've already received all the other msgs BEFORE we get this notify
        // in this case the # of outstanding msgs should now be zero and we can send our replies
        if (maf_entry->outstanding_msgs == 0) {
          bool sent_upgrade = (original_miss->type() == MemoryMessage::UpgradeReq);
          if (!maf_entry->data_received && result->state() != State::Invalid) {
            tracker->setNetworkTrafficRequired(true);
            tracker->setResponder(theNodeId);
            if (!tracker->fillLevel()) {
              tracker->setFillLevel(theCacheLevel);
            }
            maf_entry->data_received = true;
            sent_upgrade = true;
          }
          DBG_Assert( (original_miss->type() == MemoryMessage::UpgradeReq) || maf_entry->data_received, ( << " # of outstanding msgs after receiveing miss notify is 0, but we don't have data yet." ));
          DBG_Assert(!from_eb, ( << "received miss notify but data in evict buffer." ));
          Action action(kReplyAndRemoveResponseMAF, tracker, true);

          intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::WriteAck, msg->address(), msg->pc()));
          if (sent_upgrade) {
            reply->type() = MemoryMessage::UpgradeAck;
          }
        // Un-protect the block before invalidating it
        result->setProtected(false);
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, reply);

          if (original_miss->type() == MemoryMessage::StoreReq) {
            msg->type() = MemoryMessage::StoreReply;
            result->setState(State::Modified);
          } else if (original_miss->type() == MemoryMessage::StorePrefetchReq) {
            msg->type() = MemoryMessage::StorePrefetchReply;
            result->setState(State::Modified);
          } else if (original_miss->type() == MemoryMessage::CmpxReq) {
            msg->type() = MemoryMessage::CmpxReply;
            result->setState(State::Modified);
          } else if (original_miss->type() == MemoryMessage::RMWReq) {
            msg->type() = MemoryMessage::RMWReply;
            result->setState(State::Modified);
          } else {
            if (result->state() == State::Modified) {
              msg->type() = MemoryMessage::MissReplyDirty;
            } else {
              DBG_Assert(result->state() != State::Invalid);
              msg->type() = MemoryMessage::MissReplyWritable;
            }
            if (original_miss->type() == MemoryMessage::UpgradeReq) {
              msg->type() = MemoryMessage::UpgradeReply;
            }

            result->setState(State::Exclusive);
          }

          if (theCacheLevel == eL1) {
            msg->reqSize() = original_miss->reqSize();
            msg->address() = original_miss->address();
            msg->coreIdx() = original_miss->coreIdx();
          } else {
            msg->reqSize() = theBlockSize;
          }

          action.theFrontMessage = true;
          action.theFrontTransport = transport;

          // We're done with this request
          theRequestTracker.endRequest(theArray->getSet(msg->address()));
          DBG_(Trace, ( << " ending Request in set " << theArray->getSet(msg->address()) << " for " << *msg ));

          return action;
        }

        return Action(kNoAction, tracker);
      }
      default:
        DBG_Assert(false, ( << "Received unexpected message type on back side : " << (*msg) ));
        break;
    }

    bool is_fill = false;
    bool is_prefetch = false;
    bool is_write = false;
    bool is_upgrade = false;
    bool is_final = false;
    bool is_fetch = false;

    // If we get here, then we're servicing a reply msg
    // This means we should have an outstanding miss for this address, otherwise, we're in trouble
    DBG_Assert( original_miss != nullptr, ( << "received reply with no corresponding miss : " << (*msg) ));

    // Most of our actions depend on the type of the original message, so use that for the high-level swich

    Action action(kReplyAndRemoveResponseMAF, tracker, true);

    switch (original_miss->type()) {
      case MemoryMessage::FetchReq: {
        DBG_Assert( state == State::Invalid, ( << "Received reply to fetch req but block not invalid - " << state << " : " << (*msg) ));

        // We need to allocate the block in the data array
        allocateBlock(result, msg->address());

        switch (msg->type()) {
          case MemoryMessage::FetchReply:
          case MemoryMessage::FwdReply:
          case MemoryMessage::MissReply:
          case MemoryMessage::FwdReplyOwned:
            result->setState(State::Shared);
            break;
          default:
            DBG_Assert(false, ( << "Received invalid reply to a fetch request : " << (*msg) ));
            break;
        }

        is_fill = true;
        is_final = true;
        is_fetch = true;

        DBG_( Iface, (  << " Creating Ack" ) );
        if (msg->ackRequired()) {
          intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::FetchAck, msg->address(), msg->pc()));
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, reply);

          if (msg->type() == MemoryMessage::FwdReplyOwned) {
            reply->type() = MemoryMessage::FetchAckDirty;
          }

          // Include Data with Ack
          reply->reqSize() = theBlockSize;
          reply->coreIdx() = msg->coreIdx();
          reply->ackRequiresData() = msg->ackRequiresData();
        }
        msg->type() = MemoryMessage::FetchReply;

        if (theCacheLevel == eL1I) {
          msg->reqSize() = original_miss->reqSize();
          msg->address() = original_miss->address();
          msg->pc() = original_miss->pc();
          msg->coreIdx() = original_miss->coreIdx();
        }

        // Make sure the reply goes to the right cache
        action.theFrontToDCache = false;
        action.theFrontToICache = true;
        break;
      }
      case MemoryMessage::LoadReq: {
        DBG_Assert( state == State::Invalid, ( << "Received reply to read req but block not invalid - " << state << " : " << (*msg) ));

        // We need to allocate the block in the data array
        allocateBlock(result, msg->address());
        bool dirty_data = false;

        switch (msg->type()) {
          case MemoryMessage::FwdReplyOwned:
            dirty_data = true;
          case MemoryMessage::MissReply:
          case MemoryMessage::FwdReply:
            result->setState(State::Shared);
            break;
          case MemoryMessage::MissReplyWritable:
            result->setState(State::Exclusive);
            break;
          case MemoryMessage::MissReplyDirty:
            result->setState(State::Modified);
            break;
          default:
            DBG_Assert(false, ( << "Received invalid reply to a read request : " << (*msg) ));
            break;
        }

        msg->type() = MemoryMessage::LoadReply;

        is_fill = true;
        is_final = true;
        if (msg->ackRequired()) {
          intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::ReadAck, msg->address(), msg->pc()));
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, reply);

          if (dirty_data) {
            reply->type() = MemoryMessage::ReadAckDirty;
          }

          // Include Data with Ack
          reply->reqSize() = theBlockSize;
          reply->ackRequiresData() = msg->ackRequiresData();
        }

        DBG_Assert(theCacheLevel == eL1);
        msg->reqSize() = original_miss->reqSize();
        msg->address() = original_miss->address();
        msg->pc() = original_miss->pc();
        msg->coreIdx() = original_miss->coreIdx();

        DBG_( Trace, (  << " Allocated Block, State = " << result->state() << ", isHit? " << std::boolalpha << result->hit() ));
        break;
      }
      case MemoryMessage::ReadReq:{
        DBG_Assert( state == State::Invalid, ( << "Received reply to read req but block not invalid - " << state << " : " << (*msg) ));

        // We need to allocate the block in the data array
        allocateBlock(result, msg->address());
        bool dirty_data = false;

        switch (msg->type()) {
          case MemoryMessage::MissReplyDirty:
            result->setState(State::Exclusive);
            dirty_data = true;
            // Fall-through
          case MemoryMessage::MissReply:
          case MemoryMessage::FwdReply:
            msg->type() = MemoryMessage::MissReply;
            result->setState(State::Shared);
            break;
          case MemoryMessage::MissReplyWritable:
            result->setState(State::Exclusive);
            break;
          default:
            DBG_Assert(false, ( << "Received invalid reply to a read request : " << (*msg) ));
            break;
        }

        is_fill = true;
        is_final = true;
        if (msg->ackRequired()) {
          intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::ReadAck, msg->address(), msg->pc()));
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, reply);

          if (dirty_data) {
            reply->type() = MemoryMessage::ReadAckDirty;
          }

          // Include Data with Ack
          reply->reqSize() = theBlockSize;
          reply->ackRequiresData() = msg->ackRequiresData();
        }
        DBG_( Trace, (  << " Allocated Block, State = " << result->state() << ", isHit? " << std::boolalpha << result->hit() ));
        break;
      }
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::CmpxReq:
      case MemoryMessage::RMWReq:
      case MemoryMessage::WriteReq: {
        // Shouldn't get these two types
        DBG_Assert( msg->type() != MemoryMessage::MissReply, ( << " received MissReply in response to outstanding write : " << (*msg) ));
        DBG_Assert( msg->type() != MemoryMessage::FwdReply, ( << " received FwdReply in response to outstanding write : " << (*msg) ));
        DBG_Assert( !from_eb );

        MissAddressFile::maf_iter maf_entry = theController->getWaitingMAFEntryIter(msg->address());

        if (msg->type() == MemoryMessage::MissNotifyData) {
          maf_entry->outstanding_msgs += msg->outstandingMsgs();
        } else {
          // One less msg to wait for
          maf_entry->outstanding_msgs--;
        }

        bool is_dirty = false;
        bool has_data = false;

        switch (msg->type()) {
          case MemoryMessage::UpgradeReply:
            maf_entry->outstanding_msgs = 0;
            break;
          case MemoryMessage::InvalidateAck:
          case MemoryMessage::InvalidateNAck:
            break;
          case MemoryMessage::MissNotifyData:
          case MemoryMessage::FwdReplyOwned:
          case MemoryMessage::FwdReplyDirty:
          case MemoryMessage::InvUpdateAck:
          case MemoryMessage::MissReplyDirty:
            is_dirty = true;
            // Fall Through!
          case MemoryMessage::FwdReplyWritable:
          case MemoryMessage::MissReplyWritable:
            has_data = true;
            break;
          default:
            DBG_Assert(false, ( << "WriteReq received unexpected reply: " << (*msg) ));
            break;
        }

        if (has_data) {
          if (result->state() == State::Invalid) {
            maf_entry->data_received = true;
            allocateBlock(result, msg->address());
            result->setProtected(true);
            is_fill = true;
          } else if (!is_dirty) {
            has_data = false;
          }
          if (is_dirty) {
            result->setState(State::Modified);
          } else {
            result->setState(State::Exclusive);
          }
        }

        if (!msg->ackRequired() && ((msg->type() == MemoryMessage::MissReplyWritable)
                                    ||  (msg->type() == MemoryMessage::MissReplyDirty))) {
          maf_entry->outstanding_msgs = 0;
        }
        if ((msg->outstandingMsgs() == -1) && ((msg->type() == MemoryMessage::MissReplyWritable)
                                               ||  (msg->type() == MemoryMessage::MissReplyDirty))) {
          maf_entry->outstanding_msgs = 0;
        }

        if (maf_entry->outstanding_msgs == 0) {
          bool sent_upgrade = false;
          if (!maf_entry->data_received && result->state() != State::Invalid) {
            tracker->setNetworkTrafficRequired(true);
            tracker->setResponder(theNodeId);
            if (!tracker->fillLevel()) {
              tracker->setFillLevel(theCacheLevel);
            }
            maf_entry->data_received = true;
            sent_upgrade = true;
          }

          DBG_Assert(maf_entry->data_received, ( << " # of outstanding msgs after receiveing miss notify is 0, but we don't have data yet." ));

          if (msg->ackRequired()) {
            intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::WriteAck, msg->address(), msg->pc()));
            if (sent_upgrade) {
              reply->type() = MemoryMessage::UpgradeAck;
            }
            reply->ackRequiresData() = false;
            action.theBackMessage = true;
            action.theBackTransport = transport;
            action.theBackTransport.set(MemoryMessageTag, reply);
          }

          msg->reqSize() = theBlockSize;
          if (original_miss->type() != MemoryMessage::WriteReq) {
            switch (original_miss->type()) {
              case MemoryMessage::StorePrefetchReq:
                msg->type() = MemoryMessage::StorePrefetchReply;
                break;
              case MemoryMessage::StoreReq:
                msg->type() = MemoryMessage::StoreReply;
                break;
              case MemoryMessage::CmpxReq:
                msg->type() = MemoryMessage::CmpxReply;
                break;
              case MemoryMessage::RMWReq:
                msg->type() = MemoryMessage::RMWReply;
                break;
              default:
                DBG_Assert(false, ( << "What??" ));
                break;
            }
            DBG_Assert(theCacheLevel == eL1);
            result->setState(State::Modified);
            DBG_(Trace, ( << theName << " received reply to Store/Atomic setting size to " << original_miss->reqSize() ));
            msg->reqSize() = original_miss->reqSize();
            msg->address() = original_miss->address();
            msg->pc() = original_miss->pc();
            msg->coreIdx() = original_miss->coreIdx();
          } else if (result->state() == State::Modified) {
            msg->type() = MemoryMessage::MissReplyDirty;
            result->setState(State::Exclusive);
          } else {
            result->setState(State::Exclusive);
            msg->type() = MemoryMessage::MissReplyWritable;
          }
          // make sure we have the right size for the request
          result->setProtected(false);
          theArray->recordAccess(result);
          is_final = true;
        } else {
          action.theAction = kNoAction;
          if (!has_data) {
            action.theRequiresData = 0;
          }
        }
        is_write = true;

        break;
      }
      case MemoryMessage::UpgradeReq: {
        // Shouldn't get these two types
        DBG_Assert( msg->type() != MemoryMessage::MissReply, ( << " received MissReply in response to outstanding write : " << (*msg) ));
        DBG_Assert( msg->type() != MemoryMessage::FwdReply, ( << " received FwdReply in response to outstanding write : " << (*msg) ));
        DBG_Assert( !from_eb );

        MissAddressFile::maf_iter maf_entry = theController->getWaitingMAFEntryIter(msg->address());

        // One less msg to wait for
        maf_entry->outstanding_msgs--;

        // This is an upgrade, so we shouldn't be in the invalid state
        DBG_Assert( result->state() != State::Invalid, ( << "received reply to upgrade with block in invalid state : " << (*msg) ));

        switch (msg->type()) {
          case MemoryMessage::UpgradeReply:
            maf_entry->outstanding_msgs = 0;
            break;
          case MemoryMessage::InvalidateAck:
          case MemoryMessage::InvalidateNAck:
            break;
          case MemoryMessage::FwdReplyOwned:
            result->setState(State::Modified);
            break;
          default:
            DBG_Assert(false, ( << "UpgradeReq received unexpected reply: " << (*msg) ));
            break;
        }

        if ((msg->outstandingMsgs() == -1) && ((msg->type() == MemoryMessage::MissReplyWritable)
                                               ||  (msg->type() == MemoryMessage::MissReplyDirty))) {
          maf_entry->outstanding_msgs = 0;
        }

        if (maf_entry->outstanding_msgs == 0) {
          //msg->reqSize() = original_miss->reqSize();
          result->setState(State::Exclusive);
          result->setProtected(false);
          theArray->recordAccess(result);
          is_final = true;
          if (msg->ackRequired()) {
            intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::UpgradeAck, msg->address(), msg->pc()));
            action.theBackMessage = true;
            action.theBackTransport = transport;
            action.theBackTransport.set(MemoryMessageTag, reply);
            reply->ackRequiresData() = false;
          }
          msg->type() = MemoryMessage::UpgradeReply;
          msg->reqSize() = theBlockSize;

          // Make sure we fill in the tracker
          tracker->setNetworkTrafficRequired(true);
          tracker->setResponder(theNodeId);

          // Upgrade request should be something else
          if (!tracker->fillLevel()) {
            tracker->setFillLevel(theCacheLevel);
          }
        } else {
          action.theAction = kNoAction;
        }

        action.theRequiresData = 0;
        is_upgrade = true;

        break;
      }
      case MemoryMessage::NonAllocatingStoreReq: {
        DBG_Assert( msg->type() == MemoryMessage::NonAllocatingStoreReply );

        action.theFrontMessage = true;

        msg->reqSize() = original_miss->reqSize();
    msg->address() = original_miss->address();
        msg->pc() = original_miss->pc();
        msg->coreIdx() = original_miss->coreIdx();

        action.theFrontTransport = transport;

        if (msg->ackRequired()) {
          intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::NASAck, msg->address(), msg->pc()));
          reply->ackRequiresData() = false;
          action.theBackMessage = true;
          action.theBackTransport = transport;
          action.theBackTransport.set(MemoryMessageTag, reply);
        }

        break;
      }
      default:
        DBG_Assert( false, ( << " MAF entry has un-expected type. original req: " << (*original_miss) << ", reply: " << (*msg) ));
        break;
    }

    // Some book keeping stuff

    if (original_miss->type() == MemoryMessage::PrefetchReadAllocReq
        ||  (original_tracker->originatorLevel() && (*original_tracker->originatorLevel() == eL2Prefetcher))) {
      is_prefetch = true;
      theTraceTracker.prefetch(theNodeId, theCacheLevel, original_miss->address());
      theTraceTracker.prefetchFill(theNodeId, theCacheLevel, msg->address(), *tracker->fillLevel());
    } else if (is_final) {
      theTraceTracker.access(theNodeId, theCacheLevel, original_miss->address(), original_miss->pc(), result->state().prefetched(), original_miss->isWrite(), !is_upgrade, original_miss->isPriv(), (original_tracker->logicalTimestamp() ? *original_tracker->logicalTimestamp() : 0 ));
    }

    if (is_final) {
      theRequestTracker.endRequest(theArray->getSet(msg->address()));
      DBG_(Trace, ( << " ending Request in set " << theArray->getSet(msg->address()) << " for " << *msg ));
      if (is_upgrade) {
        upg_replies++;
      } else {
        fills++;
      }
    }

    if (is_fill && !is_prefetch) {
      DBG_Assert( tracker->fillLevel(), ( << "Fill Level NOT set in reply to " << *original_miss ));
      theTraceTracker.fill(theNodeId, thePeerLevel, msg->address(), *tracker->fillLevel(), is_fetch, is_write);
    }

    theReadTracker.finishMiss(original_tracker);

    if (result->state() != State::Invalid) {
      if (is_prefetch) {
        result->setPrefetched(true);
      } else {
        result->setPrefetched(false);
      }
    }

    if (action.theAction != kNoAction) {
      action.theFrontMessage = true;
      action.theFrontTransport = transport;
    }
    return action;
  }  // handleBackSideMessage()

  //Action InclusiveMESI::finalizeSnoop(MemoryMessage_p msg, TransactionTracker_p tracker, LookupResult_p result) {
  Action InclusiveMESI::finalizeSnoop(MemoryTransport transport, LookupResult_p result) {

    MemoryMessage_p      msg = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_Assert( msg != nullptr );
    DBG_Assert( tracker != nullptr );

    SnoopBuffer::snoop_iter snp = theSnoopBuffer.findEntry(getBlockAddress(msg->address()));
    DBG_Assert(snp != theSnoopBuffer.end(), ( << "received snoop reply but no waiting snoops: " << *msg));

    EvictBuffer<State>::iterator evEntry( theEvictBuffer.find(msg->address()) );

    // InvUpdateAck or DownUpdateAck need to write data
    bool requires_data = false;
    if ( (msg->type() == MemoryMessage::InvUpdateAck)
         || (msg->type() == MemoryMessage::DownUpdateAck)) {

      // requested block might be in array or in the EB
      if (result->state() == State::Invalid) {
        DBG_Assert(evEntry != theEvictBuffer.end());
        evEntry->state() = State::Modified;
        evEntry->type() = MemoryMessage::EvictDirty;
      } else {
        result->setState(State::Modified);
      }
      requires_data = true;
    }

    if (msg->dstream()) {
      snp->d_snoop_outstanding = false;
    } else {
      snp->i_snoop_outstanding = false;
    }

    // Do we have another snoop reply outstanding?
    if (snp->d_snoop_outstanding || snp->i_snoop_outstanding) {
      DBG_(Trace, ( << "reply received, but still waiting for other snoop (D|I) = (" << std::boolalpha << snp->d_snoop_outstanding << "|" << snp->i_snoop_outstanding << "): " << *snp->transport[MemoryMessageTag] ));
      return Action(kNoAction, tracker, requires_data);
    }

    MemoryMessage_p orig_msg = snp->transport[MemoryMessageTag];
    DBG_Assert( snp->transport[MemoryMessageTag] != nullptr );
    DBG_Assert( snp->transport[TransactionTrackerTag] != nullptr );

    // Determine proper response based on current cache/EB state
    State block_state(State::Invalid);
    if (result->state() != State::Invalid) {
      block_state = (result->state());
    } else {
      block_state = (evEntry->state());
    }

    bool wake_evicts = false;

    switch (orig_msg->type()) {
      case MemoryMessage::ReadFwd:
      case MemoryMessage::FetchFwd:
        if ((block_state == State::Modified)) {
          orig_msg->type() = MemoryMessage::FwdReplyOwned;
          if (theSnoopLRU && result->state() != State::Invalid) {
            theArray->invalidateBlock(result);
          }
        } else {
          orig_msg->type() = MemoryMessage::FwdReply;
        }
        tracker->setNetworkTrafficRequired(true);
        tracker->setResponder(theNodeId);
        if (!tracker->fillLevel()) {
          tracker->setFillLevel(thePeerLevel);
          if (block_state == State::Modified) {
            tracker->setPreviousState(eModified);
          } else {
            tracker->setPreviousState(eShared);
          }
        }
        if (result->state() != State::Invalid) {
          result->setState(State::Shared);
        } else {
          evEntry->state() = State::Shared;
          evEntry->type() = MemoryMessage::EvictClean;
        }

        // We either got data back that we need to write, or we need to read it
        requires_data = true;
        break;

      case MemoryMessage::WriteFwd:
        // If we got dirty data, then we don't need to access the data array
        requires_data = !requires_data;

      case MemoryMessage::Invalidate:
      case MemoryMessage::BackInvalidate:

        if ((block_state == State::Modified)) {
          if (orig_msg->type() == MemoryMessage::BackInvalidate) {
            orig_msg->type() = MemoryMessage::InvUpdateAck;
            if (!tracker->previousState()) {
              if (block_state == State::Modified) {
                tracker->setPreviousState(eModified);
              } else {
                tracker->setPreviousState(eShared);
              }
            }
            requires_data = true;
          } else if (orig_msg->type() == MemoryMessage::Invalidate) {
            orig_msg->type() = MemoryMessage::InvalidateAck;
            requires_data = false;
            if (!tracker->fillLevel()) {
              tracker->setFillLevel(thePeerLevel);
              if (block_state == State::Modified) {
                tracker->setPreviousState(eModified);
              } else {
                tracker->setPreviousState(eShared);
              }
            }
            if (!tracker->fillType()) {
              tracker->setFillType(eCoherence);
            }
          } else {
            orig_msg->type() = MemoryMessage::FwdReplyOwned;
            tracker->setNetworkTrafficRequired(true);
            tracker->setResponder(theNodeId);
            if (!tracker->fillLevel()) {
              tracker->setFillLevel(thePeerLevel);
              if (block_state == State::Modified) {
                tracker->setPreviousState(eModified);
              } else {
                tracker->setPreviousState(eShared);
              }
            }
            requires_data = true;
          }
        } else if (block_state == State::Exclusive) {
          if (orig_msg->type() == MemoryMessage::BackInvalidate) {
            orig_msg->type() = MemoryMessage::InvUpdateAck;
            requires_data = true;
            if (!tracker->previousState()) {
              if (block_state == State::Modified) {
                tracker->setPreviousState(eModified);
              } else {
                tracker->setPreviousState(eShared);
              }
            }
          } else if (orig_msg->type() == MemoryMessage::WriteFwd) {
            orig_msg->type() = MemoryMessage::FwdReplyWritable;
            tracker->setNetworkTrafficRequired(true);
            tracker->setResponder(theNodeId);
            if (!tracker->fillLevel()) {
              tracker->setFillLevel(thePeerLevel);
              if (block_state == State::Modified) {
                tracker->setPreviousState(eModified);
              } else {
                tracker->setPreviousState(eShared);
              }
            }
            requires_data = true;
          } else {
            orig_msg->type() = MemoryMessage::InvalidateAck;
            if (!tracker->fillLevel()) {
              tracker->setFillLevel(thePeerLevel);
              if (block_state == State::Modified) {
                tracker->setPreviousState(eModified);
              } else {
                tracker->setPreviousState(eShared);
              }
            }
            if (!tracker->fillType()) {
              tracker->setFillType(eCoherence);
            }
          }
        } else {
          if (orig_msg->type() == MemoryMessage::WriteFwd) {
            orig_msg->type() = MemoryMessage::FwdReplyWritable;
            tracker->setNetworkTrafficRequired(true);
            tracker->setResponder(theNodeId);
            tracker->setFillLevel(theCacheLevel);
            requires_data = true;
          } else {
            orig_msg->type() = MemoryMessage::InvalidateAck;
            if (!tracker->fillType()) {
              tracker->setFillType(eCoherence);
            }
          }
        }
        if (result->state() != State::Invalid) {
          result->setState(State::Invalid);
          theArray->invalidateBlock(result);
        } else {
          DBG_Assert( evEntry != theEvictBuffer.end());

          // if we've already sent an Evict to the next level, don't do anything
          // any new requests will stall until the evict completes
          // and the next level will ensure we don't get any other snoops
          if (!evEntry->pending()) {
            // We haven't sent an evict yet, just clear the block from the evict buffer

            // the block is in the evict buffer
            // but it's invalid, so just get rid of the evict buffer entry
            theEvictBuffer.remove(evEntry);
            evEntry = theEvictBuffer.end();

            // Look for waiting entries in the snoop buffer that belong to this evict
            // We don't have to worry about snoops already in transit because snoops are serialized and we're
            // in the process of finalizing a snoop (therefore no other snoops are in progress).
            SnoopBuffer::snoop_iter iter, end;
            std::tie(iter, end) = theSnoopBuffer.getWaitingEntries(msg->address());
            for (; iter != end; iter++) {
              if (iter->transport[MemoryMessageTag]->isEvictType()) {
                theSnoopBuffer.remove(iter);
                // There should only be one evict in the snoop buffer
                break;
              }
            }
          }
        }
        break;

      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictDirty:
        if (block_state == State::Modified) {
          orig_msg->type() = MemoryMessage::EvictDirty;
        }
        evEntry->setEvictable(true);
        break;
      default:
        DBG_Assert(false, ( << "Unknown message type in snoop buffer: " << (*orig_msg) ));
        break;
    }

    // Look for waiting entries;
    /*bool woke_snoops = */ theSnoopBuffer.wakeWaitingEntries(msg->address());

    Action action(kSend, tracker, requires_data);

    // Evicts stay in the evict buffer until we need the space
    if (orig_msg->isEvictType()) {
      action.theAction = kNoAction;
    }

    // If we didn't find any snoops to wake-up, then look for MAF entries waiting.
    //action.theWakeSnoops = !woke_snoops;
    action.theWakeSnoops = true; // Always true, races are handled in the rest of the code
    // and we need to make sure MAFs are woken eventually

// I don't think this has any effect since WakeSnoops is going to wake the same messages and its always true
    if (wake_evicts) {
      action.theWakeEvicts = true;
    }

    action.theBackMessage = true;
    action.theBackTransport = snp->transport;

    DBG_(Trace, ( << "Removing Snoop Buffer entry after receiving final reply: " << *snp->transport[MemoryMessageTag] ));
    // Now remove the snoop buffer entry
    theSnoopBuffer.remove(snp);

    return action;
  }

  // Handles a Snoop process from the front side snoop channel
  //Action InclusiveMESI::handleSnoopMessage(MemoryMessage_p msg, TransactionTracker_p tracker) {
  Action InclusiveMESI::handleSnoopMessage(MemoryTransport transport) {

    MemoryMessage_p      msg = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Trace, ( << "Snoop message: " << *msg ) );
    DBG_Assert(msg);
    DBG_Assert(msg->usesSnoopChannel());

    accesses++;

    LookupResult_p result = (*theArray)[msg->address()];

    switch (msg->type()) {
        //Snoops
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::DowngradeAck:
      case MemoryMessage::DownUpdateAck:
        snoops++;
        return finalizeSnoop(transport, result);

        //Allocations
      case MemoryMessage::EvictDirty:
        // We ignore the snoop buffer
        // it will do another cache lookup at this level before it completes
        // and will determine the correct state at that time.

        // need to check evict buffer
        if (result->state() == State::Invalid) {
          EvictBuffer<State>::iterator evEntry( theEvictBuffer.find(msg->address()) );
          if (evEntry != theEvictBuffer.end()) {
            DBG_Assert(evEntry->type() != MemoryMessage::EvictDirty);
            // Might as well leave it in the evict buffer, just change the
            // evict type to EvictDirty
            evEntry->type() = MemoryMessage::EvictDirty;
            evEntry->state() = State::Modified;
          }
        } else {
          DBG_Assert(result->state() == State::Exclusive, ( << "received dirty evict for block in " << result->state() << " state" ));
          result->setState(State::Modified);
          theArray->recordAccess(result);
        }
        return Action(kNoAction, tracker, true);

        // No clean evicts
        // The L1 can send these in rare cases where the state changes while a block
        // is in the evict buffer (ie. it receives a downgrade)
        // Rather than change the L1, we just drop clean evicts here
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
        return Action(kNoAction, tracker); //Suppress compiler warning

        //Flushes
        // I don't know where these come from or exactly how to handle them
        // I know they don't come from FLUSH instructions, so for now, I'm going to ignore them
      case MemoryMessage::FlushReq:
      case MemoryMessage::Flush:

        // We only have IProbes, these are caught by the Controller and passed to handleIprobe()
      case MemoryMessage::ProbedNotPresent:
      case MemoryMessage::ProbedClean:
      case MemoryMessage::ProbedWritable:
      case MemoryMessage::ProbedDirty:

        // No PrefetchInserts
        // Support can be added as required
      case MemoryMessage::PrefetchInsert:
      case MemoryMessage::PrefetchInsertWritable:
      case MemoryMessage::ReturnReply:
        //Anything else is an error
      default:
        DBG_Assert( false,  ( << "Invalid message processed in handleSnoopMessage: " << *msg) );
        return Action(kNoAction, tracker); //Suppress compiler warning
    }
  }

  // This handles a probe that was generated as a result of a Fetch miss
  // at this cache level.
  //Action InclusiveMESI::handleIprobe(bool aHit, MemoryMessage_p fetchReq, TransactionTracker_p tracker) {
  Action InclusiveMESI::handleIprobe(bool aHit, MemoryTransport transport) {

    MemoryMessage_p      fetchReq = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    accesses++;
    iprobes++;

    LookupResult_p lookup = (*theArray)[fetchReq->address()];

    // We only really sent the probe to get dirty data
    // This will be a little bit sketchy, but whatever
    DBG_Assert( lookup->state() != State::Invalid );

    // We no longer need to protect the block
    lookup->setProtected(false);

    theRequestTracker.endRequest(theArray->getSet(fetchReq->address()));
    DBG_(Trace, ( << " ending request for " << *fetchReq ));

    // If a snoop came in while we were waiting, then let it proceed and we'll wait
    if (theSnoopBuffer.hasSnoopsOutstanding(fetchReq->address())) {
      Action action(kInsertMAF_WaitSnoop, tracker);
      return action;
    }

    fetchReq->type() = MemoryMessage::FetchReply;
    if (tracker && !tracker->fillLevel()) {
      tracker->setNetworkTrafficRequired(false);
      tracker->setResponder(theNodeId);
      tracker->setFillLevel(theCacheLevel);
    }
    hits++;
    if (tracker->OS() && *tracker->OS()) {
      hits_system_I++;
    } else {
      hits_user_I++;
    }

    Action action(kReplyAndRemoveMAF, tracker, 1);
    action.theFrontMessage = true;
    action.theFrontTransport = transport;
    // Make sure the reply goes to the right cache
    action.theFrontToDCache = false;
    action.theFrontToICache = true;
    return action;
  }

  // Idle Work for us means evictions that require snoops
  bool InclusiveMESI::idleWorkAvailable() {
    if (theController->isFrontSideOutFull()) {
      return false;
    }
    if (theSnoopBuffer.full()) {
      return false;
    }
    bool ret = !theEvictBuffer.empty() && theEvictBuffer.back().theSnoopRequired;
    ret = ret || (!theEvictBuffer.full() && theArray->evictionResourcePressure());

    return ret;
  }

  MemoryMessage_p InclusiveMESI::getIdleWorkMessage(ProcessEntry_p process) {
    // Reserve a snoop buffer
    theController->reserveSnoopBuffer(process);
    theController->reserveFrontSideOut(process);

    // Find the oldest entry in the Evict Buffer that requires evictions
    EvictBuffer<State>::iterator iter = theEvictBuffer.getOldestRequiringSnoops();

    // We might be doing idle work just to remove something from the array
    if (iter == theEvictBuffer.end()) {
      return MemoryMessage_p(new MemoryMessage(MemoryMessage::NumMemoryMessageTypes, MemoryAddress(0)));
    }

    iter->theSnoopRequired = false;
    MemoryMessage_p msg(new MemoryMessage(iter->theType, iter->theBlockAddress));
    msg->reqSize() = 0;
    return msg;
  }

  void InclusiveMESI::removeIdleWorkReservations(ProcessEntry_p process, Action & action) {
    theController->unreserveSnoopBuffer(process);

    if (action.theAction == kNoAction) {
      theController->unreserveFrontSideOut(process);
    }
  }

  //Action InclusiveMESI::handleIdleWork(MemoryMessage_p msg, TransactionTracker_p tracker) {
  Action InclusiveMESI::handleIdleWork(MemoryTransport transport) {

    MemoryMessage_p      msg = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    if (!theEvictBuffer.full() && theArray->evictionResourcePressure()) {
      // Move a block from the Array to the Evict Buffer
      std::pair<State, MemoryAddress> victim = theArray->getPreemptiveEviction();
      MemoryMessage::MemoryMessageType evict_type = MemoryMessage::EvictClean;

      if (victim.first != State::Invalid) {
        if ((victim.first == State::Modified)) {
          evict_type = MemoryMessage::EvictDirty;
        } else if (victim.first == State::Exclusive) {
          evict_type = MemoryMessage::EvictWritable;
        }
        EvictBuffer<State>::iterator iter = theEvictBuffer.allocEntry(victim.second, evict_type, victim.first, false);
        theTraceTracker.eviction(theNodeId, theCacheLevel, victim.second, false);
        DBG_(Trace, ( << " Adding " << std::hex << victim.second << " to the EvictBuffer." ));

        if (msg->type() == MemoryMessage::NumMemoryMessageTypes) {
          msg->type() = evict_type;
          msg->address() = victim.second;
          iter->theSnoopRequired = false;
          DBG_(Trace, ( << "Getting Idle work: evicting " << std::hex << iter->theBlockAddress ));
        }
      }
    }

    // We send a phony message if we want to evict blocks from the array
    // Double check if we added an evictable block
    if (msg->type() == MemoryMessage::NumMemoryMessageTypes) {
      Action action(kNoAction, tracker, false);
      action.theRememberSnoopTransport = false;
      return action;
    }

    EvictBuffer<State>::iterator iter = theEvictBuffer.find(msg->address());
    // There is a race condition where a new request hits on the EB entry and puts it into the cache
    // between the time that the idleWork processes is created and the time we get here.
    // To handle this, we just return an empty action if we can't find the block in question
    if (iter == theEvictBuffer.end()) {
      DBG_(Trace, ( << "Failed to find " << std::hex << msg->address() << " in the evict buffer, skipping idle work." ));
      Action action(kNoAction, tracker, false);
      action.theRememberSnoopTransport = false;
      return action;
    }
    DBG_Assert( iter != theEvictBuffer.end());

    iter->theSnoopScheduled = true;

    if (!theSnoopBuffer.hasEntry(msg->address())) {
      SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
      snp->i_snoop_outstanding = (theCacheLevel != eL1);

      Action act(kSend, tracker, false);
      intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
      // Need to make sure reqSize is 0 so we don't get InvUpdateAck when we shouldn't
      request->reqSize() = 0;
      act.theFrontMessage = true;
      act.theFrontTransport = transport;
      act.theFrontTransport.set(MemoryMessageTag, request);
      if ((theCacheLevel != eL1) && iter->state() == State::Modified) {
        act.theFrontToDCache = false;
        snp->d_snoop_outstanding = false;
      } else {
        act.theFrontToDCache = true;
        snp->d_snoop_outstanding = true; // This is redundant, only here for clarity
      }
      act.theFrontToICache = (theCacheLevel != eL1);

      return act;
    } else {
      theSnoopBuffer.addWaitingEntry(transport);
      return Action(kNoAction, tracker, false);
    }
  }

  // Evicts are the same as IdleWork above, all other messages are BackMessages and handled the same way
  //Action InclusiveMESI::handleWakeSnoop ( MemoryMessage_p      msg,
  //                TransactionTracker_p tracker ) {
  Action InclusiveMESI::handleWakeSnoop ( MemoryTransport transport ) {

    MemoryMessage_p      msg = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_( Trace, (  << " Handle WakeSnoop: " << *msg) );
    if (msg->isEvictType()) {
      SnoopBuffer::snoop_iter snp = theSnoopBuffer.findEntry(msg->address());
      DBG_Assert(snp == theSnoopBuffer.end());
      snp = theSnoopBuffer.allocEntry(transport);

      Action act(kSend, tracker, false);
      intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address(), msg->pc()));
      request->reqSize() = 0;
      request->coreIdx() = msg->coreIdx();
      act.theFrontMessage = true;
      act.theFrontTransport = transport;
      act.theFrontTransport.set(MemoryMessageTag, request);

      EvictBuffer<State>::iterator iter = theEvictBuffer.find(msg->address());
      if (theCacheLevel != eL1 && iter->state() == State::Modified) {
        act.theFrontToDCache = false;
        snp->d_snoop_outstanding = false;
      } else {
        act.theFrontToDCache = true;
        snp->d_snoop_outstanding = true; // This is redundant, only here for clarity
      }
      act.theFrontToICache = (theCacheLevel != eL1);
      snp->i_snoop_outstanding = (theCacheLevel != eL1);
      return act;
    } else {
      return handleBackMessage(transport);
    }
  }

  uint32_t InclusiveMESI::freeEvictBuffer() const {
    int32_t ret = (theEvictBuffer.freeEntries() + thePendingEvicts) - theEvictThreshold;
    if (ret < 0) {
      return 0;
    } else {
      return ret;
    }
  }

  bool InclusiveMESI::evictableBlockExists(int32_t anIndex) const {
    // Limit 4 pending evicts
    if ((thePendingEvicts + anIndex) >= theEvictThreshold) {
      return false;
    }
    return const_EvictBuffer().headEvictable(anIndex + thePendingEvicts);
  }

  Action InclusiveMESI::doEviction() {
    Action action(kSend);

    if (evictBuffer().headEvictable(thePendingEvicts)) {
      MemoryMessage_p msg( theEvictBuffer.pop(thePendingEvicts) );
      msg->reqSize() = theBlockSize;

      DBG_(Trace, ( << " Queuing eviction " << *msg ) );

      if (msg->type() == MemoryMessage::EvictDirty) {
        msg->evictHasData() = true;
        evicts_dirty++;
        evicts_with_data++;
      } else if (msg->type() == MemoryMessage::EvictWritable && theInit->theWritableEvictsHaveData) {
        msg->evictHasData() = true;
        evicts_with_data++;
        evicts_clean++;
      } else {
        evicts_clean++;
      }

      //theTraceTracker.eviction(theNodeId, theCacheLevel, msg->address(), false);

      //Create a transaction tracker for the eviction
      TransactionTracker_p tracker = new TransactionTracker;
      tracker->setAddress(msg->address());
      tracker->setInitiator(theNodeId);
      tracker->setSource(theName + " Evict");
      tracker->setDelayCause(theName, "Evict");

      action.theBackMessage = true;
      action.theBackTransport.set(MemoryMessageTag, msg);
      action.theBackTransport.set(TransactionTrackerTag, tracker);
      action.theOutputTracker = tracker;
      action.theRequiresData = false;

      thePendingEvicts++;

      if (!theEvictAcksRequired) {
        EvictBuffer<State>::iterator evictee = theEvictBuffer.find(msg->address());
        theEvictBuffer.remove(evictee);
        thePendingEvicts--;
      }
    } else {
      action.theAction = kNoAction;
    }

    return action;
  }

  void InclusiveMESI::dumpEvictBuffer() const {
    BaseCacheControllerImpl::dumpEvictBuffer();
    DBG_(VVerb, ( << " Pending Evicts = " << thePendingEvicts ));

    EvictBuffer<State>::const_iterator iter = theEvictBuffer.begin();
    for (int32_t i = 0; iter != theEvictBuffer.end(); i++, iter++) {
      DBG_(VVerb, ( << " EB[" << i << "] = " << *iter ));
    }
  }

}  // end namespace nCache
