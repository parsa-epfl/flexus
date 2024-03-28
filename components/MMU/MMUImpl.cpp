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

// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <components/MMU/MMU.hpp>

#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>

#include "MMUUtil.hpp"
#include "pageWalk.hpp"
#include <components/CommonQEMU/Translation.hpp>

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <vector>

#define DBG_DefineCategories MMU
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()
#define DBG_DefineCategories TLBMissTracking
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace std {
template <> struct hash<VirtualMemoryAddress> {
  std::size_t operator()(const VirtualMemoryAddress &anAddress) const {
    return ((hash<uint64_t>()(uint64_t(anAddress))));
  }
};
} // namespace std

namespace nMMU {

using namespace Flexus;
using namespace Qemu;
using namespace Core;
using namespace SharedTypes;
uint64_t PAGEMASK;

class FLEXUS_COMPONENT(MMU) {
  FLEXUS_COMPONENT_IMPL(MMU);

private:
private:
  struct TLBentry {

    friend class boost::serialization::access;

    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theRate;
      ar &theVaddr;
      ar &thePaddr;
    }

    TLBentry() {
    }

    TLBentry(VirtualMemoryAddress anAddress) : theRate(0), theVaddr(anAddress) {
    }
    uint64_t theRate;
    VirtualMemoryAddress theVaddr;
    PhysicalMemoryAddress thePaddr;

    bool operator==(const TLBentry &other) const {
      return (theVaddr == other.theVaddr);
    }

    TLBentry &operator=(const PhysicalMemoryAddress &other) {
      PhysicalMemoryAddress otherAligned(other & PAGEMASK);
      thePaddr = otherAligned;
      return *this;
    }
  };

  struct TLB {

    friend class boost::serialization::access;

    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theTLB;
      ar &theSize;
    }

