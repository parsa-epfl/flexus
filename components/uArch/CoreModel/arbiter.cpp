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

#include "coreModelImpl.hpp"

#include <core/qemu/mai_api.hpp>

#include "../ValueTracker.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

MemoryPortArbiter::MemoryPortArbiter(CoreImpl &aCore, int32_t aNumPorts,
                                     int32_t aMaxStorePrefetches)
    : theNumPorts(aNumPorts), theCore(aCore), theMaxStorePrefetches(aMaxStorePrefetches) {
}

/* - This version of inorder arbitrate was used for the forced in-order memory
  SMS experiments
   - It adds many stalls and is incompatible with a store buffer

  void MemoryPortArbiter::inOrderArbitrate() {
    if
      (   theCore.isFullyStalled() &&  theCore.hasMemPort()  &&
  theCore.theMemQueue.front().status() == kAwaitingPort ) { if
  (theCore.theMemQueue.front().theOperation == kStore &&
  theCore.theMemQueue.front().theQueue == kLSQ ) {
          //Do not issue stores from the LSQ - make them retire first.
          return;
        }
        theCore.issue( theCore.theMemQueue.front().theInstruction );
    }
  }
*/

void MemoryPortArbiter::inOrderArbitrate() {
  // load at LSQ head gets to go first
  memq_t::index<by_queue>::type::iterator iter, end;
  std::tie(iter, end) = theCore.theMemQueue.get<by_queue>().equal_range(std::make_tuple(kLSQ));
  while (iter != end) {
    if (iter->status() == kComplete) {
      ++iter;
      continue;
    }
    if (theCore.hasMemPort() && iter->status() == kAwaitingPort && iter->theOperation != kStore) {
      theCore.issue(iter->theInstruction);
    }
    break;
  }

  // Now try SB head
  std::tie(iter, end) = theCore.theMemQueue.get<by_queue>().equal_range(std::make_tuple(kSB));
  if (iter != end) {
    DBG_Assert(iter->theOperation == kStore);
    if (theCore.hasMemPort() && iter->status() == kAwaitingPort) {
      theCore.issue(iter->theInstruction);
    }
  }

  // Now try Store prefetch
  while (!theStorePrefetchRequests.empty() && theCore.hasMemPort() &&
         theCore.outstandingStorePrefetches() < theMaxStorePrefetches) {
    // issue one store prefetch request
    theCore.issueStorePrefetch(theStorePrefetchRequests.begin()->theInstruction);
    theStorePrefetchRequests.erase(theStorePrefetchRequests.begin());
  }
}

void MemoryPortArbiter::arbitrate() {
  if (theCore.theInOrderMemory) {
    inOrderArbitrate();
    return;
  }
  while (!empty() && theCore.hasMemPort()) {
    DBG_(VVerb, (<< "Arbiter granting request"));

    // Always issue a store or RMW if there is one ready to go.
    if (!thePriorityRequests.empty()) {
      theCore.issue(thePriorityRequests.top().theEntry);
      thePriorityRequests.pop();
      continue;
    }

    eInstructionClass rob_head = theCore.getROBHeadClass();

    // We issue store prefetches ahead of loads when
    if (!theStorePrefetchRequests.empty() &&
        (theCore.isSpinning() || rob_head == clsAtomic || rob_head == clsMEMBAR ||
         rob_head == clsSynchronizing || theStorePrefetchRequests.size() > theMaxStorePrefetches)) {
      if (theCore.outstandingStorePrefetches() < theMaxStorePrefetches) {
        // issue one store prefetch request
        theCore.issueStorePrefetch(theStorePrefetchRequests.begin()->theInstruction);
        theStorePrefetchRequests.erase(theStorePrefetchRequests.begin());
        continue;
      }
    }

    if (!theReadRequests.empty()) {
      theCore.issue(theReadRequests.top().theEntry);
      theReadRequests.pop();
      continue;
    }

    if (!theStorePrefetchRequests.empty() &&
        theCore.outstandingStorePrefetches() < theMaxStorePrefetches) {
      // issue one store prefetch request
      theCore.issueStorePrefetch(theStorePrefetchRequests.begin()->theInstruction);
      theStorePrefetchRequests.erase(theStorePrefetchRequests.begin());
      continue;
    }

    // If we got here, we can't issue anything
    break;
  }
}
void MemoryPortArbiter::request(eOperation op, uint64_t age,
                                boost::intrusive_ptr<Instruction> anInstruction) {
  if (theCore.theInOrderMemory) {
    return; // Queues aren't used for in-order memory
  }

  switch (op) {
  case kLoad:
    theReadRequests.push(PortRequest(age, anInstruction));
    break;
  case kStore:
  case kRMW:
  case kCAS:
    thePriorityRequests.push(PortRequest(age, anInstruction));
    break;

  default:
    DBG_Assert(false);
  }
}

