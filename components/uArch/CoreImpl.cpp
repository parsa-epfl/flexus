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

#include <boost/optional/optional_io.hpp>
#include <boost/polymorphic_pointer_cast.hpp>

#include <components/CommonQEMU/TraceTracker.hpp>
#include <core/qemu/mai_api.hpp>
#include "CoreImpl.hpp"
#include "ValueTracker.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

using namespace Flexus::SharedTypes;

namespace nDecoder {
extern uint32_t theInsnCount;
}

namespace nuArch {

std::ostream &operator<<(std::ostream &anOstream, eQueue aCode) {
  const char *codes[] = {"LSQ", "SSB", "SB"};
  if (aCode >= kLastQueueType) {
    anOstream << "Invalid Queue Code(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << codes[aCode];
  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, eStatus aCode) {
  const char *codes[] = {"Complete",     "Annulled",        "IssuedToMemory", "AwaitingIssue",
                         "AwaitingPort", "AwaitingAddress", "AwaitingValue"};
  anOstream << codes[aCode];
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, MSHR const &anMSHR) {
  anOstream << "MSHR " << anMSHR.theOperation << " " << anMSHR.thePaddr << "[";
  std::for_each(anMSHR.theWaitingLSQs.begin(), anMSHR.theWaitingLSQs.end(),
                ll::var(anOstream) << *ll::_1);
  anOstream << "]";
  return anOstream;
}

bits value(MemQueueEntry const &anEntry, bool aFlipEndian) {
  DBG_Assert(anEntry.theValue);
  return *anEntry.theValue << (16 - anEntry.theSize) * 8;
}

bits loadValue(MemQueueEntry const &anEntry) {
  DBG_Assert(anEntry.loadValue());
  return *anEntry.loadValue() << (16 - anEntry.theSize) * 8;
}

uint64_t offset(MemQueueEntry const &anEntry) {
  return (16 - static_cast<int>(anEntry.theSize)) -
         (static_cast<uint64_t>(anEntry.thePaddr) - anEntry.thePaddr_aligned);
}

bits makeMask(MemQueueEntry const &anEntry) {
  bits mask = nuArch::mask(anEntry.theSize) << (16 - anEntry.theSize) * 8;
  mask = mask >> (offset(anEntry) * 8);
  return mask;
}

bool covers(MemQueueEntry const &aStore, MemQueueEntry const &aLoad) {
  bits required = makeMask(aLoad);
  bits available = makeMask(aStore);
  return ((required & available) == required);
}

bool intersects(MemQueueEntry const &aStore, MemQueueEntry const &aLoad) {
  bits required = makeMask(aLoad);
  bits available = makeMask(aStore);
  return ((required & available) != 0);
}

bits align(MemQueueEntry const &anEntry, bool flipEndian) {
  return (value(anEntry, flipEndian) >> (offset(anEntry) * 8));
}

bits alignLoad(MemQueueEntry const &anEntry) {
  return (loadValue(anEntry) >> (offset(anEntry) * 8));
}

void writeAligned(MemQueueEntry const &anEntry, bits anAlignedValue, bool use_ext = false) {
  bits masked_value = anAlignedValue & makeMask(anEntry);
  masked_value <<= (offset(anEntry) * 8);
  masked_value >>= (16 - anEntry.theSize) * 8;
  if (!use_ext) {
    anEntry.loadValue() = masked_value;
  } else {
    anEntry.theExtendedValue = masked_value;
  }
}

void overlay(MemQueueEntry const &aStore, MemQueueEntry const &aLoad, bool use_ext = false) {
  bits available = makeMask(aStore); // Determine what part of the int64_t the store covers
  bits store_aligned = align(aStore,
                             aStore.theInverseEndian !=
                                 aLoad.theInverseEndian); // align the store value for computation,
                                                          // flip endianness if neccessary
  bits load_aligned = alignLoad(aLoad);                   // align the load value for computation
  load_aligned = load_aligned & (~available);             // zero out the overlap region in the load
  load_aligned =
      load_aligned | (store_aligned & available); // paste the store value into the overlap
  writeAligned(aLoad, load_aligned,
               use_ext); // un-align the value and put it back in the load
}

void dec_mod8(uint32_t &anInt) {
  if (anInt == 0) {
    anInt = 7;
  } else {
    --anInt;
  }
}

void inc_mod8(uint32_t &anInt) {
  ++anInt;
  if (anInt >= 8) {
    anInt = 0;
  }
}


void MemQueueEntry::describe(std::ostream &anOstream) const {
  if (theOperation == kMEMBARMarker) {
    anOstream << theQueue << "(" << status() << ")"
              << "[" << theSequenceNum << "] " << theOperation;
  } else {
    anOstream << theQueue << "(" << status() << ")"
              << "[" << theSequenceNum << "] " << theOperation << "(" << static_cast<int>(theSize)
              << ") " << theVaddr << " " << thePaddr;

    if (theValue) {
      anOstream << " =" << std::hex << *theValue;
    }
    if (theExtendedValue) {
      anOstream << " x=" << *theExtendedValue;
    }
    if (theCompareValue) {
      anOstream << " ?=" << *theCompareValue;
    }

    if (theAnnulled) {
      anOstream << " {annulled}";
    }
    if (theStoreComplete) {
      anOstream << " {store-complete}";
    }
    if (thePartialSnoop) {
      anOstream << " {partial-snoop}";
    }
    if (theException >= kException_None) {
      anOstream << " {raise " << theException << " }";
    }
    if (theSideEffect) {
      anOstream << " {side-effect}";
    }
    if (theSpeculatedValue) {
      anOstream << " {value-speculation}";
    }
    if (theBypassSB) {
      anOstream << " {bypass-SB}";
    }
    //    if (theMMU) {
    //      anOstream << " {mmu}";
    //    }a
    if (theNonCacheable) {
      anOstream << " {non-cacheable}";
    }
    if (theExtraLatencyTimeout) {
      anOstream << " {completes at " << *theExtraLatencyTimeout << "}";
    }
  }
  anOstream << " {" << *theInstruction << "}";
}

std::ostream &operator<<(std::ostream &anOstream, MemQueueEntry const &anEntry) {
  anEntry.describe(anOstream);
  return anOstream;
}

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

void CoreImpl::requestStorePrefetch(boost::intrusive_ptr<Instruction> anInstruction) {
  FLEXUS_PROFILE();

  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInstruction);
  if (lsq_entry == theMemQueue.get<by_insn>().end()) {
    // Memory operation already complete
    return;
  }
  requestStorePrefetch(lsq_entry);
}

bool MemoryPortArbiter::empty() const {
  return (theReadRequests.empty()) && (thePriorityRequests.empty()) &&
         (theStorePrefetchRequests.empty());
}


CoreImpl::CoreImpl(uArchOptions_t options,
                   std::function<int(bool)> _advance,
                   std::function<void(eSquashCause)> _squash,
                   std::function<void(VirtualMemoryAddress)> _redirect,
                   std::function<void(boost::intrusive_ptr<BranchFeedback>)> _feedback,
                   std::function<void(bool)> _signalStoreForwardingHit,
                   std::function<void(int32_t)> _mmuResync
                  )
    : theName(options.name), theNode(options.node),
      advance_fn(_advance), squash_fn(_squash), redirect_fn(_redirect),
      feedback_fn(_feedback),
      signalStoreForwardingHit_fn(_signalStoreForwardingHit), mmuResync_fn(_mmuResync),
      thePendingTrap(kException_None),
      theBypassNetwork(kxRegs + 3 * options.ROBSize, kvRegs + 4 * options.ROBSize),
      theLastGarbageCollect(0), thePreserveInteractions(false),
      theMemoryPortArbiter(*this, options.numMemoryPorts, options.numStorePrefetches),
      theROBSize(options.ROBSize), theRetireWidth(options.retireWidth),
      theInterruptSignalled(false), thePendingInterrupt(kException_None),
      theInterruptInstruction(0), theMemorySequenceNum(0), theLSQCount(0), theSBCount(0),
      theSBNAWCount(0), theSBSize(options.SBSize), theNAWBypassSB(options.NAWBypassSB),
      theNAWWaitAtSync(options.NAWWaitAtSync), theConsistencyModel(options.consistencyModel),
      theCoherenceUnit(options.coherenceUnit), theSustainedIPCCycles(0), theKernelPanicCount(0),
      theSpinning(false), theSpinDetectCount(0), theSpinRetireSinceReset(0),
      theSpinControlEnabled(options.spinControlEnabled), thePrefetchEarly(options.prefetchEarly),
      theSBLines_Permission_falsecount(0), theSpeculativeOrder(options.speculativeOrder),
      theSpeculateOnAtomicValue(options.speculateOnAtomicValue),
      theSpeculateOnAtomicValuePerfect(options.speculateOnAtomicValuePerfect),
      theValuePredictInhibit(false), theIsSpeculating(false), theIsIdle(false),
      theRetiresSinceCheckpoint(0),
      theAllowedSpeculativeCheckpoints(options.speculativeCheckpoints),
      theCheckpointThreshold(options.checkpointThreshold), theAbortSpeculation(false),
      theTSOBReplayStalls(0), theSLATHits_Load(theName + "-SLATHits:Load"),
      theSLATHits_Store(theName + "-SLATHits:Store"),
      theSLATHits_Atomic(theName + "-SLATHits:Atomic"),
      theSLATHits_AtomicAvoided(theName + "-SLATHits:AtomicAvoided"),
      theValuePredictions(theName + "-ValuePredict"),
      theValuePredictions_Successful(theName + "-ValuePredict:Success"),
      theValuePredictions_Failed(theName + "-ValuePredict:Fail"),
      theDG_HitSB(theName + "-SBLostPermission:Downgrade"),
      theInv_HitSB(theName + "-SBLostPermission:Invalidate"),
      theRepl_HitSB(theName + "-SBLostPermission:Replacement"),
      theEarlySGP(options.earlySGP),
      theTrackParallelAccesses(options.trackParallelAccesses),
      theInOrderMemory(options.inOrderMemory), theInOrderExecute(options.inOrderExecute),
      theIdleThisCycle(false), theIdleCycleCount(0),
      theOnChipLatency(options.onChipLatency),
      theOffChipLatency(options.offChipLatency),
      theNumMemoryPorts(options.numMemoryPorts), theNumSnoopPorts(options.numSnoopPorts),
      theMispredictCycles(0), // for IStall and mispredict stats
      theMispredictCount(0),  // for IStall and mispredict stats
      theCommitNumber(0), theCommitCount(theName + "-Commits"),
      theCommitCount_NonSpin_User(theName + "-Commits:NonSpin:User"),
      theCommitCount_NonSpin_System(theName + "-Commits:NonSpin:System"),
      theCommitCount_NonSpin_Trap(theName + "-Commits:NonSpin:Trap"),
      theCommitCount_NonSpin_Idle(theName + "-Commits:NonSpin:Idle"),
      theCommitCount_Spin_User(theName + "-Commits:Spin:User"),
      theCommitCount_Spin_System(theName + "-Commits:Spin:System"),
      theCommitCount_Spin_Trap(theName + "-Commits:Spin:Trap"),
      theCommitCount_Spin_Idle(theName + "-Commits:Spin:Idle"),
      theLSQOccupancy(theName + "-Occupancy:LSQ"), theSBOccupancy(theName + "-Occupancy:SB"),
      theSBUniqueOccupancy(theName + "-Occupancy:SB:UniqueBlocks"),
      theSBNAWOccupancy(theName + "-Occupancy:SBNAW"), theSpinCount(theName + "-Spins"),
      theSpinCycles(theName + "-SpinCycles"), theStorePrefetches(theName + "-StorePrefetches"),
      theAtomicPrefetches(theName + "-AtomicPrefetches"),
      theStorePrefetchConflicts(theName + "-StorePrefetchConflicts"),
      theStorePrefetchDuplicates(theName + "-StorePrefetchDuplicates"),
      thePartialSnoopLoads(theName + "-PartialSnoopLoads"), theRaces(theName + "-Races"),
      theRaces_LoadReplayed_User(theName + "-Races:LoadReplay:User"),
      theRaces_LoadReplayed_System(theName + "-Races:LoadReplay:System"),
      theRaces_LoadForwarded_User(theName + "-Races:LoadFwd:User"),
      theRaces_LoadForwarded_System(theName + "-Races:LoadFwd:System"),
      theRaces_LoadAlreadyOrdered_User(theName + "-Races:LoadOrderOK:User"),
      theRaces_LoadAlreadyOrdered_System(theName + "-Races:LoadOrderOK:System"),
      theSpeculations_Started(theName + "-Speculations:Started"),
      theSpeculations_Successful(theName + "-Speculations:Successful"),
      theSpeculations_Rollback(theName + "-Speculations:Rollback"),
      theSpeculations_PartialRollback(theName + "-Speculations:PartialRollback"),
      theSpeculations_Resync(theName + "-Speculations:Resync"),
      theSpeculativeCheckpoints(theName + "-SpeculativeCheckpoints"),
      theMaxSpeculativeCheckpoints(theName + "-SpeculativeCheckpoints:Max"),
      theROBOccupancyTotal(theName + "-ROB-Occupancy:Total"),
      theROBOccupancySpin(theName + "-ROB-Occupancy:Spin"),
      theROBOccupancyNonSpin(theName + "-ROB-Occupancy:NonSpin"),
      theSideEffectOnChip(theName + "-SideEffects:OnChip"),
      theSideEffectOffChip(theName + "-SideEffects:OffChip"),
      theNonSpeculativeAtomics(theName + "-NonSpeculativeAtomics"),
      theRequiredDiscards(theName + "-Discards:Required"),
      theRequiredDiscards_Histogram(theName + "-Discards:Required:Histogram"),
      theNearestCkptDiscards(theName + "-Discards:ToCkpt"),
      theNearestCkptDiscards_Histogram(theName + "-Discards:ToCkpt:Histogram"),
      theSavedDiscards(theName + "-Discards:Saved"),
      theSavedDiscards_Histogram(theName + "-Discards:Saved:Histogram"),
      theCheckpointsDiscarded(theName + "-RollbackOverNCheckpoints"),
      theRollbackToCheckpoint(theName + "-RollbackToCheckpoint"),
      theWP_CoherenceMiss(theName + "-WP:CoherenceMisses"), theWP_SVBHit(theName + "-WP:SVBHits"),
      theResync_GarbageCollect(theName + "-Resync:GarbageCollect"),
      theResync_AbortedSpec(theName + "-Resync:AbortedSpec"),
      theResync_BlackBox(theName + "-Resync:BlackBox"), theResync_MAGIC(theName + "-Resync:MAGIC"),
      theResync_ITLBMiss(theName + "-Resync:ITLBMiss"),
      theResync_UnimplementedAnnul(theName + "-Resync:UnimplementedAnnul"),
      theResync_RDPRUnsupported(theName + "-Resync:RDPRUnsupported"),
      theResync_WRPRUnsupported(theName + "-Resync:WRPRUnsupported"),
      theResync_MEMBARSync(theName + "-Resync:MEMBARSync"),
      theResync_UnexpectedException(theName + "-Resync:UnexpectedException"),
      theResync_Interrupt(theName + "-Resync:Interrupt"),
      theResync_DeviceAccess(theName + "-Resync:DeviceAccess"),
      theResync_FailedValidation(theName + "-Resync:FailedValidation"),
      theResync_FailedHandleTrap(theName + "-Resync:FailedHandleTrap"),
      theResync_SideEffectLoad(theName + "-Resync:SideEffectLoad"),
      theResync_SideEffectStore(theName + "-Resync:SideEffectStore"),
      theResync_Unknown(theName + "-Resync:Unknown"),
      theResync_CPUHaltedState(theName + "-Resync:CPUHalted"),
      theFalseITLBMiss(theName + "-FalseITLBMiss"), theEpochs(theName + "-MLPEpoch"),
      theEpochs_Instructions(theName + "-MLPEpoch:Instructions"),
      theEpochs_Instructions_Avg(theName + "-MLPEpoch:Instructions:Avg"),
      theEpochs_Instructions_StdDev(theName + "-MLPEpoch:Instructions:StdDev"),
      theEpochs_Loads(theName + "-MLPEpoch:Loads"),
      theEpochs_Loads_Avg(theName + "-MLPEpoch:Loads:Avg"),
      theEpochs_Loads_StdDev(theName + "-MLPEpoch:Loads:StdDev"),
      theEpochs_Stores(theName + "-MLPEpoch:Stores"),
      theEpochs_Stores_Avg(theName + "-MLPEpoch:Stores:Avg"),
      theEpochs_Stores_StdDev(theName + "-MLPEpoch:Stores:StdDev"),
      theEpochs_OffChipStores(theName + "-MLPEpoch:Stores:OffChip"),
      theEpochs_OffChipStores_Avg(theName + "-MLPEpoch:Stores:OffChip:Avg"),
      theEpochs_OffChipStores_StdDev(theName + "-MLPEpoch:Stores:OffChip:StdDev"),
      theEpochs_StoreAfterOffChipStores(theName + "-MLPEpoch:StoresAfterOffChipStore"),
      theEpochs_StoreAfterOffChipStores_Avg(theName + "-MLPEpoch:StoresAfterOffChipStore:Avg"),
      theEpochs_StoreAfterOffChipStores_StdDev(theName +
                                               "-MLPEpoch:StoresAfterOffChipStore:StdDev"),
      theEpochs_StoresOutstanding(theName + "-MLPEpoch:StoresOutstanding"),
      theEpochs_StoresOutstanding_Avg(theName + "-MLPEpoch:StoresOutstanding:Avg"),
      theEpochs_StoresOutstanding_StdDev(theName + "-MLPEpoch:StoresOutstanding:StdDev"),
      theEpochs_StoreBlocks(theName + "-MLPEpoch:StoreBlocks"),
      theEpochs_LoadOrStoreBlocks(theName + "-MLPEpoch:LoadOrStoreBlocks"), theEpoch_StartInsn(0),
      theEpoch_LoadCount(0), theEpoch_StoreCount(0), theEpoch_OffChipStoreCount(0),
      theEpoch_StoreAfterOffChipStoreCount(0), theEmptyROBCause(kSync), theLastCycleIStalls(0),
      theRetireCount(0), theTimeBreakdown(theName + "-TB"), theMix_Total(theName + "-Mix:Total"),
      theMix_Exception(theName + "-Mix:Exception"), theMix_Load(theName + "-Mix:Load"),
      theMix_Store(theName + "-Mix:Store"), theMix_Atomic(theName + "-Mix:Atomic"),
      theMix_Branch(theName + "-Mix:Branch"), theMix_MEMBAR(theName + "-Mix:MEMBAR"),
      theMix_Computation(theName + "-Mix:Computation"),
      theMix_Synchronizing(theName + "-Mix:Synchronizing"),
      theAtomicVal_RMWs(theName + "-AtmVal:RMW"),
      theAtomicVal_RMWs_Zero(theName + "-AtmVal:RMW:Zero"),
      theAtomicVal_RMWs_NonZero(theName + "-AtmVal:RMW:NonZero"),
      theAtomicVal_CASs(theName + "-AtmVal:CAS"),
      theAtomicVal_CASs_Mismatch(theName + "-AtmVal:CAS:Mismatch"),
      theAtomicVal_CASs_MismatchAfterMismatch(theName + "-AtmVal:CAS:Mismatch:AfterMismatch"),
      theAtomicVal_CASs_Match(theName + "-AtmVal:CAS:Match"),
      theAtomicVal_CASs_MatchAfterMismatch(theName + "-AtmVal:CAS:Match:AfterMismatch"),
      theAtomicVal_CASs_Zero(theName + "-AtmVal:CAS:Zero"),
      theAtomicVal_CASs_NonZero(theName + "-AtmVal:CAS:NonZero"),
      theAtomicVal_LastCASMismatch(false), theCoalescedStores(theName + "-CoalescedStores"),
      intAluOpLatency(options.intAluOpLatency),
      intAluOpPipelineResetTime(options.intAluOpPipelineResetTime),
      intMultOpLatency(options.intMultOpLatency),
      intMultOpPipelineResetTime(options.intMultOpPipelineResetTime),
      intDivOpLatency(options.intDivOpLatency),
      intDivOpPipelineResetTime(options.intDivOpPipelineResetTime),
      fpAddOpLatency(options.fpAddOpLatency),
      fpAddOpPipelineResetTime(options.fpAddOpPipelineResetTime),
      fpCmpOpLatency(options.fpCmpOpLatency),
      fpCmpOpPipelineResetTime(options.fpCmpOpPipelineResetTime),
      fpCvtOpLatency(options.fpCvtOpLatency),
      fpCvtOpPipelineResetTime(options.fpCvtOpPipelineResetTime),
      fpMultOpLatency(options.fpMultOpLatency),
      fpMultOpPipelineResetTime(options.fpMultOpPipelineResetTime),
      fpDivOpLatency(options.fpDivOpLatency),
      fpDivOpPipelineResetTime(options.fpDivOpPipelineResetTime),
      fpSqrtOpLatency(options.fpSqrtOpLatency),
      fpSqrtOpPipelineResetTime(options.fpSqrtOpPipelineResetTime),
      // Each FU starts ready to accept an operation
      intAluCyclesToReady(options.numIntAlu, 0), intMultCyclesToReady(options.numIntMult, 0),
      fpAluCyclesToReady(options.numFpAlu, 0), fpMultCyclesToReady(options.numFpMult, 0)
{

  theQEMUCPU = Flexus::Qemu::Processor::getProcessor(theNode);

  // original constructor continues here...
  prepareMemOpAccounting();

  std::vector<uint32_t> reg_file_sizes;
  reg_file_sizes.resize(kLastMapTableCode + 2);
  reg_file_sizes[xRegisters] = kxRegs + 3 * theROBSize;
  reg_file_sizes[vRegisters] = kvRegs + 4 * theROBSize;
  theRegisters.initialize(reg_file_sizes);

  // Map table for xRegisters
  theMapTables.push_back(std::make_shared<PhysicalMap>(kxRegs, reg_file_sizes[xRegisters]));

  // Map table for vRegisters
  theMapTables.push_back(std::make_shared<PhysicalMap>(kvRegs, reg_file_sizes[vRegisters]));

  reset();

  theCommitUSArray[0] = &theCommitCount_NonSpin_User;
  theCommitUSArray[1] = &theCommitCount_NonSpin_System;
  theCommitUSArray[2] = &theCommitCount_NonSpin_Trap;
  theCommitUSArray[3] = &theCommitCount_NonSpin_Idle;
  theCommitUSArray[4] = &theCommitCount_Spin_User;
  theCommitUSArray[5] = &theCommitCount_Spin_System;
  theCommitUSArray[6] = &theCommitCount_Spin_Trap;
  theCommitUSArray[7] = &theCommitCount_Spin_Idle;

  for (int32_t i = 0; i < codeLastCode; ++i) {
    std::stringstream user_name;
    user_name << theName << "-InsnCount:User:" << eInstructionCode(i);
    std::stringstream system_name;
    system_name << theName + "-InsnCount:System:" << eInstructionCode(i);
    std::stringstream trap_name;
    trap_name << theName + "-InsnCount:Trap:" << eInstructionCode(i);
    std::stringstream idle_name;
    idle_name << theName + "-InsnCount:Idle:" << eInstructionCode(i);

    theCommitsByCode[0].push_back(new Stat::StatCounter(user_name.str()));
    theCommitsByCode[1].push_back(new Stat::StatCounter(system_name.str()));
    theCommitsByCode[2].push_back(new Stat::StatCounter(trap_name.str()));
    theCommitsByCode[3].push_back(new Stat::StatCounter(idle_name.str()));
  }

  kTBUser = theTimeBreakdown.addClass("User");
  kTBSystem = theTimeBreakdown.addClass("System");
  kTBTrap = theTimeBreakdown.addClass("Trap");
  kTBIdle = theTimeBreakdown.addClass("Idle");

  theLastStallCause = nXactTimeBreakdown::kUnknown;
  theCycleCategory = kTBUser;

  cpuHalted = false;
}

void CoreImpl::skipCycle() {
  theTimeBreakdown.skipCycle();
}

void CoreImpl::cycle(eExceptionType aPendingInterrupt) {
  // qemu warmup
  if (theFlexus->cycleCount() == 1) {
    advance_fn(true);
    throw ResynchronizeWithQemuException(true);
  }

  CORE_DBG("--------------START CORE------------------------");

  FLEXUS_PROFILE();

  if (aPendingInterrupt != thePendingInterrupt) {

    DBG_(Dev, (<< "Interrupting..."));

    if (aPendingInterrupt == kException_None) {
      DBG_(Dev, (<< theName << " Interrupt: " << thePendingInterrupt << " pending for "
                 << (theFlexus->cycleCount() - theInterruptReceived)));
    } else {
      theInterruptReceived = theFlexus->cycleCount();
    }
    thePendingInterrupt = aPendingInterrupt;
  }

  if (nDecoder::theInsnCount > 10000ULL &&
      (static_cast<uint64_t>(theFlexus->cycleCount() - theLastGarbageCollect) >
       1000ULL - nDecoder::theInsnCount / 100)) {
    DBG_(VVerb,
         (<< theName << "Garbage-collect count before clean:  " << nDecoder::theInsnCount));

    FLEXUS_PROFILE_N("CoreImpl::cycle() collect-dead-dependencies");
    DBG_(VVerb,
         (<< theName << "Garbage-collect count before clean:  " << nDecoder::theInsnCount));
    theBypassNetwork.collectAll();
    DBG_(VVerb, (<< theName << "Garbage-collect between bypass and registers:  "
                 << nDecoder::theInsnCount));
    theRegisters.collectAll();
    DBG_(VVerb,
         (<< theName << "Garbage-collect count after clean:  " << nDecoder::theInsnCount));
    theLastGarbageCollect = theFlexus->cycleCount();

    if (nDecoder::theInsnCount > 1000000) {
      DBG_(VVerb, (<< theName
                   << "Garbage-collect detects too many live instructions.  "
                      "Forcing resynchronize."));
      ++theResync_GarbageCollect;
      throw ResynchronizeWithQemuException();
    }
  }

  if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 3) {
    dumpROB();
    dumpSRB();
    dumpMemQueue();
    dumpMSHR();
    dumpCheckpoints();
    dumpSBPermissions();
  }

