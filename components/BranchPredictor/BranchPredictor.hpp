#ifndef FLEXUS_BRANCHPREDICTOR
#define FLEXUS_BRANCHPREDICTOR

#include "BTB.hpp"
#include "TAGEImpl.hpp"
#include "core/stats.hpp"
#include "core/types.hpp"

#include <components/uFetch/uFetchTypes.hpp>

namespace Stat = Flexus::Stat;

class BranchPredictor
{
  private:
    std::string theName;
    uint32_t theIndex;
    uint32_t theSerial;
    BTB theBTB;
    PREDICTOR theTage;

    std::vector<boost::intrusive_ptr<BPredState>> theTrainingHistory;

  public:
    Stat::StatCounter theBranches;

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

    Stat::StatCounter theMispredict_BTB_Unconditional;
    Stat::StatCounter theMispredict_BTB_Unconditional_User;
    Stat::StatCounter theMispredict_BTB_Unconditional_System;

    Stat::StatCounter theMispredict_BTB_WrongType;
    Stat::StatCounter theMispredict_BTB_WrongType_User;
    Stat::StatCounter theMispredict_BTB_WrongType_System;

    Stat::StatCounter theMispredict_BTB_Conditional;
    Stat::StatCounter theMispredict_BTB_Conditional_User;
    Stat::StatCounter theMispredict_BTB_Conditional_System;


  private:
    /* Depending on whether the prediction of the Branch Predictor we use is Taken or Not Taken, the target is returned
     * If the prediction is NotTaken, there is no need to read the BTB as we will anyway jump to the next instruction
     * If the prediction is taken, we jump to the target address (if present) as given by the BTB
     */
    VirtualMemoryAddress predictConditional(VirtualMemoryAddress anAddress, BPredState& aBPState);

  public:
    BranchPredictor(std::string const& aName, uint32_t anIndex, uint32_t aBTBSets, uint32_t aBTBWays);
    bool isBranch(VirtualMemoryAddress anAddress);

    void checkpointHistory(BPredState& aBPState) const;

    VirtualMemoryAddress predict(VirtualMemoryAddress anAddress, BPredState& aBPState);

    // This function is called whenever a prediction is resolved.
    void recoverHistory(const BPredRedictRequest& aRequest);

    // This function is called whenever an instruction triggering a prediction retires.
    void train(boost::intrusive_ptr<BPredState>& aBPState);

    void loadState(std::string const& aDirName);
    void saveState(std::string const& aDirName);
};
#endif // FLEXUS_BRANCHPREDICTOR_TIMING