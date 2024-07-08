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

#include <core/component.hpp>

#include "MMUImpl.hpp"
#include "PageWalk.hpp"

#define CSR_SATP 0x180

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

uint64_t mask(uint64_t i) {
  return (1lu << i) - 1lu;
}

void TlbEntry::set(uint64_t _vaddr,
                   uint64_t _paddr,
                   uint64_t _attr,
                   uint64_t _asid) {
  asid  = _asid;
  attr  = _attr;
  vaddr = _vaddr;
  paddr = _paddr;
}

bool TlbEntry::hit(uint64_t _base,
                   uint64_t _asid) {
  return ((vaddr == _base) &&
         ((asid  == _asid) || (attr & 0x20)));
}

bool TlbEntry::inv(uint64_t _addr,
                   uint64_t _asid) {
  return (!_addr || (vaddr == _addr)) &&
         (!_asid || (asid  == _asid)  || (attr & 0x20));
}

void TlbEntry::load(boost::archive::text_iarchive &m) {
  m >> asid;
  m >> attr;
  m >> vaddr;
  m >> paddr;
}

void TlbEntry::save(boost::archive::text_oarchive &m) {
  m << asid;
  m << attr;
  m << vaddr;
  m << paddr;
}

PhysicalMemoryAddress TlbEntry::translate(const VirtualMemoryAddress &vaddr, API::trans_type_t trans) const {
  if ((int64_t)(paddr) < 0)
    return PhysicalMemoryAddress(API::qemu_api.get_pa(mmu->cpu, trans, vaddr));

  return PhysicalMemoryAddress(paddr + (vaddr & mask(12)));
}

void Tlb::initialize(const std::string &name,
                     MMUComponent *_mmu,
                     size_t _tlbSets,
                     size_t _tlbWays) {
  mmu = _mmu;

  tlbSets = _tlbSets;
  tlbWays = _tlbWays;

  for (size_t i = 0; i < tlbSets; i++) {
    tlb.emplace_back();
    tlbFreelist.emplace_back();

    auto &fl = tlbFreelist.back();

    for (size_t i = 0; i < tlbWays; i++)
      fl.push_back(new TlbEntry(mmu));
  }
}

void Tlb::load(boost::archive::text_iarchive &m, bool data) {
  m >> tlbSets;
  m >> tlbWays;

  for (size_t i = 0; i < tlbSets; i++) {
    size_t len;

    m >> len;

    for (size_t j = 0; j < len; j++) {
      auto e = tlbFreelist[i].back();

      e->load(m);

      tlbFreelist[i].pop_back();
      tlb[i].push_back(e);
    }
  }
}

void Tlb::save(boost::archive::text_oarchive &m) {
  m << tlbSets;
  m << tlbWays;

  for (size_t i = 0; i < tlbSets; i++) {
    size_t len = tlb[i].size();

    m << len;

    for (const auto &e: tlb[i])
      e->save(m);
  }
}

void Tlb::resync() {
  faulty = boost::none;
}

LookupResult Tlb::lookup(TranslationPtr &tr) {
  auto trans = (tr->theType == Translation::eLoad ) ? API::QEMU_Curr_Load  :
               (tr->theType == Translation::eStore) ? API::QEMU_Curr_Store :
                                                      API::QEMU_Curr_Fetch;

  auto base = tr->theVaddr & ~mask(12);
  auto idx  = tr->theSet % tlbSets;

  for (auto i = tlb[idx].begin(); i != tlb[idx].end(); i++) {
    auto e = *i;

    if (e->hit(base, tr->theAsid)) {
      LookupResult res(e->translate(tr->theVaddr, trans), e->attr);

      if (i != tlb[idx].begin()) {
        tlb[idx].erase(i);
        tlb[idx].push_front(e);
      }

      return res;
    }
  }

  if (faulty && faulty->hit(tr->theVaddr & ~mask(12), tr->theAsid))
    return LookupResult(faulty->translate(tr->theVaddr, trans), faulty->attr);

  return LookupResult();
}

