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
#include <components/DecoupledFeederQEMU/DecoupledFeeder.hpp>

#include <components/DecoupledFeederQEMU/QemuTracer.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/qemu/api_wrappers.hpp>

#include <core/flexus.hpp>
#include <core/stats.hpp>

#include <functional>

#define DBG_DefineCategories Feeder
#define DBG_SetDefaultOps AddCat(Feeder)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDecoupledFeeder {

using namespace Flexus;
using namespace Flexus::Qemu;
extern "C" {
void houseKeeping(void *, void *, void *);
}

struct MMUStats {
  Stat::StatCounter theTotalReqs_stat;
  Stat::StatCounter theHit_stat;
  Stat::StatCounter theMiss_stat;
  Stat::StatCounter theMemAccess_stat;

  Stat::StatCounter ttwHitL1;
  Stat::StatCounter ttwHitPeerL1;
  Stat::StatCounter ttwHitL2;
  Stat::StatCounter ttwHitMem;

  Stat::StatCounter datHitL1;
  Stat::StatCounter datHitPeerL1;
  Stat::StatCounter datHitL2;
  Stat::StatCounter datHitMem;

  int64_t theTotalReqs;
  int64_t theHit;
  int64_t theMiss;
  int64_t theMemAccess;

  MMUStats(std::string const &aName)
      : theTotalReqs_stat(aName + "TotalReqs"), theHit_stat(aName + "Hits"),
        theMiss_stat(aName + "Misses"), theMemAccess_stat(aName + "MemAccesses"),
        ttwHitL1(aName + "ttwHitL1"),
        ttwHitPeerL1(aName + "ttwHitPeerL1"),
        ttwHitL2(aName + "ttwHitL2"),
        ttwHitMem(aName + "ttwHitMem"),
        datHitL1(aName + "datHitL1"),
        datHitPeerL1(aName + "datHitPeerL1"),
        datHitL2(aName + "datHitL2"),
        datHitMem(aName + "datHitMem"),
        theTotalReqs(0),
        theHit(0), theMiss(0), theMemAccess(0) {
  }

  void update() {
    theTotalReqs_stat += theTotalReqs;
    theHit_stat += theHit;
    theMiss_stat += theMiss;
    theMemAccess_stat += theMemAccess;
    theTotalReqs = 0;
    theHit = 0;
    theMiss = 0;
    theMemAccess = 0;
  }
};

class DecoupledFeederComponent;

static DecoupledFeederComponent *theFeeder = NULL;

class FLEXUS_COMPONENT(DecoupledFeeder) {
  FLEXUS_COMPONENT_IMPL(DecoupledFeeder);

