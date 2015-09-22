#include "TraceTracker.hpp"
#include <components/TraceTrackerQEMU/SharingTracker.hpp>  /* CMU-ONLY */
#include <components/TraceTrackerQEMU/OffChipTracker.hpp>  /* CMU-ONLY */
#include <components/SpatialPrefetcher/SpatialPrefetcherComp.hpp>  /* CMU-ONLY */
#include <components/SpatialPrefetcher/ghb.hpp>  /* CMU-ONLY */
#include <components/SpatialPrefetcher/seq_map.hpp> /* CMU-ONLY */
#include <core/boost_extensions/padded_string_cast.hpp>

#define DBG_DefineCategories TraceTrack
#define DBG_SetDefaultOps AddCat(TraceTrack)
#include DBG_Control()

using namespace Flexus;
using namespace Core;

#define MyLevel Iface

#define LOG2(x)         \
  ((x)==1 ? 0 :         \
  ((x)==2 ? 1 :         \
  ((x)==4 ? 2 :         \
  ((x)==8 ? 3 :         \
  ((x)==16 ? 4 :        \
  ((x)==32 ? 5 :        \
  ((x)==64 ? 6 :        \
  ((x)==128 ? 7 :       \
  ((x)==256 ? 8 :       \
  ((x)==512 ? 9 :       \
  ((x)==1024 ? 10 :     \
  ((x)==2048 ? 11 :     \
  ((x)==4096 ? 12 :     \
  ((x)==8192 ? 13 :     \
  ((x)==16384 ? 14 :    \
  ((x)==32768 ? 15 :    \
  ((x)==65536 ? 16 :    \
  ((x)==131072 ? 17 :   \
  ((x)==262144 ? 18 :   \
  ((x)==524288 ? 19 :   \
  ((x)==1048576 ? 20 :  \
  ((x)==2097152 ? 21 :  \
  ((x)==4194304 ? 22 :  \
  ((x)==8388608 ? 23 :  \
  ((x)==16777216 ? 24 : \
  ((x)==33554432 ? 25 : \
  ((x)==67108864 ? 26 : -0xffff)))))))))))))))))))))))))))

#define MIN(a,b) ((a) < (b) ? (a) : (b))

namespace nTraceTracker {

/* CMU-ONLY-BLOCK-BEGIN */
struct NodeEntry {
  SharedTypes::tFillLevel level;
  nSpatialPrefetcher::SpatialPrefetcher sgp;

  NodeEntry(SharedTypes::tFillLevel aLevel, std::string aName, std::string aLegacyName, int32_t aNode)
    : level(aLevel)
    , sgp(aName, aLegacyName, aNode)
  {}
};
struct GhbEntry {
  SharedTypes::tFillLevel level;
  nGHBPrefetcher::GHBPrefetcher ghb;

  GhbEntry(SharedTypes::tFillLevel aLevel, std::string aName, int32_t aNode)
    : level(aLevel)
    , ghb(aName, aNode)
  {}
};
typedef flexus_boost_set_assoc<address_t, int> SimCache;
typedef SimCache::iterator SimCacheIter;
struct SimCacheEntry {
  Stat::StatCounter statAccesses;
  Stat::StatCounter statMisses;
  Stat::StatCounter statReads;
  Stat::StatCounter statReadMisses;
  Stat::StatCounter statInvals;
  SimCache theCache;

