

//#include <boost/none.hpp>

#include "core/stats.hpp"
#include "components/uArch/uArchInterfaces.hpp"
#include "uFetch.hpp"
#include "uFetchTypes.hpp"

#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories uFetch
#define DBG_SetDefaultOps AddCat(uFetch)
#include DBG_Control()

using namespace Flexus::Qemu;

namespace nuFetch {


class FLEXUS_COMPONENT(uFetch) {
  FLEXUS_COMPONENT_IMPL(uFetch);

private:
// ==================== FetchAddressGenerate list =================
  std::vector<std::list<FetchAddr>> theFAQ;
  pFetchBundle waitingForOpcodeQueue;
  BijectionMapType_t tr_op_bijection;
  ExpectedTranslation_t translationsExpected;

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

    std::vector<boost::optional<PhysicalMemoryAddress>> theIcacheMiss;
    std::vector<boost::optional<VirtualMemoryAddress>> theIcacheVMiss;
    std::vector<boost::intrusive_ptr<TransactionTracker>> theFetchReplyTransactionTracker;
    // Indicates whether a prefetch is outstanding, and what paddr was prefetched
    std::vector<boost::optional<PhysicalMemoryAddress>> theIcachePrefetch;


  // ================== CACHE ========================
  int32_t theBundleCoreID;
  uint64_t theBlockMask;

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

  // =============================== LOGIC =========================

  Processor cpu(index_t anIndex) {
    return Processor::getProcessor(flexusIndex() * cfg.Threads + anIndex);
  }
  void send_translation_request(index_t anIndex, VirtualMemoryAddress const &anAddress, opcodeQIterator newOpcIterator) {

    TranslationPtr xlat{new Translation()};
    xlat->theVaddr = anAddress;
    xlat->theType = Translation::eFetch;
    xlat->theException = 0; // just for now
    xlat->theIndex = anIndex;
    xlat->setInstr();

    // Insert an entry into the translation<->opcode map and outstanding translations expected
    std::pair<BijectionMapType_t::iterator, bool> bijection_insertion = tr_op_bijection.insert(std::make_pair(xlat, newOpcIterator));

    DBG_(VVerb, Comp(*this)(<< "Inserting xlat objection with vaddr " << xlat->theVaddr
                                 << " pointing to PC " << newOpcIterator->thePC
                                 << " into waitingForOpcodesQueue bijection "));
    DBG_AssertSev(
        Iface, bijection_insertion.second == true,
        (<< "Inserting xlat object with vaddr " << xlat->theVaddr
         << " failed!! Clashing object has vaddr: " << bijection_insertion.first->first->theVaddr));

    translationsExpected.emplace(xlat);

    DBG_(VVerb, Comp(*this)(<< "Adding translation request entry for " << xlat->theVaddr));

    DBG_Assert(FLEXUS_CHANNEL(iTranslationOut).available());
    FLEXUS_CHANNEL(iTranslationOut) << xlat;
  }



  void issueFetch(PhysicalMemoryAddress anAddress, VirtualMemoryAddress vPC) {
    DBG_Assert(anAddress != 0);
    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation(MemoryMessage::newFetch(anAddress, vPC));
    operation->reqSize() = 64;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anAddress);
    tracker->setInitiator(flexusIndex());
    tracker->setFetch(true);
    tracker->setSource("uFetch");
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);

    theMissQueue.push_back(transport);
  }



    // Helper functions
    PhysicalMemoryAddress translateAddress(uint64_t vaddr, int anIndex) {
        Flexus::SharedTypes::Translation xlat;
        xlat.theVaddr = vaddr;
        xlat.theType = Translation::eFetch;
        xlat.thePaddr = cpu(anIndex).translate_va2pa(xlat.theVaddr);
        return xlat.thePaddr;
    }

    void handleCacheMiss(PhysicalMemoryAddress paddr, uint64_t vaddr, int anIndex, uint64_t tagset) {
        PhysicalMemoryAddress temp(paddr & theBlockMask);
        theIcacheMiss[anIndex] = temp;
        theIcacheVMiss[anIndex] = VirtualMemoryAddress(vaddr);
        theFetchReplyTransactionTracker[anIndex] = nullptr;

        DBG_(VVerb, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "."
                                << anIndex << "] L1I MISS " << vaddr << " " << *theIcacheMiss[anIndex]));

        if (theIcachePrefetch[anIndex] && *theIcacheMiss[anIndex] == *theIcachePrefetch[anIndex]) {
            theIcachePrefetch[anIndex] = boost::none;
            prefetchNext(anIndex);
        } else {
            theIcachePrefetch[anIndex] = boost::none;
            issueFetch(*theIcacheMiss[anIndex], *theIcacheVMiss[anIndex]);
            theLastPrefetchVTagSet[anIndex] = tagset;
            prefetchNext(anIndex);
        }
    }