  if (theIsSpeculating) {
    CORE_DBG("Speculating");
    DBG_Assert(theCheckpoints.size() > 0);
    DBG_Assert(!theSRB.empty());
    DBG_Assert(theSBCount + theSBNAWCount > 0);
    DBG_Assert(theOpenCheckpoint != 0);
    DBG_Assert(theCheckpoints.find(theOpenCheckpoint) != theCheckpoints.end());

  } else {
    CORE_DBG("Not Speculating");

    DBG_Assert(theCheckpoints.size() == 0);
    DBG_Assert(theSRB.empty());
    DBG_Assert(theOpenCheckpoint == 0);
  }

  DBG_Assert((theSBCount + theSBNAWCount) >= 0);
  DBG_Assert((static_cast<int>(theSBLines_Permission.size())) <= theSBCount + theSBNAWCount);

  DBG_(VVerb, (<< "*** Prepare *** "));

  processMemoryReplies();
  prepareCycle();

  DBG_(VVerb, (<< "*** Eval *** "));
  evaluate();

  DBG_(VVerb, (<< "*** Issue Mem *** "));

  issuePartialSnoop();
  issueStore();
  issueAtomic();
  issueAtomicSpecWrite();
  issueSpecial();
  valuePredictAtomic();
  //  checkExtraLatencyTimeout();
  resolveCheckpoint();

  DBG_(Verb, (<< "*** Arb *** "));
  arbitrate();

  if (cpuHalted) {
    int qemu_rcode = advance_fn(false); // don't count instructions in halt state
    if (qemu_rcode != QEMU_HALT_CODE) {
      DBG_(Dev, (<< "Core " << theNode << " leaving halt state, after QEMU sent execution code "
                 << qemu_rcode));
      cpuHalted = false;
    }
    throw ResynchronizeWithQemuException(true);
  }

  // Retire instruction from the ROB to the SRB
  retire();

  if (theRetireCount > 0) {
    theIdleThisCycle = false;
  }

  // Do cycle accounting
  completeAccounting();

  if (theTSOBReplayStalls > 0) {
    --theTSOBReplayStalls;
  }

  while (!theBranchFeedback.empty()) {
    feedback_fn(theBranchFeedback.front());
    DBG_(Verb, (<< " Sent Branch Feedback"));
    theBranchFeedback.pop_front();
    theIdleThisCycle = false;
  }

  if (theSquashRequested) {
    DBG_(Verb, (<< " Core triggering Squash: " << theSquashReason));
    doSquash();
    squash_fn(theSquashReason);
    theSquashRequested = false;
    theIdleThisCycle = false;
  }

  // Commit instructions the SRB and compare to simics
  commit();

  handleTrap();

  if (theRedirectRequested) {
    DBG_(Iface, (<< " Core triggering Redirect to " << theRedirectPC));
    redirect_fn(theRedirectPC);
    thePC = theRedirectPC;
    theRedirectRequested = false;
    theIdleThisCycle = false;
  }
  if (theIdleThisCycle) {
    ++theIdleCycleCount;
  } else {
    theIdleCycleCount = 0;
  }
  theIdleThisCycle = true;

  rob_t::iterator i;
  for (i = theROB.begin(); i != theROB.end(); ++i) {
    i->get()->decrementCanRetireCounter();
  }

  CORE_DBG("--------------FINISH CORE------------------------");
}

std::string CoreImpl::dumpState() {
  std::stringstream ss;

  ss << std::hex << "PC=" << std::setw(16) << std::setfill('0') << (uint64_t)theDumpPC << std::dec << std::endl;

  for (int i = 0; i < kxRegs; i++) {
    mapped_reg mreg;
    mreg.theType = xRegisters;
    mreg.theIndex = theMapTables[0]->mapArchitectural(i);

    ss << "X" << std::setw(2) << std::setfill('0') << i << "=" << std::hex << std::setw(16) << std::setfill('0') << boost::get<uint64_t>(theRegisters.peek(mreg)) << std::dec;

    if ((i % 4) == 3) {
      ss << std::endl;
    } else {
      ss << " ";
    }
  }

  return ss.str();
}

bool CoreImpl::checkValidatation() {
  theFlexusDumpState = dumpState();
  theQemuDumpState = Flexus::Qemu::Processor::getProcessor(theNode).dump_state();

  bool ret = (theFlexusDumpState.compare(theQemuDumpState) == 0);
  if (!ret) {

    // Vector of string to save tokens
    std::vector<std::string> flex_diff, qemu_diff, diff;

    std::replace(theFlexusDumpState.begin(), theFlexusDumpState.end(), '\n', ' ');
    std::replace(theQemuDumpState.begin(), theQemuDumpState.end(), '\n', ' ');

    // stringstream class check1
    std::stringstream flex_check(theFlexusDumpState);
    std::stringstream qemu_check(theQemuDumpState);

    std::string iflex, iqemu;

    // Tokenizing w.r.t. space ' '
    while (std::getline(flex_check, iflex, ' ')) {
      flex_diff.push_back(iflex);
    }
    while (std::getline(qemu_check, iqemu, ' ')) {
      qemu_diff.push_back(iqemu);
    }

    for (size_t i = 0; i < flex_diff.size() && i < qemu_diff.size(); i++) {
      if (flex_diff[i].compare(qemu_diff[i]) != 0) {
        diff.push_back("flexus: " + flex_diff[i]);
        diff.push_back("qemu:   " + qemu_diff[i]);
      }
    }

    if (diff.size() > 0) {
      DBG_(Dev, (<< "state mismatch: "));
      for (auto &i : diff) {
        DBG_(Dev, (<< i));
      }
    }
  }

  return ret;
}

void CoreImpl::prepareCycle() {
  FLEXUS_PROFILE();
  thePreserveInteractions = false;
  if (!theRescheduledActions.empty() || !theActiveActions.empty()) {
    theIdleThisCycle = false;
  }

  std::swap(theRescheduledActions, theActiveActions);
}

void CoreImpl::arbitrate() {
  FLEXUS_PROFILE();
  theMemoryPortArbiter.arbitrate();
}

void CoreImpl::evaluate() {
  FLEXUS_PROFILE();
  CORE_DBG("--------------START EVALUATING------------------------");

  while (!theActiveActions.empty()) {
    theActiveActions.top()->evaluate();
    theActiveActions.pop();
  }
  CORE_DBG("--------------FINISH EVALUATING------------------------");
}

void CoreImpl::satisfy(InstructionDependance const &aDep) {
  aDep.satisfy();
}

void CoreImpl::squash(InstructionDependance const &aDep) {
  aDep.squash();
}

void CoreImpl::satisfy(std::list<InstructionDependance> &dependances) {
  std::list<InstructionDependance>::iterator iter = dependances.begin();
  std::list<InstructionDependance>::iterator end = dependances.end();
  while (iter != end) {
    iter->satisfy();
    ++iter;
  }
}

void CoreImpl::squash(std::list<InstructionDependance> &dependances) {
  std::list<InstructionDependance>::iterator iter = dependances.begin();
  std::list<InstructionDependance>::iterator end = dependances.end();
  while (iter != end) {
    iter->squash();
    ++iter;
  }
}

void CoreImpl::clearExclusiveLocal() {
  theLocalExclusivePhysicalMonitor.clear();
  theLocalExclusiveVirtualMonitor.clear();
}

void CoreImpl::clearExclusiveGlobal() {
  GLOBAL_EXCLUSIVE_MONITOR[theNode].clear();
}

void CoreImpl::markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker) {
  if (theLocalExclusivePhysicalMonitor.find(anAddress) != theLocalExclusivePhysicalMonitor.end()) {
    theLocalExclusivePhysicalMonitor[anAddress] = (marker << 8) | aSize;
    return;
  }
  if (theLocalExclusivePhysicalMonitor.size() > 0) {
    DBG_(Dev, (<< "Only one local exclusive tag allowed per core, clearing original! " << anAddress
               << ", " << marker));
    clearExclusiveLocal();
  }
  theLocalExclusivePhysicalMonitor[anAddress] = (marker << 8) | aSize;
}

void CoreImpl::markExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker) {
  GLOBAL_EXCLUSIVE_MONITOR[theNode][anAddress] = (marker << 8) | aSize;
}

void CoreImpl::markExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize, uint64_t marker) {
  if (theLocalExclusiveVirtualMonitor.find(anAddress) != theLocalExclusiveVirtualMonitor.end()) {
    theLocalExclusiveVirtualMonitor[anAddress] = (marker << 8) | aSize;
    return;
  }
  if (theLocalExclusiveVirtualMonitor.size() > 0) {
    DBG_(Dev, (<< "Only one local exclusive tag allowed per core, clearing original! " << anAddress
               << ", " << marker));
    clearExclusiveLocal();
  }
  theLocalExclusiveVirtualMonitor[anAddress] = (marker << 8) | aSize;
}

int CoreImpl::isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize) {
  if (theLocalExclusivePhysicalMonitor.find(anAddress) == theLocalExclusivePhysicalMonitor.end())
    return kMonitorDoesntExist;
  return int(theLocalExclusivePhysicalMonitor[anAddress] >> 8);
}

int CoreImpl::isExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize) {
  if (GLOBAL_EXCLUSIVE_MONITOR[theNode].find(anAddress) == GLOBAL_EXCLUSIVE_MONITOR[theNode].end())
    return kMonitorDoesntExist;
  return int(GLOBAL_EXCLUSIVE_MONITOR[theNode][anAddress] >> 8);
}

int CoreImpl::isExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize) {
  if (theLocalExclusiveVirtualMonitor.find(anAddress) == theLocalExclusiveVirtualMonitor.end())
    return kMonitorDoesntExist;
  return int(theLocalExclusiveVirtualMonitor[anAddress] >> 8);
}

bool CoreImpl::isROBHead(boost::intrusive_ptr<Instruction> anInstruction) {
  return (anInstruction == theROB.front());
}

bool CoreImpl::mayRetire_MEMBARStSt() const {
  if (consistencyModel() != kRMO) {
    return true;
  } else {
    return sbEmpty();
  }
}

