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

#include "../ValueTracker.hpp"
#include "coreModelImpl.hpp"

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/polymorphic_pointer_cast.hpp>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

bool
mmuASI(int32_t asi)
{
    switch (asi) {
        case 0x50: // I-MMU
        case 0x51: // I-MMU TSB_8KB_PTR_REG
        case 0x52: // I-MMU TSB_64KB_PTR_REG
        case 0x54: // ITLB_DATA_IN_REG
        case 0x57: // IMMU_DEMAP
        case 0x58: // D-MMU
        case 0x59: // D-MMU TSB_8KB_PTR_REG
        case 0x5A: // D-MMU TSB_64KB_PTR_REG
        case 0x5C: // DTLB_DATA_IN_REG
        case 0x5D: // DTLB_DATA_ACCESS_REG
        case 0x5F: // DMMU_DEMAP
            return true;

        default: // all others
            return false;
    }
}

bool
interruptASI(int32_t asi)
{
    switch (asi) {
        case 0x48: // ASI_INTR_DISPATCH_STATUS
        case 0x49: // ASI_INTR_RECEIVE
        case 0x77: // ASI_INTR_DATA_W
        case 0x7F: // ASI_INTR_DATA_R

        case 0x4a: // ASI_FIREPLANE
            return true;

        default: // all others
            return false;
    }
}

void
CoreImpl::breakMSHRLink(memq_t::index<by_insn>::type::iterator iter)
{
    FLEXUS_PROFILE();
    iter->theIssued = false;
    if (iter->theMSHR) {
        DBG_(Verb, (<< "Breaking MSHR connection of " << *iter));
        // page walk requests can be chained to normal load/store misses
        for (const auto &tr: (*iter->theMSHR)->second.theWaitingPagewalks)
            thePageWalkReissues.push_back(tr);

        (*iter->theMSHR)->second.theWaitingPagewalks.clear();
        // Break dependance on pending load
        std::list<memq_t::index<by_insn>::type::iterator>::iterator link =
          find((*iter->theMSHR)->second.theWaitingLSQs.begin(), (*iter->theMSHR)->second.theWaitingLSQs.end(), iter);
        DBG_Assert(link != (*iter->theMSHR)->second.theWaitingLSQs.end());
        (*iter->theMSHR)->second.theWaitingLSQs.erase(link);

        // Delete the MSHR and wake blocked mem ops
        if ((*iter->theMSHR)->second.theWaitingLSQs.empty()) {
            // Reissue blocked store prefetches
            std::list<boost::intrusive_ptr<Instruction>>::iterator pf_iter, pf_end;
            pf_iter = (*iter->theMSHR)->second.theBlockedPrefetches.begin();
            pf_end  = (*iter->theMSHR)->second.theBlockedPrefetches.end();
            while (pf_iter != pf_end) {

                memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(*pf_iter);
                if (lsq_entry != theMemQueue.get<by_insn>().end()) {
#ifdef VALIDATE_STORE_PREFETCHING
                    PhysicalMemoryAddress aligned =
                      PhysicalMemoryAddress(static_cast<uint64_t>(lsq_entry->thePaddr) & ~(theCoherenceUnit - 1));
                    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator bl_iter;
                    bl_iter = theBlockedStorePrefetches.find(aligned);
                    DBG_Assert(bl_iter != theBlockedStorePrefetches.end(),
                               (<< theName << " Non-blocked store prefetch by: " << **pf_iter));
                    DBG_Assert(bl_iter->second.count(*pf_iter) > 0);
                    bl_iter->second.erase(*pf_iter);
                    if (bl_iter->second.empty()) { theBlockedStorePrefetches.erase(bl_iter); }
#endif // VALIDATE_STORE_PREFETCHING
                    requestStorePrefetch(lsq_entry);
                }

                ++pf_iter;
            }
            (*iter->theMSHR)->second.theBlockedPrefetches.clear();

            std::list<boost::intrusive_ptr<Instruction>> wake_list;
            wake_list.swap((*iter->theMSHR)->second.theBlockedOps);
            if ((*iter->theMSHR)->second.theTracker) { (*iter->theMSHR)->second.theTracker->setWrongPath(true); }
            DBG_(VVerb, (<< "breakingMSHRLink: erasing MSHR " << (*iter->theMSHR)->second));
            theMSHRs.erase(*iter->theMSHR);

            std::list<boost::intrusive_ptr<Instruction>>::iterator wake_iter, wake_end;
            wake_iter = wake_list.begin();
            wake_end  = wake_list.end();
            while (wake_iter != wake_end) {
                DBG_(Verb, (<< theName << " Port request from here: " << *wake_iter));
                requestPort(*wake_iter);
                ++wake_iter;
            }
        }
        iter->theMSHR = boost::none;
    }
}