void issuePrefetch(PhysicalMemoryAddress paddr, VirtualMemoryAddress vaddr, int anIndex) {
    theIcachePrefetch[anIndex] = paddr;
    DBG_(Iface, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "."
                            << anIndex << "] L1I PREFETCH " << *theIcachePrefetch[anIndex]));
    issueFetch(paddr, vaddr);
    ++thePrefetches;
}

void prefetchNext(int anIndex) {
    if (theMissQueue.size() >= theMissQueueSize || !cfg.PrefetchEnabled || !theLastPrefetchVTagSet[anIndex]) {
        return;
    }

    ++(*theLastPrefetchVTagSet[anIndex]);
    VirtualMemoryAddress vprefetch(*theLastPrefetchVTagSet[anIndex] << theIndexShift);
    PhysicalMemoryAddress pprefetch = translateAddress(vprefetch, anIndex);

    if (pprefetch == nuArch::kUnresolved || lookup(pprefetch)) {
        theLastPrefetchVTagSet[anIndex] = boost::none;
        return;
    }

    issuePrefetch(pprefetch, vprefetch, anIndex);
}
  bool l1i_lookup(index_t anIndex, VirtualMemoryAddress vaddr) {
    DBG_(VVerb, (<< "Looking up instruction in cache"));

    uint64_t tagset = vaddr >> theIndexShift;
    PhysicalMemoryAddress paddr;

    // Check if it's the last accessed address
    if (tagset == theLastVTagSet) {
        paddr = theLastPhysical;
        if (paddr == nuArch::kUnresolved) {
            DBG_(VVerb, (<< "Last Physical translation lookup failed!"));
            ++theFailedTranslations;
            return true; // Failed translations cause an MMU miss in the pipe.
        }
    } else {
        DBG_(VVerb, (<< "Not in Flexus cache... Will look into Qemu now!"));
        paddr = translateAddress(vaddr, anIndex);
        if (paddr == nuArch::kUnresolved) {
            DBG_(VVerb, (<< "Translation failed!"));
            ++theFailedTranslations;
            return true; // Failed translations cause an MMU miss in the pipe.
        }

        DBG_(VVerb, (<< "Translation success!"));
        // Cache translation
        theLastPhysical = paddr;
        theLastVTagSet = tagset;
    }

    bool hit = lookup(paddr);
    ++theFetchAccesses;

    if (hit) {
        ++theHits;
        if (theLastPrefetchVTagSet[anIndex] && (*theLastPrefetchVTagSet[anIndex] == tagset)) {
            prefetchNext(anIndex);
        }
        return true;
    }

    // Handle cache miss
    ++theMisses;
    DBG_Assert(!theIcacheMiss[anIndex]);
    handleCacheMiss(paddr, vaddr, anIndex, tagset);

    DBG_(VVerb, (<< "in icacheLookup before return false"));
    return false;
 }

  bool consume_fetch_slots(index_t idx) {
        FetchAddr fetch_addr = theFAQ[idx].front();
        VirtualMemoryAddress block_addr(fetch_addr.theAddress & theBlockMask);
        std::set<VirtualMemoryAddress> available_lines;

        if (available_lines.count(block_addr) == 0) {
            // Line needs to be fetched from I-cache
            if (available_lines.size() >= cfg.MaxFetchLines) {
                // Reached limit of I-cache reads per cycle
                return false;
            }

            // Notify the PowerTracker of Icache access
            bool garbage = true;
            FLEXUS_CHANNEL(InstructionFetchSeen) << garbage;

            if (!cfg.PerfectICache && !l1i_lookup(idx, block_addr)) {
                return false;
            }

            if (cfg.PerfectICache) {
                DBG_(Verb, (<< "FETCH UNIT: Instruction Cache disabled!"));
            }

            available_lines.insert(block_addr);
        }

        /* MARK: Pseudo-algorithm for rewritten translate->opcode->output code
         * A) Pop this from the FAQ because it was an I$ hit, add to fetched instruction queue
         * (waiting for opcode) B) Create a "Transaction" that represents the MMU/Opcode access flow
         * C) Append said transaction to something like "outstandingOpcodes", get index
         * D) Associate "Transaction"->index in a hashmap so that when the MMU replies, we set the
         * exact correct opcode E) Rework the response path to look up said hashmap
         */
        theFAQ[idx].pop_front();

        waitingForOpcodeQueue->theOpcodes.emplace_back(
            new FetchedOpcode(
                fetch_addr.theAddress,
                0xefffffff, // op_code not resolved yet - waiting for translation
                fetch_addr.theBPState,
                theFetchReplyTransactionTracker[0]));

        waitingForOpcodeQueue->theFillLevels.emplace_back(new tFillLevel(eL1I));

        DBG_(VVerb, Comp(*this)(<< "added entry in waiting for opcode queue" << fetch_addr.theAddress));
        send_translation_request(0, fetch_addr.theAddress, *std::prev(waitingForOpcodeQueue->theOpcodes.end()));


        return true;
}

  void doFetch(index_t idx){
    FETCH_DBG("--------------START FETCHING------------------------");

    int32_t remaining_fetch = cfg.MaxFetchInstructions;
    int32_t available_fiq = 0;



    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx).available());
    FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx) >> available_fiq;

    if (available_fiq < remaining_fetch) {
      remaining_fetch = available_fiq;
    }

    theAvailableFetchSlots += remaining_fetch;

    if (theIcacheMiss[idx]) {
      ++theMissCycles;
      DBG_(VVerb, (<< "FETCH UNIT: in theIcacheMiss" << theMissCycles.theRefCount
                   << "cycles missed so far"));
      return;
    }

    if (available_fiq <= 0) goto fetch_end;
    if (theFAQ[idx].size() <= 0) goto fetch_end;

    FETCH_DBG("starting to process the fetches..." << remaining_fetch);

    while(consume_fetch_slots(idx))
    {
        theFetches++;
        theUsedFetchSlots++;
        remaining_fetch--;
    }