bool CoreImpl::mayRetire_MEMBARStLd() const {
  switch (consistencyModel()) {
  case kSC:
    if (theSpeculativeOrder) {
      return !sbFull();
    } else {
      return sbEmpty();
    }
  case kTSO:
    DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
    if (theSpeculativeOrder) {
      return !sbFull() &&
             (theAllowedSpeculativeCheckpoints == 0
                  ? true
                  : (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints);
    } else {
      return sbEmpty();
    }
  case kRMO:
    DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
    if (theSpeculativeOrder) {
      return !sbFull() &&
             (theAllowedSpeculativeCheckpoints == 0
                  ? true
                  : (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints);
    } else {
      return sbEmpty();
    }
  }

  return true;
}

bool CoreImpl::mayRetire_MEMBARSync() const {
  if (theSBNAWCount > 0 && theNAWWaitAtSync) {
    return false;
  }
  return mayRetire_MEMBARStLd();
}

void CoreImpl::spinDetect(memq_t::index<by_insn>::type::iterator iter) {
  if (theSpinning) {
    // Check leave conditions
    bool leave = false;

    if (!leave && iter->theOperation != kLoad) {
      leave = true;
    }

    if (!leave && iter->isAbnormalAccess()) {
      leave = true;
    }

    if (!leave && theSpinMemoryAddresses.count(iter->thePaddr) == 0) {
      leave = true;
    }

    if (!leave && theSpinPCs.count(iter->theInstruction->pc()) == 0) {
      leave = true;
    }

    ++theSpinDetectCount;

    if (leave) {
      std::string spin_string = "Addrs {";
      std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
      while (ma_iter != theSpinMemoryAddresses.end()) {
        spin_string += std::to_string(*ma_iter) + ",";
        ++ma_iter;
      }
      spin_string += "} PCs {";
      std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
      while (va_iter != theSpinPCs.end()) {
        spin_string += std::to_string(*va_iter) + ",";
        ++va_iter;
      }
      spin_string += "}";
      DBG_(Iface, (<< theName << " leaving spin mode. " << spin_string));

      theSpinning = false;
      theSpinCount += theSpinDetectCount;
      theSpinCycles += theFlexus->cycleCount() - theSpinStartCycle;
      theSpinRetireSinceReset = 0;
      theSpinDetectCount = 0;
      theSpinMemoryAddresses.clear();
      theSpinPCs.clear();
    }
  } else {
    // Check all reset conditions
    bool reset = false;

    if (!reset && iter->theOperation != kLoad) {
      reset = true;
    }

    if (!reset && iter->isAbnormalAccess()) {
      reset = true;
    }

    theSpinMemoryAddresses.insert(iter->thePaddr);
    if (!reset && theSpinMemoryAddresses.size() > 3) {
      reset = true;
    }

    theSpinPCs.insert(iter->theInstruction->pc());
    if (!reset && theSpinPCs.size() > 3) {
      reset = true;
    }

    ++theSpinDetectCount;
    if (reset) {
      theSpinRetireSinceReset = 0;
      theSpinDetectCount = 0;
      theSpinMemoryAddresses.clear();
      theSpinPCs.clear();
    } else {
      // Check condition for entering spin mode
      if (theSpinDetectCount > 24 || theSpinRetireSinceReset > 99) {
        std::string spin_string = "Addrs {";
        std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
        while (ma_iter != theSpinMemoryAddresses.end()) {
          spin_string += std::to_string(*ma_iter) + ",";
          ++ma_iter;
        }
        spin_string += "} PCs {";
        std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
        while (va_iter != theSpinPCs.end()) {
          spin_string += std::to_string(*va_iter) + ",";
          ++va_iter;
        }
        spin_string += "}";
        DBG_(Iface, (<< theName << " entering spin mode. " << spin_string));

        theSpinning = true;
        theSpinStartCycle = theFlexus->cycleCount();
      }
    }
  }
}

void CoreImpl::retireMem(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(iter != theMemQueue.get<by_insn>().end());

  // Spin detection / control
  if (!anInsn->isAnnulled()) {
    spinDetect(iter);
  }

  DBG_Assert(iter->theQueue == kLSQ);

  if (iter->theOperation == kCAS || iter->theOperation == kRMW) {
    if (!iter->isAbnormalAccess() && iter->thePaddr != 0) {
      PhysicalMemoryAddress block_addr(static_cast<uint64_t>(iter->thePaddr) &
                                       ~(theCoherenceUnit - 1));
      addSLATEntry(block_addr, anInsn);

      // TRACE TRACKER : Notify trace tracker of store
      // uint64_t logical_timestamp = theCommitNumber + theSRB.size();
      theTraceTracker.store(theNode, eCore, iter->thePaddr, anInsn->pc(), false /*unknown*/,
                            anInsn->isPriv(), 0);
      /* CMU-ONLY-BLOCK-BEGIN */
      //      if (theTrackParallelAccesses ) {
      //        theTraceTracker.parallelList(theNode, eCore, iter->thePaddr,
      //        iter->theParallelAddresses);
      /*
                DBG_( Dev,  ( << theName << " CAS/RMW " << std::hex <<
         iter->thePaddr << std::dec << " parallel set follows " ) ); std::set<
         PhysicalMemoryAddress >::iterator pal_iter =
         iter->theParallelAddresses.begin(); std::set< PhysicalMemoryAddress
         >::iterator pal_end = iter->theParallelAddresses.end(); while (pal_iter
         != pal_end ) { DBG_ ( Dev, ( << "    " << std::hex << *pal_iter <<
         std::dec ) );
                  ++pal_iter;
                }
       */
      //      }
      /* CMU-ONLY-BLOCK-END */
    }

    if (iter->theOperation == kRMW) {
      ++theAtomicVal_RMWs;
      DBG_Assert(iter->theExtendedValue);
      if (*iter->theExtendedValue == 0) {
        ++theAtomicVal_RMWs_Zero;
      } else {
        ++theAtomicVal_RMWs_NonZero;
      }
    } else { /* CAS */
      ++theAtomicVal_CASs;
      DBG_Assert(iter->theExtendedValue, (<< *iter));
      DBG_Assert(iter->theCompareValue, (<< *iter));
      if (*iter->theExtendedValue == 0) {
        ++theAtomicVal_CASs_Zero;
      } else {
        ++theAtomicVal_CASs_NonZero;
      }
      if (*iter->theExtendedValue == *iter->theCompareValue) {
        ++theAtomicVal_CASs_Match;
        if (theAtomicVal_LastCASMismatch) {
          ++theAtomicVal_CASs_MatchAfterMismatch;
        }
        theAtomicVal_LastCASMismatch = false;
      } else {
        ++theAtomicVal_CASs_Mismatch;
        if (theAtomicVal_LastCASMismatch) {
          ++theAtomicVal_CASs_MismatchAfterMismatch;
        }
        theAtomicVal_LastCASMismatch = true;
      }
    }

    if (iter->theStoreComplete == true) {
      // Completed non-speculatively
      eraseLSQ(anInsn); // Will setAccessAddress
    } else {
      // Speculatively retiring an atomic
      DBG_Assert(theSpeculativeOrder);
      DBG_(Trace, (<< theName << " Spec-Retire Atomic: " << *iter));

      // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
      // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
      theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
      --theLSQCount;
      DBG_Assert(theLSQCount >= 0);
      ++theSBCount;

      iter->theIssued = false;

      DBG_Assert(iter->thePaddr != kUnresolved);
      DBG_Assert(!iter->isAbnormalAccess());
      iter->theInstruction->setAccessAddress(iter->thePaddr);

      createCheckpoint(iter->theInstruction);
      requireWritePermission(iter);
    }
  } else if (iter->theOperation == kLoad) {
    DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
    bool speculate = false;
    if (!iter->isAbnormalAccess() && ((iter->thePaddr & ~(theCoherenceUnit - 1)) != 0)) {
      if (consistencyModel() == kSC && theSpeculativeOrder) {
        if ((!isSpeculating() && !sbEmpty()) // mandatory checkpoint
            ||
            (isSpeculating() // optional checkpoint
             && theCheckpointThreshold > 0 && theRetiresSinceCheckpoint > theCheckpointThreshold &&
             (theAllowedSpeculativeCheckpoints == 0 ||
              (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints)) ||
            (isSpeculating() // Checkpoint may not require more than 15 stores
             && theCheckpoints[theOpenCheckpoint].theRequiredPermissions.size() > 15 &&
             (theAllowedSpeculativeCheckpoints == 0 ||
              (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints))) {
          speculate = true;
          // Must enter speculation mode to retire load past store
          DBG_(Verb, (<< theName << " Entering SC speculation on: " << *iter));
          iter->theInstruction->setMayCommit(false);

          // Move the MEMBAR to the SSB and update counts
          // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
          // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
          theMemQueue.get<by_insn>().modify(iter,
                                            ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
          --theLSQCount;
          DBG_Assert(theLSQCount >= 0);
          ++theSBNAWCount;
          iter->theBypassSB = true;
          createCheckpoint(iter->theInstruction);
        }
      }

      PhysicalMemoryAddress block_addr(static_cast<uint64_t>(iter->thePaddr) &
                                       ~(theCoherenceUnit - 1));
      addSLATEntry(block_addr, anInsn);

      // TRACE TRACKER : Notify trace tracker of load commit
      // uint64_t logical_timestamp = theCommitNumber + theSRB.size();
      theTraceTracker.access(theNode, eCore, iter->thePaddr, anInsn->pc(), false, false, false,
                             anInsn->isPriv(), 0);
      theTraceTracker.commit(theNode, eCore, iter->thePaddr, anInsn->pc(), 0);
      /* CMU-ONLY-BLOCK-BEGIN */
      //      if (theTrackParallelAccesses ) {
      //        theTraceTracker.parallelList(theNode, eCore, iter->thePaddr,
      //        iter->theParallelAddresses);
      //      }
      /* CMU-ONLY-BLOCK-END */
    }
    if (!speculate) {
      eraseLSQ(anInsn); // Will setAccessAddress
    }
  } else if (iter->theOperation == kStore) {
    if (anInsn->isAnnulled() || iter->isAbnormalAccess()) {
      //      if (! anInsn->isAnnulled()  && iter->theMMU) {
      ////        DBG_Assert(mmuASI(iter->theASI));
      //        DBG_( Verb, ( << theName << " MMU write: " << *iter ) );
      ////        Flexus::Qemu::Processor::getProcessor( theNode
      ///)->mmuWrite(iter->theVaddr/*, iter->theASI*/, *iter->theValue );
      //      }
      eraseLSQ(anInsn);
    } else {
      // See if this store is to the same cache block as the preceding store
      memq_t::index<by_seq>::type::iterator pred = theMemQueue.project<by_seq>(iter);
      if (pred != theMemQueue.begin()) {
        --pred;
        DBG_Assert(pred->theQueue != kLSQ);
        PhysicalMemoryAddress pred_aligned(pred->thePaddr & ~(theCoherenceUnit - 1));
        PhysicalMemoryAddress iter_aligned(iter->thePaddr & ~(theCoherenceUnit - 1));
        if (iter_aligned != 0 && pred_aligned == iter_aligned && pred->theOperation == kStore) {
          // Can coalesce this with preceding SB enry
          ++theCoalescedStores;
          iter->theBypassSB = true; // HEHE
        }
      }

      // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
      // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
      theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
      --theLSQCount;
      DBG_Assert(theLSQCount >= 0);

      if (iter->theBypassSB) {
        ++theSBNAWCount;
      } else {
        ++theSBCount;
      }

      // TRACE TRACKER : Notify trace tracker of store
      //      uint64_t logical_timestamp = theCommitNumber + theSRB.size();
      theTraceTracker.store(theNode, eCore, iter->thePaddr, anInsn->pc(), false /*unknown*/,
                            anInsn->isPriv(), 0);

      requireWritePermission(iter);
    }
  } else if (iter->theOperation == kMEMBARMarker) {
    // Retiring a MEMBAR Marker
    if (sbEmpty()) {
      DBG_(Verb, (<< theName << " Non-Spec-Retire MEMBAR: " << *iter));
      iter->theInstruction->resolveSpeculation();
      eraseLSQ(anInsn);
    } else {
      DBG_(Verb, (<< theName << " Spec-Retire MEMBAR: " << *iter));

      //! sbFull() should be in the retirement constraint for MEMBARs
      DBG_Assert(!sbFull());

      // Move the MEMBAR to the SSB and update counts
      // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
      // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
      theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
      --theLSQCount;
      DBG_Assert(theLSQCount >= 0);
      ++theSBCount;

      if (theConsistencyModel == kSC) {
        if (!isSpeculating()) {
          createCheckpoint(iter->theInstruction);
        } else {
          iter->theInstruction->resolveSpeculation(); // Will commit with its atomic sequence
        }
      } else if (theConsistencyModel == kTSO) {
        createCheckpoint(iter->theInstruction);
      } else if (theConsistencyModel == kRMO) { // FIXME
        createCheckpoint(iter->theInstruction);
      }
    }
  } else {
    DBG_Assert(false, (<< "Tried to retire non-load or non-store: " << *iter));
  }
}

void CoreImpl::checkPageFault(boost::intrusive_ptr<Instruction> anInsn) {
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(iter != theMemQueue.get<by_insn>().end());

  if (iter->theException != kException_None) {
    DBG_(Dev, (<< theName << " Taking MMU exception: " << iter->theException << " " << *iter));
    takeTrap(anInsn, iter->theException);
    anInsn->setAccessAddress(PhysicalMemoryAddress(0));
    //      anInsn->squash();
  }
}

void CoreImpl::commitStore(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  if (iter != theMemQueue.get<by_insn>().end()) {
    DBG_Assert(iter->theOperation == kStore);
    DBG_Assert(iter->theQueue == kSSB);
    DBG_Assert(!anInsn->isAnnulled());
    DBG_Assert(!anInsn->isSquashed());
    DBG_Assert(!iter->isAbnormalAccess());
    DBG_Assert(iter->theValue);
    DBG_Assert(iter->thePaddr != 0);

    bits value = *iter->theValue;
    //    if (iter->theInverseEndian) {
    //      value = Flexus::Qemu::endianFlip(value, iter->theSize);
    //      DBG_(Verb, ( << theName << " Inverse endian store: " << *iter << "
    //      inverted value: " <<  std::hex << value << std::dec ) );
    //    }
    ValueTracker::valueTracker(theNode).store(theNode, iter->thePaddr, iter->theSize, value);
    // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSB;
    // });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSB );
    theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSB);
  }
}

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

void CoreImpl::accessMem(PhysicalMemoryAddress anAddress,
                         boost::intrusive_ptr<Instruction> anInsn) {
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
    ValueTracker::valueTracker(theNode).access(theNode, anAddress);
  }
}

void CoreImpl::retire() {
  FLEXUS_PROFILE();
  bool stop_retire = false;

  if (theROB.empty()) {
    CORE_DBG("ROB is empty! ->" << theROB.size());
    return;
  }

  theRetireCount = 0;
  while (!theROB.empty() && !stop_retire) {
    if (!theROB.front()->mayRetire()) {
      CORE_DBG("Cant Retire due to pending retirement dependance " << *theROB.front());
      break;
    }

    if (theTSOBReplayStalls > 0) {
      break;
    }

    // Check for sufficient speculative checkpoints
    DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
    if (theIsSpeculating && theAllowedSpeculativeCheckpoints > 0 &&
        theROB.front()->instClass() == clsAtomic && !theROB.front()->isAnnulled() &&
        (static_cast<int>(theCheckpoints.size())) >= theAllowedSpeculativeCheckpoints) {
      DBG_(VVerb, (<< " theROB.front()->instClass() == clsAtomic "));
      break;
    }

    if ((thePendingInterrupt != kException_None) && theIsSpeculating) {
      // stop retiring so we can stop speculating and handle the interrupt
      DBG_(VVerb, (<< " thePendingInterrupt && theIsSpeculating "));
      break;
    }

    if ((theROB.front()->resync() || (theROB.front() == theInterruptInstruction)) &&
        theIsSpeculating) {
      // Do not retire sync instructions or interrupts while speculating
      break;
    }

    // Stop retirement for the cycle if we retire an instruction that
    // requires resynchronization
    if (theROB.front()->resync()) {
      stop_retire = true;
    }

    if (theSquashRequested && (theROB.begin() == theSquashInstruction)) {
      if (!theSquashInclusive) {
        ++theSquashInstruction;
        theSquashInclusive = true;
      } else {
        break;
      }
    }

    // FOR in-order SMS Experiments only - not normal in-order
    // Under theInOrderMemory, we do not allow stores or atomics to retire
    // unless the OoO core is fully idle.  This is not neccessary for loads -
    // they will only issue when they reach the MemQueue head and the core is
    // fully stalled
    // if ( theInOrderMemory && (theROB.front()->instClass() == clsStore ||
    // theROB.front()->instClass() == clsAtomic ) && ( !isFullyStalled() ||
    // theRetireCount > 0)) { break;
    //}

    theSpinRetireSinceReset++;

    CORE_DBG(theName << " Retire:" << *theROB.front());
    if (!acceptInterrupt()) {
      theROB.front()->checkTraps(); // take traps only if we don't take interrupt
    }
    if (thePendingTrap != kException_None) {
      theROB.front()->changeInstCode(codeException);
      DBG_(Verb, (<< theName << " Trap raised by " << *theROB.front()));
      stop_retire = true;
    } else {
      theROB.front()->doRetirementEffects();
      if (thePendingTrap != kException_None) {
        theROB.front()->changeInstCode(codeException);
        DBG_(Verb, (<< theName << " Trap raised by " << *theROB.front()));
        stop_retire = true;
      }
    }

    accountRetire(theROB.front());

    ++theRetireCount;
    if (theRetireCount >= theRetireWidth) {
      stop_retire = true;
    }

    // Value prediction enabled after forward progress is made
    theValuePredictInhibit = false;

    thePC = theROB.front()->pc() + 4;

    CORE_DBG("Move instruction to the secondary retirement buffer " << *(theROB.front()));
    theSRB.push_back(theROB.front());

    if (thePendingTrap == kException_None) {
      theROB.pop_front();
      // Need to squash and retire instructions that cause traps
    }
  }
}

void CoreImpl::commit() {
  FLEXUS_PROFILE();

  if (theAbortSpeculation) {
    doAbortSpeculation();
  }

  while (!theSRB.empty() && (theSRB.front()->mayCommit() || theSRB.front()->isSquashed())) {

    if (theSRB.front()->hasCheckpoint()) {
      freeCheckpoint(theSRB.front());
    }

    DBG_(VVerb, (<< theName << " FinalCommit:" << *theSRB.front()));
    if (theSRB.front()->willRaise() == kException_None) {
      theSRB.front()->doCommitEffects();
      DBG_(VVerb, (<< theName << " commit effects complete"));
    }

    commit(theSRB.front());
    DBG_(VVerb, (<< theName << " committed in Qemu"));

    theSRB.pop_front();
  }
}

void CoreImpl::commit(boost::intrusive_ptr<Instruction> anInstruction) {
  FLEXUS_PROFILE();
  CORE_DBG(*anInstruction);
  bool validation_passed = true;
  int raised = 0;
  bool resync_accounted = false;

  if (anInstruction->advancesSimics()) {
    CORE_DBG("Instruction is neither annuled nor is a micro-op");

    validation_passed &= anInstruction->preValidate();
    DBG_(Iface, (<< "Pre Validating... " << validation_passed));

    theInterruptSignalled = false;
    theInterruptInstruction = 0;

    int qemu_rcode = advance_fn(true);  // count time
    if (qemu_rcode == QEMU_HALT_CODE) { // QEMU CPU Halted
      /* If cpu is halted, turn off insn counting until the CPU is woken up again */
      cpuHalted = true;
      DBG_(Dev, (<< "Core " << theNode << " entering halt state, after executing instruction "
                 << *anInstruction));
      anInstruction->forceResync();
    }

    if (raised != 0) {
      if (anInstruction->willRaise() !=
          (raised == 0 ? kException_None
                       : kException_)) { // FIXME get exception mapper HEHE
        DBG_(VVerb,
             (<< *anInstruction
              << " Core did not predict correct exception for this "
                 "instruction raised=0x"
              << std::hex << raised << " will_raise=0x" << anInstruction->willRaise() << std::dec));
        if (anInstruction->instCode() != codeITLBMiss) {
          anInstruction->changeInstCode(codeExceptionUnsupported);
        }
        anInstruction->forceResync();
        resync_accounted = true;
        if (raised < 0x400) {
          ++theResync_UnexpectedException;
        } else {
          ++theResync_Interrupt;
        }
      } else {
        DBG_(VVerb, (<< *anInstruction << " Core correctly identified raise=0x" << std::hex
                     << raised << std::dec));
      }
      anInstruction->raise(raised == 0 ? kException_None : kException_); // HEHE
    } else if (anInstruction->willRaise() != kException_None) {
      DBG_(VVerb, (<< *anInstruction << " DANGER:  Core predicted exception: " << std::hex
                   << anInstruction->willRaise() << " but simics says no exception"));
    }
  }

  accountCommit(anInstruction, raised);

  theDumpPC = anInstruction->pcNext();
  if (anInstruction->resync()) {
    DBG_(Dev, Cond(!cpuHalted)(<< "Forced Resync:" << *anInstruction));

    // Subsequent Empty ROB stalls (until next dispatch) are the result of a
    // synchronizing instruction.
    theEmptyROBCause = kSync;
    if (!resync_accounted) {
      accountResyncReason(anInstruction);
    }
    throw ResynchronizeWithQemuException(true);
  }

  // validation_passed &= checkValidatation();

  validation_passed &= anInstruction->postValidate();
  DBG_(Iface, (<< "Post Validating... " << validation_passed));

  if (!validation_passed) {
    DBG_(Dev, (<< "Failed Validated " << std::internal << *anInstruction << std::left));

    // Subsequent Empty ROB stalls (until next dispatch) are the result of a
    // modelling error resynchronization instruction.
    theEmptyROBCause = kResync;
    ++theResync_FailedValidation;

    throw ResynchronizeWithQemuException();
  }
  DBG_(Iface, (<< "uARCH Validated "));
  DBG_(VVerb, (<< std::internal << *anInstruction << std::left));
}

bool CoreImpl::acceptInterrupt() {
  if ((thePendingInterrupt != kException_None)) {
    DBG_(Dev, (<< "IRQ is pending..."));
  }
  if ((thePendingInterrupt != kException_None) // Interrupt is pending
      && !theInterruptSignalled                // Already accepted the interrupt
      && !theROB.empty()                       // Need an instruction to take the interrupt on
      && !theROB.front()->isAnnulled()         // Do not take interrupts on annulled instructions
      && !theROB.front()->isSquashed()         // Do not take interrupts on squashed instructions
      && !theROB.front()->isMicroOp()          // Do not take interrupts on squashed micro-ops
      && !theIsSpeculating                     // Do not take interrupts while speculating
  ) {
    // Interrupt was signalled this cycle.  Clear the ROB
    theInterruptSignalled = false;

    // theROB.front()->makePriv();

    DBG_(Dev, (<< theName << " Accepting interrupt " << thePendingInterrupt << " on instruction "
               << *theROB.front()));
    theInterruptInstruction = theROB.front();
    takeTrap(theInterruptInstruction,
             /*thePendingInterrupt*/ kException_); // HEHE

    return true;
  }
  return false;
}

void CoreImpl::accountRetire(boost::intrusive_ptr<Instruction> anInst) {
  FLEXUS_PROFILE();
  DBG_Assert(anInst);
  DBG_(VVerb, (<< " accountRetire: " << *anInst));

  if (theIsSpeculating) {
    ++theRetiresSinceCheckpoint;
  }

  // Determine cycle category (always based on last retire in cycle)
  bool system = (currentEL() == 1); /*theROB.front()->isPriv();*/

  theIsIdle = ! Flexus::Qemu::Processor::getProcessor(theNode).is_busy();

  if (theIsIdle) {
    theCycleCategory = kTBIdle;
  } else if (theROB.front()->isTrap()) {
    theCycleCategory = kTBTrap;
  } else if (system) {
    theCycleCategory = kTBSystem;
  } else {
    theCycleCategory = kTBUser;
  }

  if (isIdleLoop()) {
    DBG_Assert(system, (<< theName << " isIdleLoop in user code? " << *theROB.front()));
    theLastStallCause = nXactTimeBreakdown::kIdle_Stall;
  } else {
    if (anInst->willRaise() != kException_None) {
      switch (anInst->instClass()) {
      case clsLoad:
        theLastStallCause = nXactTimeBreakdown::kWillRaise_Load;
        break;
      case clsStore:
        theLastStallCause = nXactTimeBreakdown::kWillRaise_Store;
        break;
      case clsAtomic:
        theLastStallCause = nXactTimeBreakdown::kWillRaise_Atomic;
        break;
      case clsBranch:
        DBG_(VVerb, (<< "Get a Branch................."));
        theLastStallCause = nXactTimeBreakdown::kWillRaise_Branch;
        break;
      case clsMEMBAR:
        theLastStallCause = nXactTimeBreakdown::kWillRaise_MEMBAR;
        break;
      case clsComputation:
        theLastStallCause = nXactTimeBreakdown::kWillRaise_Computation;
        break;
      case clsSynchronizing:
        theLastStallCause = nXactTimeBreakdown::kWillRaise_Synchronizing;
        break;
      default:
        DBG_(Crit, (<< "Unaccounted willRaise stall: " << *anInst));
      }
    } else {

      boost::intrusive_ptr<TransactionTracker> tracker = anInst->getTransactionTracker();

      switch (anInst->instClass()) {
      case clsLoad: {
        if (!tracker) {
          theLastStallCause = nXactTimeBreakdown::kLoad_Forwarded;
        } else {
          if (tracker->fillLevel()) {
            switch (*tracker->fillLevel()) {
            case eL1:
              theLastStallCause = nXactTimeBreakdown::kLoad_L1;
              break;
            case eL2:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eDataPrivate:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L2_Data_Private;
                  break;
                case eDataSharedRO:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L2_Data_Shared_RO;
                  break;
                case eDataSharedRW:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L2_Data_Shared_RW;
                  break;
                case eCoherence:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L2_Coherence;
                  break;
                default:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L2;
                  break;
                }
              } else
                theLastStallCause = nXactTimeBreakdown::kLoad_L2;
              break;
            case eL3:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eDataPrivate:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L3_Data_Private;
                  break;
                case eDataSharedRO:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L3_Data_Shared_RO;
                  break;
                case eDataSharedRW:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L3_Data_Shared_RW;
                  break;
                case eCoherence:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L3_Coherence;
                  break;
                default:
                  theLastStallCause = nXactTimeBreakdown::kLoad_L3;
                  break;
                }
              } else
                theLastStallCause = nXactTimeBreakdown::kLoad_L2;
              break;
            case ePrefetchBuffer:
              theLastStallCause = nXactTimeBreakdown::kLoad_PB;
              break;
            case eLocalMem:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eCold:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Local_Cold;
                  break;
                case eReplacement:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Local_Replacement;
                  break;
                case eDGP:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Local_DGP;
                  break;
                case eCoherence:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Local_Coherence;
                  break;
                default:
                  DBG_(Crit, (<< "Unaccounted load local stall cycles. Invalid fill "
                                 "type: "
                              << *tracker->fillType() << " for load " << *anInst));
                  break;
                }
              } else {
                DBG_(Crit, (<< "Unaccounted load local stall cycles. Missing "
                               "fill type for load "
                            << *anInst));
              }
              break;
            case eRemoteMem:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eCold:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Remote_Cold;
                  break;
                case eReplacement:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Remote_Replacement;
                  break;
                case eDGP:
                  theLastStallCause = nXactTimeBreakdown::kLoad_Remote_DGP;
                  break;
                case eCoherence:
                  if (tracker->previousState()) {
                    switch (*tracker->previousState()) {
                    case eShared:
                      theLastStallCause = nXactTimeBreakdown::kLoad_Remote_Coherence_Shared;
                      break;
                    case eModified:
                      theLastStallCause = nXactTimeBreakdown::kLoad_Remote_Coherence_Modified;
                      break;
                    case eInvalid:
                      theLastStallCause = nXactTimeBreakdown::kLoad_Remote_Coherence_Invalid;
                      break;
                    default:
                      DBG_(Crit, (<< "Unaccounted load remote coherence stall "
                                     "cycles. Invalid previous state: "
                                  << *tracker->previousState() << " for load " << *anInst));
                      break;
                    }
                  } else {
                    DBG_(Crit, (<< "Unaccounted load remote coherence stall "
                                   "cycles. Missing previous state for load "
                                << *anInst));
                  }
                  break;
                default:
                  DBG_(Crit, (<< "Unaccounted load remote stall cycles. Invalid fill "
                                 "type: "
                              << *tracker->fillType() << " for load " << *anInst));
                  break;
                }
              } else {
                DBG_(Crit, (<< "Unaccounted load remote stall cycles. Missing "
                               "fill type for load "
                            << *anInst));
              }
              break;
            case ePeerL1Cache:
              if (tracker->previousState() && *tracker->previousState() == eShared)
                theLastStallCause = nXactTimeBreakdown::kLoad_PeerL1Cache_Coherence_Shared;
              else
                theLastStallCause = nXactTimeBreakdown::kLoad_PeerL1Cache_Coherence_Modified;
              break;
            case ePeerL2:
              if (tracker->previousState() && *tracker->previousState() == eShared)
                theLastStallCause = nXactTimeBreakdown::kLoad_PeerL2Cache_Coherence_Shared;
              else
                theLastStallCause = nXactTimeBreakdown::kLoad_PeerL2Cache_Coherence_Modified;
              break;
            default:
              DBG_(Crit, (<< "Unaccounted load stall cycles. Invalid load level: "
                          << *tracker->fillLevel() << " for load " << *anInst));
              break;
            }
          } else {
            DBG_(Crit, (<< "Unaccounted load stall cycles. Missing Fill level "
                           "for retiring load: "
                        << *anInst));
          }
        }
        break;
      } // clsLoad
      case clsStore:
        theLastStallCause = getStoreStallType(tracker);
        break;
      case clsAtomic: {
        if (!tracker) {
          theLastStallCause = nXactTimeBreakdown::kAtomic_Forwarded;
        } else {
          if (tracker->fillLevel()) {
            switch (*tracker->fillLevel()) {
            case eL1:
              theLastStallCause = nXactTimeBreakdown::kAtomic_L1;
              break;
            case eL2:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eDataPrivate:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L2_Data_Private;
                  break;
                case eDataSharedRO:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L2_Data_Shared_RO;
                  break;
                case eDataSharedRW:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L2_Data_Shared_RW;
                  break;
                case eCoherence:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L2_Coherence;
                  break;
                default:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L2;
                  break;
                }
              } else
                theLastStallCause = nXactTimeBreakdown::kAtomic_L2;
              break;
            case eL3:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eDataPrivate:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L3_Data_Private;
                  break;
                case eDataSharedRO:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L3_Data_Shared_RO;
                  break;
                case eDataSharedRW:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L3_Data_Shared_RW;
                  break;
                case eCoherence:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L3_Coherence;
                  break;
                default:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_L3;
                  break;
                }
              } else
                theLastStallCause = nXactTimeBreakdown::kAtomic_L3;
              break;
            case eDirectory:
              theLastStallCause = nXactTimeBreakdown::kAtomic_Directory;
              break;
            case ePrefetchBuffer:
              theLastStallCause = nXactTimeBreakdown::kAtomic_PB;
              break;
            case eLocalMem:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eCold:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Local_Cold;
                  break;
                case eReplacement:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Local_Replacement;
                  break;
                case eDGP:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Local_DGP;
                  break;
                case eCoherence:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Local_Coherence;
                  break;
                default:
                  DBG_(Crit, (<< "Unaccounted atomic local stall cycles. Invalid "
                                 "fill type: "
                              << *tracker->fillType() << " for atomic " << *anInst));
                  break;
                }
              } else {
                DBG_(Crit, (<< "Unaccounted atomic local stall cycles. Missing "
                               "fill type for atomic "
                            << *anInst));
              }
              break;
            case eRemoteMem:
              if (tracker->fillType()) {
                switch (*tracker->fillType()) {
                case eCold:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Remote_Cold;
                  break;
                case eReplacement:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Remote_Replacement;
                  break;
                case eDGP:
                  theLastStallCause = nXactTimeBreakdown::kAtomic_Remote_DGP;
                  break;
                case eCoherence:
                  if (tracker->previousState()) {
                    switch (*tracker->previousState()) {
                    case eShared:
                      theLastStallCause = nXactTimeBreakdown::kAtomic_Remote_Coherence_Shared;
                      break;
                    case eModified:
                      theLastStallCause = nXactTimeBreakdown::kAtomic_Remote_Coherence_Modified;
                      break;
                    case eInvalid:
                      theLastStallCause = nXactTimeBreakdown::kAtomic_Remote_Coherence_Invalid;
                      break;
                    default:
                      DBG_(Crit, (<< "Unaccounted load remote coherence stall "
                                     "cycles. Invalid previous state: "
                                  << *tracker->previousState() << " for load " << *anInst));
                      break;
                    }
                  } else {
                    DBG_(Crit, (<< "Unaccounted load remote coherence stall "
                                   "cycles. Missing previous state for load "
                                << *anInst));
                  }
                  break;
                default:
                  DBG_(Crit, (<< "Unaccounted load remote stall cycles. Invalid fill "
                                 "type: "
                              << *tracker->fillType() << " for load " << *anInst));
                  break;
                }
              } else {
                DBG_(Crit, (<< "Unaccounted load remote stall cycles. Missing "
                               "fill type for load "
                            << *anInst));
              }
              break;
            case ePeerL1Cache:
              if (tracker->previousState() && *tracker->previousState() == eShared)
                theLastStallCause = nXactTimeBreakdown::kAtomic_PeerL1Cache_Coherence_Shared;
              else
                theLastStallCause = nXactTimeBreakdown::kAtomic_PeerL1Cache_Coherence_Modified;
              break;
            default:
              DBG_(Crit, (<< "Unaccounted atomic stall cycles. Invalid load level: "
                          << *tracker->fillLevel() << " for atomic " << *anInst));
              break;
            }
          } else {
            DBG_(Crit, (<< "Unaccounted atomic stall cycles. Missing Fill "
                           "level for retiring atomic: "
                        << *anInst));
          }
        }
        break;
      } // clsAtomic
      case clsBranch:
        //          DBG_(VVerb, (<<"Get a Branch........."));
        theLastStallCause = nXactTimeBreakdown::kBranch;
        break;
      case clsMEMBAR:
        theLastStallCause = nXactTimeBreakdown::kMEMBAR;
        break;
      case clsComputation:
        theLastStallCause = nXactTimeBreakdown::kDataflow;
        break;
      case clsSynchronizing:
        theLastStallCause = nXactTimeBreakdown::kSyncPipe;
        break;
      default:
        DBG_Assert(false, (<< "Invalid instruction class: " << anInst->instClass()));
      }
    }
  }

  int32_t retire_stall_cycles = theTimeBreakdown.retire(
      theLastStallCause, theROB.front()->sequenceNo(), theCycleCategory, theSpinning);
  anInst->setRetireStallCycles(retire_stall_cycles);
}

