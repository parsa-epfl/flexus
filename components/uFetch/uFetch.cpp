

#include "uFetch.hpp"

#include "SimCache.hpp"
#include "components/MTManager/MTManager.hpp"
#include "components/uArch/uArchInterfaces.hpp"
#include "core/exception.hpp"
#include "core/stats.hpp"

#include <deque>
#include <limits>
#include <unordered_map>
#include <utility>

#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories uFetch
#define DBG_SetDefaultOps    AddCat(uFetch)
#include DBG_Control()

using namespace Flexus::Qemu;

namespace nuFetch {

class FLEXUS_COMPONENT(uFetch)
{
    FLEXUS_COMPONENT_IMPL(uFetch);

  private:
    enum FetchState {
        S_INIT,
        S_ITLB_REQ,
        S_ITLB_RESP,
        S_MISS,
        S_DONE,
    };

    struct FetchInfo {
        uint64_t va;
        uint64_t pa;
        uint32_t opcode;

        FetchState state;
        FetchAddr  addr;

        // Next-line measurements only. Keep the first access classification
        // across stalls, retries, miss completion and eventual delivery.
        bool accessCounted = false;

        boost::intrusive_ptr<TransactionTracker> tracker;

        FetchInfo(FetchAddr &a):
            va    (a.theAddress & ~0xffflu),
            pa    (0),
            opcode(0),
            state (S_INIT),
            addr  (a) {
        }
    };

    // ==================== FetchAddressGenerate list =================
    std::list<FetchInfo> theFAQ;

    std::unordered_set<uint64_t> theTAM;
    std::unordered_set<uint64_t> theFAM;

    // Keep the original local settings used by the fetch implementation.
    // Their configured values are loaded once in initialize().
    bool theUseNextLinePath = false;
    bool theNextLinePrefetching = true;

    enum class PrefetchTrigger : uint32_t {
        OnAccess = 0,
        OnMiss = 1,
    };
    PrefetchTrigger thePrefetchTrigger = PrefetchTrigger::OnAccess;

    uint32_t theNextLineCount = 1;
    uint32_t theTagLookupsPerCycle = 1;
    uint32_t thePrefetchQueueSize = 16;

    struct PrefetchRequest {
        uint64_t pa;
        VirtualMemoryAddress pc;
    };
    std::deque<PrefetchRequest> thePrefetchQueue;
    std::unordered_set<uint64_t> theQueuedPrefetches;

    enum class RequestOrigin { Demand, Prefetch };
    enum class NextLineAccess { Hit, PrefetchTimely, Miss, PrefetchNotTimely, Fault };

    struct ResidentLineInfo {
        RequestOrigin origin;
        bool accessed;
    };

    // Measurement-only sidecars, keyed by aligned physical cache-line address.
    // They never control requests, cache replacement, or cycle budgets. An
    // outstanding request retains its original issuer when demand joins it.
    std::unordered_map<uint64_t, RequestOrigin> theNextLineOutstandingOrigins;
    std::unordered_map<uint64_t, ResidentLineInfo> theNextLineResidentLines;

    // ================== STATS ==================
    Flexus::Stat::StatCounter theFetchAccesses;
    Flexus::Stat::StatCounter theFetches;
    Flexus::Stat::StatCounter thePrefetches;
    Flexus::Stat::StatCounter theFailedTranslations;
    Flexus::Stat::StatCounter theL1Misses;
    Flexus::Stat::StatCounter theL1MissesAll;
    Flexus::Stat::StatCounter theL2Misses;
    Flexus::Stat::StatCounter theHits;
    Flexus::Stat::StatCounter theMissCycles;
    Flexus::Stat::StatCounter theAllocations;
    Flexus::Stat::StatMax theMaxOutstandingEvicts;
    Flexus::Stat::StatCounter theAvailableFetchSlots;
    Flexus::Stat::StatCounter theUsedFetchSlots;

    // Per-FAQ-instruction outcomes for the next-line path only. Timely is a
    // subset of normal hits; not-timely is a subset of normal misses. Existing
    // Hits/L1Misses/L1MissesAll retain their lookup/request-based meanings.
    Flexus::Stat::StatCounter theNextLineInstructionsCounted;
    Flexus::Stat::StatCounter theNextLineNormalHits;
    Flexus::Stat::StatCounter theNextLineNormalMisses;
    Flexus::Stat::StatCounter theNextLinePrefetchTimely;
    Flexus::Stat::StatCounter theNextLinePrefetchNotTimely;
    Flexus::Stat::StatCounter theNextLineOverFetch;
    Flexus::Stat::StatCounter theNextLineFaults;

    std::set<uint64_t> theEvictSet;

    // ================== CACHE ========================
    uint32_t theIndexShift;
    uint64_t theBlockMask;

    // The I-cache
    SimCache theI;