void
CoreImpl::insertLSQ(boost::intrusive_ptr<Instruction> anInsn,
                    eOperation anOperation,
                    eSize aSize,
                    bool aBypassSB,
                    InstructionDependance const& aDependance,
                    eAccType type)
{
    FLEXUS_PROFILE();
    theMemQueue.push_back(
      MemQueueEntry(anInsn, ++theMemorySequenceNum, anOperation, aSize, aBypassSB && theNAWBypassSB, aDependance));
    DBG_(Verb, (<< "Pushed LSQEntry: " << theMemQueue.back()));
    ++theLSQCount;
    //  DBG_Assert( theLSQCount + theSBCount + theSBNAWCount ==
    //  static_cast<long>(theMemQueue.size()) );
}

void
CoreImpl::insertLSQ(boost::intrusive_ptr<Instruction> anInsn,
                    eOperation anOperation,
                    eSize aSize,
                    bool aBypassSB,
                    eAccType type)
{
    FLEXUS_PROFILE();
    theMemQueue.push_back(
      MemQueueEntry(anInsn, ++theMemorySequenceNum, anOperation, aSize, aBypassSB && theNAWBypassSB));
    DBG_(VVerb, (<< "Pushed LSQEntry: " << theMemQueue.back()));
    ++theLSQCount;
    //  DBG_Assert( theLSQCount + theSBCount + theSBNAWCount ==
    //  static_cast<long>(theMemQueue.size()) );
}

void
CoreImpl::eraseLSQ(boost::intrusive_ptr<Instruction> anInsn)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
    if (iter == theMemQueue.get<by_insn>().end()) { return; }

    breakMSHRLink(iter);
    DBG_(Verb, (<< "Popping LSQEntry: " << *iter));

    if (iter->thePartialSnoop) { --thePartialSnoopersOutstanding; }

    switch (iter->theQueue) {
        case kLSQ:
            --theLSQCount;
            DBG_Assert(theLSQCount >= 0);
            break;
        case kSSB:
        case kSB:
            if (iter->theBypassSB) {
                --theSBNAWCount;
                DBG_Assert(theSBNAWCount >= 0);
            } else {

                // commit stall cycles due to full store buffer
                if (sbFull()) {
                    boost::intrusive_ptr<TransactionTracker> tracker = anInsn->getFetchTransactionTracker();
                    nXactTimeBreakdown::eCycleClass theStallType;
                    if (tracker && tracker->fillLevel()) {
                        switch (*tracker->fillLevel()) {
                            case eL2:
                                if (tracker->fillType()) {
                                    switch (*tracker->fillType()) {
                                        case eDataPrivate:
                                            theStallType = nXactTimeBreakdown::kStore_BufferFull_L2_Data_Private;
                                            break;
                                        case eDataSharedRO:
                                        case eDataSharedRW:
                                            theStallType = nXactTimeBreakdown::kStore_BufferFull_L2_Data_Shared;
                                            break;
                                        case eCoherence:
                                            theStallType = nXactTimeBreakdown::kStore_BufferFull_L2_Coherence;
                                            break;
                                        default: theStallType = nXactTimeBreakdown::kStore_BufferFull_L2_Unknown; break;
                                    }
                                } else
                                    theStallType = nXactTimeBreakdown::kStore_BufferFull_L2_Unknown;
                                break;
                            case eL3:
                                if (tracker->fillType()) {
                                    switch (*tracker->fillType()) {
                                        case eDataPrivate:
                                            theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Data_Private;
                                            break;
                                        case eDataSharedRO:
                                        case eDataSharedRW:
                                            theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Data_Shared;
                                            break;
                                        case eCoherence:
                                            theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Coherence;
                                            break;
                                        default: theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Unknown; break;
                                    }
                                } else
                                    theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Unknown;
                                break;
                            case eLocalMem:
                            case eRemoteMem: theStallType = nXactTimeBreakdown::kStore_BufferFull_Mem; break;
                            case ePeerL2: theStallType = nXactTimeBreakdown::kStore_BufferFull_PeerL2Cache; break;
                            case ePeerL1Cache: theStallType = nXactTimeBreakdown::kStore_BufferFull_PeerL1Cache; break;
                            default: theStallType = nXactTimeBreakdown::kStore_BufferFull_Other; break;
                        }
                    } else {
                        theStallType = nXactTimeBreakdown::kStore_BufferFull_Other;
                    }
                    theTimeBreakdown.commitAccumulatedStoreCycles(theStallType);
                }

                --theSBCount;
                DBG_Assert(theSBCount >= 0);
            }
            if (iter->isStore() || iter->isAtomic()) {
                DBG_(Iface, (<< theName << " unrequire " << *iter));
                unrequireWritePermission(iter->thePaddr);
            }
            break;
        default: DBG_Assert(false, (<< " invalid queue:" << iter->theQueue));
    }

    /*
    if (iter->isStore() && !anInsn->isAnnulled() &&
    anInsn->getTransactionTracker()) { consortAppend( PredictorMessage::eWrite,
    iter->thePaddr,  anInsn->getTransactionTracker());
    }
    */

    if (iter->thePaddr == kUnresolved || iter->isAbnormalAccess()) {
        iter->theInstruction->setAccessAddress(PhysicalMemoryAddress(0));
    } else {
        iter->theInstruction->setAccessAddress(iter->thePaddr);
    }

    // Kill off any pending prefetch request
    killStorePrefetches(iter->theInstruction);

    theMemQueue.get<by_insn>().erase(iter);
    DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
}