nXactTimeBreakdown::eCycleClass
CoreImpl::getStoreStallType(boost::intrusive_ptr<TransactionTracker> tracker) {
  nXactTimeBreakdown::eCycleClass theStallType = nXactTimeBreakdown::kStore_Unknown;
  if (tracker && tracker->fillLevel()) {
    switch (*tracker->fillLevel()) {
    case eL1:
      theStallType = nXactTimeBreakdown::kStore_L1;
      break;
    case eL2:
      if (tracker->fillType()) {
        switch (*tracker->fillType()) {
        case eDataPrivate:
          theStallType = nXactTimeBreakdown::kStore_L2_Data_Private;
          break;
        case eDataSharedRO:
          theStallType = nXactTimeBreakdown::kStore_L2_Data_Shared_RO;
          break;
        case eDataSharedRW:
          theStallType = nXactTimeBreakdown::kStore_L2_Data_Shared_RW;
          break;
        case eCoherence:
          theStallType = nXactTimeBreakdown::kStore_L2_Coherence;
          break;
        default:
          theStallType = nXactTimeBreakdown::kStore_L2;
          break;
        }
      } else
        theStallType = nXactTimeBreakdown::kStore_L2;
      break;
    case eL3:
      if (tracker->fillType()) {
        switch (*tracker->fillType()) {
        case eDataPrivate:
          theStallType = nXactTimeBreakdown::kStore_L3_Data_Private;
          break;
        case eDataSharedRO:
          theStallType = nXactTimeBreakdown::kStore_L3_Data_Shared_RO;
          break;
        case eDataSharedRW:
          theStallType = nXactTimeBreakdown::kStore_L3_Data_Shared_RW;
          break;
        case eCoherence:
          theStallType = nXactTimeBreakdown::kStore_L3_Coherence;
          break;
        default:
          theStallType = nXactTimeBreakdown::kStore_L3;
          break;
        }
      } else
        theStallType = nXactTimeBreakdown::kStore_L3;
      break;
    case ePrefetchBuffer:
      theStallType = nXactTimeBreakdown::kStore_PB;
      break;
    case eLocalMem:
      if (tracker->fillType()) {
        switch (*tracker->fillType()) {
        case eCold:
          theStallType = nXactTimeBreakdown::kStore_Local_Cold;
          break;
        case eReplacement:
          theStallType = nXactTimeBreakdown::kStore_Local_Replacement;
          break;
        case eDGP:
          theStallType = nXactTimeBreakdown::kStore_Local_DGP;
          break;
        case eCoherence:
          theStallType = nXactTimeBreakdown::kStore_Local_Coherence;
          break;
        default:
          break;
        }
      }
      break;
    case eRemoteMem:
      if (tracker->fillType()) {
        switch (*tracker->fillType()) {
        case eCold:
          theStallType = nXactTimeBreakdown::kStore_Remote_Cold;
          break;
        case eReplacement:
          theStallType = nXactTimeBreakdown::kStore_Remote_Replacement;
          break;
        case eDGP:
          theStallType = nXactTimeBreakdown::kStore_Remote_DGP;
          break;
        case eCoherence:
          if (tracker->previousState()) {
            switch (*tracker->previousState()) {
            case eShared:
              theStallType = nXactTimeBreakdown::kStore_Remote_Coherence_Shared;
              break;
            case eModified:
              theStallType = nXactTimeBreakdown::kStore_Remote_Coherence_Modified;
              break;
            case eInvalid:
              theStallType = nXactTimeBreakdown::kStore_Remote_Coherence_Invalid;
              break;
            default:
              break;
            }
          }
          break;
        default:
          break;
        }
      }
      break;
    case ePeerL1Cache:
      if (tracker->previousState() && *tracker->previousState() == eShared)
        theStallType = nXactTimeBreakdown::kStore_PeerL1Cache_Coherence_Shared;
      else
        theStallType = nXactTimeBreakdown::kStore_PeerL1Cache_Coherence_Modified;
      break;
    case ePeerL2:
      if (tracker->previousState() && *tracker->previousState() == eShared)
        theStallType = nXactTimeBreakdown::kStore_PeerL2Cache_Coherence_Shared;
      else
        theStallType = nXactTimeBreakdown::kStore_PeerL2Cache_Coherence_Modified;
      break;
    default:
      break;
    }
  }
  return theStallType;
}

void CoreImpl::chargeStoreStall(boost::intrusive_ptr<Instruction> inst,
                                boost::intrusive_ptr<TransactionTracker> tracker) {
  nXactTimeBreakdown::eCycleClass theStallType = getStoreStallType(tracker);
  int32_t stall_cycles = theTimeBreakdown.stall(theStallType);
  inst->setRetireStallCycles(stall_cycles);
}

void CoreImpl::completeAccounting() {
  FLEXUS_PROFILE();

  // count occupancy in ROB
  theROBOccupancyTotal << std::make_pair(theROB.size(), 1);

  theLSQOccupancy << std::make_pair(theLSQCount, 1);
  theSBOccupancy << std::make_pair(theSBCount, 1);
  theSBNAWOccupancy << std::make_pair(theSBNAWCount, 1);
  theSBUniqueOccupancy << std::make_pair(theSBLines_Permission.size(), 1);

  // count checkpoint occupancy
  theSpeculativeCheckpoints << std::make_pair(theCheckpoints.size(), 1);

  if (theSpinning) {
    // occupancy during spinning
    theROBOccupancySpin << std::make_pair(theROB.size(), 1);
  } else {
    // occupancy while not spinning
    theROBOccupancyNonSpin << std::make_pair(theROB.size(), 1);

    if (theRetireCount == 0) {
      // Account for non-accumulating stall causes
      if (theROB.empty()) {
        // Stall due to empty ROB
        switch (theEmptyROBCause) {

          // deal with IStalls
        case kIStall_PeerL1:
        case kIStall_L2:
        case kIStall_PeerL2:
        case kIStall_L3:
        case kIStall_Mem:
        case kIStall_Other:
          theTimeBreakdown.accumulateCyclesTmp(); // put IStalls into a
                                                  // temporary cycle accumulator
          break;

        case kMispredict:
          theTimeBreakdown.accumulateCyclesTmp(); // put mispredict stalls into a temporary
                                                  // cycle accumulator
          break;

        case kSync:
          theTimeBreakdown.stall(nXactTimeBreakdown::kEmptyROB_Sync);
          break;
        case kResync:
          theTimeBreakdown.stall(nXactTimeBreakdown::kEmptyROB_Resync);
          break;
        case kFailedSpeculation:
          theTimeBreakdown.stall(nXactTimeBreakdown::kEmptyROB_FailedSpeculation);
          break;
        case kRaisedException:
          theTimeBreakdown.stall(nXactTimeBreakdown::kEmptyROB_Exception);
          break;
        case kRaisedInterrupt:
          theTimeBreakdown.stall(nXactTimeBreakdown::kEmptyROB_Interrupt);
          break;
        }
      } else if (theIsSpeculating && theTSOBReplayStalls > 0) {
        theTimeBreakdown.stall(nXactTimeBreakdown::kTSOBReplay);

      } else if (theIsSpeculating && theROB.front()->resync()) {
        // HERE
        theTimeBreakdown.stall(nXactTimeBreakdown::kSyncWhileSpeculating);
      } else if (theROB.front()->willRaise()) {
        // Stall because of exception at head of ROB
        switch (theROB.front()->instClass()) {
        case clsLoad:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_Load);
          break;
        case clsStore:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_Store);
          break;
        case clsAtomic:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_Atomic);
          break;
        case clsBranch:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_Branch);
          break;
        case clsMEMBAR:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_MEMBAR);
          break;
        case clsComputation:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_Computation);
          break;
        case clsSynchronizing:
          theTimeBreakdown.stall(nXactTimeBreakdown::kWillRaise_Synchronizing);
          break;
        default:
          DBG_(Crit, (<< "Unaccounted willRaise stall: " << *theROB.front()));
        }

      } else if (theROB.front()->instClass() == clsStore && sbFull()) {
        // Stall while waiting to commit a store to the SB
        theTimeBreakdown.accumulateStoreCyclesTmp(); // put mispredict stalls into a
                                                     // temporary cycle accumulator

      } else if (theROB.front()->instClass() == clsAtomic) {
        if (!theSpeculativeOrder && !sbEmpty()) {
          theTimeBreakdown.stall(nXactTimeBreakdown::kAtomic_SBDrain);
        } else {
          memq_t::index<by_insn>::type::iterator iter =
              theMemQueue.get<by_insn>().find(theROB.front());
          if (iter != theMemQueue.get<by_insn>().end() &&
              (iter->thePartialSnoop || iter->theSideEffect)) {
            // Stall while waiting for ROB to drain for an atomic op
            theTimeBreakdown.stall(nXactTimeBreakdown::kAtomic_SBDrain);
          }
        }
      } else {
        // Check for Stall on a Non-cacheable access
        memq_t::index<by_insn>::type::iterator iter =
            theMemQueue.get<by_insn>().find(theROB.front());
        if (iter != theMemQueue.get<by_insn>().end()) {
          if (iter->theSideEffect) {
            if (theIsSpeculating) {
              theTimeBreakdown.stall(nXactTimeBreakdown::kSideEffect_Speculating);
            } else {
              if (iter->isAtomic()) {
                theTimeBreakdown.stall(nXactTimeBreakdown::kSideEffect_Atomic);
              } else if (iter->isLoad()) {
                theTimeBreakdown.stall(nXactTimeBreakdown::kSideEffect_Load);
              } else {
                theTimeBreakdown.stall(nXactTimeBreakdown::kSideEffect_Store);
              }
            }
          }
        }
      }

    } else {
      // Reset stall cause to unknown
      theLastStallCause = nXactTimeBreakdown::kUnknown;
    }
  }

  if (theRetireCount >= 6) {
    ++theSustainedIPCCycles;
  } else {
    theSustainedIPCCycles = 0;
  }
}

void CoreImpl::accountAbortSpeculation(uint64_t aStopSequenceNumber) {
  theTimeBreakdown.modifyAndApplyTransactionsBackwards(
      aStopSequenceNumber, nXactTimeBreakdown::kFailedSpeculation,
      nXactTimeBreakdown::eDiscard_FailedSpeculation);
  if (!theIsSpeculating) {
    ++theSpeculations_Rollback;
  } else {
    ++theSpeculations_PartialRollback;
  }
}

void CoreImpl::accountStartSpeculation() {
  ++theSpeculations_Started;
}

void CoreImpl::accountEndSpeculation() {
  ++theSpeculations_Successful;
}

void CoreImpl::accountResyncSpeculation() {
  theTimeBreakdown.modifyAndApplyAllTransactions(nXactTimeBreakdown::kSyncWhileSpeculating,
                                                 nXactTimeBreakdown::eDiscard_Resync);
  ++theSpeculations_Resync;
}

void CoreImpl::accountResyncReason(boost::intrusive_ptr<Instruction> anInstruction) {
  if (cpuHalted) {
    ++theResync_CPUHaltedState;
  }
  switch (anInstruction->instCode()) {
  case codeBlackBox:
    ++theResync_BlackBox;
    break;
  case codeMAGIC:
    ++theResync_MAGIC;
    break;
  case codeITLBMiss:
    ++theResync_ITLBMiss;
    break;
  case codeRETURN:
    if (anInstruction->isAnnulled()) {
      ++theResync_UnimplementedAnnul;
    } else {
      ++theResync_Unknown;
    }
    break;
  case codeRDPRUnsupported:
    ++theResync_RDPRUnsupported;
    break;
  case codeWRPRUnsupported:
    ++theResync_WRPRUnsupported;
    break;
  case codeMEMBARSync:
    ++theResync_MEMBARSync;
    break;
  case codeDeviceAccess:
    ++theResync_DeviceAccess;
    break;
  case codeSideEffectLoad:
    ++theResync_SideEffectLoad;
    break;
  case codeSideEffectStore:
    ++theResync_SideEffectStore;
    break;
  default:
    DBG_(Dev,
         Cond(!cpuHalted)(<< "Unknown resync for instruction code: " << anInstruction->instCode()));
    ++theResync_Unknown;
    break;
  }
}

void CoreImpl::accountResync() {
  theTimeBreakdown.modifyAndApplyAllTransactions(nXactTimeBreakdown::kSyncPipe,
                                                 nXactTimeBreakdown::eDiscard_Resync);
}

void CoreImpl::prepareMemOpAccounting() {
  theMemOpCounters[false][false][clsLoad] =
      new MemOpCounter(theName + "-MemCommit:OnChip:NonCoh:Read");
  theMemOpCounters[false][true][clsLoad] = new MemOpCounter(theName + "-MemCommit:OnChip:Coh:Read");
  theMemOpCounters[true][false][clsLoad] =
      new MemOpCounter(theName + "-MemCommit:OffChip:NonCoh:Read");
  theMemOpCounters[true][true][clsLoad] = new MemOpCounter(theName + "-MemCommit:OffChip:Coh:Read");
  theMemOpCounters[false][false][clsStore] =
      new MemOpCounter(theName + "-MemCommit:OnChip:NonCoh:Write");
  theMemOpCounters[false][true][clsStore] =
      new MemOpCounter(theName + "-MemCommit:OnChip:Coh:Write");
  theMemOpCounters[true][false][clsStore] =
      new MemOpCounter(theName + "-MemCommit:OffChip:NonCoh:Write");
  theMemOpCounters[true][true][clsStore] =
      new MemOpCounter(theName + "-MemCommit:OffChip:Coh:Write");
  theMemOpCounters[false][false][clsAtomic] =
      new MemOpCounter(theName + "-MemCommit:OnChip:NonCoh:Atomic");
  theMemOpCounters[false][true][clsAtomic] =
      new MemOpCounter(theName + "-MemCommit:OnChip:Coh:Atomic");
  theMemOpCounters[true][false][clsAtomic] =
      new MemOpCounter(theName + "-MemCommit:OffChip:NonCoh:Atomic");
  theMemOpCounters[true][true][clsAtomic] =
      new MemOpCounter(theName + "-MemCommit:OffChip:Coh:Atomic");

  theEpochEnd[true][clsLoad] = new Stat::StatCounter(theName + "-MLPEpoch:End:Coh:Load");
  theEpochEnd[true][clsStore] = new Stat::StatCounter(theName + "-MLPEpoch:End:Coh:Store");
  theEpochEnd[true][clsAtomic] = new Stat::StatCounter(theName + "-MLPEpoch:End:Coh:Atomic");
  theEpochEnd[false][clsLoad] = new Stat::StatCounter(theName + "-MLPEpoch:End:NonCoh:Load");
  theEpochEnd[false][clsStore] = new Stat::StatCounter(theName + "-MLPEpoch:End:NonCoh:Store");
  theEpochEnd[false][clsAtomic] = new Stat::StatCounter(theName + "-MLPEpoch:End:NonCoh:Atomic");
}

void CoreImpl::accountCommit(boost::intrusive_ptr<Instruction> anInstruction, bool aRaised) {
  int32_t level = 0 /* user */;
  if (theIsIdle == true)
    level = 3 /* idle */;
  else if (anInstruction->isTrap())
    level = 2 /* trap */;
  else if (anInstruction->isPriv())
    level = 1 /* system */;
  int32_t spin = (theSpinning) ? 4 : 0;

  // DBG_(VVerb, (<<"In accountCommit, aRaised: "<<aRaised));
  theTimeBreakdown.applyTransactions(anInstruction->sequenceNo());

  ++theCommitNumber;
  ++theCommitCount;
  (*theCommitUSArray[spin | level])++;

  eInstructionCode code = anInstruction->instCode();
  eInstructionClass klass = anInstruction->instClass();

  // Account for the type of instruction committed
  ++theMix_Total;
  if (aRaised) {
    ++theMix_Exception;
  } else {
    switch (klass) {
    case clsLoad:
      DBG_(VVerb, (<< "Get a LOAD.........."));
      accountCommitMemOp(anInstruction);
      ++theMix_Load;
      break;
    case clsStore:
      DBG_(VVerb, (<< "Get a Store.........."));
      accountCommitMemOp(anInstruction);
      ++theMix_Store;
      break;
    case clsAtomic:
      DBG_(VVerb, (<< "Get an Atomic.........."));
      accountCommitMemOp(anInstruction);
      ++theMix_Atomic;
      break;
    case clsBranch:
      DBG_(VVerb, (<< "Get a BRANCH.........."));
      ++theMix_Branch;
      break;
    case clsMEMBAR:
      DBG_(VVerb, (<< "Get a Membar.........."));
      ++theMix_MEMBAR;
      break;
    case clsComputation:
      DBG_(VVerb, (<< "Get a Computation.........."));
      ++theMix_Computation;
      break;
    case clsSynchronizing:
      DBG_(VVerb, (<< "Get a Synchronization.........."));
      ++theMix_Synchronizing;
      break;
    }
  }

  // Count the instruction
  ++(*theCommitsByCode[level][code]);
}

