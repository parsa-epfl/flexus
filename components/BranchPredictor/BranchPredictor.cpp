#include "BranchPredictor.hpp"

#include "core/debug/debug.hpp"
#include "core/types.hpp"
#include "core/qemu/mai_api.hpp"

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
  , theResyncRedirects(aName + "-redirects:resync")

  , theBranchMispredictionPenalty(aName + "-mispredict:penalty")
  , theBranchMispredictionPenalty_TAGE(aName + "-mispredict:TAGE:penalty")
  , theBranchMispredictionPenalty_Indirect(aName + "-mispredict:Indirect:penalty")
  , theBranchMispredictionPenalty_Return(aName + "-mispredict:Return:penalty")
  , theBranchMispredictionPenalty_BTB(aName + "-mispredict:BTB:penalty")
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

    if (aBPState.thePrediction <= kTaken)
        return *theBTB.target(anAddress); 

    return aBPState.pc + 4;
}

void
BranchPredictor::recoverHistory(const BPredRedictRequest& aRequest)
{
    BPredState &aBPState = *aRequest.theBPState;

    RAS.recover(aBPState);

    theTage.restore_history(*aRequest.theBPState);

    if (!aRequest.theInsertNewHistory)
        return;

    if (aBPState.theActualDirection <= kTaken || aBPState.theActualType == kNonBranch)
        theBTB.update(aBPState.pc, aBPState.theActualType, aBPState.theActualTarget);

    if (aBPState.theActualType != kNonBranch || aBPState.thePredictedType != kNonBranch) {
        bool isTaken = aBPState.theActualDirection == kTaken;
        theTage.update_history(aBPState, isTaken, aBPState.pc);
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

void BranchPredictor::recordResyncRedirectStats() {
    theResyncRedirects++;
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
    aBPState.thePredictedTarget  = anAddress + 4;
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
        case kConditional:
            aBPState.thePredictedTarget = predictConditional(VirtualMemoryAddress(anAddress), aBPState);
            break;

        case kIndirectReg:
        case kUnconditional:
            aBPState.thePredictedTarget = *theBTB.target(anAddress);

            theTage.update_history(aBPState, true, anAddress);
            break;

        case kIndirectCall:
        case kCall:
            aBPState.thePredictedTarget = *theBTB.target(anAddress);

            // speculative
            RAS.push(anAddress + 4);

            theTage.update_history(aBPState, true, anAddress);
            break;

        case kReturn:
            if (RAS.valid()) {
                // speculative
                aBPState.thePredictedTarget = RAS.pop();
                aBPState.returnUsedRAS = true;
            } else
                aBPState.thePredictedTarget = *theBTB.target(anAddress);

            theTage.update_history(aBPState, true, anAddress);
            break;

        default:
            DBG_Assert(false, (<< "Unknown branch type: " << aBPState.thePredictedType));
            break;
    }

    return aBPState.thePredictedTarget;
}

void BranchPredictor::helperPenaltyCalculator(BPredState& aBPState, std::vector<std::pair<uint64_t, uint64_t>> &mispredictList, Stat::StatCounter &stat) {
    mispredictList.push_back(std::make_pair(aBPState.thePredCycle >> 8, aBPState.theCorrectionCycle));
    if (mispredictList.size() % 1000 == 0) {
        stat = calculateRedirectCycles(mispredictList);
    }
}

void
BranchPredictor::train(BPredState& aBPState)
{
    DBG_(VVerb, (<< "Training Branch Predictor by PC: " << std::hex << aBPState.pc));

    DBG_Assert(aBPState.theActualTarget != VirtualMemoryAddress(0));

    bool is_system = ((uint64_t)aBPState.pc >> 63) != 0;

    if (aBPState.theActualType != kNonBranch)
        ++theBranches;

    if (aBPState.theActualTarget != aBPState.thePredictedTarget) {
        if (!aBPState.theCorrectionCycle) {
            // Only happens when resync is at the same cycle as the commit
            aBPState.theCorrectionCycle = theFlexus->cycleCount();
        }
        helperPenaltyCalculator(aBPState, mispredictCycles, theBranchMispredictionPenalty);

        if (aBPState.theActualType != aBPState.thePredictedType) {
            switch (aBPState.theActualType) {
                case kIndirectReg:
                case kIndirectCall:
                    ++theMispredict_Indirect;
                    helperPenaltyCalculator(aBPState, mispredictCycles_Indirect, theBranchMispredictionPenalty_Indirect);
                    break;

                default:
                    ;
            }

            helperPenaltyCalculator(aBPState, mispredictCycles_BTB, theBranchMispredictionPenalty_BTB);
            ++theMispredict_BTB;
            if (is_system)
                ++theMispredict_BTB_System;
            else
                ++theMispredict_BTB_User;

        } else {
            switch (aBPState.theActualType) {
                case kIndirectReg:
                case kIndirectCall:
                    helperPenaltyCalculator(aBPState, mispredictCycles_Indirect, theBranchMispredictionPenalty_Indirect);
                    ++theMispredict_Indirect;

                case kCall:
                case kUnconditional:
                    helperPenaltyCalculator(aBPState, mispredictCycles_BTB, theBranchMispredictionPenalty_BTB);
                    ++theMispredict_BTB;
                    if (is_system)
                        ++theMispredict_BTB_System;
                    else
                        ++theMispredict_BTB_User;
                    break;

                case kReturn:
                    // suppose always using ras
                    helperPenaltyCalculator(aBPState, mispredictCycles_Return, theBranchMispredictionPenalty_Return);
                    ++theMispredict_Return;
                    break;

                case kConditional:
                    helperPenaltyCalculator(aBPState, mispredictCycles_TAGE, theBranchMispredictionPenalty_TAGE);
                    ++theMispredict_TAGE;
                    if (is_system)
                        ++theMispredict_TAGE_System;
                    else
                        ++theMispredict_TAGE_User;
                    break;

                default:
                    ;
            }
        }
    }

    if (aBPState.thePredictedType != kNonBranch || aBPState.theActualType != kNonBranch) {
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