  SimCacheEntry(std::string aStatName)
    : statAccesses(aStatName + "-Accesses")
    , statMisses(aStatName + "-Misses")
    , statReads(aStatName + "-Reads")
    , statReadMisses(aStatName + "-ReadMisses")
    , statInvals(aStatName + "-Invals")
  {}
  void loadState(int32_t aNode, std::string const & aDirName) {
    std::string fname(aDirName);
    std::string cname(boost::padded_string_cast < 2, '0' > (aNode) + "-L1d");
    fname += "/" + cname;
    DBG_(Dev, ( << "Loading state: " << cname << " for duplicate in-order L1d cache" ) );
    std::ifstream ifs(fname.c_str());
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      ifs >> std::skipws;

      if ( ! loadArray( ifs ) ) {
        DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                      ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
      ifs.close();
    }
  }
  bool loadArray( std::istream & s ) {
    static const int32_t kSave_ValidBit = 1;
    int32_t tagShift = LOG2(theCache.sets());

    char paren;
    int32_t dummy;
    int32_t load_state;
    uint64_t load_tag;
    for ( uint32_t i = 0; i < theCache.sets() ; i++ ) {
      s >> paren; // {
      if ( paren != '{' ) {
        DBG_ (Crit, ( << "Expected '{' when loading checkpoint" ) );
        return false;
      }
      for ( uint32_t j = 0; j < theCache.assoc(); j++ ) {
        s >> paren >> load_state >> load_tag >> paren;
        if (load_state & kSave_ValidBit) {
          theCache.insert( std::make_pair((load_tag << tagShift) | i, 0) );
          DBG_Assert(theCache.size() <= theCache.assoc());
        }
      }
      s >> paren; // }
      if ( paren != '}' ) {
        DBG_ (Crit, ( << "Expected '}' when loading checkpoint" ) );
        return false;
      }

      // useless associativity information
      s >> paren; // <
      if ( paren != '<' ) {
        DBG_ (Crit, ( << "Expected '<' when loading checkpoint" ) );
        return false;
      }
      for ( uint32_t j = 0; j < theCache.assoc(); j++ ) {
        s >> dummy;
      }
      s >> paren; // >
      if ( paren != '>' ) {
        DBG_ (Crit, ( << "Expected '>' when loading checkpoint" ) );
        return false;
      }
    }
    return true;
  }
};
/* CMU-ONLY-BLOCK-END */

void TraceTracker::access(int32_t aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc,
                          bool prefetched, bool write, bool miss, bool priv, uint64_t ltime) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] access 0x" << std::hex << addr));
  //DBG_(Dev, (<< "[" << aNode << ":" << cache << "] access 0x" << std::hex << addr << " (ts:" << ltime <<")"));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      if (cache == SharedTypes::eCore) {
        miss = simCacheAccess(aNode, addr, write);
      }
      theSGPs[aNode][cache]->sgp.access(addr, pc, prefetched, write, miss, priv);
      // ltime is always 0 for eCore accesses
      if (ltime > 0 && miss) {
        //if (ltime > 0) {
        commit(aNode, cache, addr, pc, ltime);
      }
    }
  }
  if (theGhbEnabled) {
    if (theGhbLevels.count(cache) > 0) {
      if (miss) {
        theGHBs[aNode][cache]->ghb.access(pc, addr);
      }
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::commit(int32_t aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc, uint64_t aLogicalTime) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] commit 0x" << std::hex << addr));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      theSGPs[aNode][cache]->sgp.commit(addr, pc, aLogicalTime);
    }
  }
  if (thePrefetchTracking) {
    address_t block = addr & ~(theBlockSize - 1);
    bool offchip = thePrefetchTrackers[aNode]->commit(block);
    if (theSgpTracking && offchip) {
      theSgpTrackers[aNode]->offchipMiss(block, false);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::store(int32_t aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc,
                         bool miss, bool priv, uint64_t aLogicalTime) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] store 0x" << std::hex << addr));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      if (cache == SharedTypes::eCore) {
        miss = simCacheAccess(aNode, addr, true);
      }
      theSGPs[aNode][cache]->sgp.store(addr, pc, miss, priv, aLogicalTime);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::prefetch(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] prefetch 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      theSGPs[aNode][cache]->sgp.access(block, 0, true, false, true, false);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

/* CMU-ONLY-BLOCK-BEGIN */
void TraceTracker::parallelList(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, std::set<uint64_t> & aParallelList) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] parallel list 0x" << std::hex << block));
  if (theSgpTracking) {
    DBG_Assert(cache == SharedTypes::eCore);
    theSgpTrackers[aNode]->parallelList(block, aParallelList);
  }
}
/* CMU-ONLY-BLOCK-END */