    // ================== QUEUE ========================
    std::list<MemoryTransport> theMissQueue;
    std::list<MemoryTransport> theSnoopQueue;
    std::list<MemoryTransport> theReplyQueue;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(uFetch)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
      , theFetchAccesses(statName() + "-FetchAccess")
      , theFetches(statName() + "-Fetches")
      , thePrefetches(statName() + "-Prefetches")
      , theFailedTranslations(statName() + "-FailedTranslations")
      , theL1Misses(statName() + "-L1Misses")
      , theL1MissesAll(statName() + "-L1MissesAll")
      , theL2Misses(statName() + "-L2Misses")
      , theHits(statName() + "-Hits")
      , theMissCycles(statName() + "-MissCycles")
      , theAllocations(statName() + "-Allocations")
      , theMaxOutstandingEvicts(statName() + "-MaxEvicts")
      , theAvailableFetchSlots(statName() + "-FetchSlotsPossible")
      , theUsedFetchSlots(statName() + "-FetchSlotsUsed")
      , theNextLineInstructionsCounted(statName() + "-NextLine-InstructionsCounted")
      , theNextLineNormalHits(statName() + "-NextLine-normal_hit")
      , theNextLineNormalMisses(statName() + "-NextLine-normal_miss")
      , theNextLinePrefetchTimely(statName() + "-NextLine-prefetch_timely")
      , theNextLinePrefetchNotTimely(statName() + "-NextLine-prefetch_not_timely")
      , theNextLineOverFetch(statName() + "-NextLine-over_fetch")
      , theNextLineFaults(statName() + "-NextLine-instruction_fault")
    {
    }

    //   Msutherl: TLB in-out functions
    FLEXUS_PORT_ALWAYS_AVAILABLE(iTranslationIn);
    void push(interface::iTranslationIn const&, TranslationPtr& tr)
    {
        PhysicalMemoryAddress magic =
            cpu(0).translate_va2pa(tr->theVaddr,
                                              tr->getInstruction() ?
                                                  tr->getInstruction()->unprivAccess():
                                                  false);

        // DBG_Assert((tr->thePaddr == magic) || (magic == nuArch::kUnresolved),
                   // Comp(*this)(<< "ERROR: Magic QEMU translation NOT EQUAL TO MMU "
                                  // "Translation. Vaddr = "
                               // << std::hex << tr->theVaddr << std::dec << ", PADDR_MMU = " << std::hex
                               // << tr->thePaddr << std::dec << ", PADDR_QEMU = " << std::hex << magic
                               // << std::dec));

        if (!tr->isPagefault() && (magic == nuArch::kUnresolved))
            tr->setPagefault();

        uint64_t va = tr->theVaddr & ~0xffflu;
        uint64_t n  = 0;

        for (auto &f: theFAQ) {
            if (f.state != S_ITLB_REQ)
                continue;
            if (f.va != va)
                continue;

            f.pa    = tr->isPagefault() ? ~0lu : ((magic & ~0xffflu) | (f.addr.theAddress & 0xffflu & theBlockMask));
            f.state = tr->isPagefault() ? S_DONE : S_ITLB_RESP;

            n++;
        }

        theTAM.erase(va);

        DBG_(VVerb, (<< "recving trans " << tr->theVaddr << " " << magic << " wakeup " << n));
    }

    // FetchAddressIn
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchAddressIn);
    void push(interface::FetchAddressIn const&, index_t anIndex, boost::intrusive_ptr<FetchCommand>& aCommand)
    {
        for (auto &f: aCommand->theFetches)
            theFAQ.emplace_back(f);
    }

    // AvailableFAQOut
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(AvailableFAQOut);
    int32_t pull(interface::AvailableFAQOut const&, index_t anIndex) { return cfg.FAQSize - theFAQ.size(); }

