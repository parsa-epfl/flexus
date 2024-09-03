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

#include <boost/dynamic_bitset.hpp>
#include <components/CommonQEMU/MessageQueues.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

using namespace Flexus;

#define DBG_DefineCategories NonInclusiveMESIPolicyCat
#define DBG_SetDefaultOps    AddCat(NonInclusiveMESIPolicyCat)
#include DBG_Control()

#include <components/CMPCache/AbstractArray.hpp>
#include <components/CMPCache/AbstractPolicy.hpp>
#include <components/CMPCache/CacheState.hpp>
#include <components/CMPCache/NonInclusiveMESIPolicy.hpp>
#include <components/CMPCache/ProcessEntry.hpp>

namespace nCMPCache {

REGISTER_CMP_CACHE_POLICY(NonInclusiveMESIPolicy, "NonInclusiveMESI");

NonInclusiveMESIPolicy::NonInclusiveMESIPolicy(const CMPCacheInfo& params)
  : theCMPCacheInfo(params)
  , theCacheEvictBuffer(params.theCacheEBSize)
  , theMAF(params.theMAFSize)
  , theStats(theCMPCacheInfo.theName)
  , theDefaultState(params.theCores)
{
    DBG_(Dev, (<< "GI = " << theCMPCacheInfo.theGroupInterleaving));

    theDirectory = constructDirectory<State, State>(params);

    std::string config = params.theCacheParams;
    theCache           = constructArray<CacheState, CacheState::Invalid>(config, theCMPCacheInfo, params.theBlockSize);

    theDirEvictBuffer = theDirectory->getEvictBuffer();
    // theCacheEvictBuffer = theCache->getEvictBuffer();
}

NonInclusiveMESIPolicy::~NonInclusiveMESIPolicy()
{
    delete theDirectory;
    delete theCache;
}

AbstractPolicy*
NonInclusiveMESIPolicy::createInstance(std::list<std::pair<std::string, std::string>>& args, const CMPCacheInfo& params)
{
    DBG_Assert(args.empty(), NoDefaultOps());
    DBG_(Dev, (<< "GI = " << params.theGroupInterleaving));
    return new NonInclusiveMESIPolicy(params);
}

void
NonInclusiveMESIPolicy::load_dir_from_ckpt(std::string const& filename)
{
    theDirectory->load_dir_from_ckpt(filename);
}

void
NonInclusiveMESIPolicy::load_cache_from_ckpt(std::string const& filename)
{
    theCache->load_cache_from_ckpt(filename, theCMPCacheInfo.theNodeId);
}
void
NonInclusiveMESIPolicy::handleRequest(ProcessEntry_p process)
{
    doRequest(process, false);
}

bool
NonInclusiveMESIPolicy::allocateDirectoryEntry(DirLookupResult_p d_lookup, MemoryAddress anAddress, const State& aState)
{
    return theDirectory->allocate(d_lookup, anAddress, aState);
}

void
NonInclusiveMESIPolicy::doRequest(ProcessEntry_p process, bool has_maf)
{
    MemoryMessage_p msg          = process->transport()[MemoryMessageTag];
    TransactionTracker_p tracker = process->transport()[TransactionTrackerTag];

    MemoryAddress address = theCache->blockAddress(process->transport()[MemoryMessageTag]->address());
    MemoryMessage::MemoryMessageType req_type = process->transport()[MemoryMessageTag]->type();
    int32_t requester                         = process->transport()[DestinationTag]->requester;

    // By default assume we'll stall and take no action
    process->setAction(eStall);

    // First we need to look for conflicting MAF entries
    maf_iter_t maf   = theMAF.find(address);
    bool maf_waiting = false;

    if (maf != theMAF.end()) {
        switch (maf->state()) {
            case eWaitSet:
                // evicts can proceed
                if (msg->isEvictType()) { return doEvict(process, has_maf); }

                // There's already a stalled request, to reduce starvation, we'll stall as
                // well
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitSet);
                } else {
                    theMAF.insert(process->transport(), address, eWaitSet, theDefaultState);
                }
                return;
            case eWaitEvict:
                // evicts can proceed
                if (msg->isEvictType()) { return doEvict(process, has_maf); }
            case eWaitRequest:
                // There's already a stalled request, to reduce starvation, we'll stall as
                // well
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitRequest);
                } else {
                    theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
                }
                return;
            case eWaitAck: {
                // This is where things get tricky
                // We can overlap two reads/fetches, but not a read and a write
                MemoryMessage::MemoryMessageType maf_type = maf->transport()[MemoryMessageTag]->type();
                if (req_type == MemoryMessage::WriteReq || req_type == MemoryMessage::UpgradeReq ||
                    req_type == MemoryMessage::NonAllocatingStoreReq || maf_type == MemoryMessage::WriteReq ||
                    maf_type == MemoryMessage::UpgradeReq || maf_type == MemoryMessage::NonAllocatingStoreReq) {
                    if (has_maf) {
                        theMAF.setState(process->maf(), eWaitRequest);
                    } else {
                        theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
                    }
                    return;
                } else {
                    maf_waiting = true;
                }
                // Both the maf and incoming request are reads so we can overlap them and
                // just ignore the maf
                break;
            }

            case eFinishing: // For now we ignore requests that are finishing
                             // I think this is going to cause problems in the
                             // long-term, but I don't know how to handle it yet

                // entries in the process of waking up are ignored
            case eWaking:
            case eInPipeline: break;

            default: DBG_Assert(false, (<< "Invalid MAFState: " << maf->state())); break;
        }
    }

    if (msg->isEvictType()) { return doEvict(process, has_maf); }

    // It's possible to get multiple redundant fetches
    // The correct thing to do in that case is to ignore them
    if (maf_waiting && req_type == MemoryMessage::FetchReq) {
        maf_iter_t first, last;
        std::tie(first, last) = theMAF.findAll(address, eWaitAck);
        for (; first != theMAF.end() && first != last; first++) {
            // The matching request is the one with the same requester
            if (first->transport()[DestinationTag]->requester == requester) {
                if (has_maf) {
                    process->setAction(eRemoveMAF);
                } else {
                    process->setAction(eNoAction);
                }
                return;
            }
        }
    }

    // No conflicting MAF entry

    // Can we find the block in the directory evict buffer
    const AbstractDirEBEntry<State>* d_eb = theDirEvictBuffer->find(address);
    if (d_eb != nullptr) {
        if (d_eb->invalidatesPending()) {
            // We've sent out back invalidates to evict this block
            // Wait until they get back before processing the new request
            if (has_maf) {
                theMAF.setState(process->maf(), eWaitEvict);
            } else {
                theMAF.insert(process->transport(), address, eWaitEvict, theDefaultState);
            }
            return;
        }
    }

    // Need to make sure we have a directory entry for this block
    // 3 cases:
    //  1. block is in the directory
    //  2. block is in the evict buffer
    //  3. block is in neither directory nor evict buffer
    //
    // If it's case 2, we should move the block BACK into the directory
    // If it's case 3, we should allocate the block in the directory
    // In either case, we can do the directory lookup first

    DirLookupResult_p dir_lookup = theDirectory->lookup(address);

    // If we didn't find it in the directory
    if (!dir_lookup->found()) {
        // We already did an EB lookup, check if we found it
        if (d_eb != nullptr) {
            // Move the entry from the EB into the directory
            if (!allocateDirectoryEntry(dir_lookup, address, d_eb->state())) {
                // if this fails then we have a set conflict, so insert the MAF and
                // return
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitSet);
                } else {
                    theMAF.insert(process->transport(), address, eWaitSet, theDefaultState);
                }
                DBG_(Trace,
                     (<< theCMPCacheInfo.theName << " - failed to allocate directory entry for " << std::hex << address
                      << " found in evict buffer."));
                return;
            }
            // remove the old d_eb entry
            theDirEvictBuffer->remove(address);
        } else if (req_type != MemoryMessage::NonAllocatingStoreReq) {
            // Create a new entry
            if (!allocateDirectoryEntry(dir_lookup, address, theDefaultState)) {
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitSet);
                } else {
                    theMAF.insert(process->transport(), address, eWaitSet, theDefaultState);
                }
                DBG_(Trace,
                     (<< theCMPCacheInfo.theName << " - failed to allocate directory entry for " << std::hex << address
                      << " stalling."));
                return;
            }
        }
    }

    // Now, let's check the state of the block in the cache
    CacheLookupResult_p c_lookup = (*theCache)[address];

    // If we didn't find the block in the cache, check the evict buffer
    if (c_lookup->state() == CacheState::Invalid) {
        CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(address);
        if (c_eb != theCacheEvictBuffer.end()) {
            // If there's a pending WB for this block, we should wait until it's done
            if (c_eb->pending()) {
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitEvict);
                } else {
                    theMAF.insert(process->transport(), address, eWaitEvict, theDefaultState);
                }
                return;
            }

            // Move the block from the evict buffer into the cache

            CacheState block_state     = c_eb->state();
            CacheLookupResult_p victim = theCache->allocate(c_lookup, address);
            theCacheEvictBuffer.remove(c_eb);
            c_lookup->setState(block_state);

            DBG_(Trace, (<< " replaceing EB entry, evicting block in state " << victim->state() << " : " << *msg));
            if (victim->state() != CacheState::Invalid) { evictCacheBlock(victim); }
        }
    }

    // Cache State is now present in c_lookup

    //  bool is_cache_hit = false;
    //  bool was_prefetched = false;
    //  if (c_lookup->state() != CacheState::Invalid) {
    //    is_cache_hit = true;
    //    was_prefetched = c_lookup->state().prefetched();
    //  }

    MemoryTransport rep_transport(process->transport());

    // The only case where we might not have a dir entry by now is for
    // Non-Allocating stores handle this case now to simplify things
    if (req_type == MemoryMessage::NonAllocatingStoreReq && !dir_lookup->found()) {
        // If the block is present in the cache, write the data to the cache and
        // finish
        if (c_lookup->state() != CacheState::Invalid) {
            c_lookup->setState(CacheState::Modified);
            theCache->recordAccess(c_lookup);

            MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::NonAllocatingStoreReply, address));
            rep_msg->ackRequired()     = true;
            rep_msg->ackRequiresData() = false;
            rep_msg->reqSize()         = process->transport()[MemoryMessageTag]->reqSize();

            rep_transport.set(MemoryMessageTag, rep_msg);
            rep_transport.set(DestinationTag,
                              DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
            rep_transport[DestinationTag]->type = DestinationMessage::Requester;

            process->setReplyTransport(rep_transport);
            process->setRequiresData(true);

            if (has_maf) {
                theMAF.setState(process->maf(), eWaitAck);
            } else {
                theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
            }
            process->setAction(eReply);

            tracker->setFillLevel(theCMPCacheInfo.theCacheLevel);
            // theTraceTracker.fill(theNodeId, theCMPCacheInfo.theCacheLevel, address,
            // *tracker->fillLevel(), false, true);

            theStats.NASHit++;

        } else {
            // Otherwise forward to memory and return
            process->transport()[DestinationTag]->type = DestinationMessage::Memory;
            process->addSnoopTransport(process->transport());
            process->setAction(eFwdAndWaitAck);
            if (has_maf) {
                theMAF.setState(process->maf(), eWaitAck);
            } else {
                theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
            }
            theStats.NASMissMemory++;
        }
        return;
    }

    // Dir State is now present in dir_lookup
    // Protect the directory entry so it can't be removed until we get a final
    // acknowledgement
    if (req_type != MemoryMessage::NonAllocatingStoreReq) { dir_lookup->setProtected(true); }

    // Check if it's necessary to convert an Upgrade to a Write
    // This happens when a block is invalidated after an upgrade is sent
    if (req_type == MemoryMessage::UpgradeReq && !dir_lookup->state().isSharer(requester)) {
        DBG_(Trace,
             (<< "Received Upgrade from Non-Sharer, converting to WriteReq: "
              << *(process->transport()[MemoryMessageTag])));
        req_type                                       = MemoryMessage::WriteReq;
        process->transport()[MemoryMessageTag]->type() = MemoryMessage::WriteReq;
    }

    // If the data is not on-chip, we need to go to memory.
    if (dir_lookup->state().noSharers() && c_lookup->state() == CacheState::Invalid) {
        if (maf_waiting) {
            // If we found a waiting MAF entry then we wait for it to complete first,
            // then do an on-chip transfer This avoids a number of complications,
            // reduces off-chip bandwidth, and will likely be faster in the int64_t
            // rung
            if (has_maf) {
                theMAF.setState(process->maf(), eWaitRequest);
            } else {
                theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
            }
            return;
        }

        // We don't have the data on chip, we need to forward the request to memory
        process->transport()[DestinationTag]->type = DestinationMessage::Memory;
        process->addSnoopTransport(process->transport());
        process->setAction(eFwdAndWaitAck);
        if (has_maf) {
            theMAF.setState(process->maf(), eWaitAck);
        } else {
            process->maf() = theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
        }

        // Write requests need a miss-notify
        if (req_type == MemoryMessage::WriteReq) {
            MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissNotify, address));
            rep_msg->outstandingMsgs() = 1;
            rep_transport.set(MemoryMessageTag, rep_msg);
            rep_transport.set(DestinationTag,
                              DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
            rep_transport[DestinationTag]->type = DestinationMessage::Requester;
            process->setReplyTransport(rep_transport);
            process->setAction(eNotifyAndWaitAck);
        } else if (req_type != MemoryMessage::NonAllocatingStoreReq) {
            // If this is a read or fetch, the ack will return data to the cache
            // keep a Cache Evict Buffer reservation in the MAF entry so we can
            // allocate the block at that time

            theMAF.setCacheEBReserved(process->maf(), process->cache_eb_reserved);
            process->cache_eb_reserved = 0;
        }

        switch (req_type) {
            case MemoryMessage::WriteReq: theStats.WriteMissMemory++; break;
            case MemoryMessage::ReadReq: theStats.ReadMissMemory++; break;
            case MemoryMessage::FetchReq: theStats.FetchMissMemory++; break;
            case MemoryMessage::NonAllocatingStoreReq: theStats.NASMissMemory++; break;
            default: DBG_Assert(false); break;
        }

        return;
    }

    // There is at least one copy of the data on-chip, either in this cache or
    // another one above us

    switch (req_type) {
        case MemoryMessage::ReadReq:
        case MemoryMessage::FetchReq: {
            // If the block is not in the cache, or if there might be a modified copy
            // higher up, forward the request to a sharer
            if ((c_lookup->state() == CacheState::Invalid) ||
                (dir_lookup->state().oneSharer() && (c_lookup->state() == CacheState::Exclusive))) {

                // If there's one sharer in Excl/Modified state then make sure it's
                // received the data before Fwding our request.
                if (dir_lookup->state().oneSharer() && maf_waiting) {
                    if (has_maf) {
                        theMAF.setState(process->maf(), eWaitRequest);
                    } else {
                        theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
                    }
                    return;
                }

                // Select a sharer, forward the message to them
                MemoryMessage_p msg(new MemoryMessage(MemoryMessage::ReadFwd, address));
                if (req_type == MemoryMessage::FetchReq) { msg->type() = MemoryMessage::FetchFwd; }
                msg->reqSize()     = process->transport()[MemoryMessageTag]->reqSize();
                msg->ackRequired() = true;
                // If we don't have a copy of the data, request one with the final Ack
                msg->ackRequiresData() = (c_lookup->state() == CacheState::Invalid);

                rep_transport.set(MemoryMessageTag, msg);

                // Setup the destination as one of the sharers
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Source;

                int32_t sharer                               = pickSharer(dir_lookup->state(),
                                            rep_transport[DestinationTag]->requester,
                                            rep_transport[DestinationTag]->directory);
                rep_transport[DestinationTag]->source        = sharer;
                process->transport()[DestinationTag]->source = sharer;

                process->addSnoopTransport(rep_transport);
                process->setAction(eFwdAndWaitAck);

                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    process->maf() = theMAF.insert(process->transport(), address, eWaitAck, dir_lookup->state());
                }

                // The final ack might include data to store in the cache
                // Keep the Cache EB reservation so we can allocate data at that time if
                // necessary
                theMAF.setCacheEBReserved(process->maf(), process->cache_eb_reserved);
                process->cache_eb_reserved = 0;

                switch (req_type) {
                    case MemoryMessage::ReadReq: theStats.ReadMissPeer++; break;
                    case MemoryMessage::FetchReq: theStats.FetchMissPeer++; break;
                    default: DBG_Assert(false); break;
                }

            } else {

                // We don't need to forward the request, just do a cache lookup and send a
                // reply
                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissReply, address));
                if (dir_lookup->state().noSharers() && c_lookup->state() != CacheState::Shared &&
                    req_type != MemoryMessage::FetchReq) {
                    rep_msg->type() = MemoryMessage::MissReplyWritable;

                    if (c_lookup->state() == CacheState::Modified) {
                        rep_msg->type() = MemoryMessage::MissReplyDirty;
                        c_lookup->setState(CacheState::Invalid);
                        theCache->invalidateBlock(c_lookup);
                    }

                    // If there's a previous active request, then it's going to get
                    // exclusive state Wait until that request completes and then do a Fwd
                    // to remove Exlcusive state
                    if (maf_waiting) {
                        if (has_maf) {
                            theMAF.setState(process->maf(), eWaitRequest);
                        } else {
                            theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
                        }
                        return;
                    }
                }
                if (req_type == MemoryMessage::FetchReq) { rep_msg->type() = MemoryMessage::FetchReply; }

                rep_msg->reqSize()         = theCMPCacheInfo.theBlockSize;
                rep_msg->ackRequired()     = true;
                rep_msg->ackRequiresData() = false;
                dir_lookup->addSharer(requester);

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;

                process->setReplyTransport(rep_transport);
                process->setRequiresData(true);

                // record the access
                theCache->recordAccess(c_lookup);
                if (c_lookup->state().prefetched()) { c_lookup->setPrefetched(false); }

                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    process->maf() = (theMAF.insert(process->transport(), address, eWaitAck, dir_lookup->state()));
                }
                process->setAction(eReply);

                tracker->setFillLevel(theCMPCacheInfo.theCacheLevel);
                // theTraceTracker.fill(theNodeId, theCMPCacheInfo.theCacheLevel, address,
                // *tracker->fillLevel(), (req_type == MemoryMessage::FetchReq), false);

                switch (req_type) {
                    case MemoryMessage::ReadReq: theStats.ReadHit++; break;
                    case MemoryMessage::FetchReq: theStats.FetchHit++; break;
                    default: DBG_Assert(false); break;
                }
            }

            break;
        }
        case MemoryMessage::WriteReq: {

            // If there are no sharers, reply directly using the copy in the cache
            // If there are sharers, we need to send some invalidates, and potentially a
            // forward request

            if (dir_lookup->state().noSharers()) {
                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissReplyWritable, address));
                if (c_lookup->state() == CacheState::Modified) { rep_msg->type() = MemoryMessage::MissReplyDirty; }
                rep_msg->reqSize()         = theCMPCacheInfo.theBlockSize;
                rep_msg->ackRequired()     = true;
                rep_msg->ackRequiresData() = false;
                dir_lookup->addSharer(requester);

                // Invalidate our copy
                if (c_lookup->state() != CacheState::Invalid) {
                    c_lookup->setState(CacheState::Invalid);
                    theCache->invalidateBlock(c_lookup);
                }

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;

                process->setReplyTransport(rep_transport);
                process->setRequiresData(true);

                // record the access
                theCache->recordAccess(c_lookup);
                if (c_lookup->state().prefetched()) { c_lookup->setPrefetched(false); }

                process->setAction(eReply);
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    process->maf() = theMAF.insert(process->transport(), address, eWaitAck, dir_lookup->state());
                }

                // Set Outstanding Msgs so Cache knows this is final
                rep_msg->outstandingMsgs() = -1;

                tracker->setFillLevel(theCMPCacheInfo.theCacheLevel);
                // theTraceTracker.fill(theNodeId, theCMPCacheInfo.theCacheLevel, address,
                // *tracker->fillLevel(), false, true);

                theStats.WriteHit++;

            } else {

                bool needs_invalidates    = true;
                bool notify_contains_data = true;
                int32_t sharer            = -1;

                // If we have Exclusive Data and there's only ONE sharer
                // then it MIGHT have a Dirty copy, and we need to forward
                // Otherwise, if we have NO copy, then we need to forward

                if ((c_lookup->state() == CacheState::Invalid) ||
                    ((c_lookup->state() == CacheState::Exclusive) && dir_lookup->state().oneSharer())) {
                    // Need  to forward data from a sharer

                    // First, pick a sharer
                    sharer = pickSharer(dir_lookup->state(),
                                        process->transport()[DestinationTag]->requester,
                                        process->transport()[DestinationTag]->directory);

                    // Now, setup the forward
                    MemoryMessage_p fwd_msg(new MemoryMessage(MemoryMessage::WriteFwd, address));
                    fwd_msg->reqSize() = process->transport()[MemoryMessageTag]->reqSize();

                    MemoryTransport fwd_transport(process->transport());
                    fwd_transport.set(MemoryMessageTag, fwd_msg);
                    fwd_transport.set(
                      DestinationTag,
                      DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                    fwd_transport[DestinationTag]->type   = DestinationMessage::Source;
                    fwd_transport[DestinationTag]->source = sharer;
                    process->addSnoopTransport(fwd_transport);

                    if (dir_lookup->state().oneSharer()) { needs_invalidates = false; }
                    notify_contains_data = false;
                }

                // Now setup the invalidates if necessary
                if (needs_invalidates) {
                    MemoryMessage_p inv_msg(new MemoryMessage(MemoryMessage::Invalidate, address));
                    // inv_msg->reqSize() =
                    // process->transport()[MemoryMessageTag]->reqSize();
                    // Make sure size is 0 or we might accidentally get back data we didn't
                    // ask for
                    inv_msg->reqSize() = 0;
                    MemoryTransport inv_transport(process->transport());
                    inv_transport.set(MemoryMessageTag, inv_msg);
                    inv_transport.set(
                      DestinationTag,
                      DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                    inv_transport[DestinationTag]->type = DestinationMessage::Multicast;
                    dir_lookup->state().getOtherSharers(inv_transport[DestinationTag]->multicast_list, sharer);
                    DBG_Assert(inv_transport[DestinationTag]->multicast_list.size() > 0,
                               (<< "No Invalidates needed for " << *process->transport()[MemoryMessageTag]));
                    process->addSnoopTransport(inv_transport);
                }

                // Finally setup the reply msg
                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissNotify, address));
                rep_msg->outstandingMsgs() = dir_lookup->state().countSharers();

                if (notify_contains_data) {
                    process->setRequiresData(true);
                    theCache->recordAccess(c_lookup);
                    // let the requester know the notify contains data
                    rep_msg->type()    = MemoryMessage::MissNotifyData;
                    rep_msg->reqSize() = theCMPCacheInfo.theBlockSize;
                    DBG_(Trace,
                         (<< theCMPCacheInfo.theName << " sending MissNotifyData, cstate = " << c_lookup->state()
                          << ", sharers = " << dir_lookup->state().getSharers()
                          << ", Req = " << *process->transport()[MemoryMessageTag]));
                    tracker->setFillLevel(theCMPCacheInfo.theCacheLevel);
                    theStats.WriteMissInvalidatesOnly++;
                } else {
                    theStats.WriteMissInvalidatesAndData++;
                }
                rep_msg->ackRequired()     = true;
                rep_msg->ackRequiresData() = false;

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;
                process->setReplyTransport(rep_transport);
                process->setAction(eNotifyAndWaitAck);

                process->transport()[DestinationTag]->source = sharer;

                // Invalidate our copy
                if (c_lookup->state() != CacheState::Invalid) { c_lookup->setState(CacheState::Invalid); }

                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
                }
            }
            break;
        }
        case MemoryMessage::UpgradeReq:
            if (dir_lookup->state().oneSharer() || dir_lookup->state().noSharers()) {
                // Assert sharer == requester
                DBG_Assert(process->transport()[DestinationTag]->requester == dir_lookup->state().getFirstSharer());

                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::UpgradeReply, address));
                // rep_msg->ackRequired() = false;

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;

                process->setReplyTransport(rep_transport);

                if (c_lookup->state() != CacheState::Invalid) {
                    // Invalidate our copy
                    c_lookup->setState(CacheState::Invalid);
                }

                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    process->maf() = theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
                }
                process->setAction(eReply);

                rep_msg->outstandingMsgs() = -1;
                rep_msg->ackRequired()     = true;
                rep_msg->ackRequiresData() = false;

                tracker->setFillLevel(eDirectory);
                // theTraceTracker.fill(theNodeId, eDirectory, address,
                // *tracker->fillLevel(), false, false);

                theStats.UpgradeHit++;

            } else {
                MemoryMessage_p inv_msg(new MemoryMessage(MemoryMessage::Invalidate, address));
                // inv_msg->reqSize() = process->transport()[MemoryMessageTag]->reqSize();
                inv_msg->reqSize() = 0;
                MemoryTransport inv_transport(process->transport());
                inv_transport.set(MemoryMessageTag, inv_msg);
                inv_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                inv_transport[DestinationTag]->type = DestinationMessage::Multicast;
                dir_lookup->state().getOtherSharers(inv_transport[DestinationTag]->multicast_list,
                                                    inv_transport[DestinationTag]->requester);
                process->addSnoopTransport(inv_transport);

                // Finally setup the reply msg
                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissNotify, address));
                rep_msg->outstandingMsgs() = dir_lookup->state().countSharers() - 1;
                rep_msg->ackRequired()     = true;
                rep_msg->ackRequiresData() = false;

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;
                process->setReplyTransport(rep_transport);

                process->setAction(eNotifyAndWaitAck);

                process->transport()[DestinationTag]->source = process->transport()[DestinationTag]->requester;

                // Invalidate our copy
                if (c_lookup->state() != CacheState::Invalid) { c_lookup->setState(CacheState::Invalid); }

                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
                }
                theStats.UpgradeMiss++;
            }
            break;
        case MemoryMessage::NonAllocatingStoreReq: {
            // TODO: Don't be so lazy, fix this code to handle NAS requests properly
            if (c_lookup->state() == CacheState::Invalid) {
                // Cheat and ignore the on-chip sharers
                // These should be rare enough that it won't make any difference
                // This should really invalidate all of the lines too
                process->transport()[DestinationTag]->type = DestinationMessage::Memory;
                process->addSnoopTransport(process->transport());
                process->setAction(eFwdAndWaitAck);
                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
                }
                theStats.NASMissInvalidatesAndData++;
            } else {
                theCache->recordAccess(c_lookup);

                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::NonAllocatingStoreReply, address));
                rep_msg->ackRequired()     = true;
                rep_msg->ackRequiresData() = false;
                rep_msg->reqSize()         = process->transport()[MemoryMessageTag]->reqSize();

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;

                process->setReplyTransport(rep_transport);
                process->setRequiresData(true);

                if (has_maf) {
                    theMAF.setState(process->maf(), eWaitAck);
                } else {
                    theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
                }
                process->setAction(eReply);

                tracker->setFillLevel(theCMPCacheInfo.theCacheLevel);
                // theTraceTracker.fill(theNodeId, theCMPCacheInfo.theCacheLevel, address,
                // *tracker->fillLevel(), false, true);
                theStats.NASHit++;
            }

            break;
        }
        default:
            DBG_Assert(false, (<< "Unknown Request type received: " << *(process->transport()[MemoryMessageTag])));
            break;
    }
}

