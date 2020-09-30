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

#include <components/armDecoder/SemanticInstruction.hpp>
#include <components/armDecoder/armDecoder.hpp>
#include <components/CommonQEMU/MessageQueues.hpp>
#include <map>

#define FLEXUS_BEGIN_COMPONENT armDecoder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

#include <boost/weak_ptr.hpp>
#include <components/MTManager/MTManager.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/MTManager/MTManager.hpp>

namespace ll = boost::lambda;

namespace narmDecoder {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using namespace nMessageQueues;

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
       int32_t aUop);

class FLEXUS_COMPONENT(armDecoder) {
  FLEXUS_COMPONENT_IMPL(armDecoder);

  eInstructionCode lastInsnCode;
  int64_t theInsnSequenceNo;
  boost::intrusive_ptr<BPredState> lastBPState;
  std::list<boost::intrusive_ptr<AbstractInstruction>> theFIQ;

  Stat::StatCounter theCallRestorePair;
  bool theSyncInsnInProgress;

  typedef PipelineFifo<boost::intrusive_ptr<AbstractInstruction>> CorePipeline;
  CorePipeline thePipeline;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(armDecoder)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS), theCallRestorePair(statName() + "-CallRestorePair"),
        thePipeline(statName() + "-Pipeline", cfg.DispatchWidth, 1, cfg.PipeLat) {
  }

  bool isQuiesced() const {
    // the FAG is always quiesced.
    return theFIQ.empty() && thePipeline.empty() && !theSyncInsnInProgress;
  }

  void initialize() {
    lastBPState = nullptr;
    theInsnSequenceNo = 0;
    theSyncInsnInProgress = false;
  }

  void finalize() {
  }

