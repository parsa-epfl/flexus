#ifndef _TRACE_TRACKER_HPP_
#define _TRACE_TRACKER_HPP_

#include <map>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <memory>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/PrefetchCommand.hpp> /* CMU-ONLY */

#include DBG_Control()

namespace nTraceTracker {

using Flexus::SharedTypes::PrefetchCommand; /* CMU-ONLY */

typedef uint32_t address_t;

struct NodeEntry;  /* CMU-ONLY */
struct GhbEntry;  /* CMU-ONLY */
struct PrefetchTracker;  /* CMU-ONLY */
struct SgpTracker;  /* CMU-ONLY */
struct SimCacheEntry;  /* CMU-ONLY */
class SharingTracker; /* CMU-ONLY */

class TraceTracker {

  /* CMU-ONLY-BLOCK-BEGIN */
  SharingTracker * theSharingTracker;
  Flexus::SharedTypes::tFillLevel theBaseSharingLevel;

  int32_t theBlockSize;
  bool theSgpEnabled;
  std::vector< Flexus::SharedTypes::tFillLevel > theCurrLevels;
  std::set<Flexus::SharedTypes::tFillLevel> theSgpLevels;
  std::vector< std::vector< NodeEntry * > > theSGPs;
  bool theGhbEnabled;
  std::set<Flexus::SharedTypes::tFillLevel> theGhbLevels;
  std::vector< std::vector< GhbEntry * > > theGHBs;
  bool thePrefetchTracking;
  std::vector< PrefetchTracker * > thePrefetchTrackers;
  bool theSgpTracking;
  std::vector< SgpTracker * > theSgpTrackers;
  int32_t theCacheSize, theCacheAssoc, theCacheBlockShift;
  std::vector< SimCacheEntry * > theSimCaches;
  /* CMU-ONLY-BLOCK-END */

public:
  void access      (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, bool prefetched, bool write, bool miss, bool priv, uint64_t ltime);
  void commit      (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, uint64_t ltime);
  void store       (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, bool miss, bool priv, uint64_t ltime);
  void prefetch    (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void fill        (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, Flexus::SharedTypes::tFillLevel fillLevel, bool isFetch, bool isWrite);
  void prefetchFill(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, Flexus::SharedTypes::tFillLevel fillLevel);
  void prefetchHit (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool isWrite);
  void prefetchRedundant(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void parallelList(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, std::set<uint64_t> & aParallelList); /* CMU-ONLY */
  void insert      (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void eviction    (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool drop);
  void invalidation(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void invalidAck  (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void invalidTagCreate (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void invalidTagRefill (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  void invalidTagReplace(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
  /* CMU-ONLY-BLOCK-BEGIN */
  void beginSpatialGen  (int32_t aNode, address_t block);
  void endSpatialGen    (int32_t aNode, address_t block);
  void sgpPredict       (int32_t aNode, address_t group, void * aPredictSet);
  /* CMU-ONLY-BLOCK-END */

  void accessLoad  (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size);
  void accessStore (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size);
  void accessFetch (int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size);
  void accessAtomic(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size);

  TraceTracker();
  void initialize();
  void finalize();

  /* CMU-ONLY-BLOCK-BEGIN */
  void initOffChipTracking(int32_t aNode);
  void initSGP(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel,
               bool enableUsage, bool enableRepet, bool enableBufFetch,
               bool enableTimeRepet, bool enablePrefetch, bool enableActive,
               bool enableOrdering, bool enableStreaming,
               int32_t blockSize, int32_t sgpBlocks, int32_t repetType, bool repetFills,
               bool sparseOpt, int32_t phtSize, int32_t phtAssoc, int32_t pcBits,
               int32_t cptType, int32_t cptSize, int32_t cptAssoc, bool cptSparse,
               bool fetchDist, int32_t streamWindow, bool streamDense, bool sendStreams,
               int32_t bufSize, int32_t streamDescs, bool delayedCommits, int32_t cptFilterSize);
  void saveSGP(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel, std::string const & aDirName);
  void loadSGP(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel, std::string const & aDirName);
  bool prefetchReady(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel);
  address_t getPrefetch(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel);
  boost::intrusive_ptr<Flexus::SharedTypes::PrefetchCommand> getPrefetchCommand(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel);
  void initGHB(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel, int32_t blockSize, int32_t ghbSize);
  bool ghbPrefetchReady(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel);
  address_t ghbGetPrefetch(int32_t aNode, Flexus::SharedTypes::tFillLevel aLevel);
  void enablePrefetchTracking(int32_t aNode);
  void enableSgpTracking(int32_t aNode, int32_t blockSize, int32_t sgpBlocks);
  bool simCacheAccess(int32_t aNode, address_t addr, bool write);
  bool simCacheInval(int32_t aNode, address_t addr);

  void initSharing(int32_t numNodes, int64_t blockSize, Flexus::SharedTypes::tFillLevel aLevel);
  /* CMU-ONLY-BLOCK-END */

};

} // namespace nTraceTracker

extern nTraceTracker::TraceTracker theTraceTracker;

#endif