void
CoreImpl::accessMem(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn)
{
    if (anAddress != 0) {
        // Ensure that the correct memory value for this node is in Simics' memory
        // image before we advance Simics
        ValueTracker::valueTracker(theNode).access(theNode, anAddress);
    }
    PhysicalMemoryAddress block_addr(static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1));
    if (block_addr != 0) {
        // Remove the SLAT entry for this Atomic/Load
        removeSLATEntry(block_addr, anInsn);
    }
    memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);

    if (iter != theMemQueue.get<by_insn>().end() && iter->isAtomic() && iter->theQueue == kSSB) {
        DBG_(Iface, (<< theName << " atomic committing in body of checkpoint " << *iter));
        // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSB;
        // });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSB );
        ValueTracker::valueTracker(theNode).access(theNode, anAddress);
    }
}

void
CoreImpl::cleanMSHRS(uint64_t aDiscardAfterSequenceNum)
{
    MSHRs_t::iterator mshr_iter, mshr_temp, mshr_end;
    mshr_iter = theMSHRs.begin();
    mshr_end  = theMSHRs.end();
    while (mshr_iter != mshr_end) {
        mshr_temp = mshr_iter;
        ++mshr_iter;

        std::list<memq_t::index<by_insn>::type::iterator>::iterator wait_iter, wait_temp, wait_end;
        wait_iter = mshr_temp->second.theWaitingLSQs.begin();
        wait_end  = mshr_temp->second.theWaitingLSQs.end();
        while (wait_iter != wait_end) {
            wait_temp = wait_iter;
            ++wait_iter;
            if ((*wait_temp)->theQueue == kLSQ ||
                ((*wait_temp)->theQueue == kSSB &&
                 (*wait_temp)->theInstruction->sequenceNo() >= (static_cast<int64_t>(aDiscardAfterSequenceNum)))) {
                mshr_temp->second.theWaitingLSQs.erase(wait_temp);
            }
        }
        if (mshr_temp->second.theWaitingLSQs.empty()) {

            // Reissue blocked store prefetches
            std::list<boost::intrusive_ptr<Instruction>>::iterator pf_iter, pf_end;
            pf_iter = mshr_temp->second.theBlockedPrefetches.begin();
            pf_end  = mshr_temp->second.theBlockedPrefetches.end();
            while (pf_iter != pf_end) {
                memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(*pf_iter);
                if (lsq_entry != theMemQueue.get<by_insn>().end()) {
#ifdef VALIDATE_STORE_PREFETCHING
                    PhysicalMemoryAddress aligned =
                      PhysicalMemoryAddress(static_cast<uint64_t>(lsq_entry->thePaddr) & ~(theCoherenceUnit - 1));
                    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator bl_iter;
                    bl_iter = theBlockedStorePrefetches.find(aligned);
                    DBG_Assert(bl_iter != theBlockedStorePrefetches.end(),
                               (<< theName << " Non-blocked store prefetch by: " << **pf_iter));
                    DBG_Assert(bl_iter->second.count(*pf_iter) > 0);
                    bl_iter->second.erase(*pf_iter);
                    if (bl_iter->second.empty()) { theBlockedStorePrefetches.erase(bl_iter); }
#endif // VALIDATE_STORE_PREFETCHING
                    requestStorePrefetch(lsq_entry);
                }
                ++pf_iter;
            }
            mshr_temp->second.theBlockedPrefetches.clear();

            std::list<boost::intrusive_ptr<Instruction>> wake_list;
            wake_list.swap(mshr_temp->second.theBlockedOps);
            if (mshr_temp->second.theTracker) { mshr_temp->second.theTracker->setWrongPath(true); }
            DBG_(VVerb, (<< "cleanMSHRS: erasing MSHR " << mshr_temp->second));
            theMSHRs.erase(mshr_temp);

            std::list<boost::intrusive_ptr<Instruction>>::iterator wake_iter, wake_end;
            wake_iter = wake_list.begin();
            wake_end  = wake_list.end();
            while (wake_iter != wake_end) {
                DBG_(Verb, (<< theName << " Port request from here: " << *wake_iter));
                requestPort(*wake_iter);
                ++wake_iter;
            }
        }
    }
}