void Tlb::insert(TranslationPtr &tr) {
  if (tr->isPagefault()) {
    if (tr->inTraceMode)
      return;

    if (!faulty)
      faulty = TlbEntry(mmu);

    faulty->set(tr->theVaddr & ~mask(12),
                tr->thePaddr & ~mask(12),
                tr->theAttr,
                tr->theAsid);

    return;
  }

  TlbEntry *f;

  auto idx = tr->theSet % tlbSets;

  if (tlbFreelist[idx].empty()) {
    auto &sub = tlb[idx];

    f = sub.back();

    sub.erase(std::prev(sub.end()));
    sub.push_front(f);

  } else {
    f = tlbFreelist[idx].back();

    tlbFreelist[idx].pop_back();
    tlb[idx].push_front(f);
  }

  f->set(tr->theVaddr & ~mask(12),
         tr->thePaddr & ~mask(12),
         tr->theAttr,
         tr->theAsid);
}

void Tlb::clear(uint64_t addr, uint64_t asid) {
  for (size_t i = 0; i < tlbSets; i++) {
    for (auto j = tlb[i].begin(); j != tlb[i].end(); ) {
      auto e = *j;

      if (e->inv(addr, asid)) {
        j = tlb[i].erase(j);
        tlbFreelist[i].push_back(e);

      } else
        j++;
    }
  }

  resync();
}

MMUComponent::FLEXUS_COMPONENT_CONSTRUCTOR(MMU) :
    base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
}

std::vector<MMUComponent *> theMmu;

void MMUComponent::initialize() {
  cpu = (API::conf_object_t *)(Flexus::Qemu::Processor::getProcessor(flexusIndex()));

  walker.reset(new PageWalk(this));

  itlb.initialize("itlb", this, cfg.itlbsets, cfg.itlbways);
  dtlb.initialize("dtlb", this, cfg.dtlbsets, cfg.dtlbways);

  // can be out-of-order
  while (theMmu.size() <= flexusIndex())
    theMmu.push_back(NULL);

  theMmu[flexusIndex()] = this;
}

bool MMUComponent::isQuiesced() const {
  return false;
}

void MMUComponent::saveState(std::string const &aDirName) {
  std::string itlb_fn(aDirName);
  std::string dtlb_fn(aDirName);

  itlb_fn.append("/").append(statName()).append("-itlb");
  dtlb_fn.append("/").append(statName()).append("-dtlb");

  std::ofstream itlb_fs(itlb_fn);
  std::ofstream dtlb_fs(dtlb_fn);

  if (!itlb_fs.good() || !dtlb_fs.good()) {
    DBG_(Dev, (<< "Could not load MMU state from " << aDirName));
    return;
  }

  boost::archive::text_oarchive itlb_os(itlb_fs);
  boost::archive::text_oarchive dtlb_os(dtlb_fs);

  itlb.save(itlb_os);
  dtlb.save(dtlb_os);
}

void MMUComponent::loadState(std::string const &aDirName) {
  std::string itlb_fn(aDirName);
  std::string dtlb_fn(aDirName);

  itlb_fn.append("/").append(statName()).append("-itlb");
  dtlb_fn.append("/").append(statName()).append("-dtlb");

  std::ifstream itlb_fs(itlb_fn);
  std::ifstream dtlb_fs(dtlb_fn);

  if (!itlb_fs.good() || !dtlb_fs.good()) {
    DBG_(Dev, (<< "Could not load MMU state from " << aDirName));
    return;
  }

  boost::archive::text_iarchive itlb_is(itlb_fs);
  boost::archive::text_iarchive dtlb_is(dtlb_fs);

  itlb.load(itlb_is, false);
  dtlb.load(dtlb_is, true);
}

void MMUComponent::finalize() {
}

void MMUComponent::drive(interface::MMUDrive const &) {
  cycle();
  walker->cycle();

  while (walker->walkSize()) {
    auto &tr = walker->walkFront();

    DBG_(VVerb, (<< "walk: " << tr->theVaddr << std::hex
                 << ":"      << tr->theAddr
                 << ":"      << tr->theID));

    FLEXUS_CHANNEL(MemoryRequestOut) << tr;

    walker->popWalk();
  }
}