void CoreImpl::killStorePrefetches(boost::intrusive_ptr<Instruction> anInstruction) {
  if (theMemoryPortArbiter.theStorePrefetchRequests.get<by_insn>().erase(anInstruction) > 0) {
#ifdef VALIDATE_STORE_PREFETCHING
    // Find the waiting store prefetch
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter,
        item, end;
    iter = theWaitingStorePrefetches.begin();
    end = theWaitingStorePrefetches.end();
    bool found = false;
    while (iter != end) {
      item = iter;
      ++iter;
      if (item->second.count(anInstruction) > 0) {
        DBG_Assert(!found);
        item->second.erase(anInstruction);
        found = true;
      }
      if (item->second.empty()) {
        theWaitingStorePrefetches.erase(item);
      }
    }
    DBG_Assert(found);
#endif // VALIDATE_STORE_PREFETCHING
  }
}

void MemoryPortArbiter::requestStorePrefetch(memq_t::index<by_insn>::type::iterator lsq_entry) {
  theStorePrefetchRequests.insert(
      StorePrefetchRequest(lsq_entry->theSequenceNum, lsq_entry->theInstruction));
}

bool MemoryPortArbiter::empty() const {
  return (theReadRequests.empty()) && (thePriorityRequests.empty()) &&
         (theStorePrefetchRequests.empty());
}

void CoreImpl::requestStorePrefetch(memq_t::index<by_insn>::type::iterator lsq_entry) {
  if (lsq_entry->thePaddr && !lsq_entry->isAbnormalAccess() && !lsq_entry->isNAW()) {
    theMemoryPortArbiter.requestStorePrefetch(lsq_entry);
    DBG_(Verb, (<< theName << " Store prefetch request: " << *lsq_entry));
#ifdef VALIDATE_STORE_PREFETCHING
    PhysicalMemoryAddress aligned =
        PhysicalMemoryAddress(static_cast<uint64_t>(lsq_entry->thePaddr) & ~(theCoherenceUnit - 1));
    DBG_Assert(theWaitingStorePrefetches[aligned].count(lsq_entry->theInstruction) == 0);
    theWaitingStorePrefetches[aligned].insert(lsq_entry->theInstruction);
#endif // VALIDATE_STORE_PREFETCHING
  }
}

void CoreImpl::requestStorePrefetch(boost::intrusive_ptr<Instruction> anInstruction) {
  FLEXUS_PROFILE();

  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInstruction);
  if (lsq_entry == theMemQueue.get<by_insn>().end()) {
    // Memory operation already complete
    return;
  }
  requestStorePrefetch(lsq_entry);
}

void CoreImpl::requestPort(memq_t::index<by_insn>::type::iterator lsq_entry) {
  if (lsq_entry->status() != kAwaitingIssue && lsq_entry->status() != kAwaitingPort) {
    if (lsq_entry->isAtomic() &&
        ((!lsq_entry->theStoreComplete && lsq_entry->status() == kComplete) ||
         (lsq_entry->status() == kAwaitingValue))) {

      DBG_Assert(theSpeculativeOrder);
      // Request may proceed, as this is the store portion of a speculatively
      // retired atomic op.

    } else {
      DBG_(Verb, (<< theName << " Memory Port request by " << *lsq_entry
                  << " ignored because entry is not waiting for issue"));
      return;
    }
  }
  if (lsq_entry->thePartialSnoop) {
    memq_t::index<by_queue>::type::iterator lsq_head =
        theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
    if (lsq_entry->theInstruction != lsq_head->theInstruction) {
      DBG_(Verb, (<< theName << " Memory Port request by " << *lsq_entry
                  << " ignored because it is a partial snoop and not the LSQ head"));
      return;
    }
    DBG_(Verb, (<< theName << " Partial Snoop " << *lsq_entry
                << " may proceed because it is the LSQ head"));
  }

  DBG_(Verb, (<< theName << " Memory Port request by " << *lsq_entry));
  theMemoryPortArbiter.request(lsq_entry->theOperation, lsq_entry->theSequenceNum,
                               lsq_entry->theInstruction);
  lsq_entry->theIssued = true;

  // Kill off any pending prefetch request
  killStorePrefetches(lsq_entry->theInstruction);
}

void CoreImpl::requestPort(boost::intrusive_ptr<Instruction> anInstruction) {
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInstruction);
  if (lsq_entry == theMemQueue.get<by_insn>().end()) {
    // Memory operation completed some other way (i.e. forwarding, annullment)
    DBG_(Verb, (<< theName << " Memory Port request by " << *anInstruction
                << " ignored because LSQ entry is gone"));
    return;
  }
  requestPort(lsq_entry);
}

