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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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

#include "coreModelImpl.hpp"

#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <components/CommonQEMU/TraceTracker.hpp>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArchARM {

void CoreImpl::invalidate(PhysicalMemoryAddress anAddress) {
  FLEXUS_PROFILE();
  DBG_Assert(static_cast<uint64_t>(anAddress) ==
                 (static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1)),
             (<< "address = " << std::hex << static_cast<uint64_t>(anAddress)));

  // If the processor is speculating, check the SLAT.  On a match, we will
  // abort the speculation, and we can save ourselves the trouble of looking
  // at the rest of theMemQueue
  if (isSpeculating()) {
    SpeculativeLoadAddressTracker::iterator iter, end;
    std::tie(iter, end) = theSLAT.equal_range(anAddress);
    boost::optional<boost::intrusive_ptr<Instruction>> violator(nullptr);
    while (iter != end) {
      if (!violator || iter->second->sequenceNo() < (*violator)->sequenceNo()) {
        if (iter->second->instClass() == clsAtomic) {
          // If the 'violator' is an atomic which still has a predicted value,
          // it does not actually violate
          memq_t::index<by_insn>::type::iterator violator_memq =
              theMemQueue.get<by_insn>().find(iter->second);
          DBG_Assert(violator_memq != theMemQueue.get<by_insn>().end());
          if (!violator_memq->theSpeculatedValue) {
            // must roll back
            DBG_(Iface, (<< theName << " SLAT hit required " << anAddress << " for: "
                         << *violator_memq << " because value is no longer speculative"));
            violator = iter->second;
          } else {
            DBG_(Iface, (<< theName << " avoided SLAT hit on " << anAddress
                         << " for: " << *violator_memq << " because value is still speculative"));
            ++theSLATHits_AtomicAvoided;
          }
        } else {
          violator = iter->second;
        }
      }
      ++iter;
    }
    if (violator) {
      // SLAT hit - need to abort speculation
      // This causes a resync with Simics the next time we get to commit()
      theViolatingInstruction = *violator;
      DBG_(Verb, (<< theName << " SLAT hit on " << anAddress << " earliest instruction: "
                  << *theViolatingInstruction << " - speculation must abort"));
      theAbortSpeculation = true;
      switch (theViolatingInstruction->instClass()) {
      case clsLoad:
        ++theSLATHits_Load;
        break;
      case clsStore:
        ++theSLATHits_Store;
        break;
      case clsAtomic:
        ++theSLATHits_Atomic;
        break;
      default:
        DBG_Assert(false); // Not possible
        break;
      }

      return;
    }
  }

  memq_t::index<by_paddr>::type::iterator start, temp, end;

  start = theMemQueue.get<by_paddr>().lower_bound(std::make_tuple(anAddress));
  end = theMemQueue.get<by_paddr>().upper_bound(
      std::make_tuple(PhysicalMemoryAddress(anAddress + static_cast<int>(theCoherenceUnit - 1))));
  if (start == end) {
    return; // No matches for this physical memory address
  }

  // Loads which meet ALL the following conditions must be squashed:
  //(1):      it is complete (i.e. has produced a value)
  //(2):      the load cannot receive its value via forwarding from an
  // unperformed store (3 SC):   the load follows an incomplete load or store in
  // program order (3 TSO):  the load follows an incomplete load in program
  // order -or- the load follows a MEMBAR in the LSQ -or- the load is or follows
  // an Atomic op in the LSQ -or- the system is speculating (an Atomic or MEMBAR
  // has speculatively retired)

  // TWENISCH - we could always just be pessimistic and assume that anything
  // still in the LSQ has to roll back.

  // Step 1 determine the sequence number of the first incomplete memory
  // operation.
  boost::optional<uint64_t> first_incomplete = 0;
  if (isSpeculating()) {
    // When speculating, the entire LSQ is after the first incomplete op
    // We use the sequence number of theMemQueue head as "first incomplete"
    first_incomplete = theMemQueue.front().theSequenceNum;
  } else if (consistencyModel() == kSC && !sbEmpty()) {
    if (theSpeculativeOrder) {
      // For speculative order, anything in the SB is considered complete.
      memq_t::index<by_queue>::type::iterator ssb_head =
          theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kSSB));
      if (ssb_head == theMemQueue.get<by_queue>().end() || ssb_head->theQueue != kSSB) {
        // SSB is empty - try the LSQ
        if (theLSQCount > 0) {
          ssb_head = theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
        } else {
          // Everthing in the SB is actually done.
          return;
        }
      }
      first_incomplete = ssb_head->theSequenceNum;
    } else {
      first_incomplete = theMemQueue.front().theSequenceNum;
      DBG_(Verb,
           (<< "Store buffer non-empty under SC.  First incomplete op: " << theMemQueue.front()));
    }
  } else {
    memq_t::index<by_queue>::type::iterator iter, end;
    std::tie(iter, end) = theMemQueue.get<by_queue>().equal_range(std::make_tuple(kLSQ));
    while (iter != end) {
      // We search from the front of the LSQ for the first operation to meet the
      // conditions above
      if ((iter->isLoad() && iter->status() != kComplete &&
           iter->status() != kAnnulled) // Any non-anulled and non-complete load
          || (iter->isMarker())         // Any MEMBAR
          || (iter->isAtomic())         // Any Atomic operation
      ) {
        DBG_(Verb, (<< "First incomplete op: " << *iter));
        first_incomplete = iter->theSequenceNum;
        break;
      }
      ++iter;
    }
  }

  bool race_counted = false;
  // Step 2 iteratore over all the matching physical addresses.
  while (start != end) {
    temp = start;
    ++start;
    DBG_(Verb, (<< "Invalidate examining: " << *temp));

    // We only care about operations with load semantics that have already
    // received a value.
    if (temp->isLoad() && temp->theQueue != kSB && temp->status() == kComplete &&
        !temp->theSpeculatedValue && !temp->theInstruction->isAnnulled()) {
      // Count the race
      if (!race_counted) {
        ++theRaces;
        race_counted = true;
      }
      bool system = temp->theInstruction->isPriv();

      // if its a completed load, and it has a higher sequence number than
      // first_incomplete, the load attempts forwarding.  If forwarding fails,
      // it must be squashed.  Otherwise, it overrides Simics.
      if (first_incomplete && temp->theSequenceNum >= *first_incomplete) {
        // Load must be squashed and re-executed
        int32_t seq_num = 0;
        PhysicalMemoryAddress addr;
        bool research = false;
        if (start != end) {
          research = true;
          seq_num = start->theSequenceNum;
          addr = start->thePaddr_aligned;
        }
        doLoad(theMemQueue.project<by_insn>(temp), boost::none);
        if (research) {
          start = theMemQueue.get<by_paddr>().lower_bound(std::make_tuple(addr, seq_num));
          end = theMemQueue.get<by_paddr>().upper_bound(std::make_tuple(
              PhysicalMemoryAddress(anAddress + static_cast<int>(theCoherenceUnit - 1))));
        }

        // Record invalidate replays
        if (!temp->status() == kComplete) {
          // Load was squashed and could not forward.
          if (system) {
            ++theRaces_LoadReplayed_System;
          } else {
            ++theRaces_LoadReplayed_User;
          }
        } else {
          // Load found a value in the SB / LSQ
          if (system) {
            ++theRaces_LoadForwarded_System;
          } else {
            ++theRaces_LoadForwarded_User;
          }
        }

      } else {
        // In this case, the load is complete and there are no incomplete
        // mem ops, MEMBARs, or loads preceding it

        // TWENISCH - While this case is strictly speaking correct, I am
        // considering rolling these back anyway, to simplify this code
        if (system) {
          ++theRaces_LoadAlreadyOrdered_System;
        } else {
          ++theRaces_LoadAlreadyOrdered_User;
        }

        // Load does not need to roll back
        DBG_(Iface, (<< "Completed load to address " << anAddress
                     << " which does not need re-execution.  Will override Simics."));
        temp->theInstruction->overrideSimics();
      }
    }
  }

  DBG_(Verb, (<< "Done w/ invalidation"));
}

