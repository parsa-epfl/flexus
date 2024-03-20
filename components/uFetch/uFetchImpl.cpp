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

#include <fstream>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <boost/none.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/stats.hpp>

#include <components/MTManager/MTManager.hpp>
#include <components/uFetch/uFetch.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories uFetch
#define DBG_SetDefaultOps AddCat(uFetch)
#include DBG_Control()

#include <components/CommonQEMU/seq_map.hpp>
#include <core/qemu/mai_api.hpp>

#define LOG2(x)                                                                                           \
  ((x) == 1                                                                                               \
       ? 0                                                                                                \
       : ((x) == 2                                                                                        \
              ? 1                                                                                         \
              : ((x) == 4 ? 2                                                                             \
                          : ((x) == 8 ? 3                                                                 \
                                      : ((x) == 16                                                        \
                                             ? 4                                                          \
                                             : ((x) == 32                                                 \
                                                    ? 5                                                   \
                                                    : ((x) == 64 ? 6                                      \
                                                                 : ((x) == 128                            \
                                                                        ? 7                               \
                                                                        : ((x) == 256                     \
                                                                               ? 8                        \
                                                                               : ((x) == 512              \
                                                                                      ? 9                 \
                                                                                      : ((x) == 1024 ? 10 \
                                                                                                     : ((x) == 2048 ? 11 : ((x) == 4096 ? 12 : ((x) == 8192 ? 13 : ((x) == 16384 ? 14 : ((x) == 32768 ? 15 : ((x) == 65536 ? 16 : ((x) == 131072 ? 17 : ((x) == 262144 ? 18 : ((x) == 524288 ? 19 : ((x) == 1048576 ? 20 : ((x) == 2097152 ? 21 : ((x) == 4194304 ? 22 : ((x) == 8388608 ? 23 : ((x) == 16777216 ? 24 : ((x) == 33554432 ? 25 : ((x) == 67108864 ? 26 : -0xffff)))))))))))))))))))))))))))

#define TR_BIJ_DBG VVerb

namespace nuFetch {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

static const uint64_t kBadTag = 0xFFFFFFFFFFFFFFFFULL;

typedef flexus_boost_set_assoc<uint64_t, int> SimCacheArray;
typedef SimCacheArray::iterator SimCacheIter;
struct SimCache {
  SimCacheArray theCache;
  int32_t theCacheSize;
  int32_t theCacheAssoc;
  int32_t theCacheBlockShift;
  int32_t theBlockSize;
  std::string theName;

  void init(int32_t aCacheSize, int32_t aCacheAssoc, int32_t aBlockSize, const std::string &aName) {
    theCacheSize = aCacheSize;
    theCacheAssoc = aCacheAssoc;
    theBlockSize = aBlockSize;
    theCacheBlockShift = LOG2(theBlockSize);
    theCache.init(theCacheSize / theBlockSize, theCacheAssoc, 0);
    theName = aName;
  }

  void loadState(std::string const &aDirName) {
    std::string fname(aDirName);
    DBG_(VVerb, (<< "Loading state: " << fname << " for ufetch order L1i cache"));
    std::ifstream ifs(fname.c_str());
    if (!ifs.good()) {
      DBG_(VVerb,
           (<< " saved checkpoint state " << fname << " not found.  Resetting to empty cache. "));
    } else {
      ifs >> std::skipws;

      if (!loadArray(ifs)) {
        DBG_(VVerb, (<< "Error loading checkpoint state from file: " << fname
                     << ".  Make sure your checkpoints match your current "
                        "cache configuration."));
        DBG_Assert(false);
      }
      ifs.close();
    }
  }
  bool loadArray(std::istream &s) {
    static const int32_t kSave_ValidBit = 1;
    int32_t tagShift = LOG2(theCache.sets());

    char paren;
    int32_t dummy;
    int32_t load_state;
    uint64_t load_tag;
    for (uint32_t i = 0; i < theCache.sets(); i++) {
      s >> paren; // {
      if (paren != '{') {
        DBG_(Crit, (<< "Expected '{' when loading checkpoint"));
        return false;
      }
      for (uint32_t j = 0; j < theCache.assoc(); j++) {
        s >> paren >> load_state >> load_tag >> paren;
        DBG_(Trace, (<< theName << " Loading block " << std::hex
                     << (((load_tag << tagShift) | i) << theCacheBlockShift) << " with state "
                     << ((load_state & kSave_ValidBit) ? "Shared" : "Invalid") << " in way " << j));
        if (load_state & kSave_ValidBit) {
          theCache.insert(std::make_pair(((load_tag << tagShift) | i), 0));
          DBG_Assert(theCache.size() <= theCache.assoc());
        }
      }
      s >> paren; // }
      if (paren != '}') {
        DBG_(Crit, (<< "Expected '}' when loading checkpoint"));
        return false;
      }

      // useless associativity information
      s >> paren; // <
      if (paren != '<') {
        DBG_(Crit, (<< "Expected '<' when loading checkpoint"));
        return false;
      }
      for (uint32_t j = 0; j < theCache.assoc(); j++) {
        s >> dummy;
      }
      s >> paren; // >
      if (paren != '>') {
        DBG_(Crit, (<< "Expected '>' when loading checkpoint"));
        return false;
      }
    }
    return true;
  }