void CoreImpl::issue(boost::intrusive_ptr<Instruction> anInstruction) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInstruction);
  if (lsq_entry == theMemQueue.get<by_insn>().end()) {
    // Memory operation completed some other way (i.e. forwarding, annullment)
    DBG_(Verb, (<< theName << " Memory Port request by " << *anInstruction
                << " ignored because LSQ entry is gone"));
    return;
  }
  if (lsq_entry->status() != kAwaitingPort) {
    if (!(lsq_entry->isAtomic() && lsq_entry->status() == kComplete &&
          !lsq_entry->theStoreComplete)) {
      DBG_(Verb, (<< theName << " Memory Port request by " << *lsq_entry
                  << " ignored because entry is not waiting for issue"));
      return;
    }
  }

  DBG_(Iface, (<< "Attempting to issue a memory requst for " << lsq_entry->thePaddr));
  DBG_Assert(lsq_entry->thePaddr != 0);

  eOperation issue_op = lsq_entry->theOperation;

  switch (lsq_entry->theOperation) {
  case kLoad:
  case kStore:
    // Leave issue_op alone
    break;
  case kRMW:
  case kCAS:
    // Figure out whether we are issuing the Atomic non-speculatively, or
    // we are issuing the Load or Store half of a speculative Atomic
    if (!theSpeculativeOrder) {
      // Issue non-speculatively
      DBG_Assert(lsq_entry->theQueue == kLSQ);
      break;
    }
    if (lsq_entry->isAbnormalAccess()) {
      // Non-speculative
      DBG_Assert(lsq_entry->theQueue == kLSQ);
      break;
    }
    if (lsq_entry->theQueue == kLSQ) {
      issue_op = kAtomicPreload; // HERE
    } else {
      // DBG_Assert( lsq_entry->theQueue == kSSB );
      issue_op = kStore;
    }
    break;
  default:
    return; // Other types don't get issued
  }

  switch (issue_op) {
  case kAtomicPreload:
    if (!lsq_entry->thePaddr) {
      DBG_(Verb, (<< "Cache atomic preload to invalid address for " << *lsq_entry));
      // Unable to map virtual address to physical address for load.  Load is
      // speculative or TLB miss.
      lsq_entry->theValue = 0;
      lsq_entry->theExtendedValue = 0;
      DBG_Assert(lsq_entry->theDependance);
      lsq_entry->theDependance->satisfy();
      return;
    }

    if (scanAndBlockMSHR(lsq_entry)) {
      // There is another access for the same PA.
      DBG_(Verb, (<< theName << " " << *lsq_entry << " stalled because of existing MSHR entry"));
      return;
    }
    break;
  case kLoad:
    if (lsq_entry->thePaddr == kUnresolved) {
      DBG_(Verb, (<< "Cache read to invalid address for " << *lsq_entry));
      // Unable to map virtual address to physical address for load.  Load is
      // speculative or TLB miss.
      if (lsq_entry->theValue)
        lsq_entry->theValue = 0;
      lsq_entry->theExtendedValue = 0;
      lsq_entry->theDependance->satisfy();
      return;
    }

    // Check for an existing MSHR for the same address
    if (scanAndAttachMSHR(lsq_entry)) {
      DBG_(Verb, (<< *lsq_entry << " attached to existing outstanding load MSHR"));
      return;
    }
    if (scanAndBlockMSHR(lsq_entry)) {
      // There is another load for the same PA which cannot satisfy this load
      // because of size issues.
      DBG_(Verb, (<< theName << " " << *lsq_entry << " stalled because of existing MSHR entry"));
      return;
    }
    break;
  case kStore:
  case kRMW:
  case kCAS:
    if (!lsq_entry->thePaddr) {
      DBG_(Crit, (<< theName << " Store/Atomic issued without a Paddr."));
      DBG_(Verb, (<< "Cache write to invalid address for " << *lsq_entry));
      // Unable to map virtual address to physical address for load.  Store is
      // a device access or  TLB miss
      if (lsq_entry->theDependance) {
        lsq_entry->theDependance->satisfy();
      } else {
        // This store doesn't have to do anything, since we don't know its
        // PAddr
        DBG_Assert(lsq_entry->theOperation == kStore);
        eraseLSQ(lsq_entry->theInstruction);
      }
      return;
    }
    // Check for an existing MSHR for the same address (issued this cycle)
    if (scanAndBlockMSHR(lsq_entry)) {
      DBG_(Verb, (<< theName << " " << *lsq_entry << " stalled because of existing MSHR entry"));
      return;
    }
    break;
  default:
    DBG_Assert(false);
  }

  // Fill in an MSHR and memory port
  boost::intrusive_ptr<MemOp> op(new MemOp());
  op->theInstruction = anInstruction;
  MSHR mshr;
  op->theOperation = mshr.theOperation = issue_op;
  DBG_Assert(op->theVAddr != kUnresolved);
  op->theVAddr = lsq_entry->theVaddr;
  op->theSideEffect = lsq_entry->theSideEffect;
  op->theReverseEndian = lsq_entry->theInverseEndian;
  op->theNonCacheable = lsq_entry->theNonCacheable;
  if (issue_op == kStore && lsq_entry->isAtomic()) {
    op->theAtomic = true;
  }
  if (lsq_entry->theBypassSB) {
    DBG_(Trace, (<< "NAW store issued: " << *lsq_entry));
    op->theNAW = true;
  }
  op->thePAddr = mshr.thePaddr = lsq_entry->thePaddr;
  op->theSize = mshr.theSize = lsq_entry->theSize;
  mshr.theWaitingLSQs.push_back(lsq_entry);
  op->thePC = lsq_entry->theInstruction->pc();
  bool system = lsq_entry->theInstruction->isPriv();
  if (lsq_entry->theValue) {
    op->theValue = *lsq_entry->theValue;
  } else {
    op->theValue = 0;
  }

  boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
  tracker->setAddress(op->thePAddr);
  tracker->setInitiator(theNode);

  /* CMU-ONLY-BLOCK-BEGIN */
  if (theEarlySGP) {
    // Only notify the SGP if there is no unresolved address
    // Calculate distance into the ROB.  This is slow, but so be it
    int32_t distance = 0;
    rob_t::iterator iter = theROB.begin();
    while (iter != theROB.end() && *iter != anInstruction) {
      ++distance;
      ++iter;
    }
    uint64_t logical_timestamp = theCommitNumber + theSRB.size() + distance;
    // tracker->setLogicalTimestamp(lsq_entry->theSequenceNum);
    tracker->setLogicalTimestamp(logical_timestamp);
  }
  /* CMU-ONLY-BLOCK-END */

  tracker->setSource("uArch");
  tracker->setOS(system);
  op->theTracker = tracker;
  mshr.theTracker = tracker;
  if (lsq_entry->isAtomic() && issue_op == kAtomicPreload) {
    tracker->setSpeculativeAtomicLoad(true);
  }

  bool ignored;
  std::tie(lsq_entry->theMSHR, ignored) = theMSHRs.insert(std::make_pair(mshr.thePaddr, mshr));
  theMemoryPorts.push_back(op);
  DBG_(Verb, (<< theName << " " << *lsq_entry << " issuing operation " << *op));
}

