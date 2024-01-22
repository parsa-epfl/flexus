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

#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <core/qemu/bitUtilities.hpp>

#include "MMU.hpp"
#include "PageWalk.hpp"

#define DBG_DefineCategories MMU
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()
#define DBG_DefineCategories TLBMissTracking
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nMMU {

using namespace Flexus;
using namespace Core;
using namespace Qemu;
using namespace SharedTypes;

static inline uint64_t mask(uint64_t i) {
  return (1lu << i) - 1lu;
}

class FLEXUS_COMPONENT(MMU) {
  FLEXUS_COMPONENT_IMPL(MMU);

private:
  struct TlbEntry {
    friend class boost::serialization::access;

    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theLru;
      ar &theAsid;
      ar &theAttr;
      ar &theVaddr;
      ar &thePaddr;
    }

    TlbEntry() {}

    uint64_t theLru;
    uint64_t theAsid;
    uint64_t theAttr;
    uint64_t theVaddr;
    uint64_t thePaddr;

    void set(uint64_t vaddr,
             uint64_t paddr,
             uint64_t asid,
             uint64_t attr,
             uint64_t lru) {
      theLru   = lru;
      theAsid  = asid;
      theAttr  = attr;
      theVaddr = vaddr;
      thePaddr = paddr;
    }

    bool hit(uint64_t base,
             uint64_t asid) {
      return ((theVaddr == base) &&
              (theAsid  == asid));
    }

    PhysicalMemoryAddress translate(const VirtualMemoryAddress &anAddress) const {
      return PhysicalMemoryAddress(thePaddr + (anAddress & ((1lu << 12) - 1lu)));
    }
  };

  struct VlbEntry {
    friend class boost::serialization::access;

    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theLru;
      ar &theAsid;
      ar &theCsid;
      ar &theAttr;
      ar &theBase;
      ar &theBound;
      ar &theOffs;
    }

    VlbEntry() {}

    uint64_t theLru;
    uint64_t theAsid;
    uint64_t theCsid;
    uint64_t theAttr;
    uint64_t theBase;
    uint64_t theBound;
    uint64_t theOffs;

    void set(uint64_t base,
             uint64_t bound,
             uint64_t offs,
             uint64_t asid,
             uint64_t csid,
             uint64_t attr,
             uint64_t lru) {
      theLru   = lru;
      theAsid  = asid;
      theCsid  = csid;
      theAttr  = attr;
      theBase  = base;
      theBound = bound;
      theOffs  = offs;
    }

    bool hit(uint64_t base,
             uint64_t asid,
             uint64_t csid) {
      return ((theBase == base) &&
              (theAsid == asid) &&
              (theCsid == csid));
    }

    PhysicalMemoryAddress translate(const VirtualMemoryAddress &anAddress) const {
      return PhysicalMemoryAddress(anAddress + (theOffs << 12));
    }
  };

  struct LookupResult {
    bool     hit;
    uint64_t addr;
    uint64_t attr;

    LookupResult():
      hit (false),
      addr(0),
      attr(0) {}

    LookupResult(uint64_t a, uint64_t b):
      hit (true),
      addr(a),
      attr(b) {}
  };

  struct TLB {
    friend class boost::serialization::access;

    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      auto tlb_free = theTlbFreelist.size();
      auto vlb_free = theVlbFreelist.size();

      ar &theTlbSets;
      ar &theTlbWays;
      ar &tlb_free;

      for (size_t i = 0; i < theTlbSets; i++)
        for (const auto &tlb: theTlb[i])
          ar &*tlb;

      ar &theVlbSets;
      ar &theVlbWays;
      ar &vlb_free;

      for (size_t i = 0; i < theVlbSets; i++)
        for (const auto &vlb: theVlb[i])
          ar &*vlb;
    }

    LookupResult lookup(TranslationPtr &t) {
      LookupResult res;

      uint64_t asid = t->theAsid;
      uint64_t csid = t->theCsid;

      if (t->isUat()) {
        uint64_t base = t->theBase;
        uint64_t idx  = t->theSet % theVlbSets;

        for (const auto& vlb: theVlb[idx])
          if (vlb->hit(base, asid, csid))
            return LookupResult(vlb->translate(t->theVaddr), vlb->theAttr);

      } else {
        uint64_t base = t->theBase;
        uint64_t idx  = t->theSet % theTlbSets;

        for (const auto& tlb: theTlb[idx])
          if (tlb->hit(base, asid))
            return LookupResult(tlb->translate(t->theVaddr), tlb->theAttr);
      }

      return LookupResult();
    }

    void insert(TranslationPtr &t) {
      if (t->isUat()) {
        VlbEntry *vlb;
        auto      idx = t->theSet % theVlbSets;

        if (theVlbFreelist[idx].empty()) {
          vlb = theVlb[idx].front();

          for (const auto &e: theVlb[idx])
            if (e->theLru < vlb->theLru)
              vlb = e;

        } else {
          vlb = theVlbFreelist[idx].back();

          theVlbFreelist[idx].pop_back();
          theVlb[idx].push_back(vlb);
        }

        vlb->set(t->theBase,
                 t->theBound,
                 t->theOffs,
                 t->theAsid,
                 t->theCsid,
                 t->theAttr,
                 theLru++);

      } else {
        TlbEntry *tlb;
        auto      idx = t->theSet % theTlbSets;

        if (theTlbFreelist[idx].empty()) {
          tlb = theTlb[idx].front();

          for (const auto &e: theTlb[idx])
            if (e->theLru < tlb->theLru)
              tlb = e;

        } else {
          tlb = theTlbFreelist[idx].back();

          theTlbFreelist[idx].pop_back();
          theTlb[idx].push_back(tlb);
        }

        tlb->set(t->theVaddr,
                 t->thePaddr,
                 t->theAsid,
                 t->theAttr,
                 theLru);
      }
    }

    void initTlb(size_t sets, size_t ways) {
      theTlbSets = sets;
      theTlbWays = ways;

      for (size_t i = 0; i < sets; i++) {
        theTlb.emplace_back();
        theTlbFreelist.emplace_back();

        auto &fl = theTlbFreelist.back();

        for (size_t i = 0; i < ways; i++)
          fl.push_back(new TlbEntry());
      }
    }

    void initVlb(size_t sets, size_t ways) {
      theVlbSets = sets;
      theVlbWays = ways;

      for (size_t i = 0; i < sets; i++) {
        theVlb.emplace_back();
        theVlbFreelist.emplace_back();

        auto &fl = theVlbFreelist[i];

        for (size_t i = 0; i < ways; i++)
          fl.push_back(new VlbEntry());
      }
    }

  private:
    uint64_t theLru;

    size_t theTlbSets;
    size_t theTlbWays;
    size_t theVlbSets;
    size_t theVlbWays;

    std::vector<std::vector<TlbEntry *>> theTlb;
    std::vector<std::vector<VlbEntry *>> theVlb;
    
    std::vector<std::vector<TlbEntry *>> theTlbFreelist;
    std::vector<std::vector<VlbEntry *>> theVlbFreelist;
  };

  TLB theITlb;
  TLB theDTlb;

  std::unique_ptr<PageWalk> thePageWalker;

  std::queue<boost::intrusive_ptr<Translation>> theLookUpEntries;
  std::queue<boost::intrusive_ptr<Translation>> thePageWalkEntries;
  std::vector<boost::intrusive_ptr<Translation>> standingEntries;
  std::set<VirtualMemoryAddress> alreadyPW;

  std::shared_ptr<Flexus::Qemu::Processor> theCPU;

  // states
  uint64_t asid;
  uint64_t csid;

  uint64_t ppn_mode;
  uint64_t ppn_root;

  uint64_t uat_en;
  uint64_t uat_root;
  uint64_t uat_idx;
  uint64_t uat_vsc;
  uint64_t uat_top;
  uint64_t uat_siz;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MMU) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  uint64_t uatBase(uint64_t vaddr) {
    uint64_t idx = (vaddr & mask(uat_vsc) >> uat_idx);
    uint64_t vsc = (vaddr & mask(uat_top) >> uat_vsc);

    return (vaddr & ~mask(idx + vsc)) >> 12;
  }

  uint64_t uatOffs(uint64_t vaddr) {
    uint64_t idx = (vaddr & mask(uat_vsc)) >> uat_idx;
    uint64_t vsc = (vaddr & mask(uat_top)) >> uat_vsc;
    uint64_t top =  vaddr                  >> uat_top;
    uint64_t tsl =  uat_vsc - uat_idx;

    uint64_t vsc_mask =  mask(vsc);
    uint64_t vsc_offs = (top << tsl) + (idx & ~vsc_mask) + (vsc_mask >> 1);

    vsc_offs = (vsc_offs << 1) + (vsc ? 1 : 0);

    if (vsc_offs >= (1lu << uat_siz))
      return vsc_offs | (1lu << 63);

    return vsc_offs;
  }

  uint64_t uatSet(uint64_t vaddr) {
    return 0;
  }

  void sync() {
    Processor cpu = Processor::getProcessor(flexusIndex());

    uint64_t satp  = API::qemu_api.get_csr(cpu, CSR_SATP);

    uint64_t ucid  = API::qemu_api.get_csr(cpu, CSR_UCID);
    uint64_t suatp = API::qemu_api.get_csr(cpu, CSR_SUATP);
    uint64_t suatc = API::qemu_api.get_csr(cpu, CSR_SUATC);

    // sv39
    asid     = extractBitsWithBounds(satp,  59, 44);
    csid     = extractBitsWithBounds(ucid,  11,  0);

    ppn_mode = extractBitsWithBounds(satp,  63, 60);
    ppn_root = extractBitsWithBounds(satp,  53, 10) << 12;

    uat_en   = extractBitsWithBounds(suatp, 63, 63);
    uat_root = extractBitsWithBounds(suatp, 62,  0) << 12;
    uat_idx  = extractBitsWithBounds(suatc,  5,  0);
    uat_vsc  = extractBitsWithBounds(suatc, 13,  8);
    uat_top  = extractBitsWithBounds(suatc, 21, 16);
    uat_siz  = extractBitsWithBounds(suatc, 29, 24);
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

    ioarch << theITlb;
    doarch << theDTlb;

    iFile.close();
    dFile.close();
  }

  void loadState(std::string const &aDirName) {
    std::ifstream iFile, dFile;
    iFile.open(aDirName + "/" + statName() + "-itlb", std::ifstream::in);
    dFile.open(aDirName + "/" + statName() + "-dtlb", std::ifstream::in);

    boost::archive::text_iarchive iiarch(iFile);
    boost::archive::text_iarchive diarch(dFile);

    iiarch >> theITlb;
    diarch >> theDTlb;

    iFile.close();
    dFile.close();
  }

  // Initialization
  void initialize() {
    theCPU = std::make_shared<Flexus::Qemu::Processor>(
        Flexus::Qemu::Processor::getProcessor(flexusIndex()));
    thePageWalker.reset(new PageWalk(flexusIndex()));

    theITlb.initTlb(cfg.iTlbSets, cfg.iTlbWays);
    theITlb.initVlb(cfg.iVlbSets, cfg.iVlbWays);
    theDTlb.initTlb(cfg.dTlbSets, cfg.dTlbWays);
    theDTlb.initVlb(cfg.dVlbSets, cfg.dVlbWays);
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

      LookupResult entry;

      // update the item
      auto pl = API::qemu_api.get_pl(Processor::getProcessor(flexusIndex()));

      // sv39
      if ((pl > 1) || (ppn_mode != 8)) {
        // m-mode
        entry.hit  = true;
        entry.addr = item->theVaddr;

      } else if (cfg.Perfect) {
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_api.get_pa(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));

        entry.hit  = true;
        entry.addr = perfectPaddr;

        if (perfectPaddr == 0xFFFFFFFFFFFFFFFF)
          item->setPagefault();

      } else {
        if (uat_en && (pl == 0))
          item->setUat();

        item->theAsid = asid;
        item->theCsid = csid;
        item->theBase = item->isUat() ? uatBase(item->theVaddr) : (item->theVaddr & ~((1lu << 12) - 1lu));
        item->theSet  = item->isUat() ? uatSet (item->theVaddr) : (item->theVaddr >> 12);

        entry = (item->isInstr() ? theITlb : theDTlb).lookup(item);
      }

      if (entry.hit) {
        DBG_(VVerb, (<< "Item is a Hit " << item->theVaddr));

        // item exists so mark hit
        item->setHit();
        item->thePaddr = entry.addr;

        if (item->isInstr())
          FLEXUS_CHANNEL(iTranslationReply) << item;
        else
          FLEXUS_CHANNEL(dTranslationReply) << item;

      } else {
        DBG_(VVerb, (<< "Item is a miss " << item->theVaddr));

        VirtualMemoryAddress base(item->theBase);

        if (alreadyPW.find(base) == alreadyPW.end()) {
          // mark miss
          item->setMiss();
          if (thePageWalker->push_back(item)) {
            alreadyPW.insert(base);
            thePageWalkEntries.push(item);
          } else {
            PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_api.get_pa(
                *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
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

        VirtualMemoryAddress base(item->theBase);

        if (alreadyPW.find(base) != alreadyPW.end()) {
          alreadyPW.erase(base);

          for (auto it = standingEntries.begin(); it != standingEntries.end();) {
            if (((*it)->isInstr() == item->isInstr()) &&
                ((*it)->theBase == item->theBase)) {
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

        if (item->isInstr()) {
          theITlb.insert(item);
          FLEXUS_CHANNEL(iTranslationReply) << item;
        } else {
          theDTlb.insert(item);
          FLEXUS_CHANNEL(dTranslationReply) << item;
        }
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

  void resyncMMU(int anIndex) {
    CORE_TRACE;
    DBG_(VVerb, (<< "Resynchronizing MMU"));

    static bool optimize = false;

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
    FLEXUS_CHANNEL(ResyncOut) << anIndex;
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
    if (cfg.Perfect)
      return;

    if (!((aTranslate->isInstr() ? theITlb : theDTlb).lookup(aTranslate).hit)) {
      thePageWalker->push_back_trace(aTranslate,
                                     Flexus::Qemu::Processor::getProcessor((int)flexusIndex()));

      if (aTranslate->isInstr())
        theITlb.insert(aTranslate);
      else
        theDTlb.insert(aTranslate);
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
