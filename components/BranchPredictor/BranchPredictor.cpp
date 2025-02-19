#include "BranchPredictor.hpp"

#include "core/debug/debug.hpp"
#include "core/types.hpp"

#include <components/uFetch/uFetchTypes.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/checkpoint/json.hpp>
#include <fstream>
using json = nlohmann::json;

// #define DBG_DefineCategories BPred
// #define DBG_SetDefaultOps    AddCat(BPred)
// #include DBG_Control()

BranchPredictor::BranchPredictor(std::string const& aName, uint32_t anIndex, uint32_t aBTBSets, uint32_t aBTBWays)
  : theName(aName)
  , theIndex(anIndex)
  , theSerial(0)
  , theBTB(aBTBSets, aBTBWays)
  , theBranches(aName + "-branches")

  , thePredictions_TAGE(aName + "-predictions:TAGE")
  , theCorrect_TAGE(aName + "-correct:TAGE")
  , theMispredict_TAGE(aName + "-mispredict:TAGE")
  , theMispredict_TAGE_User(aName + "-mispredict:TAGE:User")
  , theMispredict_TAGE_System(aName + "-mispredict:TAGE:System")

  , thePredictions_BTB(aName + "-predictions:BTB")
  , theCorrect_BTB(aName + "-correct:BTB")
  , theMispredict_BTB(aName + "-mispredict:BTB")
  , theMispredict_BTB_User(aName + "-mispredict:BTB:User")
  , theMispredict_BTB_System(aName + "-mispredict:BTB:System")

  , theMispredict_BTB_Unconditional(aName + "-mispredict:BTB:Unconditional")
  , theMispredict_BTB_Unconditional_User(aName + "-mispredict:BTB:Unconditional:User")
  , theMispredict_BTB_Unconditional_System(aName + "-mispredict:BTB:Unconditional:System")

  , theMispredict_BTB_WrongType(aName + "-mispredict:BTB:WrongType")
  , theMispredict_BTB_WrongType_User(aName + "-mispredict:BTB:WrongType:User")
  , theMispredict_BTB_WrongType_System(aName + "-mispredict:BTB:WrongType:System")

  , theMispredict_BTB_Conditional(aName + "-mispredict:BTB:Conditional")
  , theMispredict_BTB_Conditional_User(aName + "-mispredict:BTB:Conditional:User")
  , theMispredict_BTB_Conditional_System(aName + "-mispredict:BTB:Conditional:System")
}

/* Depending on whether the prediction of the Branch Predictor we use is Taken or Not Taken, the target is returned
 * If the prediction is NotTaken, there is no need to read the BTB as we will anyway jump to the next instruction
 * If the prediction is taken, we jump to the target address (if present) as given by the BTB
 */
VirtualMemoryAddress
BranchPredictor::predictConditional(VirtualMemoryAddress anAddress, BPredState& aBPState)
{
    ++thePredictions_TAGE;

    bool isTaken = theTage.get_prediction((uint64_t)anAddress, aBPState);

    aBPState.thePrediction = isTaken ? kTaken : kNotTaken;

    if (aBPState.thePrediction <= kTaken && theBTB.target(anAddress)) { 
        ++thePredictions_BTB;
        return *theBTB.target(anAddress); 
    }

    return VirtualMemoryAddress(0);
}

void
BranchPredictor::recoverHistory(const BPredRedictRequest& aRequest)
{
    theTage.restore_history(*aRequest.theBPState);

    if (!aRequest.theInsertNewHistory) {
        return;
    }

    const BPredState &aBPState = *aRequest.theBPState;

    if(aBPState.theActualType == Flexus::SharedTypes::kNonBranch) {
        return;
    }

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

void
BranchPredictor::checkpointHistory(BPredState& aBPState) const
{
    theTage.checkpointHistory(aBPState);
}

VirtualMemoryAddress
BranchPredictor::predict(VirtualMemoryAddress anAddress, BPredState& aBPState)
{
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
            DBG_Assert(false);
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
        case kReturn:
            if (theBTB.target(anAddress)) {
                aBPState.thePredictedTarget = *theBTB.target(anAddress);
            } else {
                aBPState.thePredictedTarget = VirtualMemoryAddress(0);
            }
            // theTage.get_prediction((uint64_t)anAddress, aBPState);
            theTage.update_history(aBPState, true, aBPState.pc);
            break;
        default: aBPState.thePredictedTarget = VirtualMemoryAddress(0); break;
    }

    if (aBPState.thePredictedType != kNonBranch) {
        // DBG_(Verb,
        //      (<< theIndex << "-BPRED-PREDICT: PC \t" << anAddress << " serial " << aBPState.theSerial << " Target \t"
        //       << aBPState.thePredictedTarget << "\tType " << aBPState.thePredictedType));
    }

    return aBPState.thePredictedTarget;
}