void
NonInclusiveMESIPolicy::handleSnoop(ProcessEntry_p process)
{
    DBG_Assert(false, (<< "Received unexpected Snoop: " << *(process->transport()[MemoryMessageTag])));
    if (process->transport()[MemoryMessageTag]->isEvictType()) { return doEvict(process, false); }
    DBG_Assert(false, (<< "Received unexpected Snoop Message: " << process->transport()[MemoryMessageTag]));
}

// Handle incoming evict messages
void
NonInclusiveMESIPolicy::doEvict(ProcessEntry_p process, bool has_maf)
{
    MemoryMessage_p req   = process->transport()[MemoryMessageTag];
    MemoryAddress address = req->address();

    DBG_(Trace, (<< theCMPCacheInfo.theName << " - doEvict() for " << *req));

    int32_t source = process->transport()[DestinationTag]->requester;

    // We received an Evict msg
    DBG_Assert(req->isEvictType(), (<< "Directory received Non-Evict Snoop msg: " << (*req)));

    process->setAction(eNoAction);

    // Look for active entries in the MAF that might require special actions
    maf_iter_t maf, last;
    std::tie(maf, last) = theMAF.findAll(req->address(), eWaitAck);

    for (maf_iter_t iter = maf; iter != last && iter != theMAF.end(); iter++) {
        if (iter->transport()[MemoryMessageTag]->address() != address) { continue; }
        if (iter->state() != eWaitAck) { continue; }
        if (iter->transport()[DestinationTag]->requester == source) {
            DBG_(Trace,
                 (<< "Found maf waiting for ack from evicting core, stalling: " << *req << " while waiting for "
                  << *(iter->transport()[MemoryMessageTag])));
            if (has_maf) {
                theMAF.setState(process->maf(), eWaitRequest);
            } else {
                theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
            }
            process->setAction(eStall);
            return;
        }
    }

    // We can ignore outstanding Reads/Fetches, they will complete as normal
    // before the EvictAck arrives Writes/Upgrades will complete as normal but
    // will get rid of the block, so we DON'T need to send an Ack

    bool active_write_req = false;
    if (maf != theMAF.end() && (maf != last) &&
        ((maf->transport()[MemoryMessageTag]->type() == MemoryMessage::WriteReq) ||
         (maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq))) {
        active_write_req = true;
    }

    MemoryTransport ack(process->transport());
    ack.set(MemoryMessageTag, new MemoryMessage(MemoryMessage::EvictAck, req->address()));
    ack.set(DestinationTag, new DestinationMessage(process->transport()[DestinationTag]));
    ack[DestinationTag]->type = DestinationMessage::Requester;

    bool requires_fwd = false;

    if (req->ackRequired()) {
        process->addSnoopTransport(ack);
        requires_fwd = true;
    }

    // If we have any back invalidates pending for this block, we can equally
    // ignore the evict and wait for the back inval to handle things.

    // Find Directory info in Directory or Dir Evict Buffer
    DirLookupResult_p dir_lookup          = theDirectory->lookup(address);
    const AbstractDirEBEntry<State>* d_eb = theDirEvictBuffer->find(req->address());

    // check if this is a redundant evict
    // This occurs when we've already invalidated the block
    bool valid_sharer = false;
    if (dir_lookup->found()) {
        valid_sharer = dir_lookup->state().isSharer(source);
    } else if (d_eb != nullptr) {
        valid_sharer = d_eb->state().isSharer(source);
    }

    // Only add Ack when we remove the sharer
    // If there was a race with a Write/Upgrade the sharer might already be
    // removed in that case we don't need to send the evict
    // process->addSnoopTransport(ack);

    // If the Evict contains data, we should write it to the array
    CacheLookupResult_p c_lookup = (*theCache)[req->address()];

    // If there was a race with an Invalidate or BackInvalidate, we need to ignore
    // any data in the evict
    if ((req->type() == MemoryMessage::EvictDirty ||
         (req->type() == MemoryMessage::EvictWritable && req->evictHasData())) &&
        valid_sharer) {

        // If we didn't find the block in the cache, check the evict buffer
        if (c_lookup->state() == CacheState::Invalid) {

            // Steal Cache EB reservations from Dir EB entry if possible
            if (d_eb != nullptr && d_eb->cacheEBReserved() > 0) {
                DBG_(Trace,
                     (<< theCMPCacheInfo.theName << " - Recovering " << d_eb->cacheEBReserved()
                      << " saved cacheEB reservations."));
                process->cache_eb_reserved += d_eb->cacheEBReserved();
                d_eb->cacheEBReserved() = 0;
            }

            // Only allocate the block if we have an EB reservation (which we probably
            // shouldn't have) Of if the EB has room
            if (process->cache_eb_reserved > 0 || CacheEBHasSpace()) {
                // Allocate the block in the cache
                CacheLookupResult_p victim = theCache->allocate(c_lookup, req->address());
                if (req->type() == MemoryMessage::EvictDirty) {
                    c_lookup->setState(CacheState::Modified);
                } else {
                    c_lookup->setState(CacheState::Exclusive);
                }

                // Make sure the block isn't sitting in the evict buffer
                CacheEvictBuffer<CacheState>::iterator c_eb =
                  theCacheEvictBuffer.find(theCache->blockAddress(req->address()));
                if (c_eb != theCacheEvictBuffer.end()) {
                    // Remove block from the evict buffer
                    theCacheEvictBuffer.remove(c_eb);
                }

                if (victim->state() != CacheState::Invalid) { evictCacheBlock(victim); }
            } else if (req->type() == MemoryMessage::EvictDirty) {
                DBG_(Trace,
                     (<< theCMPCacheInfo.theName << " Forwarding evict to memory. cache_eb_reserved = "
                      << process->cache_eb_reserved << ", CacheEBHasSpace = " << std::boolalpha << CacheEBHasSpace()
                      << ", Msg = " << *(process->transport()[MemoryMessageTag])));
                process->transport()[DestinationTag]->type = DestinationMessage::Memory;
                process->addSnoopTransport(process->transport());
                requires_fwd = true;
            }

        } else {
            theCache->recordAccess(c_lookup);
            c_lookup->setState(CacheState::Modified);
        }
    }

    if (d_eb != nullptr) {
        DBG_Assert(!active_write_req);

        if (d_eb->state().isSharer(source)) { d_eb->state().removeSharer(source); }

        bool wake_maf = false;
        if (d_eb->invalidatesPending() == 0 && d_eb->state().noSharers()) {
            theDirEvictBuffer->remove(d_eb->address());

            maf_iter_t waiting_maf = theMAF.findFirst(req->address(), eWaitEvict);
            if (waiting_maf != theMAF.end()) {
                DBG_(Trace,
                     (<< theCMPCacheInfo.theName << " - found maf waiting on Evict of " << std::hex << address << " -> "
                      << *(waiting_maf->transport()[MemoryMessageTag])));
                DBG_Assert(waiting_maf->transport()[MemoryMessageTag]->address() == address);

                process->setMAF(waiting_maf);
                wake_maf = true;
            }
        }

        // Forward Ack and wake MAF if necessary
        if (wake_maf) {
            if (requires_fwd) {
                process->setAction(eFwdAndWakeEvictMAF);
            } else {
                process->setAction(eWakeEvictMAF);
            }
        } else {
            if (requires_fwd) {
                process->setAction(eForward);
            } else {
                process->setAction(eNoAction);
            }
        }

        return;
    }

    // If an Evict is racing with a BackInvalidate, we can process the
    // InvalidateAck before we process the evict This means it's possible for the
    // block to be gone at this point. Print a message so it's easy to track, but
    // we don't need an assertion
    // DBG_Assert( dir_lookup->found(), ( << "Directory received evict for block
    // not in directory: " << (*req) ));
    if (dir_lookup->found()) { dir_lookup->removeSharer(source); }

    // Always forward (either just Ack or Ack+Evict to mem)
    if (requires_fwd) {
        process->setAction(eForward);
    } else {
        process->setAction(eNoAction);
    }
}