void MMUComponent::cycle() {
  // requests coming at this cycle can see walks just done
  while (walker->size()) {
    auto &tr = walker->front();

    if (tr->isAnnul()) {
      DBG_(VVerb, (<< "walk "      << tr->theVaddr
                   << ":"          << tr->theID
                   << ": annulled"));

      done(tr);

    } else if (tr->isDone()) {
      DBG_(VVerb, (<< "walk "  << tr->theVaddr
                   << ":"      << tr->theID
                   << ": done"));

      done(tr);

      if (tr->isInstr()) {
        itlb.insert(tr);

        FLEXUS_CHANNEL(iTranslationReply) << tr;

      } else {
        dtlb.insert(tr);

        FLEXUS_CHANNEL(dTranslationReply) << tr;
      }

    } else
      break;

    walker->pop();
  }

  while (lookupEntries.size()) {
    auto tr  = lookupEntries.front();
    auto res = lookup(tr);

    if (res.hit) {
      DBG_(VVerb, (<< "lookup " << tr->theVaddr
                   << ":"       << tr->theID
                   << std::hex
                   << ": hit "  << res.addr));

      tr->setHit();

      tr->thePaddr = res.addr;
      tr->theAttr  = res.attr;

      if (check(tr))
        fault(tr, false);

      if (tr->isInstr())
        FLEXUS_CHANNEL(iTranslationReply) << tr;
      else
        FLEXUS_CHANNEL(dTranslationReply) << tr;

    } else {
      DBG_(VVerb, (<< "lookup " << tr->theVaddr
                   << ":"       << tr->theID
                   << ": miss"));

      tr->theTime = Flexus::Core::theFlexus->cycleCount();

      if (replayEntries.find(tr->theBase) == replayEntries.end())
        walker->push(tr);

      replayEntries[tr->theBase].emplace_back(tr);
    }

    lookupEntries.pop();
  }
}

LookupResult MMUComponent::lookup(TranslationPtr &tr) {
  auto pl = API::qemu_api.get_pl(cpu);

  if (cfg.perfect) {
    if (!tr->inTraceMode) {
      auto trans = (tr->theType == Translation::eLoad ) ? API::QEMU_Curr_Load  :
                   (tr->theType == Translation::eStore) ? API::QEMU_Curr_Store :
                                                          API::QEMU_Curr_Fetch;

      PhysicalMemoryAddress addr(API::qemu_api.get_pa(cpu, trans, tr->theVaddr));

      if ((int64_t)(addr) < 0)
        fault(tr);

      // TODO
      return LookupResult(addr, 0xf);
    }

    return LookupResult(0, 0);
  }

  if ((pl > 1) || (ppnMode == 0)) {
    if (tr->theVaddr >= 0x40000000000lu)
      fault(tr);

    return LookupResult(tr->theVaddr, 0xf);
  }

  if (ppnMode != 8) {
    fault(tr);
    return LookupResult(0, 0);
  }

  walker->base(tr);

  return (tr->isInstr() ? itlb : dtlb).lookup(tr);
}

bool MMUComponent::check(TranslationPtr &tr) {
  if (tr->isDone())
    return false;

  switch (tr->theType) {
  case Translation::eLoad:
    if (!(tr->theAttr & 0x2))
      return true;
    break;
  case Translation::eStore:
    if (!(tr->theAttr & 0x4))
      return true;
    break;
  case Translation::eFetch:
    if (!(tr->theAttr & 0x8))
      return true;
    break;
  }

  return false;
}

bool MMUComponent::fault(TranslationPtr &tr, bool inv) {
  DBG_(VVerb, (<< "fault " << tr->theVaddr
               << ":"      << tr->theID));

  if (inv) {
    tr->thePaddr = ~0lu;
    tr->theAttr  =  0;
  }

  if (tr->isDone())
    return true;

  tr->setPagefault();
  tr->setDone();
  return true;
}

void MMUComponent::clear(uint64_t addr, uint64_t asid) {
  addr &= ~mask(12);

  itlb.clear(addr, asid);
  dtlb.clear(addr, asid);

  if (walker)
    walker->annul();

  replayEntries.clear();

  while (!lookupEntries.empty())
    lookupEntries.pop();
}

