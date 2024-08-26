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

/*! \file BaseCacheController.cpp
 * \brief
 *
 *  This file contains the implementation of the BaseCacheController.  Alternate
 *  or extended definitions should be provided elsewhere.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  the guts of a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 */

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/TraceTracker.hpp>
#include <core/metaprogram.hpp>
#include <core/performance/profile.hpp>
#include <fstream>

using namespace boost::multi_index;
using namespace Flexus;

#include "BaseCacheControllerImpl.hpp"

#include <components/CommonQEMU/AbstractFactory.hpp>

#define DBG_DeclareCategories Cache
#define DBG_SetDefaultOps     AddCat(Cache) Set((CompName) << theName) Set((CompIdx) << theNodeId)
#include DBG_Control()

namespace nCache {

namespace Stat = Flexus::Stat;

std::ostream&
operator<<(std::ostream& s, const Action action)
{
    return s << action.theAction;
}

std::ostream&
operator<<(std::ostream& s, const enumAction eAction)
{
    const char* actions[] = { "kNoAction",
                              "kSend",
                              "kInsertMAF_WaitAddress",
                              "kInsertMAF_WaitSnoop",
                              "kInsertMAF_WaitEvict",
                              "kInsertMAF_WaitResponse",
                              "kInsertMAF_WaitProbe",
                              "kReplyAndRemoveMAF",
                              "kReplyAndRemoveResponseMAF",
                              "kInsertMAF_WaitRegion",
                              "kRetryRequest",
                              "kReplyAndRetryMAF",
                              "kReplyAndInsertMAF_WaitResponse" };
    return s << actions[eAction];
}

BaseCacheControllerImpl*
BaseCacheControllerImpl::construct(CacheController* aController, CacheInitInfo* anInfo, const std::string& type)
{
    ControllerParams params;
    params.first  = aController;
    params.second = anInfo;

    return AbstractFactory<BaseCacheControllerImpl, ControllerParams>::createInstance(type, params);
}

////////////////////////////////////////////////////////////////////////////////
// BaseCacheControllerImpl method definitions

BaseCacheControllerImpl::BaseCacheControllerImpl(CacheController* aController, CacheInitInfo* anInit)
  : theController(aController)
  , theSnoopBuffer(anInit->theSBSize)
  , theInit(anInit)
  , theName(anInit->theName)
  , theBlockSize(anInit->theBlockSize)
  , theNodeId(anInit->theNodeId)
  , theProbeOnIfetchMiss(anInit->theProbeOnIfetchMiss)
  , theReadTracker(anInit->theName)
  , theCacheLevel(anInit->theCacheLevel)
  , theDoCleanEvictions(anInit->theDoCleanEvictions)
  , accesses(anInit->theName + "-Accesses")
  , accesses_user_I(anInit->theName + "-Accesses:User:I")
  , accesses_user_D(anInit->theName + "-Accesses:User:D")
  , accesses_system_I(anInit->theName + "-Accesses:System:I")
  , accesses_system_D(anInit->theName + "-Accesses:System:D")
  , requests(anInit->theName + "-Requests")
  , hits(anInit->theName + "-Hits")
  , hits_user_I(anInit->theName + "-Hits:User:I")
  , hits_user_D(anInit->theName + "-Hits:User:D")
  , hits_user_D_Read(anInit->theName + "-Hits:User:D:Read")
  , hits_user_D_Write(anInit->theName + "-Hits:User:D:Write")
  , hits_user_D_PrefetchRead(anInit->theName + "-Hits:User:D:PrefetchRead")
  , hits_user_D_PrefetchWrite(anInit->theName + "-Hits:User:D:PrefetchWrite")
  , hits_system_I(anInit->theName + "-Hits:System:I")
  , hits_system_D(anInit->theName + "-Hits:System:D")
  , hits_system_D_Read(anInit->theName + "-Hits:System:D:Read")
  , hits_system_D_Write(anInit->theName + "-Hits:System:D:Write")
  , hits_system_D_PrefetchRead(anInit->theName + "-Hits:System:D:PrefetchRead")
  , hits_system_D_PrefetchWrite(anInit->theName + "-Hits:System:D:PrefetchWrite")
  , hitsEvict(anInit->theName + "-HitsEvictBuf")
  , misses(anInit->theName + "-Misses")
  , misses_user_I(anInit->theName + "-Misses:User:I")
  , misses_user_D(anInit->theName + "-Misses:User:D")
  , misses_user_D_Read(anInit->theName + "-Misses:User:D:Read")
  , misses_user_D_Write(anInit->theName + "-Misses:User:D:Write")
  , misses_user_D_PrefetchRead(anInit->theName + "-Misses:User:D:PrefetchRead")
  , misses_user_D_PrefetchWrite(anInit->theName + "-Misses:User:D:PrefetchWrite")
  , misses_system_I(anInit->theName + "-Misses:System:I")
  , misses_system_D(anInit->theName + "-Misses:System:D")
  , misses_system_D_Read(anInit->theName + "-Misses:System:D:Read")
  , misses_system_D_Write(theName + "-Misses:System:D:Write")
  , misses_system_D_PrefetchRead(anInit->theName + "-Misses:System:D:PrefetchRead")
  , misses_system_D_PrefetchWrite(anInit->theName + "-Misses:System:D:PrefetchWrite")
  , misses_peerL1_system_I(anInit->theName + "-Misses:PeerL1:System:I")
  , misses_peerL1_system_D(anInit->theName + "-Misses:PeerL1:System:D")
  , misses_peerL1_user_I(anInit->theName + "-Misses:PeerL1:User:I")
  , misses_peerL1_user_D(anInit->theName + "-Misses:PeerL1:User:D")
  , upgrades(anInit->theName + "-Upgrades")
  , fills(anInit->theName + "-Fills")
  , upg_replies(anInit->theName + "-UpgradeReplies")
  , evicts_clean(anInit->theName + "-EvictsClean")
  , evicts_dirty(anInit->theName + "-EvictsDirty")
  , evicts_with_data(anInit->theName + "-EvictsWithData")
  , evicts_dropped(anInit->theName + "-EvictsDropped")
  , evicts_protocol(anInit->theName + "-EvictsProtocol")
  , snoops(anInit->theName + "-Snoops")
  , probes(anInit->theName + "-Probes")
  , iprobes(anInit->theName + "-Iprobes")
  , lockedAccesses(anInit->theName + "-LockedAccesses")
  , tag_match_invalid(anInit->theName + "-TagMatchesInvalid")
  , prefetchReads(anInit->theName + "-PrefetchReads")
  , prefetchHitsRead(anInit->theName + "-PrefetchHitsRead")
  , prefetchHitsPrefetch(anInit->theName + "-PrefetchHitsPrefetch")
  , prefetchHitsWrite(anInit->theName + "-PrefetchHitsWrite")
  , prefetchHitsButUpgrade(anInit->theName + "-PrefetchHitsButUpgrade")
  , prefetchEvicts(anInit->theName + "-PrefetchEvicts")
  , prefetchInvals(anInit->theName + "-PrefetchInvals")
  , atomicPreloadWrites(anInit->theName + "-AtomicPreloadWrites")
{
    // The array is now the responsibility of the derived class (so it can have
    // appropriate state

    if ((theCacheLevel == eL1) || (theCacheLevel == eL1I)) {
        thePeerLevel = ePeerL1Cache;
    } else if (theCacheLevel == eL2) {
        thePeerLevel = ePeerL2;
    } else {
        DBG_Assert(false, (<< "No Peer Level available for selected cache level " << theCacheLevel));
    }
}

///////////////////////////
// State management

void
BaseCacheControllerImpl::loadState(std::string const& ckpt_dirname)
{

    std::string ckpt_filename(ckpt_dirname);
    ckpt_filename += "/" + theName + ".json";

    std::ifstream ifs(ckpt_filename.c_str(), std::ios::in);

    if (!ifs.good()) {
        DBG_(Dev, (<< "checkpoint file: " << ckpt_filename << " not found."));
        DBG_Assert(false, (<< "FILE NOT FOUND"));
    }

    load_from_ckpt(ifs);

    ifs.close();
}

///////////////////////////
// Eviction Processing

Action
BaseCacheControllerImpl::doEviction()
{
    Action action(kSend);

    if (evictBuffer().headEvictable(0)) {
        MemoryMessage_p msg(evictBuffer().pop());
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
    } else {
        action.theAction = kNoAction;
    }

    return action;
}

Action
BaseCacheControllerImpl::handleRequestTransport(MemoryTransport transport, bool has_maf_entry)
{
    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Iface, (<< " Handle Request: " << *msg));