void
CoreImpl::clearLSQ()
{
    theMemQueue.get<by_queue>().erase(theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ)),
                                      theMemQueue.get<by_queue>().upper_bound(std::make_tuple(kLSQ)));
    theLSQCount = 0;
    DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
}

void
CoreImpl::clearSSB()
{
    int32_t sb_count    = 0;
    int32_t sbnaw_count = 0;
    memq_t::index<by_queue>::type::iterator lb, iter, ub;
    std::tie(lb, ub) = theMemQueue.get<by_queue>().equal_range(std::make_tuple(kSSB));
    iter             = lb;
    while (iter != ub) {
        DBG_(Iface, (<< theName << " unrequire " << *iter));
        if (iter->isStore() || iter->isAtomic()) { unrequireWritePermission(iter->thePaddr); }
        if (iter->theBypassSB) {
            sbnaw_count++;
        } else {
            sb_count++;
        }
        ++iter;
    }

    // commit stall cycles due to full store buffer
    if (sbFull()) { theTimeBreakdown.commitAccumulatedStoreCycles(nXactTimeBreakdown::kStore_BufferFull_Other); }

    theMemQueue.get<by_queue>().erase(lb, ub);
    theSBCount -= sb_count;
    theSBNAWCount -= sbnaw_count;
    DBG_Assert(theSBCount >= 0);
    DBG_Assert(theSBNAWCount >= 0);
    DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
}

int32_t
CoreImpl::clearSSB(uint64_t aLowestInsnSeq)
{
    int32_t sb_count            = 0;
    int32_t sbnaw_count         = 0;
    int32_t remaining_ssb_count = 0;
    memq_t::index<by_queue>::type::iterator lb, iter, ub;
    std::tie(lb, ub) = theMemQueue.get<by_queue>().equal_range(std::make_tuple(kSSB));
    iter             = lb;
    bool first_found = false;
    while (iter != ub) {
        DBG_Assert((static_cast<int64_t>(aLowestInsnSeq)) >= 0);
        if (!first_found && iter->theInstruction->sequenceNo() >= (static_cast<int64_t>(aLowestInsnSeq))) {
            lb          = iter;
            first_found = true;
        } else {
            ++remaining_ssb_count;
        }
        if (first_found) {
            DBG_(Iface, (<< theName << " unrequire " << *iter));
            if (iter->isStore() || iter->isAtomic()) { unrequireWritePermission(iter->thePaddr); }
            if (iter->theBypassSB) {
                sbnaw_count++;
            } else {
                sb_count++;
            }
        }
        ++iter;
    }

    // commit stall cycles due to full store buffer
    if (sbFull()) { theTimeBreakdown.commitAccumulatedStoreCycles(nXactTimeBreakdown::kStore_BufferFull_Other); }

    theMemQueue.get<by_queue>().erase(lb, ub);
    theSBCount -= sb_count;
    theSBNAWCount -= sbnaw_count;
    DBG_Assert(theSBCount >= 0);
    DBG_Assert(theSBNAWCount >= 0);
    DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
    return remaining_ssb_count;
}

bool
CoreImpl::canPushMemOp()
{
    return theMemoryReplies.size() < theNumMemoryPorts + theNumSnoopPorts;
}

void
CoreImpl::pushMemOp(boost::intrusive_ptr<MemOp> anOp)
{
    theMemoryReplies.push_back(anOp);
    DBG_(Iface, (<< " Received: " << *anOp));
}

boost::intrusive_ptr<MemOp>
CoreImpl::popMemOp()
{
    FLEXUS_PROFILE();
    boost::intrusive_ptr<MemOp> ret_val;
    if (!theMemoryPorts.empty()) {
        ret_val = theMemoryPorts.front();

        DBG_(Iface, (<< "Sending: " << *ret_val));
        theMemoryPorts.pop_front();
    }
    return ret_val;
}

TranslationPtr
CoreImpl::popTranslation()
{
    TranslationPtr ret_val;
    if (!theTranslationQueue.empty()) {
        ret_val = theTranslationQueue.front();
        theTranslationQueue.pop();
    }
    return ret_val;
}