void CoreImpl::accountCommitMemOp(boost::intrusive_ptr<Instruction> inst) {
  eInstructionClass klass = inst->instClass();
  DBG_Assert((klass == clsLoad || klass == clsStore || klass == clsAtomic));
  boost::intrusive_ptr<TransactionTracker> tracker = inst->getTransactionTracker();
  boost::intrusive_ptr<TransactionTracker> prefetch = inst->getPrefetchTracker();

  bool is_offchip = false;
  if (prefetch && prefetch->fillLevel()) {
    switch (*prefetch->fillLevel()) {
    case eLocalMem:
    case eRemoteMem:
      is_offchip = true;
      break;
    default:
      break;
    }
  } else if (tracker && tracker->fillLevel()) {
    switch (*tracker->fillLevel()) {
    case eLocalMem:
    case eRemoteMem:
      is_offchip = true;
      break;
    default:
      break;
    }
  }

  bool is_coherence = false;
  if (prefetch && prefetch->fillType()) {
    switch (*prefetch->fillType()) {
    case eCoherence:
      is_coherence = true;
      break;
    default:
      break;
    }
  } else if (tracker && tracker->fillType()) {
    switch (*tracker->fillType()) {
    case eCoherence:
      is_coherence = true;
      break;
    default:
      break;
    }
  }

  theMemOpCounters[is_offchip][is_coherence][klass]->theCount++;
  theMemOpCounters[is_offchip][is_coherence][klass]->theRetireStalls += inst->retireStallCycles();
  theMemOpCounters[is_offchip][is_coherence][klass]->theRetireStalls_Histogram
      << std::make_pair(inst->retireStallCycles(), 1);
  if (prefetch && !prefetch->wasCounted()) {
    DBG_Assert(prefetch->completionCycle());
    prefetch->setWasCounted();
    theMemOpCounters[is_offchip][is_coherence][klass]->thePrefetchCount++;
    uint64_t prefetch_latency = *prefetch->completionCycle() - prefetch->startCycle();
    theMemOpCounters[is_offchip][is_coherence][klass]->thePrefetchLatency += prefetch_latency;
    theMemOpCounters[is_offchip][is_coherence][klass]->thePrefetchLatency_Histogram
        << std::make_pair(prefetch_latency, 1);
  }
  if (tracker && !tracker->wasCounted()) {
    DBG_Assert(tracker->completionCycle());
    tracker->setWasCounted();
    theMemOpCounters[is_offchip][is_coherence][klass]->theRequestCount++;
    uint64_t latency = *tracker->completionCycle() - tracker->startCycle();
    theMemOpCounters[is_offchip][is_coherence][klass]->theRequestLatency += latency;
    theMemOpCounters[is_offchip][is_coherence][klass]->theRequestLatency_Histogram
        << std::make_pair(latency, 1);
  }

  memq_t::index<by_insn>::type::iterator iter;
  switch (klass) {
  case clsLoad:
    ++theEpoch_LoadCount;
    iter = theMemQueue.get<by_insn>().find(inst);
    if (iter != theMemQueue.get<by_insn>().end()) {
      theEpoch_LoadOrStoreBlocks.insert(iter->thePaddr & ~63ULL);
    } else if (inst->getAccessAddress()) {
      theEpoch_LoadOrStoreBlocks.insert((inst->getAccessAddress()) & ~63ULL);
    }
    break;
  case clsAtomic:
  case clsStore:
    ++theEpoch_StoreCount;
    if (theEpoch_OffChipStoreCount > 0) {
      ++theEpoch_StoreAfterOffChipStoreCount;
    }
    if (is_offchip) {
      ++theEpoch_OffChipStoreCount;
    }
    iter = theMemQueue.get<by_insn>().find(inst);
    if (iter != theMemQueue.get<by_insn>().end()) {
      theEpoch_StoreBlocks.insert(iter->thePaddr & ~63ULL);
      theEpoch_LoadOrStoreBlocks.insert(iter->thePaddr & ~63ULL);
    } else if (inst->getAccessAddress()) {
      theEpoch_StoreBlocks.insert((inst->getAccessAddress()) & ~63ULL);
      theEpoch_LoadOrStoreBlocks.insert((inst->getAccessAddress()) & ~63ULL);
    }
    break;
  default:
    break;
  }

  // 21 cycles forces an epoch to have more than an L2 latency
  if (is_offchip && klass != clsStore && inst->retireStallCycles() > 21) {
    // New off-chip miss epoch
    ++theEpochs;

    theEpochs_Instructions << theCommitNumber - theEpoch_StartInsn;
    theEpochs_Instructions_Avg << theCommitNumber - theEpoch_StartInsn;
    theEpochs_Instructions_StdDev << theCommitNumber - theEpoch_StartInsn;
    theEpochs_Loads << theEpoch_LoadCount;
    theEpochs_Loads_Avg << theEpoch_LoadCount;
    theEpochs_Loads_StdDev << theEpoch_LoadCount;
    theEpochs_Stores << theEpoch_StoreCount;
    theEpochs_Stores_Avg << theEpoch_StoreCount;
    theEpochs_Stores_StdDev << theEpoch_StoreCount;
    theEpochs_OffChipStores << theEpoch_OffChipStoreCount;
    theEpochs_OffChipStores_Avg << theEpoch_OffChipStoreCount;
    theEpochs_OffChipStores_StdDev << theEpoch_OffChipStoreCount;
    theEpochs_StoreAfterOffChipStores << theEpoch_StoreAfterOffChipStoreCount;
    theEpochs_StoreAfterOffChipStores_Avg << theEpoch_StoreAfterOffChipStoreCount;
    theEpochs_StoreAfterOffChipStores_StdDev << theEpoch_StoreAfterOffChipStoreCount;
    theEpochs_StoresOutstanding << theSBCount + theSBNAWCount;
    theEpochs_StoresOutstanding_Avg << theSBCount + theSBNAWCount;
    theEpochs_StoresOutstanding_StdDev << theSBCount + theSBNAWCount;

    theEpochs_StoreBlocks << std::make_pair(theEpoch_StoreBlocks.size(), 1);
    theEpochs_LoadOrStoreBlocks << std::make_pair(theEpoch_LoadOrStoreBlocks.size(), 1);

    ++(*theEpochEnd[is_coherence][klass]);
    theEpoch_StartInsn = theCommitNumber;
    theEpoch_LoadCount = 0;
    theEpoch_StoreCount = 0;
    theEpoch_OffChipStoreCount = 0;
    theEpoch_StoreAfterOffChipStoreCount = 0;

    theEpoch_StoreBlocks.clear();
    theEpoch_LoadOrStoreBlocks.clear();
  }
}

bool CoreImpl::squashFrom(boost::intrusive_ptr<Instruction> anInsn) {
  if (!theSquashRequested || (anInsn->sequenceNo() <= (*theSquashInstruction)->sequenceNo())) {
    theSquashRequested = true;
    theSquashReason = kBranchMispredict;
    theEmptyROBCause = kMispredict;
    theSquashInstruction = theROB.project<0>(theROB.get<by_insn>().find(anInsn));
    theSquashInclusive = true;
    return true;
  }
  return false;
}

void CoreImpl::redirectFetch(VirtualMemoryAddress anAddress) {
  DBG_(Iface, (<< "redirectFetch anAddress: " << anAddress));
  theRedirectRequested = true;
  theRedirectPC = anAddress;
}

void CoreImpl::branchFeedback(boost::intrusive_ptr<BranchFeedback> feedback) {
  theBranchFeedback.push_back(feedback);
}

void CoreImpl::takeTrap(boost::intrusive_ptr<Instruction> anInstruction, eExceptionType aTrapType) {

  CORE_DBG(theName << " Take trap: " << aTrapType);

  // Ensure ROB is not empty
  DBG_Assert(!theROB.empty());
  // Only ROB head should raise
  DBG_Assert(anInstruction == theROB.front());
  anInstruction->forceResync();
  // return;

  // Clear ROB
  theSquashRequested = true;
  theSquashReason = kException;
  theEmptyROBCause = kRaisedException;
  theSquashInstruction = theROB.begin();
  theSquashInclusive = true;

  // Record the pending trap
  thePendingTrap = aTrapType;
  theTrapInstruction = anInstruction;

  anInstruction->setWillRaise(aTrapType);
  anInstruction->raise(aTrapType);
}

void CoreImpl::handleTrap() {
  if (thePendingTrap == kException_None) {
    return;
  }
  DBG_(Dev, (<< theName << " Handling trap: " << thePendingTrap
             << " raised by: " << *theTrapInstruction));
  DBG_(Crit, (<< theName << " ROB non-empty in handle trap.  Resynchronize instead."));
  theEmptyROBCause = kRaisedException;
  ++theResync_FailedHandleTrap;
  throw ResynchronizeWithQemuException();
}

void CoreImpl::doSquash() {
  FLEXUS_PROFILE();
  if (theSquashInstruction != theROB.end()) {

    // There is at least one instruction in the ROB
    rob_t::iterator erase_iter = theSquashInstruction;
    if (!theSquashInclusive) {
      ++erase_iter;
    }

    {
      FLEXUS_PROFILE_N("CoreImpl::doSquash() squash");
      rob_t::reverse_iterator iter = theROB.rbegin();
      rob_t::reverse_iterator end = boost::make_reverse_iterator(erase_iter);
      while (iter != end) {
        (*iter)->squash();
        ++iter;
      }
    }

    if (!thePreserveInteractions) {
      FLEXUS_PROFILE_N("CoreImpl::doSquash() clean-interactions");
      // Discard any defferred interactions
      theDispatchInteractions.clear();
    }

    {
      FLEXUS_PROFILE_N("CoreImpl::doSquash() ROB-clear");
      theROB.erase(erase_iter, theROB.end());
    }
  }
}

void CoreImpl::doAbortSpeculation() {
  DBG_Assert(theIsSpeculating);

  // Need to treat ROB head as if it retired to account for stall cycles
  // properly
  if (!theROB.empty()) {
    accountRetire(theROB.front());
  }

  // Ensure that the violating instruction is in the SRB.
  DBG_Assert(!theSRB.empty());
  rob_t::index<by_insn>::type::iterator iter = theSRB.get<by_insn>().find(theViolatingInstruction);
  if (iter == theSRB.get<by_insn>().end()) {
    DBG_Assert(false, (<< " Violating instruction is not in SRB: " << *theViolatingInstruction));
  }

  // Locate the instruction that caused the violation and the nearest
  // preceding checkpoint
  rob_t::iterator srb_iter = theSRB.project<0>(iter);
  rob_t::iterator srb_begin = theSRB.begin();
  rob_t::iterator srb_ckpt = srb_begin;
  int32_t saved_discard_count = 0;
  int32_t ckpt_discard_count = 0;
  int32_t remaining_checkpoints = 0;
  bool checkpoint_found = false;
  bool done = false;
  do {
    if (!checkpoint_found) {
      if ((*srb_iter)->hasCheckpoint()) {
        srb_ckpt = srb_iter;
        checkpoint_found = true;
      } else {
        ++ckpt_discard_count;
      }
    } else {
      ++saved_discard_count;
      if ((*srb_iter)->hasCheckpoint()) {
        ++remaining_checkpoints;
      }
    }
    if (srb_iter == srb_begin) {
      done = true;
    } else {
      --srb_iter;
    }
  } while (!done);

  // Account for location of checkpoint, track discarded instructions
  int32_t required = (theSRB.size() - saved_discard_count - ckpt_discard_count);
  DBG_Assert(required >= 0);
  theSavedDiscards += saved_discard_count;
  theSavedDiscards_Histogram << saved_discard_count;
  theRequiredDiscards += required;
  theRequiredDiscards_Histogram << required;
  theNearestCkptDiscards += ckpt_discard_count;
  theNearestCkptDiscards_Histogram << ckpt_discard_count;

  // Determine which checkpoint we rolled back to.  1 is the eldest,
  // 2 is the next, and so on
  theRollbackToCheckpoint << std::make_pair(remaining_checkpoints + 1LL, 1LL);

  // Determine how many checkpoints were crossed.  Minimum is 1 (rolling back
  // to the most recently created checkpoint.
  int32_t num_ckpts_discarded = theCheckpoints.size() - remaining_checkpoints;
  theCheckpointsDiscarded << std::make_pair(static_cast<int64_t>(num_ckpts_discarded), 1LL);

  // Find the checkpoint corresponding to srb_ckpt
  std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt;
  ckpt = theCheckpoints.find(*srb_ckpt);
  int64_t ckpt_seq_num = (*srb_ckpt)->sequenceNo();
  DBG_Assert(ckpt != theCheckpoints.end());

  DBG_(Dev, (<< theName << " Aborting Speculation.  Violating Instruction: "
             << *theViolatingInstruction << " Roll back to: " << **srb_ckpt << " Required: "
             << required << " Ckpt: " << ckpt_discard_count << " Saved: " << saved_discard_count));

  // Wipe uArchARM and LSQ
  resetArch();

  cleanMSHRS(ckpt_seq_num);

  // Restore checkpoint
  restoreState(ckpt->second.theState);

  // Clean up SRB and SSB.
  theSRB.erase(srb_ckpt, theSRB.end());
  srb_ckpt = theSRB.end(); // srb_ckpt is no longer valid;
  int32_t remaining_ssb = clearSSB(ckpt_seq_num);

  // redirect fetch
  squash_fn(kFailedSpec);
  theRedirectRequested = true;
  theRedirectPC = VirtualMemoryAddress(ckpt->second.theState.thePC);

  // Clean up SLAT
  SpeculativeLoadAddressTracker::iterator slat_iter = theSLAT.begin();
  SpeculativeLoadAddressTracker::iterator slat_item;
  SpeculativeLoadAddressTracker::iterator slat_end = theSLAT.end();
  while (slat_iter != slat_end) {
    slat_item = slat_iter;
    ++slat_iter;
    if (slat_item->second->sequenceNo() >= ckpt_seq_num) {
      theSLAT.erase(slat_item);
    }
  }

  // Clean up checkpoints
  std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ck_iter =
      theCheckpoints.begin();
  std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ck_item;
  std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ck_end = theCheckpoints.end();
  theOpenCheckpoint = 0;
  while (ck_iter != ck_end) {
    ck_item = ck_iter;
    ++ck_iter;
    if (ck_item->first->sequenceNo() >= ckpt_seq_num) {
      theCheckpoints.erase(ck_item);
    } else {
      if (!theOpenCheckpoint || theOpenCheckpoint->sequenceNo() < ck_item->first->sequenceNo()) {
        theOpenCheckpoint = ck_item->first;
      }
    }
  }
  theRetiresSinceCheckpoint = 0;

  // Stop speculating
  theAbortSpeculation = false;

  theEmptyROBCause = kFailedSpeculation;
  theIsSpeculating = !theCheckpoints.empty();
  accountAbortSpeculation(ckpt_seq_num);

  // Charge stall cycles for TSOB replay
  if (remaining_ssb > 0) {
    theTSOBReplayStalls = remaining_ssb / theNumMemoryPorts;
  }
}

int32_t CoreImpl::availableROB() const {
  if (theInterruptSignalled) {
    // Halt dispatch when we are trying to take an interrupt
    return 0;
  } else if (theSpinning && theSpinControlEnabled && theLSQCount > 0) {
    return 0;
  } else {
    return theROBSize - theROB.size();
  }
}

bool CoreImpl::isStalled() const {
  return (theMemoryReplies.empty() &&
          (theROB.empty() ||
           (!theMSHRs.empty() &&
            (getROBHeadClass() == clsLoad || getROBHeadClass() == clsAtomic ||
             getROBHeadClass() == clsMEMBAR || getROBHeadClass() == clsSynchronizing))));
}

bool CoreImpl::isHalted() const {
  return cpuHalted;
}

int32_t CoreImpl::iCount() const {
  return theROB.size();
}

void CoreImpl::dispatch(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();

  if (static_cast<int>(theNode) == Flexus::Core::theFlexus->breakCPU()) {
    if (anInsn->sequenceNo() == Flexus::Core::theFlexus->breakInsn()) {
      DBG_(VVerb, (<< theName << " Encounted break instruction: " << *anInsn));
      Flexus::Core::theFlexus->setDebug("verb");
    }
  }

  theIdleThisCycle = false;

  if (anInsn->haltDispatch()) {
    // A synchronizing instruction which had to move through the pipeline by
    // itself.  There will be empty ROB stalls as a result of this instruction.

    theEmptyROBCause = kSync; // Empty ROB stalls are the result of a
                              // synchronizing instruction.
  } else {
    // Any time we dispatch an instruction, any empty ROB stalls that happen
    // before the next branch misprediction or resync will be as a result of
    // I-cache misses.

    boost::intrusive_ptr<TransactionTracker> tracker = anInsn->getFetchTransactionTracker();

    // account for mipredictions by seperating the misprediction cycle cost when
    // filled by L1 from the rest of the cycle cost when filled by another level
    // of the memory hierarchy
    if (theROB.empty() && theEmptyROBCause == kMispredict) {
      if (!tracker || (tracker && tracker->fillLevel() && *tracker->fillLevel() != ePeerL1Cache &&
                       *tracker->fillLevel() != eL2 && *tracker->fillLevel() != ePeerL2 &&
                       *tracker->fillLevel() != eL3 && *tracker->fillLevel() != eLocalMem &&
                       *tracker->fillLevel() != eRemoteMem)) {
        // This is a plain old misprediction that hit in L1. Use the cycles to
        // estimate the avg misprediction cost when filled by L1.
        theMispredictCount++;
        theMispredictCycles += theTimeBreakdown.getTmpStallCycles();
      } else {
        // This is a misprediction that was filled by a cache other than L1 or
        // by main mem. Calculate the avg stall time of a misprediction filled
        // by L1.
        const uint64_t theAvgMispredictCycles =
            (theMispredictCount > 0
                 ? static_cast<uint64_t>(theMispredictCycles * 1.0 / theMispredictCount + 0.5)
                 : 0);
        // Subtract the avg misprediction cost from the temporary cycle
        // accumulator. The remaining cycles will account for an instruction
        // PeerL1 or L2 or Mem stall category.
        const uint64_t theCurrMispredictCycles =
            theTimeBreakdown.subtTmpStallCycles(theAvgMispredictCycles);
        // Put the avg misprediction cost (when filled by L1) into the
        // misprediction stats bin.
        theTimeBreakdown.commitAccumulatedCycles(nXactTimeBreakdown::kEmptyROB_Mispredict,
                                                 theCurrMispredictCycles);
      }
    }

    if (tracker && tracker->fillLevel()) {
      switch (*tracker->fillLevel()) {
      case ePeerL1Cache:
        theEmptyROBCause = kIStall_PeerL1;
        break;
      case eL2:
        theEmptyROBCause = kIStall_L2;
        break;
      case ePeerL2:
        theEmptyROBCause = kIStall_PeerL2;
        break;
      case eL3:
        theEmptyROBCause = kIStall_L3;
        break;
      case eLocalMem:
      case eRemoteMem:
        theEmptyROBCause = kIStall_Mem;
        break;
      default:
        theEmptyROBCause = (theEmptyROBCause == kMispredict ? kMispredict : kIStall_Other);
        break;
      }
    } else {
      theEmptyROBCause = (theEmptyROBCause == kMispredict ? kMispredict : kIStall_Other);
    }

    if (theROB.empty()) {
      switch (theEmptyROBCause) {
      case kIStall_PeerL1:
        theTimeBreakdown.commitAccumulatedCycles(nXactTimeBreakdown::kEmptyROB_IStall_PeerL1);
        break;
      case kIStall_L2:
        theTimeBreakdown.commitAccumulatedCycles(nXactTimeBreakdown::kEmptyROB_IStall_L2);
        break;
      case kIStall_PeerL2:
        theTimeBreakdown.commitAccumulatedCycles(nXactTimeBreakdown::kEmptyROB_IStall_PeerL2);
        break;
      case kIStall_L3:
        theTimeBreakdown.commitAccumulatedCycles(nXactTimeBreakdown::kEmptyROB_IStall_L3);
        break;
      case kIStall_Mem:
        theTimeBreakdown.commitAccumulatedCycles(nXactTimeBreakdown::kEmptyROB_IStall_Mem);
        break;
      case kIStall_Other:
        theTimeBreakdown.commitAccumulatedCycles(
            nXactTimeBreakdown::kEmptyROB_IStall_Other); // put L1 and unknown IStall fill
                                                         // levels into "other" category
        break;
      case kMispredict:
        theTimeBreakdown.commitAccumulatedCycles(
            nXactTimeBreakdown::kEmptyROB_Mispredict); // put L1 and unknown mispredict fill
                                                       // levels into "mispredict" category
        break;
      default:
        DBG_Assert(false, (<< theName << " The cause=" << theEmptyROBCause));
      }
    }
  }

  anInsn->connectuArch(*this);
  // If in-order execution is enabled, hook instructions together to force
  // them to execute in order.
  if (theInOrderExecute && !theROB.empty()) {
    anInsn->setPreceedingInstruction(theROB.back());
  }
  theROB.push_back(anInsn);
  // theNPC = boost::none;

  // Due to OoO, we don't know what instruction will be sent to a functional
  // unit when it becomes available and thus cannot track their ready times.
  // Thus if we want to never overestimate the amount of time to execute an
  // instruction, we have to assume each can go without stalling.
  if (anInsn->usesIntAlu()) {
    anInsn->setCanRetireCounter(intAluOpLatency);
  } else if (anInsn->usesIntMult()) {
    anInsn->setCanRetireCounter(intMultOpLatency);
  } else if (anInsn->usesIntDiv()) {
    anInsn->setCanRetireCounter(intDivOpLatency);
  } else if (anInsn->usesFpAdd()) {
    anInsn->setCanRetireCounter(fpAddOpLatency);
  } else if (anInsn->usesFpCmp()) {
    anInsn->setCanRetireCounter(fpCmpOpLatency);
  } else if (anInsn->usesFpCvt()) {
    anInsn->setCanRetireCounter(fpCvtOpLatency);
  } else if (anInsn->usesFpMult()) {
    anInsn->setCanRetireCounter(fpMultOpLatency);
  } else if (anInsn->usesFpDiv()) {
    anInsn->setCanRetireCounter(fpDivOpLatency);
  } else if (anInsn->usesFpSqrt()) {
    anInsn->setCanRetireCounter(fpSqrtOpLatency);
  }

  std::list<boost::intrusive_ptr<Interaction>> dispatch_interactions;
  std::swap(dispatch_interactions, theDispatchInteractions);

  DBG_(VVerb, (<< *anInsn));
  anInsn->doDispatchEffects();

  while (!dispatch_interactions.empty()) {
    DBG_(VVerb, (<< theName << " applying deferred interation " << (*dispatch_interactions.front())
                 << " to " << *anInsn));
    (*dispatch_interactions.front())(anInsn, *this);
    dispatch_interactions.pop_front();
  }
  DISPATCH_DBG("Done");
}

