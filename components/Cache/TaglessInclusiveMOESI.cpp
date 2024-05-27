//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

/*! \file CacheController.cpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 */

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <components/CommonQEMU/GlobalHasher.hpp>
#include <components/CommonQEMU/TraceTracker.hpp>
#include <core/metaprogram.hpp>
#include <deque>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace boost::multi_index;

#include <core/performance/profile.hpp>
#include <core/stats.hpp>
#include <core/types.hpp>

using namespace Flexus;

#include "AbstractArray.hpp"
#include "CacheBuffers.hpp"
#include "MissTracker.hpp"
#include "TaglessInclusiveMOESI.hpp"

#define DBG_DeclareCategories Cache
#define DBG_SetDefaultOps     AddCat(Cache) Set((CompName) << theName) Set((CompIdx) << theNodeId)
#include DBG_Control()

namespace nCache {

namespace Stat = Flexus::Stat;

////////////////////////////////////////////////////////////////////////////////
// TaglessInclusiveMOESIControllerImpl class

// Register the Implementation so that it can be created automagically
REGISTER_CONTROLLER_IMPLEMENTATION(TaglessInclusiveMOESIControllerImpl, "TaglessInclusiveMOESI");

// The automagic creation process calls this function
// The args should be empty, and the params contain a pointer to the controller
// and the init info.
BaseCacheControllerImpl*
TaglessInclusiveMOESIControllerImpl::createInstance(std::list<std::pair<std::string, std::string>>& args,
                                                    const ControllerParams& params)
{
    bool anAlwaysNAck = false;
    while (!args.empty()) {
        if (strcasecmp(args.front().first.c_str(), "AlwaysNAck") == 0) {
            if ((strcasecmp(args.front().second.c_str(), "true") == 0) ||
                (strcasecmp(args.front().second.c_str(), "true") == 0)) {
                anAlwaysNAck = true;
            } else {
                anAlwaysNAck = false;
            }
        } else {
            DBG_Assert(
              false,
              NoDefaultOps()(<< "Unknown argument to InclusiveMOESI controller: '" << args.front().first << "'"));
        }
        args.pop_front();
    }
    DBG_Assert(args.empty(),
               NoDefaultOps()(<< "Tried to create InclusiveMOESI controller with "
                                 "invalid arguments"));
    return new TaglessInclusiveMOESIControllerImpl(params.first, params.second, anAlwaysNAck);
}

TaglessInclusiveMOESIControllerImpl::TaglessInclusiveMOESIControllerImpl(CacheController* aController,
                                                                         CacheInitInfo* anInit,
                                                                         bool aNAckAlways)
  : BaseCacheControllerImpl(aController, anInit)
  , theEvictBuffer(anInit->theEBSize)
  , theNAckAlways(aNAckAlways)
  , thePendingEvicts(0)
  , theEvictThreshold(4)
{
    DBG_Assert(anInit->theCores == 1,
               (<< "InclusiveMOESI cache does not support cfg.Cores == " << anInit->theCores
                << " (only supports 1 right now!)"));
    DBG_Assert(((anInit->theCacheLevel != eL1) && (anInit->theCacheLevel != eL1I)),
               (<< "InclusiveMOESI cache does not work as L1 cache!"));

    // We need to fix this to make it use a template and our state
    theArray = constructArray<State, State::Invalid>(anInit->theArrayConfiguration,
                                                     anInit->theName,
                                                     anInit->theNodeId,
                                                     anInit->theBlockSize);
}

// Perform lookup, select action and update cache state if necessary
std::tuple<bool, bool, Action>
TaglessInclusiveMOESIControllerImpl::doRequest(MemoryTransport transport,
                                               bool has_maf_entry,
                                               TransactionTracker_p waking_tracker)
{
    bool is_write       = false;
    bool is_upgrade     = false;
    bool is_miss        = false;
    bool is_hit         = false;
    bool was_prefetched = false;

    Action action(kReplyAndRemoveMAF, transport[TransactionTrackerTag], 1);
    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];
    MemoryMessage_p request;

    LookupResult_p lookup = (*theArray)[msg->address()];

    DBG_(Iface, (<< " Do Request: " << *msg));

    // For now, we stall ALL requests if we have an outstanding request for this
    // block
    if (has_maf_entry) {
        // DBG_(Trace, ( << " Stalling request while we have a MAF entry, block
        // state is " << lookup->state() << " : " << (*msg) ));
        return std::make_tuple(false, false, Action(kInsertMAF_WaitAddress, tracker));
    }

    // If this is a miss, then look in the Evict buffer
    if (lookup->state() == State::Invalid) {
        EvictBuffer<State>::iterator evictee = theEvictBuffer.find(getBlockAddress(msg->address()));
        bool evictee_write                   = false;
        if (evictee != theEvictBuffer.end()) {
            // If we're in the process of evicting the block, wait for the evict to
            // complete to avoid any races
            if (evictee->pending()) { return std::make_tuple(false, false, Action(kInsertMAF_WaitEvict, tracker)); }

            State block_state = evictee->state();

            LookupResult_p victim = theArray->allocate(lookup, getBlockAddress(msg->address()));

            // Remove block from the evict buffer
            theEvictBuffer.remove(evictee);

            // Keep state from EB.
            lookup->setState(block_state);

            DBG_(Trace, (<< " replacing EB entry, evicting block in state " << victim->state() << " : " << *msg));
            if (victim->state() != State::Invalid) { evictee_write = evictBlock(victim); }
            theTraceTracker
              .fill(theNodeId, theCacheLevel, getBlockAddress(msg->address()), theCacheLevel, false, evictee_write);
        }
    }

    if (lookup->state() != State::Invalid) {
        is_hit         = true;
        was_prefetched = lookup->state().prefetched();
    }

    // Check the Snoop Buffer
    // First, look for any active snoops
    SnoopBuffer::snoop_iter iter = theSnoopBuffer.getActiveEntry(getBlockAddress(msg->address()));

    // bool snoop_inval_pending = false;
    if (iter != theSnoopBuffer.end()) {
        return std::make_tuple(false, false, Action(kInsertMAF_WaitSnoop, tracker));
    } else if (theSnoopBuffer.hasEntry(getBlockAddress(msg->address()))) {
        return std::make_tuple(false, false, Action(kInsertMAF_WaitSnoop, tracker));
    }

    if (msg->type() == MemoryMessage::UpgradeReq && lookup->state() == State::Invalid) {
        // Upgrade must have been over-taken by an invalidate while waiting in the
        // buffer Convert to a WriteReq and continue
        msg->type() = MemoryMessage::WriteReq;
    }