void
CoreImpl::pushTranslation(TranslationPtr aTranslation)
{

    boost::intrusive_ptr<Instruction> insn =
      boost::polymorphic_pointer_downcast<Instruction>(aTranslation->getInstruction());
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(insn);
    // DBG_Assert( lsq_entry != theMemQueue.get<by_insn>().end(), ( << *insn) );
    if (lsq_entry == theMemQueue.get<by_insn>().end()) return;
    if (lsq_entry->theVaddr != aTranslation->theVaddr) return;

    if (!insn->resync() && !(aTranslation->isPagefault())) {
        resolvePAddr(insn, aTranslation->thePaddr);

        DBG_(Iface, (<< "Resolved.. vaddr: " << lsq_entry->theVaddr << " to paddr " << lsq_entry->thePaddr));
    } else {
        DBG_(Iface, (<< "Not Resolved.. vaddr: " << lsq_entry->theVaddr << " due to resync."));

        lsq_entry->theException = kException_UNCATEGORIZED;
        //        lsq_entry->theDependance->squash();
        //        insn->squash();
        insn->pageFault();
        //        squashFrom(insn);
    }

    DBG_Assert(lsq_entry->thePaddr != kInvalid);
}

boost::intrusive_ptr<MemOp>
CoreImpl::popSnoopOp()
{
    boost::intrusive_ptr<MemOp> ret_val;
    if (!theSnoopPorts.empty()) {
        ret_val = theSnoopPorts.front();
        DBG_(Iface, (<< "Sending: " << *ret_val));
        theSnoopPorts.pop_front();
    }
    return ret_val;
}

bits
CoreImpl::retrieveLoadValue(boost::intrusive_ptr<Instruction> anInsn)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);

    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end(), (<< *anInsn));
    DBG_Assert(lsq_entry->status() == kComplete, (<< *lsq_entry));
    return *lsq_entry->theValue;
}

void
CoreImpl::setLoadValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end(), (<< *anInsn));
    DBG_Assert(lsq_entry->status() == kComplete, (<< *lsq_entry));
    lsq_entry->theValue = aValue;
}

bits
CoreImpl::retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction> anInsn)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);

    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end(), (<< *anInsn));
    DBG_Assert(lsq_entry->theExtendedValue);
    return *lsq_entry->theExtendedValue;
}

void
CoreImpl::updateVaddr(memq_t::index<by_insn>::type::iterator lsq_entry, VirtualMemoryAddress anAddr)
{
    FLEXUS_PROFILE();
    lsq_entry->theVaddr = anAddr;
    DBG_(VVerb, (<< "in updateVaddr")); // NOOOSHIN
    lsq_entry->thePaddr = PhysicalMemoryAddress(kUnresolved);
    // theMemQueue.get<by_insn>().modify( lsq_entry, [](auto& x){
    // x.thePaddr_aligned = PhysicalMemoryAddress(kUnresolved);});//ll::bind(
    // &MemQueueEntry::thePaddr_aligned, ll::_1 ) =
    // PhysicalMemoryAddress(kUnresolved));
    lsq_entry->thePaddr = kUnresolved;
    theMemQueue.get<by_insn>().modify(lsq_entry,
                                      ll::bind(&MemQueueEntry::thePaddr_aligned, ll::_1) =
                                        PhysicalMemoryAddress(kUnresolved));
}

void
CoreImpl::updatePaddr(memq_t::index<by_insn>::type::iterator lsq_entry, PhysicalMemoryAddress anAddr)
{
    FLEXUS_PROFILE();
    lsq_entry->thePaddr = anAddr;
    DBG_(VVerb, (<< "in updatePaddr")); // NOOOSHIN
    // Map logical to physical

    lsq_entry->theInstruction->setWillRaise(kException_None);

    if (lsq_entry->theInstruction->instCode() == codeSideEffectLoad ||
        lsq_entry->theInstruction->instCode() == codeSideEffectStore ||
        lsq_entry->theInstruction->instCode() == codeSideEffectAtomic) {
        DBG_(Verb, (<< theName << " no longer SideEffect access: " << *lsq_entry));
        lsq_entry->theInstruction->restoreOriginalInstCode();
    }

    PhysicalMemoryAddress addr_aligned(lsq_entry->thePaddr & 0xFFFFFFFFFFFFFFF8ULL);
    // theMemQueue.get<by_insn>().modify( lsq_entry, [&addr_aligned](auto& x){
    // x.thePaddr_aligned = addr_aligned; });//ll::bind(
    // &MemQueueEntry::thePaddr_aligned, ll::_1 ) = addr_aligned);
    theMemQueue.get<by_insn>().modify(lsq_entry, ll::bind(&MemQueueEntry::thePaddr_aligned, ll::_1) = addr_aligned);

    if (thePrefetchEarly && lsq_entry->isStore() && lsq_entry->thePaddr != kUnresolved && lsq_entry->thePaddr != 0 &&
        !lsq_entry->isAbnormalAccess()) {
        requestStorePrefetch(lsq_entry);
    }
}

