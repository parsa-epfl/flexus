
#include "Decoder.hpp"
#include "SemanticInstruction.hpp"

#include <core/component.hpp>

#define FLEXUS_BEGIN_COMPONENT Decoder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories Decoder
#define DBG_SetDefaultOps    AddCat(Decoder)
#include DBG_Control()

#include <boost/weak_ptr.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
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
        std::list<std::shared_ptr<tFillLevel>>::iterator fill_iter = aBundle->theFillLevels.begin();
        for (auto it = aBundle->theOpcodes.begin(); it != aBundle->theOpcodes.end(); it++) {
            auto iter = *it;

            int32_t uop = 0;
            boost::intrusive_ptr<AbstractInstruction> insn;
            bool final_uop = false;
            // Note that multi-uop instructions can cause theFIQ to fill beyond its
            // configured size.
            while (!final_uop) {
                boost::tie(insn, final_uop) = decode(*iter, aBundle->coreID, ++theInsnSequenceNo, uop++);
                if (insn) {
                    insn->setFetchTransactionTracker(iter->theTransaction);
                    // Set Fill Level for the insn
                    insn->setSourceLevel(**fill_iter);
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
        std::tuple<int, int, int> available_regs;
        std::tuple<int, bool, std::tuple<int, int, int>> dispatch_state;
        FLEXUS_CHANNEL(AvailableDispatchIn) >> dispatch_state;
        std::tie(available_dispatch, is_sync, available_regs) = dispatch_state;
        bool noRegs = (std::get<0>(available_regs) == 0) || (std::get<1>(available_regs) == 0) || (std::get<2>(available_regs) == 0);
        return (theFIQ.empty() || (available_dispatch == 0) || (theSyncInsnInProgress && !is_sync) || (noRegs));
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
        std::tuple<int, int, int> available_regs;
        std::tuple<int, bool, std::tuple<int, int, int>> dispatch_state;
        FLEXUS_CHANNEL(AvailableDispatchIn) >> dispatch_state;
        std::tie(available_dispatch, is_sync, available_regs) = dispatch_state;

        int numFreeXRegs = std::get<0>(available_regs);
        int numFreeVRegs = std::get<1>(available_regs);
        int numFreeCCs   = std::get<2>(available_regs);

        int64_t theOpcode;

        if (is_sync) {
            // If we had a synchronized instruction in progress, it has completed
            theSyncInsnInProgress = false;
        }
        uint32_t dispatched = 0;

        if (0 >= available_dispatch)
            DISPATCH_DBG("Can't dispatch REASON: available_dispatch (=" << available_dispatch << ") is under 0");

        if (dispatched > cfg.DispatchWidth)
            DISPATCH_DBG("Can't dispatch REASON: it dispatched (="
                         << dispatched << ") more than the cfg.DispatchWidth (=" << cfg.DispatchWidth << ")");

        if (theFIQ.empty()) DISPATCH_DBG("Can't dispatch REASON: the Fast Interrupt Query queue is empty");

        if (theSyncInsnInProgress) DISPATCH_DBG("Can't dispatch REASON: A sync is in progress");

        if (numFreeXRegs == 0)
            DISPATCH_DBG("Can't dispatch REASON: No free Int registers (=" << numFreeXRegs << ")");

        if (numFreeVRegs == 0)
            DISPATCH_DBG("Can't dispatch REASON: No free FP registers (=" << numFreeVRegs << ")");

        if (numFreeCCs == 0)
            DISPATCH_DBG("Can't dispatch REASON: No free CC registers (=" << numFreeCCs << ")");

        int numReads, numWrites, numWCCs;
        while (available_dispatch > 0 && dispatched < cfg.DispatchWidth && !theFIQ.empty() && !theSyncInsnInProgress
                && numFreeXRegs > 0 && numFreeVRegs > 0 && numFreeCCs > 0) {
            if (theFIQ.front()->haltDispatch()) {
                // May only dispatch black box op when core is synchronized
                if (is_sync && dispatched == 0) {
                    DISPATCH_DBG("Halt-dispatch " << *theFIQ.front());
                    boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
                    std::tie(numReads, numWrites, numWCCs) = boost::polymorphic_pointer_downcast<Instruction>(inst)->numReadsWrites();
                    numFreeXRegs -= numWrites;
                    numFreeXRegs -= numReads;   // This is worst case scenario, not all reads need a new physical reg
                    numFreeCCs -= numWCCs;
                    if (numFreeXRegs < 0 || numFreeVRegs < 0 || numFreeCCs < 0) {
                        DBG_(VVerb, (<< "Cannot dispatch " << *theFIQ.front() << " Num Free Int Regs: " << numFreeXRegs
                                            << " Num Free FP Regs: " << numFreeVRegs << " Num Free CCs: " << numFreeCCs));
                        break; // No more dispatching this cycle
                    }
                    theFIQ.pop_front();

                    // Report the dispatched instruction to the PowerTracker
                    theOpcode = (dynamic_cast<ArchInstruction*>(inst.get()))->getOpcode();
                    FLEXUS_CHANNEL(DispatchedInstructionOut) << theOpcode;

                    FLEXUS_CHANNEL(DispatchOut) << inst;
                    std::tie(numReads, numWrites, numWCCs) = boost::polymorphic_pointer_downcast<Instruction>(inst)->numReadsWrites();
                    numFreeXRegs -= numWrites;
                    numFreeXRegs -= numReads;   // This is worst case scenario, not all reads need a new physical reg
                    numFreeCCs -= numWCCs;
                    theSyncInsnInProgress = true;
                } else {
                    DISPATCH_DBG("No available " << *theFIQ.front());
                    break; // No more dispatching this cycle
                }

            } else {
                DISPATCH_DBG("Dispatching " << *theFIQ.front());
                boost::intrusive_ptr<AbstractInstruction> inst(theFIQ.front());
                std::tie(numReads, numWrites, numWCCs) = boost::polymorphic_pointer_downcast<Instruction>(inst)->numReadsWrites();
                numFreeXRegs -= numWrites;
                numFreeXRegs -= numReads;   // This is worst case scenario, not all reads need a new physical reg
                numFreeCCs -= numWCCs;
                if (numFreeXRegs < 0 || numFreeVRegs < 0 || numFreeCCs < 0) {
                    DBG_(VVerb, (<< "Cannot dispatch " << *theFIQ.front() << " Num Free Int Regs: " << numFreeXRegs
                                         << " Num Free FP Regs: " << numFreeVRegs << " Num Free CCs: " << numFreeCCs));
                    break; // No more dispatching this cycle
                }
                theFIQ.pop_front();

                // Report the dispatched instruction to the PowerTracker
                theOpcode = (dynamic_cast<ArchInstruction*>(inst.get()))->getOpcode();
                FLEXUS_CHANNEL(DispatchedInstructionOut) << theOpcode;

                FLEXUS_CHANNEL(DispatchOut) << inst;
            }
            DBG_(VVerb, (<< "Num Free Int Regs: " << numFreeXRegs << " Num Free FP Regs: " << numFreeVRegs << " Num Free CCs: " << numFreeCCs));
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