    // handle each operation slightly differently
    switch (msg->type()) {

        case MemoryMessage::NonAllocatingStoreReq:
            is_write = true;

            if (!(lookup->state() == State::Modified) && !(lookup->state() == State::Exclusive)) {
                is_miss                = true;
                action.theAction       = kSend;
                action.theRequiresData = 0;
            } else {
                is_hit = true;
                theArray->recordAccess(lookup);
                msg->type() = MemoryMessage::NonAllocatingStoreReply;
            }
            break;

        case MemoryMessage::FetchReq:
            // IF we have the block in Exclusive state we need to probe the DCache to
            // see if it has a modified version. Otherwise, we don't need to probe,
            // because we're an Inclusive Cache
            if (lookup->state() == State::Exclusive) {
                request                  = new MemoryMessage(MemoryMessage::Probe, getBlockAddress(msg->address()));
                action.theAction         = kInsertMAF_WaitProbe;
                action.theRequiresData   = 0;
                action.theFrontMessage   = true;
                action.theFrontTransport = transport;
                action.theFrontTransport.set(MemoryMessageTag, request);
                request->reqSize() = 0;
                // Protect the block so it doesn't get evicted while we're waiting for the
                // probe
                lookup->setProtected(true);
                is_hit = false;
            } else if (lookup->state() == State::Invalid) {
                action.theAction = kInsertMAF_WaitResponse;
                is_miss          = true;
            } else { // In MOS states we record the access (ie. update LRU) and send a
                     // reply
                is_hit = true;
                theArray->recordAccess(lookup);
                msg->type() = MemoryMessage::FetchReply;
                // Make sure the reply goes to the right cache
                action.theFrontToDCache = false;
                action.theFrontToICache = true;
            }
            break;

        case MemoryMessage::ReadReq:
            if (lookup->state() == State::Invalid) {
                action.theAction = kInsertMAF_WaitResponse;
                is_miss          = true;
            } else if (lookup->state() == State::Modified) {
                lookup->setState(State::Exclusive);
                theArray->recordAccess(lookup);
                msg->type() = MemoryMessage::MissReplyDirty;
                is_hit      = true;
            } else if (lookup->state() == State::Exclusive) {
                theArray->recordAccess(lookup);
                msg->type() = MemoryMessage::MissReplyWritable;
                is_hit      = true;
            } else { // Owned or Shared, no state change
                theArray->recordAccess(lookup);
                msg->type() = MemoryMessage::MissReply;
                is_hit      = true;
            }
            if (lookup->state().prefetched() && is_hit) {
                if (tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher)) {
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
                is_miss          = true;
            } else { // Shared or Owned
                request                 = new MemoryMessage(MemoryMessage::UpgradeReq, getBlockAddress(msg->address()));
                action.theAction        = kInsertMAF_WaitResponse;
                action.theBackMessage   = true;
                action.theBackTransport = transport;
                action.theBackTransport.set(MemoryMessageTag, request);
                is_miss = true;
                upgrades++;
                is_upgrade = true;
                is_hit     = false;

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

        case MemoryMessage::UpgradeReq:
            if (lookup->state() == State::Owned || lookup->state() == State::Shared) {
                action.theAction = kInsertMAF_WaitResponse;
                is_miss          = true;
                is_upgrade       = true;
                upgrades++;
                is_hit = false;

                // Protect this block so it doesn't get evicted
                lookup->setProtected(true);

                if (lookup->state().prefetched()) {
                    theTraceTracker.prefetchHit(theNodeId, theCacheLevel, getBlockAddress(msg->address()), true);
                    prefetchHitsButUpgrade++;
                    lookup->setPrefetched(false);
                }
            } else if (lookup->state() == State::Exclusive) {
                // TODO: find out HOW this happens.
                // Coherence protocol shouldn't allow this, but somehow it's happening.
                DBG_(Dev, (<< " Cache received Upgrade in Exclusive state: " << (*msg)));
                msg->type() = MemoryMessage::UpgradeReply;
                theArray->recordAccess(lookup);
                is_hit     = true;
                is_upgrade = true;
                upgrades++;
            } else {
                DBG_Assert(false,
                           (<< " cache received Upgrade request in state " << lookup->state() << " : " << (*msg)));
            }
            is_write = true;

            break;

        case MemoryMessage::PrefetchReadNoAllocReq:
            action.theRequiresData = 0;
            if (lookup->state() == State::Invalid) {
                action.theAction = kSend;
                is_miss          = true;
            } else {
                msg->type() = MemoryMessage::PrefetchReadRedundant;
            }
            break;

            // Nothing else should reach here
        default: DBG_Assert(false, (<< "Got invalid message: " << *msg));
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
    if (is_upgrade) { tracker->setFillType(eCoherence); }

    switch (action.theAction) {
        case kInsertMAF_WaitResponse:
            action.theRequiresData = 0;
            if (action.theBackMessage == false) {
                action.theBackMessage   = true;
                action.theBackTransport = transport;
            }
            break;

        case kReplyAndRemoveMAF:
            action.theFrontMessage   = true;
            action.theFrontTransport = transport;
            break;

        case kSend:
            action.theBackMessage   = true;
            action.theBackTransport = transport;
            action.theRequiresData  = 0;
            break;

        case kInsertMAF_WaitProbe:
        default: break;
    }

    if (is_miss) {
        // TraceTracker miss
        if (!msg->isPrefetchType()) {
            theTraceTracker.access(theNodeId,
                                   theCacheLevel,
                                   msg->address(),
                                   msg->pc(),
                                   false,
                                   msg->isWrite(),
                                   !is_upgrade,
                                   msg->isPriv(),
                                   (tracker->logicalTimestamp() ? *tracker->logicalTimestamp() : 0));
        }

        misses++;
        if (tracker && tracker->isFetch() && *tracker->isFetch()) {
            if (tracker->OS() && *tracker->OS()) {
                misses_system_I++;
            } else {
                misses_user_I++;
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

bool
TaglessInclusiveMOESIControllerImpl::evictBlock(LookupResult_p victim)
{

    MemoryMessage::MemoryMessageType evict_type = MemoryMessage::EvictClean;

    if (victim->state() == State::Invalid) {
        return false;
    } else if ((victim->state() == State::Modified) || (victim->state() == State::Owned)) {
        evict_type = MemoryMessage::EvictDirty;
    } else if (victim->state() == State::Exclusive) {
        evict_type = MemoryMessage::EvictWritable;
    }

    theEvictBuffer.allocEntry(victim->blockAddress(), evict_type, victim->state(), false);
    theTraceTracker.eviction(theNodeId, theCacheLevel, victim->blockAddress(), false);

    DBG_(Trace, (<< " Adding " << std::hex << victim->blockAddress() << " to the EvictBuffer."));

    return (evict_type == MemoryMessage::EvictDirty);
}

void
TaglessInclusiveMOESIControllerImpl::allocateBlock(LookupResult_p result, MemoryAddress aBlockAddress)
{

    LookupResult_p victim = theArray->allocate(result, aBlockAddress);

    // Move the victim to the victim buffer as necessary
    if (victim->state() != State::Invalid) { evictBlock(victim); }
}

// Handles a MemoryTransport from the back side
// Action TaglessInclusiveMOESIControllerImpl::handleBackMessage(MemoryMessage_p
// msg, TransactionTracker_p tracker) {
Action
TaglessInclusiveMOESIControllerImpl::handleBackMessage(MemoryTransport transport)
{
    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Trace, (<< " Handle BackProcess: " << *msg));

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

        act.theBackMessage   = true;
        act.theBackTransport = transport;

        return act;
    }

    // Do a cache lookup
    LookupResult_p result = (*theArray)[msg->address()];

    // Look for an outstanding request
    MemoryMessage_p original_miss;
    TransactionTracker_p original_tracker;
    std::tie(original_miss, original_tracker) = theController->getWaitingMAFEntry(msg->address());
    bool waiting_maf                          = (original_miss != nullptr);

    State state = result->state();

    EvictBuffer<State>::iterator evictee = theEvictBuffer.end();
    bool from_eb                         = false;
    if (state == State::Invalid) {
        evictee = theEvictBuffer.find(getBlockAddress(msg->address()));
        if (evictee != theEvictBuffer.end()) {
            from_eb = true;
            state   = evictee->state();
        }
    }

    switch (msg->type()) {
            // ---- First, look at Request Types ---- //

            // A Read or Fetch forwarded from a peer cache.
        case MemoryMessage::ReadFwd:
        case MemoryMessage::FetchFwd:

            if (state == State::Invalid) {
                // Send NAcks to the directory
                Action act(kSend, tracker, false);

                msg->type()          = MemoryMessage::FwdNAck;
                act.theBackMessage   = true;
                act.theBackTransport = transport;

                act.theBackTransport[DestinationTag]->type = DestinationMessage::Directory;

                return act;

            } else if (state == State::Shared) {
                Action act(kSend, tracker, true);
                msg->type()          = MemoryMessage::FwdReply;
                act.theBackMessage   = true;
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
                    //            SnoopBuffer::snoop_iter snp =
                    //            theSnoopBuffer.allocEntry(transport);
                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Downgrade, msg->address()));
                    // make sure size is 0 so we don't get un-necessary update
                    request->reqSize()    = 0;
                    act.theFrontMessage   = true;
                    act.theFrontTransport = transport;
                    act.theFrontTransport.set(MemoryMessageTag, request);
                    return act;
                } else {
                    theSnoopBuffer.addWaitingEntry(transport);
                    return Action(kNoAction, tracker, false);
                }
            } else if (state == State::Owned) {
                if (from_eb) {
                    evictee->state() = State::Shared;
                } else {
                    result->setState(State::Shared);
                }
                msg->type() = MemoryMessage::FwdReplyOwned;
                Action act(kSend, tracker, true);
                act.theBackMessage   = true;
                act.theBackTransport = transport;
                tracker->setNetworkTrafficRequired(false);
                tracker->setFillLevel(thePeerLevel);
                tracker->setPreviousState(eShared);
                return act;
            } else if (state == State::Modified) {
                if (from_eb) {
                    evictee->state() = State::Shared;
                } else {
                    result->setState(State::Shared);
                }
                msg->type() = MemoryMessage::FwdReplyOwned;
                Action act(kSend, tracker, true);
                act.theBackMessage   = true;
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
                // We need to send FwdNack to the requester
                Action act(kSend, tracker, false);

                msg->type()                                = MemoryMessage::FwdNAck;
                act.theBackMessage                         = true;
                act.theBackTransport                       = transport;
                act.theBackTransport[DestinationTag]->type = DestinationMessage::Requester;

                return act;
            } else {
                // Check if we have a waiting upgrade, convert it to a Write if we do
                if (waiting_maf && original_miss->type() == MemoryMessage::UpgradeReq) {
                    original_miss->type() = MemoryMessage::WriteReq;
                }

                // First, wait for outstanding Read/Fetch downgrade requests
                if (!theSnoopBuffer.hasEntry(msg->address())) {

                    SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
                    snp->i_snoop_outstanding    = true;
                    // if it's modified it can't be cached in the dcache

                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
                    request->reqSize()    = 0;
                    act.theFrontMessage   = true;
                    act.theFrontTransport = transport;
                    act.theFrontTransport.set(MemoryMessageTag, request);
                    if (result->state() == State::Modified) {
                        act.theFrontToDCache     = false;
                        snp->d_snoop_outstanding = false;
                    } else {
                        act.theFrontToDCache     = true;
                        snp->d_snoop_outstanding = true;
                    }
                    act.theFrontToICache = true;
                    return act;
                } else {
                    theSnoopBuffer.addWaitingEntry(transport);
                    return Action(kNoAction, tracker, false);
                }
            }
            break;

        case MemoryMessage::Invalidate:
            if (state == State::Invalid) {
                // We need to send InvalidateNAck to the Requester
                Action act(kSend, tracker, false);

                msg->type()          = MemoryMessage::InvalidateNAck;
                act.theBackMessage   = true;
                act.theBackTransport = transport;

                return act;
            } else if ((state == State::Exclusive) || (state == State::Modified)) {
                // We should get a WriteFwd request instead of an Invalidate if we're
                // modified or exclusive This is bad DBG_Assert(false, ( << "received
                // Invalidate while in " << state << " state : " << (*msg) )); This can
                // happen with Tagless Directory since we don't know if a block is
                // exclusive or not
                DBG_Assert(!waiting_maf,
                           (<< "Received Invalidate while in Exlcusive state and waiting for: " << (*original_miss)));

                // First, look for outstanding snoops, and wait for them to complete
                if (!theSnoopBuffer.hasEntry(msg->address())) {

                    SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
                    snp->i_snoop_outstanding    = true;
                    snp->d_snoop_outstanding    = true;

                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
                    request->reqSize()    = 0;
                    act.theFrontMessage   = true;
                    act.theFrontTransport = transport;
                    act.theFrontTransport.set(MemoryMessageTag, request);
                    act.theFrontToDCache = true;
                    act.theFrontToICache = true;
                    return act;

                } else {
                    theSnoopBuffer.addWaitingEntry(transport);
                    return Action(kNoAction, tracker, false);
                }
            } else {
                // state == Shared or Owner
                // If we have an outstanind upgrade, change it to a write
                if (waiting_maf && original_miss->type() == MemoryMessage::UpgradeReq) {
                    original_miss->type() = MemoryMessage::WriteReq;
                }
                // Make sure we don't have an outstanding read/fetch because that
                // shouldn't be possibl
                DBG_Assert(
                  !waiting_maf || original_miss->type() == MemoryMessage::WriteReq,
                  (<< "Received invalidate for block in " << state << " state while waiting for: " << *original_miss));

                // First, look for outstanding snoops, and wait for them to complete
                if (!theSnoopBuffer.hasEntry(msg->address())) {

                    SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
                    snp->i_snoop_outstanding    = true;
                    snp->d_snoop_outstanding    = true;

                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
                    request->reqSize()    = 0;
                    act.theFrontMessage   = true;
                    act.theFrontTransport = transport;
                    act.theFrontTransport.set(MemoryMessageTag, request);
                    act.theFrontToDCache = true;
                    act.theFrontToICache = true;
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

                    msg->type()          = MemoryMessage::InvalidateNAck;
                    act.theBackMessage   = true;
                    act.theBackTransport = transport;

                    return act;
                } else {
                    DBG_(Trace,
                         (<< "Received BackInval for block in invalid state. "
                             "Droping backinval: "
                          << *msg));
                    return Action(kNoAction, tracker, false);
                }
            } else {
                // If there's a request in the MAF then it's an upgrade
                // The Other request has been ordered before our upgrade
                // so we change the upgrade to a write request
                if (waiting_maf) {
                    DBG_Assert(original_miss->type() == MemoryMessage::UpgradeReq && state != State::Modified);
                    original_miss->type() = MemoryMessage::WriteReq;
                }

                // First, look for outstanding snoops, and wait for them to complete
                if (!theSnoopBuffer.hasEntry(msg->address())) {

                    SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
                    snp->i_snoop_outstanding    = true;
                    snp->d_snoop_outstanding    = (state != State::Modified);

                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
                    request->reqSize()    = 0;
                    act.theFrontMessage   = true;
                    act.theFrontTransport = transport;
                    act.theFrontTransport.set(MemoryMessageTag, request);
                    act.theFrontToICache = true;
                    act.theFrontToDCache = (state != State::Modified);
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

            // Need to remove the block from the evict buffer and wake any requests
            // waiting on the evict
            theEvictBuffer.remove(evictee);

            Action act(kNoAction, tracker, false);
            act.theWakeEvicts = true;

            thePendingEvicts--;

            return act;
        }

        // ---- Reply Types are all pretty similar, we'll handle most of them
        // separately below ---- //
        case MemoryMessage::FwdReply:
        case MemoryMessage::FwdReplyOwned:
        case MemoryMessage::FwdReplyWritable:
        case MemoryMessage::FwdReplyDirty:
        case MemoryMessage::InvalidateAck:
        case MemoryMessage::InvalidateNAck:
        case MemoryMessage::InvUpdateAck:
        case MemoryMessage::FetchReply:
        case MemoryMessage::UpgradeReply:
        case MemoryMessage::MissReply:
        case MemoryMessage::MissReplyWritable:
        case MemoryMessage::FwdNAck:
            break;

            // Prefetch replies are just sent up the hierarchy
            // This will really mess with our inclusion if they get inserted into L1s,
            // but ignore this for now
        case MemoryMessage::PrefetchReadReply:
        case MemoryMessage::PrefetchWritableReply:
        case MemoryMessage::PrefetchReadRedundant: {
            Action act(kSend, tracker, false);
            act.theFrontMessage   = true;
            act.theFrontTransport = transport;
            return act;
        }
        case MemoryMessage::NonAllocatingStoreReply: {
            Action act(kSend, tracker, false);
            act.theFrontMessage   = true;
            act.theFrontTransport = transport;
            intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::NASAck, msg->address()));
            act.theBackMessage   = true;
            act.theBackTransport = transport;
            act.theBackTransport.set(MemoryMessageTag, reply);
            return act;
        }

        // MissNotify -> tells us how many invalidates we should wait for
        case MemoryMessage::MissNotify: {
            DBG_Assert(waiting_maf, (<< " received miss notify with no outstanding request : " << (*msg)));
            DBG_Assert(original_miss->type() == MemoryMessage::WriteReq ||
                         original_miss->type() == MemoryMessage::UpgradeReq,
                       (<< " received MissNotify while wrong type of message was "
                           "waiting. orig msg : "
                        << (*msg) << ", reply : " << (*msg)));

            MissAddressFile::maf_iter maf_entry = theController->getWaitingMAFEntryIter(msg->address());
            maf_entry->outstanding_msgs += msg->outstandingMsgs();

            // It's possible we've already received all the other msgs BEFORE we get
            // this notify in this case the # of outstanding msgs should now be zero and
            // we can send our replies
            if (maf_entry->outstanding_msgs == 0) {
                bool sent_upgrade = (original_miss->type() == MemoryMessage::UpgradeReq);
                if (!maf_entry->data_received && result->state() != State::Invalid) {
                    // DBG_Assert(MemoryMessage::WriteReq); //TODO FIXME
                    sent_upgrade = true;
                }
                if (sent_upgrade) {
                    maf_entry->data_received = true;
                    tracker->setNetworkTrafficRequired(true);
                    tracker->setResponder(theNodeId);
                    tracker->setFillLevel(theCacheLevel);
                }

                // If we didn't receive any data, then we need to send a retry to the
                // directory The directory will then forward that to memory
                if (!maf_entry->data_received) {
                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> retry(new MemoryMessage(MemoryMessage::WriteRetry, msg->address()));
                    act.theBackMessage   = true;
                    act.theBackTransport = transport;
                    act.theBackTransport.set(MemoryMessageTag, retry);
                    act.theBackTransport[DestinationTag]->type = DestinationMessage::Directory;
                    // One more reply to wait for
                    maf_entry->outstanding_msgs++;
                    return act;
                }

                DBG_Assert(!from_eb, (<< "received miss notify but data in evict buffer."));
                Action action(kReplyAndRemoveResponseMAF, tracker, true);
                if (result->state() == State::Modified || result->state() == State::Owned) {
                    msg->type() = MemoryMessage::MissReplyDirty;
                } else {
                    DBG_Assert(result->state() != State::Invalid);
                    msg->type() = MemoryMessage::MissReplyWritable;
                }
                if (original_miss->type() == MemoryMessage::UpgradeReq) { msg->type() = MemoryMessage::UpgradeReply; }
                msg->reqSize() = original_miss->reqSize();
                result->setState(State::Exclusive);

                intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::WriteAck, msg->address()));
                if (sent_upgrade) { reply->type() = MemoryMessage::UpgradeAck; }
                action.theBackMessage   = true;
                action.theBackTransport = transport;
                action.theBackTransport.set(MemoryMessageTag, reply);
                action.theBackTransport.set(TaglessDirMsgTag, maf_entry->transport[TaglessDirMsgTag]);
                if (action.theBackTransport[TaglessDirMsgTag] == nullptr) {
                    action.theBackTransport.set(TaglessDirMsgTag, TaglessDirMsg_p(new TaglessDirMsg()));
                }
                action.theFrontMessage   = true;
                action.theFrontTransport = transport;
                return action;
            }

