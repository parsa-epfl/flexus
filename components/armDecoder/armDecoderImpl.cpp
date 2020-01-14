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

#include <components/armDecoder/SemanticInstruction.hpp>
#include <components/armDecoder/armDecoder.hpp>

#define FLEXUS_BEGIN_COMPONENT armDecoder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

#include <boost/weak_ptr.hpp>
#include <components/MTManager/MTManager.hpp>

namespace ll = boost::lambda;

namespace narmDecoder {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
       int32_t aUop);

class FLEXUS_COMPONENT(armDecoder) {
  FLEXUS_COMPONENT_IMPL(armDecoder);

  int64_t theInsnSequenceNo;
  std::list<boost::intrusive_ptr<AbstractInstruction>> theFIQ;

  bool theSyncInsnInProgress;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(armDecoder) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  bool isQuiesced() const {
    // the FAG is always quiesced.
    return theFIQ.empty() && !theSyncInsnInProgress;
  }

  void initialize() {
    theInsnSequenceNo = 0;
    theSyncInsnInProgress = false;
  }

  void finalize() {
  }

public:
  FLEXUS_PORT_ALWAYS_AVAILABLE(FetchBundleIn);
  void push(interface::FetchBundleIn const &, pFetchBundle &aBundle) {
    std::list<tFillLevel>::iterator fill_iter = aBundle->theFillLevels.begin();
    for (auto it = aBundle->theOpcodes.begin(); it != aBundle->theOpcodes.end(); it++) {
      int32_t uop = 0;
      boost::intrusive_ptr<AbstractInstruction> insn;
      bool final_uop = false;
      // Note that multi-uop instructions can cause theFIQ to fill beyond its
      // configured size.
      while (!final_uop) {
        boost::tie(insn, final_uop) = decode(*it, aBundle->coreID, ++theInsnSequenceNo, uop++);
        if (insn) {
          insn->setFetchTransactionTracker(it->theTransaction);
          // Set Fill Level for the insn
          insn->setSourceLevel(*fill_iter);
          theFIQ.push_back(insn);
        } else
          DBG_(VVerb, (<< "No INSTRUCTION"));
      }
      ++fill_iter;
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableFIQOut);
  int32_t pull(interface::AvailableFIQOut const &) {
    int32_t avail = cfg.FIQSize - theFIQ.size();
    if (avail < 0) {
      avail = 0;
    }
    return avail;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &) {
    return theFIQ.size();
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
           !theSyncInsnInProgress) {
      if (theFIQ.front()->haltDispatch()) {
        // May only dispatch black box op when core is synchronized
        if (is_sync && dispatched == 0) {
          DISPATCH_DBG("Halt-dispatch " << *theFIQ.front());
          boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
          theFIQ.pop_front();

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
        boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
        theFIQ.pop_front();

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