void CoreImpl::processMemoryReplies() {
  FLEXUS_PROFILE();
  while (!theMemoryReplies.empty()) {
    MemOp const &anOperation = *theMemoryReplies.front();
    if (processReply(anOperation)) {
      theMemoryReplies.pop_front();
    } else {
      break;
    }
  }
}

void CoreImpl::ackInvalidate(MemOp const &anInvalidate) {
  FLEXUS_PROFILE();

  loseWritePermission(eLosePerm_Invalidate, anInvalidate.thePAddr);
  invalidate(anInvalidate.thePAddr);
  boost::intrusive_ptr<MemOp> op(new MemOp(anInvalidate));
  op->theOperation = kInvAck;
  DBG_(Iface, (<< "Sending InvAck: " << *op));
  theSnoopPorts.push_back(op);
}

void CoreImpl::ackDowngrade(MemOp const &aDowngrade) {
  FLEXUS_PROFILE();

  loseWritePermission(eLosePerm_Downgrade, aDowngrade.thePAddr);
  boost::intrusive_ptr<MemOp> op(new MemOp(aDowngrade));
  op->theOperation = kDowngradeAck;
  DBG_(Iface, (<< "Sending DowngradeAck: " << *op));
  theSnoopPorts.push_back(op);
}

void CoreImpl::ackProbe(MemOp const &aProbe) {
  FLEXUS_PROFILE();
  // Right now, this code is correct for non-speculative SC and TSO
  DBG_Assert(consistencyModel() == kSC || consistencyModel() == kTSO || consistencyModel() == kRMO);
  boost::intrusive_ptr<MemOp> op(new MemOp(aProbe));
  op->theOperation = kProbeAck;
  DBG_(Iface, (<< "Sending ProbeAck: " << *op));
  theSnoopPorts.push_back(op);
}