    std::pair<bool, PhysicalMemoryAddress> lookUp(const VirtualMemoryAddress &anAddress) {
      VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
      std::pair<bool, PhysicalMemoryAddress> ret{false, PhysicalMemoryAddress(0)};
      for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
        iter->second.theRate++;
        if (iter->second.theVaddr == anAddressAligned) {
          iter->second.theRate = 0;
          ret.first = true;
          ret.second = iter->second.thePaddr;
        }
      }
      return ret;
    }

    TLBentry &operator[](VirtualMemoryAddress anAddress) {
      VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
      auto iter = theTLB.find(anAddressAligned);
      if (iter == theTLB.end()) {
        size_t s = theTLB.size();
        if (s == theSize) {
          evict();
        }
        std::pair<tlbIterator, bool> result;
        result = theTLB.insert({anAddressAligned, TLBentry(anAddressAligned)});
        assert(result.second);
        iter = result.first;
      }
      return iter->second;
    }

    void resize(size_t aSize) {
      theTLB.reserve(aSize);
      theSize = aSize;
    }

    size_t capacity() {
      return theSize;
    }

    void clear() {
      theTLB.clear();
    }

    size_t size() {
      return theTLB.size();
    }

  private:
    void evict() {
      auto res = theTLB.begin();
      for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
        if (iter->second.theRate > res->second.theRate) {
          res = iter;
        }
      }
      theTLB.erase(res);
    }

    size_t theSize;
    std::unordered_map<VirtualMemoryAddress, TLBentry> theTLB;
    typedef std::unordered_map<VirtualMemoryAddress, TLBentry>::iterator tlbIterator;
  };

  std::unique_ptr<PageWalk> thePageWalker;
  TLB theInstrTLB;
  TLB theDataTLB;

  std::queue<boost::intrusive_ptr<Translation>> theLookUpEntries;
  std::queue<boost::intrusive_ptr<Translation>> thePageWalkEntries;
  std::set<VirtualMemoryAddress> alreadyPW;
  std::vector<boost::intrusive_ptr<Translation>> standingEntries;

  Flexus::Qemu::Processor theCPU;
  std::shared_ptr<mmu_t> theMMU;

  bool theMMUInitialized;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MMU) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  bool isQuiesced() const {
    return false;
  }

  void saveState(std::string const &aDirName) {
    std::ofstream iFile, dFile;
    iFile.open(aDirName + "/" + statName() + "-itlb", std::ofstream::out | std::ofstream::app);
    dFile.open(aDirName + "/" + statName() + "-dtlb", std::ofstream::out | std::ofstream::app);

    boost::archive::text_oarchive ioarch(iFile);
    boost::archive::text_oarchive doarch(dFile);

    ioarch << theInstrTLB;
    doarch << theDataTLB;

    iFile.close();
    dFile.close();
  }

  void loadState(std::string const &aDirName) {
    std::ifstream iFile, dFile;
    iFile.open(aDirName + "/" + statName() + "-itlb", std::ifstream::in);
    dFile.open(aDirName + "/" + statName() + "-dtlb", std::ifstream::in);

    boost::archive::text_iarchive iiarch(iFile);
    boost::archive::text_iarchive diarch(dFile);

    uint64_t iSize = theInstrTLB.capacity();
    uint64_t dSize = theDataTLB.capacity();
    iiarch >> theInstrTLB;
    diarch >> theDataTLB;
    if (iSize != theInstrTLB.capacity()) {
      theInstrTLB.clear();
      theInstrTLB.resize(iSize);
      DBG_(Dev, (<< "Changing iTLB capacity from " << theInstrTLB.capacity() << " to " << iSize));
    }
    if (dSize != theDataTLB.capacity()) {
      theDataTLB.clear();
      theDataTLB.resize(dSize);
      DBG_(Dev, (<< "Changing dTLB capacity from " << theInstrTLB.capacity() << " to " << iSize));
    }
    DBG_(Dev, (<< "Size - iTLB:" << theInstrTLB.capacity() << ", dTLB:" << theDataTLB.capacity()));

    iFile.close();
    dFile.close();
  }

  // Initialization
  void initialize() {
    theCPU = Flexus::Qemu::Processor::getProcessor(flexusIndex());
    thePageWalker.reset(new PageWalk(flexusIndex()));
    thePageWalker->setMMU(theMMU);
    theMMUInitialized = false;
    theInstrTLB.resize(cfg.iTLBSize);
    theDataTLB.resize(cfg.dTLBSize);
  }

  void finalize() {
  }

  // MMUDrive
  //----------
  void drive(interface::MMUDrive const &) {
    DBG_(VVerb, Comp(*this)(<< "MMUDrive"));
    busCycle();
    thePageWalker->cycle();
    processMemoryRequests();
  }

  void busCycle() {

    while (!theLookUpEntries.empty()) {

      TranslationPtr item = theLookUpEntries.front();
      theLookUpEntries.pop();
      DBG_(VVerb, (<< "Processing lookup entry for " << item->theVaddr));
      DBG_Assert(item->isInstr() != item->isData());

      DBG_(VVerb, (<< "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry "
                   << item->theVaddr));

      std::pair<bool, PhysicalMemoryAddress> entry =
          (item->isInstr() ? theInstrTLB : theDataTLB)
              .lookUp((VirtualMemoryAddress)(item->theVaddr));
      if (cfg.PerfectTLB) {
        PhysicalMemoryAddress perfectPaddr(API::qemu_api.translate_va2pa(
          flexusIndex(),
          item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
          item->theVaddr));
        entry.first = true;
        entry.second = perfectPaddr;
        if (perfectPaddr == 0xFFFFFFFFFFFFFFFF)
          item->setPagefault();
      }
      if (entry.first) {
        DBG_(VVerb, (<< "Item is a Hit " << item->theVaddr));

        // item exists so mark hit
        item->setHit();
        item->thePaddr = (PhysicalMemoryAddress)(entry.second | (item->theVaddr & ~(PAGEMASK)));

        if (item->isInstr())
          FLEXUS_CHANNEL(iTranslationReply) << item;
        else
          FLEXUS_CHANNEL(dTranslationReply) << item;
      } else {
        DBG_(VVerb, (<< "Item is a miss " << item->theVaddr));

        VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
        if (alreadyPW.find(pageAddr) == alreadyPW.end()) {
          // mark miss
          item->setMiss();
          if (thePageWalker->push_back(item)) {
            alreadyPW.insert(pageAddr);
            thePageWalkEntries.push(item);
          } else {
            PhysicalMemoryAddress perfectPaddr(API::qemu_api.translate_va2pa(
                flexusIndex(),
                item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
                item->theVaddr));
            item->setHit();
            item->thePaddr = perfectPaddr;
            if (item->isInstr())
              FLEXUS_CHANNEL(iTranslationReply) << item;
            else
              FLEXUS_CHANNEL(dTranslationReply) << item;
          }
        } else {
          standingEntries.push_back(item);
        }
      }
    }

    while (!thePageWalkEntries.empty()) {

      TranslationPtr item = thePageWalkEntries.front();
      DBG_(VVerb, (<< "Processing PW entry for " << item->theVaddr));

      if (item->isAnnul()) {
        DBG_(VVerb, (<< "Item was annulled " << item->theVaddr));
        thePageWalkEntries.pop();
      } else if (item->isDone()) {
        DBG_(VVerb, (<< "Item was Done translationg " << item->theVaddr));

        CORE_DBG("MMU: Translation is Done " << item->theVaddr << " -- " << item->thePaddr);

        thePageWalkEntries.pop();

        VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
        if (alreadyPW.find(pageAddr) != alreadyPW.end()) {
          alreadyPW.erase(pageAddr);
          for (auto it = standingEntries.begin(); it != standingEntries.end();) {
            if (((*it)->theVaddr & pageAddr) == pageAddr && item->isInstr() == (*it)->isInstr()) {
              theLookUpEntries.push(*it);
              standingEntries.erase(it);
            } else {
              ++it;
            }
          }
        }
        DBG_Assert(item->isInstr() != item->isData());
        DBG_(Iface, (<< "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry "
                     << item->theVaddr));
        (item->isInstr() ? theInstrTLB : theDataTLB)[(VirtualMemoryAddress)(item->theVaddr)] =
            (PhysicalMemoryAddress)(item->thePaddr);
        if (item->isInstr())
          FLEXUS_CHANNEL(iTranslationReply) << item;
        else
          FLEXUS_CHANNEL(dTranslationReply) << item;
      } else {
        break;
      }
    }
  }

  void processMemoryRequests() {
    CORE_TRACE;
    while (thePageWalker->hasMemoryRequest()) {
      TranslationPtr tmp = thePageWalker->popMemoryRequest();
      DBG_(VVerb,
           (<< "Sending a Memory Translation request to Core ready(" << tmp->isReady() << ")  "
            << tmp->theVaddr << " -- " << tmp->thePaddr << "  -- ID " << tmp->theID));
      FLEXUS_CHANNEL(MemoryRequestOut) << tmp;
    }
  }

  void
  resyncMMU(std::size_t anIndex)
  {
    CORE_TRACE;
    DBG_(VVerb, (<< "Resynchronizing MMU"));

    static bool optimize = false;
    theMMUInitialized = optimize;

    if (!theMMUInitialized) {
      theMMU.reset(new mmu_t());
      theMMUInitialized = true;
    }
    theMMU->init_mmu_regs(anIndex);
    theMMU->setupAddressSpaceSizesAndGranules();
    DBG_Assert(theMMU->Gran0->getlogKBSize() == 12, (<< "TG0 has non-4KB size - unsupported"));
    DBG_Assert(theMMU->Gran1->getlogKBSize() == 12, (<< "TG1 has non-4KB size - unsupported"));
    PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
    if (thePageWalker) {
      DBG_(VVerb, (<< "Annulling all PW entries"));
      thePageWalker->annulAll();
    }
    if (thePageWalkEntries.size() > 0) {
      DBG_(VVerb, (<< "deleting PageWalk Entries"));
      while (!thePageWalkEntries.empty()) {
        DBG_(VVerb, (<< "deleting MMU PageWalk Entrie " << thePageWalkEntries.front()));
        thePageWalkEntries.pop();
      }
    }
    alreadyPW.clear();
    standingEntries.clear();

    if (theLookUpEntries.size() > 0) {
      while (!theLookUpEntries.empty()) {
        DBG_(VVerb, (<< "deleting MMU Lookup Entrie " << theLookUpEntries.front()));
        theLookUpEntries.pop();
      }
    }
    FLEXUS_CHANNEL(ResyncOut) << (int&)anIndex;
  }

  bool IsTranslationEnabledAtEL(uint8_t &anEL) {
    return true; // theCore->IsTranslationEnabledAtEL(anEL);
  }

  bool available(interface::ResyncIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::ResyncIn const &, index_t anIndex, int &aResync) {
    resyncMMU(aResync);
  }

  bool available(interface::iRequestIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::iRequestIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    CORE_DBG("MMU: Instruction RequestIn");

    aTranslate->theIndex = anIndex;
    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
  }

  bool available(interface::dRequestIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::dRequestIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    CORE_DBG("MMU: Data RequestIn");

    aTranslate->theIndex = anIndex;

    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
  }

  void sendTLBresponse(TranslationPtr aTranslation) {
    if (aTranslation->isInstr()) {
      FLEXUS_CHANNEL(iTranslationReply) << aTranslation;
    } else {
      FLEXUS_CHANNEL(dTranslationReply) << aTranslation;
    }
  }

  bool available(interface::TLBReqIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::TLBReqIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    if (!cfg.PerfectTLB && (aTranslate->isInstr() ? theInstrTLB : theDataTLB).lookUp(aTranslate->theVaddr).first == false)
    {
      if (!theMMUInitialized)
      {
        theMMU.reset(new mmu_t());
        theMMU->init_mmu_regs(flexusIndex());
        theMMU->setupAddressSpaceSizesAndGranules();
        DBG_Assert(theMMU->Gran0->getlogKBSize() == 12, (<< "TG0 has non-4KB size - unsupported"));
        DBG_Assert(theMMU->Gran1->getlogKBSize() == 12, (<< "TG1 has non-4KB size - unsupported"));
        PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
        thePageWalker->setMMU(theMMU);
        theMMUInitialized = true;
      }

      thePageWalker->push_back_trace(aTranslate, Flexus::Qemu::Processor::getProcessor(flexusIndex()));
      (aTranslate->isInstr() ? theInstrTLB : theDataTLB)[aTranslate->theVaddr] =
          aTranslate->thePaddr;
    }
  }
};

} // End Namespace nMMU

FLEXUS_COMPONENT_INSTANTIATOR(MMU, nMMU);
FLEXUS_PORT_ARRAY_WIDTH(MMU, dRequestIn) {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, iRequestIn) {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, ResyncIn) {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, iTranslationReply) {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, dTranslationReply) {
  return (cfg.Cores);
}

FLEXUS_PORT_ARRAY_WIDTH(MMU, MemoryRequestOut) {
  return (cfg.Cores);
}

FLEXUS_PORT_ARRAY_WIDTH(MMU, TLBReqIn) {
  return (cfg.Cores);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MMU

#define DBG_Reset
#include DBG_Control()