            return Action(kNoAction, tracker);
        }
        default: DBG_Assert(false, (<< "Received unexpected message type on back side : " << (*msg))); break;
    }

    bool is_fill     = false;
    bool is_prefetch = false;
    bool is_write    = false;
    bool is_upgrade  = false;
    bool is_final    = false;
    bool is_fetch    = false;

    // If we get here, then we're servicing a reply msg
    // This means we should have an outstanding miss for this address, otherwise,
    // we're in trouble
    DBG_Assert(original_miss != nullptr, (<< "received reply with no corresponding miss : " << (*msg)));

    // Most of our actions depend on the type of the original message, so use that
    // for the high-level swich

    Action action(kReplyAndRemoveResponseMAF, tracker, true);

    switch (original_miss->type()) {
        case MemoryMessage::FetchReq: {
            DBG_Assert(state == State::Invalid,
                       (<< "Received reply to fetch req but block not invalid - " << state << " : " << (*msg)));

            // We need to allocate the block in the data array
            allocateBlock(result, msg->address());

            switch (msg->type()) {
                case MemoryMessage::FetchReply:
                case MemoryMessage::FwdReply:
                case MemoryMessage::MissReply: result->setState(State::Shared); break;
                case MemoryMessage::FwdReplyOwned: result->setState(State::Owned); break;
                default: DBG_Assert(false, (<< "Received invalid reply to a fetch request : " << (*msg))); break;
            }

            is_fill  = true;
            is_final = true;
            is_fetch = true;

            DBG_(Iface, (<< " Creating Ack"));
            msg->type() = MemoryMessage::FetchReply;
            intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::FetchAck, msg->address()));
            action.theBackMessage   = true;
            action.theBackTransport = transport;
            action.theBackTransport.set(MemoryMessageTag, reply);

            // Make sure the reply goes to the right cache
            action.theFrontToDCache = false;
            action.theFrontToICache = true;
            break;
        }
        case MemoryMessage::ReadReq: {
            DBG_Assert(state == State::Invalid,
                       (<< "Received reply to read req but block not invalid - " << state << " : " << (*msg)));

            // We need to allocate the block in the data array
            allocateBlock(result, msg->address());

            switch (msg->type()) {
                case MemoryMessage::FwdReply:
                    msg->type() = MemoryMessage::MissReply;
                    result->setState(State::Shared);
                    break;
                case MemoryMessage::FwdReplyOwned:
                    msg->type() = MemoryMessage::MissReply;
                    result->setState(State::Owned);
                    break;
                case MemoryMessage::MissReplyWritable: result->setState(State::Exclusive); break;
                default: DBG_Assert(false, (<< "Received invalid reply to a read request : " << (*msg))); break;
            }

            is_fill  = true;
            is_final = true;
            intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::ReadAck, msg->address()));
            action.theBackMessage   = true;
            action.theBackTransport = transport;
            action.theBackTransport.set(MemoryMessageTag, reply);
            DBG_(
              Trace,
              (<< " Allocated Block, State = " << result->state() << ", isHit? " << std::boolalpha << result->hit()));
            break;
        }
        case MemoryMessage::WriteReq: {
            // Shouldn't get these two types
            DBG_Assert(msg->type() != MemoryMessage::MissReply,
                       (<< " received MissReply in response to outstanding write : " << (*msg)));
            DBG_Assert(msg->type() != MemoryMessage::FwdReply,
                       (<< " received FwdReply in response to outstanding write : " << (*msg)));
            DBG_Assert(!from_eb);

            MissAddressFile::maf_iter maf_entry = theController->getWaitingMAFEntryIter(msg->address());

            // One less msg to wait for
            maf_entry->outstanding_msgs--;

            bool is_dirty = false;
            bool has_data = false;

            switch (msg->type()) {
                case MemoryMessage::UpgradeReply: maf_entry->outstanding_msgs = 0; break;
                case MemoryMessage::InvalidateAck:
                case MemoryMessage::InvalidateNAck:
                case MemoryMessage::FwdNAck: break;
                case MemoryMessage::FwdReplyOwned:
                case MemoryMessage::FwdReplyDirty:
                case MemoryMessage::InvUpdateAck:
                    is_dirty = true;
                    // Fall Through!
                case MemoryMessage::FwdReplyWritable:
                case MemoryMessage::MissReplyWritable: has_data = true; break;
                default: DBG_Assert(false, (<< "WriteReq received unexpected reply: " << (*msg))); break;
            }

            if (transport[TaglessDirMsgTag] != nullptr) {
                if (maf_entry->transport[TaglessDirMsgTag] == nullptr) {
                    maf_entry->transport.set(TaglessDirMsgTag, transport[TaglessDirMsgTag]);
                } else {
                    maf_entry->transport[TaglessDirMsgTag]->merge(transport[TaglessDirMsgTag]);
                }
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

            if (maf_entry->outstanding_msgs == 0) {
                bool sent_upgrade = (!maf_entry->data_received && result->state() != State::Invalid);
                if (sent_upgrade) {
                    tracker->setNetworkTrafficRequired(true);
                    tracker->setResponder(theNodeId);
                    tracker->setFillLevel(theCacheLevel);
                    maf_entry->data_received = true;
                }

                // If we didn't receive any data, then we need to send a retry to the
                // directory The directory will then forward that to memory
                if (!maf_entry->data_received) {
                    Action act(kSend, tracker, false);
                    intrusive_ptr<MemoryMessage> retry(new MemoryMessage(MemoryMessage::WriteRetry, msg->address()));
                    act.theBackMessage   = true;
                    act.theBackTransport = transport;
                    act.theBackTransport.set(MemoryMessageTag, retry);
                    act.theBackTransport[DestinationTag]->type = DestinationMessage::Directory;
                    // One more reply to wait for
                    maf_entry->outstanding_msgs++;
                    return act;
                }

                DBG_Assert(maf_entry->data_received,
                           (<< " # of outstanding msgs after receiveing miss notify is "
                               "0, but we don't have data yet."));

                intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::WriteAck, msg->address()));
                if (sent_upgrade) { reply->type() = MemoryMessage::UpgradeAck; }
                action.theBackMessage   = true;
                action.theBackTransport = transport;
                action.theBackTransport.set(MemoryMessageTag, reply);
                action.theBackTransport.set(TaglessDirMsgTag, maf_entry->transport[TaglessDirMsgTag]);
                if (action.theBackTransport[TaglessDirMsgTag] == nullptr) {
                    action.theBackTransport.set(TaglessDirMsgTag, TaglessDirMsg_p(new TaglessDirMsg()));
                }

                if (result->state() == State::Modified || result->state() == State::Owned) {
                    msg->type() = MemoryMessage::MissReplyDirty;
                    result->setState(State::Exclusive);
                } else {
                    result->setState(State::Exclusive);
                    msg->type() = MemoryMessage::MissReplyWritable;
                }
                // make sure we have the right size for the request
                msg->reqSize() = original_miss->reqSize();
                result->setProtected(false);
                theArray->recordAccess(result);
                is_final = true;
            } else {
                action.theAction = kNoAction;
                if (!has_data) { action.theRequiresData = 0; }
            }
            is_write = true;

            break;
        }
        case MemoryMessage::UpgradeReq: {
            // Shouldn't get these two types
            DBG_Assert(msg->type() != MemoryMessage::MissReply,
                       (<< " received MissReply in response to outstanding write : " << (*msg)));
            DBG_Assert(msg->type() != MemoryMessage::FwdReply,
                       (<< " received FwdReply in response to outstanding write : " << (*msg)));
            DBG_Assert(!from_eb);

            MissAddressFile::maf_iter maf_entry = theController->getWaitingMAFEntryIter(msg->address());

            // One less msg to wait for
            maf_entry->outstanding_msgs--;

            // This is an upgrade, so we shouldn't be in the invalid state
            DBG_Assert(result->state() != State::Invalid,
                       (<< "received reply to upgrade with block in invalid state : " << (*msg)));

            switch (msg->type()) {
                case MemoryMessage::UpgradeReply: maf_entry->outstanding_msgs = 0; break;
                case MemoryMessage::InvalidateAck:
                case MemoryMessage::InvalidateNAck: break;
                case MemoryMessage::FwdReplyOwned: result->setState(State::Modified); break;
                case MemoryMessage::FwdNAck:
                    DBG_Assert(false, (<< "UpgradeReq received FwdNAck! We don't need no stinkin' data!"));
                default: DBG_Assert(false, (<< "UpgradeReq received unexpected reply: " << (*msg))); break;
            }

            if (transport[TaglessDirMsgTag] != nullptr) {
                if (maf_entry->transport[TaglessDirMsgTag] == nullptr) {
                    maf_entry->transport.set(TaglessDirMsgTag, transport[TaglessDirMsgTag]);
                } else {
                    maf_entry->transport[TaglessDirMsgTag]->merge(transport[TaglessDirMsgTag]);
                }
            }

            if (maf_entry->outstanding_msgs == 0) {
                msg->reqSize() = original_miss->reqSize();
                result->setState(State::Exclusive);
                result->setProtected(false);
                theArray->recordAccess(result);
                is_final = true;
                intrusive_ptr<MemoryMessage> reply(new MemoryMessage(MemoryMessage::UpgradeAck, msg->address()));
                action.theBackMessage   = true;
                action.theBackTransport = transport;
                action.theBackTransport.set(MemoryMessageTag, reply);
                action.theBackTransport.set(TaglessDirMsgTag, maf_entry->transport[TaglessDirMsgTag]);
                if (action.theBackTransport[TaglessDirMsgTag] == nullptr) {
                    action.theBackTransport.set(TaglessDirMsgTag, TaglessDirMsg_p(new TaglessDirMsg()));
                }
                msg->type() = MemoryMessage::UpgradeReply;

                // Make sure we fill in the tracker
                tracker->setNetworkTrafficRequired(true);
                tracker->setResponder(theNodeId);
                tracker->setFillLevel(theCacheLevel);
            } else {
                action.theAction = kNoAction;
            }

            action.theRequiresData = 0;
            is_upgrade             = true;

            break;
        }
        default:
            DBG_Assert(
              false,
              (<< " MAF entry has un-expected type. original req: " << (*original_miss) << ", reply: " << (*msg)));
            break;
    }

    // Some book keeping stuff

    if (original_miss->type() == MemoryMessage::PrefetchReadAllocReq ||
        (original_tracker->originatorLevel() && (*original_tracker->originatorLevel() == eL2Prefetcher))) {
        is_prefetch = true;
        theTraceTracker.prefetch(theNodeId, theCacheLevel, original_miss->address());
        theTraceTracker.prefetchFill(theNodeId, theCacheLevel, msg->address(), *tracker->fillLevel());
    } else if (is_final) {
        DBG_(Iface, (<< "calling theTraceTracker.access()"));
        theTraceTracker.access(theNodeId,
                               theCacheLevel,
                               original_miss->address(),
                               original_miss->pc(),
                               result->state().prefetched(),
                               original_miss->isWrite(),
                               !is_upgrade,
                               original_miss->isPriv(),
                               (original_tracker->logicalTimestamp() ? *original_tracker->logicalTimestamp() : 0));
        DBG_(Iface, (<< "return from theTraceTracker.access()"));
    }

    if (is_final) {
        if (is_upgrade) {
            upg_replies++;
        } else {
            fills++;
        }
    }

    if (is_fill && !is_prefetch) {
        DBG_(Iface, (<< "calling theTraceTracker.fill()"));
        theTraceTracker.fill(theNodeId, thePeerLevel, msg->address(), *tracker->fillLevel(), is_fetch, is_write);
        DBG_(Iface, (<< "return from theTraceTracker.fill()"));
    }

    DBG_(Iface, (<< "calling theReadTracker.finishMiss()"));
    theReadTracker.finishMiss(original_tracker);
    DBG_(Iface, (<< "return from theReadTracker.finishMiss()"));

    if (result->state() != State::Invalid) {
        if (is_prefetch) {
            result->setPrefetched(true);
        } else {
            result->setPrefetched(false);
        }
    }

    if (action.theAction != kNoAction) {
        action.theFrontMessage   = true;
        action.theFrontTransport = transport;
    }
    return action;
} // handleBackSideMessage()