  uint64_t insert(uint64_t addr) {
    uint64_t ret_val = 0;
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.move_back(iter);
      return ret_val; // already present
    }
    if ((int)theCache.size() >= theCacheAssoc) {
      ret_val = theCache.front_key() << theCacheBlockShift;
      theCache.pop_front();
    }
    theCache.insert(std::make_pair(addr, 0));
    return ret_val;
  }

  bool lookup(uint64_t addr) {
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.move_back(iter);
      return true; // present
    }
    return false; // not present
  }

  bool inval(uint64_t addr) {
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.erase(iter);
      return true; // invalidated
    }
    return false; // not present
  }
};

class FLEXUS_COMPONENT(uFetch) {
  FLEXUS_COMPONENT_IMPL(uFetch);

  std::vector<std::list<FetchAddr>> theFAQ;
  pFetchBundle waitingForOpcodeQueue;

  // MARK: Meta-map which pairs each FetchedOpcode with a TranslationPtr.
  //       MANDATORY because two FetchedOpcs can have same PC but indep. TranslationPtrs
  //       Also, any MMU transaction can complete out of order (in general)
  typedef std::list<FetchedOpcode>::iterator opcodeQIterator;
  typedef std::unordered_map<TranslationPtr, opcodeQIterator, // K/V
                             TranslationPtrHasher,            // Custom hash object
                             TranslationPtrEqualityCheck      // Custom equality comparison
                             >
      BijectionMapType_t;
  BijectionMapType_t tr_op_bijection;

  typedef std::unordered_set<TranslationPtr,             // Key
                             TranslationPtrHasher,       // Custom hash
                             TranslationPtrEqualityCheck // Custom equality
                             >
      ExpectedTranslation_t;
  ExpectedTranslation_t translationsExpected;

  // This opcode is used to signal an MMU miss to the core, to force
  // a resync with Qemu
  static const int32_t kITLBMiss = 0UL; // illtrap

  // Statistics on the ICache
  Stat::StatCounter theFetchAccesses;
  Stat::StatCounter theFetches;
  Stat::StatCounter thePrefetches;
  Stat::StatCounter theFailedTranslations;
  Stat::StatCounter theMisses;
  Stat::StatCounter theHits;
  Stat::StatCounter theMissCycles;
  Stat::StatCounter theAllocations;
  Stat::StatMax theMaxOutstandingEvicts;

  // MARK: Statistics on slots available and used
  Stat::StatCounter theAvailableFetchSlots;
  Stat::StatCounter theUsedFetchSlots;

  // The I-cache
  SimCache theI;

  // Indicates whether a miss is outstanding, and what the physical address
  // of the miss is
  std::vector<boost::optional<PhysicalMemoryAddress>> theIcacheMiss;
  std::vector<boost::optional<VirtualMemoryAddress>> theIcacheVMiss;
  std::vector<boost::intrusive_ptr<TransactionTracker>> theFetchReplyTransactionTracker;
  std::vector<boost::optional<std::pair<PhysicalMemoryAddress, tFillLevel>>> theLastMiss;

  // Indicates whether a prefetch is outstanding, and what paddr was prefetched
  std::vector<boost::optional<PhysicalMemoryAddress>> theIcachePrefetch;
  // Indicates whether a prefetch is outstanding, and what paddr was prefetched
  std::vector<boost::optional<uint64_t>> theLastPrefetchVTagSet;

  // Set of outstanding evicts.
  std::set<uint64_t> theEvictSet;

  // Cache the last translation to avoid calling Qemu
  uint64_t theLastVTagSet;
  PhysicalMemoryAddress theLastPhysical;

  uint32_t theIndexShift;
  uint64_t theBlockMask;
  uint32_t theMissQueueSize;
  int32_t theBundleCoreID;