public:
  FLEXUS_PORT_ALWAYS_AVAILABLE(FetchBundleIn);
  void push(interface::FetchBundleIn const &, pFetchBundle &aBundle) {
    std::list<FetchedOpcode>::iterator iter = aBundle->theOpcodes.begin();

    std::list<tFillLevel>::iterator fill_iter = aBundle->theFillLevels.begin();
    for (auto it = aBundle->theOpcodes.begin(); it != aBundle->theOpcodes.end(); it++) {
      int32_t uop = 0;
      boost::intrusive_ptr<AbstractInstruction> insn;
      bool final_uop = false;
      // Note that multi-uop instructions can cause theFIQ to fill beyond its
      // configured size.
      while (!final_uop) {
        boost::tie(insn, final_uop) = decode(*it, aBundle->coreID, ++theInsnSequenceNo, uop++);
        insn->fetchSerial() = final_uop ? iter->theSerial : 0;
        if (insn) {
          insn->setFetchTransactionTracker(it->theTransaction);
          insn->setMissStatsInfo(iter->missStatsInfo);
          /*insn->sethadIcacheMiss(iter->thehadIcacheMiss);
            insn->setMissPrefetchOnWay(iter->theMissPrefetchOnWay);
            insn->setIcacheMissCycles(iter->theIcacheMissCycles);*/

          eInstructionCode insnCode = (dynamic_cast<armInstruction *>(insn.get()))->instCode();
          if ((lastInsnCode == codeCALL) /*&& lastBPState->callUpdatedRAS == true*/) {
            trackSpecialCall(insnCode, (*iter).theOpcode);
          }

          lastInsnCode = insnCode;
          lastBPState = (*iter).theBPState;
          // Set Fill Level for the insn
          insn->setSourceLevel(*fill_iter);
          theFIQ.push_back(insn);
        } else
          DBG_(VVerb, (<< "No INSTRUCTION"));
      }
      ++fill_iter;
    }
  }

  void trackSpecialCall(eInstructionCode insnCode, Opcode theOpcode) {
    // FIXME: PORT TO ARM
#if 0
      if (insnCode == codeBranchJmpl) {
  		theCallRestorePair++;
  		lastBPState->detectedSpecialCall = true;
//        		DBG_(Tmp, ( << "Decoding a special call " << (*iter).thePC << " " << Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble((*iter).thePC)));
//  		DBG_(Tmp, ( << "Decoding a special call "));
      } else {
          uint32_t opcode = theOpcode;
          uint32_t op = opcode >> 30;
          if (op == 2) {
          	uint32_t op3 = (opcode >> 19) & 0x3F;
          	if (op3 == 0x2 || op3 == 0x12) {
          		uint32_t rd = (opcode >> 25) & 0x1F;
          		if (rd == 15) {
              		theCallRestorePair++;
              		lastBPState->detectedSpecialCall = true;
//                    		DBG_(Tmp, ( << "Decoding a special call " << (*iter).thePC << " " << Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble((*iter).thePC)));
//              		DBG_(Tmp, ( << "Decoding a special call "));
          		}
          	}
          }
      }
#endif
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(RASOpsIn);
  void push(interface::RASOpsIn const &, std::list<boost::intrusive_ptr<BPredState>> &theRASops) {
    //	  DBG_( Tmp, ( << " reconstruct RAS Decode" ));

    for (uint32_t i = 0; i < thePipeline.size(); i++) {
      boost::intrusive_ptr<AbstractInstruction> inst = thePipeline.peek();
      boost::intrusive_ptr<BPredState> bpState =
          (dynamic_cast<armInstruction *>(inst.get()))->bpState();
      thePipeline.dequeue();

      if (bpState->thePredictedType == kCall || bpState->thePredictedType == kIndirectCall ||
          bpState->thePredictedType == kIndirectReg || bpState->thePredictedType == kReturn) {
        //	    	  DBG_( Tmp, ( << " RAS Decode " << bpState->pc << " pred type " <<
        // bpState->thePredictedType << " " <<
        // Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble(bpState->pc)));
        theRASops.push_front(bpState);
      }
    }

    for (std::list<boost::intrusive_ptr<AbstractInstruction>>::iterator it = theFIQ.begin();
         it != theFIQ.end(); it++) {
      //		  boost::intrusive_ptr< AbstractInstruction > inst(*it);
      boost::intrusive_ptr<BPredState> bpState =
          (dynamic_cast<armInstruction *>((*it).get()))->bpState();
      if (bpState->thePredictedType == kCall || bpState->thePredictedType == kIndirectCall ||
          bpState->thePredictedType == kIndirectReg || bpState->thePredictedType == kReturn) {
        //	    	  DBG_( Tmp, ( << " RAS Decode " << bpState->pc << " pred type " <<
        // bpState->thePredictedType << " " <<
        // Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble(bpState->pc)));
        theRASops.push_front(bpState);
      }
    }

    FLEXUS_CHANNEL(RASOpsOut) << theRASops;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableFIQOut);
  dispatch_status pull(interface::AvailableFIQOut const &) {
    int32_t avail = cfg.FIQSize - theFIQ.size();
    if (avail < 0) {
      avail = 0;
    }
    return std::make_pair(avail, (theFIQ.empty() && thePipeline.empty()));
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &) {
    return theFIQ.size() + thePipeline.size();
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &) {
    int32_t available_dispatch = 0;
    bool is_sync = false;
    std::pair<int, bool> dispatch_state;
    FLEXUS_CHANNEL(AvailableDispatchIn) >> dispatch_state;
    boost::tie(available_dispatch, is_sync) = dispatch_state;

    return (theFIQ.empty() || (available_dispatch == 0) || (theSyncInsnInProgress && !is_sync));
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(SquashIn);
  void push(interface::SquashIn const &, eSquashCause &aReason) {
    DBG_(VVerb, Comp(*this)(<< "DISPATCH SQUASH: " << aReason
                            << " FIQ discarding: " << theFIQ.size() << " instructions"));
    theFIQ.clear();
    thePipeline.clear();

    FLEXUS_CHANNEL(SquashOut) << aReason;
  }

public:
  void drive(interface::DecoderDrive const &) {
    // if ((flexusIndex() < (cfg.CoreOffset + cfg.NumCores)) && (flexusIndex()
    // >= cfg.CoreOffset)){ Implementation is in the doFetch() member below
    if (cfg.Multithread) {
      if (nMTManager::MTManager::get()->runThisD(flexusIndex())) {
        doDecode();
      }
    } else {
      doDecode();
    }
    //}
  }

private:
  // Implementation of the FetchDrive drive interface
  void doDecode() {
    DISPATCH_DBG("--------------START DISPATCHING------------------------");
    /* Delay pipe for Boomerang */
    uint64_t decoded = 0;
    while (decoded < cfg.DispatchWidth && !theFIQ.empty() &&
           (thePipeline.size() < (cfg.DispatchWidth * cfg.PipeLat))) {

      boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
      theFIQ.pop_front();
      thePipeline.enqueue(inst);

      decoded++;
    }

    DBG_Assert(FLEXUS_CHANNEL(AvailableDispatchIn).available());
    // the FLEXUS_CHANNEL can only write to an lvalue, hence this rather
    // ugly construction.
    int32_t available_dispatch = 0;
    bool is_sync = false;
    std::pair<int, bool> dispatch_state;
    FLEXUS_CHANNEL(AvailableDispatchIn) >> dispatch_state;
    boost::tie(available_dispatch, is_sync) = dispatch_state;

    int64_t theOpcode;

    if (is_sync) {
      // If we had a synchronized instruction in progress, it has completed
      theSyncInsnInProgress = false;
    }
    uint32_t dispatched = 0;

    if (!(available_dispatch > 0 && dispatched < cfg.DispatchWidth && !theFIQ.empty() &&
          !theSyncInsnInProgress)) {
      DISPATCH_DBG("Can't dispatch "
                   << "available_dispatch " << available_dispatch
                   << ", dispatched < cfg.DispatchWidth " << int(dispatched < cfg.DispatchWidth)
                   << ", theFIQ is empty " << int(theFIQ.empty()) << ", no Sync Insn In Progress "
                   << int(!theSyncInsnInProgress));
    }
    while (available_dispatch > 0 && dispatched < cfg.DispatchWidth && !theFIQ.empty() &&
           thePipeline.ready() && !theSyncInsnInProgress) {
      boost::intrusive_ptr<AbstractInstruction> inst = thePipeline.peek();
      if (theFIQ.front()->haltDispatch()) {
        // May only dispatch black box op when core is synchronized
        if (is_sync && dispatched == 0) {
          DISPATCH_DBG("Halt-dispatch " << *theFIQ.front());
          thePipeline.dequeue();
          // boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
          // theFIQ.pop_front();

          // Report the dispatched instruction to the PowerTracker
          theOpcode = (dynamic_cast<armInstruction *>(inst.get()))->getOpcode();
          FLEXUS_CHANNEL(DispatchedInstructionOut) << theOpcode;
          FLEXUS_CHANNEL(DispatchOut) << inst;
          theSyncInsnInProgress = true;
        } else {
          DISPATCH_DBG("No available " << *theFIQ.front());
          break; // No more dispatching this cycle
        }

      } else {
        DISPATCH_DBG("Dispatching " << *theFIQ.front());
        // boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
        // theFIQ.pop_front();
        thePipeline.dequeue();

        // Report the dispatched instruction to the PowerTracker
        theOpcode = (dynamic_cast<armInstruction *>(inst.get()))->getOpcode();
        FLEXUS_CHANNEL(DispatchedInstructionOut) << theOpcode;

        FLEXUS_CHANNEL(DispatchOut) << inst;
      }
      ++dispatched;
      --available_dispatch;
    }

    DISPATCH_DBG("--------------FINISH DISPATCHING------------------------");
  }
};

} // End namespace narmDecoder

FLEXUS_COMPONENT_INSTANTIATOR(armDecoder, narmDecoder);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT armDecoder

#define DBG_Reset
#include DBG_Control()