void
CoreImpl::addSLATEntry(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInstruction)
{
    DBG_Assert(anAddress != 0);
    DBG_(Verb, (<< theName << " add SLAT: " << anAddress << " for " << *anInstruction));
    theSLAT.insert(std::make_pair(anAddress, anInstruction));
}

void
CoreImpl::removeSLATEntry(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInstruction)
{
    DBG_Assert(anAddress != 0);
    SpeculativeLoadAddressTracker::iterator iter, end;
    std::tie(iter, end) = theSLAT.equal_range(anAddress);
    DBG_Assert(iter != end);
    bool found = false;
    while (iter != end) {
        if (iter->second == anInstruction) {
            DBG_(Verb, (<< theName << " remove SLAT: " << anAddress << " for " << *anInstruction));
            theSLAT.erase(iter);
            found = true;
            break;
        }
        ++iter;
    }
    DBG_Assert(found, (<< "No SLAT entry matching " << anAddress << " for " << *anInstruction));
}

void
CoreImpl::translate(boost::intrusive_ptr<Instruction> anInsn)
{

    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());

    TranslationPtr tr(new Translation());
    tr->theVaddr = lsq_entry->theVaddr;
    tr->setData();
    tr->setInstruction(anInsn);

    DBG_(
      Iface,
      (<< "Sending Translation Request to MMU: " << *anInsn << ", VAddr: " << tr->theVaddr << ", ID: " << tr->theID));

    theTranslationQueue.push(tr);
}

void
CoreImpl::resolveVAddr(boost::intrusive_ptr<Instruction> anInsn, VirtualMemoryAddress anAddr)
{
    FLEXUS_PROFILE();

    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
    if (lsq_entry->theVaddr == anAddr) {
        CORE_DBG("no update neccessary for " << anAddr);
        return; // No change
    }
    DBG_(Verb, (<< "Resolved VAddr for " << *lsq_entry << " to " << anAddr));
    CORE_DBG("Resolved VAddr for " << *lsq_entry << " to " << anAddr);

    if (lsq_entry->isStore() && lsq_entry->theValue) {
        CORE_DBG("lsq_entry->isStore() && lsq_entry->theValue");
        resnoopDependantLoads(lsq_entry);
    }

    if (anAddr == kUnresolved) {
        CORE_DBG("anAddr == kUnresolved");
        if (lsq_entry->isLoad() && lsq_entry->loadValue()) {
            CORE_DBG("lsq_entry->isLoad() && lsq_entry->loadValue()");
            DBG_Assert(lsq_entry->theDependance);
            lsq_entry->theDependance->squash();
            lsq_entry->loadValue() = boost::none;
        }
        if (lsq_entry->theMSHR) {
            CORE_DBG("lsq_entry->theMSHR");
            breakMSHRLink(lsq_entry);
        }
        lsq_entry->theIssued = false;
    }

    updateVaddr(lsq_entry, anAddr);
}

void
CoreImpl::resolvePAddr(boost::intrusive_ptr<Instruction> anInsn, PhysicalMemoryAddress anAddr)
{

    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
    DBG_Assert(lsq_entry->thePaddr != 0);

    if (lsq_entry->thePaddr == anAddr) {
        CORE_DBG("no update neccessary for " << anAddr);
        return; // No change
    }

    updatePaddr(lsq_entry, anAddr);
    anInsn->setResolved();

    if (anAddr == (PhysicalMemoryAddress)kUnresolved) return;

    switch (lsq_entry->status()) {
        case kComplete:
            DBG_Assert(lsq_entry->isLoad()); // Only loads & atomics can be complete

            // Resnoop / re-issue the operation
            doLoad(lsq_entry, boost::none);
            if (lsq_entry->isStore()) {
                DBG_Assert(lsq_entry->isAtomic()); // Only atomic stores can be complete
                DBG_Assert(theSpeculativeOrder);   // Can only occur under atomic
                                                   // speculation
                // The write portion of the atomic operation must forward its value to all
                // matching loads
                doStore(lsq_entry);
            }
            break;
        case kAnnulled:
            // Don't need to take any other action
            break;
        case kAwaitingIssue:
            if (lsq_entry->isLoad()) {
                // Send out the load
                doLoad(lsq_entry, boost::none);
            }
            if (lsq_entry->isStore()) {
                // The store operation must forward its value to all matching loads
                doStore(lsq_entry);
            }
            break;
        case kAwaitingPort:
            if (lsq_entry->isLoad()) {
                // Resnoop / re-issue the operation, as there may be a store to forward
                // a value on the new address
                doLoad(lsq_entry, boost::none);
            }
            if (lsq_entry->isStore()) {
                DBG_Assert(lsq_entry->isAtomic()); // Only atomic stores can change
                                                   // address when AwaitingPort
                DBG_Assert(theSpeculativeOrder);   // Can only occur under atomic
                                                   // speculation
                // The store operation must forward its value to all matching loads
                doStore(lsq_entry);
            }
            break;
        case kIssuedToMemory:
            DBG_Assert(lsq_entry->isLoad()); // Only loads can be issued to memory and
                                             // have their address change
            // Resnoop / re-issue the operation
            doLoad(lsq_entry, boost::none);
            if (lsq_entry->isStore()) {
                DBG_Assert(lsq_entry->isAtomic()); // Only atomic stores can be AwaitingPort
                // The store operation must forward its value to all matching loads
                doStore(lsq_entry);
            }
            break;
        case kAwaitingValue:
            DBG_Assert(lsq_entry->theOperation != kLoad); // Only non-loads can be awaiting value
            if (lsq_entry->isAtomic()) { doLoad(lsq_entry, boost::none); }

            break;
        case kAwaitingAddress:
            // This resolve operation cleared the address.  No furter action
            // at this time.
            break;
    }
}

