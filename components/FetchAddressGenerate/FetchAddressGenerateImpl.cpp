// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

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
//  std::vector<uint32_t> theConvertedInstruction;
#if FLEXUS_TARGET_IS(v9)
  std::vector<MemoryAddress> theNextPC;
#endif
  std::vector<MemoryAddress> theRedirectPC;
#if FLEXUS_TARGET_IS(v9)
  std::vector<MemoryAddress> theRedirectNextPC;
#endif
  std::vector<bool> theRedirect;
  std::unique_ptr<BranchPredictor> theBranchPredictor;
  uint32_t theCurrentThread;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FetchAddressGenerate) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  void initialize() {
    thePC.resize(cfg.Threads);
    // theConvertedInstruction.resize(cfg.Threads);
#if FLEXUS_TARGET_IS(v9)
    theNextPC.resize(cfg.Threads);
#endif
    theRedirectPC.resize(cfg.Threads);
#if FLEXUS_TARGET_IS(v9)
    theRedirectNextPC.resize(cfg.Threads);
#endif
    theRedirect.resize(cfg.Threads);
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      Qemu::Processor cpu = Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + i);
      thePC[i] = cpu->getPC();
      AGU_DBG("PC(" << i << ") = " << thePC[i]);
#if FLEXUS_TARGET_IS(v9)
      theNextPC[i] = cpu->getNPC();
#endif
      theRedirectPC[i] = MemoryAddress(0);
#if FLEXUS_TARGET_IS(v9)
      theRedirectNextPC[i] = MemoryAddress(0);
#endif
      theRedirect[i] = false;
    }
    theCurrentThread = cfg.Threads;
    theBranchPredictor.reset(BranchPredictor::combining(statName(), flexusIndex()));
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

#if FLEXUS_TARGET_IS(ARM)
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
  void push(interface::RedirectIn const &, index_t anIndex, MemoryAddress &aRedirect) {
    theRedirectPC[anIndex] = aRedirect;
    theRedirect[anIndex] = true;
  }
#elif FLEXUS_TARGET_IS(v9)
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
  void push(interface::RedirectIn const &, index_t anIndex,
            std::pair<MemoryAddress, MemoryAddress> &aRedirect) {
    theRedirectPC[anIndex] = aRedirect.first;
    theRedirectNextPC[anIndex] = aRedirect.second;
    theRedirect[anIndex] = true;
  }
#endif

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

    if (theFlexus->quiescing()) {
      DBG_(VVerb, (<< "FGU: Flexus is quiescing!.. come back later"));
      return;
    }

    if (theRedirect[anIndex]) {
      thePC[anIndex] = theRedirectPC[anIndex];
#if FLEXUS_TARGET_IS(v9)
      theNextPC[anIndex] = theRedirectNextPC[anIndex];
#endif
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
#if FLEXUS_TARGET_IS(v9)

        thePC[anIndex] = theNextPC[anIndex];
        theNextPC[anIndex] = theBranchPredictor->predict(faddr);
        if (theNextPC[anIndex] == 0) {
          theNextPC[anIndex] = thePC[anIndex] + 4;
        }
#else
        VirtualMemoryAddress prediction = theBranchPredictor->predict(faddr);
        if (prediction == 0)
          thePC[anIndex] += 4;
        else
          thePC[anIndex] = prediction;
#endif
        AGU_DBG("Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex);
        AGU_DBG("Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress);

        fetch->theFetches.push_back(faddr);
        --max_predicts;
      } else {
#if FLEXUS_TARGET_IS(v9)
        thePC[anIndex] = theNextPC[anIndex];
        theNextPC[anIndex] = thePC[anIndex] + 4;
#else
        DBG_(VVerb, (<< "Before Advancing PC to: " << thePC[anIndex] << " for core: " << anIndex));

        thePC[anIndex] += 4;
#endif

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