    // SquashIn
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashIn);
    void push(interface::SquashIn const&, index_t anIndex, eSquashCause& aReason)
    {
        theFAQ.clear();
        theTAM.clear();
        // Discard unissued candidates from the old path, but keep issued misses
        // in theFAM until their replies arrive, just like demand requests.
        thePrefetchQueue.clear();
        theQueuedPrefetches.clear();
        //theFAM should not be cleared here, since it is used to track the addresses that
        // are currently being fetched and squashing should not affect that.
        //theFAM.clear();
    }

    // FetchMissIn
    FLEXUS_PORT_ALWAYS_AVAILABLE(FetchMissIn);
    void push(interface::FetchMissIn const&, MemoryTransport& aTransport)
    {
        DBG_(VVerb,
             Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                         << "] Fetch Miss Reply Received on Port FMI: " << *aTransport[MemoryMessageTag]));
        recv_fetch(aTransport);
    }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ICount);
    int32_t pull(ICount const&, index_t anIndex) { return theFAQ.size(); }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const&, index_t anIndex)
    {
        int fiq = 0;
        FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex) >> fiq;

        return (theFAQ.size() == cfg.FAQSize) || !fiq;
    }

    void push(interface::ResyncIn const&, index_t anIndex, int& aResync) {
    }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ResyncIn);

    // =============================== LOGIC =========================

    NextLineAccess classifyNextLineAccess(uint64_t pa, bool hit) const
    {
        pa &= theBlockMask;
        if (hit) {
            auto resident = theNextLineResidentLines.find(pa);
            return resident != theNextLineResidentLines.end() &&
                       resident->second.origin == RequestOrigin::Prefetch
                     ? NextLineAccess::PrefetchTimely : NextLineAccess::Hit;
        }

        // Outstanding provenance takes precedence over a queued duplicate. A
        // prefetch candidate cannot claim a miss already issued by demand.
        if (theFAM.count(pa)) {
            auto pending = theNextLineOutstandingOrigins.find(pa);
            return pending != theNextLineOutstandingOrigins.end() &&
                       pending->second == RequestOrigin::Prefetch
                     ? NextLineAccess::PrefetchNotTimely : NextLineAccess::Miss;
        }
        return theQueuedPrefetches.count(pa)
                 ? NextLineAccess::PrefetchNotTimely : NextLineAccess::Miss;
    }

    void markNextLineLineAccessed(uint64_t pa)
    {
        if (!theUseNextLinePath)
            return;
        pa &= theBlockMask;
        // Lines restored from an existing checkpoint have no saved prefetch
        // provenance; treat them as demand-origin lines when first accessed.
        auto resident = theNextLineResidentLines.emplace(pa, ResidentLineInfo{RequestOrigin::Demand, false});
        resident.first->second.accessed = true;
    }

    void recordNextLineAccess(FetchInfo& f, NextLineAccess access)
    {
        if (!theUseNextLinePath)
            return;

        // Even a previously counted miss may retry and hit a newly installed
        // line. Record that residence's use without changing the first outcome.
        if (!cfg.PerfectICache && (access == NextLineAccess::Hit || access == NextLineAccess::PrefetchTimely))
            markNextLineLineAccessed(f.pa);
        if (f.accessCounted)
            return;
        f.accessCounted = true;
        ++theNextLineInstructionsCounted;

        switch (access) {
            case NextLineAccess::PrefetchTimely:
                ++theNextLinePrefetchTimely;
                ++theNextLineNormalHits;
                break;
            case NextLineAccess::Hit:
                ++theNextLineNormalHits;
                break;
            case NextLineAccess::PrefetchNotTimely:
                ++theNextLinePrefetchNotTimely;
                ++theNextLineNormalMisses;
                break;
            case NextLineAccess::Miss:
                ++theNextLineNormalMisses;
                break;
            case NextLineAccess::Fault:
                // A translation-fault placeholder never accessed the I-cache.
                ++theNextLineFaults;
                break;
        }
    }

    void recordNextLineRequest(uint64_t pa, RequestOrigin origin)
    {
        if (theUseNextLinePath)
            theNextLineOutstandingOrigins.emplace(pa & theBlockMask, origin);
    }

    void recordNextLineFill(uint64_t pa)
    {
        if (!theUseNextLinePath)
            return;
        pa &= theBlockMask;
        auto pending = theNextLineOutstandingOrigins.find(pa);
        const RequestOrigin origin = pending == theNextLineOutstandingOrigins.end()
                                       ? RequestOrigin::Demand : pending->second;
        // Only an actual insertion starts a residence. A duplicate reply must
        // not reset a resident line's origin or its accessed bit.
        theNextLineResidentLines.emplace(pa, ResidentLineInfo{origin, false});
        theNextLineOutstandingOrigins.erase(pa);
    }

    void recordNextLineRemoval(uint64_t pa)
    {
        if (!theUseNextLinePath)
            return;
        auto resident = theNextLineResidentLines.find(pa & theBlockMask);
        if (resident == theNextLineResidentLines.end())
            return;
        // Eviction and invalidation both end a residence. Still-resident unused
        // lines are not charged at shutdown: this measures actual removals.
        if (resident->second.origin == RequestOrigin::Prefetch && !resident->second.accessed)
            ++theNextLineOverFetch;
        theNextLineResidentLines.erase(resident);
    }

    Processor cpu(index_t anIndex) {
        return Processor::getProcessor(flexusIndex() * cfg.Threads + anIndex);
    }

    void send_trans(index_t anIndex, VirtualMemoryAddress const& anAddress)
    {
        TranslationPtr tr(new Translation());
        tr->theVaddr     = anAddress;
        tr->theType      = Translation::eFetch;
        tr->theException = 0;
        tr->setInstr();

        DBG_(VVerb, (<< "sending trans " << tr->theVaddr));

        FLEXUS_CHANNEL(iTranslationOut) << tr;
    }

    boost::intrusive_ptr<TransactionTracker> send_fetch(PhysicalMemoryAddress pa, VirtualMemoryAddress pc, bool is_head)
    {
        boost::intrusive_ptr<MemoryMessage> mm(MemoryMessage::newFetch(pa, pc));
        mm->reqSize() = 64;

        boost::intrusive_ptr<TransactionTracker> tt(new TransactionTracker);
        tt->setAddress  (pa);
        tt->setInitiator(flexusIndex());
        tt->setFetch    (true);
        if (is_head)
            tt->setSource   ("uFetchHead");
        else
            tt->setSource   ("uFetch");

        MemoryTransport mt;
        mt.set(TransactionTrackerTag, tt);
        mt.set(MemoryMessageTag,      mm);

        theMissQueue.push_back(std::move(mt));

        DBG_(VVerb, (<< "sending fetch " << pa));

        return tt;
    }

    void doFetch(index_t idx)
    {
        FETCH_DBG("--------------START FETCHING------------------------");
        
        int fetches = 0;
        int tlbreqs = 4; //this should be parametrized
        int l1ireqs = cfg.MaxFetchLines; // ufetch ports

        // Translation-entry checks remain local; cache-entry checks are shared
        // with the next-line path through the component configuration.
        int tlbdeps = 16;
        int l1ideps = cfg.MaxICacheEntryChecks;

        // Get the number of fetches that can be sent this cycle based on 
        // instruction queue availability and the max fetch instructions config parameter
        FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx) >> fetches;
        fetches = std::min(fetches, (int)(cfg.MaxFetchInstructions));

        DBG_(VVerb, (<< "fetches: " << fetches << " entries: " << theFAQ.size()));

        // lines that hit in the cache and looked up in this cycle
        std::unordered_set<uint64_t> l1ihits;

        pFetchBundle bundle;
        bool is_head;

        for (auto t = theFAQ.begin(); t != theFAQ.end(); ) {
            auto &f = *t;

            DBG_(VVerb, (<< "  " << f.addr.theAddress << std::hex << " " << f.pa << " " << f.state));

            switch (f.state) {
                case S_INIT:
                    if (tlbdeps) {
                        tlbdeps--;

                        VirtualMemoryAddress va(f.addr.theAddress & ~0xffflu);

                        if (theTAM.count(va))
                            f.state = S_ITLB_REQ;

                        else if (tlbreqs) {
                            tlbreqs--;

                            theTAM.insert(va);
                            f.state = S_ITLB_REQ;

                            send_trans(idx + flexusIndex() * cfg.Threads, f.addr.theAddress);
                        }
                    }

                    t++;
                    continue;

                case S_ITLB_REQ:
                case S_MISS:
                    t++;
                    continue;
                default:            // TLB Responses are handled next, because they can arrive in the same clock cycle
                    t++;
                    continue;
            }
        }

        for (auto t = theFAQ.begin(); t != theFAQ.end(); ) {    // handle the remaining cases
            auto &f = *t;

            DBG_(VVerb, (<< "  " << f.addr.theAddress << std::hex << " " << f.pa << " " << f.state));

            switch (f.state) {
                case S_ITLB_RESP:
                    if (l1ideps) {
                        if (!cfg.PerfectICache)
                            l1ideps--;

                        PhysicalMemoryAddress pa(f.pa);

                        if (cfg.PerfectICache || l1ihits.count(pa))
                            f.state = S_DONE;

                        else if (theFAM.count(pa))
                            f.state = S_MISS;

                        else if (l1ireqs) {
                            l1ireqs--;

                            if (theI.lookup(pa)) {
                                theHits++;
                                l1ihits.insert(pa);
                                f.state = S_DONE;

                            } else if (theFAM.size() < cfg.MissQueueSize) {
                                is_head = false;
                                if (t == theFAQ.begin()) {
                                    theL1Misses++;
                                    is_head = true;
                                }
                                theL1MissesAll++;
                                send_fetch(pa, f.addr.theAddress, is_head);

                                theFAM.insert(pa);
                                f.state = S_MISS;
                            }
                        }
                    }

                    fetches = 0;
                    t++;
                    continue;

                case S_DONE:
                    /*
                     * TODO: Revalidate L1-I residency before delivering to decode.
                     * An entry can remain S_DONE after its line is evicted or
                     * invalidated while it waits in the FAQ. Delivery currently
                     * reads the opcode from QEMU without another cache lookup,
                     * so it can bypass the miss penalty for a line no longer in L1-I.
                     */
                    
                    if (fetches) {
                        fetches--;

                        if (bundle.get() == nullptr) {
                            bundle.reset(new FetchBundle);
                            bundle->coreID = flexusIndex();
                        }

                        // pa is all-one if page fault happens
                        auto opcode = ~f.pa ? cpu(0).fetch_inst(f.addr.theAddress) : 0xefffffff;

                        bundle->theOpcodes.emplace_back(
                            new FetchedOpcode(f.addr.theAddress, PhysicalMemoryAddress(f.pa), opcode, f.addr.theBPState, f.tracker));
                        bundle->theFillLevels.emplace_back(
                            new tFillLevel(eL1I));

                        DBG_Assert(t == theFAQ.begin());
                        t = theFAQ.erase(t);
                        continue;
                    } else {
                        t++;
                        continue;
                    }

                default:         // Already handled before
                    fetches = 0; // But do not add the next ones into bundle
                    t++;
                    continue;
            }
        }

        if (bundle.get())
            DBG_(VVerb, (<< "sending " << bundle->theOpcodes.size()));

        if (bundle.get() && bundle->theOpcodes.size())
            FLEXUS_CHANNEL_ARRAY(FetchBundleOut, idx) << bundle;

        FETCH_DBG("--------------FINISH FETCHING------------------------");
    }

    bool shouldTriggerNextLinePrefetch(bool accessed, bool demand_missed) const
    {
        // Waiting in S_MISS/S_DONE without a new access is never a new trigger.
        if (!accessed)
            return false;

        switch (thePrefetchTrigger) {
            case PrefetchTrigger::OnAccess:
                return true;
            case PrefetchTrigger::OnMiss:
                return demand_missed;
        }
        return false;
    }

    void enqueueNextLines(uint64_t demand_line, VirtualMemoryAddress pc)
    {
        // Match the 4 KiB page granularity used by fetch translation.
        constexpr uint64_t page_size = 4096;
        uint64_t pa = demand_line & theBlockMask;

        // N counts physical positions, including duplicates/outstanding lines.
        // Enqueue only: cache tags are checked later using their own cycle budget.
        for (uint32_t line = 0; line < theNextLineCount; ++line) {
            // Check before adding, so even the final physical page cannot wrap.
            if (cfg.ICacheLineSize >= page_size - (pa & (page_size - 1)))
                break;
            pa += cfg.ICacheLineSize;

            if (theFAM.count(pa) || theQueuedPrefetches.count(pa))
                continue;

            // Keep older candidates when full; discard the rest of this trigger
            // without delaying demand fetching.
            if (thePrefetchQueue.size() >= thePrefetchQueueSize)
                break;

            thePrefetchQueue.push_back({pa, pc});
            theQueuedPrefetches.insert(pa);
        }
    }

    void issueNextLinePrefetches(uint32_t unused_fetch_lookups)
    {
        // Keep fetch capacity separate: a tag hit needs only a probe, while a
        // tag miss also needs a remaining demand-port slot to issue its fetch.
        // Spare demand slots still augment tag capacity as well. This budget
        // arithmetic performs no access and does not update LRU state.
        // Widen before addition so the two uint32_t budgets cannot overflow.
        uint64_t tag_lookups = uint64_t(theTagLookupsPerCycle) + unused_fetch_lookups;
        uint32_t fetch_accesses = unused_fetch_lookups;

        // Service the FIFO even when no demand access/delivery occurred this
        // cycle. Exhausting either budget leaves work queued for a later cycle.
        while (!thePrefetchQueue.empty()) {
            const auto& request = thePrefetchQueue.front();

            // A demand or an earlier prefetch may have requested this line
            // since it was enqueued. Merging needs no cache tag probe.
            if (!theFAM.count(request.pa)) {
                if (tag_lookups == 0)
                    break;
                --tag_lookups;

                // Presence only: no demand data port or replacement-state update.
                if (!theI.lookupTag(request.pa)) {
                    // A miss requires both fetch bandwidth and an outstanding-
                    // request slot. If either is unavailable, stop at this FIFO
                    // head and retry later, including a fresh tag check in case
                    // the line has arrived in the meantime.
                    if (fetch_accesses == 0 || theFAM.size() >= cfg.MissQueueSize)
                        break;

                    --fetch_accesses;
                    send_fetch(PhysicalMemoryAddress(request.pa), request.pc, false);
                    recordNextLineRequest(request.pa, RequestOrigin::Prefetch);
                    theFAM.insert(request.pa);
                    ++thePrefetches;
                    ++theL1MissesAll;
                }
            }

            theQueuedPrefetches.erase(request.pa);
            thePrefetchQueue.pop_front();
        }
    }

    void doFetchNextLine(index_t idx)
    {
        int fetches = 0;
        FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx) >> fetches;
        fetches = std::min(fetches, (int)(cfg.MaxFetchInstructions));

        int tlbreqs = 4;
        int tlbdeps = 16;
        int l1ireqs = cfg.MaxFetchLines;
        int l1ideps = cfg.MaxICacheEntryChecks;

        // Translation may still run ahead, independently of demand delivery.
        for (auto& f : theFAQ) {
            if (f.state != S_INIT || tlbdeps == 0)
                continue;
            --tlbdeps;

            VirtualMemoryAddress va(f.addr.theAddress & ~0xffflu);
            if (theTAM.count(va)) {
                f.state = S_ITLB_REQ;
            } else if (tlbreqs) {
                --tlbreqs;
                theTAM.insert(va);
                f.state = S_ITLB_REQ;
                send_trans(idx + flexusIndex() * cfg.Threads, f.addr.theAddress);
            }
        }

        pFetchBundle bundle;

        // Deliver the already-completed prefix first. Cache accesses below may
        // set S_DONE, but cannot deliver those entries until the next cycle.
        // Delivery itself performs no additional modeled cache access.
        while (fetches > 0 && !theFAQ.empty() && theFAQ.front().state == S_DONE) {
            auto& f = theFAQ.front();
            // Normal hit/miss outcomes were counted when this entry received
            // its cache result, including same-line propagation. Faults bypass
            // that stage; classify them separately rather than inventing a hit.
            if (!f.accessCounted) {
                if (f.pa == ~uint64_t(0))
                    recordNextLineAccess(f, NextLineAccess::Fault);
                else if (cfg.PerfectICache)
                    recordNextLineAccess(f, NextLineAccess::Hit);
                else
                    recordNextLineAccess(f, classifyNextLineAccess(f.pa, theI.lookupTag(f.pa)));
            }
            // Preserve the existing S_DONE semantics, including page faults:
            // a measurement-only tag observation cannot block delivery or
            // issue a new fetch, even if this line has since disappeared.
            if (!bundle.get()) {
                bundle.reset(new FetchBundle);
                bundle->coreID = flexusIndex();
            }
            auto opcode = ~f.pa ? cpu(0).fetch_inst(f.addr.theAddress) : 0xefffffff;
            bundle->theOpcodes.emplace_back(
                new FetchedOpcode(f.addr.theAddress, PhysicalMemoryAddress(f.pa), opcode, f.addr.theBPState, f.tracker));
            bundle->theFillLevels.emplace_back(new tFillLevel(eL1I));
            theFAQ.pop_front();
            --fetches;
        }

        // Only the current head initiates a demand access. This stage is
        // independent of delivery capacity: it can prepare the next head even
        // after finst is exhausted, overlapping lookup and delivery as a pipeline.
        if (!theFAQ.empty() && theFAQ.front().state == S_ITLB_RESP) {
            auto& f = theFAQ.front();
            if (!cfg.PerfectICache)
                --l1ideps;
            bool accessed = false;
            bool demand_missed = false;
            // Keep the CURRENT access outcome separate from the head's first
            // counted outcome: a retried miss can later hit, and new followers
            // sharing that hit must be counted as hits themselves.
            NextLineAccess current_access = NextLineAccess::Hit;

            // Match doFetch: a perfect I-cache bypasses tag/data access, pending
            // misses, and cache budgets. Delivery still waits until next cycle.
            if (cfg.PerfectICache) {
                recordNextLineAccess(f, current_access);
                f.state = S_DONE;
            } else if (theFAM.count(f.pa)) {
                //requests that reach here must be prefetches
                current_access = classifyNextLineAccess(f.pa, false);
                recordNextLineAccess(f, current_access);
                f.state = S_MISS;
                accessed = true;
                ++theL1Misses;
                ++theL1MissesAll;
                // Joining an outstanding request is still a demand miss, even
                // when no new request (or miss-counter increment) is needed.
                demand_missed = true;
            } else if (l1ireqs) {
                --l1ireqs;
                accessed = true;
                if (theI.lookup(f.pa)) {
                    current_access = classifyNextLineAccess(f.pa, true);
                    recordNextLineAccess(f, current_access);
                    ++theHits;
                    f.state = S_DONE;
                } else {
                    // Count the detected miss even if admission is blocked.
                    // Classify before inserting our own demand request, so a
                    // queued-only prefetch still counts as not timely.
                    
                    // A detected miss does not create an OnMiss trigger if no
                    // outstanding-request slot is available yet.
                    if (theFAM.size() < cfg.MissQueueSize) {
                        current_access = classifyNextLineAccess(f.pa, false);
                        recordNextLineAccess(f, current_access);
                        demand_missed = true;
                        ++theL1Misses;
                        ++theL1MissesAll;
                        send_fetch(PhysicalMemoryAddress(f.pa), f.addr.theAddress, true);
                        recordNextLineRequest(f.pa, RequestOrigin::Demand);
                        theFAM.insert(f.pa);
                        f.state = S_MISS;
                    }
                }
            }

            // One line access also serves consecutive translated instructions
            // on that line. This preserves multi-instruction delivery next cycle
            // without giving younger lines their own demand cache lookups.
            // With a perfect I-cache, all consecutive translated entries are
            // ready regardless of line, without consuming the entry-check budget.
            if (f.state == S_DONE || f.state == S_MISS) {
                auto t = theFAQ.begin();
                for (++t; t != theFAQ.end() && l1ideps > 0; ++t) {
                    if (t->state != S_ITLB_RESP || (!cfg.PerfectICache && t->pa != f.pa))
                        break;
                    if (!cfg.PerfectICache)
                        --l1ideps;
                    // Every instruction sharing this access receives exactly
                    // one outcome, before a miss reply can turn it into S_DONE.
                    // recordNextLineAccess(*t, current_access);
                    t->state = f.state;
                }
            }

            // Trigger selection only controls candidate generation. The FIFO
            // continues draining on later cycles without another trigger.
            // Instructions sharing this line have the same N targets, so their
            // candidates are represented by this one enqueue operation.
            if (theNextLinePrefetching && !cfg.PerfectICache &&
                shouldTriggerNextLinePrefetch(accessed, demand_missed))
                enqueueNextLines(f.pa, f.addr.theAddress);
        }

        if (bundle.get())
            FLEXUS_CHANNEL_ARRAY(FetchBundleOut, idx) << bundle;

        // Demand has first use of MaxFetchLines. Remaining slots bound prefetch
        // fetch issuance and also augment the independent tag-probe budget for
        // this cycle. A tag hit does not consume a fetch slot.
        if (theNextLinePrefetching && !cfg.PerfectICache)
            issueNextLinePrefetches(l1ireqs);
    }

    void sendMisses()
    {
        while (!theMissQueue.empty() && FLEXUS_CHANNEL(FetchMissOut).available()) {
            MemoryTransport trans = theMissQueue.front();

            if (!trans[MemoryMessageTag]->isEvictType()) {
                PhysicalMemoryAddress temp(trans[MemoryMessageTag]->address() & theBlockMask);
                if (theEvictSet.find(temp) != theEvictSet.end()) {
                    DBG_(VVerb,
                         Comp(*this)(<< "Trying to fetch block while evict in "
                                        "process, stalling miss: "
                                     << *trans[MemoryMessageTag]));
                    break;
                }
            }

            DBG_(VVerb,
                 Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                             << "] L1I Sending Miss on Port FMO: " << *trans[MemoryMessageTag]));
            ;

            trans[MemoryMessageTag]->coreIdx() = flexusIndex();
            FLEXUS_CHANNEL(FetchMissOut) << trans;
            theMissQueue.pop_front();
        }

        while (!theSnoopQueue.empty() && FLEXUS_CHANNEL(FetchSnoopOut).available()) {
            MemoryTransport trans = theSnoopQueue.front();
            DBG_(VVerb,
                 Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                             << "] L1I Sending Snoop on Port FSO: " << *trans[MemoryMessageTag]));
            ;
            trans[MemoryMessageTag]->coreIdx() = flexusIndex();
            FLEXUS_CHANNEL(FetchSnoopOut) << trans;
            theSnoopQueue.pop_front();
        }

        if (cfg.UseReplyChannel) {
            while (!theReplyQueue.empty() && FLEXUS_CHANNEL(FetchReplyOut).available()) {
                MemoryTransport trans = theReplyQueue.front();
                DBG_(VVerb,
                     Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                                 << "] L1I Sending Reply on Port FRO: " << *trans[MemoryMessageTag]));
                ;
                trans[MemoryMessageTag]->coreIdx() = flexusIndex();
                FLEXUS_CHANNEL(FetchReplyOut) << trans;
                theReplyQueue.pop_front();
            }
        }
    }

    void initialize()
    {
        DBG_Assert(cfg.Threads == 1);

        if (cfg.PrefetchTrigger != static_cast<uint32_t>(PrefetchTrigger::OnAccess) &&
            cfg.PrefetchTrigger != static_cast<uint32_t>(PrefetchTrigger::OnMiss))
            throw Flexus::Core::FlexusException("uFetch PrefetchTrigger must be 0 (OnAccess) or 1 (OnMiss)");
        // Both fetch paths keep their original int cycle budget. Require a
        // positive, representable configured limit before assigning that budget.
        if (cfg.MaxICacheEntryChecks == 0 || cfg.MaxICacheEntryChecks > std::numeric_limits<int>::max())
            throw Flexus::Core::FlexusException("uFetch MaxICacheEntryChecks must be between 1 and INT_MAX");

        // Feed configuration into the original settings; keep the fetch logic
        // using its existing names and typed trigger selection.
        theUseNextLinePath = cfg.UseNextLinePath;
        theNextLinePrefetching = cfg.NextLinePrefetching;
        thePrefetchTrigger = static_cast<PrefetchTrigger>(cfg.PrefetchTrigger);
        theNextLineCount = cfg.NextLineCount;
        theTagLookupsPerCycle = cfg.TagLookupsPerCycle;
        thePrefetchQueueSize = cfg.PrefetchQueueSize;

        theI.init(cfg.Size, cfg.Associativity, cfg.ICacheLineSize, statName());

        theIndexShift = LOG2(cfg.ICacheLineSize);
        theBlockMask  = ~(cfg.ICacheLineSize - 1);
    }

    void finalize() {
    }

    void drive(interface::uFetchDrive const&)
    {

        bool garbage = true;
        FLEXUS_CHANNEL(ClockTickSeen) << garbage;

        int32_t td = 0;

        if (cfg.Threads > 1) { td = nMTManager::MTManager::get()->scheduleFThread(flexusIndex()); }

        if (theUseNextLinePath)
            doFetchNextLine(td);
        else
            doFetch(td);
        sendMisses();
    }

    bool invalidate(PhysicalMemoryAddress const& anAddress)
    {
        const bool invalidated = theI.inval(anAddress);
        if (invalidated && theUseNextLinePath)
            recordNextLineRemoval(anAddress);
        return invalidated;
    }
    void issueEvict(PhysicalMemoryAddress anAddress)
    {
        if (!cfg.CleanEvict || anAddress == 0) return;

        auto operation = boost::intrusive_ptr<MemoryMessage>(new MemoryMessage(MemoryMessage::EvictClean, anAddress));
        operation->reqSize()     = 64;
        operation->ackRequired() = true;

        std::pair<std::set<uint64_t>::iterator, bool> inserted = theEvictSet.insert(anAddress);
        DBG_Assert(inserted.second);
        theMaxOutstandingEvicts << theEvictSet.size();

        auto tracker = boost::intrusive_ptr<TransactionTracker>(new TransactionTracker);
        tracker->setAddress(anAddress);
        tracker->setInitiator(flexusIndex());

        MemoryTransport transport;
        transport.set(TransactionTrackerTag, tracker);
        transport.set(MemoryMessageTag, operation);

        (cfg.EvictOnSnoop ? theSnoopQueue : theMissQueue).push_back(transport);
    }

    PhysicalMemoryAddress l1i_insert(const PhysicalMemoryAddress& anAddress)
    {

        if (!theI.lookup(anAddress)) {
            ++theAllocations;
            if (!theUseNextLinePath)
                return PhysicalMemoryAddress(theI.insert(anAddress));

            // Observe actual array replacement, independent of CleanEvict and
            // including victim address zero, whose old return value is ambiguous.
            bool replaced = false;
            const uint64_t victim = theI.insert(anAddress, &replaced);
            if (replaced)
                recordNextLineRemoval(victim);
            recordNextLineFill(anAddress);
            return PhysicalMemoryAddress(victim);
        }

        if (theUseNextLinePath)
            theNextLineOutstandingOrigins.erase(uint64_t(anAddress) & theBlockMask);

        return PhysicalMemoryAddress(0);
    }

    void push_back_to_channel(MemoryTransport& aTransport,
                              MemoryMessage::MemoryMessageType aType,
                              PhysicalMemoryAddress& anAddress,
                              bool aContainsData = false)
    {
        boost::intrusive_ptr<MemoryMessage> msg = new MemoryMessage(aType, anAddress);
        msg->reqSize()                          = cfg.ICacheLineSize;
        if (aType == MemoryMessage::FetchAck) { msg->ackRequiresData() = aContainsData; }
        aTransport.set(MemoryMessageTag, msg);
        if (cfg.UseReplyChannel) {
            theReplyQueue.push_back(aTransport);
        } else {
            theSnoopQueue.push_back(aTransport);
        }
    }

    void recv_fetch(MemoryTransport& aTransport)
    {

        boost::intrusive_ptr<MemoryMessage> reply        = aTransport[MemoryMessageTag];
        boost::intrusive_ptr<TransactionTracker> tracker = aTransport[TransactionTrackerTag];

        // The FETCH UNIT better only get load replies
        // DBG_Assert (reply->type() == MemoryMessage::FetchReply);

        switch (reply->type()) {
            case MemoryMessage::FwdReply:      // JZ: For new protoocl
            case MemoryMessage::FwdReplyOwned: // JZ: For new protoocl
            case MemoryMessage::MissReply:
            case MemoryMessage::FetchReply:
            case MemoryMessage::MissReplyWritable: {
                // Insert the address into the array
                PhysicalMemoryAddress replacement = l1i_insert(reply->address());
                if (tracker->source()) {
                    if ((*(tracker->source()) == "uFetchHead") && (tracker->fillLevel() == eLocalMem))
                        theL2Misses++;
                }

                issueEvict(replacement);

                // See if it is our outstanding miss or our outstanding prefetch
                for (uint32_t i = 0; i < cfg.Threads; ++i) {
                    auto n = 0;

                    for (auto &f: theFAQ) {
                        if (f.state != S_MISS)
                            continue;
                        if (f.pa != reply->address())
                            continue;

                        // A fill serving an active demand is used, even though
                        // its instructions were already counted as misses. If
                        // all waiters were squashed, the new residence stays unused.
                        if (theUseNextLinePath)
                            markNextLineLineAccessed(f.pa);
                        f.state   = S_DONE;
                        f.tracker = tracker;

                        n++;
                    }

                    theFAM.erase(reply->address());

                    DBG_(VVerb, (<< "recving fetch " << reply->address() << " wakeup " << n));
                }

                // Send an Ack if necessary
                if (cfg.SendAcks && reply->ackRequired()) {
                    DBG_Assert((aTransport[DestinationTag]));
                    aTransport[DestinationTag]->type = DestinationMessage::Directory;

                    // We should include data in the ack iff:
                    //   we received a reply directly from memory and we want to send a copy
                    //   to the L2 we received a reply from a peer cache (if the L2 had a
                    //   copy, it would have replied instead of Fwding the request)
                    MemoryMessage::MemoryMessageType ack_type = MemoryMessage::FetchAck;
                    bool contains_data                        = reply->ackRequiresData();
                    if (reply->type() == MemoryMessage::FwdReplyOwned) {
                        contains_data = true;
                        ack_type      = MemoryMessage::FetchAckDirty;
                    }
                    push_back_to_channel(aTransport, ack_type, reply->address(), contains_data);
                }
                break;
            }

            case MemoryMessage::Invalidate:

                invalidate(reply->address());
                if (aTransport[DestinationTag]) { aTransport[DestinationTag]->type = DestinationMessage::Requester; }
                push_back_to_channel(aTransport, MemoryMessage::InvalidateAck, reply->address());
                break;

            case MemoryMessage::ReturnReq:
                push_back_to_channel(aTransport, MemoryMessage::ReturnReply, reply->address());
                break;

            case MemoryMessage::Downgrade:
                // We can always reply to this message, regardless of the hit status
                push_back_to_channel(aTransport, MemoryMessage::DowngradeAck, reply->address());
                break;

            case MemoryMessage::ReadFwd:
            case MemoryMessage::FetchFwd:
                // If we have the data, send a FwdReply
                // Otherwise send a FwdNAck
                DBG_Assert(aTransport[DestinationTag]);
                aTransport[TransactionTrackerTag]->setFillLevel(Flexus::SharedTypes::ePeerL1Cache);
                aTransport[TransactionTrackerTag]->setPreviousState(eShared);
                if (theI.lookup(reply->address())) {
                    aTransport[DestinationTag]->type = DestinationMessage::Requester;
                    push_back_to_channel(aTransport, MemoryMessage::FwdReply, reply->address());
                } else {
                    std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
                    if (iter != theEvictSet.end()) {
                        aTransport[DestinationTag]->type = DestinationMessage::Requester;
                        push_back_to_channel(aTransport, MemoryMessage::FwdReply, reply->address());
                    } else {
                        aTransport[DestinationTag]->type = DestinationMessage::Directory;
                        push_back_to_channel(aTransport, MemoryMessage::FwdNAck, reply->address());
                    }
                }
                break;

            case MemoryMessage::WriteFwd:
                // similar, but invalidate data
                DBG_Assert(aTransport[DestinationTag]);
                aTransport[TransactionTrackerTag]->setFillLevel(Flexus::SharedTypes::ePeerL1Cache);
                aTransport[TransactionTrackerTag]->setPreviousState(eShared);
                if (invalidate(reply->address())) {
                    aTransport[DestinationTag]->type = DestinationMessage::Requester;
                    push_back_to_channel(aTransport, MemoryMessage::FwdReplyWritable, reply->address());
                } else {
                    std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
                    if (iter != theEvictSet.end()) {
                        aTransport[DestinationTag]->type = DestinationMessage::Requester;
                        push_back_to_channel(aTransport, MemoryMessage::FwdReplyWritable, reply->address());
                    } else {
                        aTransport[DestinationTag]->type = DestinationMessage::Directory;
                        push_back_to_channel(aTransport, MemoryMessage::FwdNAck, reply->address());
                    }
                }
                break;

            case MemoryMessage::BackInvalidate:
                // Same as invalidate
                aTransport[DestinationTag]->type = DestinationMessage::Directory;

                // small protocol change - always send the InvalAck, whether we have the
                // block or not
                invalidate(reply->address());
                push_back_to_channel(aTransport, MemoryMessage::InvalidateAck, reply->address());
                break;

            case MemoryMessage::EvictAck: {
                std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
                DBG_Assert(iter != theEvictSet.end(),
                           Comp(*this)(<< "Block not found in EvictBuffer upon receipt of Ack: " << *reply));
                theEvictSet.erase(iter);
                break;
            }
            default: DBG_Assert(false, Comp(*this)(<< "FETCH UNIT: Unhandled message received: " << *reply));
        }
    }

    void loadState(std::string const& aDirName)
    {
        // I need to load the instruction cache here.
        this->theI.loadState(aDirName + "/" + statName() + "-L1i.json");
    }

    void saveState(std::string const& aDirName)
    {
        // Not implemented.

    }
};

} // END OF NAMESPACE nuFETCH

FLEXUS_COMPONENT_INSTANTIATOR(uFetch, nuFetch);

FLEXUS_PORT_ARRAY_WIDTH(uFetch, FetchAddressIn)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, SquashIn)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, AvailableFAQOut)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, AvailableFIQ)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, FetchBundleOut)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, ICount)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, Stalled)
{
    return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, ResyncIn)
{
    return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uFetch

#define DBG_Reset
#include DBG_Control()