void TraceTracker::fill(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, SharedTypes::tFillLevel fillLevel, bool isFetch, bool isWrite) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] fill 0x" << std::hex << block));
  DBG_Assert(fillLevel != SharedTypes::eUnknown);
  /* CMU-ONLY-BLOCK-BEGIN */
  bool offChip = (fillLevel == SharedTypes::eLocalMem || fillLevel == SharedTypes::eRemoteMem);
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      theSGPs[aNode][cache]->sgp.fill(block, offChip);
    }
  }
  if (thePrefetchTracking && !isFetch) {
    thePrefetchTrackers[aNode]->fill(block, cache, isWrite);
  }
  if (theSharingTracker) {
    theSharingTracker->fill(aNode, cache, block);
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::prefetchFill(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, SharedTypes::tFillLevel fillLevel) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] prefetch fill 0x" << std::hex << block));
  DBG_Assert(fillLevel != SharedTypes::eUnknown);
  /* CMU-ONLY-BLOCK-BEGIN */
  bool offChip = (fillLevel == SharedTypes::eLocalMem || fillLevel == SharedTypes::eRemoteMem);
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      theSGPs[aNode][cache]->sgp.fill(block, offChip);
    }
  }
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->prefetchFill(block, cache);
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::prefetchHit(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool isWrite) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] prefetch hit 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->hit(block, isWrite);
    if (theSgpTracking) {
      theSgpTrackers[aNode]->sgpHit(block, isWrite);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::prefetchRedundant(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] prefetch redundant 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->redundant(block);
  }
  /* CMU-ONLY-BLOCK-END */
}

/* CMU-ONLY-BLOCK-BEGIN */
void TraceTracker::beginSpatialGen(int32_t aNode, address_t block) {
  SharedTypes::tFillLevel cache = theCurrLevels[aNode];
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] begin gen 0x" << std::hex << block));
}

void TraceTracker::endSpatialGen(int32_t aNode, address_t block) {
  SharedTypes::tFillLevel cache = theCurrLevels[aNode];
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] end gen 0x" << std::hex << block));
  if (theSgpTracking) {
    theSgpTrackers[aNode]->endGen(block);
  }
}

void TraceTracker::sgpPredict(int32_t aNode, address_t group, void * aPredictSet) {
  SharedTypes::tFillLevel cache = theCurrLevels[aNode];
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] sgp predict 0x" << std::hex << group));
  if (theSgpTracking) {
    theSgpTrackers[aNode]->sgpPredict(group, aPredictSet);
  }
}
/* CMU-ONLY-BLOCK-END */

