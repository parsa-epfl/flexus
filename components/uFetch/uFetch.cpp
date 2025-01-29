

#include "uFetch.hpp"

#include "SimCache.hpp"
#include "components/MTManager/MTManager.hpp"
#include "components/uArch/uArchInterfaces.hpp"
#include "core/stats.hpp"

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
    std::vector<boost::optional<std::pair<PhysicalMemoryAddress, tFillLevel>>> theLastMiss;
    // Indicates whether a prefetch is outstanding, and what paddr was prefetched
    std::vector<boost::optional<PhysicalMemoryAddress>> theIcachePrefetch;
    std::vector<boost::optional<uint64_t>> theLastPrefetchVTagSet;

    // Set of outstanding evicts.
    std::set<uint64_t> theEvictSet;

    // ================== CACHE ========================
    int32_t theBundleCoreID;
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
      , theMisses(statName() + "-Misses")
      , theHits(statName() + "-Hits")
      , theMissCycles(statName() + "-MissCycles")
      , theAllocations(statName() + "-Allocations")
      , theMaxOutstandingEvicts(statName() + "-MaxEvicts")
      , theAvailableFetchSlots(statName() + "-FetchSlotsPossible")
      , theUsedFetchSlots(statName() + "-FetchSlotsUsed")
      , theLastVTagSet(0)
      , theLastPhysical(0)
    {
    }

    //   Msutherl: TLB in-out functions
    FLEXUS_PORT_ALWAYS_AVAILABLE(iTranslationIn);
    void push(interface::iTranslationIn const&, TranslationPtr& retdTranslations)
    {

        ExpectedTranslation_t::iterator tr_iter = translationsExpected.find(retdTranslations);
        if (tr_iter != translationsExpected.end()) {
            update_translation_response(retdTranslations);
            translationsExpected.erase(tr_iter);
        }
        DBG_(VVerb, (<< "Got response from iTranslationIn for PC " << retdTranslations->theVaddr));
    }

    // FetchAddressIn
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchAddressIn);
    void push(interface::FetchAddressIn const&, index_t anIndex, boost::intrusive_ptr<FetchCommand>& aCommand)
    {

        std::copy(aCommand->theFetches.begin(), aCommand->theFetches.end(), std::back_inserter(theFAQ[anIndex]));
    }

    // AvailableFAQOut
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(AvailableFAQOut);
    int32_t pull(interface::AvailableFAQOut const&, index_t anIndex) { return cfg.FAQSize - theFAQ[anIndex].size(); }

    // SquashIn
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashIn);
    void push(interface::SquashIn const&, index_t anIndex, eSquashCause& aReason)
    {

        theFAQ[anIndex].clear();
        theIcacheMiss[anIndex]                   = boost::none;
        theIcacheVMiss[anIndex]                  = boost::none;
        theFetchReplyTransactionTracker[anIndex] = nullptr;
        theIcachePrefetch[anIndex]               = boost::none;
        theLastPrefetchVTagSet[anIndex]          = 0;
        waitingForOpcodeQueue->clear();
        tr_op_bijection.clear();
        translationsExpected.clear();
    }

    // FetchMissIn
    FLEXUS_PORT_ALWAYS_AVAILABLE(FetchMissIn);
    void push(interface::FetchMissIn const&, MemoryTransport& aTransport)
    {

        DBG_(Trace,
             Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                         << "] Fetch Miss Reply Received on Port FMI: " << *aTransport[MemoryMessageTag]));
        fetch_reply(aTransport);
    }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ICount);
    int32_t pull(ICount const&, index_t anIndex) { return theFAQ[anIndex].size(); }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const&, index_t anIndex)
    {
        int32_t available_fiq = 0;
        DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex).available());
        FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex) >> available_fiq;

        return theFAQ[anIndex].empty() || available_fiq == 0 || theIcacheMiss[anIndex];
    }

    void push(interface::ResyncIn const&, index_t anIndex, int& aResync) {}
    bool available(interface::ResyncIn const&, index_t anIndex) { return true; }

    // =============================== LOGIC =========================

    Processor cpu(index_t anIndex) { return Processor::getProcessor(flexusIndex() * cfg.Threads + anIndex); }

    void update_translation_response(TranslationPtr& tr)
    {

        DBG_Assert(tr->isDone() || tr->isHit());
        DBG_(VVerb,
             Comp(*this)(<< "Updating translation response for " << tr->theVaddr << " @ cpu index " << flexusIndex()));
        PhysicalMemoryAddress magicTranslation = cpu(tr->theIndex).translate_va2pa(tr->theVaddr, (tr->getInstruction() ? tr->getInstruction()->unprivAccess(): false));

        if (tr->thePaddr == magicTranslation || magicTranslation == nuArch::kUnresolved) {
            DBG_(VVerb,
                 Comp(*this)(<< "Magic QEMU translation == MMU Translation. Vaddr = " << std::hex << tr->theVaddr
                             << std::dec << ", Paddr = " << std::hex << tr->thePaddr << std::dec));
        } else {
            DBG_Assert(false,
                       Comp(*this)(<< "ERROR: Magic QEMU translation NOT EQUAL TO MMU "
                                      "Translation. Vaddr = "
                                   << std::hex << tr->theVaddr << std::dec << ", PADDR_MMU = " << std::hex
                                   << tr->thePaddr << std::dec << ", PADDR_QEMU = " << std::hex << magicTranslation
                                   << std::dec));
        }
        uint32_t opcode = 0xffffffff;
        // MARK: Look up in the opc bijection and get the correct index
        BijectionMapType_t::iterator bijection_iter = tr_op_bijection.find(tr);
        DBG_AssertSev(Crit,
                      bijection_iter != tr_op_bijection.end(),
                      Comp(*this)(<< "ERROR: Opcode index was NOT found for translationPtr with ID" << tr->theID
                                  << " and address" << tr->theVaddr));

        // respect qemu result as flexus does not have pmp
        // TODO: but this should only happen for access faults
        if (!tr->isPagefault() && (magicTranslation == nuArch::kUnresolved)) tr->setPagefault();

        if (!tr->isPagefault()) opcode = cpu(tr->theIndex).fetch_inst(tr->theVaddr);

        waitingForOpcodeQueue->updateOpcode(tr->theVaddr, bijection_iter->second, opcode);
        // Remove this mapping, opcode is updated
        tr_op_bijection.erase(bijection_iter);
    }

    void send_translation_request(index_t anIndex,
                                  VirtualMemoryAddress const& anAddress,
                                  opcodeQIterator newOpcIterator)
    {

        TranslationPtr xlat{ new Translation() };
        xlat->theVaddr     = anAddress;
        xlat->theType      = Translation::eFetch;
        xlat->theException = 0; // just for now
        xlat->theIndex     = anIndex;
        xlat->setInstr();

        // Insert an entry into the translation<->opcode map and outstanding translations expected
        std::pair<BijectionMapType_t::iterator, bool> bijection_insertion =
          tr_op_bijection.insert(std::make_pair(xlat, newOpcIterator));

        DBG_(VVerb,
             Comp(*this)(<< "Inserting xlat objection with vaddr " << xlat->theVaddr << " pointing to PC "
                         << newOpcIterator->thePC << " into waitingForOpcodesQueue bijection "));
        DBG_AssertSev(Iface,
                      bijection_insertion.second == true,
                      (<< "Inserting xlat object with vaddr " << xlat->theVaddr
                       << " failed!! Clashing object has vaddr: " << bijection_insertion.first->first->theVaddr));

        translationsExpected.emplace(xlat);

        DBG_(VVerb, Comp(*this)(<< "Adding translation request entry for " << xlat->theVaddr));

        DBG_Assert(FLEXUS_CHANNEL(iTranslationOut).available());
        FLEXUS_CHANNEL(iTranslationOut) << xlat;
    }

    void issueFetch(PhysicalMemoryAddress anAddress, VirtualMemoryAddress vPC)
    {
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

    bool is_li1_cache_hit(PhysicalMemoryAddress const& anAddress)
    {
        if (theI.lookup(anAddress)) {
            DBG_(VVerb,
                 Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                             << "] I-Lookup hit: " << anAddress));
            DBG_(Verb,
                 Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                             << "] I-Lookup hit: " << anAddress));
            return true;
        }

        DBG_(VVerb,
             Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                         << "] I-Lookup Miss addr: " << anAddress));
        DBG_(Verb,
             Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                         << "] I-Lookup Miss addr: " << anAddress));
        return false;
    }

    // Helper functions
    PhysicalMemoryAddress translateAddress(uint64_t vaddr, int anIndex)
    {
        Flexus::SharedTypes::Translation xlat;
        xlat.theVaddr = vaddr;
        xlat.theType  = Translation::eFetch;
        xlat.thePaddr = cpu(anIndex).translate_va2pa(xlat.theVaddr, false);
        return xlat.thePaddr;
    }

    void handleCacheMiss(PhysicalMemoryAddress paddr, uint64_t vaddr, int anIndex, uint64_t tagset)
    {
        PhysicalMemoryAddress temp(paddr & theBlockMask);
        theIcacheMiss[anIndex]                   = temp;
        theIcacheVMiss[anIndex]                  = VirtualMemoryAddress(vaddr);
        theFetchReplyTransactionTracker[anIndex] = nullptr;

        DBG_(VVerb,
             Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex
                         << "] L1I MISS " << vaddr << " " << *theIcacheMiss[anIndex]));

        if (theIcachePrefetch[anIndex] && *theIcacheMiss[anIndex] == *theIcachePrefetch[anIndex]) {
            theIcachePrefetch[anIndex] = boost::none;
            // We have already sent a prefetch request out for this miss. No need
            // to request again.  However, we can advance the prefetcher to the
            // next miss
            prefetchNext(anIndex);
        } else {
            theIcachePrefetch[anIndex] = boost::none;
            // Need to issue the miss.
            issueFetch(*theIcacheMiss[anIndex], *theIcacheVMiss[anIndex]);
            // Also issue a new prefetch
            theLastPrefetchVTagSet[anIndex] = tagset;
            prefetchNext(anIndex);
        }
    }

    void issuePrefetch(PhysicalMemoryAddress paddr, VirtualMemoryAddress vaddr, int anIndex)
    {
        theIcachePrefetch[anIndex] = paddr;
        DBG_(Iface,
             Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex
                         << "] L1I PREFETCH " << *theIcachePrefetch[anIndex]));
        issueFetch(paddr, vaddr);
        ++thePrefetches;
    }

    void prefetchNext(int anIndex)
    {

        // Limit the number of prefetches. With some backpressure, the number of
        // outstanding prefetches can otherwise be unbounded.
        if (theMissQueue.size() >= cfg.MissQueueSize) return;
        if (!cfg.PrefetchEnabled) return;
        if (!theLastPrefetchVTagSet[anIndex]) return;

        // Prefetch the line following theLastPrefetchVTagSet
        //(if it has a valid translation)
        ++(*theLastPrefetchVTagSet[anIndex]);
        VirtualMemoryAddress vprefetch(*theLastPrefetchVTagSet[anIndex] << theIndexShift);
        PhysicalMemoryAddress pprefetch = translateAddress(vprefetch, anIndex);

        if (pprefetch == nuArch::kUnresolved || is_li1_cache_hit(pprefetch)) {
            theLastPrefetchVTagSet[anIndex] = boost::none;
            return;
        }

        issuePrefetch(pprefetch, vprefetch, anIndex);
    }
    bool l1i_lookup(index_t anIndex, VirtualMemoryAddress vaddr)
    {
        // Translate virtual address to physical.
        // First, see if it is our cached translation
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
            theLastVTagSet  = tagset;
        }

        bool hit = is_li1_cache_hit(paddr);
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

    bool consume_fetch_slots(index_t idx)
    {
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

            if (cfg.PerfectICache) {
                DBG_(Verb, (<< "FETCH UNIT: Instruction Cache disabled!"));
            } else {
                if (!l1i_lookup(idx, block_addr)) return false;
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
          new FetchedOpcode(fetch_addr.theAddress,
                            0xefffffff, // op_code not resolved yet - waiting for translation
                            fetch_addr.theBPState,
                            theFetchReplyTransactionTracker[idx]));

        waitingForOpcodeQueue->theFillLevels.emplace_back(new tFillLevel(eL1I));

        DBG_(VVerb, Comp(*this)(<< "added entry in waiting for opcode queue" << fetch_addr.theAddress));
        send_translation_request(idx, fetch_addr.theAddress, *std::prev(waitingForOpcodeQueue->theOpcodes.end()));

        return (theFAQ[idx].size() > 0);
    }

    void process_available(uint32_t available_fiq)
    {

        if (waitingForOpcodeQueue->theOpcodes.size() == 0) return;
        if ((*waitingForOpcodeQueue->theOpcodes.begin())->theOpcode == 0xefffffff) return;

        pFetchBundle bundle(new FetchBundle);
        bundle->coreID = theBundleCoreID;

        // Only pop fetched instructions up to the limit of decoder FIQ
        while ((waitingForOpcodeQueue->theOpcodes.size() > 0) && (bundle->theOpcodes.size() < available_fiq)) {
            auto i         = waitingForOpcodeQueue->theOpcodes.begin();
            auto iter      = *i;
            auto fill_iter = waitingForOpcodeQueue->theFillLevels.begin();

            if (iter->theOpcode == 0xefffffff) break;

            bundle->theOpcodes.emplace_back(*i);
            bundle->theFillLevels.emplace_back(*fill_iter);
            DBG_(VVerb, Comp(*this)(<< "popping entry out of the waitingForOpcodeQueue " << iter->thePC));
            waitingForOpcodeQueue->theOpcodes.erase(i);
            waitingForOpcodeQueue->theFillLevels.erase(fill_iter);
        }

        if (bundle->theOpcodes.size() > 0) { FLEXUS_CHANNEL_ARRAY(FetchBundleOut, 0) << bundle; }
    }

    void doFetch(index_t idx)
    {
        FETCH_DBG("--------------START FETCHING------------------------");

        int32_t remaining_fetch = cfg.MaxFetchInstructions;
        int32_t available_fiq   = 0;

        DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx).available());
        FLEXUS_CHANNEL_ARRAY(AvailableFIQ, idx) >> available_fiq;

        if (available_fiq < remaining_fetch) { remaining_fetch = available_fiq; }

        theAvailableFetchSlots += remaining_fetch;

        if (theIcacheMiss[idx]) {
            ++theMissCycles;
            DBG_(VVerb, (<< "FETCH UNIT: l1I >> " << theMissCycles.theRefCount << " cycles missed so far"));
            goto fetch_end;
        }

        if (available_fiq <= 0) goto process_fiq;
        if (theFAQ[idx].size() <= 0) goto process_fiq;

        FETCH_DBG("starting to process the fetches..." << remaining_fetch);

        while (consume_fetch_slots(idx)) {
            theFetches++;
            theUsedFetchSlots++;
            remaining_fetch--;
        }

    process_fiq:
        process_available(available_fiq);
    fetch_end:
        FETCH_DBG("--------------FINISH FETCHING------------------------");
    }

    void sendMisses()
    {

        while (!theMissQueue.empty() && FLEXUS_CHANNEL(FetchMissOut).available()) {
            MemoryTransport trans = theMissQueue.front();

            if (!trans[MemoryMessageTag]->isEvictType()) {
                PhysicalMemoryAddress temp(trans[MemoryMessageTag]->address() & theBlockMask);
                if (theEvictSet.find(temp) != theEvictSet.end()) {
                    DBG_(Trace,
                         Comp(*this)(<< "Trying to fetch block while evict in "
                                        "process, stalling miss: "
                                     << *trans[MemoryMessageTag]));
                    break;
                }
            }

            DBG_(Trace,
                 Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                             << "] L1I Sending Miss on Port FMO: " << *trans[MemoryMessageTag]));
            ;

            trans[MemoryMessageTag]->coreIdx() = flexusIndex();
            FLEXUS_CHANNEL(FetchMissOut) << trans;
            theMissQueue.pop_front();
        }

        while (!theSnoopQueue.empty() && FLEXUS_CHANNEL(FetchSnoopOut).available()) {
            MemoryTransport trans = theSnoopQueue.front();
            DBG_(Trace,
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
                DBG_(Trace,
                     Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                                 << "] L1I Sending Reply on Port FRO: " << *trans[MemoryMessageTag]));
                ;
                trans[MemoryMessageTag]->coreIdx() = flexusIndex();
                FLEXUS_CHANNEL(FetchReplyOut) << trans;
                theReplyQueue.pop_front();
            }
        }
    }
    void initialize() override
    {

        theI.init(cfg.Size, cfg.Associativity, cfg.ICacheLineSize, statName());
        theIndexShift                 = LOG2(cfg.ICacheLineSize);
        theBlockMask                  = ~(cfg.ICacheLineSize - 1);
        theBundleCoreID               = flexusIndex();
        waitingForOpcodeQueue         = new FetchBundle();
        waitingForOpcodeQueue->coreID = theBundleCoreID;
        theFAQ.resize(cfg.Threads);
        theIcacheMiss.resize(cfg.Threads);
        theIcacheVMiss.resize(cfg.Threads);
        theFetchReplyTransactionTracker.resize(cfg.Threads);
        theLastMiss.resize(cfg.Threads);
        theIcachePrefetch.resize(cfg.Threads);
        theLastPrefetchVTagSet.resize(cfg.Threads);
    }
    void finalize() override {}
    void drive(interface::uFetchDrive const&) override
    {

        bool garbage = true;
        FLEXUS_CHANNEL(ClockTickSeen) << garbage;

        int32_t td = 0;

        if (cfg.Threads > 1) { td = nMTManager::MTManager::get()->scheduleFThread(flexusIndex()); }

        doFetch(td);
        sendMisses();
    }

    bool invalidate(PhysicalMemoryAddress const& anAddress) { return theI.inval(anAddress); }
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

        if (!is_li1_cache_hit(anAddress)) {
            ++theAllocations;
            return PhysicalMemoryAddress(theI.insert(anAddress));
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

    void fetch_reply(MemoryTransport& aTransport)
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

                issueEvict(replacement);

                // See if it is our outstanding miss or our outstanding prefetch
                for (uint32_t i = 0; i < cfg.Threads; ++i) {
                    if (theIcacheMiss[i] && *theIcacheMiss[i] == reply->address()) {
                        DBG_(Iface,
                             Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << i
                                         << "] L1I FILL " << reply->address()));
                        theIcacheMiss[i]                   = boost::none;
                        theIcacheVMiss[i]                  = boost::none;
                        theFetchReplyTransactionTracker[i] = tracker;
                        if (aTransport[TransactionTrackerTag] && aTransport[TransactionTrackerTag]->fillLevel()) {
                            theLastMiss[i] = std::make_pair(PhysicalMemoryAddress(reply->address() & theBlockMask),
                                                            *aTransport[TransactionTrackerTag]->fillLevel());
                        } else {
                            DBG_(VVerb, (<< "Received Fetch Reply with no TransactionTrackerTag"));
                        }
                    }

                    if (theIcachePrefetch[i] && *theIcachePrefetch[i] == reply->address()) {
                        DBG_(Iface,
                             Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << i
                                         << "] L1I PREFETCH-FILL " << reply->address()));
                        theIcachePrefetch[i] = boost::none;
                    }
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
                if (is_li1_cache_hit(reply->address())) {
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

    void loadState(std::string const& aDirName) override
    {
        // I need to load the instruction cache here.
        this->theI.loadState(aDirName + "/" + statName() + "-L1i.json");
    }

    void saveState(std::string const& aDirName) override
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