boost::optional<memq_t::index<by_insn>::type::iterator>
CoreImpl::doLoad(memq_t::index<by_insn>::type::iterator lsq_entry,
                 boost::optional<memq_t::index<by_insn>::type::iterator> aCachedSnoopState)
{
    DBG_Assert(lsq_entry->isLoad());

    CORE_DBG(theName << " doLoad: " << *lsq_entry);
    DBG_(Verb, (<< theName << " doLoad: " << *lsq_entry));

    if (lsq_entry->isAtomic() && lsq_entry->thePartialSnoop) {
        CORE_DBG("lsq_entry->isAtomic() && lsq_entry->thePartialSnoop");

        // Partial-snoop atomics must wait for the SB to drain and issue
        // non-speculatively
        if (lsq_entry->loadValue() && lsq_entry->theDependance) {
            CORE_DBG("lsq_entry->loadValue() && lsq_entry->theDependance");
            lsq_entry->theDependance->squash();
        }
        lsq_entry->loadValue() = boost::none;
        lsq_entry->theIssued   = false;
        return aCachedSnoopState;
    }

    boost::optional<bits> previous_value = boost::make_optional(false, bits());
    // First, deal with cleaning up the previous status of this load instruction
    switch (lsq_entry->status()) {
        case kAwaitingValue:
            DBG_Assert(lsq_entry->isAtomic());
            // Pass through to kComplete case below, as kAwaitingValue indicates
            // the load is complete
        case kComplete:
            // Record previous value.  If we can't get the same value again
            // via snooping, we will squash the load's dependance
            previous_value       = lsq_entry->loadValue();
            lsq_entry->theIssued = false;
            break;
        case kAnnulled:
            // No need to do anything
            lsq_entry->theIssued = false;
            return aCachedSnoopState;
        case kIssuedToMemory:
            // We assume that the memory address of the load has changed.
            // Break the connection to the existing MSHR.
            breakMSHRLink(lsq_entry);
            break;

        case kAwaitingPort:
        case kAwaitingIssue:
            if (lsq_entry->isAtomic()) {
                previous_value       = lsq_entry->loadValue();
                lsq_entry->theIssued = false;
            }
            // No need to cancel anything.  If the load ends up getting a value via
            // forwarding, its port request will be discarded
            break;

        case kAwaitingAddress:
            // Once the load gets its address, doLoad will be called again
            return aCachedSnoopState;

        default: DBG_Assert(false,
                            (<< *lsq_entry)); // Loads cannot be in kAwaitingValue state
    }

    // Now, if the load has a SideEffect, we are done.  We do not snoop anything.
    // We set theIssued back to false, in case we previously issued a port request
    if (lsq_entry->isAbnormalAccess()) {
        if (previous_value) {
            DBG_Assert(lsq_entry->theDependance);
            lsq_entry->theDependance->squash();
        }
        lsq_entry->theIssued = false;
        return aCachedSnoopState;
    }

    if (lsq_entry->isAtomic() && !theSpeculativeOrder) {
        DBG_(Verb, (<< theName << " speculation off " << *lsq_entry));
        // doLoad() may not issue an atomic operation if atomic speculation is off
        return aCachedSnoopState;
    }

    // Attempt fulfilling the load by snooping stores
    aCachedSnoopState = snoopStores(lsq_entry, aCachedSnoopState);

    if (lsq_entry->isAtomic() && lsq_entry->thePartialSnoop) {
        // Partial-snoop atomics must wait for the SB to drain and issue
        // non-speculatively
        return aCachedSnoopState;
    }

    DBG_(Verb, (<< theName << " after snoopStores: " << *lsq_entry));

    // Check the loads new status, see if we need to request an MSHR, etc.
    switch (lsq_entry->status()) {
        case kComplete:
        case kAwaitingValue:
            if (previous_value != lsq_entry->loadValue()) {
                DBG_(
                  Verb,
                  (<< theName << "previous_value: " << previous_value << " loadValue(): " << lsq_entry->loadValue()));
                lsq_entry->theIssued = false;
                lsq_entry->theInstruction->setTransactionTracker(
                  0); // Remove any transaction tracker since we changed load value
                DBG_Assert(lsq_entry->theDependance);
                if (previous_value) { lsq_entry->theDependance->squash(); }
                if (lsq_entry->loadValue()) {
                    lsq_entry->theDependance->satisfy();
                } else {
                    DBG_Assert(lsq_entry->isAtomic(),
                               (<< theName << "Only atomics can be complete but not have loadValue() " << *lsq_entry));
                }
            }
            break;
        case kAwaitingIssue:
            if (previous_value) {
                lsq_entry->theInstruction->setTransactionTracker(
                  0); // Remove any transaction tracker since we changed load value
                DBG_Assert(lsq_entry->theDependance);
                lsq_entry->theDependance->squash();
            }
            DBG_(Verb, (<< theName << " Port request from here: " << *lsq_entry));
            requestPort(lsq_entry);
            break;
        case kAwaitingPort:
            if (previous_value) {
                lsq_entry->theInstruction->setTransactionTracker(
                  0); // Remove any transaction tracker since we changed load value
                DBG_Assert(lsq_entry->theDependance);
                lsq_entry->theDependance->squash();
            }
            break; // Port request is already done, it will pick up the new state of the
                   // load
        default:
            // All other states are errors at this point
            DBG_Assert(false,
                       (<< *lsq_entry)); // Loads cannot be in kAwaitingValue state
    }
    return aCachedSnoopState;
}