void CoreImpl::issueMMU(TranslationPtr aTranslation) {
  boost::intrusive_ptr<MemOp> op(new MemOp());
  eOperation issue_op = kPageWalkRequest;

  op->theInstruction.reset();
  MSHR mshr;
  op->theOperation = mshr.theOperation = issue_op;
  DBG_Assert(op->theVAddr != kUnresolved);
  op->theVAddr = aTranslation->theVaddr;

  op->thePAddr = mshr.thePaddr = aTranslation->thePaddr;
  op->theSize = mshr.theSize = kDoubleWord /*aTranslate->theSize*/;
  op->thePC = VirtualMemoryAddress(0);
  bool system = true;

  boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
  tracker->setAddress(op->thePAddr);
  tracker->setInitiator(theNode);

  tracker->setSource("MMU");
  tracker->setOS(system);
  op->theTracker = tracker;
  mshr.theTracker = tracker;

  bool ignored;
  /*std::tie(lsq_entry->theMSHR, ignored) = */ theMSHRs.insert(std::make_pair(mshr.thePaddr, mshr));
  theMemoryPorts.push_back(op);
  DBG_(Iface, (<< theName << " "
               << " issuing translation operation " << *op << "  -- ID " << aTranslation->theID));
  bool inserted =
      thePageWalkRequests.emplace(std::make_pair(aTranslation->theVaddr, aTranslation)).second;
  while (!inserted) {
    std::map<VirtualMemoryAddress, TranslationPtr>::iterator item =
        thePageWalkRequests.find(aTranslation->theVaddr);
    thePageWalkRequests.erase(item);
    inserted =
        thePageWalkRequests.emplace(std::make_pair(aTranslation->theVaddr, aTranslation)).second;
  }
}

bool CoreImpl::scanAndAttachMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry) {
  FLEXUS_PROFILE();
  // Check for an existing MSHR for the same address (issued this cycle)
  MSHRs_t::iterator existing = theMSHRs.find(anLSQEntry->thePaddr);
  if (existing != theMSHRs.end()) {
    DBG_Assert(!existing->second.theWaitingLSQs.empty());
    if (existing->second.theOperation == kLoad && existing->second.theSize == anLSQEntry->theSize) {
      existing->second.theWaitingLSQs.push_back(anLSQEntry);
      DBG_Assert(!anLSQEntry->theMSHR);
      anLSQEntry->theMSHR = existing;
      return true;
    }
  }
  return false;
}

bool CoreImpl::scanMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry) {
  FLEXUS_PROFILE();
  if (!anLSQEntry->thePaddr) {
    return false;
  }
  // Check for an existing MSHR for the same address (issued this cycle)
  MSHRs_t::iterator existing = theMSHRs.find(anLSQEntry->thePaddr);
  return (existing != theMSHRs.end());
}