fetch_end:
    FETCH_DBG("--------------FINISH FETCHING------------------------");
  }

  void sendMisses(){}
  void initialize() {

    //theI.init(cfg.Size, cfg.Associativity, cfg.ICacheLineSize, statName());
    //theIndexShift = LOG2(cfg.ICacheLineSize);
    theBlockMask = ~(cfg.ICacheLineSize - 1);
    theBundleCoreID = flexusIndex();
    waitingForOpcodeQueue = new FetchBundle();
    waitingForOpcodeQueue->coreID = theBundleCoreID;
    theFAQ.resize(cfg.Threads);
    theIcacheMiss.resize(cfg.Threads);
    theIcacheVMiss.resize(cfg.Threads);
    //theFetchReplyTransactionTracker.resize(cfg.Threads);
    //theLastMiss.resize(cfg.Threads);
    //theIcachePrefetch.resize(cfg.Threads);
    //theLastPrefetchVTagSet.resize(cfg.Threads);
    //theMissQueueSize = cfg.MissQueueSize;


    }
  void finalize() { }
  void drive(interface::uFetchDrive const &) {

    bool garbage = true;
    FLEXUS_CHANNEL(ClockTickSeen) << garbage;

    int32_t td = 0;
    //if (cfg.Threads > 1) {
    //  td = nMTManager::MTManager::get()->scheduleFThread(flexusIndex());
    //}
    doFetch(td);
    sendMisses();
    }
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