void TraceTracker::insert(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] insert 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      theSGPs[aNode][cache]->sgp.insert(block);
    }
  }
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->insert(block, cache);
  }
  if (theSharingTracker) {
    theSharingTracker->insert(aNode, cache, block);
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::eviction(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, bool drop) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] evict 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      theSGPs[aNode][cache]->sgp.evict(block);
    }
  }
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->evict(block, cache);
  }
  if (theSharingTracker) {
    theSharingTracker->evict(aNode, cache, block, drop);
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::invalidation(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] invalidate 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSgpEnabled) {
    if (theSgpLevels.count(cache) > 0) {
      theCurrLevels[aNode] = cache;
      bool actual = true;
      if (cache == SharedTypes::eCore) {
        actual = simCacheInval(aNode, block);
      }
      if (actual) {
        theSGPs[aNode][cache]->sgp.invalidate(block);
      }
    }
  }
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->inval(block, cache);
  }
  if (theSharingTracker) {
    theSharingTracker->invalidate(aNode, cache, block);
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::invalidAck(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] invAck 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSharingTracker) {
    if (cache == theBaseSharingLevel) {
      theSharingTracker->invalidAck(aNode, block);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::invalidTagCreate(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] invTagCreate 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSharingTracker) {
    if (cache == theBaseSharingLevel) {
      theSharingTracker->invTagCreate(aNode, block);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::invalidTagRefill(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] invTagRefill 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSharingTracker) {
    if (cache == theBaseSharingLevel) {
      theSharingTracker->invTagRefill(aNode, block);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::invalidTagReplace(int32_t aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] invTagReplace 0x" << std::hex << block));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSharingTracker) {
    if (cache == theBaseSharingLevel) {
      theSharingTracker->invTagReplace(aNode, block);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::accessLoad(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] accessLoad 0x" << std::hex << block << "," << offset));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (theSharingTracker) {
    if (cache == SharedTypes::eL1) {
      theSharingTracker->accessLoad(aNode, block, offset, size);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::accessStore(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] accessStore 0x" << std::hex << block << "," << offset));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (thePrefetchTracking) {
    bool offchip = thePrefetchTrackers[aNode]->store(block);
    if (theSgpTracking && offchip) {
      theSgpTrackers[aNode]->offchipMiss(block, true);
    }
  }
  if (theSharingTracker) {
    if (cache == SharedTypes::eL1) {
      theSharingTracker->accessStore(aNode, block, offset, size);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

void TraceTracker::accessFetch(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] accessLoad 0x" << std::hex << block << "," << offset));
}

void TraceTracker::accessAtomic(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size) {
  DBG_(MyLevel, ( << "[" << aNode << ":" << cache << "] accessAtomic 0x" << std::hex << block << "," << offset));
  /* CMU-ONLY-BLOCK-BEGIN */
  if (thePrefetchTracking) {
    thePrefetchTrackers[aNode]->store(block);
  }
  if (theSharingTracker) {
    if (cache == SharedTypes::eL1) {
      theSharingTracker->accessAtomic(aNode, block, offset, size);
    }
  }
  /* CMU-ONLY-BLOCK-END */
}

TraceTracker::TraceTracker()
  : theSharingTracker(0)  /* CMU-ONLY */
  , theSgpEnabled(false)  /* CMU-ONLY */
  , thePrefetchTracking(false)  /* CMU-ONLY */
  , theSgpTracking(false)  /* CMU-ONLY */
{}

void TraceTracker::initialize() {
  DBG_(Iface, ( << "initializing TraceTracker"));
  Flexus::Stat::getStatManager()->addFinalizer( [this](){return this->finalize();});
}

void TraceTracker::finalize() {
  DBG_(Iface, ( << "finalizing TraceTracker"));
  /* CMU-ONLY-BLOCK-BEGIN */
  unsigned ii, jj;
  for (ii = 0; ii < theSGPs.size(); ii++) {
    for (jj = 0 ; jj < theSGPs[ii].size(); ++jj) {
      if (theSGPs[ii][jj]) {
        theSGPs[ii][jj]->sgp.finalize();
      }
    }
  }
  if (theSharingTracker) {
    theSharingTracker->finalize();
  }
  /* CMU-ONLY-BLOCK-END */
}

/* CMU-ONLY-BLOCK-BEGIN */
void TraceTracker::initOffChipTracking(int32_t aNode) {
  enablePrefetchTracking(aNode);
}

void TraceTracker::initSGP(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel,
                           bool enableUsage, bool enableRepet, bool enableBufFetch,
                           bool enableTimeRepet, bool enablePrefetch, bool enableActive,
                           bool enableOrdering, bool enableStreaming,
                           int32_t blockSize, int32_t sgpBlocks, int32_t repetType, bool repetFills,
                           bool sparseOpt, int32_t phtSize, int32_t phtAssoc, int32_t pcBits,
                           int32_t cptType, int32_t cptSize, int32_t cptAssoc, bool cptSparse,
                           bool fetchDist, int32_t streamWindow, bool streamDense, bool sendStreams,
                           int32_t bufSize, int32_t streamDescs, bool delayedCommits, int32_t cptFilterSize) {
  theSgpLevels.insert(aLevel);
  if ((int)theSGPs.size() <= aNode) {
    theSGPs.resize(aNode + 1);
    theCurrLevels.resize(aNode + 1);
  }
  if (theSGPs[aNode].size() < static_cast<unsigned>(aLevel)) {
    theSGPs[aNode].resize(aLevel + 1, 0);
  }
  theSGPs[aNode][aLevel] = new NodeEntry(aLevel, std::string(boost::padded_string_cast < 2, '0' > (aNode) + "-sgpL" + boost::padded_string_cast < 1, '0' > (aLevel)), std::string(boost::padded_string_cast < 2, '0' > (aNode) + "-sgp"), aNode);
  theSGPs[aNode][aLevel]->sgp.init(enableUsage, enableRepet, enableBufFetch,
                                   enableTimeRepet, enablePrefetch, enableActive,
                                   enableOrdering, enableStreaming,
                                   blockSize, sgpBlocks, repetType, repetFills,
                                   sparseOpt, phtSize, phtAssoc, pcBits,
                                   cptType, cptSize, cptAssoc, cptSparse,
                                   fetchDist, streamWindow, streamDense, sendStreams,
                                   bufSize, streamDescs, delayedCommits, cptFilterSize);
  theSgpEnabled = true;
  theBlockSize = blockSize;
  if (enablePrefetch && delayedCommits) {
    enablePrefetchTracking(aNode);
    enableSgpTracking(aNode, blockSize, sgpBlocks);
  }
  if (aLevel == SharedTypes::eCore) {
    if ((int)theSimCaches.size() <= aNode) {
      theSimCaches.resize(aNode + 1);
    }
    theSimCaches[aNode] = new SimCacheEntry( std::string(boost::padded_string_cast < 2, '0' > (aNode) + "-inOrderCache") );
    theCacheSize = 65536;
    theCacheAssoc = 2;
    theCacheBlockShift = LOG2(blockSize);
    theSimCaches[aNode]->theCache.init(theCacheSize / theBlockSize, theCacheAssoc, 0);
  }
}

void TraceTracker::saveSGP(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel, std::string const & aDirName) {
  DBG_Assert(aNode < (int)theSGPs.size());
  DBG_Assert( theSgpLevels.count(aLevel) > 0 );
  theSGPs[aNode][aLevel]->sgp.saveState(aDirName);
}

void TraceTracker::loadSGP(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel, std::string const & aDirName) {
  DBG_Assert(aNode < (int)theSGPs.size());
  DBG_Assert( theSgpLevels.count(aLevel) > 0 );
  theSGPs[aNode][aLevel]->sgp.loadState(aDirName);
  if (!theSimCaches.empty()) {
    theSimCaches[aNode]->loadState(aNode, aDirName);
  }
}

bool TraceTracker::prefetchReady(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel) {
  if (theSgpEnabled) {
    if (theSgpLevels.count(aLevel) > 0) {
      return theSGPs[aNode][aLevel]->sgp.prefetchReady();
    }
  }
  return false;
}

address_t TraceTracker::getPrefetch(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel) {
  if (theSgpEnabled) {
    if (theSgpLevels.count(aLevel) > 0) {
      return theSGPs[aNode][aLevel]->sgp.getPrefetch();
    }
  }
  return 0;
}

boost::intrusive_ptr<SharedTypes::PrefetchCommand> TraceTracker::getPrefetchCommand(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel) {
  return theSGPs[aNode][aLevel]->sgp.getPrefetchCommand();
}

void TraceTracker::initGHB(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel, int32_t blockSize, int32_t ghbSize) {
  theGhbLevels.insert(aLevel);
  if ((int)theGHBs.size() <= aNode) {
    theGHBs.resize(aNode + 1);
  }
  if (theGHBs[aNode].size() < static_cast<unsigned>(aLevel)) {
    theGHBs[aNode].resize(aLevel + 1, 0);
  }
  theGHBs[aNode][aLevel] = new GhbEntry(aLevel, std::string(boost::padded_string_cast < 2, '0' > (aNode) + "-ghbL" + boost::padded_string_cast < 1, '0' > (aLevel)), aNode);
  theGHBs[aNode][aLevel]->ghb.init(blockSize, ghbSize);
  theGhbEnabled = true;
  theBlockSize = blockSize;
  enablePrefetchTracking(aNode);
}

bool TraceTracker::ghbPrefetchReady(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel) {
  if (theGhbEnabled) {
    if (theGhbLevels.count(aLevel) > 0) {
      return theGHBs[aNode][aLevel]->ghb.prefetchReady();
    }
  }
  return false;
}

address_t TraceTracker::ghbGetPrefetch(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel) {
  if (theGhbEnabled) {
    if (theGhbLevels.count(aLevel) > 0) {
      return theGHBs[aNode][aLevel]->ghb.getPrefetch();
    }
  }
  return 0;
}
void TraceTracker::enablePrefetchTracking(int32_t aNode) {
  thePrefetchTracking = true;
  if ((int)thePrefetchTrackers.size() <= aNode) {
    thePrefetchTrackers.resize(aNode + 1);
  }
  thePrefetchTrackers[aNode] = new PrefetchTracker(std::string(boost::padded_string_cast < 2, '0' > (aNode) + "-prefTrack"));
}
void TraceTracker::enableSgpTracking(int32_t aNode, int32_t blockSize, int32_t sgpBlocks) {
  theSgpTracking = true;
  if ((int)theSgpTrackers.size() <= aNode) {
    theSgpTrackers.resize(aNode + 1);
  }
  theSgpTrackers[aNode] = new SgpTracker(std::string(boost::padded_string_cast < 2, '0' > (aNode) + "-sgpTrack"), blockSize, sgpBlocks);
}

bool TraceTracker::simCacheAccess(int32_t aNode, address_t addr, bool write) {
  theSimCaches[aNode]->statAccesses++;
  if (!write) theSimCaches[aNode]->statReads++;
  addr = addr >> theCacheBlockShift;
  SimCacheIter iter = theSimCaches[aNode]->theCache.find(addr);
  if (iter != theSimCaches[aNode]->theCache.end()) {
    theSimCaches[aNode]->theCache.move_back(iter);
    return false;  // not a miss
  }
  if ((int)theSimCaches[aNode]->theCache.size() >= theCacheAssoc) {
    address_t temp = theSimCaches[aNode]->theCache.front_key() << theCacheBlockShift;
    eviction(aNode, SharedTypes::eCore, temp, false);
    theSimCaches[aNode]->theCache.pop_front();
  }
  theSimCaches[aNode]->theCache.insert( std::make_pair(addr, 0) );
  ++theSimCaches[aNode]->statMisses;
  if (!write) theSimCaches[aNode]->statReadMisses++;
  return true;  // was a miss
}
bool TraceTracker::simCacheInval(int32_t aNode, address_t addr) {
  addr = addr >> theCacheBlockShift;
  SimCacheIter iter = theSimCaches[aNode]->theCache.find(addr);
  if (iter != theSimCaches[aNode]->theCache.end()) {
    theSimCaches[aNode]->theCache.erase(iter);
    ++theSimCaches[aNode]->statInvals;
    return true;  // invalidated
  }
  return false;  // not present
}

void TraceTracker::initSharing(int32_t numNodes, int64_t blockSize, SharedTypes::tFillLevel aLevel) {
  theSharingTracker = new SharingTracker(numNodes, blockSize);
  theBaseSharingLevel = aLevel;
}
/* CMU-ONLY-BLOCK-END */

} // namespace nTraceTracker

#ifndef _TRACETRACKER_OBJECT_DEFINED_
#define _TRACETRACKER_OBJECT_DEFINED_
nTraceTracker::TraceTracker theTraceTracker;
#endif