  // The Qemu objects (one for each processor) for getting trace data
  int32_t theNumCPUs;
  int32_t theCMPWidth;
  QemuTracerManager *theTracer;
  MMUStats **os_itlb_stats, **os_dtlb_stats, **user_itlb_stats, **user_dtlb_stats;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeeder) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
    theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();

    theTracer = QemuTracerManager::construct(
        theNumCPUs,
        [this](int32_t x, MemoryMessage &y) {
          this->toL1D(x, y);
        }, // std::bind( &DecoupledFeederComponent::toL1D, this, _1, _2)
        [this](int32_t x, MemoryMessage &y, uint32_t dummy) {
          this->modernToL1I(x, y);
        }, // std::bind( &DecoupledFeederComponent::modernToL1I, this, _1, _2)
        [this](MemoryMessage &x) {
          this->toDMA(x);
        }, // std::bind( &DecoupledFeederComponent::toDMA, this, _1)
        [this](int32_t x, MemoryMessage &y) {
          this->toNAW(x, y);
        }, // std::bind( &DecoupledFeederComponent::toNAW, this, _1, _2)
        //, cfg.WhiteBoxDebug
        //, cfg.WhiteBoxPeriod
        cfg.SendNonAllocatingStores);
    //    printf("Is the FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeeder) run?\n");
    Flexus::SharedTypes::MemoryMessage msg(MemoryMessage::LoadReq);
    //    DecoupledFeederComponent::toL1D((int32_t) 0, msg);

    //  printf("toL1D %p\n", DecoupledFeederComponent::toL1D);
    theFeeder = this;
  }

  // InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize(void) {
    DBG_(VVerb, (<< "Inititializing Decoupled feeder..."));

    // Disable cycle-callback
    // Flexus::Qemu::theQemuInterface->disableCycleHook();
    //    printf("Decoupled feeder intialized? START...");
    // theTracer->setQemuQuantum(cfg.QemuQuantum);
    if (cfg.TrackIFetch) {
      theTracer->enableInstructionTracing();
    }

    if (cfg.SystemTickFrequency > 0.0) {
      theTracer->setSystemTick(cfg.SystemTickFrequency);
    }

    os_itlb_stats = new MMUStats *[theNumCPUs];
    os_dtlb_stats = new MMUStats *[theNumCPUs];
    user_itlb_stats = new MMUStats *[theNumCPUs];
    user_dtlb_stats = new MMUStats *[theNumCPUs];
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      os_itlb_stats[i] = new MMUStats(boost::padded_string_cast<2, '0'>(i) + "-itlb-OS:");
      os_dtlb_stats[i] = new MMUStats(boost::padded_string_cast<2, '0'>(i) + "-dtlb-OS:");
      user_itlb_stats[i] = new MMUStats(boost::padded_string_cast<2, '0'>(i) + "-itlb-User:");
      user_dtlb_stats[i] = new MMUStats(boost::padded_string_cast<2, '0'>(i) + "-dtlb-User:");
    }

    theFlexus->advanceCycles(0);
    theCMPWidth = cfg.CMPWidth;
    if (theCMPWidth == 0) {
      theCMPWidth = Qemu::API::qemu_api.get_num_cores();
    }
  }

  void finalize(void) {
  }

  std::pair<uint64_t, uint32_t> theFetchInfo;

  void toL1D(int32_t anIndex, MemoryMessage &aMessage) {
    TranslationPtr tr(new Translation);
    tr->setData();
    tr->theType = aMessage.type() == MemoryMessage::LoadReq ? Translation::eLoad : Translation::eStore;
    tr->theVaddr = aMessage.pc();
    tr->thePaddr = aMessage.address();
    tr->inTraceMode = true;

    FLEXUS_CHANNEL_ARRAY(ToMMU, anIndex) << tr;
    MemoryMessage theMMUMemoryMessage(aMessage);
    theMMUMemoryMessage.setPageWalk();
    bool isHit = true;
    aMessage.isPriv() ? os_dtlb_stats[anIndex]->theTotalReqs++
                      : user_dtlb_stats[anIndex]->theTotalReqs++;
    while (tr->trace_addresses.size()) {
      theMMUMemoryMessage.type() = MemoryMessage::LoadReq;
      theMMUMemoryMessage.address() = tr->trace_addresses.front();
      tr->trace_addresses.pop();
      FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << theMMUMemoryMessage;
      aMessage.isPriv() ? os_dtlb_stats[anIndex]->theMemAccess++
                        : user_dtlb_stats[anIndex]->theMemAccess++;
      isHit = false;
    }
    if (isHit)
      aMessage.isPriv() ? os_dtlb_stats[anIndex]->theHit++ : user_dtlb_stats[anIndex]->theHit++;
    else
      aMessage.isPriv() ? os_dtlb_stats[anIndex]->theMiss++ : user_dtlb_stats[anIndex]->theMiss++;

    FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << aMessage;
  }

  void toNAW(int32_t anIndex, MemoryMessage &aMessage) {
    FLEXUS_CHANNEL_ARRAY(ToNAW, anIndex / theCMPWidth) << aMessage;
  }

  void toDMA(MemoryMessage &aMessage) {
    FLEXUS_CHANNEL(ToDMA) << aMessage;
  }
  /*
  void toL1I(int32_t anIndex, MemoryMessage & aMessage, uint32_t anOpcode) {
    FLEXUS_CHANNEL_ARRAY( ToL1I, anIndex ) << aMessage;
    theFetchInfo.first = aMessage.pc();
    theFetchInfo.second = anOpcode;
    FLEXUS_CHANNEL_ARRAY( ToBPred, anIndex ) << theFetchInfo;
  }
  */
  void modernToL1I(int32_t anIndex, MemoryMessage &aMessage) {
    TranslationPtr tr(new Translation);
    tr->setInstr();
    tr->theType = Translation::eFetch;
    tr->theVaddr = aMessage.pc();
    tr->thePaddr = aMessage.address();
    tr->inTraceMode = true;

    FLEXUS_CHANNEL_ARRAY(ToMMU, anIndex) << tr;
    MemoryMessage theMMUMemoryMessage(MemoryMessage::LoadReq);
    theMMUMemoryMessage.setPageWalk();
    bool isHit = true;
    aMessage.isPriv() ? os_itlb_stats[anIndex]->theTotalReqs++
                      : user_itlb_stats[anIndex]->theTotalReqs++;
    while (tr->trace_addresses.size()) {
      theMMUMemoryMessage.type() = MemoryMessage::LoadReq;
      theMMUMemoryMessage.address() = tr->trace_addresses.front();
      tr->trace_addresses.pop();
      FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << theMMUMemoryMessage;
      aMessage.isPriv() ? os_itlb_stats[anIndex]->theMemAccess++
                        : user_itlb_stats[anIndex]->theMemAccess++;
      isHit = false;
    }
    if (isHit)
      aMessage.isPriv() ? os_itlb_stats[anIndex]->theHit++ : user_itlb_stats[anIndex]->theHit++;
    else
      aMessage.isPriv() ? os_itlb_stats[anIndex]->theMiss++ : user_itlb_stats[anIndex]->theMiss++;

    FLEXUS_CHANNEL_ARRAY(ToL1I, anIndex) << aMessage;

    pc_type_annul_triplet thePCTypeAndAnnulTriplet;
    thePCTypeAndAnnulTriplet.first = aMessage.pc();

    std::pair<uint32_t, uint32_t> theTypeAndAnnulPair;
    theTypeAndAnnulPair.first = (uint32_t)aMessage.branchType();
    theTypeAndAnnulPair.second = (uint32_t)aMessage.branchAnnul();

    thePCTypeAndAnnulTriplet.second = theTypeAndAnnulPair;

    FLEXUS_CHANNEL_ARRAY(ToBPred, anIndex) << thePCTypeAndAnnulTriplet;
  }
  void updateInstructionCounts() {
    // Count instructions
    // FIXME Currently Does nothing since step_count ha not been implemented
  }
  void updateMMUStats() {
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      os_itlb_stats[i]->update();
      os_dtlb_stats[i]->update();
      user_itlb_stats[i]->update();
      user_dtlb_stats[i]->update();
    }
  }

  void doHousekeeping() {
    updateMMUStats();
    updateInstructionCounts();
    theTracer->updateStats();

    theFlexus->advanceCycles(cfg.HousekeepingPeriod);
    theFlexus->invokeDrives();
  }

  void OnPeriodicEvent(Qemu::API::conf_object_t *ignored, long long aPeriod) {
    doHousekeeping();
  }

  // typedef Qemu::HapToMemFnBinding<Qemu::HAPs::Core_Periodic_Event, self,
  // &self::OnPeriodicEvent> periodic_hap_t; periodic_hap_t * thePeriodicHap;

}; // end class DecoupledFeeder
extern "C" {
void houseKeeping(void *obj, void *ign, void *ign2) {
  static_cast<DecoupledFeederComponent *>(obj)->doHousekeeping();
}
}
} // end Namespace nDecoupledFeeder

// extern "C" {
// void houseKeeping(void* obj, void * ign, void* ign2){
//    printf("Is houseKeeping being run?  1\n");
//        nDecoupledFeeder::houseKeep(obj);
//}
//}
FLEXUS_COMPONENT_INSTANTIATOR(DecoupledFeeder, nDecoupledFeeder);
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToL1D) {
  //  printf("DecoupldFeeder 1\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToL1I) {
  //   printf("DecoupldFeeder 2\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToBPred) {
  //   printf("DecoupldFeeder 3\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToNAW) {
  //   printf("DecoupldFeeder 4\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToMMU) {
  //   printf("DecoupldFeeder 4\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder

#define DBG_Reset
#include DBG_Control()