bool CoreImpl::scanAndBlockMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry) {
  FLEXUS_PROFILE();
  if (!anLSQEntry->thePaddr) {
    DBG_(Crit, (<< "LSQ Entry missing PADDR in scanAndBlockMSHR" << *anLSQEntry));
  }
  // Check for an existing MSHR for the same address (issued this cycle)
  MSHRs_t::iterator existing = theMSHRs.find(anLSQEntry->thePaddr);
  if (existing != theMSHRs.end()) {
    DBG_Assert(!existing->second.theWaitingLSQs.empty());
    existing->second.theBlockedOps.push_back(anLSQEntry->theInstruction);
    return true;
  }
  return false;
}

bool CoreImpl::scanAndBlockPrefetch(memq_t::index<by_insn>::type::iterator anLSQEntry) {
  FLEXUS_PROFILE();
  if (!anLSQEntry->thePaddr) {
    return false;
  }
  // Check for an existing MSHR for the same address (issued this cycle)
  MSHRs_t::iterator existing = theMSHRs.find(anLSQEntry->thePaddr);
  if (existing != theMSHRs.end()) {
    DBG_Assert(!existing->second.theWaitingLSQs.empty());
    existing->second.theBlockedPrefetches.push_back(anLSQEntry->theInstruction);
    return true;
  }
  return false;
}

void CoreImpl::issueStore() {
  FLEXUS_PROFILE();
  if ((!theMemQueue.empty()) && (theMemQueue.front().theQueue == kSB) &&
      theMemQueue.front().status() == kAwaitingIssue && !theMemQueue.front().isAbnormalAccess()) {
    if (theConsistencyModel == kRMO && !theMemQueue.front().isAtomic() &&
        theMemQueue.front().thePaddr && !theMemQueue.front().theNonCacheable) {
      PhysicalMemoryAddress aligned = PhysicalMemoryAddress(
          static_cast<uint64_t>(theMemQueue.front().thePaddr) & ~(theCoherenceUnit - 1));
      if (theSBLines_Permission.find(aligned) != theSBLines_Permission.end()) {
        if (theSBLines_Permission[aligned].second) {
          // Short circuit the store because we already have permission on chip
          DBG_(Verb, (<< theName << " short-circuit store: " << theMemQueue.front()));
          MemOp op;
          op.theOperation = kStoreReply;
          op.theVAddr = theMemQueue.front().theVaddr;
          op.theSideEffect = theMemQueue.front().theSideEffect;
          op.theReverseEndian = theMemQueue.front().theInverseEndian;
          op.theNonCacheable = theMemQueue.front().theNonCacheable;
          op.thePAddr = theMemQueue.front().thePaddr;
          op.theSize = theMemQueue.front().theSize;
          op.thePC = theMemQueue.front().theInstruction->pc();
          if (theMemQueue.front().theValue) {
            op.theValue = *theMemQueue.front().theValue;
          } else {
            op.theValue = 0;
          }
          theMemQueue.front().theIssued = true;
          // Need to inform ValueTracker that this store is complete
          bits value = op.theValue;
          //          if (op.theReverseEndian) {
          //            value = bits(Flexus::Qemu::endianFlip(value.to_ulong(),
          //            op.theSize)); DBG_(Verb, ( << "Performing inverse endian
          //            store for addr " << std::hex << op.thePAddr << " val: "
          //            << op.theValue << " inv: " << value << std::dec ));
          //          }
          ValueTracker::valueTracker(theNode).commitStore(theNode, op.thePAddr, op.theSize, value);
          completeLSQ(theMemQueue.project<by_insn>(theMemQueue.begin()), op);
          return;
        }
      }
    }
    DBG_Assert(theMemQueue.front().thePaddr != kInvalid,
               (<< "Abnormal stores should not get to issueStore: " << theMemQueue.front()));
    DBG_(Verb, (<< theName << " Port request from here: "
                << *theMemQueue.project<by_insn>(theMemQueue.begin())));
    requestPort(theMemQueue.project<by_insn>(theMemQueue.begin()));
  }
}

void CoreImpl::issueAtomicSpecWrite() {
  FLEXUS_PROFILE();
  if ((!theMemQueue.empty()) && (theMemQueue.front().theQueue == kSB) &&
      (!theMemQueue.front().theIssued) && (theMemQueue.front().status() == kComplete)) {
    DBG_(Verb, (<< theName << "issueAtomicSpecWrite() " << theMemQueue.front()));
    DBG_Assert(theMemQueue.front().thePaddr != kInvalid,
               (<< "issueAtomicSpecWrite: " << theMemQueue.front()));
    DBG_Assert(!theMemQueue.front().theSideEffect,
               (<< "issueAtomicSpecWrite: " << theMemQueue.front()));
    DBG_Assert(theMemQueue.front().isAtomic(),
               (<< "issueAtomicSpecWrite: " << theMemQueue.front()));
    DBG_Assert(theSpeculativeOrder);
    theMemQueue.front().theIssued = false;
    DBG_(Verb, (<< theName << " Port request from here: "
                << *theMemQueue.project<by_insn>(theMemQueue.begin())));
    requestPort(theMemQueue.project<by_insn>(theMemQueue.begin()));
  }
}

