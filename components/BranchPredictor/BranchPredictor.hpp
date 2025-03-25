#ifndef FLEXUS_BRANCHPREDICTOR
#define FLEXUS_BRANCHPREDICTOR

#include "BTB.hpp"
#include "TAGEImpl.hpp"
#include "RAS.hpp"
#include "core/stats.hpp"
#include "core/types.hpp"
#include <set>

#include <components/uFetch/uFetchTypes.hpp>

namespace Stat = Flexus::Stat;

class BranchPredictor
{
  private:
    std::string theName;
    uint32_t theIndex;
    uint32_t theSerial;
    std::vector<std::pair<uint64_t, uint64_t>> redirectCycles;
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles;

    BTB theBTB;
    PREDICTOR theTage;
    ReturnAddressStack RAS;

    struct BranchInfo {
      uint64_t pc;
      int theActualType;
      uint64_t theActualTarget;
      uint64_t theSerial;
    };

    std::vector<BranchInfo> theOracleBPU;
    uint64_t theOracleIdx = 0;
    std::set<VirtualMemoryAddress> theOracleBTB;

  public:
    Stat::StatCounter theBranches;
    Stat::StatCounter theRedirects;

    Stat::StatCounter theBranchMispredictionPenalty;
    Stat::StatCounter theRedirectionPenalty;

    Stat::StatCounter thePredictions_TAGE;
    Stat::StatCounter theCorrect_TAGE;
    Stat::StatCounter theMispredict_TAGE;
    Stat::StatCounter theMispredict_TAGE_User;
    Stat::StatCounter theMispredict_TAGE_System;

    Stat::StatCounter thePredictions_BTB;
    Stat::StatCounter theCorrect_BTB;
    Stat::StatCounter theMispredict_BTB;
    Stat::StatCounter theMispredict_BTB_User;
    Stat::StatCounter theMispredict_BTB_System;
    Stat::StatCounter theMispredict_Return;
    Stat::StatCounter theMispredict_Indirect;


  private:
    /* Depending on whether the prediction of the Branch Predictor we use is Taken or Not Taken, the target is returned
     * If the prediction is NotTaken, there is no need to read the BTB as we will anyway jump to the next instruction
     * If the prediction is taken, we jump to the target address (if present) as given by the BTB
     */
    VirtualMemoryAddress predictConditional(VirtualMemoryAddress anAddress, BPredState& aBPState);

  public:
    BranchPredictor(std::string const& aName, uint32_t anIndex, uint32_t aBTBSets, uint32_t aBTBWays);
    bool isBranch(VirtualMemoryAddress anAddress, bool PerfectBPU);

    void checkpointHistory(BPredState& aBPState) const;

    VirtualMemoryAddress predict(VirtualMemoryAddress anAddress, BPredState& aBPState, bool PerfectBPU);

    // This function is called whenever a prediction is resolved.
    void recoverHistory(const BPredRedictRequest& aRequest);

    void recoverOracle(const uint64_t aSerial);
    uint32_t getSerial();
    void recordRedirectStats(std::pair<uint64_t, uint64_t> aRange);
    uint64_t calculateRedirectCycles(std::vector<std::pair<uint64_t, uint64_t>> &ranges);

    // This function is called whenever an instruction triggering a prediction retires.
    void train(BPredState& aBPState);

    void getRAS(std::vector<uint64_t> &vec) { return RAS.get(vec); }

    void loadState(std::string const& aDirName, bool PerfectBPU);
    void saveState(std::string const& aDirName);
};
#endif // FLEXUS_BRANCHPREDICTOR_TIMING