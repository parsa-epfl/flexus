#include <components/PerfectPlacement/PerfectPlacement.hpp>

#include <list>
#include <iterator>

#include <core/stats.hpp>

#include <boost/bind.hpp>
#include <unordered_map>

#define DBG_DefineCategories PerfectPlacement
#define DBG_SetDefaultOps AddCat(PerfectPlacement) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT PerfectPlacement
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nPerfectPlacement {
using namespace Flexus;
using namespace Flexus::Core;

namespace Stat = Flexus::Stat;

typedef uint32_t block_address_t;

uint32_t log_base2(uint32_t num) {
  uint32_t ii = 0;
  while (num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

class FLEXUS_COMPONENT(PerfectPlacement) {
  FLEXUS_COMPONENT_IMPL( PerfectPlacement );

  //perfect placement/replacement in a traditional cache
  typedef struct cacheEntry_t {
    block_address_t theBlockAddress;
    bool            theCachedFlag;
    bool            isWritable;           //used to detect upgrade misses
    bool            isValid;              //used to detect coherence misses
    uint32_t   theNumCachedBlocks;
  } cacheEntry_t;
  typedef std::list<cacheEntry_t> cacheRefList_t;

  //private variables
  int32_t                theSetShift;
  int32_t                theTagShift;
  block_address_t    theSetMask;
  block_address_t    theBlockMask;
  uint32_t      theAssociativity;
  bool               thePerfectReplacementOnlyFlag;
  cacheRefList_t  ** theCacheRefList;

  //stats
  Stat::StatCounter theCacheRefs_stat;
  Stat::StatCounter theCacheHits_stat;
  Stat::StatCounter theCacheCapMissesFetch_stat;
  Stat::StatCounter theCacheCapMissesRead_stat;
  Stat::StatCounter theCacheCapMissesWrite_stat;
  Stat::StatCounter theCacheCohMissesFetch_stat;
  Stat::StatCounter theCacheCohMissesRead_stat;
  Stat::StatCounter theCacheCohMissesWrite_stat;
  Stat::StatCounter theCacheUpgMisses_stat;

  int64_t theCacheRefs;
  int64_t theCacheHits;
  int64_t theCacheCapMissesFetch;
  int64_t theCacheCapMissesRead;
  int64_t theCacheCapMissesWrite;
  int64_t theCacheCohMissesFetch;
  int64_t theCacheCohMissesRead;
  int64_t theCacheCohMissesWrite;
  int64_t theCacheUpgMisses;

private:
  inline block_address_t blockAddress(const PhysicalMemoryAddress addr) const {
    return (addr & theBlockMask);
  }

  //locate a cache entry in the reference list
  cacheRefList_t::iterator findCacheEntry(const int32_t aSet, const block_address_t aBlockAddress) const {
    cacheRefList_t::iterator iter;
    for (iter = theCacheRefList[aSet]->begin(); iter != theCacheRefList[aSet]->end(); iter++) {
      if (iter->theBlockAddress == aBlockAddress) {
        break;
      }
    }

    return iter;
  }

  void update( void ) {
    theCacheRefs_stat += theCacheRefs;
    theCacheHits_stat += theCacheHits;
    theCacheCapMissesFetch_stat += theCacheCapMissesFetch;
    theCacheCapMissesRead_stat  += theCacheCapMissesRead;
    theCacheCapMissesWrite_stat += theCacheCapMissesWrite;
    theCacheCohMissesFetch_stat += theCacheCohMissesFetch;
    theCacheCohMissesRead_stat  += theCacheCohMissesRead;
    theCacheCohMissesWrite_stat += theCacheCohMissesWrite;
    theCacheUpgMisses_stat      += theCacheUpgMisses;

    theCacheRefs = 0;
    theCacheHits = 0;
    theCacheCapMissesFetch = 0;
    theCacheCapMissesRead  = 0;
    theCacheCapMissesWrite = 0;
    theCacheCohMissesFetch = 0;
    theCacheCohMissesRead  = 0;
    theCacheCohMissesWrite = 0;
    theCacheUpgMisses      = 0;
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(PerfectPlacement)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theSetShift(0)
    , theTagShift(0)
    , theSetMask(0)
    , theBlockMask(0)
    , theAssociativity(0)
    , thePerfectReplacementOnlyFlag(true)
    , theCacheRefs_stat(statName() + "-CacheRefs")
    , theCacheHits_stat(statName() + "-CacheHits")
    , theCacheCapMissesFetch_stat(statName() + "-CacheMisses:Cap:Fetch")
    , theCacheCapMissesRead_stat(statName() + "-CacheMisses:Cap:Read")
    , theCacheCapMissesWrite_stat(statName() + "-CacheMisses:Cap:Write")
    , theCacheCohMissesFetch_stat(statName() + "-CacheMisses:Coh:Fetch")
    , theCacheCohMissesRead_stat(statName() + "-CacheMisses:Coh:Read")
    , theCacheCohMissesWrite_stat(statName() + "-CacheMisses:Coh:Write")
    , theCacheUpgMisses_stat(statName() + "-CacheMisses:Upg") {
    theCacheRefs = 0;
    theCacheHits = 0;
    theCacheCapMissesFetch = 0;
    theCacheCapMissesRead = 0;
    theCacheCapMissesWrite = 0;
    theCacheCohMissesFetch = 0;
    theCacheCohMissesRead = 0;
    theCacheCohMissesWrite = 0;
    theCacheUpgMisses = 0;
  }

  void finalize( void ) {
    update();
  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize( void ) {
    //Confirm that BlockSize is a power of 2
    DBG_Assert( (cfg.BlockSize & (cfg.BlockSize - 1)) == 0);
    DBG_Assert( cfg.BlockSize  >= 4);

    int32_t num_sets = cfg.Size / cfg.BlockSize / cfg.Associativity;

    //Confirm that num_sets is a power of 2
    DBG_Assert( (num_sets & (num_sets  - 1)) == 0);

    //Confirm that settings are consistent
    DBG_Assert( cfg.BlockSize * num_sets * cfg.Associativity == cfg.Size);

    //Calculate masks and shifts
    theSetShift = log_base2( cfg.BlockSize ) ;
    theTagShift = log_base2( num_sets ) ;
    theSetMask = ( cfg.BlockSize * num_sets - 1 );
    theBlockMask = ~(cfg.BlockSize - 1);

    theAssociativity = cfg.Associativity;
    thePerfectReplacementOnlyFlag = cfg.PerfReplOnly;

    //Allocate the cache reference list array (one list per set)
    // theCacheRefList = (cacheRefList_t **) new uint8_t(sizeof(cacheRefList_t *) * num_sets);
    int64_t array_size = sizeof(cacheRefList_t *) * num_sets;
    uint8_t * theCacheRefList_base =  new uint8_t [ array_size + 128];
    theCacheRefList = reinterpret_cast<cacheRefList_t **>
                      ( (reinterpret_cast<uint32_t>(theCacheRefList_base) + 127UL) & (~127UL) ); //128-byte align
    for (int32_t i = 0; i < num_sets; i++) {
      theCacheRefList[i] = new cacheRefList_t;
    }

    Stat::getStatManager()->addFinalizer( boost::lambda::bind( &nPerfectPlacement::PerfectPlacementComponent::finalize, this ) );
  }

  void allocate( const MemoryMessage & aMessage ) {
    uint32_t aBlockAddress = blockAddress(aMessage.address());

    uint32_t set_bits = (aBlockAddress & theSetMask);
    uint32_t set = (set_bits) >> theSetShift;

    // add the last ref in the ref list and make it not cached
    cacheEntry_t aCacheEntry = { aBlockAddress,
                                 /* isCached */ false,
                                 /* isWritable */ false, //will be fixed by processWritableBlock()
                                 /* isValid */ true,
                                 /* theNumCachedBlocks */ (thePerfectReplacementOnlyFlag ? 1 : 0)
                               };
    theCacheRefList[set]->push_front(aCacheEntry);
    if (theCacheRefList[set]->begin()->theNumCachedBlocks == theAssociativity) {
      cacheRefList_t::iterator iter = theCacheRefList[set]->begin();
      iter++;
      theCacheRefList[set]->erase(iter, theCacheRefList[set]->end());
    }
  }

  void processRequest( const MemoryMessage & aMessage ) {
    uint32_t aBlockAddress = blockAddress(aMessage.address());

    uint32_t set_bits = (aBlockAddress & theSetMask);
    uint32_t set = (set_bits) >> theSetShift;

    theCacheRefs++;
    cacheRefList_t::iterator iterHit = findCacheEntry(set, aBlockAddress);

    // found the entry
    if (iterHit != theCacheRefList[set]->end()) {

      //coherence miss
      if (!iterHit->isValid) {
        if (aMessage.isWrite()) {
          theCacheCohMissesWrite ++;
        } else if (aMessage.isDstream()) {
          theCacheCohMissesRead ++;
        } else {
          theCacheCohMissesFetch ++;
        }
        allocate(aMessage);
      }

      // upgrade miss
      else if (aMessage.isWrite() && !iterHit->isWritable) {
        theCacheUpgMisses++;

        //will be fixed by processWritableBlock()
        // iterHit->isWritable = true; // make it writable
      }

      // cache hit
      else {
        theCacheHits++;
      }

      cacheRefList_t::iterator iter;
      for (iter = theCacheRefList[set]->begin(); iter != iterHit; iter++) {
        //account for one more cached block for the window in which the cache line is live
        iter->theNumCachedBlocks ++;
        if (iter->theNumCachedBlocks == theAssociativity) {
          break; //the cache is full
        }
      }
      //fix the cached flag and the cached entries count in the beginning of the window
      if (iter == iterHit) {
        if (iter->theCachedFlag == false) {
          iter->theCachedFlag = true;
          iter->theNumCachedBlocks ++;
        }
      }
      //prune the reference list
      if (iter->theNumCachedBlocks == theAssociativity) {
        theCacheRefList[set]->erase(++iter, theCacheRefList[set]->end());
      }

      // add the last ref in the ref list and make it cached
      cacheEntry_t aCacheEntry = { aBlockAddress,
                                   /* isCached */ true,
                                   /* isWritable */ false, //will be fixed by processWritableBlock()
                                   /* isValid */ true,
                                   /* theNumCachedBlocks */ 1
                                 };
      theCacheRefList[set]->push_front(aCacheEntry);
    }

    // capacity miss
    else {
      if (aMessage.isWrite()) {
        theCacheCapMissesWrite ++;
      } else if (aMessage.isDstream()) {
        theCacheCapMissesRead ++;
      } else {
        theCacheCapMissesFetch ++;
      }
      allocate(aMessage);
    }
  }

  void processInvalidate( const MemoryMessage & aMessage ) {
    uint32_t aBlockAddress = blockAddress(aMessage.address());

    uint32_t set_bits = (aBlockAddress & theSetMask);
    uint32_t set = (set_bits) >> theSetShift;

    cacheRefList_t::iterator iterHit = findCacheEntry(set, aBlockAddress);

    // cache hit
    if (iterHit != theCacheRefList[set]->end()) {
      iterHit->isValid = false;
    }
  }

  void processDowngrade( const MemoryMessage & aMessage ) {
    uint32_t aBlockAddress = blockAddress(aMessage.address());

    uint32_t set_bits = (aBlockAddress & theSetMask);
    uint32_t set = (set_bits) >> theSetShift;

    cacheRefList_t::iterator iterHit = findCacheEntry(set, aBlockAddress);

    // cache hit
    if (iterHit != theCacheRefList[set]->end()) {
      iterHit->isWritable = false;
    }
  }

  void processWritableBlock( const PhysicalMemoryAddress anAddress ) {
    uint32_t aBlockAddress = blockAddress(anAddress);

    uint32_t set_bits = (aBlockAddress & theSetMask);
    uint32_t set = (set_bits) >> theSetShift;

    cacheRefList_t::iterator iterHit = findCacheEntry(set, aBlockAddress);

    // DBG_Assert(iterHit == theCacheRefList[set]->begin());
    iterHit->isWritable = true;
  }

  ////////////////////////
  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &, PerfectPlacementSlice & aSlice ) {
    if (aSlice.type() == PerfectPlacementSlice::MakeBlockWritable) {
      processWritableBlock(aSlice.address());
    } else {
      DBG_Assert(aSlice.type() == PerfectPlacementSlice::ProcessMsg);

      MemoryMessage & aMessage = aSlice.memMsg();

      switch (aMessage.type()) {
        case MemoryMessage::FetchReq:
        case MemoryMessage::LoadReq:
        case MemoryMessage::PrefetchReadAllocReq:
        case MemoryMessage::PrefetchReadNoAllocReq:
        case MemoryMessage::StoreReq:
        case MemoryMessage::StorePrefetchReq:
        case MemoryMessage::RMWReq:
        case MemoryMessage::CmpxReq:
        case MemoryMessage::ReadReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
          processRequest(aMessage);
          break;

        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
        case MemoryMessage::PrefetchInsert:
          allocate(aMessage);
          break;

        case MemoryMessage::Invalidate:
          processInvalidate(aMessage);
          break;

        case MemoryMessage::Downgrade:
          processDowngrade(aMessage);
          break;

        case MemoryMessage::ReturnReq:
          //do nothing
          break;

        default:
          DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
      }
    }
  }

  void drive( interface::UpdateStatsDrive const & ) {
    update();
  }

};  // end class PerfectPlacement

} // end namespace nPerfectPlacement

FLEXUS_COMPONENT_INSTANTIATOR( PerfectPlacement, nPerfectPlacement);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PerfectPlacement

#define DBG_Reset
#include DBG_Control()
