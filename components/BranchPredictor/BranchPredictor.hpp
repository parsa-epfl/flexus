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

  public:
    Stat::StatCounter theBranches;
    Stat::StatCounter thePredictions_TAGE;
    Stat::StatCounter theCorrect_TAGE;
    Stat::StatCounter theMispredict_TAGE;

  private:
    /* Depending on whether the prediction of the Branch Predictor we use is Taken or Not Taken, the target is returned
     * If the prediction is NotTaken, there is no need to read the BTB as we will anyway jump to the next instruction
     * If the prediction is taken, we jump to the target address (if present) as given by the BTB
     */
    VirtualMemoryAddress predictConditional(VirtualMemoryAddress anAddress, BPredState& aBPState);

    void reconstructHistory(BPredState aBPState);

  public:
    BranchPredictor(std::string const& aName, uint32_t anIndex, uint32_t aBTBSets, uint32_t aBTBWays);
    bool isBranch(VirtualMemoryAddress anAddress);

    VirtualMemoryAddress predict(VirtualMemoryAddress anAddress, BPredState& aBPState);
    void feedback(VirtualMemoryAddress anAddress,
                  eBranchType anActualType,
                  eDirection anActualDirection,
                  VirtualMemoryAddress anActualAddress,
                  BPredState& aBPState);

    void loadState(std::string const& aDirName);
    void saveState(std::string const& aDirName);
};
#endif // FLEXUS_BRANCHPREDICTOR_TIMING