void CoreImpl::ackReturn(MemOp const &aReturn) {
  boost::intrusive_ptr<MemOp> op(new MemOp(aReturn));
  op->theOperation = kReturnReply;
  theSnoopPorts.push_back(op);
}

bool CoreImpl::processReply(MemOp const &anOperation) {
  FLEXUS_PROFILE();
  std::set<boost::intrusive_ptr<Instruction>>::iterator iter, end;
  PhysicalMemoryAddress addr(static_cast<uint64_t>(anOperation.thePAddr) & ~(theCoherenceUnit - 1));

  switch (anOperation.theOperation) {
  case kRMWReply:
  case kCASReply:
  case kStoreReply:
    acquireWritePermission(addr);
  case kLoadReply:
  case kAtomicPreloadReply:
    if (anOperation.theTracker && !anOperation.theTracker->completionCycle()) {
      anOperation.theTracker->complete(); // The transaction is done.
    }
    complete(anOperation);
    break;
  case kInvalidate:
    theTraceTracker.invalidation(theNode, eCore, addr);
    if (!hasSnoopBuffer()) {
      return false;
    }
    ackInvalidate(anOperation);
    break;
  case kDowngrade:
    if (!hasSnoopBuffer()) {
      return false;
    }
    ackDowngrade(anOperation);
    break;
  case kProbe:
    if (!hasSnoopBuffer()) {
      return false;
    }
    ackProbe(anOperation);
    break;
  case kStorePrefetchReply:
    if (anOperation.theTracker && !anOperation.theTracker->completionCycle()) {
      anOperation.theTracker->complete(); // The transaction is done.
    }
    iter = theOutstandingStorePrefetches[addr].begin();
    end = theOutstandingStorePrefetches[addr].end();

    while (iter != end) {
      (*iter)->setPrefetchTracker(anOperation.theTracker);
      ++iter;
    }

    acquireWritePermission(addr);

    theOutstandingStorePrefetches.erase(addr);
    break;
  case kReturnReq:
    ackReturn(anOperation);
    break;

  default:
    DBG_Assert(false,
               (<< "Memory operation type " << anOperation.theOperation << " not supported "));
  }
  return true;
}