void CoreImpl::applyToNext(boost::intrusive_ptr<Instruction> anInsn,
                           boost::intrusive_ptr<Interaction> anInteraction) {
  FLEXUS_PROFILE();
  //  typedef rob_t::index<by_insn>::type insn_index;
  rob_t::iterator insn = theROB.project<0>(theROB.get<by_insn>().find(anInsn));
  rob_t::iterator end = theROB.end();

  DBG_Assert(insn != end);
  rob_t::iterator next = ++insn;

  DBG_(Verb, (<< theName << *anInsn << " applying interation " << *anInteraction
              << " to following instruction"));
  if (next == end) {
    // Need to defer effect
    DBG_(Verb, (<< theName << "Interaction deferred"));
    theDispatchInteractions.push_back(anInteraction);
  } else {
    DBG_(Verb, (<< theName << "Applying to " << **next));
    (*anInteraction)(*next, *this);
  }
}

void CoreImpl::deferInteraction(boost::intrusive_ptr<Instruction> anInsn,
                                boost::intrusive_ptr<Interaction> anInteraction) {
  DBG_(Verb, (<< theName << *anInsn << " saving deffered interaction " << *anInteraction << "."));
  thePreserveInteractions = true;
  theDispatchInteractions.push_back(anInteraction);
}

void CoreImpl::create(boost::intrusive_ptr<SemanticAction> anAction) {
  CORE_DBG(*anAction);
  theRescheduledActions.push(anAction);
}

void CoreImpl::reschedule(boost::intrusive_ptr<SemanticAction> anAction) {
  CORE_DBG(*anAction);
  theRescheduledActions.push(anAction);
}

bool ActionOrder::operator()(boost::intrusive_ptr<SemanticAction> const &l,
                             boost::intrusive_ptr<SemanticAction> const &r) const {
  return l->instructionNo() > r->instructionNo();
}

MemoryPortArbiter::MemoryPortArbiter(CoreImpl &aCore, int32_t aNumPorts,
                                     int32_t aMaxStorePrefetches)
    : theNumPorts(aNumPorts), theCore(aCore), theMaxStorePrefetches(aMaxStorePrefetches) {
}

void CoreImpl::bypass(mapped_reg aReg, register_value aValue) {
  theBypassNetwork.write(aReg, aValue, *this);
}

void CoreImpl::connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst,
                             std::function<bool(register_value)> fn) {
  theBypassNetwork.connect(aReg, inst, fn);
}

void CoreImpl::mapRegister(mapped_reg aRegister) {
  FLEXUS_PROFILE();
  eResourceStatus status = theRegisters.status(aRegister);
  DBG_Assert(status == kUnmapped, (<< " aRegister=" << aRegister << " status=" << status));
  theRegisters.map(aRegister);
}

void CoreImpl::unmapRegister(mapped_reg aRegister) {
  FLEXUS_PROFILE();
  eResourceStatus status = theRegisters.status(aRegister);
  DBG_Assert(status != kUnmapped, (<< " aRegister=" << aRegister << " status=" << status));
  theRegisters.unmap(aRegister);
  theBypassNetwork.unmap(aRegister);
}

eResourceStatus CoreImpl::requestRegister(mapped_reg aRegister) {
  return theRegisters.status(aRegister);
}

eResourceStatus CoreImpl::requestRegister(mapped_reg aRegister,
                                          InstructionDependance const &aDependance) {
  return theRegisters.request(aRegister, aDependance);
}

void CoreImpl::squashRegister(mapped_reg aRegister) {
  return theRegisters.squash(aRegister, *this);
}

register_value CoreImpl::readRegister(mapped_reg aRegister) {
  return theRegisters.read(aRegister);
}

register_value CoreImpl::readArchitecturalRegister(reg aRegister, bool aRotate) {
  reg reg = aRegister;
  mapped_reg mreg;
  mreg.theType = reg.theType;
  mreg.theIndex = mapTable(reg.theType).mapArchitectural(reg.theIndex);
  return theRegisters.peek(mreg);
}

void CoreImpl::writeRegister(mapped_reg aRegister, register_value aValue, bool isW = false) {
  return theRegisters.write(aRegister, aValue, *this, isW);
}

void CoreImpl::initializeRegister(mapped_reg aRegister, register_value aValue) {
  theRegisters.poke(aRegister, aValue);
  theRegisters.setStatus(aRegister, kReady);
}

void CoreImpl::copyRegValue(mapped_reg aSource, mapped_reg aDest) {
  theRegisters.poke(aDest, theRegisters.peek(aSource));
}

PhysicalMap &CoreImpl::mapTable(eRegisterType aMapTable) {
  return *theMapTables[aMapTable];
}

mapped_reg CoreImpl::map(reg aReg) {
  FLEXUS_PROFILE();
  mapped_reg ret_val;
  ret_val.theType = aReg.theType;
  ret_val.theIndex = mapTable(aReg.theType).map(aReg.theIndex);
  return ret_val;
}

std::pair<mapped_reg, mapped_reg> CoreImpl::create(reg aReg) {
  FLEXUS_PROFILE();
  std::pair<mapped_reg, mapped_reg> mapped;
  mapped.first.theType = mapped.second.theType = aReg.theType;
  std::tie(mapped.first.theIndex, mapped.second.theIndex) =
      mapTable(aReg.theType).create(aReg.theIndex);
  mapRegister(mapped.first);

  eResourceStatus status = theRegisters.status(mapped.second);
  DBG_Assert(status != kUnmapped, (<< " aRegister=" << mapped.second << " status=" << status));
  return mapped;
}

void CoreImpl::free(mapped_reg aReg) {
  FLEXUS_PROFILE();
  mapTable(aReg.theType).free(aReg.theIndex);
  unmapRegister(aReg);
}

void CoreImpl::restore(reg aName, mapped_reg aReg) {
  FLEXUS_PROFILE();
  DBG_Assert(aName.theType == aReg.theType);
  mapTable(aName.theType).restore(aName.theIndex, aReg.theIndex);
}

void CoreImpl::reset() {
  FLEXUS_PROFILE();

  cleanMSHRS(0);

  resetArch();

  theSRB.clear();

  // theBranchFeedback is NOT cleared

  clearSSB();

  if (theIsSpeculating) {
    DBG_(Iface, (<< theName << " Speculation aborted by sync"));
    accountResyncSpeculation();
  } else {
    accountResync();
  }

  theSLAT.clear();
  theIsSpeculating = false;
  theIsIdle = false;
  theCheckpoints.clear();
  theOpenCheckpoint = 0;
  theRetiresSinceCheckpoint = 0;

  theAbortSpeculation = false;

  theKernelPanicCount = 0;

  if (theSpinning) { // Note: will be false on first call to reset()
    theSpinCount += theSpinDetectCount;
    theSpinning = false;
    theSpinCycles += theFlexus->cycleCount() - theSpinStartCycle;
  }

  theSpinning = false;
  theSpinMemoryAddresses.clear();
  theSpinPCs.clear();
  theSpinDetectCount = 0;
  theSpinRetireSinceReset = 0;

  // theMemoryPorts and theSnoopPorts are not reset.
  // theMemoryReplies is not reset

  theIdleCycleCount = 0;
  theIdleThisCycle = false;

  while (!theTranslationQueue.empty()) {
    theTranslationQueue.pop();
  }
}

void CoreImpl::resetArch() {

  mapTable(xRegisters).reset();
  mapTable(vRegisters).reset();
  theRegisters.reset();

  thePendingTrap = kException_None;
  theTrapInstruction = 0;

  theInterruptSignalled = false;
  thePendingInterrupt = kException_None;
  theInterruptInstruction = 0;

  theBypassNetwork.reset();

  theActiveActions = action_list_t();
  theRescheduledActions = action_list_t();

  theDispatchInteractions.clear();
  thePreserveInteractions = false;

  theROB.clear();

  theSquashRequested = false;
  theSquashReason = eSquashCause(0);
  theSquashInclusive = false;

  theRedirectRequested = false;
  theRedirectPC = VirtualMemoryAddress(0);
  theDumpPC = VirtualMemoryAddress(0);

  clearLSQ();

  thePartialSnoopersOutstanding = 0;
}

void CoreImpl::getState(State &aState) {
  reg r;
  r.theType = xRegisters;

  for (int32_t i = 0; i < 32; ++i) {
    r.theIndex = i;
    aState.theGlobalRegs[i] = boost::get<uint64_t>(readArchitecturalRegister(r, false));
  }

  aState.thePC = pc();
}

void CoreImpl::restoreState(State &aState) {
  for (int32_t i = 0; i < 32; ++i) {
    initializeRegister(xReg(i), aState.theGlobalRegs[i]);
  }

  setPC(aState.thePC);
}

void CoreImpl::compareState(State &aLeft, State &aRight) {
  for (int32_t i = 0; i < 32; ++i) {
    DBG_Assert((aLeft.theGlobalRegs[i] == aRight.theGlobalRegs[i]),
               (<< "aLeft.theGlobalRegs[" << i << "] = " << std::hex << aLeft.theGlobalRegs[i]
                << std::dec << ", aRight.theGlobalRegs[" << i << "] = " << std::hex
                << aRight.theGlobalRegs[i] << std::dec));
  }
  for (int32_t i = 0; i < 64; ++i) {
    DBG_Assert((aLeft.theFPRegs[i] == aRight.theFPRegs[i]),
               (<< "aLeft.theFPRegs[" << i << "] = " << std::hex << aLeft.theFPRegs[i] << std::dec
                << ", aRight.theFPRegs[" << i << "] = " << std::hex << aRight.theFPRegs[i]
                << std::dec));
  }

  DBG_Assert((aLeft.thePC == aRight.thePC), (<< std::hex << "aLeft.thePC = " << aLeft.thePC
                                             << ", aRight.thePC = " << aRight.thePC << std::dec));
}

bool CoreImpl::isIdleLoop() {
  return false; // ALEX - we don't currently have WhiteBox in QEMU
}

uint64_t CoreImpl::pc() const {
  return thePC;
}

void CoreImpl::setException(Flexus::Qemu::API::exception_t anEXP) {
  theEXP = anEXP;
}

Flexus::Qemu::API::exception_t CoreImpl::getException() {
  return theEXP;
}

void CoreImpl::setXRegister(uint32_t aReg, uint64_t aVal) {
  reg r;
  r.theType = xRegisters;
  r.theIndex = aReg;
  mapped_reg reg_p = map(r);
  writeRegister(reg_p, aVal);
}

uint64_t CoreImpl::getXRegister(uint32_t aReg) {
  reg r;
  r.theType = xRegisters;
  r.theIndex = aReg;
  mapped_reg reg_p = map(r);
  register_value reg_val = readRegister(reg_p);
  return boost::get<uint64_t>(reg_val);
}

void CoreImpl::setPC(uint64_t aPC) {
  thePC = aPC;
}

uint64_t CoreImpl::readUnhashedSysReg(uint32_t no) {
  return theQEMUCPU.read_sysreg_from_qemu(no);
}

void CoreImpl::breakMSHRLink(memq_t::index<by_insn>::type::iterator iter) {
  FLEXUS_PROFILE();
  iter->theIssued = false;
  if (iter->theMSHR) {
    DBG_(Verb, (<< "Breaking MSHR connection of " << *iter));
    // Break dependance on pending load
    std::list<memq_t::index<by_insn>::type::iterator>::iterator link =
        find((*iter->theMSHR)->second.theWaitingLSQs.begin(),
             (*iter->theMSHR)->second.theWaitingLSQs.end(), iter);
    DBG_Assert(link != (*iter->theMSHR)->second.theWaitingLSQs.end());
    (*iter->theMSHR)->second.theWaitingLSQs.erase(link);

    // Delete the MSHR and wake blocked mem ops
    if ((*iter->theMSHR)->second.theWaitingLSQs.empty()) {
      // Reissue blocked store prefetches
      std::list<boost::intrusive_ptr<Instruction>>::iterator pf_iter, pf_end;
      pf_iter = (*iter->theMSHR)->second.theBlockedPrefetches.begin();
      pf_end = (*iter->theMSHR)->second.theBlockedPrefetches.end();
      while (pf_iter != pf_end) {

        memq_t::index<by_insn>::type::iterator lsq_entry =
            theMemQueue.get<by_insn>().find(*pf_iter);
        if (lsq_entry != theMemQueue.get<by_insn>().end()) {
          requestStorePrefetch(lsq_entry);
        }

        ++pf_iter;
      }
      (*iter->theMSHR)->second.theBlockedPrefetches.clear();

      std::list<boost::intrusive_ptr<Instruction>> wake_list;
      wake_list.swap((*iter->theMSHR)->second.theBlockedOps);
      if ((*iter->theMSHR)->second.theTracker) {
        (*iter->theMSHR)->second.theTracker->setWrongPath(true);
      }
      theMSHRs.erase(*iter->theMSHR);

      std::list<boost::intrusive_ptr<Instruction>>::iterator wake_iter, wake_end;
      wake_iter = wake_list.begin();
      wake_end = wake_list.end();
      while (wake_iter != wake_end) {
        DBG_(Verb, (<< theName << " Port request from here: " << *wake_iter));
        requestPort(*wake_iter);
        ++wake_iter;
      }
    }
    iter->theMSHR = boost::none;
  }
}

void CoreImpl::insertLSQ(boost::intrusive_ptr<Instruction> anInsn, eOperation anOperation,
                         eSize aSize, bool aBypassSB, eAccType type) {
  FLEXUS_PROFILE();
  theMemQueue.push_back(MemQueueEntry(anInsn, ++theMemorySequenceNum, anOperation, aSize,
                                      aBypassSB && theNAWBypassSB));
  DBG_(VVerb, (<< "Pushed LSQEntry: " << theMemQueue.back()));
  ++theLSQCount;
}

void CoreImpl::insertLSQ(boost::intrusive_ptr<Instruction> anInsn, eOperation anOperation,
                         eSize aSize, bool aBypassSB, InstructionDependance const &aDependance,
                         eAccType type) {
  FLEXUS_PROFILE();
  theMemQueue.push_back(MemQueueEntry(anInsn, ++theMemorySequenceNum, anOperation, aSize,
                                      aBypassSB && theNAWBypassSB, aDependance));
  DBG_(Verb, (<< "Pushed LSQEntry: " << theMemQueue.back()));
  ++theLSQCount;
}

void CoreImpl::eraseLSQ(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  if (iter == theMemQueue.get<by_insn>().end()) {
    return;
  }

  breakMSHRLink(iter);
  DBG_(Verb, (<< "Popping LSQEntry: " << *iter));

  if (iter->thePartialSnoop) {
    --thePartialSnoopersOutstanding;
  }

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
              default:
                theStallType = nXactTimeBreakdown::kStore_BufferFull_L2_Unknown;
                break;
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
              default:
                theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Unknown;
                break;
              }
            } else
              theStallType = nXactTimeBreakdown::kStore_BufferFull_L3_Unknown;
            break;
          case eLocalMem:
          case eRemoteMem:
            theStallType = nXactTimeBreakdown::kStore_BufferFull_Mem;
            break;
          case ePeerL2:
            theStallType = nXactTimeBreakdown::kStore_BufferFull_PeerL2Cache;
            break;
          case ePeerL1Cache:
            theStallType = nXactTimeBreakdown::kStore_BufferFull_PeerL1Cache;
            break;
          default:
            theStallType = nXactTimeBreakdown::kStore_BufferFull_Other;
            break;
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
  default:
    DBG_Assert(false, (<< " invalid queue:" << iter->theQueue));
  }

  /*
  if (iter->isStore() && !anInsn->isAnnulled() &&
  anInsn->getTransactionTracker()) { consortAppend( PredictorMessage::eWrite,
  iter->thePaddr,  anInsn->getTransactionTracker());
  }
  */

  if ((iter->thePaddr == kUnresolved) || iter->isAbnormalAccess()) {
    iter->theInstruction->setAccessAddress(PhysicalMemoryAddress(0));
  } else {
    iter->theInstruction->setAccessAddress(iter->thePaddr);
  }

  // Kill off any pending prefetch request
  killStorePrefetches(iter->theInstruction);

  theMemQueue.get<by_insn>().erase(iter);
  DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
}

void CoreImpl::cleanMSHRS(uint64_t aDiscardAfterSequenceNum) {
  MSHRs_t::iterator mshr_iter, mshr_temp, mshr_end;
  mshr_iter = theMSHRs.begin();
  mshr_end = theMSHRs.end();
  while (mshr_iter != mshr_end) {
    mshr_temp = mshr_iter;
    ++mshr_iter;

    std::list<memq_t::index<by_insn>::type::iterator>::iterator wait_iter, wait_temp, wait_end;
    wait_iter = mshr_temp->second.theWaitingLSQs.begin();
    wait_end = mshr_temp->second.theWaitingLSQs.end();
    while (wait_iter != wait_end) {
      wait_temp = wait_iter;
      ++wait_iter;
      if ((*wait_temp)->theQueue == kLSQ ||
          ((*wait_temp)->theQueue == kSSB &&
           (*wait_temp)->theInstruction->sequenceNo() >=
               (static_cast<int64_t>(aDiscardAfterSequenceNum)))) {
        mshr_temp->second.theWaitingLSQs.erase(wait_temp);
      }
    }
    if (mshr_temp->second.theWaitingLSQs.empty()) {

      // Reissue blocked store prefetches
      std::list<boost::intrusive_ptr<Instruction>>::iterator pf_iter, pf_end;
      pf_iter = mshr_temp->second.theBlockedPrefetches.begin();
      pf_end = mshr_temp->second.theBlockedPrefetches.end();
      while (pf_iter != pf_end) {
        memq_t::index<by_insn>::type::iterator lsq_entry =
            theMemQueue.get<by_insn>().find(*pf_iter);
        if (lsq_entry != theMemQueue.get<by_insn>().end()) {
          requestStorePrefetch(lsq_entry);
        }
        ++pf_iter;
      }
      mshr_temp->second.theBlockedPrefetches.clear();

      std::list<boost::intrusive_ptr<Instruction>> wake_list;
      wake_list.swap(mshr_temp->second.theBlockedOps);
      if (mshr_temp->second.theTracker) {
        mshr_temp->second.theTracker->setWrongPath(true);
      }
      theMSHRs.erase(mshr_temp);

      std::list<boost::intrusive_ptr<Instruction>>::iterator wake_iter, wake_end;
      wake_iter = wake_list.begin();
      wake_end = wake_list.end();
      while (wake_iter != wake_end) {
        DBG_(Verb, (<< theName << " Port request from here: " << *wake_iter));
        requestPort(*wake_iter);
        ++wake_iter;
      }
    }
  }
}

void CoreImpl::clearLSQ() {
  theMemQueue.get<by_queue>().erase(theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ)),
                                    theMemQueue.get<by_queue>().upper_bound(std::make_tuple(kLSQ)));
  theLSQCount = 0;
  DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
}

void CoreImpl::clearSSB() {
  int32_t sb_count = 0;
  int32_t sbnaw_count = 0;
  memq_t::index<by_queue>::type::iterator lb, iter, ub;
  std::tie(lb, ub) = theMemQueue.get<by_queue>().equal_range(std::make_tuple(kSSB));
  iter = lb;
  while (iter != ub) {
    DBG_(Iface, (<< theName << " unrequire " << *iter));
    if (iter->isStore() || iter->isAtomic()) {
      unrequireWritePermission(iter->thePaddr);
    }
    if (iter->theBypassSB) {
      sbnaw_count++;
    } else {
      sb_count++;
    }
    ++iter;
  }

  // commit stall cycles due to full store buffer
  if (sbFull()) {
    theTimeBreakdown.commitAccumulatedStoreCycles(nXactTimeBreakdown::kStore_BufferFull_Other);
  }

  theMemQueue.get<by_queue>().erase(lb, ub);
  theSBCount -= sb_count;
  theSBNAWCount -= sbnaw_count;
  DBG_Assert(theSBCount >= 0);
  DBG_Assert(theSBNAWCount >= 0);
  DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
}

int32_t CoreImpl::clearSSB(uint64_t aLowestInsnSeq) {
  int32_t sb_count = 0;
  int32_t sbnaw_count = 0;
  int32_t remaining_ssb_count = 0;
  memq_t::index<by_queue>::type::iterator lb, iter, ub;
  std::tie(lb, ub) = theMemQueue.get<by_queue>().equal_range(std::make_tuple(kSSB));
  iter = lb;
  bool first_found = false;
  while (iter != ub) {
    DBG_Assert((static_cast<int64_t>(aLowestInsnSeq)) >= 0);
    if (!first_found &&
        iter->theInstruction->sequenceNo() >= (static_cast<int64_t>(aLowestInsnSeq))) {
      lb = iter;
      first_found = true;
    } else {
      ++remaining_ssb_count;
    }
    if (first_found) {
      DBG_(Iface, (<< theName << " unrequire " << *iter));
      if (iter->isStore() || iter->isAtomic()) {
        unrequireWritePermission(iter->thePaddr);
      }
      if (iter->theBypassSB) {
        sbnaw_count++;
      } else {
        sb_count++;
      }
    }
    ++iter;
  }

  // commit stall cycles due to full store buffer
  if (sbFull()) {
    theTimeBreakdown.commitAccumulatedStoreCycles(nXactTimeBreakdown::kStore_BufferFull_Other);
  }

  theMemQueue.get<by_queue>().erase(lb, ub);
  theSBCount -= sb_count;
  theSBNAWCount -= sbnaw_count;
  DBG_Assert(theSBCount >= 0);
  DBG_Assert(theSBNAWCount >= 0);
  DBG_Assert(theLSQCount + theSBCount + theSBNAWCount == static_cast<long>(theMemQueue.size()));
  return remaining_ssb_count;
}