  std::list<MemoryTransport> theMissQueue;
  std::list<MemoryTransport> theSnoopQueue;
  std::list<MemoryTransport> theReplyQueue;

private:
  // I-Cache manipulation functions
  //=================================================================
  /*
      uint64_t index( PhysicalMemoryAddress const & anAddress ) {
        return ( static_cast<uint64_t>(anAddress) >> theIndexShift ) &
     theIndexMask;
      }

      uint64_t tag( PhysicalMemoryAddress const & anAddress ) {
        return ( static_cast<uint64_t>(anAddress) >> theTagShift );
      }
  */
  bool lookup(PhysicalMemoryAddress const &anAddress) {
    if (theI.lookup(anAddress)) {
      DBG_(VVerb, Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                              << "] I-Lookup hit: " << anAddress));
      DBG_(Verb, Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                             << "] I-Lookup hit: " << anAddress));
      return true;
    }
    DBG_(VVerb, Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                            << "] I-Lookup Miss addr: " << anAddress));
    DBG_(Verb, Comp(*this)(<< "Core[" << std::setfill('0') << std::setw(2) << flexusIndex()
                           << "] I-Lookup Miss addr: " << anAddress));
    return false;
  }

  PhysicalMemoryAddress insert(PhysicalMemoryAddress const &anAddress) {
    PhysicalMemoryAddress ret_val(0);
    if (!lookup(anAddress)) {
      ++theAllocations;
      ret_val = PhysicalMemoryAddress(theI.insert(anAddress));
    }
    return ret_val;
  }
  bool inval(PhysicalMemoryAddress const &anAddress) {
    return theI.inval(anAddress);
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(uFetch)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS), theFetchAccesses(statName() + "-FetchAccess"),
        theFetches(statName() + "-Fetches"), thePrefetches(statName() + "-Prefetches"),
        theFailedTranslations(statName() + "-FailedTranslations"),
        theMisses(statName() + "-Misses"), theHits(statName() + "-Hits"),
        theMissCycles(statName() + "-MissCycles"), theAllocations(statName() + "-Allocations"),
        theMaxOutstandingEvicts(statName() + "-MaxEvicts"),
        theAvailableFetchSlots(statName() + "-FetchSlotsPossible"),
        theUsedFetchSlots(statName() + "-FetchSlotsUsed"), theLastVTagSet(0), theLastPhysical(0) {
  }

  void initialize() {

    theI.init(cfg.Size, cfg.Associativity, cfg.ICacheLineSize, statName());
    theIndexShift = LOG2(cfg.ICacheLineSize);
    theBlockMask = ~(cfg.ICacheLineSize - 1);
    theBundleCoreID = flexusIndex();
    waitingForOpcodeQueue = new FetchBundle();
    waitingForOpcodeQueue->coreID = theBundleCoreID;
    theFAQ.resize(cfg.Threads);
    theIcacheMiss.resize(cfg.Threads);
    theIcacheVMiss.resize(cfg.Threads);
    theFetchReplyTransactionTracker.resize(cfg.Threads);
    theLastMiss.resize(cfg.Threads);
    theIcachePrefetch.resize(cfg.Threads);
    theLastPrefetchVTagSet.resize(cfg.Threads);

    theMissQueueSize = cfg.MissQueueSize;
  }

  void finalize() {
  }

  bool isQuiesced() const {
    if (!theMissQueue.empty() || !theSnoopQueue.empty() || !theReplyQueue.empty()) {
      return false;
    }
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      if (!theFAQ[i].empty()) {
        return false;
      }
    }
    return true;
  }

  void saveState(std::string const &aDirName) {
    std::string fname(aDirName);
    fname += "/" + boost::padded_string_cast<3, '0'>(flexusIndex()) + "-L1i";
    // Not supported
  }

  void loadState(std::string const &aDirName) {
    std::string fname(aDirName);
    if (flexusWidth() == 1) {
      fname += "/sys-L1i";
    } else {
      fname += "/" + boost::padded_string_cast<2, '0'>(flexusIndex()) + "-L1i";
    }
    DBG_(VVerb, (<< "file for sys-L1i " << fname));
    theI.loadState(fname);
  }

  //   Msutherl: TLB in-out functions
  FLEXUS_PORT_ALWAYS_AVAILABLE(iTranslationIn);
  void push(interface::iTranslationIn const &, TranslationPtr &retdTranslations) {
    ExpectedTranslation_t::iterator tr_iter = translationsExpected.find(retdTranslations);
    if (tr_iter != translationsExpected.end()) {
      updateTranslationResponse(retdTranslations);
      translationsExpected.erase(tr_iter);
    }
    DBG_(VVerb, (<< "Got response from iTranslationIn for PC " << retdTranslations->theVaddr));
  }

  // FetchAddressIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchAddressIn);
  void push(interface::FetchAddressIn const &, index_t anIndex,
            boost::intrusive_ptr<FetchCommand> &aCommand) {
    std::copy(aCommand->theFetches.begin(), aCommand->theFetches.end(),
              std::back_inserter(theFAQ[anIndex]));
  }

  // AvailableFAQOut
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(AvailableFAQOut);
  int32_t pull(interface::AvailableFAQOut const &, index_t anIndex) {
    return cfg.FAQSize - theFAQ[anIndex].size();
  }

  // SquashIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashIn);
  void push(interface::SquashIn const &, index_t anIndex, eSquashCause &aReason) {
    DBG_(Iface, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "."
                            << anIndex << "] Fetch SQUASH: " << aReason));
    theFAQ[anIndex].clear();
    theIcacheMiss[anIndex] = boost::none;
    theIcacheVMiss[anIndex] = boost::none;
    theFetchReplyTransactionTracker[anIndex] = nullptr;
    theLastMiss[anIndex] = boost::none;
    theIcachePrefetch[anIndex] = boost::none;
    theLastPrefetchVTagSet[anIndex] = 0;
    waitingForOpcodeQueue->clear();
    tr_op_bijection.clear();
    translationsExpected.clear();
  }

  // FetchMissIn
  FLEXUS_PORT_ALWAYS_AVAILABLE(FetchMissIn);
  void push(interface::FetchMissIn const &, MemoryTransport &aTransport) {
    DBG_(Trace, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                            << "] Fetch Miss Reply Received on Port FMI: "
                            << *aTransport[MemoryMessageTag]));
    fetchReply(aTransport);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &, index_t anIndex) {
    return theFAQ[anIndex].size();
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &, index_t anIndex) {
    int32_t available_fiq = 0;
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex).available());
    FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex) >> available_fiq;
    return theFAQ[anIndex].empty() || available_fiq == 0 || theIcacheMiss[anIndex];
  }