void
NonInclusiveMESIPolicy::handleReply(ProcessEntry_p process)
{
    // Replies include InvalidateAck, InvUpdateAck, FetchAck, ReadAck, WriteAck,
    // UpgradeAck, EvictAck The rest refer to oustanding requests

    MemoryMessage_p req = process->transport()[MemoryMessageTag];
    int32_t requester   = process->transport()[DestinationTag]->requester;
    MemoryTransport rep_transport(process->transport());

    switch (req->type()) {
        case MemoryMessage::InvalidateAck:
        case MemoryMessage::InvUpdateAck: {
            // InvalidateAck+InvUpdateAck imply there is an outstanding Eviction
            process->setAction(eNoAction);

            const AbstractDirEBEntry<State>* d_eb = theDirEvictBuffer->find(req->address());
            DBG_Assert(d_eb != nullptr);
            if (d_eb->state().isSharer(process->transport()[DestinationTag]->other)) {
                d_eb->state().removeSharer(process->transport()[DestinationTag]->other);
            }

            d_eb->completeInvalidate();

            if (req->type() == MemoryMessage::InvUpdateAck) {
                // We write the dirty data to the cache instead of forwarding it to memory

                // First, check current block status
                CacheLookupResult_p c_lookup = (*theCache)[req->address()];

                if (c_lookup->state() == CacheState::Invalid) {
                    // double check that the block is NOT in the evict buffer
                    // (we shouldn't be trying to write-back data if there's a dirty copy
                    // sending us an InvUpdateAck)
                    CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());
                    DBG_Assert(c_eb == theCacheEvictBuffer.end());

                    // Allocate the block in the cache
                    CacheLookupResult_p victim = theCache->allocate(c_lookup, req->address());
                    DBG_(Trace,
                         (<< " allocating block on InvUpdateAck, evicting block " << std::hex << victim->blockAddress()
                          << " in state " << victim->state() << " : " << *req));

                    if (victim->state() != CacheState::Invalid) { evictCacheBlock(victim); }
                }

                c_lookup->setState(CacheState::Modified);
                process->setRequiresData(true);

                // We can give up our Cache EB reservations now because there can only be
                // one Dirty copy
                DBG_Assert(d_eb->cacheEBReserved() > 0 || c_lookup->state() == CacheState::Modified);
                process->cache_eb_reserved += d_eb->cacheEBReserved();
                d_eb->cacheEBReserved() = 0;
            }

            if (d_eb->invalidatesPending() == 0) {
                DBG_Assert(d_eb->state().noSharers());
                // Make sure we release any Cache EB reservations
                if (d_eb->cacheEBReserved() > 0) {
                    process->cache_eb_reserved = d_eb->cacheEBReserved();
                    d_eb->cacheEBReserved()    = 0;
                }

                theDirEvictBuffer->remove(d_eb->address());

                maf_iter_t waiting_maf = theMAF.findFirst(req->address(), eWaitEvict);
                if (waiting_maf != theMAF.end()) {
                    process->setMAF(waiting_maf);
                    process->setAction(eWakeEvictMAF);
                }
            } else {
                // Make sure we have pending invalidates for any remaining sharers
                DBG_Assert(d_eb->invalidatesPending() != 0);
            }

            break;
        }
        case MemoryMessage::FetchAckDirty:
        case MemoryMessage::ReadAckDirty:
        case MemoryMessage::FetchAck:
        case MemoryMessage::ReadAck: {
            maf_iter_t first, last;
            std::tie(first, last) = theMAF.findAll(req->address(), eWaitAck);
            DBG_Assert(first != theMAF.end(), (<< "Unable to find MAF waiting for Ack: " << *req));
            for (; first != last; first++) {
                DBG_Assert(first != theMAF.end(), (<< "Unable to find MAF waiting for Ack: " << *req));
                // The matching request is the one with the same requester
                if (first->transport()[DestinationTag]->requester == requester) {
                    process->setMAF(first);
                    break;
                }
            }
            DBG_Assert(first != last, (<< "No matching MAF found for " << *req));

            // Now that we have the MAF, return any Cache EB reservations to the process
            // so they can be cleared.
            if (first->cacheEBReserved() > 0) {
                process->cache_eb_reserved = first->cacheEBReserved();
                theMAF.setCacheEBReserved(first, 0);
            }

            // Do a directory lookup and set the state
            DirLookupResult_p d_lookup = theDirectory->lookup(req->address());
            DBG_Assert(d_lookup->found(), (<< "Received ACK but couldn't find matching directory entry: " << *req));
            if (!d_lookup->state().isSharer(requester)) { d_lookup->addSharer(requester); }

            // When we try to Wake other MAF's we'll make sure there aren't any active
            // ones We'll also remove the protected bit at that point.
            process->setAction(eRemoveAndWakeMAF);

            // Check if the request included data, and write it to the cache if it did
            if (req->ackRequiresData()) {

                CacheLookupResult_p c_lookup = (*theCache)[req->address()];

                if (c_lookup->state() == CacheState::Invalid) {
                    // double check that the block is NOT in the evict buffer
                    // (we shouldn't be trying to write-back data if there's a dirty copy
                    // sending us an InvUpdateAck)
                    CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());
                    DBG_Assert(c_eb == theCacheEvictBuffer.end());

                    // Allocate the block in the cache
                    CacheLookupResult_p victim = theCache->allocate(c_lookup, req->address());
                    DBG_(Trace,
                         (<< " allocating block " << std::hex << req->address() << " on Ack, evicting block "
                          << std::hex << victim->blockAddress() << " in state " << victim->state() << " : " << *req));

                    if (victim->state() != CacheState::Invalid) { evictCacheBlock(victim); }
                }

                // c_lookup->setState(CacheState::Modified);
                process->setRequiresData(true);

                if ((req->type() == MemoryMessage::FetchAckDirty) || (req->type() == MemoryMessage::ReadAckDirty)) {
                    c_lookup->setState(CacheState::Modified);
                } else if (req->type() == MemoryMessage::FetchAck && c_lookup->state() != CacheState::Modified) {
                    c_lookup->setState(CacheState::Shared);
                } else if (req->type() == MemoryMessage::ReadAck && c_lookup->state() != CacheState::Modified) {
                    c_lookup->setState(CacheState::Exclusive);
                }
            }

            break;
        }
        case MemoryMessage::NASAck: {

            maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
            DBG_Assert(maf != theMAF.end() &&
                       maf->transport()[MemoryMessageTag]->type() == MemoryMessage::NonAllocatingStoreReq);
            process->setMAF(maf);

            if (maf->cacheEBReserved() > 0) {
                process->cache_eb_reserved = maf->cacheEBReserved();
                theMAF.setCacheEBReserved(maf, 0);
            }

            // remove this maf and look for stalled requests
            process->setAction(eRemoveAndWakeMAF);

            break;
        }
        case MemoryMessage::WriteAck: {

            maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
            DBG_Assert(maf != theMAF.end(), (<< "No MAF waiting for ack: " << *req));
            DBG_Assert(maf->transport()[MemoryMessageTag]->type() == MemoryMessage::WriteReq,
                       (<< "Received WriteAck in response to non-write: " << *(maf->transport()[MemoryMessageTag])));
            DBG_Assert(maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::WriteReq);

            // Do a directory lookup and set the state
            DirLookupResult_p d_lookup = theDirectory->lookup(req->address());
            DBG_Assert(d_lookup->found(),
                       (<< theCMPCacheInfo.theName << "Received WriteAck for unfound block: " << *req));
            d_lookup->setSharer(requester);
            d_lookup->setProtected(false);

            CacheLookupResult_p c_lookup = (*theCache)[req->address()];

            if (c_lookup->state() == CacheState::Invalid) {
                // double check that the block is NOT in the evict buffer
                // (we shouldn't be trying to write-back data if there's a dirty copy
                // sending us an InvUpdateAck)
                CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());

                if (c_eb != theCacheEvictBuffer.end()) {
                    if (theCMPCacheInfo.theEvictClean) {
                        c_eb->state() = CacheState::Exclusive;
                        c_eb->type()  = MemoryMessage::EvictClean;
                    } else {
                        DBG_(Trace,
                             (<< theCMPCacheInfo.theName << " Removing CEB entry for block " << std::hex
                              << req->address()));
                        theCacheEvictBuffer.remove(c_eb);
                    }
                }
            } else if (c_lookup->state() != CacheState::Invalid) {
                c_lookup->setState(CacheState::Exclusive);
            }

            process->setMAF(maf);
            if (maf->cacheEBReserved() > 0) {
                process->cache_eb_reserved = maf->cacheEBReserved();
                theMAF.setCacheEBReserved(maf, 0);
            }

            // remove this maf and look for stalled requests
            process->setAction(eRemoveAndWakeMAF);

            break;
        }
        case MemoryMessage::UpgradeAck: {
            maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
            DBG_Assert(maf != theMAF.end(), (<< "No matching MAF for request: " << *req));
            DBG_Assert(maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq,
                       (<< "Matching MAF is not an Upgrade. MAF = " << *(maf->transport()[MemoryMessageTag])
                        << " reply = " << *req));
            DBG_Assert(maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq);

            // Do a directory lookup and set the state
            DirLookupResult_p d_lookup = theDirectory->lookup(req->address());
            DBG_Assert(d_lookup->found());
            d_lookup->setSharer(requester);
            d_lookup->setProtected(false);

            CacheLookupResult_p c_lookup = (*theCache)[req->address()];
            if (c_lookup->state() == CacheState::Invalid) {
                // double check that the block is NOT in the evict buffer
                // (we shouldn't be trying to write-back data if there's a dirty copy
                // sending us an InvUpdateAck)
                CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());

                if (c_eb != theCacheEvictBuffer.end()) {
                    if (theCMPCacheInfo.theEvictClean) {
                        c_eb->state() = CacheState::Exclusive;
                        c_eb->type()  = MemoryMessage::EvictClean;
                    } else {
                        DBG_(Trace,
                             (<< theCMPCacheInfo.theName << " Removing CEB entry for block " << std::hex
                              << req->address()));
                        theCacheEvictBuffer.remove(c_eb);
                    }
                }
            } else if (c_lookup->state() != CacheState::Invalid) {
                c_lookup->setState(CacheState::Exclusive);
            }

            process->setMAF(maf);
            if (maf->cacheEBReserved() > 0) {
                process->cache_eb_reserved = maf->cacheEBReserved();
                theMAF.setCacheEBReserved(maf, 0);
            }

            // remove this maf and look for stalled requests
            process->setAction(eRemoveAndWakeMAF);

            break;
        }

        case MemoryMessage::NonAllocatingStoreReply: {
            // We don't bother with MAF entries for NAS requests
            // Just forward the reply to the requester

            maf_iter_t first, last;
            std::tie(first, last) = theMAF.findAll(req->address(), eWaitAck);
            DBG_Assert(first != theMAF.end());
            for (; first != last; first++) {
                // The matching request is the one with the same requester
                if (first->transport()[DestinationTag]->requester == requester) {
                    process->setMAF(first);
                    break;
                }
            }
            DBG_Assert(first != last, (<< "No matching MAF found for " << *req));

            process->setAction(eReplyAndRemoveMAF);

            rep_transport.set(MemoryMessageTag, req);
            rep_transport.set(DestinationTag,
                              DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
            rep_transport[DestinationTag]->type = DestinationMessage::Requester;

            // Make sure the requester knows not to send us an acknowledgment
            req->ackRequired() = false;

            process->setReplyTransport(rep_transport);

            break;
        }

        // Reply from Memory
        case MemoryMessage::FetchReply:
        case MemoryMessage::MissReply:
        case MemoryMessage::MissReplyWritable: {
            // Sometimes memory replies directly to us instead of replying to the
            // requester For Reads/Fetches, store data in the cache For all requests,
            // update sharing information and forward reply to requester (with no Ack
            // required)

            maf_iter_t first, last;
            std::tie(first, last) = theMAF.findAll(req->address(), eWaitAck);
            DBG_Assert(first != theMAF.end());
            for (; first != last; first++) {
                // The matching request is the one with the same requester
                if (first->transport()[DestinationTag]->requester == requester) {
                    process->setMAF(first);
                    break;
                }
            }
            DBG_Assert(first != last, (<< "No matching MAF found for " << *req));

            // Now that we have the MAF, return any Cache EB reservations to the process
            // so they can be cleared.
            if (first->cacheEBReserved() > 0) {
                process->cache_eb_reserved = first->cacheEBReserved();
                theMAF.setCacheEBReserved(first, 0);
            }

            rep_transport.set(MemoryMessageTag, req);
            rep_transport.set(DestinationTag,
                              DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
            rep_transport[DestinationTag]->type = DestinationMessage::Requester;

            process->setAction(eReply);

            process->setReplyTransport(rep_transport);

            // If this is a Read or Fetch Request, put the data in the cache
            if ((first->transport()[MemoryMessageTag]->type() == MemoryMessage::ReadReq) ||
                (first->transport()[MemoryMessageTag]->type() == MemoryMessage::FetchReq)) {

                CacheLookupResult_p c_lookup = (*theCache)[req->address()];

                if (c_lookup->state() == CacheState::Invalid) {
                    // double check that the block is NOT in the evict buffer
                    // (we shouldn't be trying to write-back data if there's a dirty copy
                    // sending us an InvUpdateAck)
                    CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());
                    DBG_Assert(c_eb == theCacheEvictBuffer.end());

                    // Allocate the block in the cache
                    CacheLookupResult_p victim = theCache->allocate(c_lookup, req->address());
                    DBG_(Trace,
                         (<< " allocating block on receiving " << req->type() << ", evicting block " << std::hex
                          << victim->blockAddress() << " in state " << victim->state() << " : " << *req));

                    if (victim->state() != CacheState::Invalid) { evictCacheBlock(victim); }
                }

                process->setRequiresData(true);
                process->setTransmitAfterTag(true);

                c_lookup->setState(CacheState::Shared);
                if ((first->transport()[MemoryMessageTag]->type() != MemoryMessage::FetchReq) &&
                    (req->type() == MemoryMessage::MissReplyWritable)) {
                    c_lookup->setState(CacheState::Exclusive);
                }
            }

            break;
        }
        case MemoryMessage::FwdNAck: {

            DirLookupResult_p d_lookup = theDirectory->lookup(req->address());
            DBG_Assert(d_lookup->found());

            // Need to find the missing request
            maf_iter_t first, last;
            std::tie(first, last) = theMAF.findAll(req->address(), eWaitAck);
            DBG_Assert(first != theMAF.end());
            bool multiple_active_requests = false;
            for (; first != last; first++) {
                // The matching request is the one with the same requester
                if (first->transport()[DestinationTag]->requester == requester) { break; }
                multiple_active_requests = true;
            }
            DBG_Assert(first != last);
            process->setMAF(first);

            if (!multiple_active_requests) {
                maf_iter_t next = first;
                next++;
                if (next != last) { multiple_active_requests = true; }
            }

            // It's possible another sharer evicted a dirty copy which was allocated in
            // the cache Check the cache for a copy
            CacheLookupResult_p c_lookup = (*theCache)[req->address()];

            // If we didn't find the block in the cache, check the evict buffer
            if (c_lookup->state() == CacheState::Invalid) {
                CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());
                if (c_eb != theCacheEvictBuffer.end()) {
                    // Move the block from the evict buffer into the cache
                    if (!c_eb->pending()) {

                        CacheState block_state     = c_eb->state();
                        CacheLookupResult_p victim = theCache->allocate(c_lookup, req->address());
                        theCacheEvictBuffer.remove(c_eb);
                        c_lookup->setState(block_state);

                        DBG_(Trace,
                             (<< " replaceing EB entry, evicting block in state " << victim->state() << " : " << *req));
                        if (victim->state() != CacheState::Invalid) { evictCacheBlock(victim); }
                    }
                }
            }

            if (c_lookup->state() != CacheState::Invalid) {
                MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissReply, req->address()));
                if (d_lookup->state().noSharers() && c_lookup->state() != CacheState::Shared) {
                    rep_msg->type() = MemoryMessage::MissReplyWritable;
                }

                // if (d_lookup->state().oneSharer() && c_lookup->state() != CacheState::Shared) {
                //     // We're racing with an evict of a potentially dirty block
                //     // 2 Phase Evict should have handled this case, so something went wrong
                //     DBG_Assert(false, (<< "Unexpected race condition detected."));
                // }

                rep_msg->reqSize() = theCMPCacheInfo.theBlockSize;
                process->setAction(eReply);

                rep_transport.set(MemoryMessageTag, rep_msg);
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Requester;

                process->setReplyTransport(rep_transport);
                process->setRequiresData(true);

                TransactionTracker_p tracker = process->transport()[TransactionTrackerTag];
                tracker->setFillLevel(theCMPCacheInfo.theCacheLevel);

                // record the access
                theCache->recordAccess(c_lookup);
                if (c_lookup->state().prefetched()) { c_lookup->setPrefetched(false); }

                theMAF.setState(first, eWaitAck);
            } else if (d_lookup->state().noSharers()) {
                // Send the original message on to memory
                first->transport()[DestinationTag]->type   = DestinationMessage::Memory;
                first->transport()[DestinationTag]->source = -1;
                process->addSnoopTransport(first->transport());
                process->setAction(eForward);
            } else {
                // Select a sharer, forward the message to them
                auto req   = first->transport()[MemoryMessageTag];
                auto msg_a = req->address();

                MemoryMessage_p msg(new MemoryMessage(MemoryMessage::ReadFwd, msg_a));

                if (first->transport()[MemoryMessageTag]->type() == MemoryMessage::FetchReq) {
                    msg->type() = MemoryMessage::FetchFwd;
                }
                msg->reqSize() = first->transport()[MemoryMessageTag]->reqSize();

                rep_transport.set(MemoryMessageTag, msg);

                // Setup the destination as one of the sharers
                rep_transport.set(DestinationTag,
                                  DestinationMessage_p(new DestinationMessage(first->transport()[DestinationTag])));
                rep_transport[DestinationTag]->type = DestinationMessage::Source;

                int32_t sharer                        = pickSharer(d_lookup->state(),
                                            rep_transport[DestinationTag]->requester,
                                            rep_transport[DestinationTag]->directory);
                rep_transport[DestinationTag]->source = sharer;

                process->addSnoopTransport(rep_transport);
                process->setAction(eForward);
            }

            break;
        }
        case MemoryMessage::EvictAck: {
            CacheEvictBuffer<CacheState>::iterator c_eb = theCacheEvictBuffer.find(req->address());
            DBG_Assert(c_eb != theCacheEvictBuffer.end());
            DBG_Assert(c_eb->pending());

            // Remove the block from the evict buffer
            theCacheEvictBuffer.remove(c_eb);

            process->setAction(eNoAction);

            maf_iter_t waiting_maf = theMAF.findFirst(req->address(), eWaitEvict);
            if (waiting_maf != theMAF.end()) {
                process->setMAF(waiting_maf);
                process->setAction(eWakeEvictMAF);
            }

            break;
        }
        default: DBG_Assert(false, (<< "Un-expected reply received at directory: " << *req)); break;
    }
}

