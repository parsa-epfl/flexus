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

} // namespace nuArch