void CoreImpl::pushMemOp(boost::intrusive_ptr<MemOp> anOp) {
  theMemoryReplies.push_back(anOp);
  DBG_(Iface, (<< " Received: " << *anOp));
}

bool CoreImpl::canPushMemOp() {
  return theMemoryReplies.size() < theNumMemoryPorts + theNumSnoopPorts;
}

boost::intrusive_ptr<MemOp> CoreImpl::popMemOp() {
  FLEXUS_PROFILE();
  boost::intrusive_ptr<MemOp> ret_val;
  if (!theMemoryPorts.empty()) {
    ret_val = theMemoryPorts.front();

    DBG_(Iface, (<< "Sending: " << *ret_val));
    theMemoryPorts.pop_front();
  }
  return ret_val;
}

boost::intrusive_ptr<MemOp> CoreImpl::popSnoopOp() {
  boost::intrusive_ptr<MemOp> ret_val;
  if (!theSnoopPorts.empty()) {
    ret_val = theSnoopPorts.front();
    DBG_(Iface, (<< "Sending: " << *ret_val));
    theSnoopPorts.pop_front();
  }
  return ret_val;
}

TranslationPtr CoreImpl::popTranslation() {
  TranslationPtr ret_val;
  if (!theTranslationQueue.empty()) {
    ret_val = theTranslationQueue.front();
    theTranslationQueue.pop();
  }
  return ret_val;
}

void CoreImpl::pushTranslation(TranslationPtr aTranslation) {

  boost::intrusive_ptr<Instruction> insn =
      boost::polymorphic_pointer_downcast<Instruction>(aTranslation->getInstruction());
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(insn);
  if (lsq_entry == theMemQueue.get<by_insn>().end())
    return;
  if (lsq_entry->theVaddr != aTranslation->theVaddr)
    return;

  if (!insn->resync() && !(aTranslation->isPagefault())) {
    resolvePAddr(insn, aTranslation->thePaddr);

    DBG_(Iface,
         (<< "Resolved.. vaddr: " << lsq_entry->theVaddr << " to paddr " << lsq_entry->thePaddr));
  } else {
    DBG_(Iface, (<< "Not Resolved.. vaddr: " << lsq_entry->theVaddr << " due to resync."));

    lsq_entry->theException = kException_; // HEHE
    insn->pageFault();
  }

  DBG_Assert(lsq_entry->thePaddr != kInvalid);
}

void CoreImpl::invalidateCache(eCacheType aType) {
  // TODO
}

void CoreImpl::invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress) {
  // TODO
}

void CoreImpl::invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, uint32_t aSize) {
  // TODO
}

void CoreImpl::SystemRegisterTrap(uint32_t no) {
}

void CoreImpl::createCheckpoint(boost::intrusive_ptr<Instruction> anInsn) {
  std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt;
  bool is_new;
  std::tie(ckpt, is_new) = theCheckpoints.insert(std::make_pair(anInsn, Checkpoint()));
  anInsn->setHasCheckpoint(true);
  getState(ckpt->second.theState);
  DBG_(Verb, (<< theName << "Collecting checkpoint for " << *anInsn));
  DBG_Assert(is_new);

  theMaxSpeculativeCheckpoints << theCheckpoints.size();
  DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
  DBG_Assert(theAllowedSpeculativeCheckpoints == 0 ||
             (static_cast<int>(theCheckpoints.size())) <= theAllowedSpeculativeCheckpoints);
  theOpenCheckpoint = anInsn;
  theRetiresSinceCheckpoint = 0;

  startSpeculating();
}

void CoreImpl::freeCheckpoint(boost::intrusive_ptr<Instruction> anInsn) {
  std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt =
      theCheckpoints.find(anInsn);
  DBG_Assert(ckpt != theCheckpoints.end());
  theCheckpoints.erase(ckpt);
  anInsn->setHasCheckpoint(false);
  if (theOpenCheckpoint == anInsn) {
    theOpenCheckpoint = 0;
  }
  checkStopSpeculating();
}

void CoreImpl::requireWritePermission(memq_t::index<by_insn>::type::iterator aWrite) {
  PhysicalMemoryAddress addr(aWrite->thePaddr & ~(theCoherenceUnit - 1));
  if (aWrite->thePaddr > 0) {
    std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline;
    bool is_new;
    DBG_(Iface, (<< theName << "requireWritePermission : " << *aWrite));
    std::tie(sbline, is_new) =
        theSBLines_Permission.insert(std::make_pair(addr, std::make_pair(1, false)));
    if (is_new) {
      ++theSBLines_Permission_falsecount;
      DBG_Assert(theSBLines_Permission_falsecount >= 0);
      DBG_Assert(theSBLines_Permission_falsecount <=
                 (static_cast<int>(theSBLines_Permission.size())));
    } else {
      sbline->second.first++;
    }
    if (!sbline->second.second) {
      requestStorePrefetch(aWrite);
    }
    if (theOpenCheckpoint) {
      if (theConsistencyModel == kSC || aWrite->isAtomic()) {
        theCheckpoints[theOpenCheckpoint].theRequiredPermissions.insert(
            std::make_pair(addr, aWrite->theInstruction));
        if (sbline->second.second) {
          theCheckpoints[theOpenCheckpoint].theHeldPermissions.insert(addr);
        }
      }
    }
  }
}

void CoreImpl::unrequireWritePermission(PhysicalMemoryAddress anAddress) {
  PhysicalMemoryAddress addr(static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1));
  std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline =
      theSBLines_Permission.find(addr);
  if (sbline != theSBLines_Permission.end()) {
    --sbline->second.first;
    DBG_Assert(sbline->second.first >= 0);
    if (sbline->second.first == 0) {
      DBG_(Iface, (<< theName << " Stop tracking: " << anAddress));
      if (!sbline->second.second) {
        --theSBLines_Permission_falsecount;
        DBG_Assert(theSBLines_Permission_falsecount >= 0);
      }

      theSBLines_Permission.erase(sbline);
    }
  }
}

void CoreImpl::acquireWritePermission(PhysicalMemoryAddress anAddress) {
  PhysicalMemoryAddress addr(static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1));
  std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline =
      theSBLines_Permission.find(addr);
  if (sbline != theSBLines_Permission.end()) {
    if (!sbline->second.second) {
      DBG_(VVerb, (<< theName << " Acquired write permission for address in SB: " << addr));
      --theSBLines_Permission_falsecount;
      DBG_Assert(theSBLines_Permission_falsecount >= 0);
    }
    sbline->second.second = true;
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt = theCheckpoints.begin();
    while (ckpt != theCheckpoints.end()) {
      if (ckpt->second.theRequiredPermissions.find(addr) !=
          ckpt->second.theRequiredPermissions.end()) {
        ckpt->second.theHeldPermissions.insert(addr);
      }
      ++ckpt;
    }
  }
}

void CoreImpl::loseWritePermission(eLoseWritePermission aReason, PhysicalMemoryAddress anAddress) {
  PhysicalMemoryAddress addr(anAddress & ~(theCoherenceUnit - 1));
  std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline =
      theSBLines_Permission.find(addr);
  if (sbline != theSBLines_Permission.end()) {
    if (sbline->second.second) {
      switch (aReason) {
      case eLosePerm_Invalidate:
        DBG_(Verb, (<< theName
                    << " Invalidate removed write permission for line in store buffer:" << addr));
        ++theInv_HitSB;
        break;
      case eLosePerm_Downgrade:
        DBG_(Verb, (<< theName
                    << " Downgrade removed write permission for line in store buffer: " << addr));
        ++theDG_HitSB;
        break;
      case eLosePerm_Replacement:
        DBG_(Verb, (<< theName
                    << " Replacement removed write permission for line in "
                       "store buffer:"
                    << addr));
        ++theRepl_HitSB;
        break;
      }
      ++theSBLines_Permission_falsecount;
      DBG_Assert(theSBLines_Permission_falsecount >= 0);
      DBG_Assert(theSBLines_Permission_falsecount <=
                 (static_cast<int>(theSBLines_Permission.size())));
    }
    sbline->second.second = false;

    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt = theCheckpoints.begin();
    bool requested = false;
    while (ckpt != theCheckpoints.end()) {
      if (ckpt->second.theHeldPermissions.count(addr) > 0) {
        ckpt->second.theHeldPermissions.erase(addr);
        ckpt->second.theLostPermissionCount++;

        if (ckpt->second.theLostPermissionCount > 5) {
          // Abort if lost permission count exceeds threshold
          if (!theAbortSpeculation || theViolatingInstruction->sequenceNo() >
                                          ckpt->second.theRequiredPermissions[addr]->sequenceNo()) {
            theAbortSpeculation = true;
            theViolatingInstruction = ckpt->second.theRequiredPermissions[addr];
          }
        } else if (!requested) {
          // otherwise, re-request permission
          requestStorePrefetch(ckpt->second.theRequiredPermissions[addr]);
        }
      }
      ++ckpt;
    }
  }
}

void CoreImpl::startSpeculating() {
  if (!isSpeculating()) {
    DBG_(Iface, (<< theName << " Starting Speculation"));
    theIsSpeculating = true;
    theAbortSpeculation = false;
    accountStartSpeculation();
  }
}

void CoreImpl::checkStopSpeculating() {
  if (isSpeculating() && !theAbortSpeculation && theCheckpoints.size() == 0) {
    DBG_(Iface, (<< theName << " Ending Successful Speculation"));
    theIsSpeculating = false;
    accountEndSpeculation();
  } else {
    DBG_(Iface, (<< theName << " continuing speculation checkpoints: " << theCheckpoints.size()
                 << " Abort pending? " << theAbortSpeculation));
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
        theMemQueue.modify(theMemQueue.begin(), ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSB);
      } else if (theMemQueue.front().isLoad()) {
        eraseLSQ(theMemQueue.front().theInstruction);
      }
    }
  }
}

void CoreImpl::invalidate(PhysicalMemoryAddress anAddress) {
  FLEXUS_PROFILE();
  DBG_Assert(static_cast<uint64_t>(anAddress) ==
                 (static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1)),
             (<< "address = " << std::hex << static_cast<uint64_t>(anAddress)));

  // If the processor is speculating, check the SLAT.  On a match, we will
  // abort the speculation, and we can save ourselves the trouble of looking
  // at the rest of theMemQueue
  DBG_(Iface, (<< "Checking invalidation for " << anAddress << ", " << theNode));
  if (isExclusiveLocal(anAddress, kWord) != 2) {
    DBG_(Iface, (<< "Clearing Local " << theNode));
    markExclusiveGlobal(anAddress, kWord, kMonitorUnset);
    markExclusiveLocal(anAddress, kWord, kMonitorUnset);
    markExclusiveVA(VirtualMemoryAddress(anAddress), kWord, kMonitorUnset);
  }
  if (isSpeculating()) {
    SpeculativeLoadAddressTracker::iterator iter, end;
    std::tie(iter, end) = theSLAT.equal_range(anAddress);
    boost::optional<boost::intrusive_ptr<Instruction>> violator =
        boost::make_optional(false, boost::intrusive_ptr<Instruction>());
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
        if ((!temp->status()) == kComplete) {
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

void CoreImpl::addSLATEntry(PhysicalMemoryAddress anAddress,
                            boost::intrusive_ptr<Instruction> anInstruction) {
  DBG_Assert(anAddress != 0);
  DBG_(Verb, (<< theName << " add SLAT: " << anAddress << " for " << *anInstruction));
  theSLAT.insert(std::make_pair(anAddress, anInstruction));
}

void CoreImpl::removeSLATEntry(PhysicalMemoryAddress anAddress,
                               boost::intrusive_ptr<Instruction> anInstruction) {
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

bits CoreImpl::retrieveLoadValue(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);

  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end(), (<< *anInsn));
  DBG_Assert(lsq_entry->status() == kComplete, (<< *lsq_entry));
  return *lsq_entry->theValue;
}

void CoreImpl::setLoadValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end(), (<< *anInsn));
  DBG_Assert(lsq_entry->status() == kComplete, (<< *lsq_entry));
  lsq_entry->theValue = aValue;
}

bits CoreImpl::retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);

  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end(), (<< *anInsn));
  DBG_Assert(lsq_entry->theExtendedValue);
  return *lsq_entry->theExtendedValue;
}

void CoreImpl::resolveVAddr(boost::intrusive_ptr<Instruction> anInsn, VirtualMemoryAddress anAddr) {
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

void CoreImpl::translate(boost::intrusive_ptr<Instruction> anInsn) {

  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());

  TranslationPtr tr(new Translation());
  tr->theVaddr = lsq_entry->theVaddr;
  tr->setData();
  tr->setInstruction(anInsn);

  DBG_(Iface, (<< "Sending Translation Request to MMU: " << *anInsn << ", VAddr: " << tr->theVaddr
               << ", ID: " << tr->theID));

  theTranslationQueue.push(tr);
}

void CoreImpl::resolvePAddr(boost::intrusive_ptr<Instruction> anInsn,
                            PhysicalMemoryAddress anAddr) {

  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
  DBG_Assert(lsq_entry->thePaddr != 0);

  if (lsq_entry->thePaddr == anAddr) {
    CORE_DBG("no update neccessary for " << anAddr);
    return; // No change
  }

  updatePaddr(lsq_entry, anAddr);
  anInsn->setResolved();

  if (anAddr == (PhysicalMemoryAddress)kUnresolved)
    return;

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
    if (lsq_entry->isAtomic()) {
      doLoad(lsq_entry, boost::none);
    }

    break;
  case kAwaitingAddress:
    // This resolve operation cleared the address.  No furter action
    // at this time.
    break;
  }
}

void CoreImpl::updateStoreValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue,
                                boost::optional<bits> anExtendedValue) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
  DBG_Assert(lsq_entry->theOperation != kLoad);
  DBG_(Iface, (<< "Updated store value for " << *lsq_entry << " to " << aValue
               << "[:" << anExtendedValue << "]"));

  boost::optional<bits> previous_value(lsq_entry->theValue);

  lsq_entry->theAnnulled = false;
  lsq_entry->theValue = aValue;
  if (anExtendedValue) {
    lsq_entry->theExtendedValue = anExtendedValue;
  }

  if (previous_value != lsq_entry->theValue) {
    doStore(lsq_entry);
  }
}

void CoreImpl::annulStoreValue(boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
  DBG_Assert(lsq_entry->theOperation != kLoad);
  DBG_(Verb, (<< "Annul store value for " << *lsq_entry));
  if (!lsq_entry->theAnnulled) {
    lsq_entry->theAnnulled = true;
    lsq_entry->theIssued = false;
    resnoopDependantLoads(lsq_entry);
  }
}

void CoreImpl::updateCASValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue,
                              bits aCompareValue) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator lsq_entry = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(lsq_entry != theMemQueue.get<by_insn>().end());
  DBG_Assert(lsq_entry->theOperation == kCAS);

  boost::optional<bits> previous_value(lsq_entry->theValue);

  lsq_entry->theAnnulled = false;
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

  if (previous_value != lsq_entry->theValue) {
    doStore(lsq_entry);
  }
}

void CoreImpl::forwardValue(MemQueueEntry const &aStore,
                            memq_t::index<by_insn>::type::iterator aLoad) {
  FLEXUS_PROFILE();
  if (covers(
          aStore,
          *aLoad) /*&& (aLoad->theASI != 0x24) && (aLoad->theASI != 0x2C)*/) { // QUAD-LDDs are
                                                                               // always considered
                                                                               // partial-snoops.
    if (aLoad->theSize == aStore.theSize) {
      // Sizes match and load covers store - simply copy value
      if (aLoad->theInverseEndian == aStore.theInverseEndian || !aStore.theValue) {
        aLoad->loadValue() = aStore.theValue;
      } else {
        assert(false);
        DBG_(Iface, (<< "Inverse endian forwarding of " << *aStore.theValue << " from " << aStore
                     << " to " << *aLoad << " load value: " << aLoad->loadValue()));
      }
      signalStoreForwardingHit_fn(true);
    } else {
      if (aStore.theValue) {
        if (aLoad->theInverseEndian == aStore.theInverseEndian) {
          writeAligned(*aLoad, align(aStore, false));
          DBG_(Verb, (<< "Forwarded value=" << aLoad->theValue << " to " << *aLoad << " from "
                      << aStore));
        } else {
          writeAligned(*aLoad, align(aStore, true));
          DBG_(Iface, (<< "Inverse endian forwarding of " << *aStore.theValue << " from " << aStore
                       << " to " << *aLoad << " load value: " << aLoad->loadValue()));
        }
        signalStoreForwardingHit_fn(true);
      } else {
        DBG_(Verb, (<< *aLoad << " depends completely on " << aStore
                    << " but value is not yet available"));
        aLoad->loadValue() = boost::none;
      }
    }
    if (aLoad->thePartialSnoop) {
      --thePartialSnoopersOutstanding;
    }
    aLoad->thePartialSnoop = false;
  } else {
    aLoad->loadValue() = boost::none;
    if (!aLoad->thePartialSnoop) {
      ++thePartialSnoopersOutstanding;
      ++thePartialSnoopLoads;
      aLoad->thePartialSnoop = true;
    }
    DBG_(Verb, (<< *aLoad << " depends partially on " << aStore
                << ". Must apply stores when completed."));
  }
}

template <class Iter>
boost::optional<memq_t::index<by_insn>::type::iterator>
CoreImpl::snoopQueue(Iter iter, Iter end, memq_t::index<by_insn>::type::iterator load) {
  FLEXUS_PROFILE();
  boost::reverse_iterator<Iter> r_iter(iter);
  boost::reverse_iterator<Iter> r_end(end);

  while (r_iter != r_end) {

    DBG_(Verb, (<< theName << " " << *load << " snooping " << *r_iter));
    if (r_iter->isStore() && (r_iter->status() != kAnnulled) && intersects(*r_iter, *load)) {
      DBG_Assert(r_iter->theSequenceNum < load->theSequenceNum);
      DBG_(Verb, (<< theName << " " << *load << " forwarding" << *r_iter));
      // See if the store covers the entire load
      forwardValue(*r_iter, load);
      return theMemQueue.get<by_insn>().find(r_iter->theInstruction);
    }
    ++r_iter;
  }
  return theMemQueue.get<by_insn>().end();
}

boost::optional<memq_t::index<by_insn>::type::iterator>
CoreImpl::snoopStores(memq_t::index<by_insn>::type::iterator aLoad,
                      boost::optional<memq_t::index<by_insn>::type::iterator> aCachedSnoopState) {
  FLEXUS_PROFILE();
  DBG_Assert(aLoad->isLoad());
  if (aLoad->isAtomic()) {
    DBG_Assert(theSpeculativeOrder);
  }

  if (aLoad->thePaddr == kUnresolved) {
    return aCachedSnoopState; // can't snoop without a resolved address
  }

  aLoad->loadValue() = boost::none; // assume no match unless we find something

  if (aCachedSnoopState) {
    DBG_(Verb, (<< "cached state in snoopStores."));
    // take advantage of the cached search
    if (*aCachedSnoopState != theMemQueue.get<by_insn>().end()) {
      DBG_(Verb, (<< "Forwarding via cached snoop state: " << **aCachedSnoopState));
      forwardValue(**aCachedSnoopState, aLoad);
    }
  } else {
    memq_t::index<by_paddr>::type::iterator load = theMemQueue.project<by_paddr>(aLoad);
    memq_t::index<by_paddr>::type::iterator oldest_match =
        theMemQueue.get<by_paddr>().lower_bound(std::make_tuple(aLoad->thePaddr_aligned));
    aCachedSnoopState = snoopQueue(load, oldest_match, aLoad);
  }

  return aCachedSnoopState;
}

void CoreImpl::updateDependantLoads(memq_t::index<by_insn>::type::iterator anUpdatedStore) {
  FLEXUS_PROFILE();
  if (anUpdatedStore->thePaddr == kUnresolved || anUpdatedStore->thePaddr == kInvalid) {
    CORE_DBG("anUpdatedStore->thePaddr == kUnresolved || "
             "anUpdatedStore->thePaddr == kInvalid");
    return;
  }

  DBG_(Verb, (<< "Updating loads dependant on " << *anUpdatedStore));
  CORE_DBG("Updating loads dependant on " << *anUpdatedStore);

  memq_t::index<by_paddr>::type::iterator updated_store =
      theMemQueue.project<by_paddr>(anUpdatedStore);
  memq_t::index<by_paddr>::type::iterator iter, entry, last_match;
  last_match =
      theMemQueue.get<by_paddr>().upper_bound(std::make_tuple(updated_store->thePaddr_aligned));

  // Loads with higher sequence numbers than anUpdatedStore must be squashed and
  // obtain their new value
  boost::optional<memq_t::index<by_insn>::type::iterator> cached_search =
      boost::make_optional(false, memq_t::index<by_insn>::type::iterator());
  iter = updated_store;
  ++iter;
  while (iter != last_match) {
    entry = iter;
    ++iter;
    uint64_t seq_no = iter->theSequenceNum;
    DBG_Assert(entry->theSequenceNum > updated_store->theSequenceNum,
               (<< " entry: " << *entry << " updated_store: " << *updated_store));
    if (entry->isLoad() && intersects(*updated_store, *entry)) {
      CORE_DBG("Loads of overlapping address must re-snoop");

      // Loads of overlapping address must re-snoop
      bool research = (iter != last_match);
      cached_search = doLoad(theMemQueue.project<by_insn>(entry), cached_search);
      if (research) {
        iter = theMemQueue.get<by_paddr>().lower_bound(
            std::make_tuple(updated_store->thePaddr_aligned, seq_no));
        last_match = theMemQueue.get<by_paddr>().upper_bound(
            std::make_tuple(updated_store->thePaddr_aligned));
      }
    } else if (entry->isStore() && (entry->status() != kAnnulled) &&
               intersects(*updated_store, *entry)) {
      CORE_DBG("Search terminated at " << *entry);
      DBG_(Verb, (<< "Search terminated at " << *entry));
      break; // Stop on a subsequent store which intersects this store.  Loads
             // past this point will not change outcome as a result of this
             // store
    }
  }
  DBG_(Verb, (<< "Search complete."));
  CORE_DBG("Search complete.");
}

