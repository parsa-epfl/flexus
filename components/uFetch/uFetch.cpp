

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

        // Transient measurement state for a detected miss that cannot
        // allocate a FAM entry yet. Once issued, the request/cache line carries
        // the classification, so younger FAQ entries do not each count it again.
        // Squash ends an unissued attempt; there is no surviving request or
        // resident line to associate with a later attempt at that address.
        bool accessCounted = false;

        // FDIP candidate discovery is independent of demand readiness. Once an
        // entry is covered by demand, queued, or outstanding, discovery skips it
        // without spending another FAQ-entry check. Queue-full retries stay unseen.
        bool prefetchSeen = false;

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
    struct PendingFetch {
        FetchOrigin origin;
        bool demandObserved;
    };
    // Keep provenance on the actual outstanding line, rather than in a second
    // map. A demand joining a prefetch does not change who issued that request.
    std::unordered_map<uint64_t, PendingFetch> theFAM;

    // Measurement only: lines whose current absence has already caused a head
    // miss. Keep these across squash/retries; a fill ends the missing episode.
    // A later eviction and demand miss for the same line can then count again.
    std::unordered_set<uint64_t> theCountedL1Misses;

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

    // A failed translation has no physical cache line. Count consecutive fault
    // placeholders once per virtual cache block, separately from cache outcomes.
    bool theFetchFaultActive = false;
    uint64_t theFetchFaultLine = 0;

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

    // Shared first-demand outcomes per physical line lifetime in both paths.
    // Hit, miss, timely and not-timely are mutually exclusive. LinesCounted
    // totals those four outcomes, not FAQ instructions. Faults and unused-
    // prefetch removals are separate events.
    // None of these counters participates in theL1Misses accounting.
    Flexus::Stat::StatCounter theFetchLinesCounted;
    Flexus::Stat::StatCounter theFetchNormalHits;
    Flexus::Stat::StatCounter theFetchNormalMisses;
    Flexus::Stat::StatCounter theFetchPrefetchTimely;
    Flexus::Stat::StatCounter theFetchPrefetchNotTimely;
    Flexus::Stat::StatCounter theFetchOverFetch;
    Flexus::Stat::StatCounter theFetchFaults;

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
      , theFetchLinesCounted(statName() + "-FetchTaxonomy-LinesCounted")
      , theFetchNormalHits(statName() + "-FetchTaxonomy-normal_hit")
      , theFetchNormalMisses(statName() + "-FetchTaxonomy-normal_miss")
      , theFetchPrefetchTimely(statName() + "-FetchTaxonomy-prefetch_timely")
      , theFetchPrefetchNotTimely(statName() + "-FetchTaxonomy-prefetch_not_timely")
      , theFetchOverFetch(statName() + "-FetchTaxonomy-over_fetch")
      , theFetchFaults(statName() + "-FetchTaxonomy-instruction_fault")
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
        theFetchFaultActive = false;
        // Changed: squash does not cancel issued requests. Keep their origin
        // and first-demand observation until the reply, avoiding duplicate
        // requests and duplicate line classifications after a redirect.
        // theFAM.clear();
        // Also retain theCountedL1Misses: a redirected head can join the same
        // still-missing line without creating a second visible miss for it.
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

    void recordL1Miss(uint64_t pa)
    {
        // Call only when the actual FAQ head observes a missing cache line.
        // This does not access the cache or change fetch timing/resources.
        // Fault placeholders and the perfect-cache model are not L1-I misses.
        if (cfg.PerfectICache || pa == ~uint64_t(0))
            return;
        if (theCountedL1Misses.insert(pa & theBlockMask).second)
            ++theL1Misses;
    }

    void recordFetchAccess(FetchInfo& f, bool hit)
    {
        // Perfect-cache execution bypasses real tags, requests and residences;
        // it therefore contributes no physical-line access classifications.
        if (cfg.PerfectICache)
            return;

        bool counted = f.accessCounted;
        bool prefetch = false;
        if (hit) {
            auto* line = theI.lineInfo(f.pa);
            DBG_Assert(line);
            counted = counted || line->used;
            prefetch = line->origin == FetchOrigin::Prefetch;
            line->used = true;
        } else {
            auto pending = theFAM.find(f.pa);
            if (pending != theFAM.end()) {
                auto& request = pending->second;
                counted = counted || request.demandObserved;
                prefetch = request.origin == FetchOrigin::Prefetch;
                request.demandObserved = true;
            }
        }

        // Retain this bit only to bridge a capacity-blocked, unissued miss to
        // its eventual FAM allocation. Resident/FAM metadata deduplicates all
        // other FAQ entries, including later accesses to the same physical line.
        f.accessCounted = true;
        if (counted)
            return;

        ++theFetchLinesCounted;
        if (hit) {
            if (prefetch)
                ++theFetchPrefetchTimely;
            else
                ++theFetchNormalHits;
        } else {
            // A queued candidate has not issued a request. Only the actual
            // outstanding issuer can make a demand miss a late prefetch.
            if (prefetch)
                ++theFetchPrefetchNotTimely;
            else
                ++theFetchNormalMisses;
        }
    }

    void trackOutstandingFetch(uint64_t pa, FetchOrigin origin)
    {
        // Both paths classify a demand miss before issuing it. Carry that
        // observation in FAM across stalls/squashes; prefetches begin unobserved.
        const bool observed = origin == FetchOrigin::Demand;
        theFAM.emplace(pa & theBlockMask, PendingFetch{origin, observed});
    }

    void recordFetchFault(const FetchInfo& f)
    {
        // Fault placeholders carry pa == ~0, not a physical line address.
        // Group consecutive delivered placeholders by their virtual cache line
        // rather than treating all faults as one physical line or counting each
        // instruction. A valid delivery or squash ends this fault episode.
        const uint64_t line = uint64_t(f.addr.theAddress) & theBlockMask;
        if (!theFetchFaultActive || line != theFetchFaultLine)
            ++theFetchFaults;
        theFetchFaultLine = line;
        theFetchFaultActive = true;
    }

    void recordFetchRemoval(const SimCacheLineInfo& line)
    {
        // Eviction and invalidation both end a residence. Still-resident unused
        // lines are not charged at shutdown: this measures actual removals.
        if (line.origin == FetchOrigin::Prefetch && !line.used)
            ++theFetchOverFetch;
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
        FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx) >> fetches;
        fetches = std::min(fetches, (int)(cfg.MaxFetchInstructions));
        int tlbreqs = 4;
        int tlbdeps = 16;
        int l1ireqs = cfg.MaxFetchLines;
        int l1ideps = cfg.MaxICacheEntryChecks;

        // Translation may run ahead independently of demand. Passing an entry
        // before its translation completes does not mark it prefetchSeen.
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

        // Deliver the already-completed prefix before preparing the new head,
        // just as in next-line mode. A hit found below delivers next cycle;
        // delivery itself spends no additional cache-access budget.
        while (fetches > 0 && !theFAQ.empty() && theFAQ.front().state == S_DONE) {
            auto& f = theFAQ.front();
            // Faults bypass cache lookup and have their own virtual-line count.
            // Valid instructions were classified at the demand head, never here.
            if (f.pa == ~uint64_t(0))
                recordFetchFault(f);
            else
                theFetchFaultActive = false;
            /*
             * TODO: Revalidate L1-I residency before delivery if required later.
             * S_DONE currently remains latched after eviction/invalidation;
             * delivery reads the opcode from QEMU without another cache lookup.
             * Preserve that existing behavior in this FDIP rework.
             */
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

        // Demand gets the first entry check and cache-data port. Waiting for a
        // miss still checks the head, but does not repeat its data access.
        if (!theFAQ.empty()) {
            auto& f = theFAQ.front();
            if ((f.state == S_ITLB_RESP || f.state == S_MISS) && l1ideps > 0) {
                if (!cfg.PerfectICache)
                    --l1ideps;

                if (f.state == S_MISS) {
                    recordL1Miss(f.pa);
                    // A new FAQ entry can rejoin a retained request after a
                    // squash. Shared FAM metadata suppresses another outcome.
                    recordFetchAccess(f, false);
                } else if (cfg.PerfectICache) {
                    f.state = S_DONE;
                } else if (theFAM.count(f.pa)) {
                    recordL1Miss(f.pa);
                    recordFetchAccess(f, false);
                    ++theL1MissesAll;
                    f.state = S_MISS;
                } else if (l1ireqs) {
                    --l1ireqs;
                    if (theI.lookup(f.pa)) {
                        recordFetchAccess(f, true);
                        ++theHits;
                        f.state = S_DONE;
                    } else {
                        // Detect absence even if FAM is full. Keep the head in
                        // S_ITLB_RESP until allocation succeeds, so its retry
                        // retains demand priority over queued prefetches.
                        recordL1Miss(f.pa);
                        // Observe the first miss before allocation, including a
                        // queued-only candidate, which is an ordinary miss.
                        recordFetchAccess(f, false);
                        if (theFAM.size() < cfg.MissQueueSize) {
                            ++theL1MissesAll;
                            send_fetch(PhysicalMemoryAddress(f.pa), f.addr.theAddress, true);
                            trackOutstandingFetch(f.pa, FetchOrigin::Demand);
                            f.state = S_MISS;
                        }
                    }
                }

                if (f.state == S_DONE || f.state == S_MISS) {
                    // One shared physical-line outcome already covers these
                    // followers. Propagating readiness adds no taxonomy counts.
                    f.prefetchSeen = true;
                    auto t = theFAQ.begin();
                    for (++t; t != theFAQ.end(); ++t) {
                        // A real cache result covers only the consecutive
                        // translated instructions on this physical line. A
                        // perfect-cache hit bypasses line matching and budgets.
                        const bool same_line_required = !cfg.PerfectICache || f.state == S_MISS;
                        if (same_line_required && t->pa != f.pa)
                            break;

                        if (t->state == S_MISS || t->state == S_DONE) {
                            // Do not repeatedly charge old same-line waiters:
                            // new translated arrivals must still be able to join
                            // the pending line in subsequent cycles.
                            t->prefetchSeen = true;
                            continue;
                        }
                        if (t->state != S_ITLB_RESP || l1ideps == 0)
                            break;
                        if (!cfg.PerfectICache)
                            --l1ideps;
                        t->state = f.state;
                        t->prefetchSeen = true;
                    }
                }
            }
        }

        if (!cfg.PerfectICache && !theFAQ.empty()) {
            // Discover candidates only from younger FAQ entries. Do not enqueue
            // the head entry as a prefetch when its demand allocation is blocked.
            // Discovery itself never probes cache tags or changes demand states.
            auto t = theFAQ.begin();
            for (++t; t != theFAQ.end(); ++t) {
                if (t->prefetchSeen || t->state != S_ITLB_RESP)
                    continue;
                if (l1ideps == 0)
                    break;
                --l1ideps;

                if (theFAM.count(t->pa) || theQueuedPrefetches.count(t->pa)) {
                    // Coalesced entries are seen even if the FIFO is full.
                    t->prefetchSeen = true;
                    continue;
                }
                if (thePrefetchQueue.size() >= thePrefetchQueueSize)
                    break; // Leave this entry unseen and retry when space frees.

                thePrefetchQueue.push_back({t->pa, t->addr.theAddress});
                theQueuedPrefetches.insert(t->pa);
                t->prefetchSeen = true;
            }
        }

        if (bundle.get())
            FLEXUS_CHANNEL_ARRAY(FetchBundleOut, idx) << bundle;

        // Reuse the shared FIFO issuer. It independently
        // drains old candidates even when no new discovery/delivery occurred.
        // Only the candidate source differs: FAQ addresses here, next N lines
        // in doFetchNextLine. NextLinePrefetching does not disable FDIP.
        if (!cfg.PerfectICache)
            issuePrefetches(l1ireqs);

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

    void issuePrefetches(uint32_t unused_fetch_lookups)
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
                    trackOutstandingFetch(request.pa, FetchOrigin::Prefetch);
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
            // Physical-line outcomes belong to the first demand access, never
            // delivery. In particular, do not reclassify S_DONE after eviction.
            // Translation faults bypass cache access and have their own count.
            if (f.pa == ~uint64_t(0))
                recordFetchFault(f);
            else
                theFetchFaultActive = false;
            // Preserve the existing S_DONE semantics, including page faults:
            // delivery cannot block or issue a new fetch if the line has since
            // disappeared. This taxonomy change does not add a cache access.
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
        // Observe waiting heads too; line-based deduplication prevents a count
        // on every stalled cycle or after a squash rejoins the same request.
        if (!theFAQ.empty() && theFAQ.front().state == S_MISS)
            recordL1Miss(theFAQ.front().pa);

        if (!theFAQ.empty() &&
            (theFAQ.front().state == S_ITLB_RESP || theFAQ.front().state == S_MISS) && l1ideps > 0) {
            auto& f = theFAQ.front();
            if (!cfg.PerfectICache)
                --l1ideps;
            bool accessed = false;
            bool demand_missed = false;
            if (f.state == S_MISS) {
                // Match FDIP: revisit a waiting head so more same-line entries
                // can join its outstanding fill. This is no new cache access,
                // request, or prefetch trigger; shared metadata deduplicates it.
                recordFetchAccess(f, false);
            } else if (cfg.PerfectICache) {
                // A perfect I-cache bypasses tag/data access, pending misses,
                // and cache budgets. Delivery still waits until next cycle.
                f.state = S_DONE;
            } else if (theFAM.count(f.pa)) {
                // This may join demand or prefetch. Only the recorded issuer
                // determines whether its first demand observation is late.
                recordFetchAccess(f, false);
                f.state = S_MISS;
                accessed = true;
                recordL1Miss(f.pa);
                ++theL1MissesAll;
                // Joining an outstanding request is still a demand miss; the
                // shared line record suppresses repeated counts until its fill.
                demand_missed = true;
            } else if (l1ireqs) {
                --l1ireqs;
                accessed = true;
                if (theI.lookup(f.pa)) {
                    recordFetchAccess(f, true);
                    ++theHits;
                    f.state = S_DONE;
                } else {
                    // Count the head's missing line even if admission is blocked.
                    // This observation changes no request/trigger decisions.
                    recordL1Miss(f.pa);
                    // Count the first detected miss even if allocation must
                    // wait. An unissued prefetch candidate gives no credit:
                    // with no FAM entry this is an ordinary demand miss.
                    recordFetchAccess(f, false);

                    // A detected miss does not create an OnMiss trigger if no
                    // outstanding-request slot is available yet.
                    if (theFAM.size() < cfg.MissQueueSize) {
                        demand_missed = true;
                        ++theL1MissesAll;
                        send_fetch(PhysicalMemoryAddress(f.pa), f.addr.theAddress, true);
                        trackOutstandingFetch(f.pa, FetchOrigin::Demand);
                        f.state = S_MISS;
                    }
                }
            }

            // Match FDIP's demand cohort, including later arrivals while the
            // head waits in S_MISS. The head costs one entry check; each newly
            // attached instruction costs another, but old waiters cost nothing.
            // No extra cache-data access or request is needed to share a fill.
            if (f.state == S_DONE || f.state == S_MISS) {
                auto t = theFAQ.begin();
                for (++t; t != theFAQ.end(); ++t) {
                    // Real requests cover only this physical line. A perfect-
                    // cache hit bypasses line matching and entry-check charges.
                    const bool same_line_required = !cfg.PerfectICache || f.state == S_MISS;
                    if (same_line_required && t->pa != f.pa)
                        break;
                    if (t->state == S_MISS || t->state == S_DONE)
                        continue;
                    if (t->state != S_ITLB_RESP || l1ideps == 0)
                        break;
                    if (!cfg.PerfectICache)
                        --l1ideps;
                    // The shared physical-line metadata already owns the one
                    // classification; propagating readiness adds no counts.
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
            issuePrefetches(l1ireqs);
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
        SimCacheLineInfo removed;
        const bool invalidated = theI.inval(anAddress, &removed);
        if (invalidated)
            recordFetchRemoval(removed);
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
            // Copy provenance directly from the actual outstanding request.
            // A demand fill is used immediately; a joined late prefetch also
            // keeps its prior classification across installation. Unobserved
            // prefetch fills remain eligible for timely/over-fetch classification.
            SimCacheLineInfo incoming{FetchOrigin::Demand, true};
            auto pending = theFAM.find(uint64_t(anAddress) & theBlockMask);
            if (pending != theFAM.end()) {
                incoming.origin = pending->second.origin;
                incoming.used = incoming.origin == FetchOrigin::Demand || pending->second.demandObserved;
            }

            // Observe actual array replacement, independent of CleanEvict and
            // including victim address zero, whose old return value is ambiguous.
            bool replaced = false;
            SimCacheLineInfo removed;
            const uint64_t victim = theI.insert(anAddress, &replaced, incoming, &removed);
            if (replaced)
                recordFetchRemoval(removed);
            return PhysicalMemoryAddress(victim);
        }

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

                        f.state   = S_DONE;
                        f.tracker = tracker;

                        n++;
                    }

                    theFAM.erase(reply->address());

                    DBG_(VVerb, (<< "recving fetch " << reply->address() << " wakeup " << n));
                }

                // The line is available again. End its counted absence even if
                // all waiters were squashed; a future miss is a new episode.
                theCountedL1Misses.erase(uint64_t(reply->address()) & theBlockMask);

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