void CoreImpl::issuePartialSnoop() {
  FLEXUS_PROFILE();
  if (thePartialSnoopersOutstanding > 0) {
    memq_t::index<by_queue>::type::iterator lsq_head =
        theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
    DBG_(VVerb, (<< "lsq_head:  " << lsq_head->status()));
    DBG_Assert(lsq_head != theMemQueue.get<by_queue>().end());
    if (lsq_head->thePartialSnoop &&
        (lsq_head->status() == kAwaitingIssue || lsq_head->status() == kAwaitingPort)) {
      DBG_(Verb, (<< theName << " Port request from here: " << *lsq_head));
      requestPort(theMemQueue.project<by_insn>(lsq_head));
    }
  }
}

void CoreImpl::issueStorePrefetch(boost::intrusive_ptr<Instruction> anInstruction) {
  FLEXUS_PROFILE();
  DBG_Assert(hasMemPort());

  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInstruction);
  if (lsq_entry == theMemQueue.get<by_insn>().end()) {
    // Memory operation completed some other way (i.e. forwarding, annullment)
    DBG_(Verb, (<< theName << " Store Prefetch request ignored because LSQ entry is gone"
                << *anInstruction));

#ifdef VALIDATE_STORE_PREFETCHING
    // Find the waiting store prefetch
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter,
        item, end;
    iter = theWaitingStorePrefetches.begin();
    end = theWaitingStorePrefetches.end();
    bool found = false;
    while (iter != end) {
      item = iter;
      ++iter;
      if (item->second.count(anInstruction) > 0) {
        DBG_Assert(!found);
        item->second.erase(anInstruction);
        found = true;
      }
      if (item->second.empty()) {
        theWaitingStorePrefetches.erase(item);
      }
    }
    DBG_Assert(found);
#endif // VALIDATE_STORE_PREFETCHING
    return;
  }

  PhysicalMemoryAddress aligned =
      PhysicalMemoryAddress(static_cast<uint64_t>(lsq_entry->thePaddr) & ~(theCoherenceUnit - 1));
  std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter;
#ifdef VALIDATE_STORE_PREFETCHING
  iter = theWaitingStorePrefetches.find(aligned);
  DBG_Assert(iter != theWaitingStorePrefetches.end(),
             (<< theName << " Non-waiting store prefetch by: " << *anInstruction));
  DBG_Assert(iter->second.count(anInstruction) > 0);
  iter->second.erase(anInstruction);
  if (iter->second.empty()) {
    DBG_(Verb, (<< theName << " Erase store prefetch " << *anInstruction));
    theWaitingStorePrefetches.erase(iter);
  }
#endif // VALIDATE_STORE_PREFETCHING

  if (lsq_entry->isAbnormalAccess() || lsq_entry->isNAW() || lsq_entry->thePaddr == kInvalid ||
      lsq_entry->thePaddr == kUnresolved || lsq_entry->thePaddr == 0) {
    DBG_(Verb, (<< theName << " Store prefetch request by " << *lsq_entry
                << " ignored because store is abnormal "));
    return;
  }
  if (theSBLines_Permission.find(aligned) == theSBLines_Permission.end()) {
    DBG_Assert(thePrefetchEarly, (<< theName << " Prefetch from untracked line: " << *lsq_entry));
  } else {
    if (theSBLines_Permission[aligned].second) {
      DBG_(Verb, (<< theName << " Store prefetch request by " << *lsq_entry
                  << " ignored because write permission already on-chip"));
      return;
    }
  }

  if (scanAndBlockPrefetch(theMemQueue.project<by_insn>(lsq_entry))) {
    DBG_(Verb, (<< theName << " Store prefetch request by " << *lsq_entry
                << " delayed because of conflicting MSHR entry."));
#ifdef VALIDATE_STORE_PREFETCHING
    DBG_Assert(theBlockedStorePrefetches[aligned].count(lsq_entry->theInstruction) == 0);
    theBlockedStorePrefetches[aligned].insert(anInstruction);
#endif // VALIDATE_STORE_PREFETCHING
    ++theStorePrefetchConflicts;
    return;
  }

  bool is_new = false;
  std::tie(iter, is_new) = theOutstandingStorePrefetches.insert(
      std::make_pair(aligned, std::set<boost::intrusive_ptr<Instruction>>()));
  iter->second.insert(anInstruction);
  if (!is_new) {
    DBG_(Verb, (<< theName << " Store prefetch request by " << *lsq_entry
                << " ignored because prefetch already outstanding."));
    ++theStorePrefetchDuplicates;
    return;
  }

  // Issue prefetch operation
  boost::intrusive_ptr<MemOp> op(new MemOp());
  op->theOperation = kStorePrefetch;
  op->theVAddr = lsq_entry->theVaddr;
  op->thePAddr = lsq_entry->thePaddr;
  op->thePC = lsq_entry->theInstruction->pc();
  op->theSize = lsq_entry->theSize;
  op->theValue = 0;
  if (lsq_entry->theBypassSB) {
    DBG_(Dev, (<< "NAW store prefetch: " << *lsq_entry));
    op->theNAW = true;
  }

  boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
  tracker->setAddress(op->thePAddr);
  tracker->setInitiator(theNode);
  op->theTracker = tracker;
  op->theInstruction = anInstruction;
  theMemoryPorts.push_back(op);
  if (lsq_entry->theOperation == kStore) {
    ++theStorePrefetches;
  } else {
    ++theAtomicPrefetches;
  }
  DBG_(Verb, (<< theName << " " << *lsq_entry << " issuing store prefetch."));
}

