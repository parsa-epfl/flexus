#ifndef FLEXUS_BRANCHPREDICTOR
#define FLEXUS_BRANCHPREDICTOR

#include "BTB.hpp"
#include "GShare.hpp"
#include "TAGEImpl.hpp"
#include "RAS.hpp"
#include "core/stats.hpp"
#include "core/types.hpp"
#include <memory>
#include <set>
#include "ITTAGE.h"

#include <components/uFetch/uFetchTypes.hpp>

namespace Stat = Flexus::Stat;

class BranchPredictor
{
  private:
    std::string theName;
    uint32_t theIndex;
    uint32_t theSerial;
    std::vector<std::pair<uint64_t, uint64_t>> redirectCycles;
    std::vector<std::pair<uint64_t, uint64_t>> redirectCycles_Resync;
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles;
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles_TAGE;
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles_Indirect;
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles_Return; 
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles_BTB;
    std::vector<std::pair<uint64_t, uint64_t>> mispredictCycles_other;
    

    BTB theBTB;
    // Only the configured direction predictor is constructed. BTB/RAS are shared.
    std::unique_ptr<PREDICTOR> theTage;
    std::unique_ptr<GShare> theGShare;
    ReturnAddressStack theRAS;
    ITTAGE theITTage;

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
    Stat::StatCounter theResyncRedirects;

    Stat::StatCounter theBranchMispredictionPenalty;
    Stat::StatCounter theBranchMispredictionPenalty_TAGE;
    Stat::StatCounter theBranchMispredictionPenalty_Indirect;
    Stat::StatCounter theBranchMispredictionPenalty_Return;
    Stat::StatCounter theBranchMispredictionPenalty_BTB;
    Stat::StatCounter theBranchMispredictionPenalty_other;
    Stat::StatCounter theRedirectionPenalty;
    Stat::StatCounter theRedirectionPenalty_Resync;

    Stat::StatCounter thePredictions_TAGE;
    Stat::StatCounter theCorrect_TAGE;
    Stat::StatCounter theMispredict_TAGE;
    Stat::StatCounter theMispredict_TAGE_User;
    Stat::StatCounter theMispredict_TAGE_System;

    Stat::StatCounter thePredictions_GShare;
    Stat::StatCounter theMispredict_GShare;

    Stat::StatCounter thePredictions_BTB;
    Stat::StatCounter theCorrect_BTB;
    Stat::StatCounter theMispredict_BTB;
    Stat::StatCounter theMispredict_BTB_User;
    Stat::StatCounter theMispredict_BTB_System;
    Stat::StatCounter theMispredict_Return;
    Stat::StatCounter theMispredict_Indirect;
    Stat::StatCounter theMispredict_other;


  private:
    /* Depending on whether the prediction of the Branch Predictor we use is Taken or Not Taken, the target is returned
     * If the prediction is NotTaken, there is no need to read the BTB as we will anyway jump to the next instruction
     * If the prediction is taken, we jump to the target address (if present) as given by the BTB
     */
    VirtualMemoryAddress predictConditional(VirtualMemoryAddress anAddress, BPredState& aBPState);
    void updateHistory(const BPredState& aBPState, bool taken, uint64_t address);
    void helperPenaltyCalculator(BPredState& aBPState, std::vector<std::pair<uint64_t, uint64_t>> &mispredictList, Stat::StatCounter &stat);

  public:
    BranchPredictor(std::string const& aName, uint32_t anIndex, uint32_t aBTBSets, uint32_t aBTBWays,
                    std::string const& aPredictor = "TAGE", uint32_t aGShareHistoryLength = 13,
                    uint32_t aGShareTableSize = 8192);
    bool isBranch(VirtualMemoryAddress anAddress, bool PerfectBPU);

    void checkpointHistory(BPredState& aBPState) const;

    VirtualMemoryAddress predict(VirtualMemoryAddress anAddress, BPredState& aBPState, bool PerfectBPU);

    // Repair speculative history when a redirect is delivered.
    void recoverHistory(const BPredRedictRequest& aRequest);

    void recoverOracle(const uint64_t aSerial);
    uint32_t getSerial();
    void recordRedirectStats(std::pair<uint64_t, uint64_t> aRange);
    void recordResyncRedirectStats(BPredState& aBPState);
    inline uint64_t calculateRedirectCycles(std::vector<std::pair<uint64_t, uint64_t>> &ranges);

    // Train direction tables when the instruction reaches final commit.
    void train(BPredState& aBPState);

    void getRAS(std::vector<uint64_t> &vec) { return theRAS.get(vec); }

    void loadState(std::string const& aDirName, bool PerfectBPU);
    void saveState(std::string const& aDirName);
};
#endif // FLEXUS_BRANCHPREDICTOR_TIMING