void MMUComponent::parseSatp(uint64_t val) {
  asid    = extractBitsWithBounds(val, 59, 44);
  ppnMode = extractBitsWithBounds(val, 63, 60);
  ppnRoot = extractBitsWithBounds(val, 43,  0) << 12;

  DBG_(Iface, (<< "c" << flexusIndex() << " satp: " << std::hex << val));
}

void MMUComponent::parse(uint64_t mode) {
  if (mode & 0x1)
    parseSatp(API::qemu_api.get_csr(cpu, CSR_SATP));
}

void MMUComponent::done(TranslationPtr &tr) {
  auto it = replayEntries.find(tr->theBase);

  if (it->second.size() > 1) {
    it->second.pop_front();

    for (const auto &e: it->second) {
      DBG_(VVerb, (<< "retry " << e->theVaddr
                   << ":"      << e->theID));

      lookupEntries.push(e);
    }
  }

  replayEntries.erase(it);
}

void MMUComponent::resync(int anIndex) {
  DBG_(VVerb, (<< "resync"));

  itlb.resync();
  dtlb.resync();

  if (walker)
    walker->annul();

  replayEntries.clear();

  if (lookupEntries.size() > 0) {
    while (!lookupEntries.empty()) {
      auto tr = lookupEntries.front();

      DBG_(VVerb, (<< "annul " << tr->theVaddr
                   << ":"      << tr->theID));

      lookupEntries.pop();
    }
  }

  FLEXUS_CHANNEL(ResyncOut) << anIndex;
}

bool MMUComponent::available(interface::ResyncIn const &, index_t anIndex) {
  return true;
}
void MMUComponent::push(interface::ResyncIn const &, index_t anIndex, int &aResync) {
  resync(aResync);
}

bool MMUComponent::available(interface::iRequestIn const &, index_t anIndex) {
  return true;
}
void MMUComponent::push(interface::iRequestIn const &, index_t anIndex, TranslationPtr &tr) {
  tr->theIndex = anIndex;
  tr->toggleReady();

  tr->theAsid = asid;

  lookupEntries.push(tr);
}

bool MMUComponent::available(interface::dRequestIn const &, index_t anIndex) {
  return true;
}
void MMUComponent::push(interface::dRequestIn const &, index_t anIndex, TranslationPtr &tr) {
  tr->theIndex = anIndex;
  tr->toggleReady();

  tr->theAsid = asid;

  lookupEntries.push(tr);
}

bool MMUComponent::available(interface::TLBReqIn const &, index_t anIndex) {
  return true;
}
void MMUComponent::push(interface::TLBReqIn const &, index_t anIndex, TranslationPtr &tr) {
  tr->theAsid = asid;

  auto res = lookup(tr);

  if (res.hit) {
    DBG_(VVerb, (<< "lookup " << tr->theVaddr
                 << ":"       << tr->theID
                 << ": hit "  << tr->thePaddr));

    return;
  }

  DBG_(VVerb, (<< "lookup " << tr->theVaddr
               << ":"       << tr->theID
               << ": miss"));

  walker->pushTrace(tr);

  if (tr->isInstr())
    itlb.insert(tr);
  else
    dtlb.insert(tr);
}

} // End Namespace nMMU

FLEXUS_COMPONENT_INSTANTIATOR(MMU, nMMU);
FLEXUS_PORT_ARRAY_WIDTH(MMU, dRequestIn) {
  return (cfg.cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, iRequestIn) {
  return (cfg.cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, ResyncIn) {
  return (cfg.cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, iTranslationReply) {
  return (cfg.cores);
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, dTranslationReply) {
  return (cfg.cores);
}

FLEXUS_PORT_ARRAY_WIDTH(MMU, MemoryRequestOut) {
  return (cfg.cores);
}

FLEXUS_PORT_ARRAY_WIDTH(MMU, TLBReqIn) {
  return (cfg.cores);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MMU

#define DBG_Reset
#include DBG_Control()


namespace Flexus {
namespace Qemu {
namespace API {

void FLEXUS_parse_mmu(int idx, uint64_t mode) {
  nMMU::theMmu[idx]->parse(mode);
}

}
}
}