// When atomic speculation is off, all atomics are issued via issueAtomic.
// When atomic speculation is on, any atomic which is issued into an empty ROB
//(suggesting that it sufferred a rollback), or encountered a partial snoop,
// will issue via issueAtomic()
void CoreImpl::issueAtomic() {
  FLEXUS_PROFILE();
  if (!theMemQueue.empty() &&
      (theMemQueue.front().theOperation == kCAS || theMemQueue.front().theOperation == kRMW) &&
      theMemQueue.front().status() == kAwaitingIssue && theMemQueue.front().theValue &&
      (theMemQueue.front().theInstruction == theROB.front())) {
    // Atomics that are issued as the head of theMemQueue don't need to
    // worry about partial snoops
    if (theMemQueue.front().thePartialSnoop) {
      theMemQueue.front().thePartialSnoop = false;
      --thePartialSnoopersOutstanding;
    }
    if (theMemQueue.front().thePaddr == kInvalid) {
      // CAS to unsupported ASI.  Pretend the operation is done.
      theMemQueue.front().theIssued = true;
      theMemQueue.front().theExtendedValue = 0;
      theMemQueue.front().theStoreComplete = true;
      theMemQueue.front().theInstruction->resolveSpeculation();
      DBG_Assert(theMemQueue.front().theDependance);
      theMemQueue.front().theDependance->satisfy();
    } else {
      DBG_(Verb, (<< theName << " Port request from here: "
                  << *theMemQueue.project<by_insn>(theMemQueue.begin())));
      requestPort(theMemQueue.project<by_insn>(theMemQueue.begin()));
    }
  }
}

void CoreImpl::resolveCheckpoint() {
  if (!theMemQueue.empty() && theMemQueue.front().theQueue == kSSB &&
      theMemQueue.front().theInstruction->hasCheckpoint() &&
      (!theMemQueue.front().theSpeculatedValue) // Must confirm value speculation before
                                                // completing checkpoint
  ) {
    DBG_Assert(isSpeculating()); // Should only be an SSB when we are
                                 // speculating
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt =
        theCheckpoints.find(theMemQueue.front().theInstruction);
    DBG_Assert(ckpt != theCheckpoints.end());

    if (ckpt->second.theHeldPermissions.size() == ckpt->second.theRequiredPermissions.size()) {
      DBG_(Verb, (<< theName << " Resolving speculation: " << theMemQueue.front()));
      theMemQueue.front().theInstruction->resolveSpeculation();
      if (theMemQueue.front().isAtomic()) {
        // theMemQueue.modify( theMemQueue.begin(), [](auto& x){ x.theQueue =
        // kSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSB );
        theMemQueue.modify(theMemQueue.begin(), ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSB);
      } else if (theMemQueue.front().isLoad()) {
        eraseLSQ(theMemQueue.front().theInstruction);
      }
    }
  }
}

/*
  void CoreImpl::resolveMEMBAR() {
    FLEXUS_PROFILE();
    if (   !   theMemQueue.empty()
        &&     theMemQueue.front().theOperation == kMEMBARMarker
        &&     theMemQueue.front().theQueue == kSSB
        ) {
          if (consistencyModel() ==
          DBG_Assert( isSpeculating()); //Should only be a MEMBAR at the SSB
  head when we are speculating
          //MEMBAR markers at the head of theMemQueue are no longer speculative
          DBG_(Trace, ( << theName << " Resolving MEMBAR speculation: " <<
  theMemQueue.front()) );
          theMemQueue.front().theInstruction->resolveSpeculation();
    }
  }
*/

bool CoreImpl::checkStoreRetirement(boost::intrusive_ptr<Instruction> aStore) {
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(aStore);
  if (iter == theMemQueue.get<by_insn>().end()) {
    return true;
  } else if (iter->theAnnulled) {
    return true;
  } else if (iter->isAbnormalAccess()) {
    if (iter->theExtraLatencyTimeout && theFlexus->cycleCount() > *iter->theExtraLatencyTimeout) {
      if (iter->theSideEffect &&
          !iter->theException /*&& ! interruptASI(iter->theASI) && ! mmuASI(iter->theASI)*/) {
        return sbEmpty();
      } else {
        return true;
      }
    } else {
      return false;
    }
  }
  return true;
}

