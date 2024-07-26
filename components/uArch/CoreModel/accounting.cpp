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

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

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
    } else {
      // DBG_(Dev, ( << " Unknown access address for " << *inst) );
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
    } else {
      // DBG_(Dev, ( << " Unknown access address for " << *inst) );
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
    // DBG_(VVerb, (<<"A raised happend!"));
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
          // DBG_( Crit, ( << "Unknown store local stall cycles. Invalid fill
          // type: " << *tracker->fillType() ));
          break;
        }
      } else {
        // DBG_( Crit, ( << "Unknown store local stall cycles. Missing fill type
        // " ));
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
              // DBG_( Crit, ( << "Unknown store remote coherence stall cycles.
              // Invalid previous state: " << *tracker->previousState() ));
              break;
            }
          } else {
            // DBG_( Crit, ( << "Unknown store remote coherence stall cycles.
            // Missing previous state " ));
          }
          break;
        default:
          // DBG_( Crit, ( << "Unknown store remote stall cycles. Invalid fill
          // type: " << *tracker->fillType() ));
          break;
        }
      } else {
        // DBG_( Crit, ( << "Unknown store remote stall cycles. Missing fill
        // type" ));
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
      // DBG_( Crit, ( << "Unknown store stall cycles. Invalid level: " <<
      // *tracker->fillLevel() ));
      break;
    }
  } else {
    // DBG_( Crit, ( << "Unknown store stall cycles. Missing Fill level " ));
  }
  return theStallType;
}

void CoreImpl::chargeStoreStall(boost::intrusive_ptr<Instruction> inst,
                                boost::intrusive_ptr<TransactionTracker> tracker) {
  nXactTimeBreakdown::eCycleClass theStallType = getStoreStallType(tracker);
  int32_t stall_cycles = theTimeBreakdown.stall(theStallType);
  inst->setRetireStallCycles(stall_cycles);
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

  theIsIdle = Flexus::Qemu::Processor::getProcessor(theNode).is_busy() ? false : true;

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

void CoreImpl::accountStartSpeculation() {
  ++theSpeculations_Started;
}

void CoreImpl::accountEndSpeculation() {
  ++theSpeculations_Successful;
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

void CoreImpl::accountResyncSpeculation() {
  theTimeBreakdown.modifyAndApplyAllTransactions(nXactTimeBreakdown::kSyncWhileSpeculating,
                                                 nXactTimeBreakdown::eDiscard_Resync);
  ++theSpeculations_Resync;
}

void CoreImpl::accountResync() {
  theTimeBreakdown.modifyAndApplyAllTransactions(nXactTimeBreakdown::kSyncPipe,
                                                 nXactTimeBreakdown::eDiscard_Resync);
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
          // theTimeBreakdown.stall(nXactTimeBreakdown::kEmptyROB_Mispredict);
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
        // theTimeBreakdown.stall(nXactTimeBreakdown::kStore_BufferFull);

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
          } /*else if (iter->theMMU) {
            theTimeBreakdown.stall(nXactTimeBreakdown::kMMUAccess);
          }*/
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

void CoreImpl::skipCycle() {
  theTimeBreakdown.skipCycle();
}

} // namespace nuArch