public:
  void drive(interface::uFetchDrive const &) {
    bool garbage = true;
    FLEXUS_CHANNEL(ClockTickSeen) << garbage;

    int32_t td = 0;
    if (cfg.Threads > 1) {
      td = nMTManager::MTManager::get()->scheduleFThread(flexusIndex());
    }
    doFetch(td);
    sendMisses();
  }

  Qemu::Processor cpu(index_t anIndex) {
    return Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + anIndex);
  }

private:
  void prefetchNext(index_t anIndex) {

    // Limit the number of prefetches.  With some backpressure, the number of
    // outstanding prefetches can otherwise be unbounded.
    if (theMissQueue.size() >= theMissQueueSize)
      return;

    if (cfg.PrefetchEnabled && theLastPrefetchVTagSet[anIndex]) {
      // Prefetch the line following theLastPrefetchVTagSet
      //(if it has a valid translation)

      ++(*theLastPrefetchVTagSet[anIndex]);
      VirtualMemoryAddress vprefetch =
          VirtualMemoryAddress(*theLastPrefetchVTagSet[anIndex] << theIndexShift);
      Flexus::SharedTypes::Translation xlat;
      xlat.theVaddr = vprefetch;
      xlat.theType = Flexus::SharedTypes::Translation::eFetch;
      xlat.thePaddr = cpu(anIndex).translateVirtualAddress(xlat.theVaddr);
      if (!xlat.thePaddr) {
        // Unable to translate for prefetch
        theLastPrefetchVTagSet[anIndex] = boost::none;
        return;
      } else {
        if (lookup(xlat.thePaddr)) {
          // No need to prefetch, already in cache
          theLastPrefetchVTagSet[anIndex] = boost::none;
        } else {
          theIcachePrefetch[anIndex] = xlat.thePaddr;
          DBG_(Iface,
               Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "."
                           << anIndex << "] L1I PREFETCH " << *theIcachePrefetch[anIndex]));
          issueFetch(xlat.thePaddr, vprefetch);
          ++thePrefetches;
        }
      }
    }
  }

  bool icacheLookup(index_t anIndex, VirtualMemoryAddress vaddr) {
    // Translate virtual address to physical.
    // First, see if it is our cached translation

    DBG_(VVerb, (<< "Looking up instruction in cache")); // NOOSHIN
    PhysicalMemoryAddress paddr;
    uint64_t tagset = vaddr >> theIndexShift;
    if (tagset == theLastVTagSet) {
      paddr = theLastPhysical;
      if (paddr == 0) {
        DBG_(VVerb, (<< "Last Physical translation lookup failed!"));
        ++theFailedTranslations;
        return true; // Failed translations are treated as hits - they will
                     // cause an MMU miss in the pipe.
      }
    } else {
      DBG_(VVerb, (<< "Not in Flexus cache...Will look into Qemu now!"));
      Flexus::SharedTypes::Translation xlat;
      xlat.theVaddr = vaddr;
      xlat.theType = Flexus::SharedTypes::Translation::eFetch;
      xlat.thePaddr = cpu(anIndex).translateVirtualAddress(xlat.theVaddr);
      paddr = xlat.thePaddr;
      if (paddr == 0) {
        assert(false);
        DBG_(VVerb, (<< "Translation failed!"));
        ++theFailedTranslations;
        return true; // Failed translations are treated as hits - they will
                     // cause an MMU miss in the pipe.
      } else {
        DBG_(VVerb, (<< "Translation success!"));
      }
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
    ++theMisses;

    // It is an error to get a second miss while one is outstanding
    DBG_Assert(!theIcacheMiss[anIndex]);
    // Record the miss address, so we know when it comes back
    PhysicalMemoryAddress temp(paddr & theBlockMask);
    theIcacheMiss[anIndex] = temp;
    theIcacheVMiss[anIndex] = vaddr;
    theFetchReplyTransactionTracker[anIndex] = nullptr;

    DBG_(VVerb,
         Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "."
                     << anIndex << "] L1I MISS " << vaddr << " " << *theIcacheMiss[anIndex]));

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
    DBG_(VVerb, (<< "in icacheLookup before return false")); // NOOSHIN
    return false;
  }

  void sendMisses() {
    while (!theMissQueue.empty() && FLEXUS_CHANNEL(FetchMissOut).available()) {
      MemoryTransport trans = theMissQueue.front();

      if (!trans[MemoryMessageTag]->isEvictType()) {
        PhysicalMemoryAddress temp(trans[MemoryMessageTag]->address() & theBlockMask);
        if (theEvictSet.find(temp) != theEvictSet.end()) {
          DBG_(Trace, Comp(*this)(<< "Trying to fetch block while evict in "
                                     "process, stalling miss: "
                                  << *trans[MemoryMessageTag]));
          break;
        }
      }

      DBG_(Trace, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                              << "] L1I Sending Miss on Port FMO: " << *trans[MemoryMessageTag]));
      ;

      trans[MemoryMessageTag]->coreIdx() = flexusIndex();
      FLEXUS_CHANNEL(FetchMissOut) << trans;
      theMissQueue.pop_front();
    }

    while (!theSnoopQueue.empty() && FLEXUS_CHANNEL(FetchSnoopOut).available()) {
      MemoryTransport trans = theSnoopQueue.front();
      DBG_(Trace, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
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

  void queueSnoopMessage(MemoryTransport &aTransport, MemoryMessage::MemoryMessageType aType,
                         PhysicalMemoryAddress &anAddress, bool aContainsData = false) {
    boost::intrusive_ptr<MemoryMessage> msg = new MemoryMessage(aType, anAddress);
    msg->reqSize() = cfg.ICacheLineSize;
    if (aType == MemoryMessage::FetchAck) {
      msg->ackRequiresData() = aContainsData;
    }
    aTransport.set(MemoryMessageTag, msg);
    if (cfg.UseReplyChannel) {
      theReplyQueue.push_back(aTransport);
    } else {
      theSnoopQueue.push_back(aTransport);
    }
  }

  void fetchReply(MemoryTransport &aTransport) {
    boost::intrusive_ptr<MemoryMessage> reply = aTransport[MemoryMessageTag];
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
      PhysicalMemoryAddress replacement = insert(reply->address());

      issueEvict(replacement);

      // See if it is our outstanding miss or our outstanding prefetch
      for (uint32_t i = 0; i < cfg.Threads; ++i) {
        if (theIcacheMiss[i] && *theIcacheMiss[i] == reply->address()) {
          DBG_(Iface, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                                  << "." << i << "] L1I FILL " << reply->address()));
          theIcacheMiss[i] = boost::none;
          theIcacheVMiss[i] = boost::none;
          theFetchReplyTransactionTracker[i] = tracker;
          if (aTransport[TransactionTrackerTag] && aTransport[TransactionTrackerTag]->fillLevel()) {
            theLastMiss[i] = std::make_pair(PhysicalMemoryAddress(reply->address() & theBlockMask),
                                            *aTransport[TransactionTrackerTag]->fillLevel());
          } else {
            DBG_(VVerb, (<< "Received Fetch Reply with no TransactionTrackerTag"));
          }
        }

        if (theIcachePrefetch[i] && *theIcachePrefetch[i] == reply->address()) {
          DBG_(Iface, Comp(*this)(<< "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex()
                                  << "." << i << "] L1I PREFETCH-FILL " << reply->address()));
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
        bool contains_data = reply->ackRequiresData();
        if (reply->type() == MemoryMessage::FwdReplyOwned) {
          contains_data = true;
          ack_type = MemoryMessage::FetchAckDirty;
        }
        queueSnoopMessage(aTransport, ack_type, reply->address(), contains_data);
      }
      break;
    }

    case MemoryMessage::Invalidate:

      inval(reply->address());
      if (aTransport[DestinationTag]) {
        aTransport[DestinationTag]->type = DestinationMessage::Requester;
      }
      queueSnoopMessage(aTransport, MemoryMessage::InvalidateAck, reply->address());
      break;

    case MemoryMessage::ReturnReq:
      queueSnoopMessage(aTransport, MemoryMessage::ReturnReply, reply->address());
      break;

    case MemoryMessage::Downgrade:
      // We can always reply to this message, regardless of the hit status
      queueSnoopMessage(aTransport, MemoryMessage::DowngradeAck, reply->address());
      break;

    case MemoryMessage::ReadFwd:
    case MemoryMessage::FetchFwd:
      // If we have the data, send a FwdReply
      // Otherwise send a FwdNAck
      DBG_Assert(aTransport[DestinationTag]);
      aTransport[TransactionTrackerTag]->setFillLevel(Flexus::SharedTypes::ePeerL1Cache);
      aTransport[TransactionTrackerTag]->setPreviousState(eShared);
      if (lookup(reply->address())) {
        aTransport[DestinationTag]->type = DestinationMessage::Requester;
        queueSnoopMessage(aTransport, MemoryMessage::FwdReply, reply->address());
      } else {
        std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
        if (iter != theEvictSet.end()) {
          aTransport[DestinationTag]->type = DestinationMessage::Requester;
          queueSnoopMessage(aTransport, MemoryMessage::FwdReply, reply->address());
        } else {
          aTransport[DestinationTag]->type = DestinationMessage::Directory;
          queueSnoopMessage(aTransport, MemoryMessage::FwdNAck, reply->address());
        }
      }
      break;

    case MemoryMessage::WriteFwd:
      // similar, but invalidate data
      DBG_Assert(aTransport[DestinationTag]);
      aTransport[TransactionTrackerTag]->setFillLevel(Flexus::SharedTypes::ePeerL1Cache);
      aTransport[TransactionTrackerTag]->setPreviousState(eShared);
      if (inval(reply->address())) {
        aTransport[DestinationTag]->type = DestinationMessage::Requester;
        queueSnoopMessage(aTransport, MemoryMessage::FwdReplyWritable, reply->address());
      } else {
        std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
        if (iter != theEvictSet.end()) {
          aTransport[DestinationTag]->type = DestinationMessage::Requester;
          queueSnoopMessage(aTransport, MemoryMessage::FwdReplyWritable, reply->address());
        } else {
          aTransport[DestinationTag]->type = DestinationMessage::Directory;
          queueSnoopMessage(aTransport, MemoryMessage::FwdNAck, reply->address());
        }
      }
      break;

    case MemoryMessage::BackInvalidate:
      // Same as invalidate
      aTransport[DestinationTag]->type = DestinationMessage::Directory;

      // small protocol change - always send the InvalAck, whether we have the
      // block or not
      inval(reply->address());
      queueSnoopMessage(aTransport, MemoryMessage::InvalidateAck, reply->address());
      break;

    case MemoryMessage::EvictAck: {
      std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
      DBG_Assert(iter != theEvictSet.end(),
                 Comp(*this)(<< "Block not found in EvictBuffer upon receipt of Ack: " << *reply));
      theEvictSet.erase(iter);
      break;
    }
    default:
      DBG_Assert(false, Comp(*this)(<< "FETCH UNIT: Unhandled message received: " << *reply));
    }
  }

  void issueEvict(PhysicalMemoryAddress anAddress) {
    if (cfg.CleanEvict && anAddress != 0) {
      MemoryTransport transport;
      boost::intrusive_ptr<MemoryMessage> operation(
          new MemoryMessage(MemoryMessage::EvictClean, anAddress));
      operation->reqSize() = 64;

      // Put in evict buffer and wait for Ack
      std::pair<std::set<uint64_t>::iterator, bool> ret = theEvictSet.insert(anAddress);
      DBG_Assert(ret.second);
      theMaxOutstandingEvicts << theEvictSet.size();
      operation->ackRequired() = true;

      boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
      tracker->setAddress(anAddress);
      tracker->setInitiator(flexusIndex());
      transport.set(TransactionTrackerTag, tracker);
      transport.set(MemoryMessageTag, operation);

      if (cfg.EvictOnSnoop) {
        theSnoopQueue.push_back(transport);
      } else {
        theMissQueue.push_back(transport);
      }
    }
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

  // Implementation of the FetchDrive drive interface
  void doFetch(index_t anIndex) {

    FETCH_DBG("--------------START FETCHING------------------------");

    // Determine available FIQ this cycle
    int32_t available_fiq = 0;
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex).available());
    FLEXUS_CHANNEL_ARRAY(AvailableFIQ, anIndex) >> available_fiq;
    int32_t remaining_fetch = cfg.MaxFetchInstructions;
    if (available_fiq < remaining_fetch) {
      remaining_fetch = available_fiq;
    }
    theAvailableFetchSlots += remaining_fetch;

    if (theIcacheMiss[anIndex]) {
      ++theMissCycles;
      DBG_(VVerb, (<< "FETCH UNIT: in theIcacheMiss" << theMissCycles.theRefCount
                   << "cycles missed so far"));
      return;
    }

    if (available_fiq > 0 && (theFAQ[anIndex].size() > 0 || theFlexus->quiescing())) {
      std::set<VirtualMemoryAddress> available_lines;
      FETCH_DBG("starting to process the fetches..." << remaining_fetch);

      while (remaining_fetch > 0 && (theFAQ[anIndex].size() > 0 || theFlexus->quiescing())) {
        FetchAddr fetch_addr = theFAQ[anIndex].front();
        VirtualMemoryAddress block_addr(fetch_addr.theAddress & theBlockMask);

        if (available_lines.count(block_addr) == 0) {
          // Line needs to be fetched from I-cache
          if (available_lines.size() >= cfg.MaxFetchLines) {
            // Reached limit of I-cache reads per cycle
            break;
          }

          // Notify the PowerTracker of Icache access
          bool garbage = true;
          FLEXUS_CHANNEL(InstructionFetchSeen) << garbage;

          if (!cfg.PerfectICache) {
            // Do I-cache access here.
            if (!icacheLookup(anIndex, block_addr)) {
              break;
            }
          } else {
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
        theFAQ[anIndex].pop_front();
        waitingForOpcodeQueue->theOpcodes.emplace_back(
            FetchedOpcode(fetch_addr.theAddress,
                          0, // op_code not resolved yet - waiting for translation
                          fetch_addr.theBPState, theFetchReplyTransactionTracker[anIndex]));
        waitingForOpcodeQueue->theFillLevels.emplace_back(eL1I);
        DBG_(TR_BIJ_DBG,
             Comp(*this)(<< "added entry in waiting for opcode queue" << fetch_addr.theAddress));
        sendTranslationReq(anIndex, fetch_addr.theAddress,
                           std::prev(waitingForOpcodeQueue->theOpcodes.end()));

        ++theFetches;
        --remaining_fetch;
        theUsedFetchSlots++;
      }
    }
    processBundle(available_fiq);
    FETCH_DBG("--------------FINISH FETCHING------------------------");
  }

  void processBundle(uint32_t available_fiq) {

    if (waitingForOpcodeQueue->theOpcodes.size() == 0)
      return;
    if (waitingForOpcodeQueue->theOpcodes.begin()->theOpcode == 0)
      return;

    pFetchBundle bundle(new FetchBundle);
    bundle->coreID = theBundleCoreID;

    // Only pop fetched instructions up to the limit of decoder FIQ
    uint32_t insns_added_to_fiq = 0;
    while (waitingForOpcodeQueue->theOpcodes.size() > 0 && (insns_added_to_fiq < available_fiq)) {
      auto i = waitingForOpcodeQueue->theOpcodes.begin();
      auto fill_iter = waitingForOpcodeQueue->theFillLevels.begin();
      if (i->theOpcode != 0) {
        bundle->theOpcodes.emplace_back(std::move(*i));
        bundle->theFillLevels.emplace_back(std::move(*fill_iter));
        DBG_(TR_BIJ_DBG,
             Comp(*this)(<< "popping entry out of the waitingForOpcodeQueue " << i->thePC));
        waitingForOpcodeQueue->theOpcodes.erase(i);
        waitingForOpcodeQueue->theFillLevels.erase(fill_iter);
        insns_added_to_fiq++;
      } else {
        break;
      }
    }

    if (bundle->theOpcodes.size() > 0) {
      FLEXUS_CHANNEL_ARRAY(FetchBundleOut, 0) << bundle;
    }
  }

  void sendTranslationReq(index_t anIndex, VirtualMemoryAddress const &anAddress,
                          opcodeQIterator newOpcIterator) {
    TranslationPtr xlat(new Flexus::SharedTypes::Translation());
    xlat->theVaddr = anAddress;
    xlat->theType = Flexus::SharedTypes::Translation::eFetch;
    xlat->theException = 0; // just for now
    xlat->theIndex = anIndex;
    xlat->setInstr();

    // Insert an entry into the translation<->opcode map and outstanding translations expected
    std::pair<BijectionMapType_t::iterator, bool> bijection_insertion =
        tr_op_bijection.insert(std::make_pair(xlat, newOpcIterator));
    DBG_(TR_BIJ_DBG, Comp(*this)(<< "Inserting xlat objection with vaddr " << xlat->theVaddr
                                 << " pointing to PC " << newOpcIterator->thePC
                                 << " into waitingForOpcodesQueue bijection "));
    DBG_AssertSev(
        Iface, bijection_insertion.second == true,
        (<< "Inserting xlat object with vaddr " << xlat->theVaddr
         << " failed!! Clashing object has vaddr: " << bijection_insertion.first->first->theVaddr));

    translationsExpected.emplace(xlat);

    DBG_(TR_BIJ_DBG, Comp(*this)(<< "Adding translation request entry for " << xlat->theVaddr));

    DBG_Assert(FLEXUS_CHANNEL(iTranslationOut).available());
    FLEXUS_CHANNEL(iTranslationOut) << xlat;
  }

  bool available(interface::ResyncIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::ResyncIn const &, index_t anIndex, int &aResync) {
    waitingForOpcodeQueue->clear();
    tr_op_bijection.clear();
    translationsExpected.clear();
    theBundleCoreID =
        aResync; // FIXME: Is this right?? Should be equal to flexusIndex() or anIndex?
    waitingForOpcodeQueue->coreID = theBundleCoreID;
  }

  void updateTranslationResponse(TranslationPtr &tr) {
    DBG_Assert(tr->isDone() || tr->isHit());
    DBG_(TR_BIJ_DBG, Comp(*this)(<< "Updating translation response for " << tr->theVaddr
                                 << " @ cpu index " << flexusIndex()));
    PhysicalMemoryAddress magicTranslation =
        cpu(tr->theIndex).translateVirtualAddress(tr->theVaddr);

    if (tr->thePaddr == magicTranslation || tr->isPagefault()) {
      DBG_(VVerb, Comp(*this)(<< "Magic QEMU translation == MMU Translation. Vaddr = " << std::hex
                              << tr->theVaddr << std::dec << ", Paddr = " << std::hex
                              << tr->thePaddr << std::dec));
    } else {
      DBG_Assert(false, Comp(*this)(<< "ERROR: Magic QEMU translation NOT EQUAL TO MMU "
                                       "Translation. Vaddr = "
                                    << std::hex << tr->theVaddr << std::dec << ", PADDR_MMU = "
                                    << std::hex << tr->thePaddr << std::dec << ", PADDR_QEMU = "
                                    << std::hex << magicTranslation << std::dec));
    }
    uint32_t opcode = 1;
    // MARK: Look up in the opc bijection and get the correct index
    BijectionMapType_t::iterator bijection_iter = tr_op_bijection.find(tr);
    DBG_AssertSev(Crit, bijection_iter != tr_op_bijection.end(),
                  Comp(*this)(<< "ERROR: Opcode index was NOT found for translationPtr with ID"
                              << tr->theID << " and address" << tr->theVaddr));
    if (!tr->isPagefault()) {
      opcode = cpu(tr->theIndex).fetchInstruction(tr->theVaddr);
      opcode += opcode ? 0 : 1;
    }
    waitingForOpcodeQueue->updateOpcode(tr->theVaddr, bijection_iter->second, opcode);
    // Remove this mapping, opcode is updated
    tr_op_bijection.erase(bijection_iter);
  }
};

} // End namespace nuFetch

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