void CoreImpl::applyStores(memq_t::index<by_paddr>::type::iterator aFirstStore,
                           memq_t::index<by_paddr>::type::iterator aLastStore,
                           memq_t::index<by_insn>::type::iterator aLoad) {
  FLEXUS_PROFILE();
  while (aFirstStore != aLastStore) {
    if (aFirstStore->isStore() && aFirstStore->theValue && (aFirstStore->status() != kAnnulled)) {
      if (intersects(*aFirstStore, *aLoad)) {
        DBG_(Verb, (<< theName << " Applying " << *aFirstStore << " to " << *aLoad));
        overlay(*aFirstStore, *aLoad);
      }
    }
    ++aFirstStore;
  }
}

void CoreImpl::applyAllStores(memq_t::index<by_insn>::type::iterator aLoad) {
  FLEXUS_PROFILE();
  DBG_(Verb, (<< theName << " Partial snoop applying stores: " << *aLoad));
  // Need to apply every overlapping store to the value of this load.
  memq_t::index<by_paddr>::type::iterator load = theMemQueue.project<by_paddr>(aLoad);
  memq_t::index<by_paddr>::type::iterator oldest_match =
      theMemQueue.get<by_paddr>().lower_bound(std::make_tuple(aLoad->thePaddr_aligned));
  applyStores(oldest_match, load, aLoad);
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

void CoreImpl::valuePredictAtomic() {
  FLEXUS_PROFILE();
  if (theSpeculateOnAtomicValue && theLSQCount > 0) {
    memq_t::index<by_queue>::type::iterator lsq_head =
        theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
    if ((lsq_head->isAtomic()) && (!lsq_head->isAbnormalAccess()) &&
        (!lsq_head->theExtendedValue) && (!lsq_head->thePartialSnoop) &&
        ((lsq_head->theOperation == kCAS) ? (!!lsq_head->theCompareValue) : true) &&
        (lsq_head->theInstruction == theROB.front()) &&
        (!theValuePredictInhibit) // Disables value prediction for one
                                  // instruction after a rollback
        && (lsq_head->status() == kIssuedToMemory)) {
      DBG_(Iface, (<< theName << " Value-predicting " << *lsq_head));
      ++theValuePredictions;

      if (theSpeculateOnAtomicValuePerfect) {
        lsq_head->theExtendedValue = ValueTracker::valueTracker(theNode).load(
            theNode, lsq_head->thePaddr, lsq_head->theSize);
      } else {
        if (lsq_head->theOperation == kCAS) {
          lsq_head->theExtendedValue = lsq_head->theCompareValue;
        } else {
          lsq_head->theExtendedValue = 0;
        }
      }
      if (lsq_head->theDependance) {
        lsq_head->theDependance->satisfy();
      }
      lsq_head->theSpeculatedValue = true;
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

void CoreImpl::doStore(memq_t::index<by_insn>::type::iterator lsq_entry) {
  FLEXUS_PROFILE();
  DBG_Assert(lsq_entry->isStore());
  CORE_TRACE;
  // Also used by atomics when their value or address changes
  updateDependantLoads(lsq_entry);
}

void CoreImpl::resnoopDependantLoads(memq_t::index<by_insn>::type::iterator lsq_entry) {
  FLEXUS_PROFILE();
  // Also used by atomics when their value or address changes
  DBG_Assert(lsq_entry->theOperation != kLoad);

  // Changing the address or annulling a store.  Need to squash all loads which
  // got values from this store.
  if (lsq_entry->thePaddr != kInvalid && lsq_entry->thePaddr != kUnresolved) {
    bool was_annulled = lsq_entry->theAnnulled;
    lsq_entry->theAnnulled = true;
    updateDependantLoads(lsq_entry);
    lsq_entry->theAnnulled = was_annulled;
  }
}

boost::optional<memq_t::index<by_insn>::type::iterator>
CoreImpl::doLoad(memq_t::index<by_insn>::type::iterator lsq_entry,
                 boost::optional<memq_t::index<by_insn>::type::iterator> aCachedSnoopState) {
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
    lsq_entry->theIssued = false;
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
    previous_value = lsq_entry->loadValue();
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
      previous_value = lsq_entry->loadValue();
      lsq_entry->theIssued = false;
    }
    // No need to cancel anything.  If the load ends up getting a value via
    // forwarding, its port request will be discarded
    break;

  case kAwaitingAddress:
    // Once the load gets its address, doLoad will be called again
    return aCachedSnoopState;

  default:
    DBG_Assert(false,
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
      DBG_(Verb, (<< theName << "previous_value: " << previous_value
                  << " loadValue(): " << lsq_entry->loadValue()));
      lsq_entry->theIssued = false;
      lsq_entry->theInstruction->setTransactionTracker(
          0); // Remove any transaction tracker since we changed load value
      DBG_Assert(lsq_entry->theDependance);
      if (previous_value) {
        lsq_entry->theDependance->squash();
      }
      if (lsq_entry->loadValue()) {
        lsq_entry->theDependance->satisfy();
      } else {
        DBG_Assert(
            lsq_entry->isAtomic(),
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

void CoreImpl::updateVaddr(memq_t::index<by_insn>::type::iterator lsq_entry,
                           VirtualMemoryAddress anAddr) {
  FLEXUS_PROFILE();
  lsq_entry->theVaddr = anAddr;
  DBG_(VVerb, (<< "in updateVaddr")); // NOOOSHIN
  lsq_entry->thePaddr = PhysicalMemoryAddress(kUnresolved);
  // theMemQueue.get<by_insn>().modify( lsq_entry, [](auto& x){
  // x.thePaddr_aligned = PhysicalMemoryAddress(kUnresolved);});//ll::bind(
  // &MemQueueEntry::thePaddr_aligned, ll::_1 ) =
  // PhysicalMemoryAddress(kUnresolved));
  theMemQueue.get<by_insn>().modify(lsq_entry, ll::bind(&MemQueueEntry::thePaddr_aligned, ll::_1) =
                                                   PhysicalMemoryAddress(kUnresolved));
}

void CoreImpl::updatePaddr(memq_t::index<by_insn>::type::iterator lsq_entry,
                           PhysicalMemoryAddress anAddr) {
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
  theMemQueue.get<by_insn>().modify(lsq_entry, ll::bind(&MemQueueEntry::thePaddr_aligned, ll::_1) =
                                                   addr_aligned);

  if (thePrefetchEarly && lsq_entry->isStore() && lsq_entry->thePaddr != kUnresolved &&
      lsq_entry->thePaddr != 0 && !lsq_entry->isAbnormalAccess()) {
    requestStorePrefetch(lsq_entry);
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

bool CoreImpl::scanMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry) {
  FLEXUS_PROFILE();
  if (!anLSQEntry->thePaddr) {
    return false;
  }
  // Check for an existing MSHR for the same address (issued this cycle)
  MSHRs_t::iterator existing = theMSHRs.find(anLSQEntry->thePaddr);
  return (existing != theMSHRs.end());
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

  tracker->setSource("uArchARM");
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

void MemoryPortArbiter::requestStorePrefetch(memq_t::index<by_insn>::type::iterator lsq_entry) {
  theStorePrefetchRequests.insert(
      StorePrefetchRequest(lsq_entry->theSequenceNum, lsq_entry->theInstruction));
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
    if (lsq_entry->theCompareValue == (bits)kAlwaysStore) {
      lsq_entry->theExtendedValue = lsq_entry->theCompareValue;
    }
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

void CoreImpl::printROB() {
  std::cout << theName << " *** ROB Contents ***" << std::endl;
  if (!theROB.empty()) {
    rob_t::iterator iter, end;
    for (iter = theROB.begin(), end = theROB.end(); iter != end; ++iter) {
      //   std::cout << **iter << " NPC=" << (*iter)->npc() << std::endl;
    }
  }
}

void CoreImpl::printSRB() {
  std::cout << theName << " *** SRB Contents ***" << std::endl;
  if (!theSRB.empty()) {
    rob_t::iterator iter, end;
    for (iter = theSRB.begin(), end = theSRB.end(); iter != end; ++iter) {
      std::cout << **iter << std::endl;
    }
  }
}

void CoreImpl::printMemQueue() {
  std::cout << theName << " *** MemQueue Contents ***" << std::endl;
  if (!theMemQueue.empty()) {
    memq_t::iterator iter, end;
    for (iter = theMemQueue.begin(), end = theMemQueue.end(); iter != end; ++iter) {
      std::cout << *iter << std::endl;
    }
  }
  if (theMemQueue.front().theInstruction->hasCheckpoint()) {
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt =
        theCheckpoints.find(theMemQueue.front().theInstruction);
    std::cout << theName << " *** Head Checkpoint ***" << std::endl;
    std::cout << "  Lost Permissions: " << ckpt->second.theLostPermissionCount << std::endl;
    std::cout << "  Required Addresses" << std::endl;
    std::map<PhysicalMemoryAddress, boost::intrusive_ptr<Instruction>>::iterator req =
        ckpt->second.theRequiredPermissions.begin();
    while (req != ckpt->second.theRequiredPermissions.end()) {
      std::cout << "        " << req->first << "\t" << *req->second << std::endl;
      ++req;
    }
    std::cout << "  Held Addresses " << std::endl;
    std::set<PhysicalMemoryAddress>::iterator held = ckpt->second.theHeldPermissions.begin();
    while (held != ckpt->second.theHeldPermissions.end()) {
      std::cout << "        " << *held << std::endl;
      ++held;
    }
  }
}

void CoreImpl::printMSHR() {
  std::cout << theName << " *** MSHR Contents *** " << std::endl;
  if (!theMSHRs.empty()) {
    MSHRs_t::iterator iter, end;
    for (iter = theMSHRs.begin(), end = theMSHRs.end(); iter != end; ++iter) {
      std::cout << " " << iter->second << std::endl;
    }
  }
  std::cout << theName << " *** Outstanding prefetches *** " << std::endl;
  if (!theOutstandingStorePrefetches.empty()) {
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter,
        end;
    for (iter = theOutstandingStorePrefetches.begin(), end = theOutstandingStorePrefetches.end();
         iter != end; ++iter) {
      std::cout << "   " << iter->first << std::endl;
      std::set<boost::intrusive_ptr<Instruction>>::iterator insn_iter, insn_end;
      for (insn_iter = iter->second.begin(), insn_end = iter->second.end(); insn_iter != insn_end;
           ++insn_iter) {
        std::cout << "         " << **insn_iter << std::endl;
      }
    }
  }
#ifdef VALIDATE_STORE_PREFETCHING
  std::cout << theName << " *** Waiting prefetches *** " << std::endl;
  if (!theWaitingStorePrefetches.empty()) {
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter,
        end;
    for (iter = theWaitingStorePrefetches.begin(), end = theWaitingStorePrefetches.end();
         iter != end; ++iter) {
      std::cout << "   " << iter->first << std::endl;
      std::set<boost::intrusive_ptr<Instruction>>::iterator insn_iter, insn_end;
      for (insn_iter = iter->second.begin(), insn_end = iter->second.end(); insn_iter != insn_end;
           ++insn_iter) {
        std::cout << "         " << **insn_iter << std::endl;
      }
    }
  }
  std::cout << theName << " *** Blocked prefetches *** " << std::endl;
  if (!theBlockedStorePrefetches.empty()) {
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter,
        end;
    for (iter = theBlockedStorePrefetches.begin(), end = theBlockedStorePrefetches.end();
         iter != end; ++iter) {
      std::cout << "   " << iter->first << std::endl;
      std::set<boost::intrusive_ptr<Instruction>>::iterator insn_iter, insn_end;
      for (insn_iter = iter->second.begin(), insn_end = iter->second.end(); insn_iter != insn_end;
           ++insn_iter) {
        std::cout << "         " << **insn_iter << std::endl;
      }
    }
  }
#endif // VALIDATE_STORE_PREFETCHING
  std::cout << theName << " *** Prefetch Queue *** " << std::endl;
  if (!theMemoryPortArbiter.theStorePrefetchRequests.empty()) {
    prefetch_queue_t::iterator iter, end;
    for (iter = theMemoryPortArbiter.theStorePrefetchRequests.begin(),
        end = theMemoryPortArbiter.theStorePrefetchRequests.end();
         iter != end; ++iter) {
      std::cout << "   " << iter->theAge << "\t" << *iter->theInstruction << std::endl;
    }
  }
}

void CoreImpl::printRegMappings(std::string aRegSet) {
  if (aRegSet == "r") {
    std::cout << theName << " *** r register mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpMappings(std::cout);
  } else if (aRegSet == "f") {
    std::cout << theName << " *** f register mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpMappings(std::cout);
  } else if (aRegSet == "ccr") {
    std::cout << theName << " *** ccr register mappings *** " << std::endl;
    //    theMapTables[ccBits]->dumpMappings(std::cout);
  } else if (aRegSet == "all") {
    std::cout << theName << " *** r register mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpMappings(std::cout);
    std::cout << theName << " *** f register mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpMappings(std::cout);
    std::cout << theName << " *** ccr register mappings *** " << std::endl;
    //    theMapTables[ccBits]->dumpMappings(std::cout);
  } else {
    std::cout << "Unknown register set " << aRegSet << std::endl;
  }
}

void CoreImpl::printRegFreeList(std::string aRegSet) {
  if (aRegSet == "r") {
    std::cout << theName << " *** r register free list *** " << std::endl;
    //    theMapTables[xRegisters]->dumpFreeList(std::cout);
  } else if (aRegSet == "f") {
    std::cout << theName << " *** f register free list *** " << std::endl;
    //    theMapTables[xRegisters]->dumpFreeList(std::cout);
  } else if (aRegSet == "ccr") {
    std::cout << theName << " *** ccr register free list *** " << std::endl;
    //    theMapTables[ccBits]->dumpFreeList(std::cout);
  } else if (aRegSet == "all") {
    std::cout << theName << " *** r register free list *** " << std::endl;
    //    theMapTables[xRegisters]->dumpFreeList(std::cout);
    std::cout << theName << " *** f register free list *** " << std::endl;
    //    theMapTables[xRegisters]->dumpFreeList(std::cout);
    std::cout << theName << " *** ccr register free list *** " << std::endl;
    //    theMapTables[ccBits]->dumpFreeList(std::cout);
  } else {
    std::cout << "Unknown register set " << aRegSet << std::endl;
  }
}

void CoreImpl::printRegReverseMappings(std::string aRegSet) {
  if (aRegSet == "r") {
    std::cout << theName << " *** r register reverse mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
  } else if (aRegSet == "f") {
    std::cout << theName << " *** f register reverse mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
  } else if (aRegSet == "ccr") {
    std::cout << theName << " *** ccr register reverse mappings *** " << std::endl;
    //    theMapTables[ccBits]->dumpReverseMappings(std::cout);
  } else if (aRegSet == "all") {
    std::cout << theName << " *** r register reverse mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
    std::cout << theName << " *** f register reverse mappings *** " << std::endl;
    //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
    std::cout << theName << " *** ccr register reverse mappings *** " << std::endl;
    //    theMapTables[ccBits]->dumpReverseMappings(std::cout);
  } else {
    std::cout << "Unknown register set " << aRegSet << std::endl;
  }
}

void CoreImpl::printAssignments(std::string aRegSet) {
  if (aRegSet == "r") {
    std::cout << theName << " *** r register assignments *** " << std::endl;
    //    theMapTables[xRegisters]->dumpAssignments(std::cout);
  } else if (aRegSet == "f") {
    std::cout << theName << " *** f register assignments *** " << std::endl;
    //    theMapTables[xRegisters]->dumpAssignments(std::cout);
  } else if (aRegSet == "ccr") {
    std::cout << theName << " *** ccr register assignments *** " << std::endl;
    //    theMapTables[ccBits]->dumpAssignments(std::cout);
  } else if (aRegSet == "all") {
    std::cout << theName << " *** r register assignments *** " << std::endl;
    //    theMapTables[xRegisters]->dumpAssignments(std::cout);
    std::cout << theName << " *** f register assignments *** " << std::endl;
    //    theMapTables[xRegisters]->dumpAssignments(std::cout);
    std::cout << theName << " *** ccr register assignments *** " << std::endl;
    //    theMapTables[ccBits]->dumpAssignments(std::cout);
  } else {
    std::cout << "Unknown register set " << aRegSet << std::endl;
  }
}

void CoreImpl::dumpROB() {
  if (!theROB.empty()) {
    DBG_(VVerb, (<< theName << "*** ROB Contents ***"));
    rob_t::iterator iter, end;
    for (iter = theROB.begin(), end = theROB.end(); iter != end; ++iter) {
      DBG_(VVerb, (/*<< std::internal*/ << **iter));
    }
  }
}

void CoreImpl::dumpSRB() {
  if (!theSRB.empty()) {
    DBG_(VVerb, (<< theName << "*** SRB Contents ***"));
    rob_t::iterator iter, end;
    for (iter = theSRB.begin(), end = theSRB.end(); iter != end; ++iter) {
      DBG_(VVerb, (/*<< std::internal*/ << **iter));
    }
  }
}

void CoreImpl::dumpMemQueue() {
  if (!theMemQueue.empty()) {
    DBG_(VVerb, (<< theName << "*** MemQueue Contents ***"));
    memq_t::iterator iter, end;
    for (iter = theMemQueue.begin(), end = theMemQueue.end(); iter != end; ++iter) {
      DBG_(VVerb, (<< *iter));
    }
  }
}

void CoreImpl::dumpMSHR() {
  if (!theMSHRs.empty()) {
    DBG_(VVerb, (<< theName << "*** MSHR Contents *** " << theMSHRs.size()));
    MSHRs_t::iterator iter, end;
    for (iter = theMSHRs.begin(), end = theMSHRs.end(); iter != end; ++iter) {
      DBG_(VVerb, (<< " " << iter->second));
    }
  }
}

void CoreImpl::dumpActions() {

  //    action_list_t::iterator iter, end;

  //    if ( ! theRescheduledActions.empty() ) {
  //      DBG_( VVerb, ( << " *** Rescheduled SemanticActions ***" ) );
  //      for ( iter = theRescheduledActions.begin(), end =
  //      theRescheduledActions.end(); iter != end; ++iter) {
  //        DBG_( VVerb, ( << **iter ) );
  //      }
  //    }
}

void CoreImpl::dumpCheckpoints() {
  if (!theCheckpoints.empty()) {
    DBG_(VVerb, (<< theName << "*** Checkpoints ***"));
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator iter, end;
    for (iter = theCheckpoints.begin(), end = theCheckpoints.end(); iter != end; ++iter) {
      DBG_(VVerb, (<< "   " << *iter->first));
      DBG_(VVerb, (<< "   Lost Permissions: " << iter->second.theLostPermissionCount));
      DBG_(VVerb, (<< "   Required Addresses"));
      std::map<PhysicalMemoryAddress, boost::intrusive_ptr<Instruction>>::iterator req =
          iter->second.theRequiredPermissions.begin();
      while (req != iter->second.theRequiredPermissions.end()) {
        DBG_(VVerb, (<< "        " << req->first << "\t" << *req->second));
        ++req;
      }
      DBG_(VVerb, (<< "  Held Addresses "));
      std::set<PhysicalMemoryAddress>::iterator held = iter->second.theHeldPermissions.begin();
      while (held != iter->second.theHeldPermissions.end()) {
        DBG_(VVerb, (<< "        " << *held));
        ++held;
      }
    }
  }
}

void CoreImpl::dumpSBPermissions() {
  if (!theSBLines_Permission.empty()) {
    DBG_(VVerb, (<< theName << "*** SB Line Tracker ***"));
    std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator iter, end;
    for (iter = theSBLines_Permission.begin(), end = theSBLines_Permission.end(); iter != end;
         ++iter) {
      DBG_(VVerb, (<< "   " << iter->first << " SB: " << iter->second.first
                   << " onchip: " << iter->second.second));
    }
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

// read physical register
CoreModel* CoreModel::construct(uArchOptions_t options,
                                std::function<int(bool)> advance,
                                std::function<void(eSquashCause)> squash,
                                std::function<void(VirtualMemoryAddress)> redirect,
                                std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
                                std::function<void(bool)> signalStoreForwardingHit,
                                std::function<void(int32_t)> mmuResync) {

  return new CoreImpl(options,
                      advance, squash, redirect, feedback, signalStoreForwardingHit,
                      mmuResync);
}




std::ostream &operator<<(std::ostream &anOstream, eExceptionType aCode) {
  std::string exceptionTypes[] = {
      "Exception_UNCATEGORIZED         ", "Exception_WFX_TRAP              ",
      "Exception_CP15RTTRAP            ", "Exception_CP15RRTTRAP           ",
      "Exception_CP14RTTRAP            ", "Exception_CP14DTTRAP            ",
      "Exception_ADVSIMDFPACCESSTRAP   ", "Exception_FPIDTRAP              ",
      "Exception_CP14RRTTRAP           ", "Exception_ILLEGALSTATE          ",
      "Exception_AA32_SVC              ", "Exception_AA32_HVC              ",
      "Exception_AA32_SMC              ", "Exception_AA64_SVC              ",
      "Exception_AA64_HVC              ", "Exception_AA64_SMC              ",
      "Exception_SYSTEMREGISTERTRAP    ", "Exception_INSNABORT             ",
      "Exception_INSNABORT_SAME_EL     ", "Exception_PCALIGNMENT           ",
      "Exception_DATAABORT             ", "Exception_DATAABORT_SAME_EL     ",
      "Exception_SPALIGNMENT           ", "Exception_AA32_FPTRAP           ",
      "Exception_AA64_FPTRAP           ", "Exception_SERROR                ",
      "Exception_BREAKPOINT            ", "Exception_BREAKPOINT_SAME_EL    ",
      "Exception_SOFTWARESTEP          ", "Exception_SOFTWARESTEP_SAME_EL  ",
      "Exception_WATCHPOINT            ", "Exception_WATCHPOINT_SAME_EL    ",
      "Exception_AA32_BKPT             ", "Exception_VECTORCATCH           ",
      "Exception_AA64_BKPT             ", "Exception_IRQ",
      "Exception_None                  "};

  if (aCode >= kException_None) {
    anOstream << "InvalidExceptionType(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << exceptionTypes[aCode];
  }
  return anOstream;
}

} // namespace nuArch