bool CoreImpl::satisfies(eOperation aResponse, eOperation anOperation) {
  switch (anOperation) {
  case kLoad:
  case kPageWalkRequest:
    return aResponse == kLoadReply;
  case kStore:
    return aResponse == kStoreReply;
  case kRMW:
    return aResponse == kRMWReply ||
           (theSpeculativeOrder && (aResponse == kAtomicPreloadReply || aResponse == kStoreReply));
  case kCAS:
    return aResponse == kCASReply ||
           (theSpeculativeOrder && (aResponse == kAtomicPreloadReply || aResponse == kStoreReply));
  case kAtomicPreload:
    return aResponse == kAtomicPreloadReply;
  default:
    return false;
  }
}

void CoreImpl::complete(MemOp const &anOperation) {
  FLEXUS_PROFILE();
  MSHRs_t::iterator match = theMSHRs.find(anOperation.thePAddr);

  if (match != theMSHRs.end()) {
    // See if this reply matches the request.  If not, ignore it, another reply
    // will arrive at some point.
    if (satisfies(anOperation.theOperation, match->second.theOperation) &&
        (match->second.theSize == anOperation.theSize)) {

      // Extract lists
      std::list<memq_t::index<by_insn>::type::iterator> complete_list;
      complete_list.swap(match->second.theWaitingLSQs);

      std::list<boost::intrusive_ptr<Instruction>> wake_list;
      wake_list.swap(match->second.theBlockedOps);

      std::list<boost::intrusive_ptr<Instruction>> pf_list;
      pf_list.swap(match->second.theBlockedPrefetches);

      if (match->second.theOperation == kPageWalkRequest) {
        std::map<VirtualMemoryAddress, TranslationPtr>::iterator item =
            thePageWalkRequests.find(anOperation.theVAddr);

        DBG_Assert(item != thePageWalkRequests.end());
        item->second->rawTTEValue = (uint64_t)anOperation.theValue;
        item->second->toggleReady();
        DBG_(Iface, (<< "Process Memory Reply for ID( " << item->second->theID << ") ready("
                     << item->second->isReady() << ")  -- vaddr: " << anOperation.theVAddr
                     << "  -- paddr: " << anOperation.thePAddr << "  --  Instruction: "
                     << anOperation.theInstruction << " --  PC: " << anOperation.thePC));

        thePageWalkRequests.erase(item);
      }
      theMSHRs.erase(match);

      // The MSHR has to be erased before we can call either of these
      for (auto &entry : complete_list) {
        this->completeLSQ(entry, anOperation);
      }

      std::list<boost::intrusive_ptr<Instruction>>::iterator iter, end;
      iter = wake_list.begin();
      end = wake_list.end();
      while (iter != end) {
        DBG_(Verb, (<< theName << " Port request from here: " << *iter));
        requestPort(*iter);
        ++iter;
      }

      // Re-request blocked store prefetches
      std::list<boost::intrusive_ptr<Instruction>>::iterator pf_iter, pf_end;
      pf_iter = pf_list.begin();
      pf_end = pf_list.end();
      while (pf_iter != pf_end) {
        memq_t::index<by_insn>::type::iterator lsq_entry =
            theMemQueue.get<by_insn>().find(*pf_iter);
        if (lsq_entry != theMemQueue.get<by_insn>().end()) {
#ifdef VALIDATE_STORE_PREFETCHING
          PhysicalMemoryAddress aligned = PhysicalMemoryAddress(
              static_cast<uint64_t>(lsq_entry->thePaddr) & ~(theCoherenceUnit - 1));
          std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator
              bl_iter;
          bl_iter = theBlockedStorePrefetches.find(aligned);
          DBG_Assert(bl_iter != theBlockedStorePrefetches.end(),
                     (<< theName << " Non-blocked store prefetch by: " << **pf_iter));
          DBG_Assert(bl_iter->second.count(*pf_iter) > 0);
          bl_iter->second.erase(*pf_iter);
          if (bl_iter->second.empty()) {
            theBlockedStorePrefetches.erase(bl_iter);
          }
#endif // VALIDATE_STORE_PREFETCHING
          requestStorePrefetch(lsq_entry);
        }
        ++pf_iter;
      }

      theIdleThisCycle = false;
    }
  } else {
    if (anOperation.theTracker) {
      if (anOperation.theTracker->fillType() && *anOperation.theTracker->fillType() == eCoherence) {
        ++theWP_CoherenceMiss;
        DBG_(Trace,
             (<< theName << " WrongPath Coherence "
              << (anOperation.theTracker->OS() && *anOperation.theTracker->OS() ? "[S] " : "[U] ")
              << anOperation));
      } else if (anOperation.theTracker->fillLevel() &&
                 *anOperation.theTracker->fillLevel() == ePrefetchBuffer) {
        ++theWP_SVBHit;
        DBG_(Trace,
             (<< theName << " WrongPath: SVB-HIT "
              << (anOperation.theTracker->OS() && *anOperation.theTracker->OS() ? "[S] " : "[U] ")
              << anOperation));
      } else {
        // fixme: add a stat for wrong path here
      }
    }
    DBG_(Iface, (<< "No matching MSHR: " << anOperation));
  }
}