// Action TaglessInclusiveMOESIControllerImpl::finalizeSnoop(MemoryMessage_p
// msg, TransactionTracker_p tracker, LookupResult_p result) {
Action
TaglessInclusiveMOESIControllerImpl::finalizeSnoop(MemoryTransport transport, LookupResult_p result)
{

    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_Assert(msg != nullptr);
    DBG_Assert(tracker != nullptr);

    SnoopBuffer::snoop_iter snp = theSnoopBuffer.findEntry(getBlockAddress(msg->address()));
    DBG_Assert(snp != theSnoopBuffer.end(), (<< "received snoop reply but no waiting snoops: " << *msg));

    EvictBuffer<State>::iterator evEntry(theEvictBuffer.find(msg->address()));

    // InvUpdateAck or DownUpdateAck need to write data
    bool requires_data        = false;
    bool requires_tagless_msg = false;
    if ((msg->type() == MemoryMessage::InvUpdateAck) || (msg->type() == MemoryMessage::DownUpdateAck)) {

        // requested block might be in array or in the EB
        if (result->state() == State::Invalid) {
            DBG_Assert(evEntry != theEvictBuffer.end());
            evEntry->state() = State::Modified;
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
        DBG_(Trace,
             (<< "reply received, but still waiting for other snoop (D|I) = (" << std::boolalpha
              << snp->d_snoop_outstanding << "|" << snp->i_snoop_outstanding
              << "): " << *snp->transport[MemoryMessageTag]));
        return Action(kNoAction, tracker, requires_data);
    }

    MemoryMessage_p orig_msg = snp->transport[MemoryMessageTag];
    DBG_Assert(snp->transport[MemoryMessageTag] != nullptr);
    DBG_Assert(snp->transport[TransactionTrackerTag] != nullptr);

    // Determine proper response based on current cache/EB state
    State block_state(State::Invalid);
    if (result->state() != State::Invalid) {
        block_state = (result->state());
    } else {
        block_state = (evEntry->state());
    }

    switch (orig_msg->type()) {
        case MemoryMessage::ReadFwd:
        case MemoryMessage::FetchFwd:
            if ((block_state == State::Owned) || (block_state == State::Modified)) {
                orig_msg->type() = MemoryMessage::FwdReplyOwned;
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
            }

            // We either got data back that we need to write, or we need to read it
            requires_data = true;
            break;

        case MemoryMessage::WriteFwd:
            // If we got dirty data, then we don't need to access the data array
            requires_data = !requires_data;

        case MemoryMessage::Invalidate:
        case MemoryMessage::BackInvalidate:

            if ((block_state == State::Owned) || (block_state == State::Modified)) {
                if (orig_msg->type() == MemoryMessage::BackInvalidate) {
                    orig_msg->type() = MemoryMessage::InvUpdateAck;
                    if (!tracker->previousState()) {
                        if (block_state == State::Modified) {
                            tracker->setPreviousState(eModified);
                        } else {
                            tracker->setPreviousState(eShared);
                        }
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
                }
                requires_data = true;
            } else if (block_state == State::Exclusive) {
                if (orig_msg->type() == MemoryMessage::BackInvalidate) {
                    orig_msg->type() = MemoryMessage::InvUpdateAck;
                    requires_data    = true;
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
                }
            }
            if (result->state() != State::Invalid) {
                result->setState(State::Invalid);
                theArray->invalidateBlock(result);
            } else {
                DBG_Assert(evEntry != theEvictBuffer.end());
                if (evEntry->pending()) { thePendingEvicts--; }
                // the block is in the evict buffer
                // but it's invalid, so just get rid of the evict buffer entry
                theEvictBuffer.remove(evEntry);
                evEntry = theEvictBuffer.end();
                // Look for waiting entries in the snoop buffer that belong to this evict
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
            requires_tagless_msg = true;
            break;

        case MemoryMessage::EvictClean:
        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictDirty:
            if (block_state == State::Modified) { orig_msg->type() = MemoryMessage::EvictDirty; }
            evEntry->setEvictable(true);
            break;
        default: DBG_Assert(false, (<< "Unknown message type in snoop buffer: " << (*orig_msg))); break;
    }

    // Look for waiting entries;
    /*bool woke_snoops = */ theSnoopBuffer.wakeWaitingEntries(msg->address());

    Action action(kSend, tracker, requires_data);

    // Evicts stay in the evict buffer until we need the space
    if (orig_msg->isEvictType()) { action.theAction = kNoAction; }

    // If we didn't find any snoops to wake-up, then look for MAF entries waiting.
    // action.theWakeSnoops = !woke_snoops;
    action.theWakeSnoops = true; // Always true, races are handled in the rest of the code
    // and we need to make sure MAFs are woken eventually

    action.theBackMessage   = true;
    action.theBackTransport = snp->transport;

    if (requires_tagless_msg) {
        // Create a TaglessDirMsg to indicate which buckets we can clear
        // First, get the set of buckets that this block hashes to
        std::set<int> buckets = nGlobalHasher::GlobalHasher::theHasher().hashAddr(msg->address());
        std::set<int> conflicting_buckets;

        // Construct a set of buckets hashed to by other blocks in this set
        std::list<MemoryAddress> addrs          = theArray->getSetTags(msg->address());
        std::list<MemoryAddress>::iterator iter = addrs.begin();
        for (; iter != addrs.end(); iter++) {
            std::set<int> my_buckets = nGlobalHasher::GlobalHasher::theHasher().hashAddr(*iter);
            conflicting_buckets.insert(my_buckets.begin(), my_buckets.end());
        }

        // Now add the buckets hashed to by other blocks in the evict buffer
        EvictBuffer<State>::const_iterator eb_iter = theEvictBuffer.begin();
        for (; eb_iter != theEvictBuffer.end(); eb_iter++) {
            if (theArray->sameSet(msg->address(), eb_iter->address())) {
                std::set<int> my_buckets = nGlobalHasher::GlobalHasher::theHasher().hashAddr(eb_iter->address());
                conflicting_buckets.insert(my_buckets.begin(), my_buckets.end());
            }
        }

        // Now, look for any buckets that are in buckets, but not the
        // conflicting_buckets;
        std::vector<int> conflict_free(buckets.size());
        std::vector<int>::iterator last_it = std::set_difference(buckets.begin(),
                                                                 buckets.end(),
                                                                 conflicting_buckets.begin(),
                                                                 conflicting_buckets.end(),
                                                                 conflict_free.begin());

        std::set<int> conflict_free_set(conflict_free.begin(), last_it);

        TaglessDirMsg_p tagless_msg(new TaglessDirMsg());
        tagless_msg->addConflictFreeSet(theNodeId, conflict_free_set);
        action.theBackTransport.set(TaglessDirMsgTag, tagless_msg);
    }

    DBG_(Trace, (<< "Removing Snoop Buffer entry after receiving final reply: " << *snp->transport[MemoryMessageTag]));
    // Now remove the snoop buffer entry
    theSnoopBuffer.remove(snp);

    return action;
}

// Handles a Snoop process from the front side snoop channel
// Action
// TaglessInclusiveMOESIControllerImpl::handleSnoopMessage(MemoryMessage_p msg,
// TransactionTracker_p tracker) {
Action
TaglessInclusiveMOESIControllerImpl::handleSnoopMessage(MemoryTransport transport)
{

    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Trace, (<< "Snoop message: " << *msg));
    DBG_Assert(msg);
    DBG_Assert(msg->usesSnoopChannel());

    accesses++;

    LookupResult_p result = (*theArray)[msg->address()];

    switch (msg->type()) {
            // Snoops
        case MemoryMessage::InvalidateAck:
        case MemoryMessage::InvUpdateAck:
        case MemoryMessage::DowngradeAck:
        case MemoryMessage::DownUpdateAck:
            snoops++;
            return finalizeSnoop(transport, result);

            // Allocations
        case MemoryMessage::EvictDirty:
            // We ignore the snoop buffer
            // it will do another cache lookup at this level before it completes
            // and will determine the correct state at that time.

            // need to check evict buffer
            if (result->state() == State::Invalid) {
                EvictBuffer<State>::iterator evEntry(theEvictBuffer.find(msg->address()));
                if (evEntry != theEvictBuffer.end()) {
                    DBG_Assert(evEntry->type() != MemoryMessage::EvictDirty);
                    // Might as well leave it in the evict buffer, just change the
                    // evict type to EvictDirty
                    evEntry->type()  = MemoryMessage::EvictDirty;
                    evEntry->state() = State::Modified;
                }
            } else {
                DBG_Assert(result->state() == State::Exclusive,
                           (<< "received dirty evict for block in " << result->state() << " state"));
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
            return Action(kNoAction, tracker); // Suppress compiler warning

            // Flushes
            // I don't know where these come from or exactly how to handle them
            // I know they don't come from FLUSH instructions, so for now, I'm going to
            // ignore them
        case MemoryMessage::FlushReq:
        case MemoryMessage::Flush:

            // We only have IProbes, these are caught by the Controller and passed to
            // handleIprobe()
        case MemoryMessage::ProbedNotPresent:
        case MemoryMessage::ProbedClean:
        case MemoryMessage::ProbedWritable:
        case MemoryMessage::ProbedDirty:

            // No PrefetchInserts
            // Support can be added as required
        case MemoryMessage::PrefetchInsert:
        case MemoryMessage::PrefetchInsertWritable:
        case MemoryMessage::ReturnReply:
            // Anything else is an error
        default:
            DBG_Assert(false, (<< "Invalid message processed in handleSnoopMessage: " << *msg));
            return Action(kNoAction, tracker); // Suppress compiler warning
    }
}

// This handles a probe that was generated as a result of a Fetch miss
// at this cache level.
// Action TaglessInclusiveMOESIControllerImpl::handleIprobe(bool aHit,
// MemoryMessage_p fetchReq, TransactionTracker_p tracker) {
Action
TaglessInclusiveMOESIControllerImpl::handleIprobe(bool aHit, MemoryTransport transport)
{

    MemoryMessage_p fetchReq     = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    accesses++;
    iprobes++;

    LookupResult_p lookup = (*theArray)[fetchReq->address()];

    // We only really sent the probe to get dirty data
    // This will be a little bit sketchy, but whatever
    DBG_Assert(lookup->state() != State::Invalid);

    // We no longer need to protect the block
    lookup->setProtected(false);

    // If a snoop came in while we were waiting, then let it proceed and we'll
    // wait
    if (theSnoopBuffer.hasSnoopsOutstanding(fetchReq->address())) { return Action(kInsertMAF_WaitSnoop, tracker); }

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
    action.theFrontMessage   = true;
    action.theFrontTransport = transport;
    // Make sure the reply goes to the right cache
    action.theFrontToDCache = false;
    action.theFrontToICache = true;
    return action;
}

uint32_t
TaglessInclusiveMOESIControllerImpl::freeEvictBuffer() const
{
    int32_t ret = (theEvictBuffer.freeEntries() + thePendingEvicts) - theEvictThreshold;
    if (ret < 0) {
        return 0;
    } else {
        return ret;
    }
}

bool
TaglessInclusiveMOESIControllerImpl::evictableBlockExists(int32_t anIndex) const
{
    // Limit 4 pending evicts
    if ((thePendingEvicts + anIndex) >= theEvictThreshold) { return false; }
    return const_EvictBuffer().headEvictable(anIndex + thePendingEvicts);
}

Action
TaglessInclusiveMOESIControllerImpl::doEviction()
{
    Action action(kSend);

    if (evictBuffer().headEvictable(thePendingEvicts)) {
        MemoryMessage_p msg(theEvictBuffer.pop(thePendingEvicts));
        msg->reqSize() = theBlockSize;

        DBG_(Trace, (<< " Queuing eviction " << *msg));

        if (msg->type() == MemoryMessage::EvictDirty) {
            evicts_dirty++;
        } else {
            evicts_clean++;
        }

        // theTraceTracker.eviction(theNodeId, theCacheLevel, msg->address(),
        // false);

        // Create a transaction tracker for the eviction
        TransactionTracker_p tracker = new TransactionTracker;
        tracker->setAddress(msg->address());
        tracker->setInitiator(theNodeId);
        tracker->setSource(theName + " Evict");
        tracker->setDelayCause(theName, "Evict");

        action.theBackMessage = true;
        action.theBackTransport.set(MemoryMessageTag, msg);
        action.theBackTransport.set(TransactionTrackerTag, tracker);
        action.theOutputTracker = tracker;
        action.theRequiresData  = false;

        // Create a TaglessDirMsg to indicate which buckets we can clear
        // First, get the set of buckets that this block hashes to
        std::set<int> buckets = nGlobalHasher::GlobalHasher::theHasher().hashAddr(msg->address());
        std::set<int> conflicting_buckets;

        // Construct a set of buckets hashed to by other blocks in this set
        std::list<MemoryAddress> addrs          = theArray->getSetTags(msg->address());
        std::list<MemoryAddress>::iterator iter = addrs.begin();
        for (; iter != addrs.end(); iter++) {
            std::set<int> my_buckets = nGlobalHasher::GlobalHasher::theHasher().hashAddr(*iter);
            conflicting_buckets.insert(my_buckets.begin(), my_buckets.end());
        }

        // Now add the buckets hashed to by other blocks in the evict buffer
        EvictBuffer<State>::const_iterator eb_iter = theEvictBuffer.begin();
        for (; eb_iter != theEvictBuffer.end(); eb_iter++) {
            if (eb_iter->pending()) { continue; }
            if (theArray->sameSet(msg->address(), eb_iter->address())) {
                std::set<int> my_buckets = nGlobalHasher::GlobalHasher::theHasher().hashAddr(eb_iter->address());
                conflicting_buckets.insert(my_buckets.begin(), my_buckets.end());
            }
        }

        // Now, look for any buckets that are in buckets, but not the
        // conflicting_buckets;
        std::vector<int> conflict_free(buckets.size());
        std::vector<int>::iterator last_it = std::set_difference(buckets.begin(),
                                                                 buckets.end(),
                                                                 conflicting_buckets.begin(),
                                                                 conflicting_buckets.end(),
                                                                 conflict_free.begin());

        std::set<int> conflict_free_set(conflict_free.begin(), last_it);

        // If there are NO conflict free buckets, don't bother sending the evict
        // unless it's a Dirty Evict
        if (conflict_free_set.size() == 0 && msg->type() != MemoryMessage::EvictDirty) {
            action.theAction = kNoAction;
            // Need to remove the evict buffer entry
            theEvictBuffer.remove(theEvictBuffer.find(msg->address()));
            return action;
        }

        TaglessDirMsg_p tagless_msg(new TaglessDirMsg());
        tagless_msg->addConflictFreeSet(theNodeId, conflict_free_set);
        action.theBackTransport.set(TaglessDirMsgTag, tagless_msg);
        thePendingEvicts++;

    } else {
        action.theAction = kNoAction;
    }

    return action;
}

// Idle Work for us means evictions that require snoops
bool
TaglessInclusiveMOESIControllerImpl::idleWorkAvailable()
{
    if (theController->isFrontSideOutFull()) { return false; }
    if (theSnoopBuffer.full()) { return false; }
    bool ret = !theEvictBuffer.empty() && theEvictBuffer.back().theSnoopRequired;
    ret      = ret || (!theEvictBuffer.full() && theArray->evictionResourcePressure());

    return ret;
}

MemoryMessage_p
TaglessInclusiveMOESIControllerImpl::getIdleWorkMessage(ProcessEntry_p process)
{
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

void
TaglessInclusiveMOESIControllerImpl::removeIdleWorkReservations(ProcessEntry_p process, Action& action)
{
    theController->unreserveSnoopBuffer(process);

    if (action.theAction == kNoAction) { theController->unreserveFrontSideOut(process); }
}

// Action TaglessInclusiveMOESIControllerImpl::handleIdleWork(MemoryMessage_p
// msg, TransactionTracker_p tracker) {
Action
TaglessInclusiveMOESIControllerImpl::handleIdleWork(MemoryTransport transport)
{

    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    if (!theEvictBuffer.full() && theArray->evictionResourcePressure()) {
        // Move a block from the Array to the Evict Buffer
        std::pair<State, MemoryAddress> victim      = theArray->getPreemptiveEviction();
        MemoryMessage::MemoryMessageType evict_type = MemoryMessage::EvictClean;

        if (victim.first != State::Invalid) {
            if ((victim.first == State::Modified) || (victim.first == State::Owned)) {
                evict_type = MemoryMessage::EvictDirty;
            } else if (victim.first == State::Exclusive) {
                evict_type = MemoryMessage::EvictWritable;
            }
            EvictBuffer<State>::iterator iter =
              theEvictBuffer.allocEntry(victim.second, evict_type, victim.first, false);
            theTraceTracker.eviction(theNodeId, theCacheLevel, victim.second, false);
            DBG_(Trace, (<< " Adding " << std::hex << victim.second << " to the EvictBuffer."));

            if (msg->type() == MemoryMessage::NumMemoryMessageTypes) {
                msg->type()            = evict_type;
                msg->address()         = victim.second;
                iter->theSnoopRequired = false;
                DBG_(Trace, (<< "Getting Idle work: evicting " << std::hex << iter->theBlockAddress));
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
    // There is a race condition where a new request hits on the EB entry and puts
    // it into the cache between the time that the idleWork processes is created
    // and the time we get here. To handle this, we just return an empty action if
    // we can't find the block in question
    if (iter == theEvictBuffer.end()) {
        DBG_(Trace,
             (<< "Failed to find " << std::hex << msg->address() << " in the evict buffer, skipping idle work."));
        Action action(kNoAction, tracker, false);
        action.theRememberSnoopTransport = false;
        return action;
    }
    DBG_Assert(iter != theEvictBuffer.end());

    iter->theSnoopScheduled = true;

    if (!theSnoopBuffer.hasEntry(msg->address())) {
        SnoopBuffer::snoop_iter snp = theSnoopBuffer.allocEntry(transport);
        snp->i_snoop_outstanding    = true;

        Action act(kSend, tracker, false);
        intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
        // Need to make sure reqSize is 0 so we don't get InvUpdateAck when we
        // shouldn't
        request->reqSize()    = 0;
        act.theFrontMessage   = true;
        act.theFrontTransport = transport;
        act.theFrontTransport.set(MemoryMessageTag, request);
        if (iter->state() == State::Modified) {
            act.theFrontToDCache     = false;
            snp->d_snoop_outstanding = false;
        } else {
            act.theFrontToDCache     = true;
            snp->d_snoop_outstanding = true; // This is redundant, only here for clarity
        }
        act.theFrontToICache = true;

        return act;
    } else {
        theSnoopBuffer.addWaitingEntry(transport);
        return Action(kNoAction, tracker, false);
    }
}

// Evicts are the same as IdleWork above, all other messages are BackMessages
// and handled the same way
// Action TaglessInclusiveMOESIControllerImpl::handleWakeSnoop ( MemoryMessage_p
// msg,
//                TransactionTracker_p tracker ) {
Action
TaglessInclusiveMOESIControllerImpl::handleWakeSnoop(MemoryTransport transport)
{

    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Trace, (<< " Handle WakeSnoop: " << *msg));
    if (msg->isEvictType()) {
        SnoopBuffer::snoop_iter snp = theSnoopBuffer.findEntry(msg->address());
        DBG_Assert(snp == theSnoopBuffer.end());
        snp = theSnoopBuffer.allocEntry(transport);

        Action act(kSend, tracker, false);
        intrusive_ptr<MemoryMessage> request(new MemoryMessage(MemoryMessage::Invalidate, msg->address()));
        request->reqSize()    = 0;
        act.theFrontMessage   = true;
        act.theFrontTransport = transport;
        act.theFrontTransport.set(MemoryMessageTag, request);

        EvictBuffer<State>::iterator iter = theEvictBuffer.find(msg->address());
        if (iter->state() == State::Modified) {
            act.theFrontToDCache     = false;
            snp->d_snoop_outstanding = false;
        } else {
            act.theFrontToDCache     = true;
            snp->d_snoop_outstanding = true; // This is redundant, only here for clarity
        }
        act.theFrontToICache = true;
        return act;
    } else {
        return handleBackMessage(transport);
    }
}

void
TaglessInclusiveMOESIControllerImpl::dumpEvictBuffer() const
{
    BaseCacheControllerImpl::dumpEvictBuffer();

    EvictBuffer<State>::const_iterator iter = theEvictBuffer.begin();
    for (int32_t i = 0; iter != theEvictBuffer.end(); i++, iter++) {
        DBG_(Trace, (<< " EB[" << i << "] = " << *iter));
    }
}

} // end namespace nCache
