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

#include <components/FetchAddressGenerate/FetchAddressGenerate.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FetchAddressGenerate
#define DBG_SetDefaultOps AddCat(FetchAddressGenerate)
#include DBG_Control()

#include <core/flexus.hpp>
#include <core/qemu/mai_api.hpp>

#include <components/CommonQEMU/BranchPredictor.hpp>
#include <components/MTManager/MTManager.hpp>

namespace nFetchAddressGenerate {

using namespace Flexus;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(FetchAddressGenerate) {
  FLEXUS_COMPONENT_IMPL(FetchAddressGenerate);

  std::vector<MemoryAddress> thePC;
  std::vector<MemoryAddress> theRedirectPC;
  std::vector<bool> theRedirect;
  std::unique_ptr<BranchPredictor> theBranchPredictor;
  uint32_t theCurrentThread;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FetchAddressGenerate) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  void initialize() {
    thePC.resize(cfg.Threads);
    theRedirectPC.resize(cfg.Threads);
    theRedirect.resize(cfg.Threads);
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      Qemu::Processor cpu = Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + i);
      thePC[i] = cpu.getPC();
      AGU_DBG("PC(" << i << ") = " << thePC[i]);
      theRedirectPC[i] = MemoryAddress(0);
      theRedirect[i] = false;
    }
    theCurrentThread = cfg.Threads;
    theBranchPredictor.reset(
        BranchPredictor::combining(statName(), flexusIndex(), cfg.BTBSets, cfg.BTBWays));
  }

  void finalize() {
  }

  bool isQuiesced() const {
    // the FAG is always quiesced.
    return true;
  }

  void saveState(std::string const &aDirName) {
    theBranchPredictor->saveState(aDirName);
  }

  void loadState(std::string const &aDirName) {
    theBranchPredictor->loadState(aDirName);
  }

public:
  // RedirectIn
  //----------

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
  void push(interface::RedirectIn const &, index_t anIndex, MemoryAddress &aRedirect) {
    theRedirectPC[anIndex] = aRedirect;
    theRedirect[anIndex] = true;
  }

  // BranchFeedbackIn
  //----------------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BranchFeedbackIn);
  void push(interface::BranchFeedbackIn const &, index_t anIndex,
            boost::intrusive_ptr<BranchFeedback> &aFeedback) {
    theBranchPredictor->feedback(*aFeedback);
  }

  // Drive Interfaces
  //----------------
  // The FetchDrive drive interface sends a commands to the Feeder and then
  // fetches instructions, passing each instruction to its FetchOut port.
  void drive(interface::FAGDrive const &) {
    int32_t td = 0;
    if (cfg.Threads > 1) {
      td = nMTManager::MTManager::get()->scheduleFAGThread(flexusIndex());
    }
    doAddressGen(td);
  }

private:
  // Implementation of the FetchDrive drive interface
  void doAddressGen(index_t anIndex) {

    AGU_DBG("--------------START ADDRESS GEN------------------------");

    DBG_Assert(FLEXUS_CHANNEL(uArchHalted).available());
    bool cpu_in_halt = false;
    FLEXUS_CHANNEL(uArchHalted) >> cpu_in_halt;
    if (cpu_in_halt) {
      return;
    }

    if (theFlexus->quiescing()) {
      DBG_(VVerb, (<< "FGU: Flexus is quiescing!.. come back later"));
      return;
    }

    if (theRedirect[anIndex]) {
      thePC[anIndex] = theRedirectPC[anIndex];
      theRedirect[anIndex] = false;
      DBG_(VVerb, Comp(*this)(<< "FGU:  Redirect core[" << anIndex << "] " << thePC[anIndex]));
    }
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex).available());
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex).available());
    int32_t available_faq = 0;
    FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex) >> available_faq;

    int32_t max_addrs = cfg.MaxFetchAddress;
    if (max_addrs > available_faq) {
      DBG_(VVerb, (<< "FGU: max address: " << max_addrs
                   << " is greater than available queue: " << available_faq));
      max_addrs = available_faq;
    }
    int32_t max_predicts = cfg.MaxBPred;

    if (available_faq == 0)
      DBG_(VVerb, (<< "FGU: available FAQ is empty"));

    //    static int test;
    boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
    while (max_addrs > 0 /*&& test == 0*/) {
      AGU_DBG("Getting addresses: " << max_addrs << " remaining");

      FetchAddr faddr(thePC[anIndex]);

      // Advance the PC
      if (theBranchPredictor->isBranch(faddr.theAddress)) {
        AGU_DBG("Predicting a Branch");
        if (max_predicts == 0) {
          AGU_DBG("Config set the max prediction to zero, so no prediction");
          break;
        }
        VirtualMemoryAddress prediction = theBranchPredictor->predict(faddr);
        if (prediction == 0)
          thePC[anIndex] += 4;
        else
          thePC[anIndex] = prediction;
        AGU_DBG("Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex);
        AGU_DBG("Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress);

        fetch->theFetches.push_back(faddr);
        --max_predicts;
      } else {
        DBG_(VVerb, (<< "Before Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex));
        thePC[anIndex] += 4;

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
  bool pull(Stalled const &, index_t anIndex) {
    int32_t available_faq = 0;
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex).available());
    FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex) >> available_faq;
    return available_faq == 0;
  }
};
} // End namespace nFetchAddressGenerate

FLEXUS_COMPONENT_INSTANTIATOR(FetchAddressGenerate, nFetchAddressGenerate);

FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, RedirectIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, BranchFeedbackIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, FetchAddrOut) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, AvailableFAQ) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, Stalled) {
  return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

#define DBG_Reset
#include DBG_Control()