void CoreImpl::completeLSQ(memq_t::index<by_insn>::type::iterator lsq_entry,
                           MemOp const &anOperation) {
  FLEXUS_PROFILE();
  DBG_Assert(lsq_entry->status() == kIssuedToMemory ||
                 (lsq_entry->status() == kComplete && lsq_entry->isAtomic()) ||
                 (theConsistencyModel == kRMO && lsq_entry->status() == kAwaitingPort),
             (<< theName << " violating entry: " << *lsq_entry));

  if (anOperation.theOperation == kAtomicPreloadReply) {
    DBG_Assert(lsq_entry->theOperation == kRMW || lsq_entry->theOperation == kCAS);
    DBG_Assert(theSpeculativeOrder);

    if (theSpeculateOnAtomicValue && lsq_entry->theSpeculatedValue &&
        (lsq_entry->theQueue != kLSQ)) {
      DBG_Assert(theIsSpeculating);
      if (anOperation.theValue != *lsq_entry->theExtendedValue) {
        // Speculation was wrong.
        DBG_(Iface, (<< theName << " Value mispredict Predicted: " << *lsq_entry->theExtendedValue
                     << " actual: " << anOperation.theValue << " LSQ entry: " << *lsq_entry));
        theViolatingInstruction = lsq_entry->theInstruction;
        theAbortSpeculation = true;
        ++theValuePredictions_Failed;
      } else {
        lsq_entry->theSpeculatedValue = false;
        ++theValuePredictions_Successful;
        DBG_(Iface, (<< theName << " Value Predict correct: " << *lsq_entry->theExtendedValue
                     << " LSQ entry: " << *lsq_entry));
      }
    } else {
      // Load portion of a RMW fulfilled via Atomic Speculation
      if (lsq_entry->theExtendedValue && *lsq_entry->theExtendedValue != anOperation.theValue) {
        if (lsq_entry->theDependance) {
          lsq_entry->theDependance->squash();
        }
      }
      lsq_entry->theExtendedValue = anOperation.theValue;
      lsq_entry->theSpeculatedValue = false;

      if (lsq_entry->theOperation == kCAS) {
        if (lsq_entry->theCompareValue && *lsq_entry->theCompareValue != anOperation.theValue) {
          // Compare failed, write will not occur.
          lsq_entry->theValue = anOperation.theValue;
          updateDependantLoads(lsq_entry);
        }
      }
    }

    if (lsq_entry->thePartialSnoop) {
      DBG_(Verb, (<< theName << " updating partial snoops: " << *lsq_entry));
      applyAllStores(lsq_entry);
      lsq_entry->thePartialSnoop = false; // Done handling the partial snoop
      --thePartialSnoopersOutstanding;
      DBG_(Verb, (<< theName << " done updating partial snoops: " << *lsq_entry));
    }

  } else if (anOperation.theOperation == kLoadReply) {
    DBG_Assert(lsq_entry->theOperation == kLoad, (<< anOperation << " " << *lsq_entry));
    lsq_entry->theValue = anOperation.theValue;
    lsq_entry->theExtendedValue = anOperation.theExtendedValue;
    if (lsq_entry->thePartialSnoop) {
      applyAllStores(lsq_entry);
      lsq_entry->thePartialSnoop = false; // Done handling the partial snoop
      --thePartialSnoopersOutstanding;
    }
  } else if (anOperation.theOperation == kCASReply) {
    DBG_Assert(lsq_entry->theOperation == kCAS, (<< anOperation << " " << *lsq_entry));
    // NOTE: CAS processing assumes CAS issued when store buffer is empty
    // DBG_Assert ( ( ! lsq_entry->thePartialSnoop ) || theSpeculativeOrder);
    DBG_Assert(!lsq_entry->thePartialSnoop, (<< theName << " no partial snoops: " << *lsq_entry));

    // Need to determine if the store wrote or not.  To do this, compare
    // the returned load
    lsq_entry->theExtendedValue = anOperation.theExtendedValue;
    DBG_Assert(lsq_entry->theCompareValue);
    if (lsq_entry->theCompareValue != lsq_entry->theExtendedValue) {
      // Compare failed, write did not occur.  Put the read value in the
      // LSQ for forwarding.
      lsq_entry->theValue = anOperation.theExtendedValue;
      updateDependantLoads(lsq_entry);
    }
    lsq_entry->theStoreComplete = true;
    // Atomic may commit
    DBG_(Verb, (<< theName << " Non-speculative CAS completion: " << *lsq_entry));
    ++theNonSpeculativeAtomics;
    lsq_entry->theInstruction->resolveSpeculation();
  } else if (anOperation.theOperation == kRMWReply) {
    DBG_Assert(lsq_entry->theOperation == kRMW, (<< anOperation << " " << *lsq_entry));
    DBG_Assert(!lsq_entry->thePartialSnoop, (<< theName << " no partial snoops: " << *lsq_entry));

    lsq_entry->theExtendedValue = anOperation.theExtendedValue;
    lsq_entry->theStoreComplete = true;

    // Atomic may commit
    DBG_(Verb, (<< theName << " Non-speculative RMW completion: " << *lsq_entry));
    ++theNonSpeculativeAtomics;
    lsq_entry->theInstruction->resolveSpeculation();

  } else if (anOperation.theOperation == kStoreReply) {
    // In the Store buffer
    DBG_Assert(lsq_entry->isStore(), (<< anOperation << " " << *lsq_entry));
    if (lsq_entry->theSideEffect) {
      DBG_Assert(lsq_entry->theQueue == kLSQ);
    } else {
      DBG_Assert(lsq_entry->theQueue == kSB ||
                 (lsq_entry->theQueue == kSSB && lsq_entry->isAtomic() && theSpeculativeOrder));
      // Consider completed SB stores as forward progress.
      theFlexus->watchdogReset(theNode);
    }
    lsq_entry->theStoreComplete = true;
  }

  DBG_(Verb, (<< "Completing: " << *lsq_entry));
  if (anOperation.theTracker) {
    lsq_entry->theInstruction->setTransactionTracker(anOperation.theTracker);
  }
  lsq_entry->theMSHR = boost::none;
  if (lsq_entry->theDependance) {
    lsq_entry->theDependance->satisfy();
  }
  // We remove stores/successfully speculated atomics from the store buffer when
  // their reply arrives
  if (anOperation.theOperation == kStoreReply) {
    if (lsq_entry->theQueue == kSB) {
      // if we are under TSO, we may need to charge stall cycles to this store
      if (consistencyModel() == kSC && !theSpeculativeOrder && getROBHeadClass() == clsLoad) {
        // Charge any accumulated stall cycles to this store
        chargeStoreStall(lsq_entry->theInstruction, anOperation.theTracker);
      }

      accountCommitMemOp(lsq_entry->theInstruction);

      eraseLSQ(lsq_entry->theInstruction);
    } else {
      DBG_Assert(!(lsq_entry->theQueue == kSSB));
      DBG_Assert(lsq_entry->theSideEffect, (<< *lsq_entry));
    }
  }
}

} // namespace nuArchARM
