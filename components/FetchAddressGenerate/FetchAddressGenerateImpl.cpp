
#include "components/uFetch/uFetchTypes.hpp"
#include "core/types.hpp"
#include <components/FetchAddressGenerate/FetchAddressGenerate.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FetchAddressGenerate
#define DBG_SetDefaultOps    AddCat(FetchAddressGenerate)
#include DBG_Control()

#include <components/BranchPredictor/BranchPredictor.hpp>
#include <components/MTManager/MTManager.hpp>
#include <core/flexus.hpp>
#include <core/qemu/mai_api.hpp>

namespace nFetchAddressGenerate {

using namespace Flexus;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(FetchAddressGenerate)
{
    FLEXUS_COMPONENT_IMPL(FetchAddressGenerate);

    std::vector<MemoryAddress> thePC;
    std::vector<MemoryAddress> theRedirectPC;
    std::vector<bool> theRedirect;
    std::unique_ptr<BranchPredictor> theBranchPredictor;
    uint32_t theCurrentThread;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(FetchAddressGenerate)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    void initialize()
    {
        thePC.resize(cfg.Threads);
        theRedirectPC.resize(cfg.Threads);
        theRedirect.resize(cfg.Threads);
        for (uint32_t i = 0; i < cfg.Threads; ++i) {
            Qemu::Processor cpu = Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + i);
            thePC[i]            = cpu.get_pc();
            AGU_DBG("PC(" << i << ") = " << thePC[i]);
            theRedirectPC[i] = MemoryAddress(0);
            theRedirect[i]   = false;
        }
        theCurrentThread   = cfg.Threads;
        theBranchPredictor = std::make_unique<BranchPredictor>(statName(), flexusIndex(), cfg.BTBSets, cfg.BTBWays);
    }

    void finalize() {
        // Fuck, dump the branch predictor state in a folder called output_state.
        this->saveState("output_state");
    }

    bool isQuiesced() const
    {
        // the FAG is always quiesced.
        return true;
    }

    void saveState(std::string const& aDirName) { theBranchPredictor->saveState(aDirName); }

    void loadState(std::string const& aDirName) { theBranchPredictor->loadState(aDirName); }

  public:
    // RedirectIn
    //----------

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
    void push(interface::RedirectIn const&, index_t anIndex, boost::intrusive_ptr<BPredRedictRequest>& redirectRequest)
    {
        if(!theRedirect[anIndex]) { // Lower priority than the RedirectDueToResyncIn
            theRedirectPC[anIndex] = redirectRequest->theTarget;
            theRedirect[anIndex]   = true;

            theBranchPredictor->recoverHistory(*redirectRequest);
        }
    }

    // TrainIn
    //----------------
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BranchTrainIn);
    void push(interface::BranchTrainIn const&, index_t anIndex, boost::intrusive_ptr<BPredState>& bpState)
    {
        theBranchPredictor->train(*bpState);
    }

    // Drive Interfaces
    //----------------
    // The FetchDrive drive interface sends a commands to the Feeder and then
    // fetches instructions, passing each instruction to its FetchOut port.
    void drive(interface::FAGDrive const&)
    {
        int32_t td = 0;
        if (cfg.Threads > 1) { td = nMTManager::MTManager::get()->scheduleFAGThread(flexusIndex()); }
        doAddressGen(td);
    }

  private:
    // Implementation of the FetchDrive drive interface
    void doAddressGen(index_t anIndex)
    {

        AGU_DBG("--------------START ADDRESS GEN------------------------");

        DBG_Assert(FLEXUS_CHANNEL(uArchHalted).available());
        bool cpu_in_halt = false;
        FLEXUS_CHANNEL(uArchHalted) >> cpu_in_halt;
        if (cpu_in_halt) { return; }

        if (theFlexus->quiescing()) {
            DBG_(VVerb, (<< "FGU: Flexus is quiescing!.. come back later"));
            return;
        }

        if (theRedirect[anIndex]) {
            thePC[anIndex]       = theRedirectPC[anIndex];
            theRedirect[anIndex] = false;
            DBG_(VVerb, Comp(*this)(<< "FGU:  Redirect core[" << anIndex << "] " << thePC[anIndex]));
        }
        DBG_Assert(FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex).available());
        DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex).available());
        int32_t available_faq = 0;
        FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex) >> available_faq;

        int32_t max_addrs = cfg.MaxFetchAddress;
        if (max_addrs > available_faq) {
            DBG_(VVerb,
                 (<< "FGU: max address: " << max_addrs << " is greater than available queue: " << available_faq));
            max_addrs = available_faq;
        }
        int32_t max_predicts = cfg.MaxBPred;

        if (available_faq == 0) DBG_(VVerb, (<< "FGU: available FAQ is empty"));

        //    static int test;
        boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
        while (max_addrs > 0 /*&& test == 0*/) {
            AGU_DBG("Getting addresses: " << max_addrs << " remaining");

            FetchAddr faddr(thePC[anIndex]);
            faddr.theBPState->pc = thePC[anIndex];

            // Checkpoint the history before advancing the PC
            theBranchPredictor->checkpointHistory(*faddr.theBPState);

            // Advance the PC
            if (theBranchPredictor->isBranch(faddr.theAddress)) {
                AGU_DBG("Predicting a Branch");
                faddr.theBPState->thePredictedType = kUnconditional;
                if (max_predicts == 0) {
                    AGU_DBG("Config set the max prediction to zero, so no prediction");
                    break;
                }
                VirtualMemoryAddress prediction = theBranchPredictor->predict(faddr.theAddress, *faddr.theBPState);
                if (prediction == 0) {
                    thePC[anIndex] += 4;
                    faddr.theBPState->thePrediction = kNotTaken;
                }
                else {
                    thePC[anIndex] = prediction;
                    faddr.theBPState->thePrediction = kTaken;
                }
                faddr.theBPState->thePredictedTarget = thePC[anIndex];
                AGU_DBG("Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex);
                AGU_DBG("Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress);

                fetch->theFetches.push_back(faddr);
                --max_predicts;
            } else {
                DBG_(VVerb, (<< "Before Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex));
                thePC[anIndex] += 4;
                faddr.theBPState->thePredictedType = kNonBranch;
                faddr.theBPState->thePredictedTarget = thePC[anIndex];
                faddr.theBPState->thePrediction = kNotTaken;

                DBG_(VVerb, (<< "Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex));
                DBG_(VVerb, (<< "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress));
                fetch->theFetches.push_back(faddr);
            }

            --max_addrs;
            //      test = 1;
        }

        if (fetch->theFetches.size() > 0) {
            DBG_(VVerb, (<< "Sending total fetches: " << fetch->theFetches.size()));

            // Send it to FetchOut
            FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex) << fetch;
        } else {
            DBG_(VVerb, (<< "No fetches to send"));
        }

        AGU_DBG("--------------FINISH ADDRESS GEN------------------------");
    }

  public:
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const&, index_t anIndex)
    {
        int32_t available_faq = 0;
        DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex).available());
        FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex) >> available_faq;
        return available_faq == 0;
    }
};
} // End namespace nFetchAddressGenerate

FLEXUS_COMPONENT_INSTANTIATOR(FetchAddressGenerate, nFetchAddressGenerate);

FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, RedirectIn)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, BranchTrainIn)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, FetchAddrOut)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, AvailableFAQ)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, Stalled)
{
    return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

#define DBG_Reset
#include DBG_Control()