void
NonInclusiveMESIPolicy::handleWakeMAF(ProcessEntry_p process)
{
    doRequest(process, true);
}

// TODO: Continue from HERE!
// Handle Evict Process to evict things from the CMP cache

void
NonInclusiveMESIPolicy::handleCacheEvict(ProcessEntry_p process)
{
    process->addSnoopTransport(process->transport());
    process->setAction(eForward);
}

void
NonInclusiveMESIPolicy::handleDirEvict(ProcessEntry_p process)
{
    // Double-check that this is a valid Dir Eviction
    DBG_Assert(process->transport()[MemoryMessageTag]->type() == MemoryMessage::BackInvalidate);

    process->addSnoopTransport(process->transport());
    process->setAction(eForward);

    // Steal the Cache EB reservation, it's been copied into the EB entry
    process->cache_eb_reserved = 0;
}

void
NonInclusiveMESIPolicy::handleIdleWork(ProcessEntry_p process)
{
    if (process->transport()[MemoryMessageTag]->type() == MemoryMessage::NumMemoryMessageTypes) {
        // This can either mean cache eviciton pressure, or we're simply trying to
        // do dir idle work

        if (theDirectory->idleWorkReady() && (theDirIdleTurn || !theCache->evictionResourcePressure())) {
            if (theCache->evictionResourcePressure()) { theDirIdleTurn = false; }

            // The directory has some Idle Work it wants to do
            // perform a lookup, but no other action
            process->setLookupsRequired(theDirectory->doIdleWork());
            process->setAction(eNoAction);
        } else if (theCache->evictionResourcePressure()) {
            if (theDirectory->idleWorkReady()) { theDirIdleTurn = true; }

            // by default, 1 tag lookup, no action
            process->setLookupsRequired(1);
            process->setAction(eNoAction);

            // Do idle work for the cache
            // This involves moving a block from the cache to the EB.
            std::pair<CacheState, MemoryAddress> victim = theCache->getPreemptiveEviction();
            MemoryMessage::MemoryMessageType evict_type = MemoryMessage::EvictClean;

            if (victim.first == CacheState::Invalid) {
                return;
            } else if ((victim.first == CacheState::Modified) || (victim.first == CacheState::Owned)) {
                evict_type = MemoryMessage::EvictDirty;
            } else if (victim.first == CacheState::Exclusive) {
                evict_type = MemoryMessage::EvictWritable;
            }

            if (theCMPCacheInfo.theEvictClean || (evict_type == MemoryMessage::EvictDirty)) {
                process->setRequiresData(1);
                theCacheEvictBuffer.allocEntry(victim.second, evict_type, victim.first);
                // theTraceTracker.eviction(theNodeId, theCacheLevel,
                // victim->blockAddress(), false);

                DBG_(Trace, (<< " Adding " << std::hex << victim.second << " to the EvictBuffer."));
            }
        } else if (!theCacheEvictBuffer.empty()) {
            if (theCacheEvictBuffer.evictableReady()) {
                // Noting else to do, let's just evict a cache block
                MemoryTransport new_transport = getCacheEvictTransport();

                process->transport() = new_transport;
                handleCacheEvict(process);
            } else {
                DBG_(Dev,
                     (<< theCMPCacheInfo.theName
                      << " evictable block taken out from under us, ignoring idle "
                         "work."));
                process->setAction(eNoAction);
            }
        } else {
            process->setAction(eNoAction);
        }

    } else {
        process->addSnoopTransport(process->transport());
        process->setAction(eForward);

        // Steal the Cache EB reservation, it's been copied into the EB entry
        process->cache_eb_reserved = 0;
    }
}