void
CoreImpl::resnoopDependantLoads(memq_t::index<by_insn>::type::iterator lsq_entry)
{
    FLEXUS_PROFILE();
    // Also used by atomics when their value or address changes
    DBG_Assert(lsq_entry->theOperation != kLoad);

    // Changing the address or annulling a store.  Need to squash all loads which
    // got values from this store.
    if (lsq_entry->thePaddr != kUnresolved) {
        bool was_annulled      = lsq_entry->theAnnulled;
        lsq_entry->theAnnulled = true;
        updateDependantLoads(lsq_entry);
        lsq_entry->theAnnulled = was_annulled;
    }
}

void
CoreImpl::doStore(memq_t::index<by_insn>::type::iterator lsq_entry)
{
    FLEXUS_PROFILE();
    DBG_Assert(lsq_entry->isStore());
    CORE_TRACE;
    // Also used by atomics when their value or address changes
    updateDependantLoads(lsq_entry);
}

void
CoreImpl::updateCASValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue, bits aCompareValue)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
    DBG_Assert(lsq_entry->theOperation == kCAS);

    boost::optional<bits> previous_value(lsq_entry->theValue);

    lsq_entry->theAnnulled     = false;
    lsq_entry->theCompareValue = aCompareValue;

    if (lsq_entry->theExtendedValue) {
        if (*lsq_entry->theExtendedValue == aCompareValue) {
            // Compare succeeded
            lsq_entry->theValue = aValue;
        } else {
            // Compare failed.  theExtendedValue overwrites theValue
            lsq_entry->theValue = lsq_entry->theExtendedValue;
        }
    } else {
        // Load has not yet occurred
        // Speculate that compare will succeed
        lsq_entry->theValue = aValue;
    }

    if (previous_value != lsq_entry->theValue) { doStore(lsq_entry); }
}

void
CoreImpl::updateStoreValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue, boost::optional<bits> anExtendedValue)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
    DBG_Assert(lsq_entry->theOperation != kLoad);
    DBG_(Iface, (<< "Updated store value for " << *lsq_entry << " to " << aValue << "[:" << anExtendedValue << "]"));

    boost::optional<bits> previous_value(lsq_entry->theValue);

    lsq_entry->theAnnulled = false;
    lsq_entry->theValue    = aValue;
    if (anExtendedValue) { lsq_entry->theExtendedValue = anExtendedValue; }

    if (previous_value != lsq_entry->theValue) { doStore(lsq_entry); }
}

void
CoreImpl::annulStoreValue(boost::intrusive_ptr<Instruction> anInsn)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
    DBG_Assert(lsq_entry->theOperation != kLoad);
    DBG_(Verb, (<< "Annul store value for " << *lsq_entry));
    if (!lsq_entry->theAnnulled) {
        lsq_entry->theAnnulled = true;
        lsq_entry->theIssued   = false;
        resnoopDependantLoads(lsq_entry);
    }
}

} // namespace nuArch