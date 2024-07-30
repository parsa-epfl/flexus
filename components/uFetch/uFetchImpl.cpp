
#include "core/stats.hpp"
#include "components/uArch/uArchInterfaces.hpp"

#include "uFetch.hpp"
#include "uFetchTypes.hpp"

#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories uFetch
#define DBG_SetDefaultOps AddCat(uFetch)
#include DBG_Control()

namespace nuFetch {


class FLEXUS_COMPONENT(uFetch) {
  FLEXUS_COMPONENT_IMPL(uFetch);

private:
// ================== STATS ==================
    Flexus::Stat::StatCounter theFetchAccesses;
    Flexus::Stat::StatCounter theFetches;
    Flexus::Stat::StatCounter thePrefetches;
    Flexus::Stat::StatCounter theFailedTranslations;
    Flexus::Stat::StatCounter theMisses;
    Flexus::Stat::StatCounter theHits;
    Flexus::Stat::StatCounter theMissCycles;
    Flexus::Stat::StatCounter theAllocations;
    Flexus::Stat::StatMax theMaxOutstandingEvicts;
    Flexus::Stat::StatCounter theAvailableFetchSlots;
    Flexus::Stat::StatCounter theUsedFetchSlots;

    uint64_t theLastVTagSet;
    PhysicalMemoryAddress theLastPhysical;


public:
  FLEXUS_COMPONENT_CONSTRUCTOR(uFetch)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS), theFetchAccesses(statName() + "-FetchAccess"),
        theFetches(statName() + "-Fetches"), thePrefetches(statName() + "-Prefetches"),
        theFailedTranslations(statName() + "-FailedTranslations"),
        theMisses(statName() + "-Misses"), theHits(statName() + "-Hits"),
        theMissCycles(statName() + "-MissCycles"), theAllocations(statName() + "-Allocations"),
        theMaxOutstandingEvicts(statName() + "-MaxEvicts"),
        theAvailableFetchSlots(statName() + "-FetchSlotsPossible"),
        theUsedFetchSlots(statName() + "-FetchSlotsUsed"), theLastVTagSet(0), theLastPhysical(0) {}

  //   Msutherl: TLB in-out functions
  FLEXUS_PORT_ALWAYS_AVAILABLE(iTranslationIn);
  void push(interface::iTranslationIn const &, TranslationPtr &retdTranslations) { }

  // FetchAddressIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchAddressIn);
  void push(interface::FetchAddressIn const &, index_t anIndex, boost::intrusive_ptr<FetchCommand> &aCommand) { }

  // AvailableFAQOut
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(AvailableFAQOut);
  int32_t pull(interface::AvailableFAQOut const &, index_t anIndex) { return 0; }

  // SquashIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashIn);
  void push(interface::SquashIn const &, index_t anIndex, eSquashCause &aReason) { }

  // FetchMissIn
  FLEXUS_PORT_ALWAYS_AVAILABLE(FetchMissIn);
  void push(interface::FetchMissIn const &, MemoryTransport &aTransport) { }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &, index_t anIndex) { return true;}

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &, index_t anIndex) { return true; }

  void push(interface::ResyncIn const&, index_t anIndex, int& aResync){}
  bool available(interface::ResyncIn const &, index_t anIndex) { return true; }

  void initialize() {}
  void finalize() { }
  void drive(interface::uFetchDrive const &) {}
};


} // END OF NAMESPACE nuFETCH

FLEXUS_COMPONENT_INSTANTIATOR(uFetch, nuFetch);

FLEXUS_PORT_ARRAY_WIDTH(uFetch, FetchAddressIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, SquashIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, AvailableFAQOut) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, AvailableFIQ) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, FetchBundleOut) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, ICount) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, Stalled) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, ResyncIn) {
  return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uFetch

#define DBG_Reset
#include DBG_Control()
