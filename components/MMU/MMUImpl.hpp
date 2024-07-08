#ifndef FLEXUS_MMUIMPL_HPP_INCLUDED
#define FLEXUS_MMUIMPL_HPP_INCLUDED

#include <core/qemu/bitUtilities.hpp>
#include <core/stats.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <components/uArch/CoreImpl.hpp>

#include "MMU.hpp"


namespace nMMU {

using namespace Flexus;
using namespace Core;
using namespace Qemu;
using namespace SharedTypes;

class MMUComponent;
class PageWalk;

uint64_t mask(uint64_t i);

struct TlbEntry {
  TlbEntry(MMUComponent *_mmu): mmu(_mmu) {
  }

  void set(uint64_t _vaddr,
           uint64_t _paddr,
           uint64_t _attr,
           uint64_t _asid);

  bool hit(uint64_t _base,
           uint64_t _asid);

  bool inv(uint64_t _addr,
           uint64_t _asid);

  void load(boost::archive::text_iarchive &m);
  void save(boost::archive::text_oarchive &m);

  PhysicalMemoryAddress translate(const VirtualMemoryAddress &addr, API::trans_type_t trans) const;

  MMUComponent *mmu;

  uint64_t vaddr;
  uint64_t paddr;
  uint64_t attr;
  uint64_t asid;
};

struct LookupResult {
  bool     hit;
  uint64_t addr;
  uint64_t attr;

  LookupResult():
    hit (false),
    addr(0),
    attr(0) {}

  LookupResult(uint64_t _addr, uint64_t _attr):
    hit (true),
    addr(_addr),
    attr(_attr) {}
};

struct Tlb {
  void initialize(const std::string &name,
                  MMUComponent *_mmu,
                  size_t _tlbSets,
                  size_t _tlbWays);

  void load(boost::archive::text_iarchive &m, bool data);
  void save(boost::archive::text_oarchive &m);

  void resync();

  LookupResult lookup(TranslationPtr &t);
  void         insert(TranslationPtr &t);

  void clear(uint64_t addr, uint64_t asid);

private:
  MMUComponent *mmu;

  size_t tlbSets;
  size_t tlbWays;

  std::vector<std::list<TlbEntry *>> tlb;
  std::vector<std::list<TlbEntry *>> tlbFreelist;

  boost::optional<TlbEntry> faulty;
};

class FLEXUS_COMPONENT(MMU) {
  FLEXUS_COMPONENT_IMPL(MMU);

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MMU);

  void initialize();
  void finalize();

  bool isQuiesced() const;

  void loadState(std::string const &aDirName);
  void saveState(std::string const &aDirName);

  void drive(interface::MMUDrive const &);

  LookupResult lookup(TranslationPtr &tr);

  bool check(TranslationPtr &tr);
  bool fault(TranslationPtr &tr, bool inv = true);

  void clear(uint64_t addr, uint64_t asid);
  void invalidate(uint64_t addr);

  void parseSatp(uint64_t val);
  void parse(uint64_t mode);

  void resync(int anIndex);

  bool available(interface::ResyncIn   const &, index_t anIndex);
  bool available(interface::iRequestIn const &, index_t anIndex);
  bool available(interface::dRequestIn const &, index_t anIndex);
  bool available(interface::TLBReqIn   const &, index_t anIndex);

  void push(interface::ResyncIn   const &, index_t anIndex, int &aResync);
  void push(interface::iRequestIn const &, index_t anIndex, TranslationPtr &aTranslate);
  void push(interface::dRequestIn const &, index_t anIndex, TranslationPtr &aTranslate);
  void push(interface::TLBReqIn   const &, index_t anIndex, TranslationPtr &aTranslate);

  API::conf_object_t *cpu;

  // states
  uint64_t asid;

  uint64_t ppnMode;
  uint64_t ppnRoot;

  std::unique_ptr<PageWalk> walker;

private:
  void cycle();

  void done(TranslationPtr &tr);

  Tlb itlb;
  Tlb dtlb;

  std::queue<TranslationPtr> lookupEntries;
  std::map<uint64_t, std::deque<TranslationPtr>> replayEntries;
};

extern std::vector<MMUComponent *> theMmu;

}

#endif