    // check invariants for handleRequestMessage
    DBG_Assert(msg);
    DBG_Assert(msg->isRequest(), (<< *msg << " is NOT a Request message"));
    DBG_Assert(!msg->usesSnoopChannel(), (<< *msg));

    // Increment stats
    accesses++;
    if (tracker && tracker->isFetch() && *tracker->isFetch()) {
        if (tracker->OS() && *tracker->OS()) {
            accesses_system_I++;
        } else {
            accesses_user_I++;
        }
    } else {
        if (tracker->OS() && *tracker->OS()) {
            accesses_system_D++;
        } else {
            accesses_user_D++;
        }
    }

    // if this is a prefetch read request, it should not get a MAF entry
    if ((msg->type() == MemoryMessage::PrefetchReadNoAllocReq || msg->type() == MemoryMessage::PrefetchReadAllocReq) &&
        has_maf_entry) {
        // consider a hit in the MAF to be similar to a hit in the
        // array - redundant for both
        // send a reply to indicate this was redundant
        msg->type() = MemoryMessage::PrefetchReadRedundant;
        return Action(kSend);
    }
    // this is an ordinary request - check if there are other transactions
    // outstanding for this address
    return examineRequest(transport, has_maf_entry);
} // handleRequestMessage

Action
BaseCacheControllerImpl::wakeMaf(MemoryTransport transport, TransactionTracker_p aWakingTracker)
{
    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Iface, (<< " Wake MAF: " << *msg));

    accesses++;
    if (tracker && tracker->isFetch() && *tracker->isFetch()) {
        if (tracker->OS() && *tracker->OS()) {
            accesses_system_I++;
        } else {
            accesses_user_I++;
        }
    } else {
        if (tracker->OS() && *tracker->OS()) {
            accesses_system_D++;
        } else {
            accesses_user_D++;
        }
    }

    if (tracker) { tracker->setBlockedInMAF(true); }

    return examineRequest(transport, false, aWakingTracker);
    // Possible return values:
    // ReplyAndRemoveMAF - this message is handled, process the next waiting MAF
    // kInsertMAF_WaitResponse - the request caused a miss.  Put it in
    // WaitResponse state and stop waking.
} // wakeMaf()