bool
NonInclusiveMESIPolicy::isQuiesced() const
{
    // Empty MAF and empty EB
    return theDirEvictBuffer->empty() && theCacheEvictBuffer.empty() && theMAF.empty();
}

bool
NonInclusiveMESIPolicy::hasIdleWorkAvailable()
{
    // Need to check if any evict buffer entries have un-sent invalidates
    if (!theCacheEvictBuffer.empty() && theCacheEvictBuffer.evictableReady()) {
        return true;
    } else if (!theCacheEvictBuffer.full()) {
        return (theDirectory->idleWorkReady() || theCache->evictionResourcePressure());
    }
    return false;
}

MemoryTransport
NonInclusiveMESIPolicy::getCacheEvictTransport()
{

    // Get the block at the head of the evict buffer
    DBG_Assert(!theCacheEvictBuffer.empty(),
               (<< "Tried to get EvicTransport from Empty EB (used/reserved/total) = ( "
                << theCacheEvictBuffer.usedEntries() << "/" << theCacheEvictBuffer.reservedEntries() << "/"
                << theCacheEvictBuffer.size() << ")"));

    MemoryMessage_p msg(theCacheEvictBuffer.firstNonPending());
    msg->reqSize()     = theCMPCacheInfo.theBlockSize;
    msg->ackRequired() = true;

    if (msg->type() == MemoryMessage::EvictDirty) { msg->evictHasData() = true; }

    MemoryTransport transport;

    transport.set(MemoryMessageTag, msg);
    transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(DestinationMessage::Memory)));
    transport[DestinationTag]->directory = theCMPCacheInfo.theNodeId;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(msg->address());
    tracker->setInitiator(theCMPCacheInfo.theNodeId);
    tracker->setSource(theCMPCacheInfo.theName + " Evict");
    tracker->setDelayCause(theCMPCacheInfo.theName, "Evict");
    transport.set(TransactionTrackerTag, tracker);

    return transport;
}

