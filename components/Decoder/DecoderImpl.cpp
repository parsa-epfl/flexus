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

#include "Decoder.hpp"
#include "SemanticInstruction.hpp"

#include <core/component.hpp>

#define FLEXUS_BEGIN_COMPONENT Decoder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories Decoder
#define DBG_SetDefaultOps    AddCat(Decoder)
#include DBG_Control()

#include <boost/weak_ptr.hpp>
#include <components/MTManager/MTManager.hpp>

namespace nDecoder {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop);

class FLEXUS_COMPONENT(Decoder)
{
    FLEXUS_COMPONENT_IMPL(Decoder);

    int64_t theInsnSequenceNo;
    std::list<boost::intrusive_ptr<AbstractInstruction>> theFIQ;

    bool theSyncInsnInProgress;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(Decoder)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const
    {
        // the FAG is always quiesced.
        return theFIQ.empty() && !theSyncInsnInProgress;
    }

    void initialize()
    {
        theInsnSequenceNo     = 0;
        theSyncInsnInProgress = false;
    }

    void finalize() {}

  public:
    FLEXUS_PORT_ALWAYS_AVAILABLE(FetchBundleIn);
    void push(interface::FetchBundleIn const&, pFetchBundle& aBundle)
    {
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
    int32_t pull(interface::AvailableFIQOut const&)
    {
        int32_t avail = cfg.FIQSize - theFIQ.size();
        if (avail < 0) { avail = 0; }
        return avail;
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
    int32_t pull(ICount const&) { return theFIQ.size(); }

    FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const&)
    {
        int32_t available_dispatch = 0;
        bool is_sync               = false;
        std::pair<int, bool> dispatch_state;
        FLEXUS_CHANNEL(AvailableDispatchIn) >> dispatch_state;
        boost::tie(available_dispatch, is_sync) = dispatch_state;

        return (theFIQ.empty() || (available_dispatch == 0) || (theSyncInsnInProgress && !is_sync));
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(SquashIn);
    void push(interface::SquashIn const&, eSquashCause& aReason)
    {
        DBG_(VVerb,
             Comp(*this)(<< "DISPATCH SQUASH: " << aReason << " FIQ discarding: " << theFIQ.size() << " instructions"));
        theFIQ.clear();

        FLEXUS_CHANNEL(SquashOut) << aReason;
    }

  public:
    void drive(interface::DecoderDrive const&)
    {
        if (cfg.Multithread) {
            if (nMTManager::MTManager::get()->runThisD(flexusIndex())) { doDecode(); }
        } else {
            doDecode();
        }
    }

  private:
    // Implementation of the FetchDrive drive interface
    void doDecode()
    {
        DISPATCH_DBG("--------------START DISPATCHING------------------------");

        DBG_Assert(FLEXUS_CHANNEL(AvailableDispatchIn).available());
        // the FLEXUS_CHANNEL can only write to an lvalue, hence this rather
        // ugly construction.
        int32_t available_dispatch = 0;
        bool is_sync               = false;
        std::pair<int, bool> dispatch_state;
        FLEXUS_CHANNEL(AvailableDispatchIn) >> dispatch_state;
        boost::tie(available_dispatch, is_sync) = dispatch_state;

        int64_t theOpcode;

        if (is_sync) {
            // If we had a synchronized instruction in progress, it has completed
            theSyncInsnInProgress = false;
        }
        uint32_t dispatched = 0;

        if (!(available_dispatch > 0 && dispatched < cfg.DispatchWidth && !theFIQ.empty() && !theSyncInsnInProgress)) {
            DISPATCH_DBG("Can't dispatch "
                         << "available_dispatch " << available_dispatch << ", dispatched < cfg.DispatchWidth "
                         << int(dispatched < cfg.DispatchWidth) << ", theFIQ is empty " << int(theFIQ.empty())
                         << ", no Sync Insn In Progress " << int(!theSyncInsnInProgress));
        }
        while (available_dispatch > 0 && dispatched < cfg.DispatchWidth && !theFIQ.empty() && !theSyncInsnInProgress) {
            if (theFIQ.front()->haltDispatch()) {
                // May only dispatch black box op when core is synchronized
                if (is_sync && dispatched == 0) {
                    DISPATCH_DBG("Halt-dispatch " << *theFIQ.front());
                    boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
                    theFIQ.pop_front();

                    // Report the dispatched instruction to the PowerTracker
                    theOpcode = (dynamic_cast<ArchInstruction*>(inst.get()))->getOpcode();
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
                theOpcode = (dynamic_cast<ArchInstruction*>(inst.get()))->getOpcode();
                FLEXUS_CHANNEL(DispatchedInstructionOut) << theOpcode;

                FLEXUS_CHANNEL(DispatchOut) << inst;
            }
            ++dispatched;
            --available_dispatch;
        }

        DISPATCH_DBG("--------------FINISH DISPATCHING------------------------");
    }
};

} // End namespace nDecoder

FLEXUS_COMPONENT_INSTANTIATOR(Decoder, nDecoder);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Decoder

#define DBG_Reset
#include DBG_Control()
