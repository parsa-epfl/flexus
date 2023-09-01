//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <iostream>
#include <fstream>

#include <components/BPWarm/BPWarm.hpp>
#include <components/BPWarm/BPStat.hpp>
#include <components/CommonQEMU/Slices/ArchitecturalInstruction.hpp>
#include <components/CommonQEMU/BranchPredictor.hpp>
#include <core/qemu/mai_api.hpp>

#define FLEXUS_BEGIN_COMPONENT BPWarm
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nBPWarm {

using namespace Flexus;
using namespace Flexus::SharedTypes;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(BPWarm) {
  FLEXUS_COMPONENT_IMPL(BPWarm);

  std::unique_ptr<FastBranchPredictor> theBranchPredictor;

  std::vector<std::vector<uint32_t>> theOpcode; // Rakesh
  std::vector<std::vector<VirtualMemoryAddress>> theFetchAddress;
  std::vector<std::vector<BPredState>> theFetchState;
  std::vector<std::vector<eBranchType>> theFetchType;
  std::vector<bool> theOne;

  // MARK: Added for FDIP/Boomerang
  std::vector<VirtualMemoryAddress> theBBAddress;
  std::vector<VirtualMemoryAddress> theExpectedAddress;
  
  //std::vector<eBranchType> theLastBranch;

  //int latest_resolved_branch;
  //BPredState BBTBState;

  struct BPFeedback {
    VirtualMemoryAddress pc;
    eBranchType theFetchType;
    eDirection dir;
    VirtualMemoryAddress target;
    BPredState theFetchState;
  } theBPFeedback[UNRESOLVED_BRANCH_ARRAY_SIZE];

  BPWarm_stats *bStats;
  std::map<uint64_t, uint64_t> stats0;

  std::pair<eBranchType, bool> decode(uint32_t opcode) {
    std::pair<eBranchType, VirtualMemoryAddress> type_and_offset = targetDecode(opcode);
    return std::make_pair(type_and_offset.first, false);
  }

  void doPredict(index_t anIndex) {
    const bool anOne = theOne[anIndex];
    theBranchPredictor->predict(theFetchAddress[anIndex][anOne], theFetchState[anIndex][anOne]);
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(BPWarm) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  void initialize() {
    theFetchAddress.resize(cfg.Cores);
    theFetchState.resize(cfg.Cores);
    theFetchType.resize(cfg.Cores);
    theOne.resize(cfg.Cores);
    theBBAddress.resize(cfg.Cores);
    theExpectedAddress.resize(cfg.Cores);

    for (int32_t i = 0; i < cfg.Cores; i++) {
      theFetchAddress[i].resize(2);
      theFetchState[i].resize(2);
      theFetchType[i].resize(2);

      theFetchAddress[i][0] = VirtualMemoryAddress(0);
      theFetchAddress[i][1] = VirtualMemoryAddress(0);
      theFetchType[i][0] = kNonBranch;
      theFetchType[i][1] = kNonBranch;
      theOne[i] = false;
      theBBAddress[i] = VirtualMemoryAddress(0);
      theExpectedAddress[i] = VirtualMemoryAddress(0);
      
    }

    theBranchPredictor.reset(
        FastBranchPredictor::combining(statName(), flexusIndex(), cfg.BTBSets, cfg.BTBWays));
  }

  void finalize() {
  }

  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const &aDirName) {
    theBranchPredictor->saveState(aDirName);
  }

  void loadState(std::string const &aDirName) {
    theBranchPredictor->loadState(aDirName);
  }

public:
  ///////////////// ITraceInModern port
  bool available(interface::ITraceInModern const &, index_t anIndex) {
    return true;
  }
  
void dumpVariablesToFile(bool prediction, const BranchFeedback& branchReq, BPredState& FetchState, const char* filename) {
    std::ofstream outputFile(filename, std::ios::app); // Open the file for writing
  // PC theActualType theActualDirection thePredictedDirection theActualTarget

  if (outputFile.is_open()) {
      outputFile << (prediction?"Correct\t":"Mispredict\t") << branchReq.thePC << "\t" << branchReq.theActualType << "\t" << (FetchState.thePredictedTarget==kTaken?1:0) << "\t" << (branchReq.theActualDirection==kTaken?1:0) << "\t" << branchReq.theActualTarget << "\n" ;
      outputFile.close(); // Close the file
      // std::cout << "Variable values dumped to " << filename << " successfully.\n";
  } else {
      std::cerr << "Unable to open file for writing.\n";
  }


}

  void push(interface::ITraceInModern const &, index_t anIndex,
            MemoryMessage &aMessage) {
    BranchFeedback branchReq;
    bool prediction = false;
    branchReq.thePC = aMessage.pc();
    branchReq.theActualType = aMessage.branchType();
    branchReq.theActualDirection = aMessage.pc() + 4 == aMessage.targetpc() ? kNotTaken : kTaken;
    branchReq.theActualTarget = branchReq.theActualDirection == kTaken ? aMessage.targetpc() : VirtualMemoryAddress(0);


  
    if(branchReq.theActualType == kNonBranch && aMessage.targetpc() != aMessage.pc() + 4){
//      assert(0);
    }


    // Here is a HACK! It has to be fixed from QEMU side
    if(theExpectedAddress[anIndex] != 0 && (aMessage.pc() != theExpectedAddress[anIndex])){
      theBBAddress[anIndex] = aMessage.pc();
    }
    
    theExpectedAddress[anIndex] = aMessage.targetpc();

    if (branchReq.theActualType != kNonBranch) {
#ifdef PC_BTB
      assert(0);
      theBranchPredictor->predict(branchReq.thePC, theFetchState[anIndex][0]);
      prediction = theBranchPredictor->feedback(branchReq.thePC, branchReq.theActualType, branchReq.theActualDirection, branchReq.theActualTarget,
                                   theFetchState[anIndex][0], 0);
#endif
#ifdef BB_BTB
      int BBSize = (branchReq.thePC - theBBAddress[anIndex])/4 + 1;

      theBranchPredictor->predict(theBBAddress[anIndex], theFetchState[anIndex][0], BBSize);
      prediction = theBranchPredictor->feedback(theBBAddress[anIndex], branchReq.theActualType, branchReq.theActualDirection, branchReq.theActualTarget,
                                   theFetchState[anIndex][0], BBSize);
      theBBAddress[anIndex] = aMessage.targetpc();
#endif
    }

    if(branchReq.theActualType == kReturn){
      std::cout << "Act: " << branchReq.theActualTarget << "\tP: " << theFetchState[anIndex][0].thePredictedTarget << "\n";
      if(branchReq.theActualTarget == theFetchState[anIndex][0].thePredictedTarget){
        std::cout << "RAS works\n";
      }
      else{
        std::cout << "RAS doesn't work\n";
      }
    }
    theBranchPredictor->increaseInstCount();

        if(branchReq.theActualType != kNonBranch){
          dumpVariablesToFile(prediction, branchReq, theFetchState[anIndex][0], "/home/ninadjangle/Projects/qflex/flexus/tests/results/BPWarmTestResults.txt");
        }
      }
};

} // End namespace nBPWarm

FLEXUS_COMPONENT_INSTANTIATOR(BPWarm, nBPWarm);
FLEXUS_PORT_ARRAY_WIDTH(BPWarm, ITraceInModern) {
  return (cfg.Cores);
}
#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT BPWarm