MemoryTransport
NonInclusiveMESIPolicy::getIdleWorkTransport()
{
    MemoryTransport transport;

    const AbstractDirEBEntry<State>* d_eb = theDirEvictBuffer->oldestRequiringInvalidates();
    // DBG_Assert(d_eb != nullptr, ( << "Called getIdleWorkTransport while no
    // IdleWork outstanding!"));

    if (d_eb == nullptr) {
        transport.set(MemoryMessageTag, MemoryMessage_p(new MemoryMessage(MemoryMessage::NumMemoryMessageTypes)));
        return transport;
    }

    transport.set(MemoryMessageTag, MemoryMessage_p(new MemoryMessage(MemoryMessage::BackInvalidate)));
    transport[MemoryMessageTag]->address() = d_eb->address();

    d_eb->setInvalidatesPending(d_eb->state().countSharers());

    // Add a Cache EB reservation incase we need to allocate space when a dirty
    // block is invalidated
    d_eb->cacheEBReserved() = 1;

    // Setup destinations
    transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(DestinationMessage::Multicast)));
    d_eb->state().getSharerList(transport[DestinationTag]->multicast_list);
    transport[DestinationTag]->directory = theCMPCacheInfo.theNodeId;

    // Make sure we have a transaction tracker
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(d_eb->address());
    tracker->setInitiator(theCMPCacheInfo.theNodeId);
    tracker->setSource(theCMPCacheInfo.theName + " BackInvalidate");
    tracker->setDelayCause(theCMPCacheInfo.theName, "BackInvalidate");
    transport.set(TransactionTrackerTag, tracker);

    return transport;
}

