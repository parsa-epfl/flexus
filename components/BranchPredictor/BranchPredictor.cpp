#include "BranchPredictor.hpp"

#include <components/uFetch/uFetchTypes.hpp>
#include "core/debug/debug.hpp"
#include "core/types.hpp"

//#define DBG_DefineCategories BPred
//#define DBG_SetDefaultOps    AddCat(BPred)
//#include DBG_Control()

BranchPredictor::BranchPredictor(std::string const& aName, uint32_t anIndex, uint32_t aBTBSets, uint32_t aBTBWays)
  : theName(aName)
  , theIndex(anIndex)
  , theSerial(0)
  , theBTB(aBTBSets, aBTBWays)
  , theBranches(aName + "-branches")
  , thePredictions_TAGE(aName + "-predictions:TAGE")
  , theCorrect_TAGE(aName + "-correct:TAGE")
  , theMispredict_TAGE(aName + "-mispredict:TAGE")
{
}

/* Depending on whether the prediction of the Branch Predictor we use is Taken or Not Taken, the target is returned
 * If the prediction is NotTaken, there is no need to read the BTB as we will anyway jump to the next instruction
 * If the prediction is taken, we jump to the target address (if present) as given by the BTB
 */
VirtualMemoryAddress
BranchPredictor::predictConditional(VirtualMemoryAddress anAddress, BPredState& aBPState)
{
    bool isTaken = theTage.get_prediction((uint64_t)anAddress, aBPState);

    aBPState.thePrediction = isTaken ? kTaken : kNotTaken;

    if (aBPState.thePrediction <= kTaken && theBTB.target(anAddress)) { return *theBTB.target(anAddress); }

    return VirtualMemoryAddress(0);
}

void
BranchPredictor::reconstructHistory(BPredState aBPState)
{
    assert(aBPState.theActualType != kNonBranch);

    theTage.restore_all_state(aBPState);

    if (aBPState.theActualType == kConditional) {
        if (aBPState.theActualDirection == kTaken) {
            theTage.update_history(aBPState, true, aBPState.pc);
        } else if (aBPState.theActualDirection == kNotTaken) {
            theTage.update_history(aBPState, false, aBPState.pc);
        } else {
            DBG_Assert(false, (<< "Should never enter here"));
        }
    } else {
        theTage.update_history(aBPState, true, aBPState.pc);
    }
}

bool
BranchPredictor::isBranch(VirtualMemoryAddress anAddress)
{
    return theBTB.contains(anAddress);
}

VirtualMemoryAddress
BranchPredictor::predict(VirtualMemoryAddress anAddress, BPredState& aBPState)
{
    ++thePredictions_TAGE;
    // Implementation of predict function
    aBPState.pc                  = anAddress;
    aBPState.thePredictedType    = theBTB.type(anAddress);
    aBPState.theSerial           = theSerial++;
    aBPState.thePredictedTarget  = VirtualMemoryAddress(0);
    aBPState.thePrediction       = kStronglyTaken;
    aBPState.callUpdatedRAS      = false;
    aBPState.detectedSpecialCall = false;

    switch (aBPState.thePredictedType) {
        case kNonBranch:
            theTage.checkpoint_history(aBPState);
            aBPState.thePredictedTarget = VirtualMemoryAddress(0);
            break;
        case kConditional:
            aBPState.thePredictedTarget = predictConditional(VirtualMemoryAddress(anAddress), aBPState);
            break;
        // TODO: These cases can be merged because they all have the same effect. However, when logging, they all
        // increment different stats. So they must be done in their individual cases and increment the corresponding
        // stats
        case kIndirectCall:
        case kIndirectReg:
        case kUnconditional:
        case kCall:
            if (theBTB.target(anAddress)) {
                aBPState.thePredictedTarget = *theBTB.target(anAddress);
            } else {
                aBPState.thePredictedTarget = VirtualMemoryAddress(0);
            }
            theTage.get_prediction((uint64_t)anAddress, aBPState);
            break;
        default: aBPState.thePredictedTarget = VirtualMemoryAddress(0); break;
    }

    if (aBPState.thePredictedType != kNonBranch) {
        //DBG_(Verb,
        //     (<< theIndex << "-BPRED-PREDICT: PC \t" << anAddress << " serial " << aBPState.theSerial << " Target \t"
        //      << aBPState.thePredictedTarget << "\tType " << aBPState.thePredictedType));
    }

    return aBPState.thePredictedTarget;
}

void
BranchPredictor::feedback(VirtualMemoryAddress anAddress,
                          eBranchType anActualType,
                          eDirection anActualDirection,
                          VirtualMemoryAddress anActualAddress,
                          BPredState& aBPState)
{
    // Implementation of feedback function
    theBTB.update(anAddress, anActualType, anActualAddress);

    bool is_mispredict = false;
    if (anActualType != aBPState.thePredictedType) {
        is_mispredict = true;
    } else {
        if (anActualType == kConditional) {
            if (!(aBPState.thePrediction >= kNotTaken) && (anActualDirection >= kNotTaken)) {
                if ((aBPState.thePrediction <= kTaken) && (anActualDirection <= kTaken)) {
                    if (anActualAddress == aBPState.thePredictedTarget) { is_mispredict = true; }
                } else {
                    is_mispredict = true;
                }
            }
        }
    }

    aBPState.theActualDirection = anActualDirection;
    aBPState.theActualType      = anActualType;

    if (is_mispredict) {
        ++theMispredict_TAGE;
        reconstructHistory(aBPState);
    } else {
        ++theCorrect_TAGE;
    }
    ++theBranches;

    if (aBPState.thePredictedType == kConditional && anActualType == kConditional) {
        bool taken = (anActualDirection <= kTaken);
        theTage.update_predictor(anAddress, aBPState, taken);
    }
}


void BranchPredictor::loadState(std::string const& aDirName) {}
void BranchPredictor::saveState(std::string const& aDirName) {}