Action
BaseCacheControllerImpl::examineRequest(MemoryTransport transport,
                                        bool has_maf_entry,
                                        TransactionTracker_p aWakingTracker)
{
    MemoryMessage_p msg          = transport[MemoryMessageTag];
    TransactionTracker_p tracker = transport[TransactionTrackerTag];

    DBG_(Iface, (<< " Examine Request " << *msg));
    if (tracker) { tracker->setDelayCause(theName, "Processing"); }

    bool was_write = msg->isWrite();

    bool was_hit, was_prefetched;
    Action action;
    std::tie(was_hit, was_prefetched, action) = doRequest(transport, has_maf_entry, aWakingTracker);

    requests++;

    // look the address up in the array
    if (was_hit) {

        // TraceTracker hit
        if (tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher)) {
            // this should probably be a redundant prefetch
            // theTraceTracker.prefetch(theNodeId, theCacheLevel, msg->address());
        } else {
            theTraceTracker.access(theNodeId,
                                   theCacheLevel,
                                   msg->address(),
                                   msg->pc(),
                                   was_prefetched,
                                   was_write,
                                   false,
                                   msg->isPriv(),
                                   (tracker->logicalTimestamp() ? *tracker->logicalTimestamp() : 0));
        }

        DBG_(Trace, (<< " Hit: " << *msg));
        hits++;
        if (tracker && tracker->isFetch() && *tracker->isFetch()) {
            if (tracker->OS() && *tracker->OS()) {
                hits_system_I++;
            } else {
                hits_user_I++;
            }
        } else if (msg->type() == MemoryMessage::PrefetchReadRedundant ||
                   (tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher))) {
            if (tracker->OS() && *tracker->OS()) {
                hits_system_D++;
                hits_system_D_PrefetchRead++;
            } else {
                hits_user_D++;
                hits_user_D_PrefetchRead++;
            }
        } else if (msg->type() == MemoryMessage::StorePrefetchReply) {
            if (tracker->OS() && *tracker->OS()) {
                hits_system_D++;
                hits_system_D_PrefetchWrite++;
            } else {
                hits_user_D++;
                hits_user_D_PrefetchWrite++;
            }
        } else {
            if (was_write) {
                if (tracker->OS() && *tracker->OS()) {
                    hits_system_D++;
                    hits_system_D_Write++;
                } else {
                    hits_user_D++;
                    hits_user_D_Write++;
                }
            } else {
                if (tracker->OS() && *tracker->OS()) {
                    hits_system_D++;
                    hits_system_D_Read++;
                } else {
                    hits_user_D++;
                    hits_user_D_Read++;
                }
            }
        }

        if (tracker && aWakingTracker) {
            // This request is being serviced via MAF wakeup.  We should record
            // the transaction fill details from the original request into this
            // request, since both transaction had to wait on the same memory
            // system events
            if (aWakingTracker->networkTrafficRequired()) {
                tracker->setNetworkTrafficRequired(*aWakingTracker->networkTrafficRequired());
            }
            if (aWakingTracker->responder()) { tracker->setResponder(*aWakingTracker->responder()); }
            if (aWakingTracker->fillLevel()) {
                if (aWakingTracker->originatorLevel() && (*aWakingTracker->originatorLevel() == eL2Prefetcher)) {
                    tracker->setFillLevel(ePrefetchBuffer);
                } else {
                    tracker->setFillLevel(*aWakingTracker->fillLevel());
                }
            }
            if (aWakingTracker->fillType()) { tracker->setFillType(*aWakingTracker->fillType()); }
            if (aWakingTracker->previousState()) { tracker->setPreviousState(*aWakingTracker->previousState()); }
        }

    } else {
        DBG_(Trace, (<< " Miss: " << *msg << " action is " << action.theAction));
        // We used to record invalid tag matches here, need to make sure we do that
        // someplace else now
    }

    return action;
} // examineRequest()

MemoryTransport
BaseCacheControllerImpl::getWakingSnoopTransport()
{
    MemoryTransport transport = theSnoopBuffer.wakeSnoop();
    return transport;
}

bool
BaseCacheControllerImpl::snoopResourcesAvailable(const const_MemoryMessage_p msg)
{
    // return !fullEvictBuffer() && !theController->isFrontSideOutFull();
    return true;
}

void
BaseCacheControllerImpl::reserveSnoopResources(MemoryMessage_p msg, ProcessEntry_p process)
{
    // theController->reserveFrontSideOut(process);
    // theController->reserveEvictBuffer(process);
}

void
BaseCacheControllerImpl::unreserveSnoopResources(MemoryMessage_p msg, ProcessEntry_p process)
{
    // theController->unreserveFrontSideOut(process);
    // theController->unreserveEvictBuffer(process);
}

// END BaseCacheControllerImpl method definitions
////////////////////////////////////////////////////////////////////////////////

}; // namespace nCache