void CoreImpl::issueSpecial() {
  FLEXUS_PROFILE();
  if (theLSQCount > 0) {
    memq_t::index<by_queue>::type::iterator lsq_head =
        theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
    if ((lsq_head->isAbnormalAccess()) && lsq_head->status() == kAwaitingIssue &&
        (!lsq_head->isMarker()) &&
        (theROB.empty() ? true : (lsq_head->theInstruction == theROB.front())) &&
        (!lsq_head->theExtraLatencyTimeout) // Make sure we only see the side
                                            // effect once
    ) {

      if (lsq_head->theException != kException_None) {
        // No extra latency for instructions that raise exceptions
        lsq_head->theExtraLatencyTimeout = theFlexus->cycleCount();
      } else if (lsq_head->theBypassSB) {
        DBG_(Dev, (<< "NAW store completed on the sly: " << *lsq_head));
        lsq_head->theExtraLatencyTimeout = theFlexus->cycleCount();
      } /*else if (lsq_head->theMMU) {
        lsq_head->theExtraLatencyTimeout = theFlexus->cycleCount() +
      theOnChipLatency;
      }*/
      else {
        DBG_Assert(lsq_head->theSideEffect /*|| interruptASI(lsq_head->theASI)*/);

        // Can't issue side-effect accesses while speculating
        if (!theIsSpeculating) {
          // SideEffect access to unsupported ASI
          uint32_t latency = 0;
          //          switch (lsq_head->theASI) {
          //              // see UltraSPARC III cu manual for details
          //            case 0x15: // ASI_PHYS_BYPASS_EC_WITH_EBIT
          //            case 0x1D: // ASI_PHYS_BYPASS_EC_WITH_EBIT_LITTLE
          //            case 0x4A: // ASI_FIREPLANE_CONFIG_REG or
          //            ASI_FIREPLANE_ADDRESS_REG
          //              latency = theOffChipLatency;
          //              ++theSideEffectOffChip;
          //              DBG_( Verb, ( << theName << " Set SideEffect access
          //              time to " << latency << " " << *lsq_head) ); break;
          //            default:
          //              // everything else is on-chip
          //              latency = theOnChipLatency;
          //              ++theSideEffectOnChip;
          //              DBG_( Verb, ( << theName << " Set SideEffect access
          //              time to " << latency << " " << *lsq_head) ); break;
          //          }
          lsq_head->theExtraLatencyTimeout = theFlexus->cycleCount() + latency;
        }
      }
    }
  }
}

void CoreImpl::checkExtraLatencyTimeout() {
  FLEXUS_PROFILE();

  if (theLSQCount > 0) {
    memq_t::index<by_queue>::type::iterator lsq_head =
        theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
    if (lsq_head->theExtraLatencyTimeout &&
        lsq_head->theExtraLatencyTimeout.get() <= theFlexus->cycleCount() &&
        lsq_head->status() == kAwaitingIssue) {

      /*if ( mmuASI(lsq_head->theASI)) {
        if (lsq_head->isLoad()) {
          //Perform MMU access for loads.  Stores are done in retireMem()
          lsq_head->theValue = Flexus::Qemu::Processor::getProcessor( theNode
      )->mmuRead(lsq_head->theVaddr, lsq_head->theASI);
          lsq_head->theExtendedValue = 0;
          DBG_( Verb, ( << theName << " MMU read: " << *lsq_head ) );
        }
      } else */
      if (lsq_head->theException != kException_None) {
        DBG_(Verb, (<< theName << " Memory access raises exception.  Completing the operation: "
                    << *lsq_head));
        lsq_head->theIssued = true;
        lsq_head->theValue = 0;
        lsq_head->theExtendedValue = 0;

      } /*else if (interruptASI(lsq_head->theASI)) {
        if (lsq_head->isLoad()) {
          //Perform Interrupt access for loads.  We let Simics worry about
      stores lsq_head->theValue = Flexus::Qemu::Processor::getProcessor( theNode
      )->interruptRead(lsq_head->theVaddr, lsq_head->theASI);
          lsq_head->theExtendedValue = 0;
          DBG_( Verb, ( << theName << " Interrupt read: " << *lsq_head ) );
        }

      } */
      else {

        DBG_(Verb, (<< theName
                    << " SideEffect access to unknown Paddr.  Completing the "
                       "operation: "
                    << *lsq_head));
        lsq_head->theIssued = true;
        lsq_head->theValue = 0;
        lsq_head->theExtendedValue = 0;
        if (lsq_head->isLoad()) {
          lsq_head->theInstruction->forceResync();
        }
      }

      if (lsq_head->isStore()) {
        lsq_head->theStoreComplete = true;
      }
      if (lsq_head->theDependance) {
        lsq_head->theDependance->satisfy();
      } else {
        // eraseLSQ( lsq_head->theInstruction );
      }
    }
  }
}

} // namespace nuArch
