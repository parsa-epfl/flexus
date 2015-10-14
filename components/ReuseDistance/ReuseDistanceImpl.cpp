#include <components/ReuseDistance/ReuseDistance.hpp>

#include <iostream>
#include <iomanip>

#include <boost/bind.hpp>
#include <unordered_map>

#include <core/flexus.hpp>
#include <core/stats.hpp>

#define DBG_DefineCategories ReuseDistance
#define DBG_SetDefaultOps AddCat(ReuseDistance) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT ReuseDistance
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nReuseDistance {
using namespace Flexus;
using namespace Flexus::Core;

namespace Stat = Flexus::Stat;
using boost::counted_base;
using boost::intrusive_ptr;

static uint32_t theBlockSizeLog2;

class FLEXUS_COMPONENT(ReuseDistance) {
  FLEXUS_COMPONENT_IMPL( ReuseDistance );

  //////////////////// type definitions
  typedef uint32_t      block_address_t;
  typedef uint16_t     access_t;
  typedef uint64_t tRefCount;

  //////////////////// standard deviation log2 histogram class
  class stdDevLog2Histogram {

  public:
    typedef double value_type;
    typedef std::pair < int64_t /*mean*/, value_type /*stdd*/ > update_type;
    std::vector<value_type> theBuckets;
    std::vector<int64_t> theBucketCounts;

  private:
    int64_t log2(int64_t aVal) const {
      uint32_t ii = 0;
      while (aVal > 1) {
        ii++;
        aVal >>= 1;
      }
      return ii;
    }

    double calculate(const value_type oldStdDev,
                     const int64_t  oldPopulation,
                     const value_type newStdDev,
                     const int64_t  newPopulation) const {
      return (value_type)(sqrt(  static_cast<double>(oldPopulation * oldStdDev * oldStdDev + newPopulation * newStdDev * newStdDev)
                                 / static_cast<double>(oldPopulation + newPopulation)));
    }

  public:
    stdDevLog2Histogram() {}

    stdDevLog2Histogram & operator << (const update_type anUpdate) {
      if (anUpdate.first == 0) {
        if (theBuckets.size() == 0) {
          theBuckets.resize(1, 0);
          theBucketCounts.resize(1, 0);
        }
        theBuckets[0] = calculate(theBuckets[0], theBucketCounts[0], anUpdate.second, 1);
        theBucketCounts[0] ++;
      } else {
        uint64_t log = log2(anUpdate.first);
        if (theBuckets.size() < log + 2) {
          theBuckets.resize(log + 2, 0);
          theBucketCounts.resize(log + 2, 0);
        }
        theBuckets[log+1] = calculate(theBuckets[log+1], theBucketCounts[log+1], anUpdate.second, 1);
        theBucketCounts[log+1] ++;
      }
      return *this;
    }

    void reset() {
      theBuckets.clear();
      theBucketCounts.clear();
    }

    void print() const {
      std::cerr << "0: " << theBuckets[0] << std::endl;
      for (uint32_t i = 1; i < theBuckets.size(); i++) {
        std::cerr << (1LL << (i - 1)) << ": " << theBuckets[i] << std::endl;
      }
    }

    int32_t buckets() const {
      return theBuckets.size();
    }

    double bucketVal(const int32_t i) const {
      return theBuckets[i];
    }
  };

  //////////////////// online standard deviation and mean for reuse distance per line
  typedef struct tLineReuseDist {
    uint64_t ReuseDist_k;
    double             ReuseDist_Sum;
    double             ReuseDist_SumSq;
    double             ReuseDist_SigmaSqSum;

    void reset( void ) {
      ReuseDist_k = 0;
      ReuseDist_Sum = 0.0;
      ReuseDist_SumSq = 0.0;
      ReuseDist_SigmaSqSum = 0.0;
    }

    void update( const uint64_t anUpdate ) {
      const double ReuseDist_Xsq = anUpdate * anUpdate;

      ReuseDist_SigmaSqSum += ReuseDist_SumSq + ReuseDist_k * ReuseDist_Xsq - 2 * anUpdate * ReuseDist_Sum;

      ReuseDist_k++;
      ReuseDist_Sum += anUpdate;
      ReuseDist_SumSq += ReuseDist_Xsq;
    }

    double stdDev( void ) const {
      if (ReuseDist_k == 0) {
        return 0;
      } else {
        return sqrt(ReuseDist_SigmaSqSum) / static_cast<double>(ReuseDist_k);
      }
    }

    int64_t mean( void ) const {
      if (ReuseDist_k == 0) {
        return 0;
      } else {
        return static_cast<int64_t>(ReuseDist_Sum / static_cast<double>(ReuseDist_k));
      }
    }

    int64_t meanWithAging( const uint64_t anUnusedDistance ) const {
      static const uint32_t ReuseFactor = 1UL;
      static const uint32_t UnuseFactor = 1UL;

      if ((ReuseDist_k == 0) && (UnuseFactor == 0)) {
        return 0;
      } else {
        return static_cast<int64_t>( (ReuseDist_Sum * ReuseFactor + anUnusedDistance * UnuseFactor)
                                     / static_cast<double>(ReuseDist_k * (ReuseFactor == 0 ? 0 : 1) + (UnuseFactor == 0 ? 0 : 1))   );
      }
    }

    int64_t samples( void ) const {
      return ReuseDist_k;
    }

    void print( void ) const {
      std::cerr << "Mean: " <<  mean() << " StdDev: " <<  stdDev() << std::endl;
    }

  } tLineReuseDist;

  //////////////////// the reuse address map
  struct IntHash {
    std::size_t operator()(uint32_t key) const {
      key = key >> nReuseDistance::theBlockSizeLog2;
      return key;
    }
  };

  typedef struct {
    tRefCount      lastAccessRefCount;       // # cache refs at last touch
    tRefCount      dirtySharedRefCount;      // # cache refs since dirty->dirtyShared transition
    tRefCount      coreA2coreBRefCount;      // # cache refs since block was referenced only by coreIdxLastRef
    tRefCount      numBlockRefsByLastCore;   // # refs issued on this block within a core lifetime
    tRefCount      numBlockRefs;             // # refs issued on this block within the stat window
    int64_t      sharersBitmask;           // # cores reading the block within the sharing window (or the DirtyShared window for RW blocks)
    int16_t          coreIdxLastWriter;        // the core index of the last writer
    int16_t          coreIdxLastRef;           // the core index of the last requestor
    bool           touchedManyFlag;          // true: touched multiple times
    bool           sharedFlag;               // true: touched by multiple processors
    bool           RWFlag;                   // true: block has been written to
    bool           dirtyFlag;                // true: block is in dirty state
    bool           istream;                  // true: block participates in the istream
    tLineReuseDist lineReuseDist;            // online standard deviation and mean for reuse distance per line

  } tReuseMapEntry;

  typedef std::unordered_map < const block_address_t, // block address
          tReuseMapEntry,        // reuse map entry  pointer
          IntHash
          > tReuseMap;

  tReuseMap theReuseMap;

  //////////////////// the PC-reuse-distance correlation stats
  typedef std::unordered_map < const block_address_t, // pc block address
          tLineReuseDist,        // PC-reuse-distance correlation stats entry pointer
          IntHash
          > tPCCorrMap;
  tPCCorrMap thePCCorrMap;

  //////////////////// type definitions for cdf of Blocks vs L2Refs for all categories
  typedef block_address_t block_address_with_flags_t;
  typedef bool ( nReuseDistance::ReuseDistanceComponent::* tIsBlockxxxFunc ) ( const block_address_with_flags_t anAddressWithFlags ) ;

  //////////////////// type definitions for the ordered multiset of reuse distances per block
  typedef std::pair<block_address_with_flags_t, tRefCount> tReuseOrderedSetEntry;
  struct ltReuseMapEntry {
    bool operator() (const tReuseOrderedSetEntry & ent1, const tReuseOrderedSetEntry & ent2) const {
      return (ent1.second < ent2.second);
    }
  };
  typedef std::multiset< tReuseOrderedSetEntry, ltReuseMapEntry > tReuseOrderedSet;

  ////////////////////// private variables
  block_address_t    theBlockMask;
  uint64_t theReuseWinSizeInCycles;
  uint64_t theCurrReuseWinCycles;
  tRefCount          theCurrRefCount;

  ////////////////////// Reuse distance stats
  Stat::StatLog2Histogram   theReuseDistPrivate;
  Stat::StatLog2Histogram   theReuseDistPrivateFetch;
  Stat::StatLog2Histogram   theNumSameBlockRefsPrivate;
  Stat::StatCounter         theNumSameBlockRefsPrivateCount;

  Stat::StatLog2Histogram   theReuseDistROFetch;
  Stat::StatLog2Histogram   theReuseDistROSingleCore;
  Stat::StatLog2Histogram   theReuseDistROCoreTransitions;
  Stat::StatLog2Histogram   theReuseDistROCoreLifetime;
  Stat::StatLog2Histogram   theNumSameBlockRefsRO;
  Stat::StatCounter         theNumSameBlockRefsROCount;
  Stat::StatLog2Histogram   theNumSameBlockRefsROInCoreLifetime;
  Stat::StatInstanceCounter<int64_t> theNumSharersRO;

  Stat::StatLog2Histogram   theReuseDistRWWriterDirty;
  Stat::StatLog2Histogram   theReuseDistRWDirtyCoreTransition;
  Stat::StatLog2Histogram   theReuseDistRWReaderShared;
  Stat::StatLog2Histogram   theReuseDistRWSharedCoreTransition;
  Stat::StatLog2Histogram   theReuseDistRWWriterShared;
  Stat::StatLog2Histogram   theReuseDistRWDirtySharedLifetime;
  Stat::StatLog2Histogram   theReuseDistRWCoreLifetime;
  Stat::StatLog2Histogram   theNumSameBlockRefsRW;
  Stat::StatCounter         theNumSameBlockRefsRWCount;
  Stat::StatLog2Histogram   theNumSameBlockRefsRWInCoreLifetime;
  Stat::StatInstanceCounter<int64_t> theNumSharersRW;

  Stat::StatLog2Histogram   theUnusedDistTouchedOnce;
  Stat::StatLog2Histogram   theUnusedDistPrivate;
  Stat::StatLog2Histogram   theUnusedDistRO;
  Stat::StatLog2Histogram   theUnusedDistRW;

  Stat::StatCounter         theBlockCountPrivate;
  Stat::StatCounter         theBlockCountPrivateFetch;
  Stat::StatCounter         theBlockCountTouchedOnce;
  Stat::StatCounter         theBlockCountTouchedOnceFetch;
  Stat::StatCounter         theBlockCountSharedRO;
  Stat::StatCounter         theBlockCountSharedROFetch;
  Stat::StatCounter         theBlockCountSharedRW;
  Stat::StatCounter         theBlockCountSharedRWFetch;

  Stat::StatCounter         theRefPrivate;
  Stat::StatCounter         theRefPrivateFetch;
  Stat::StatCounter         theRefSharedRO;
  Stat::StatCounter         theRefSharedROFetch;
  Stat::StatCounter         theRefSharedRW;
  Stat::StatCounter         theRefSharedRWFetch;

  Stat::StatLog2Histogram   theDiscardedDist;
  Stat::StatCounter         theDiscardedCount;

  Stat::StatLog2Histogram         avgReuseDistPrivateData;         // mean of reuse distance per block (priv-data)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsPrivateData;   // mean of reuse distance per block (priv-data) vs. L2 refs
  Stat::StatWeightedLog2Histogram stdDevReuseDistPrivateData;      // std.d. of reuse distance per block (priv-data)
  stdDevLog2Histogram             lineStdDevReuseDistPrivateData;  // std.d. of reuse distance per block temp (priv-data)

  Stat::StatLog2Histogram         avgReuseDistROData;              // mean of reuse distance per block (RO-data)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsROData;        // mean of reuse distance per block (RO-data) vs. L2 refs
  Stat::StatWeightedLog2Histogram stdDevReuseDistROData;           // std.d. of reuse distance per block (RO-data)
  stdDevLog2Histogram             lineStdDevReuseDistROData;       // std.d. of reuse distance per block temp (RO-data)

  Stat::StatLog2Histogram         avgReuseDistRWData;              // mean of reuse distance per block (RW-data)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsRWData;        // mean of reuse distance per block (RW-data) vs. L2 refs
  Stat::StatWeightedLog2Histogram stdDevReuseDistRWData;           // std.d. of reuse distance per block (RW-data)
  stdDevLog2Histogram             lineStdDevReuseDistRWData;       // std.d. of reuse distance per block temp (RW-data)

  Stat::StatLog2Histogram         avgReuseDistPrivateFetch;         // mean of reuse distance per block (priv-instr)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsPrivateFetch;   // mean of reuse distance per block (priv-instr) vs. L2 refs
  Stat::StatWeightedLog2Histogram stdDevReuseDistPrivateFetch;      // std.d. of reuse distance per block (priv-instr)
  stdDevLog2Histogram             lineStdDevReuseDistPrivateFetch;  // std.d. of reuse distance per block temp (priv-instr)

  Stat::StatLog2Histogram         avgReuseDistROFetch;              // mean of reuse distance per block (RO-instr)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsROFetch;        // mean of reuse distance per block (RO-instr) vs. L2 refs
  Stat::StatWeightedLog2Histogram stdDevReuseDistROFetch;           // std.d. of reuse distance per block (RO-instr)
  stdDevLog2Histogram             lineStdDevReuseDistROFetch;       // std.d. of reuse distance per block temp (RO-instr)

  Stat::StatLog2Histogram         avgReuseDistRWFetch;              // mean of reuse distance per block (RW-instr)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsRWFetch;        // mean of reuse distance per block (RW-instr) vs. L2 refs
  Stat::StatWeightedLog2Histogram stdDevReuseDistRWFetch;           // std.d. of reuse distance per block (RW-instr)
  stdDevLog2Histogram             lineStdDevReuseDistRWFetch;       // std.d. of reuse distance per block temp (RW-instr)

  Stat::StatLog2Histogram         avgReuseDistPCCorr;               // mean of reuse distance per block (pc)
  Stat::StatWeightedLog2Histogram avgReuseDistL2RefsPCCorr;         // mean of reuse distance per block (pc) vs. L2 refs covered
  Stat::StatWeightedLog2Histogram stdDevReuseDistPCCorr;            // std.d. of reuse distance per block (pc)
  stdDevLog2Histogram             lineStdDevReuseDistPCCorr;        // std.d. of reuse distance per block temp (pc)

  Stat::StatInstanceCounter<int64_t> cdfBlocksL2Refs;
  Stat::StatInstanceCounter<int64_t> cdfBlocksL2RefsPrivateData;
  Stat::StatInstanceCounter<int64_t> cdfBlocksL2RefsPrivateFetch;
  Stat::StatInstanceCounter<int64_t> cdfBlocksL2RefsROData;
  Stat::StatInstanceCounter<int64_t> cdfBlocksL2RefsROFetch;
  Stat::StatInstanceCounter<int64_t> cdfBlocksL2RefsRWData;
  Stat::StatInstanceCounter<int64_t> cdfBlocksL2RefsRWFetch;
  // Stat::StatLog2Histogram         theReuseDistanceBlock0;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock1;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock2;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock3;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock4;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock5;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock6;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock7;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock8;           // hack
  // Stat::StatLog2Histogram         theReuseDistanceBlock9;           // hack

  /////////////////////////// function implementations
private:
  uint32_t log_base2(uint32_t num) const {
    uint32_t ii = 0;
    while (num > 1) {
      ii++;
      num >>= 1;
    }
    return ii;
  }

  inline block_address_t blockAddress(const PhysicalMemoryAddress addr) const {
    return (addr & theBlockMask);
  }

  inline block_address_t blockAddress(const VirtualMemoryAddress addr) const {
    return (addr & theBlockMask);
  }

  inline int64_t CoreIdx2SharerBit( const int16_t coreIdx ) const {
    return (1LL << coreIdx);
  }

  inline bool isSharer( const int64_t sharersBitmask,
                        const int16_t     coreIdx ) const {
    return ((sharersBitmask & CoreIdx2SharerBit(coreIdx)) != 0LL);
  }

  inline int32_t countNumSharers( const int64_t sharersBitmask ) const {
    int32_t count = 0;
    for (int32_t i = 0; i < (int) (sizeof(sharersBitmask) * 8); i++) {
      if (sharersBitmask & (1LL << i)) {
        count ++;
      }
    }
    return count;
  }

  inline bool isPrivateBlock( const tReuseMapEntry & aReuseMapEntry,
                              const int16_t            coreIdx ) const {
    return (
             // it was private, and will remain
             !aReuseMapEntry.sharedFlag
             && aReuseMapEntry.coreIdxLastRef == coreIdx
           );
  }

  inline bool isSharedROBlock( const tReuseMapEntry & aReuseMapEntry,
                               const int16_t            coreIdx,
                               const bool             isWrite ) const {
    return (
             // it was shared RO, and will remain
             (aReuseMapEntry.sharedFlag &&
              !aReuseMapEntry.RWFlag &&
              !isWrite)
             ||
             // it was private, now it will become sharedRO
             (!aReuseMapEntry.sharedFlag &&
              aReuseMapEntry.coreIdxLastRef != coreIdx &&
              !aReuseMapEntry.RWFlag &&
              !isWrite)
           );
  }

  inline bool isSharedRWBlock( const tReuseMapEntry & aReuseMapEntry,
                               const int16_t            coreIdx,
                               const bool             isWrite ) const {
    return (
             // it was sharedRW
             (aReuseMapEntry.sharedFlag &&
              aReuseMapEntry.RWFlag)
             ||
             // it was sharedRO, will become sharedRW
             (aReuseMapEntry.sharedFlag &&
              isWrite)
             ||
             // it was private, will become sharedRW
             (!aReuseMapEntry.sharedFlag
              && aReuseMapEntry.coreIdxLastRef != coreIdx
              && (aReuseMapEntry.RWFlag || isWrite))
           );
  }

  bool isBlockPrivateD(const block_address_with_flags_t anAddressWithFlags) {
    return ((static_cast<uint32_t>(anAddressWithFlags) & 0x7UL) == 0x0UL);
  }
  bool isBlockPrivateI(const block_address_with_flags_t anAddressWithFlags) {
    return ((static_cast<uint32_t>(anAddressWithFlags) & 0x7UL) == 0x1UL);
  }
  bool isBlockROD     (const block_address_with_flags_t anAddressWithFlags) {
    return ((static_cast<uint32_t>(anAddressWithFlags) & 0x7UL) == 0x2UL);
  }
  bool isBlockROI     (const block_address_with_flags_t anAddressWithFlags) {
    return ((static_cast<uint32_t>(anAddressWithFlags) & 0x7UL) == 0x3UL);
  }
  bool isBlockRWD     (const block_address_with_flags_t anAddressWithFlags) {
    return ((static_cast<uint32_t>(anAddressWithFlags) & 0x7UL) == 0x4UL);
  }
  bool isBlockRWI     (const block_address_with_flags_t anAddressWithFlags) {
    return ((static_cast<uint32_t>(anAddressWithFlags) & 0x7UL) == 0x5UL);
  }
  bool isBlockAny     (const block_address_with_flags_t anAddressWithFlags) {
    return true;
  }

  void computeCDFBlocksL2Refs( const tReuseOrderedSet        &        aReuseOrderedSet,
                               const tRefCount                        aTotalRefs,
                               Stat::StatInstanceCounter<int64_t> & aCdfBlocksRefs,
                               tIsBlockxxxFunc                        anIsBlockxxxFunc ) {
    if (aTotalRefs == 0) {
      return;
    }

    static const uint32_t pct_step   = 5;
    const uint32_t        ref_step   = aTotalRefs * pct_step / 100;
    uint32_t              ref_target = ref_step;

    tRefCount     currRefs   = 0;
    uint32_t currBlocks = 0;

    for ( tReuseOrderedSet::reverse_iterator iter = aReuseOrderedSet.rbegin();
          iter != aReuseOrderedSet.rend();
        ) {
      while (currRefs < ref_target && iter != aReuseOrderedSet.rend()) {
        if ((this->*anIsBlockxxxFunc)(iter->first) == true) {
          currRefs += iter->second;
          ++currBlocks;
        }
        iter++;
      }
      aCdfBlocksRefs << std::make_pair( currRefs * 100 / aTotalRefs, currBlocks );
      ref_target += ref_step;
    }
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(ReuseDistance)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theBlockMask(0)
    , theReuseWinSizeInCycles(0)
    , theCurrReuseWinCycles(0)
    , theCurrRefCount(0)
    , theReuseDistPrivate( statName() + "-Private")
    , theReuseDistPrivateFetch( statName() + "-Private:Fetch")
    , theNumSameBlockRefsPrivate( statName() + "-NumSameBlockRefs:Private")
    , theNumSameBlockRefsPrivateCount( statName() + "-NumSameBlockRefs:Count:Private")
    , theReuseDistROFetch( statName() + "-RO:Fetch")
    , theReuseDistROSingleCore( statName() + "-RO:SingleCore")
    , theReuseDistROCoreTransitions( statName() + "-RO:CoreTransitions")
    , theReuseDistROCoreLifetime( statName() + "-RO:CoreLifetime")
    , theNumSameBlockRefsRO( statName() + "-NumSameBlockRefs:RO")
    , theNumSameBlockRefsROCount( statName() + "-NumSameBlockRefs:Count:RO")
    , theNumSameBlockRefsROInCoreLifetime( statName() + "-NumSameBlockRefs:RO:InCoreLifetime")
    , theNumSharersRO( statName() + "-NumSharers:RO")
    , theReuseDistRWWriterDirty( statName() + "-RW:WriterDirty")
    , theReuseDistRWDirtyCoreTransition( statName() + "-RW:DirtyCoreTransition")
    , theReuseDistRWReaderShared( statName() + "-RW:ReaderShared")
    , theReuseDistRWSharedCoreTransition( statName() + "-RW:SharedCoreTransition")
    , theReuseDistRWWriterShared( statName() + "-RW:WriterShared")
    , theReuseDistRWDirtySharedLifetime( statName() + "-RW:DirtySharedLifetime")
    , theReuseDistRWCoreLifetime( statName() + "-RW:CoreLifetime")
    , theNumSameBlockRefsRW( statName() + "-NumSameBlockRefs:RW")
    , theNumSameBlockRefsRWCount( statName() + "-NumSameBlockRefs:Count:RW")
    , theNumSameBlockRefsRWInCoreLifetime( statName() + "-NumSameBlockRefs:RW:InCoreLifetime")
    , theNumSharersRW( statName() + "-NumSharers:RW")
    , theUnusedDistTouchedOnce( statName() + "-UnusedDist:TouchedOnce" )
    , theUnusedDistPrivate( statName() + "-UnusedDist:TouchedMany:Private" )
    , theUnusedDistRO( statName() + "-UnusedDist:TouchedMany:RO" )
    , theUnusedDistRW( statName() + "-UnusedDist:TouchedMany:RW" )
    , theBlockCountPrivate( statName() + "-BlockCount:Private")
    , theBlockCountPrivateFetch( statName() + "-BlockCount:Private:Fetch")
    , theBlockCountTouchedOnce( statName() + "-BlockCount:TouchedOnce")
    , theBlockCountTouchedOnceFetch( statName() + "-BlockCount:TouchedOnce:Fetch")
    , theBlockCountSharedRO( statName() + "-BlockCount:Shared:RO")
    , theBlockCountSharedROFetch( statName() + "-BlockCount:Shared:RO:Fetch")
    , theBlockCountSharedRW( statName() + "-BlockCount:Shared:RW")
    , theBlockCountSharedRWFetch( statName() + "-BlockCount:Shared:RW:Fetch")
    , theRefPrivate( statName() + "-Ref:Private")
    , theRefPrivateFetch( statName() + "-Ref:Private:Fetch")
    , theRefSharedRO( statName() + "-Ref:Shared:RO")
    , theRefSharedROFetch( statName() + "-Ref:Shared:RO:Fetch")
    , theRefSharedRW( statName() + "-Ref:Shared:RW")
    , theRefSharedRWFetch( statName() + "-Ref:Shared:RW:Fetch")
    , theDiscardedDist( statName() + "-DiscardedDist" )
    , theDiscardedCount( statName() + "-DiscardedCount" )
    , avgReuseDistPrivateData( statName() + "-Private:Data:Avg" )
    , avgReuseDistL2RefsPrivateData( statName() + "-L2Refs-Private:Data:Avg" )
    , stdDevReuseDistPrivateData( statName() + "-Private:Data:StdDev" )
    , avgReuseDistROData( statName() + "-RO:Data:Avg" )
    , avgReuseDistL2RefsROData( statName() + "-L2Refs-RO:Data:Avg" )
    , stdDevReuseDistROData( statName() + "-RO:Data:StdDev" )
    , avgReuseDistRWData( statName() + "-RW:Data:Avg" )
    , avgReuseDistL2RefsRWData( statName() + "-L2Refs-RW:Data:Avg" )
    , stdDevReuseDistRWData( statName() + "-RW:Data:StdDev" )
    , avgReuseDistPrivateFetch( statName() + "-Private:Fetch:Avg" )
    , avgReuseDistL2RefsPrivateFetch( statName() + "-L2Refs-Private:Fetch:Avg" )
    , stdDevReuseDistPrivateFetch( statName() + "-Private:Fetch:StdDev" )
    , avgReuseDistROFetch( statName() + "-RO:Fetch:Avg" )
    , avgReuseDistL2RefsROFetch( statName() + "-L2Refs-RO:Fetch:Avg" )
    , stdDevReuseDistROFetch( statName() + "-RO:Fetch:StdDev" )
    , avgReuseDistRWFetch( statName() + "-RW:Fetch:Avg" )
    , avgReuseDistL2RefsRWFetch( statName() + "-L2Refs-RW:Fetch:Avg" )
    , stdDevReuseDistRWFetch( statName() + "-RW:Fetch:StdDev" )
    , avgReuseDistPCCorr( statName() + "-PCCorr:Avg" )
    , avgReuseDistL2RefsPCCorr( statName() + "-L2Refs-PCCorr:Avg" )
    , stdDevReuseDistPCCorr( statName() + "-PCCorr:StdDev" )
    , cdfBlocksL2Refs( statName() + "-cdfBlocksL2Refs")
    , cdfBlocksL2RefsPrivateData( statName() + "-cdfBlocksL2Refs-Private:Data")
    , cdfBlocksL2RefsPrivateFetch( statName() + "-cdfBlocksL2Refs-Private:Fetch")
    , cdfBlocksL2RefsROData( statName() + "-cdfBlocksL2Refs-RO:Data")
    , cdfBlocksL2RefsROFetch( statName() + "-cdfBlocksL2Refs-RO:Fetch")
    , cdfBlocksL2RefsRWData( statName() + "-cdfBlocksL2Refs-RW:Data")
    , cdfBlocksL2RefsRWFetch( statName() + "-cdfBlocksL2Refs-RW:Fetch")
    // , theReuseDistanceBlock0( statName() + "-Block:0")           // hack
    // , theReuseDistanceBlock1( statName() + "-Block:1")           // hack
    // , theReuseDistanceBlock2( statName() + "-Block:2")           // hack
    // , theReuseDistanceBlock3( statName() + "-Block:3")           // hack
    // , theReuseDistanceBlock4( statName() + "-Block:4")           // hack
    // , theReuseDistanceBlock5( statName() + "-Block:5")           // hack
    // , theReuseDistanceBlock6( statName() + "-Block:6")           // hack
    // , theReuseDistanceBlock7( statName() + "-Block:7")           // hack
    // , theReuseDistanceBlock8( statName() + "-Block:8")           // hack
    // , theReuseDistanceBlock9( statName() + "-Block:9")           // hack
  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize( void ) {
    theBlockMask = ~(cfg.BlockSize - 1);
    theBlockSizeLog2 = log_base2(cfg.BlockSize);

    theReuseWinSizeInCycles = cfg.ReuseWindowSizeInCycles;
    theCurrReuseWinCycles = theFlexus->cycleCount();

    Stat::getStatManager()->addFinalizer( boost::lambda::bind( &nReuseDistance::ReuseDistanceComponent::finalizeWindow, this ) );
  }

  int64_t getMeanReuseDist(const PhysicalMemoryAddress aPhysAddress) {
    const block_address_t addr = blockAddress(aPhysAddress);
    tReuseMap::iterator iter = theReuseMap.find(addr);
    if ( iter != theReuseMap.end() ) {
      tReuseMapEntry & theReuseMapEntry = iter->second;
      // return theReuseMapEntry.lineReuseDist.mean();
      return theReuseMapEntry.lineReuseDist.meanWithAging(theFlexus->cycleCount() - theReuseMapEntry.lastAccessRefCount);
    } else {
      return 0LL;
    }
  }

  void finalizeWindow( void ) {
    // NOTE: at the end of each window, we reset the sharers bitmask for shared-RO lines.
    // Either do not do it, or create a large enough window to deal with it

    // cat /proc/4436/status | grep VmSize

    // order blocks by their number of block references
    tReuseOrderedSet reuseOrderedSet;
    tRefCount totalRefsPrivateData = 0;
    tRefCount totalRefsPrivateFetch = 0;
    tRefCount totalRefsROData = 0;
    tRefCount totalRefsROFetch = 0;
    tRefCount totalRefsRWData = 0;
    tRefCount totalRefsRWFetch = 0;
    for (tReuseMap::iterator iter = theReuseMap.begin(); iter != theReuseMap.end(); iter++) {
      block_address_t theAddress = iter->first;
      tReuseMapEntry & theReuseMapEntry = iter->second;
      tRefCount refCount = theReuseMapEntry.numBlockRefs;
      // Private
      if (theReuseMapEntry.sharedFlag == false) {
        if (theReuseMapEntry.istream == false) {
          totalRefsPrivateData += refCount;
          reuseOrderedSet.insert(std::make_pair(theAddress | 0x0UL, refCount));
        } else {
          totalRefsPrivateFetch += refCount;
          reuseOrderedSet.insert(std::make_pair(theAddress | 0x1UL, refCount));
        }
      }
      // RO
      else if (theReuseMapEntry.RWFlag == false) {
        if (theReuseMapEntry.istream == false) {
          totalRefsROData += refCount;
          reuseOrderedSet.insert(std::make_pair(theAddress | 0x2UL, refCount));
        } else {
          totalRefsROFetch += refCount;
          reuseOrderedSet.insert(std::make_pair(theAddress | 0x3UL, refCount));
        }
      }
      // RW
      else {
        if (theReuseMapEntry.istream == false) {
          totalRefsRWData += refCount;
          reuseOrderedSet.insert(std::make_pair(theAddress | 0x4UL, refCount));
        } else {
          totalRefsRWFetch += refCount;
          reuseOrderedSet.insert(std::make_pair(theAddress | 0x5UL, refCount));
        }
      }
    }

    // histogram the cdf of xxx I/Dstr blocks vs L2 refs
    const unsigned totalL2Refs =   totalRefsPrivateData + totalRefsPrivateFetch
                                   + totalRefsROData      + totalRefsROFetch
                                   + totalRefsRWData      + totalRefsRWFetch;
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalL2Refs,
                            cdfBlocksL2Refs,
                            &nReuseDistance::ReuseDistanceComponent::isBlockAny );
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalRefsPrivateData,
                            cdfBlocksL2RefsPrivateData,
                            &nReuseDistance::ReuseDistanceComponent::isBlockPrivateD );
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalRefsPrivateFetch,
                            cdfBlocksL2RefsPrivateFetch,
                            &nReuseDistance::ReuseDistanceComponent::isBlockPrivateI );
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalRefsROData,
                            cdfBlocksL2RefsROData,
                            &nReuseDistance::ReuseDistanceComponent::isBlockROD );
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalRefsROFetch,
                            cdfBlocksL2RefsROFetch,
                            &nReuseDistance::ReuseDistanceComponent::isBlockROI );
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalRefsRWData,
                            cdfBlocksL2RefsRWData,
                            &nReuseDistance::ReuseDistanceComponent::isBlockRWD );
    computeCDFBlocksL2Refs( reuseOrderedSet,
                            totalRefsRWFetch,
                            cdfBlocksL2RefsRWFetch,
                            &nReuseDistance::ReuseDistanceComponent::isBlockRWI );

    // hack: print the top 10 contributors
    /*
    currRefs = 0;
    currBlocks = 0;
    for (tReuseOrderedSet::reverse_iterator iter = reuseOrderedSet.rbegin();
         iter != reuseOrderedSet.rend() && currBlocks < 10;
         iter++, currBlocks++)
    {
      block_address_t theAddress = iter->first;
      tRefCount theRefCount = iter->second;
      currRefs += theRefCount;
      DBG_(Tmp, ( << "Addr: " << std::hex << theAddress
                  << " L2_refs: " << std::dec << theRefCount
                  << " cumm_L2_refs: " << (currRefs * 100 / totalL2Refs)));
    }
    */

    reuseOrderedSet.clear(); // cleanup

    // PC correlation stats
    for (tPCCorrMap::iterator iter = thePCCorrMap.begin(); iter != thePCCorrMap.end(); iter++) {
      tLineReuseDist & thePCCorrMapEntry = iter->second;
      avgReuseDistPCCorr << thePCCorrMapEntry.mean();
      lineStdDevReuseDistPCCorr << std::make_pair(thePCCorrMapEntry.mean(),
                                thePCCorrMapEntry.stdDev());
      avgReuseDistL2RefsPCCorr << std::make_pair(thePCCorrMapEntry.mean(),
                               thePCCorrMapEntry.samples() + 1);  // add 1 to account for the first reference
    }
    for (int32_t i = 0; i < lineStdDevReuseDistPCCorr.buckets(); ++i) {
      stdDevReuseDistPCCorr << std::make_pair( i ? (1LL << (i - 1)) : 0,
                            static_cast<int64_t>(lineStdDevReuseDistPCCorr.bucketVal(i)) );
    }

    // get rid of uninteresting blocks (if parameter set to non-zero)
    if (cfg.IgnoreBlocksWithMinDist) {
      for (tReuseMap::iterator iter = theReuseMap.begin(); iter != theReuseMap.end(); ) {
        tReuseMapEntry & theReuseMapEntry = iter->second;
        tRefCount distFromLastAccess = theCurrRefCount - theReuseMapEntry.lastAccessRefCount;
        tReuseMap::iterator iter_to_erase = iter;
        iter++;
        if (distFromLastAccess >= (tRefCount) cfg.IgnoreBlocksWithMinDist) {
          theDiscardedDist << (distFromLastAccess);
          theDiscardedCount++;
          theReuseMap.erase(iter_to_erase);
        }
      }
    }

    // Within the stats window: update theNumSameBlockRefsxxx, theNumSameBlockRefsxxxCount histograms.
    // Also update the theNumSharersRO and UnusedDist histograms, and theBlockCountxxx counters.
    // Look at these particular stats ONLY at the last measurement window, never look at the "all-measurements" output
    for (tReuseMap::iterator iter = theReuseMap.begin(); iter != theReuseMap.end(); iter++) {
      tReuseMapEntry & theReuseMapEntry = iter->second;
      tRefCount distFromLastAccess = theCurrRefCount - theReuseMapEntry.lastAccessRefCount;

      // private block
      if (theReuseMapEntry.sharedFlag == false) {
        theBlockCountPrivate++;

        theNumSameBlockRefsPrivate << (theReuseMapEntry.numBlockRefs);
        theNumSameBlockRefsPrivateCount += theReuseMapEntry.numBlockRefs;

        if (theReuseMapEntry.touchedManyFlag == false) {
          theBlockCountTouchedOnce++;
          theUnusedDistTouchedOnce << (distFromLastAccess);
        } else {
          theUnusedDistPrivate << (distFromLastAccess);
        }

        if (!theReuseMapEntry.istream) {
          avgReuseDistPrivateData << theReuseMapEntry.lineReuseDist.mean();
          lineStdDevReuseDistPrivateData << std::make_pair(theReuseMapEntry.lineReuseDist.mean(),
                                         theReuseMapEntry.lineReuseDist.stdDev());

          avgReuseDistL2RefsPrivateData << std::make_pair( theReuseMapEntry.lineReuseDist.mean(),
                                        theReuseMapEntry.numBlockRefs );
        } else {
          theBlockCountPrivateFetch++;
          if (theReuseMapEntry.touchedManyFlag == false) {
            theBlockCountTouchedOnceFetch++;
          }
          avgReuseDistPrivateFetch << theReuseMapEntry.lineReuseDist.mean();
          lineStdDevReuseDistPrivateFetch << std::make_pair(theReuseMapEntry.lineReuseDist.mean(),
                                          theReuseMapEntry.lineReuseDist.stdDev());

          avgReuseDistL2RefsPrivateFetch << std::make_pair( theReuseMapEntry.lineReuseDist.mean(),
                                         theReuseMapEntry.numBlockRefs );
        }
      }

      // RO block
      else if (theReuseMapEntry.RWFlag == false) {
        theBlockCountSharedRO++;

        theNumSameBlockRefsRO << (theReuseMapEntry.numBlockRefs);
        theNumSameBlockRefsROCount += theReuseMapEntry.numBlockRefs;

        int32_t curr_sharers_count = countNumSharers(theReuseMapEntry.sharersBitmask);
        theNumSharersRO << std::make_pair( curr_sharers_count, 1 );
        theReuseMapEntry.sharersBitmask = CoreIdx2SharerBit(theReuseMapEntry.coreIdxLastRef);

        theUnusedDistRO << (distFromLastAccess);

        if (!theReuseMapEntry.istream) {
          avgReuseDistROData << theReuseMapEntry.lineReuseDist.mean();
          lineStdDevReuseDistROData << std::make_pair(theReuseMapEntry.lineReuseDist.mean(),
                                    theReuseMapEntry.lineReuseDist.stdDev());

          avgReuseDistL2RefsROData << std::make_pair( theReuseMapEntry.lineReuseDist.mean(),
                                   theReuseMapEntry.numBlockRefs );
        } else {
          theBlockCountSharedROFetch++;
          avgReuseDistROFetch << theReuseMapEntry.lineReuseDist.mean();
          lineStdDevReuseDistROFetch << std::make_pair(theReuseMapEntry.lineReuseDist.mean(),
                                     theReuseMapEntry.lineReuseDist.stdDev());

          avgReuseDistL2RefsROFetch << std::make_pair( theReuseMapEntry.lineReuseDist.mean(),
                                    theReuseMapEntry.numBlockRefs );
        }
      }

      // RW block
      else {
        theBlockCountSharedRW++;

        theNumSameBlockRefsRW << (theReuseMapEntry.numBlockRefs);
        theNumSameBlockRefsRWCount += theReuseMapEntry.numBlockRefs;

        theUnusedDistRW << (distFromLastAccess);

        if (!theReuseMapEntry.istream) {
          avgReuseDistRWData << theReuseMapEntry.lineReuseDist.mean();
          lineStdDevReuseDistRWData << std::make_pair(theReuseMapEntry.lineReuseDist.mean(),
                                    theReuseMapEntry.lineReuseDist.stdDev());

          avgReuseDistL2RefsRWData << std::make_pair( theReuseMapEntry.lineReuseDist.mean(),
                                   theReuseMapEntry.numBlockRefs );
        } else {
          theBlockCountSharedRWFetch++;
          avgReuseDistRWFetch << theReuseMapEntry.lineReuseDist.mean();
          lineStdDevReuseDistRWFetch << std::make_pair(theReuseMapEntry.lineReuseDist.mean(),
                                     theReuseMapEntry.lineReuseDist.stdDev());

          avgReuseDistL2RefsRWFetch << std::make_pair( theReuseMapEntry.lineReuseDist.mean(),
                                    theReuseMapEntry.numBlockRefs );
        }
      }
    }

    // fix the std.d. stats histogram
    for (int32_t i = 0; i < lineStdDevReuseDistPrivateData.buckets(); ++i) {
      stdDevReuseDistPrivateData << std::make_pair( i ? (1LL << (i - 1)) : 0,
                                 static_cast<int64_t>(lineStdDevReuseDistPrivateData.bucketVal(i)) );
    }
    for (int32_t i = 0; i < lineStdDevReuseDistROData.buckets(); ++i) {
      stdDevReuseDistROData << std::make_pair( i ? (1LL << (i - 1)) : 0,
                            static_cast<int64_t>(lineStdDevReuseDistROData.bucketVal(i)) );
    }
    for (int32_t i = 0; i < lineStdDevReuseDistRWData.buckets(); ++i) {
      stdDevReuseDistRWData << std::make_pair( i ? (1LL << (i - 1)) : 0,
                            static_cast<int64_t>(lineStdDevReuseDistRWData.bucketVal(i)) );
    }

    for (int32_t i = 0; i < lineStdDevReuseDistPrivateFetch.buckets(); ++i) {
      stdDevReuseDistPrivateFetch << std::make_pair( i ? (1LL << (i - 1)) : 0,
                                  static_cast<int64_t>(lineStdDevReuseDistPrivateFetch.bucketVal(i)) );
    }
    for (int32_t i = 0; i < lineStdDevReuseDistROFetch.buckets(); ++i) {
      stdDevReuseDistROFetch << std::make_pair( i ? (1LL << (i - 1)) : 0,
                             static_cast<int64_t>(lineStdDevReuseDistROFetch.bucketVal(i)) );
    }
    for (int32_t i = 0; i < lineStdDevReuseDistRWFetch.buckets(); ++i) {
      stdDevReuseDistRWFetch << std::make_pair( i ? (1LL << (i - 1)) : 0,
                             static_cast<int64_t>(lineStdDevReuseDistRWFetch.bucketVal(i)) );
    }

    // begin new window
    theCurrReuseWinCycles = theFlexus->cycleCount();
  }

  // Insert an entry in the reuse map for the referenced block.
  void insertReuseMapEntry( const MemoryMessage & aMessage,
                            const block_address_t addr,         // the block address of the reference
                            const int16_t           coreIdx,      // the index of the core that issued the reference
                            const bool            isDCache,     // differentiate icache from dcache references
                            const bool            isWrite) {    // true if this is the effective address of a store
    tReuseMapEntry theReuseMapEntry;
    theReuseMapEntry.lastAccessRefCount = theCurrRefCount;
    theReuseMapEntry.dirtySharedRefCount = theCurrRefCount;
    theReuseMapEntry.coreA2coreBRefCount = theCurrRefCount;
    theReuseMapEntry.numBlockRefsByLastCore = 1;
    theReuseMapEntry.numBlockRefs = 1;
    theReuseMapEntry.sharersBitmask = CoreIdx2SharerBit(coreIdx);
    theReuseMapEntry.coreIdxLastRef = coreIdx;
    theReuseMapEntry.touchedManyFlag = false;
    theReuseMapEntry.sharedFlag = false;

    if ( isDCache ) {
      theReuseMapEntry.istream = false;
    } else {
      theReuseMapEntry.istream = true;
    }

    if ( isWrite ) {
      theReuseMapEntry.sharersBitmask = 0ULL;
      theReuseMapEntry.coreIdxLastWriter = coreIdx;
      theReuseMapEntry.RWFlag = true;
      theReuseMapEntry.dirtyFlag = true;
    } else {
      theReuseMapEntry.coreIdxLastWriter = -1;
      theReuseMapEntry.RWFlag = false;
      theReuseMapEntry.dirtyFlag = false;
    }

    theReuseMapEntry.lineReuseDist.reset( );

    theReuseMap.insert(std::make_pair(addr, theReuseMapEntry));

    DBG_ ( VVerb, ( << "RefCount: " << theCurrRefCount
                    << " addr: " << addr
                    << " coreIdx: " << coreIdx
                    << " reuse distance: " << 0xffffffffffffffffULL) );
  }

  ////////////////////////
  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &, ReuseDistanceSlice & aReuseDistanceSlice ) {
    // return the reuse distance for the block in question
    if (aReuseDistanceSlice.type() == ReuseDistanceSlice::GetMeanReuseDist_Data) {
      aReuseDistanceSlice.meanReuseDist() = getMeanReuseDist(aReuseDistanceSlice.address());
      return;
    }

    // process actual memory request
    DBG_Assert(aReuseDistanceSlice.type() == ReuseDistanceSlice::ProcessMemMsg);

    MemoryMessage    &    aMessage = aReuseDistanceSlice.memMsg();
    const block_address_t addr     = blockAddress(aMessage.address());       // the block address of the reference
    const int16_t           coreIdx  = static_cast<short>(aMessage.coreIdx()); // the index of the core that issued the reference
    const bool            isDCache = aMessage.isDstream();                   // differentiate icache from dcache references

    // cat /proc/4436/status | grep VmSize

    theCurrRefCount++; // get the current  reference count

    // Within the stats window: update theNumSameBlockRefsxxx, theNumSameBlockRefsxxxCount histograms.
    // Also update the theNumSharersRO and UnusedDist histograms, and theBlockCountxxx counters.
    // Look at these particular stats ONLY at the last measurement window, never look at the "all-measurements" output
    if ( (theFlexus->cycleCount() - theCurrReuseWinCycles) >= theReuseWinSizeInCycles ) {
      finalizeWindow();
    }

    tReuseMap::iterator iterReuseMap = theReuseMap.find(addr);   // find the entry for this block
    tReuseMapEntry & theReuseMapEntry = iterReuseMap->second;

    //PC correlation of reuse addresses
    const block_address_t pc = blockAddress(aMessage.pc());      // the block address of the pc issuing this reference
    tPCCorrMap::iterator iterPCCorrMap = thePCCorrMap.find(pc);  // find the entry for the PC block in the map
    if (iterPCCorrMap == thePCCorrMap.end()) {
      tLineReuseDist thePCCorrMapEntry;
      thePCCorrMapEntry.reset( );
      thePCCorrMap.insert(std::make_pair(pc, thePCCorrMapEntry));
      iterPCCorrMap = thePCCorrMap.find(pc);
    }
    tLineReuseDist & thePCCorrMapEntry = iterPCCorrMap->second;

    // if the block is referenced again
    if (iterReuseMap != theReuseMap.end()) {

      // remember the distance since the last access, and reset the reference count for the last touch
      tRefCount distFromLastAccess = theCurrRefCount - theReuseMapEntry.lastAccessRefCount;

      // hack: per block reuse distance pdf
      /*
      switch (addr) {
        //oracle tpcc <1%
        case 0x95a70840: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0x02c01940: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0x02c01900: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0xbf0f9940: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0xbf0f99c0: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x42012a80: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0xbe432c40: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x02c36840: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0x02c06600: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0x02d58d00: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //db2v7 tpcc 40%
        case 0xea535c0: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0xe304b40: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0xe3e35c0: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0xea24b40: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0xea0b5c0: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0xea520c0: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0xea0a0c0: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0xe304b00: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0xe3e20c0: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0xea24b00: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //db2v8 tpcc <1%
        case 0x0305b600: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0x0305b5c0: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0x0305da40: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0x1129ba40: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x1129ba00: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x0ed28300: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0x02c36900: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x93ad8040: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0x93285dc0: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0xbf284740: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //zeus 1%
        case 0xe6d0600: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0xe6d05c0: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0xe6d0640: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0xdfbe380: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x30f4cc0: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x30f7a40: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0x30f71c0: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x3126800: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0xdfbe400: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0xe6d0580: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //apache 2%
        case 0xe6d0600: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0xe6d05c0: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0xe6d0640: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0xdfbe380: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x30f4cc0: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x30f7a40: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0xe6d0580: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0xe6d0680: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0xdfbe400: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0xe152cc0: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //db2v8 q1 18%
        case 0x92addf00: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0x92adda00: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0x92b4ea40: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0x92b4fc00: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x92b50e00: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x92addb80: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0x92adda80: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x92addc00: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0x92b51fc0: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0x92addb00: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //db2v8 q2 1%
        case 0x030f1b00: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0x47910e00: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0x027e1f40: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0x0305da40: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x97008e80: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x0fa30000: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0x03060400: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x07f2b100: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0x02c0a2c0: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0x9ef9c8c0: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //db2v8 q6 8%
        case 0x6480e000: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0x47916900: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0x7fcf6000: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0x7f456000: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x649e6000: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x63a7e000: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0x80596000: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x64146000: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0x7f62e000: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0x638a6000: theReuseDistanceBlock9 << (distFromLastAccess); break;

        //db2v8 q16 2%
        case 0x030f1b00: theReuseDistanceBlock0 << (distFromLastAccess); break;
        case 0x027e1f40: theReuseDistanceBlock1 << (distFromLastAccess); break;
        case 0x0305da40: theReuseDistanceBlock2 << (distFromLastAccess); break;
        case 0x02c4a2c0: theReuseDistanceBlock3 << (distFromLastAccess); break;
        case 0x92be5700: theReuseDistanceBlock4 << (distFromLastAccess); break;
        case 0x03060400: theReuseDistanceBlock5 << (distFromLastAccess); break;
        case 0x02c0a2c0: theReuseDistanceBlock6 << (distFromLastAccess); break;
        case 0x0fa30000: theReuseDistanceBlock7 << (distFromLastAccess); break;
        case 0x02c1a2c0: theReuseDistanceBlock8 << (distFromLastAccess); break;
        case 0x03060440: theReuseDistanceBlock9 << (distFromLastAccess); break;
      }
      */

      ////////////// private block
      if (isPrivateBlock(theReuseMapEntry, coreIdx)) {

        DBG_ ( VVerb, ( << "PRIVATE addr: " << std::hex << addr
                        << " cpu: " << std::dec << coreIdx
                        << " sharersBitmask: 0 "
                        << aMessage ) );

        theReuseDistPrivate << (distFromLastAccess);
        theRefPrivate++;
        if (!isDCache) {
          theReuseDistPrivateFetch << (distFromLastAccess);
          theRefPrivateFetch++;
          theReuseMapEntry.istream = true;
        }

        theReuseMapEntry.lineReuseDist.update( distFromLastAccess );
        thePCCorrMapEntry.update( distFromLastAccess ); //PC correlation of reuse addresses
      }

      ////////////// shared RO block
      else if ( isSharedROBlock(theReuseMapEntry, coreIdx, aMessage.isWrite()) ) {

        DBG_ ( VVerb, ( << "SHARED RO addr: " << std::hex << addr
                        << " cpu: " << std::dec << coreIdx
                        << " sharersBitmask: " << std::hex << theReuseMapEntry.sharersBitmask
                        << " " << aMessage ) );

        // update histograms: core transition
        if (theReuseMapEntry.coreIdxLastRef != coreIdx) {
          theReuseDistROCoreTransitions << (distFromLastAccess);
          theReuseDistROCoreLifetime << (theCurrRefCount - theReuseMapEntry.coreA2coreBRefCount);
          theNumSameBlockRefsROInCoreLifetime << (theReuseMapEntry.numBlockRefsByLastCore);
        } else {
          theReuseDistROSingleCore << (distFromLastAccess);
        }
        theRefSharedRO++;
        if (!isDCache) {
          theReuseDistROFetch << (distFromLastAccess);
          theRefSharedROFetch++;
          theReuseMapEntry.istream = true;
        }

        theReuseMapEntry.lineReuseDist.update( distFromLastAccess );
        thePCCorrMapEntry.update( distFromLastAccess ); //PC correlation of reuse addresses

        theReuseMapEntry.sharedFlag = true; // this is a shared block
      }

      ////////////// shared RW block
      else {

        DBG_Assert(isSharedRWBlock(theReuseMapEntry, coreIdx, aMessage.isWrite()));
        DBG_ ( VVerb, ( << "SHARED RW addr: " << std::hex << addr
                        << " cpu: " << std::dec << coreIdx
                        << " sharersBitmask: " << std::hex << theReuseMapEntry.sharersBitmask
                        << " " << aMessage ) );

        // update histograms: core transition
        if (theReuseMapEntry.coreIdxLastRef != coreIdx) {
          if (theReuseMapEntry.dirtyFlag) {
            theReuseDistRWDirtyCoreTransition << (distFromLastAccess);
          } else {
            theReuseDistRWSharedCoreTransition << (distFromLastAccess);
          }
          theNumSameBlockRefsRWInCoreLifetime << (theReuseMapEntry.numBlockRefsByLastCore);
          theReuseDistRWCoreLifetime << (theCurrRefCount - theReuseMapEntry.coreA2coreBRefCount);
        }
        // update histograms: dirty block and last writer accesses it
        else if (theReuseMapEntry.dirtyFlag) { // && (theReuseMapEntry.coreIdxLastWriter == coreIdx)
          theReuseDistRWWriterDirty << (distFromLastAccess);
        }
        // update histograms: dirtyShared block and someone accesses it
        else { // !(theReuseMapEntry.dirtyFlag)
          if (theReuseMapEntry.coreIdxLastWriter != coreIdx) {
            theReuseDistRWReaderShared << (distFromLastAccess);
          } else {
            theReuseDistRWWriterShared << (distFromLastAccess);
          }
        }
        // update histograms: end of dirtyshared period
        if (aMessage.isWrite() && !theReuseMapEntry.dirtyFlag) {
          theReuseDistRWDirtySharedLifetime << (theCurrRefCount - theReuseMapEntry.dirtySharedRefCount);
          int32_t curr_sharers_count = countNumSharers(theReuseMapEntry.sharersBitmask);
          theNumSharersRW << std::make_pair( curr_sharers_count, 1 );
          DBG_ ( VVerb, ( << (curr_sharers_count == 0 ? "ZERO COUNT " : " ")
                          << "END OF PERIOD addr: " << std::hex << addr
                          << " cpu: " << std::dec << coreIdx
                          << " sharersBitmask: " << std::hex << theReuseMapEntry.sharersBitmask
                          << " count: " << curr_sharers_count
                          << " " << aMessage ) );
        }
        theRefSharedRW++;
        if (!isDCache) {
          theRefSharedRWFetch++;
          theReuseMapEntry.istream = true;
        }

        theReuseMapEntry.lineReuseDist.update( distFromLastAccess );
        thePCCorrMapEntry.update( distFromLastAccess ); //PC correlation of reuse addresses

        theReuseMapEntry.sharedFlag = true; // this is a shared block
      }

      ////////////// update
      theReuseMapEntry.lastAccessRefCount = theCurrRefCount;
      theReuseMapEntry.touchedManyFlag = true;

      // update reuse map entry: core transition
      if (theReuseMapEntry.coreIdxLastRef != coreIdx) {
        theReuseMapEntry.coreIdxLastRef = coreIdx;
        theReuseMapEntry.coreA2coreBRefCount = theCurrRefCount;
        theReuseMapEntry.sharersBitmask |= CoreIdx2SharerBit(coreIdx);
        theReuseMapEntry.numBlockRefsByLastCore = 1;
        DBG_ ( VVerb, ( << "READING addr: " << std::hex << addr
                        << " cpu: " << std::dec << coreIdx
                        << " sharersBitmask: " << std::hex << theReuseMapEntry.sharersBitmask
                        << " " << aMessage ) );

        // update reuse map entry: dirty->dirtyShared transition
        if (theReuseMapEntry.dirtyFlag && !aMessage.isWrite()) {
          theReuseMapEntry.dirtySharedRefCount = theCurrRefCount;
          theReuseMapEntry.dirtyFlag = false;
        }
      } else {
        // update reuse map entry: same core
        theReuseMapEntry.numBlockRefsByLastCore++;
      }

      // update reuse map entry: write request
      if (aMessage.isWrite()) {
        theReuseMapEntry.coreIdxLastWriter = coreIdx;
        theReuseMapEntry.RWFlag = true;
        theReuseMapEntry.dirtyFlag = true;
        theReuseMapEntry.sharersBitmask = 0LL;
        DBG_ ( VVerb, ( << "WRITING addr: " << std::hex << addr
                        << " cpu: " << std::dec << coreIdx
                        << " sharersBitmask: " << std::hex << theReuseMapEntry.sharersBitmask
                        << " " << aMessage ) );
      }

      // update reuse map entry: one more reference for this block
      theReuseMapEntry.numBlockRefs++;

      DBG_ ( VVerb, ( << "RefCount: " << theCurrRefCount
                      << " addr: " << addr
                      << " reuse distance: " << distFromLastAccess) );
    }

    /////////////////// if the block is referenced for the first time
    else {
      insertReuseMapEntry( aMessage,
                           addr,                // the block address of the reference
                           coreIdx,             // the index of the core that issued the reference
                           isDCache,            // differentiate icache from dcache references
                           aMessage.isWrite());
    }

    return;
  }

  void drive( interface::UpdateStatsDrive const & )
  { }

};  // end class ReuseDistance

} // end namespace nReuseDistance

FLEXUS_COMPONENT_INSTANTIATOR( ReuseDistance, nReuseDistance);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT ReuseDistance

#define DBG_Reset
#include DBG_Control()