MemoryTransport
NonInclusiveMESIPolicy::getDirEvictTransport()
{
    // for us this is the same as our normal idle work

    return getIdleWorkTransport();
}

void
NonInclusiveMESIPolicy::wakeMAFs(MemoryAddress anAddress)
{

    // First, check for active MAFs
    MissAddressFile::iterator active = theMAF.findFirst(anAddress, eWaitAck);
    if (active != theMAF.end()) { return; }

    // If we had multiple simultaneous Reads/Fetches, we unprotect the block
    // here because this is when we know there are no more outstanding
    DirLookupResult_p d_lookup = theDirectory->lookup(anAddress);
    if (d_lookup->found()) { d_lookup->setProtected(false); }

    MissAddressFile::order_iterator iter = theMAF.ordered_begin();
    MissAddressFile::order_iterator next = iter;

    while (iter != theMAF.ordered_end()) {
        next++;

        if ((iter->state() == eWaitRequest) && (iter->address() == anAddress)) {
            theMAF.wake(iter);
        } else if ((iter->state() == eWaitSet) && theDirectory->sameSet(anAddress, iter->address())) {
            theMAF.wake(iter);
        }

        iter = next;
    }
}

int32_t
NonInclusiveMESIPolicy::pickSharer(const SimpleDirectoryState& state, int32_t requester, int32_t dir)
{
    if (state.isSharer(dir)) {
        return dir;
    } else {
        return state.getFirstSharer();
    }
}

void
NonInclusiveMESIPolicy::evictCacheBlock(CacheLookupResult_p victim)
{
    MemoryMessage::MemoryMessageType evict_type = MemoryMessage::EvictClean;

    if (victim->state() == CacheState::Invalid) {
        return;
    } else if ((victim->state() == CacheState::Modified) || (victim->state() == CacheState::Owned)) {
        evict_type = MemoryMessage::EvictDirty;
    } else if (victim->state() == CacheState::Exclusive) {
        evict_type = MemoryMessage::EvictWritable;
    }

    if (theCMPCacheInfo.theEvictClean || (evict_type == MemoryMessage::EvictDirty)) {
        theCacheEvictBuffer.allocEntry(victim->blockAddress(), evict_type, victim->state());
        // theTraceTracker.eviction(theNodeId, theCacheLevel,
        // victim->blockAddress(), false);
        DBG_(Trace, (<< " Adding " << std::hex << victim->blockAddress() << " to the EvictBuffer."));
    }
}

}; // namespace nCMPCache
