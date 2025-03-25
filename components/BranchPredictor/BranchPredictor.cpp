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
  , theRedirects(aName + "-redirects")

  , theBranchMispredictionPenalty(aName + "-mispredict:penalty")
  , theRedirectionPenalty(aName + "-redirect:penalty")

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
  , theMispredict_Return(aName + "-mispredict:Return")
  , theMispredict_Indirect(aName + "-mispredict:Indirect")
{
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
    BPredState &aBPState = *aRequest.theBPState;

    RAS.recover(aBPState);

    theTage.restore_history(*aRequest.theBPState);
    if (!aRequest.theInsertNewHistory) {
        return;
    }

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
BranchPredictor::isBranch(VirtualMemoryAddress anAddress, bool PerfectBPU)
{
    if (PerfectBPU){
        return theOracleBTB.find(anAddress) != theOracleBTB.end();
    }

    return theBTB.contains(anAddress);
}

void
BranchPredictor::checkpointHistory(BPredState& aBPState) const
{
    theTage.checkpointHistory(aBPState);
}

void BranchPredictor::recoverOracle(const uint64_t aSerial){
    theOracleIdx--;
    while (theOracleBPU[theOracleIdx].theSerial > aSerial){
        theOracleBPU[theOracleIdx].theSerial = 0;
        theOracleIdx--;
    }
    theOracleIdx++;
}

uint32_t BranchPredictor::getSerial(){
    return theSerial++;
}

void BranchPredictor::recordRedirectStats(std::pair<uint64_t, uint64_t> aRange){
    theRedirects++;
    redirectCycles.push_back(aRange);
    // fix this by setting the counter at the end 
    if(redirectCycles.size() % 1000 == 0)
        theRedirectionPenalty = calculateRedirectCycles(redirectCycles);
}

uint64_t BranchPredictor::calculateRedirectCycles
    (std::vector<std::pair<uint64_t, uint64_t>> &ranges){
    
    if (ranges.empty()) return 0;

    // Sort ranges based on start points
    std::sort(ranges.begin(), ranges.end());

    uint64_t totalLength = 0;
    uint64_t start = ranges[0].first, end = ranges[0].second;

    for (size_t i = 1; i < ranges.size(); ++i) {
        uint64_t s = ranges[i].first;
        uint64_t e = ranges[i].second;

        if (s <= end) {
            // Merge overlapping or adjacent ranges
            end = std::max(end, e);
        } else {
            // Add the previous merged range length and start a new range
            totalLength += end - start;
            start = s;
            end = e;
        }
    }

    // Add the last merged range length
    totalLength += end - start;
    return totalLength;
}

VirtualMemoryAddress
BranchPredictor::predict(VirtualMemoryAddress anAddress, BPredState& aBPState, bool PerfectBPU)
{
    // Implementation of predict function
    aBPState.pc                  = anAddress;
    aBPState.thePredictedType    = theBTB.type(anAddress);
    aBPState.theSerial           = theSerial++;
    aBPState.thePredictedTarget  = VirtualMemoryAddress(0);
    aBPState.thePrediction       = kStronglyTaken;
    aBPState.callUpdatedRAS      = false;
    aBPState.detectedSpecialCall = false;

    RAS.get(aBPState.theRAS);

    if(PerfectBPU){
        if (anAddress == theOracleBPU[theOracleIdx].pc){
            aBPState.thePredictedType = static_cast<eBranchType>(theOracleBPU[theOracleIdx].theActualType);
            if (theOracleBPU[theOracleIdx].theActualTarget - theOracleBPU[theOracleIdx].pc == 4){
                aBPState.thePredictedTarget = VirtualMemoryAddress(0);    
            }
            else{
                aBPState.thePredictedTarget = VirtualMemoryAddress(theOracleBPU[theOracleIdx].theActualTarget);    
            }
            theOracleBPU[theOracleIdx].theSerial = aBPState.theSerial;
            theOracleIdx++;
        }
        else{
            // The core is in wrong path, continue in fall-through until be redirected
            // This case should not happen without any blackbox instruction
            aBPState.thePredictedTarget = VirtualMemoryAddress(0); 
        }
        //std::cout << "returning: " << (uint64_t)aBPState.thePredictedTarget << "\n";
        return aBPState.thePredictedTarget;
    }

    switch (aBPState.thePredictedType) {
        case kNonBranch:
            aBPState.thePredictedTarget = VirtualMemoryAddress(0);
            break;
        case kConditional:
            aBPState.thePredictedTarget = predictConditional(VirtualMemoryAddress(anAddress), aBPState);
            break;
        // TODO: These cases can be merged because they all have the same effect. However, when logging, they all
        // increment different stats. So they must be done in their individual cases and increment the corresponding
        // stats
        case kIndirectReg:
        case kUnconditional:
            if (theBTB.target(anAddress)) {
                aBPState.thePredictedTarget = *theBTB.target(anAddress);
            } else {
                aBPState.thePredictedTarget = VirtualMemoryAddress(0);
            }
            // theTage.get_prediction((uint64_t)anAddress, aBPState);
            theTage.update_history(aBPState, true, aBPState.pc);
            break;

        case kIndirectCall:
        case kCall:
            theTage.update_history(aBPState, true, aBPState.pc);

            // btb must hit
            aBPState.thePredictedTarget = *theBTB.target(anAddress);

            // speculative
            RAS.push(anAddress + 4);
            break;

        case kReturn:
            theTage.update_history(aBPState, true, aBPState.pc);

            if (RAS.valid()) {
                // speculative
                aBPState.thePredictedTarget = RAS.pop();
                aBPState.returnUsedRAS = true;
            } else
                // btb must hit
                aBPState.thePredictedTarget = *theBTB.target(anAddress);
            break;

        default:
            aBPState.thePredictedTarget = VirtualMemoryAddress(0); break;
    }

    if (aBPState.thePredictedType != kNonBranch) {
        // DBG_(Verb,
        //      (<< theIndex << "-BPRED-PREDICT: PC \t" << anAddress << " serial " << aBPState.theSerial << " Target \t"
        //       << aBPState.thePredictedTarget << "\tType " << aBPState.thePredictedType));
    }
    return aBPState.thePredictedTarget;
}

void
BranchPredictor::train(BPredState& aBPState)
{
    DBG_(VVerb, (<< "Training Branch Predictor by PC: " << std::hex << aBPState.pc));
    // Implementation of feedback function

    DBG_Assert(aBPState.theActualTarget != VirtualMemoryAddress(0));

    if (aBPState.theActualDirection <= kTaken || aBPState.theActualType == kNonBranch) {
        // BTB is only updated when the branch is taken, or when the branch is not a branch
        theBTB.update(aBPState.pc, aBPState.theActualType, aBPState.theActualTarget);
    }

    bool is_system = ((uint64_t)aBPState.pc >> 63) != 0;

    bool is_mispredict = aBPState.theActualTarget != aBPState.thePredictedTarget;

    if (is_mispredict) {
        /*std::cout << "mispredict, pc: " << (uint64_t)aBPState.pc 
                << ", pred: " << (uint64_t)aBPState.thePredictedTarget 
                << ", actual: " << (uint64_t)aBPState.theActualTarget 
                << ", serial: " << aBPState.theSerial 
                << ", type: " << aBPState.theActualType
                << ", thePredCycle: " << aBPState.thePredCycle << "\n";*/

        if (aBPState.theCorrectionCycle){
            mispredictCycles.push_back(std::make_pair(aBPState.thePredCycle, aBPState.theCorrectionCycle));
            
            // fix this by setting the counter at the end 
            if(mispredictCycles.size() % 1000 == 0)
                theBranchMispredictionPenalty = calculateRedirectCycles(mispredictCycles);
        }

        if(aBPState.theActualType == kReturn){
            ++theMispredict_Return;
        }
        else if(aBPState.theActualType == kIndirectCall || aBPState.theActualType == kIndirectReg){
            ++theMispredict_Indirect;
        }

        if(aBPState.theActualType != kConditional) {
            // Wrong target for non-conditional
            ++theMispredict_BTB;
            if (is_system) {
                ++theMispredict_BTB_System;
            } else {
                ++theMispredict_BTB_User;
            }

            // theTrainingHistory.push_back(aBPState);
        } else {
            if (aBPState.theActualType != aBPState.thePredictedType) {
                // Wrong type
                ++theMispredict_BTB;
                if (is_system) {
                    ++theMispredict_BTB_System;
                } else {
                    ++theMispredict_BTB_User;
                }

                // theTrainingHistory.push_back(aBPState);
            } else {
                bool direction_matching = 
                (aBPState.theActualDirection <= kTaken && aBPState.thePrediction <= kTaken) || 
                (aBPState.theActualDirection > kTaken && aBPState.thePrediction > kTaken);
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
                    if (is_system) {
                        ++theMispredict_BTB_System;
                    } else {
                        ++theMispredict_BTB_User;
                    }

                    // theTrainingHistory.push_back(aBPState);
                }
            }
        }
    }

    ++theBranches;

    if (aBPState.thePredictedType == kConditional && aBPState.thePredictedType == kConditional) {
        bool taken = (aBPState.theActualDirection <= kTaken);
        theTage.update_predictor(aBPState.pc, aBPState, taken);
    }
}

void
BranchPredictor::loadState(std::string const& aDirName, bool PerfectBPU)
{
    std::string fname(aDirName);
    fname += "/" + boost::padded_string_cast<3, '0'>(theIndex) + "-bpred" + ".json";
    std::ifstream ifs(fname.c_str());

    json checkpoint;
    ifs >> checkpoint;

    theBTB.loadState(checkpoint["btb"]);
    theTage.loadState(checkpoint["tage"]);
    ifs.close();

    if (PerfectBPU){
        std::string fname(aDirName);
        fname += "/" + boost::padded_string_cast<3, '0'>(theIndex) + "-ideal-bpred" + ".json";
        std::ifstream ifs(fname.c_str());

        json checkpoint;
        ifs >> checkpoint;

        for (const auto& item : checkpoint) {
            theOracleBPU.push_back({
                item["addr"].get<uint64_t>(),  // Read as uint64_t
                item["type"].get<int>(),
                item["tgt"].get<uint64_t>(),  // Read as uint64_t
                0
            });

            theOracleBTB.insert(VirtualMemoryAddress(theOracleBPU.back().pc));
        }
        ifs.close();
    }
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