void
BranchPredictor::train(boost::intrusive_ptr<BPredState>& aBPState)
{
    DBG_(VVerb, (<< "Training Branch Predictor by PC: " << std::hex << aBPState->pc));
    // Implementation of feedback function

    DBG_Assert(aBPState->theActualTarget != VirtualMemoryAddress(0));

    if (aBPState->theActualDirection <= kTaken || aBPState->theActualType == kNonBranch) {
        // BTB is only updated when the branch is taken, or when the branch is not a branch
        theBTB.update(aBPState->pc, aBPState->theActualType, aBPState->theActualTarget);
    }

    bool is_system = ((uint64_t)aBPState->pc >> 63) != 0;

    bool is_mispredict = aBPState->theActualTarget != aBPState->thePredictedTarget;

    if (is_mispredict) {
        if(aBPState->theActualType != kConditional) {
            // Wrong target for non-conditional
            ++theMispredict_BTB;
            ++theMispredict_BTB_Unconditional;
            if (is_system) {
                ++theMispredict_BTB_System;
                ++theMispredict_BTB_Unconditional_System;
            } else {
                ++theMispredict_BTB_User;
                ++theMispredict_BTB_Unconditional_User;
            }
        } else {
            if (aBPState->theActualType != aBPState->thePredictedType) {
                // Wrong type
                ++theMispredict_BTB;
                ++theMispredict_BTB_WrongType;
                if (is_system) {
                    ++theMispredict_BTB_System;
                    ++theMispredict_BTB_WrongType_System;
                } else {
                    ++theMispredict_BTB_User;
                    ++theMispredict_BTB_WrongType_User;
                }
            } else {
                bool direction_matching = 
                (aBPState->theActualDirection <= kTaken && aBPState->thePrediction <= kTaken) || 
                (aBPState->theActualDirection > kTaken && aBPState->thePrediction > kTaken);
                if (!direction_matching) {
                    // Wrong direction
                    ++theMispredict_TAGE;
                    if (is_system) {
                        ++theMispredict_TAGE_System;
                    } else {
                        ++theMispredict_TAGE_User;
                    }
                } else {
                    // Wrong target for conditional
                    ++theMispredict_BTB;
                    ++theMispredict_BTB_Conditional;
                    if (is_system) {
                        ++theMispredict_BTB_System;
                        ++theMispredict_BTB_Conditional_System;
                    } else {
                        ++theMispredict_BTB_User;
                        ++theMispredict_BTB_Conditional_User;
                    }
                }
            }
        }
    }

    ++theBranches;

    if (aBPState->thePredictedType == kConditional) {
        bool taken = (aBPState->theActualDirection <= kTaken);
        theTage.update_predictor(aBPState->pc, *aBPState, taken);
    }
}

void
BranchPredictor::loadState(std::string const& aDirName)
{
    std::string fname(aDirName);
    fname += "/" + boost::padded_string_cast<3, '0'>(theIndex) + "-bpred" + ".json";
    std::ifstream ifs(fname.c_str());

    json checkpoint;
    ifs >> checkpoint;

    theBTB.loadState(checkpoint["btb"]);
    theTage.loadState(checkpoint["tage"]);
    ifs.close();
}

void
BranchPredictor::saveState(std::string const& aDirName)
{
    std::string fname(aDirName);
    fname += "/" + boost::padded_string_cast<3, '0'>(theIndex) + "-bpred" + ".json";
    std::ofstream ofs(fname.c_str());

    json checkpoint;

    checkpoint["btb"]  = theBTB.saveState();
    checkpoint["tage"] = theTage.saveState();

    ofs << std::setw(4) << checkpoint << std::endl;
    ofs.close();
}