#ifndef FLEXUS_CMP_CACHE_RTARRAY_HPP_INCLUDED
#define FLEXUS_CMP_CACHE_RTARRAY_HPP_INCLUDED

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/CMPCache/AbstractArray.hpp>

#include <cstring>
#include <list>

#include <boost/intrusive_ptr.hpp>
#include <boost/function.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/export.hpp>

#include <components/Common/RegionTrackerCoordinator.hpp>

#include <components/Common/Util.hpp>
using nCommonUtil::log_base2;

using namespace boost::multi_index;

using std::list;
using std::string;
using std::pair;

namespace nCMPCache {

struct RTSerializer {
  uint64_t tag;
  uint8_t  state;
  int8_t  owner;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & tag;
    ar & state;
    ar & owner;
  }
};

struct BlockSerializer {
  uint64_t tag;
  uint8_t  state;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & tag;
    ar & state;
  }
};

template<typename _BState, const _BState & _DefaultBState>
class RTArray : public AbstractArray<_BState> {
public:

  static const uint64_t kStateMask = 3;

  enum RegionState {
    NON_SHARED_REGION,
    PARTIAL_SHARED_REGION,
    SHARED_REGION
  };

  // RegionTracker (RVA) entry structure
  struct RTEntry {
    RTEntry(int32_t blocksPerRegion) : region_state(SHARED_REGION), owner(-1),
      state(blocksPerRegion, _DefaultBState), ways(blocksPerRegion, 0), shared(blocksPerRegion) {}

    RTEntry(int32_t blocksPerRegion, uint64_t tag, int32_t way)
      : tag(tag), way(way), region_state(SHARED_REGION), owner(-1),
        state(blocksPerRegion, _DefaultBState), ways(blocksPerRegion, 0), shared(blocksPerRegion) {}

    RTEntry(int32_t blocksPerRegion, uint64_t tag, int32_t way, RegionState rstate, int32_t owner)
      : tag(tag), way(way), region_state(rstate), owner(owner),
        state(blocksPerRegion, _DefaultBState), ways(blocksPerRegion, 0), shared(blocksPerRegion) {}

    uint64_t tag;
    int32_t way;
    RegionState region_state;
    int32_t owner;

    std::vector<_BState> state;
    std::vector<uint16_t> ways;
    boost::dynamic_bitset<> shared;

    bool hasCachedBlocks() const {
      for (uint32_t i = 0; i < state.size(); i++) {
        // If we have a valid copy, or if a higher level has it cached
        if (isValid(state[i])) return true;
      }
      return false;
    }

    uint64_t validVector() const {
      uint64_t ret = 0;
      uint64_t mask = 1;
      for (uint32_t i = 0; i < state.size(); i++, mask <<= 1) {
        // If we have a valid copy, or if a higher level has it cached
        if (isValid(state[i])) {
          ret |= mask;
        }
      }
      return ret;
    }
    void getPresence(boost::dynamic_bitset<> &present) const {
      for (uint32_t i = 0; i < state.size(); i++) {
        present[i] = state[i].isValid();
      }
    }
  };

  // Modifier Objects for updating RTEntry objects in multi-index containers

  struct ChangeRTState {
    ChangeRTState(int32_t offset, const _BState & state) : offset(offset), state(state) {}
    void operator()(RTEntry & entry) {
      //DBG_(Verb, Addr(entry.tag | (offset << 6)) ( << theName << " Changing state of block " << std::hex << (entry.tag|(offset<<6)) << " from " << entry.state[offset] << " to " << state << std::dec ));
      entry.state[offset] = state;
    }
  private:
    int32_t offset;
    const _BState & state;
  };
  struct ChangeRTProtected {
    ChangeRTProtected(int32_t offset, bool val) : offset(offset), val(val) {}
    void operator()(RTEntry & entry) {
      entry.state[offset].setProtected(val);
    }
  private:
    int32_t offset;
    bool val;
  };
  struct ChangeRTPrefetched {
    ChangeRTPrefetched(int32_t offset, bool val) : offset(offset), val(val) {}
    void operator()(RTEntry & entry) {
      entry.state[offset].setPrefetched(val);
    }
  private:
    int32_t offset;
    bool val;
  };

  struct ChangeRTLocked {
    ChangeRTLocked(int32_t offset, bool val) :
      offset(offset), val(val) {}
    void operator()(RTEntry& entry) {
      entry.state[offset].setLocked(val);
    }
  private:
    int32_t offset;
    bool val;
  };

  struct InvalidateRTState {
    InvalidateRTState(int32_t offset) : offset(offset) {}
    void operator()(RTEntry & entry) {
      // Only keep the cached bit
      entry.state[offset] = _DefaultBState;
    }
  private:
    int32_t offset;
  };

  struct ChangeRegionState {
    ChangeRegionState(const RegionState & state) : state(state) {}
    void operator()(RTEntry & entry) {
      entry.region_state = state;
    }
  private:
    RegionState state;
  };

  struct ChangeRegionOwner {
    ChangeRegionOwner(const int32_t & owner) : owner(owner) {}
    void operator()(RTEntry & entry) {
      entry.owner = owner;
    }
  private:
    int32_t owner;
  };

  struct CorrectRTWay {
    CorrectRTWay(int32_t offset, uint16_t way) : offset(offset), way(way) {}
    void operator()(RTEntry & entry) {
      entry.ways[offset] = way;
    }
  private:
    int32_t offset;
    uint16_t way;
  };

  struct AllocateBlock {
    AllocateBlock(int32_t offset, const _BState & state, uint16_t way) : offset(offset), state(state), way(way) {}
    void operator()(RTEntry & entry) {
      entry.state[offset] = state;
      entry.ways[offset]  = way;
    }
  private:
    int32_t offset;
    const _BState & state;
    uint16_t way;
  };

  struct ChangeSharedBlocks {
    ChangeSharedBlocks(const uint32_t & shared) : shared(shared) {}
    void operator()(RTEntry & entry) {
      entry.shared = boost::dynamic_bitset<>(entry.state.size(), shared);
    }
  private:
    uint32_t shared;
  };

  struct SetSharedBlock {
    SetSharedBlock(const int32_t & offset) : offset(offset) {}
    void operator()(RTEntry & entry) {
      entry.shared[offset] = true;
    }
  private:
    int32_t offset;
  };

  struct by_order {};
  struct by_tag {};
  struct by_way {};

  struct ULLHash {
    const std::size_t operator()(uint64_t x) const {
      return (std::size_t)x;
    }
  };

  // Structure to mimic traditional tag array (BST like structure)
  struct BlockEntry {
    uint64_t tag;
    mutable _BState  state;
    uint16_t  way;

    BlockEntry(uint64_t t, _BState s, uint16_t w)
      : tag(t), state(s), way(w) {}
  };

  // Modifier Objects to update BlockEntry objects in multi-index containers

  struct ChangeBlockState {
    ChangeBlockState(const _BState & state) : state(state) {}
    void operator()(BlockEntry & entry) {
      //DBG_(Verb, Addr(entry.tag) ( << theName << " Changing state of block " << std::hex << entry.tag << " from " << entry.state << " to " << state << std::dec ));
      entry.state = state;
    }
  private:
    const _BState & state;
  };

  struct ChangeBlockProtected {
    ChangeBlockProtected(const bool val) : val(val) {}
    void operator()(BlockEntry & entry) {
      entry.state.setProtected(val);
    }
  private:
    const bool val;
  };
  struct ChangeBlockPrefetched {
    ChangeBlockPrefetched(const bool val) : val(val) {}
    void operator()(BlockEntry & entry) {
      entry.state.setPrefetched(val);
    }
  private:
    const bool val;
  };

  struct ChangeBlockLocked {
    ChangeBlockLocked(const bool val) : val(val) {}
    void operator()(BlockEntry& entry) {
      entry.state.setLocked(val);
    }
  private:
    const bool val;
  };

  struct ReplaceBlock {
    ReplaceBlock(uint64_t tag, const _BState & state) : tag(tag), state(state) {}
    void operator()(BlockEntry & entry) {
      //DBG_(Verb, Addr(entry.tag) ( << theName << " Replacing block " << std::hex << entry.tag << " (" << entry.state << ") with " << tag << " (" << state << ")" << std::dec ));
      entry.state = state;
      entry.tag = tag;
    }
  private:
    uint64_t  tag;
    const _BState & state;
  };

  typedef multi_index_container
  < RTEntry
  , indexed_by
  < sequenced < tag<by_order> >
  , hashed_unique
  < tag<by_tag>
  , member<RTEntry, uint64_t, &RTEntry::tag>
  , ULLHash
  >
  >
  >
  rt_set_t;

  typedef typename rt_set_t::template nth_index<0>::type::iterator rt_order_iterator;
  typedef typename rt_set_t::template nth_index<1>::type::iterator rt_iterator;
  typedef typename rt_set_t::template nth_index<1>::type    rt_index;

  typedef multi_index_container
  < BlockEntry
  , indexed_by
  < sequenced < tag<by_order> >
  , hashed_unique
  < tag<by_tag>
  , member<BlockEntry, uint64_t, &BlockEntry::tag>
  , ULLHash
  >
  , hashed_unique
  < tag<by_way>
  , member<BlockEntry, uint16_t, &BlockEntry::way>
  >
  >
  >
  block_set_t;

  typedef typename block_set_t::template nth_index<0>::type::iterator order_iterator;
  typedef typename block_set_t::template nth_index<1>::type::iterator tag_iterator;
  typedef typename block_set_t::template nth_index<2>::type::iterator way_iterator;

  typedef typename block_set_t::template nth_index<2>::type    way_index;

  class RTLookupResult : public AbstractArrayLookupResult<_BState> {
  public:
    RTArray  * rt_cache;
    rt_set_t * rt_set;
    block_set_t * block_set;
    rt_iterator  region;
    way_iterator block;
    int    offset;
    uint64_t tagset;
    bool   block_allocated;
    bool   region_allocated;
    _BState   orig_state;

    RTLookupResult() : rt_cache(nullptr), block_allocated(false), region_allocated(false), orig_state(_DefaultBState) {}
    RTLookupResult(bool rvalid, bool bvalid, RTArray<_BState, _DefaultBState> *rtc, uint64_t addr)
      : rt_cache(rtc), tagset(addr), block_allocated(bvalid), region_allocated(rvalid), orig_state(_DefaultBState) {}

    friend class RTArray;

    virtual void changeState(_BState new_state, bool make_MRU, bool make_LRU) {
      rt_cache->updateState(*this, new_state, make_MRU, make_LRU);
    }

    virtual bool updateLRU() {
      region = rt_cache->update_lru(rt_set, region);
      block = rt_cache->update_lru(block_set, block);
      return true;
    }

    virtual const _BState & state( void ) const {
      return (block_allocated ? block->state : orig_state);
    }

    virtual bool setState( const _BState & aNewState ) {
      if (block->state != aNewState) {
        rt_cache->updateState(*this, aNewState, false, false);
        return true;
      }
      return false;
    }

    virtual void setProtected( bool val) {
      rt_cache->setProtected(*this, val);
    }
    virtual void setPrefetched( bool val) {
      rt_cache->setPrefetched(*this, val);
    }

    virtual bool hit  ( void ) const {
      return (block_allocated && block->state.isValid());
    }
    virtual bool miss ( void ) const {
      return (!region_allocated || !block_allocated || !block->state.isValid());
    }
    virtual bool found( void ) const {
      return region_allocated;
    }
    virtual bool valid( void ) const {
      return block_allocated;
    }

    virtual MemoryAddress blockAddress ( void ) const {
      return MemoryAddress(tagset);
    }

    virtual bool isProtected() const {
      return (block_allocated ? block->state.isProtected() : false);
    }

    virtual void setLocked(bool val) {
      rt_cache->setLocked(*this, val);
    }

  };

  friend class RTLookupResult;

  typedef AbstractArrayLookupResult<_BState> AbstractArrayLookup;
  typedef typename boost::intrusive_ptr<AbstractArrayLookup> AbstractArrayLookup_p;
  typedef typename boost::intrusive_ptr<RTLookupResult> LookupResult_p;

  enum RTReplPolicy_t { SET_LRU, REGION_LRU };

  typedef boost::function<void (MemoryAddress, int, bool)> observer_func_t;
  typedef std::list<observer_func_t> observer_list_t;

private:
  // Private data
  std::string theName;
  int32_t theNodeId;

  bool theTraceTrackerFlag;

  int32_t theBlockSize;
  int32_t theRegionSize;
  int32_t theNumDataSets;
  int32_t theAssociativity;

  int32_t theNumRTSets;
  int32_t theRTAssociativity;

  int32_t theERBSize;
  int32_t theERBReserve;
  int32_t theERBThreshold;

  int32_t theOrigERBSets;
  int32_t theOrigERBAssoc;

  bool theBlockScout;
  bool theImpreciseRS;

  int32_t theBanks;
  int32_t theTotalBanks;
  int32_t theBankInterleaving;
  int32_t theLocalBankIndex;
  int32_t theGlobalBankIndex;
  int32_t theGroups;
  int32_t theGroupInterleaving;
  int32_t theGroupIndex;

  RTReplPolicy_t theReplPolicy;

  uint64_t theEvictMask;

  block_set_t * theBlocks;
  rt_set_t * theRVA;
  rt_set_t theERB;

  int32_t regionShift, rtSetMidShift, rtSetHighShift;
  uint64_t rtSetLowMask;
  uint64_t rtSetMidMask;
  uint64_t rtSetHighMask;

  uint64_t blockOffsetMask;
  int32_t blockShift, blockSetMidShift, blockSetHighShift;
  uint64_t blockSetLowMask;
  uint64_t blockSetMidMask;
  uint64_t blockSetHighMask;

  uint64_t blockTagMask;
  uint64_t rtTagMask;

  int32_t blockTagShift;
  int32_t theBlocksPerRegion;

  uint64_t rtGlobalSetMask;
  uint64_t globalSetMask;

  int32_t theBankMask, theBankShift;
  int32_t theGroupMask, theGroupShift;

  observer_list_t theAllocObserverList;
  observer_list_t theEvictObserverList;

  inline uint64_t get_rt_set(uint64_t addr) {
    return ((addr >> regionShift) & rtSetLowMask) | ((addr >> rtSetMidShift) & rtSetMidMask) | ((addr >> rtSetHighShift) & rtSetHighMask);
  }

  inline uint64_t get_rt_tag(uint64_t addr) {
    return (addr & rtTagMask);
  }

  inline uint64_t get_block_offset(uint64_t addr) {
    return ( (addr >> blockShift) & blockOffsetMask );
  }

  inline uint64_t get_block_set(uint64_t addr, uint64_t way) {
    return ((addr >> blockShift) & blockSetLowMask) | ((addr >> blockSetMidShift) & blockSetMidMask) | ((addr >> blockSetHighShift) & blockSetHighMask);
  }

  inline uint64_t get_block_tag(uint64_t addr) {
    return (addr & blockTagMask);
  }

  inline int32_t getBankIndex(uint64_t addr) {
    return ((addr >> theBankShift) & theBankMask);
  }

  inline int32_t getGroupIndex(uint64_t addr) {
    return ((addr >> theGroupShift) & theGroupMask);
  }

public:

  inline uint64_t getRegionNumber(MemoryAddress addr) {
    return (addr >> regionShift);
  }

  inline MemoryAddress getFirstBlockAddr(MemoryAddress addr) {
    return MemoryAddress((addr >> regionShift) << regionShift);
  }

  inline int32_t getBlocksPerRegion() {
    return theBlocksPerRegion;
  }

  static RTReplPolicy_t string2ReplPolicy(const std::string & policy) {
    if (policy == "RegionLRU") {
      return REGION_LRU;
    } else if (policy == "SetLRU") {
      return SET_LRU;
    } else {
      DBG_(Crit, ( << "Unknown RTCache Replacement Policy '" << policy << "'" ));
    }
    return SET_LRU;
  }

  RTArray(CMPCacheInfo & anInfo, int32_t aBlockSize, const list<pair<string, string> > &args)
    : theName(anInfo.theName)
    , theNodeId(anInfo.theNodeId)
    , theBlockSize(aBlockSize)
    , theRegionSize(-1)
    , theNumDataSets(-1)
    , theAssociativity(-1)
    , theNumRTSets(-1)
    , theRTAssociativity(-1)

    , theERBSize(-1)
    , theERBReserve(-1)
    , theERBThreshold(1)
    , theBlockScout(false)
    , theImpreciseRS(false)
    , theBanks(anInfo.theNumBanks)
    , theBankInterleaving(anInfo.theBankInterleaving)
    , theGroups(anInfo.theNumGroups)
    , theGroupInterleaving(anInfo.theGroupInterleaving) {
    theTotalBanks = theBanks * theGroups;
    theGlobalBankIndex = anInfo.theNodeId;
    theLocalBankIndex = anInfo.theNodeId % theBanks;
    theGroupIndex = anInfo.theNodeId / theBanks;

    list<pair<string, string> >::const_iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      DBG_(Iface, ( << "RTArray parsing option '" << iter->first << "' = '" << iter->second << "'" ));
      if (strcasecmp(iter->first.c_str(), "sets") == 0) {
        theNumDataSets = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "total_sets") == 0 || strcasecmp(iter->first.c_str(), "global_sets") == 0) {
        int32_t global_sets = strtol(iter->second.c_str(), nullptr, 0);
        theNumDataSets = global_sets / theTotalBanks;
        DBG_Assert( (theNumDataSets * theTotalBanks) == global_sets, ( << "global_sets (" << global_sets << ") is not divisible by number of banks (" << theTotalBanks << ")" ));
      } else if ((strcasecmp(iter->first.c_str(), "assoc") == 0)
                 || (strcasecmp(iter->first.c_str(), "associativity") == 0)) {
        theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "rsize") == 0)
                 || (strcasecmp(iter->first.c_str(), "region_size") == 0)) {
        theRegionSize = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "rtsets") == 0)
                 || (strcasecmp(iter->first.c_str(), "rt_sets") == 0)) {
        theNumRTSets = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "total_rtsets") == 0)
                 || (strcasecmp(iter->first.c_str(), "total_rt_sets") == 0)
                 || (strcasecmp(iter->first.c_str(), "global_rtsets") == 0)
                 || (strcasecmp(iter->first.c_str(), "global_rt_sets") == 0)) {
        int32_t global_sets = strtol(iter->second.c_str(), nullptr, 0);
        theNumRTSets = global_sets / theTotalBanks;
        DBG_Assert( (theNumRTSets * theTotalBanks) == global_sets, ( << "global_rt_sets (" << global_sets << ") "
                    << "is not divisible by number of banks (" << theTotalBanks << ")" ));
      } else if ((strcasecmp(iter->first.c_str(), "rtassoc") == 0)
                 || (strcasecmp(iter->first.c_str(), "rt_assoc") == 0)
                 || (strcasecmp(iter->first.c_str(), "rtassociativity") == 0)
                 || (strcasecmp(iter->first.c_str(), "rt_associativity") == 0)) {
        theRTAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "repl") == 0)
                 || (strcasecmp(iter->first.c_str(), "replacement") == 0)) {
        theReplPolicy = string2ReplPolicy(iter->second);
      } else if ((strcasecmp(iter->first.c_str(), "erb") == 0)
                 || (strcasecmp(iter->first.c_str(), "erb_size") == 0)) {
        theERBSize = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "orig_erb_sets") == 0)) {
        theOrigERBSets = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "orig_erb_assoc") == 0)) {
        theOrigERBAssoc = strtol(iter->second.c_str(), nullptr, 0);
      } else if ((strcasecmp(iter->first.c_str(), "erb_threshold") == 0)
                 || (strcasecmp(iter->first.c_str(), "erbthreshold") == 0)) {
        theERBThreshold = strtol(iter->second.c_str(), nullptr, 0);
      } else {
        DBG_Assert(false, ( << "RTArray received invalid option '" << iter->first << "' = '" << iter->second << "'" ));
      }
    }
    init();
  }
  virtual ~RTArray() {
    delete [] theBlocks;
    delete [] theRVA;
  }

  void init() {
    DBG_(Dev, ( << theName << " - Creating RT Array: Bsize = " << theBlockSize << ", RSize = " << theRegionSize << ", Sets = " << theNumDataSets << ", Assoc = " << theAssociativity << ", RTSets = " << theNumRTSets << ", RTAssoc = " << theRTAssociativity << ", ERBSize = " << theERBSize ));

    DBG_Assert( theNumDataSets > 0 );
    DBG_Assert( theNumRTSets > 0 );
    DBG_Assert( theAssociativity > 0 );
    DBG_Assert( theRTAssociativity > 0 );
    DBG_Assert( theRegionSize > theBlockSize );
    DBG_Assert( theERBSize > 0 );
    DBG_Assert( theERBThreshold > 0 );

    theBlocks = new block_set_t[theNumDataSets];

    theBlocksPerRegion = theRegionSize / theBlockSize;

    DBG_Assert ( (theNumDataSets / theBlocksPerRegion) <= theNumRTSets, ( << "DataSets = " << theNumDataSets << ", RTSets = " << theNumRTSets << ", BlocksPerRegion = " << theBlocksPerRegion ));

    // Make sure everything is a power of 2
    DBG_Assert( ((theNumDataSets - 1) & theNumDataSets) == 0 );
    DBG_Assert( ((theNumRTSets - 1) & theNumRTSets) == 0 );
    DBG_Assert( ((theRegionSize - 1) & theRegionSize) == 0 );
    DBG_Assert( ((theBlockSize - 1) & theBlockSize) == 0 );

    DBG_Assert( ((theBanks - 1) & theBanks) == 0);
    DBG_Assert( ((theBankInterleaving - 1) & theBankInterleaving) == 0);
    DBG_Assert( theBankInterleaving >= theRegionSize );
    DBG_Assert( ((theGroups - 1) & theGroups) == 0);
    DBG_Assert( ((theGroupInterleaving - 1) & theGroupInterleaving) == 0);
    DBG_Assert( theGroupInterleaving >= (theBanks * theBankInterleaving));

    DBG_Assert( (theBankInterleaving * theBanks) <= theGroupInterleaving, ( << "Invalid interleaving: BI = "
                << theBankInterleaving << ", Banks = " << theBanks << ", GI = " << theGroupInterleaving << ", Groups = " << theGroups ));

    theRVA = new rt_set_t[theNumRTSets];

    // Calculate shifts and masks
    int32_t blockOffsetBits     = log_base2(theBlockSize);
    int32_t regionOffsetBits    = log_base2(theRegionSize);
    int32_t rtIndexBits         = log_base2(theNumRTSets);
    int32_t bIndexBits          = log_base2(theNumDataSets);
    int32_t bankBits            = log_base2(theBanks);
    int32_t bankInterleavingBits    = log_base2(theBankInterleaving);
    int32_t groupBits            = log_base2(theGroups);
    int32_t groupInterleavingBits    = log_base2(theGroupInterleaving);

    int32_t rtLowBits = bankInterleavingBits - regionOffsetBits;
    int32_t rtMidBits = groupInterleavingBits - bankInterleavingBits - bankBits;
    int32_t rtHighBits = rtIndexBits - rtMidBits - rtLowBits;
    if ((rtMidBits + rtLowBits) > rtIndexBits) {
      rtMidBits = rtIndexBits - rtLowBits;
      rtHighBits = 0;
    }

    int32_t bLowBits = bankInterleavingBits - blockOffsetBits;
    int32_t bMidBits = groupInterleavingBits - bankInterleavingBits - bankBits;
    int32_t bHighBits = bIndexBits - bMidBits - bLowBits;
    if ((bMidBits + bLowBits) > bIndexBits) {
      bMidBits = bIndexBits - bLowBits;
      bHighBits = 0;
    }

    DBG_Assert( bHighBits >= 0, ( << "Interleaving is too high for the number of sets." ));

    regionShift     = regionOffsetBits;;
    rtSetLowMask    = ((1 << rtLowBits) - 1);
    rtSetMidMask   = ((1 << rtMidBits) - 1) << rtLowBits;
    rtSetHighMask   = ((1 << rtHighBits) - 1) << (rtMidBits + rtLowBits);
    rtSetMidShift  = bankBits + regionShift;
    rtSetHighShift  = groupBits + bankBits + regionShift;
    rtTagMask       = ~(theRegionSize - 1);

    blockShift      = blockOffsetBits;
    blockTagMask    = ~(theBlockSize - 1);
    blockSetLowMask = (1 << bLowBits) - 1;;
    blockSetMidMask    = ((1 << bMidBits) - 1) << bLowBits;
    blockSetHighMask    = ((1 << bHighBits) - 1) << (bMidBits + bLowBits);
    blockSetMidShift   = bankBits + blockOffsetBits;
    blockSetHighShift   = groupBits + bankBits + blockOffsetBits;
    blockOffsetMask = (theBlocksPerRegion - 1);

    globalSetMask = (1 << (bankBits + bIndexBits)) - 1;
    rtGlobalSetMask = (1 << (bankBits + rtIndexBits)) - 1;

    theBankMask = theBanks - 1;
    theBankShift = bankInterleavingBits;
    theGroupMask = theGroups - 1;
    theGroupShift = bankInterleavingBits;

    theERBReserve = 0;

    nRTCoordinator::RTFunctionList my_functions;
    my_functions.probeOwner = boost::bind(&RTArray::regionProbeOwner, this, _1);
    my_functions.setOwner = boost::bind(&RTArray::regionSetOwner, this, _1, _2);
    my_functions.probePresence = boost::bind(&RTArray::regionProbePresence, this, _1);
    nRTCoordinator::RTCoordinator::getCoordinator().registerRTFunctions(theNodeId, my_functions);
  }

  void allocRegion(MemoryAddress region) {
    nRTCoordinator::RTCoordinator::getCoordinator().regionAllocNotify(theNodeId, region);
    observer_list_t::iterator iter = theAllocObserverList.begin();
    for (; iter != theAllocObserverList.end(); iter++) {
      (*iter)(region, 0, false);
    }
  }

  void evictRegion(MemoryAddress region, int32_t owner, bool shared) {
    observer_list_t::iterator iter = theEvictObserverList.begin();
    for (; iter != theEvictObserverList.end(); iter++) {
      (*iter)(region, owner, shared);
    }
  }

  void registerRegionAllocObserver(observer_func_t func) {
    theAllocObserverList.push_back(func);
  }

  void registerRegionEvictObserver(observer_func_t func) {
    theEvictObserverList.push_back(func);
  }

  bool evictionResourcePressure() const {
    bool ret = ((theERBSize - ((int)theERB.size() + theERBReserve)) < theERBThreshold) && (theERB.size() > 0);
    if (!ret && freeEvictionResources() == 0) {
      DBG_(Trace, Set( (CompName) << theName ) ( << theName << "No Eviction Pressure, but No Free ERB: ERBSize " << theERBSize << ", used " << (int)theERB.size() << ", reserved " << theERBReserve << ", threshold " << theERBThreshold ));
    } else if (ret) {
      DBG_(Trace, Set( (CompName) << theName ) ( << "Eviction Pressure detected. ERB Size: " << theERBSize << ", used " << (int)theERB.size() << ", reserved " << theERBReserve << ", threshold " << theERBThreshold ));
    }
    return ret;
  }
  uint32_t freeEvictionResources() const {
    return (theERBSize - (theERB.size() + theERBReserve) );
  }
  bool evictionResourcesAvailable() const {
    return ( ((int)theERB.size() + theERBReserve) < theERBSize );
  }
  bool evictionResourcesEmpty() const {
    return (theERB.size() == 0);
  }
  void reserveEvictionResource(int32_t n) {
    theERBReserve += n;
    DBG_Assert( ((int)theERB.size() + theERBReserve) <= theERBSize);
    DBG_(Trace, Set( (CompName) << theName ) ( << "Reserving ERB Entry: Size/Used/Reserved = " << theERBSize << "/" << (int)theERB.size() << "/" << theERBReserve ));
  }
  void unreserveEvictionResource(int32_t n) {
    theERBReserve -= n;
    DBG_Assert(theERBReserve >= 0);
    DBG_(Trace, Set( (CompName) << theName ) ( << "Un-Reserving ERB Entry: Size/Used/Reserved = " << theERBSize << "/" << (int)theERB.size() << "/" << theERBReserve ));
  }

  std::pair<_BState, MemoryAddress> getPreemptiveEviction() {

    MemoryAddress victim_addr(0);
    _BState victim_state(_DefaultBState);

    const RTEntry & region = (theERB.template get<by_order>()).front();
    uint64_t addr = region.tag;

    bool found_valid_blocks = false;
    bool found_victim = false;
    int32_t i;
    for (i = 0; (i < theBlocksPerRegion) && (!found_victim || !found_valid_blocks); i++, addr += theBlockSize) {
      block_set_t * tag_set = theBlocks + get_block_set(addr, region.way);

      if (region.state[i].isValid()) {
        if (region.state[i].isProtected()) {
          found_valid_blocks = true;
          continue;
        }
        if (found_victim) {
          break;
        }

        way_iterator block = (tag_set->template get<by_way>()).find(region.ways[i]);
        DBG_Assert( block != (tag_set->template get<by_way>()).end() );

        // Paranoid consistency check
        if (get_rt_tag(block->tag) != region.tag) {
          DBG_( Crit, ( << theName << " RT tag: " << std::hex << region.tag << " Block tag: " << block->tag << " offset: " << i << std::dec ));
          tag_iterator t_block = (tag_set->template get<by_tag>()).find(addr);
          tag_iterator t_end = (tag_set->template get<by_tag>()).end();
          if (t_block != t_end) {
            DBG_Assert( !region.state[i].isValid(), ( << "found block in way " << t_block->way << " instead of way " << region.ways[i] ));
          } else {
            DBG_Assert( !region.state[i].isValid(), ( << "Did not find matching block tag." ));
          }
        } else {
          DBG_(Verb, ( << "Matching region and block tags in region eviction." ));
        }

        victim_state = region.state[i];
        victim_addr = MemoryAddress(addr);

        (tag_set->template get<by_way>()).modify(block, ChangeBlockState(_DefaultBState));
        (theERB.template get<by_order>()).modify((theERB.template get<by_order>()).begin(), InvalidateRTState(i));

        make_block_lru(tag_set, block);

        found_victim = true;
      }
    }
    if (i == theBlocksPerRegion && !found_valid_blocks) {
      if (!found_victim) {
        victim_addr = MemoryAddress(region.tag);
      }
      evictRegion(MemoryAddress(region.tag), region.owner, (region.region_state == SHARED_REGION));
      (theERB.template get<by_order>()).pop_front();
    }
    DBG_(Trace, Set( (CompName) << theName ) Addr(victim_addr) ( << "Getting Preemptive Eviction for block: " << std::hex << victim_addr << " in state " << victim_state ));
    return std::make_pair(victim_state, victim_addr);
  }

  bool isCached(_BState state) {
    return state.isValid();
  }

  void updateOwner( RTLookupResult & result, int32_t owner, uint64_t tagset) {
    uint64_t rt_tag = get_rt_tag(tagset);
    if (!result.region_allocated || result.region->tag != rt_tag) {
      int rt_set_index = get_rt_set(tagset);

      rt_set_t * rt_set = &(theRVA[rt_set_index]);
      rt_index * rt  = &(rt_set->template get<by_tag>());
      rt_iterator entry = rt->find(rt_tag);
      rt_iterator end  = rt->end();

      if (entry == end) {
        rt_set  = &theERB;
        rt  = &(theERB.template get<by_tag>());
        entry = rt->find(rt_tag);
        end  = rt->end();
      }

      if (entry != end) {
        rt->modify(entry, ChangeRegionOwner(owner));
      }
    } else {
      (result.rt_set->template get<by_tag>()).modify(result.region, ChangeRegionOwner(owner));
    }
  }

  int32_t getOwner( RTLookupResult & result, uint64_t tagset) {
    uint64_t rt_tag = get_rt_tag(tagset);
    if (!result.region_allocated || result.region->tag != rt_tag) {
      int rt_set_index = get_rt_set(tagset);

      rt_set_t * rt_set = &(theRVA[rt_set_index]);
      rt_index * rt  = &(rt_set->template get<by_tag>());
      rt_iterator entry = rt->find(rt_tag);
      rt_iterator end  = rt->end();

      if (entry == end) {
        rt_set  = &theERB;
        rt  = &(theERB.template get<by_tag>());
        entry = rt->find(rt_tag);
        end  = rt->end();
      }

      if (entry != end) {
        return entry->owner;
      } else {
        return -1;
      }
    } else {
      return result.region->owner;
    }
  }

  inline rt_iterator update_lru(rt_set_t * rt_set, rt_iterator entry) {
    rt_order_iterator cur = (rt_set->template project<by_order>(entry));
    rt_order_iterator tail = (rt_set->template get<by_order>()).end();
    (rt_set->template get<by_order>()).relocate(tail, cur);
    return (rt_set->template project<by_tag>(cur));
  }

  inline way_iterator update_lru(block_set_t * set, way_iterator entry) {
    order_iterator cur = (set->template project<by_order>(entry));
    order_iterator tail = (set->template get<by_order>()).end();
    (set->template get<by_order>()).relocate(tail, cur);
    return (set->template project<by_way>(cur));
  }

  inline way_iterator make_block_lru(block_set_t * set, way_iterator entry) {
    order_iterator cur = (set->template project<by_order>(entry));
    order_iterator head = (set->template get<by_order>()).begin();
    (set->template get<by_order>()).relocate(head, cur);
    return (set->template project<by_way>(cur));
  }

  inline tag_iterator make_block_lru(block_set_t * set, tag_iterator entry) {
    order_iterator cur = (set->template project<by_order>(entry));
    order_iterator head = (set->template get<by_order>()).begin();
    (set->template get<by_order>()).relocate(head, cur);
    return (set->template project<by_tag>(cur));
  }

  way_iterator replace_lru_block(block_set_t * set, uint64_t tag, const _BState & state, LookupResult_p v_lookup) {
    order_iterator block = (set->template get<by_order>()).begin();
    DBG_Assert(block != (set->template get<by_order>()).end());
    while (block->state.isValid() && block->state.isProtected()) {
      block++;
      DBG_Assert(block != (set->template get<by_order>()).end(), ( << "No Un-protected blocks to evict"));
    }
    if (block->state.isValid()) {
      uint64_t rt_tag = get_rt_tag(block->tag);
      uint64_t rt_set_index = get_rt_set(block->tag);
      int32_t offset = get_block_offset(block->tag);

      rt_set_t * rt_set = &(theRVA[rt_set_index]);
      rt_index * rt  = &(rt_set->template get<by_tag>());
      rt_iterator entry = rt->find(rt_tag);
      rt_iterator end  = rt->end();

      if (entry == end) {
        rt  = &(theERB.template get<by_tag>());
        entry = rt->find(rt_tag);
        end  = rt->end();
        if (entry == end) {
          // Should NEVER get here
          DBG_Assert( false );
        }
      }
      rt->modify(entry, InvalidateRTState(offset));

      // Do the eviction
      DBG_(Verb, ( << "evicting tail block"));
      DBG_(Trace, Addr(block->tag) ( << theName << " evicting block " << std::hex << block->tag << " in state " << block->state ));
      if (block->state.isProtected()) {
        DBG_(Trace, ( << theName << " evicting PROTECTED block " << std::hex << block->tag ));
      }
      v_lookup->orig_state = block->state;
      v_lookup->tagset = block->tag;
    }

    (set->template get<by_order>()).modify(block, ReplaceBlock(tag, state));
    order_iterator tail = (set->template get<by_order>()).end();
    (set->template get<by_order>()).relocate(tail, block);
    DBG_Assert(isConsistent(tag));
    if (v_lookup->orig_state.isValid()) {
      DBG_Assert(isConsistent(v_lookup->tagset));
    }
    return (set->template project<by_way>(block));
  }

  inline bool isConsistent( uint64_t tagset) {
    return true;
#if 0
    block_set_t * block_set = &(theBlocks[get_block_set(tagset, 0)]);
    tag_iterator block = (block_set->template get<by_tag>()).find(get_block_tag(tagset));
    tag_iterator bend   = (block_set->template get<by_tag>()).end();

    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);
    int32_t offset = get_block_offset(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry == end) {
      if (block != bend) {
        if (block->state.isValid()) {
          DBG_(Crit, ( << "block " << std::hex << tagset << " region not present but block is valid"));
          return false;
        }
      }
      return true;
    }
    if ((block == bend) && (entry->state[offset].isValid())) {
      DBG_(Crit, Set( (CompName) << theName) Addr(tagset) ( << "block " << std::hex << tagset << " not present but RVA says valid"));
      return false;
    }
    if ((block != bend) && entry->state[offset] != block->state)  {
      DBG_(Crit, Set( (CompName) << theName) Addr(tagset) ( << "block " << std::hex << tagset << " RVA and BST states do not match (" << entry->state[offset] << " != " << block->state << ")" ));
      return false;
    }
    if ((block != bend) && (entry->ways[offset] != block->way) && (block->state.isValid())) {
      DBG_(Crit, Set( (CompName) << theName) Addr(tagset) ( << "block " << std::hex << tagset << " RVA and BST ways do not match (" << entry->ways[offset] << " != " << block->way << ")" ));
      return false;
    }
    return true;
#endif
  }

  virtual AbstractArrayLookup_p operator[](const MemoryAddress & anAddress) {

    uint64_t tagset = anAddress;
    uint64_t rt_tag = get_rt_tag(tagset);

    int rt_set_index = get_rt_set(tagset);
    int32_t offset   = get_block_offset(tagset);
    bool region_allocated = false;

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      // No region entry in RVA, check ERB
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      region_allocated = true;
      block_set_t * block_set = &(theBlocks[get_block_set(tagset, entry->way)]);

      // We have a region entry
      if (entry->state[offset].isValid()) {
        // The block is valid

        tag_iterator t_block = (block_set->template get<by_tag>()).find(get_block_tag(tagset));
        tag_iterator t_end   = (block_set->template get<by_tag>()).end();

        DBG_Assert( t_block != t_end, ( << "tagset 0x" << std::hex << (uint64_t)tagset << ", BTag 0x" << (uint64_t)get_block_tag(tagset) << ", rt_tag 0x" << rt_tag << ", rt_set 0x" << rt_set_index << ", offset " << std::dec << offset ));

        way_iterator block = block_set->template project<by_way>(t_block);

        LookupResult_p ret = new RTLookupResult(true, true, this, tagset);

        ret->rt_set  = rt_set;
        ret->offset  = offset;
        ret->region  = entry;
        ret->block  = block;
        ret->block_set = block_set;
        ret->orig_state = block->state;

        DBG_Assert( isConsistent(tagset) );

        return ret;
      }
    }

    return LookupResult_p( new RTLookupResult(region_allocated, false, this, tagset)) ;
  }

  bool  recordAccess(AbstractArrayLookup_p abs_lookup) {
    RTLookupResult * lookup = dynamic_cast<RTLookupResult *>(abs_lookup.get());
    DBG_Assert(lookup != nullptr);
    return lookup->updateLRU();
  }

  void  invalidateBlock(AbstractArrayLookup_p abs_lookup) {
    RTLookupResult * lookup = dynamic_cast<RTLookupResult *>(abs_lookup.get());
    DBG_Assert(lookup != nullptr);

    lookup->block = make_block_lru(lookup->block_set, lookup->block);
    DBG_Assert(isConsistent(lookup->tagset));
  }

  AbstractArrayLookup_p allocate(AbstractArrayLookup_p abs_lookup, const MemoryAddress & anAddress) {
    RTLookupResult * lookup = dynamic_cast<RTLookupResult *>(abs_lookup.get());
    DBG_Assert(lookup != nullptr);

    uint64_t tagset = anAddress;
    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index  = get_rt_set(tagset);
    int32_t offset    = get_block_offset(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      // No region entry in RVA, check ERB
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    // We didn't find a region entry in the RVA or the ERB
    if (entry == end) {
      DBG_(Verb, ( << theName << " - Allocating new RVA entry with tag: " << std::hex << rt_tag << std::dec ));
      rt_set = &(theRVA[rt_set_index]);
      rt  = &(rt_set->template get<by_tag>());

      // Start by adding another way to the set
      int32_t way = rt->size();

      // If the RVA set has too may ways, move one to the ERB
      if (way >= theRTAssociativity) {
        way = (rt_set->template get<by_order>()).front().way;
        (theERB.template get<by_order>()).push_back((rt_set->template get<by_order>()).front());
        (rt_set->template get<by_order>()).pop_front();

        // We need to reserve space in the ERB anytime we might need to evict a region
        DBG_Assert( (int)theERB.size() <= theERBSize );

      }

      // Finally, add the new region entry to the RVA set
      std::pair<rt_order_iterator, bool> temp;
      temp = (rt_set->template get<by_order>()).push_back(RTEntry(theBlocksPerRegion, rt_tag, way));
      DBG_Assert((int)(rt_set->template get<by_order>()).size() <= theRTAssociativity);
      DBG_Assert(temp.second);
      entry = rt_set->template project<by_tag>(temp.first);
      end   = rt->end();

      allocRegion(anAddress);
    }

    DBG_Assert( entry != end );

    block_set_t * block_set = &(theBlocks[get_block_set(tagset, entry->way)]);

    // We have a region entry
    if (entry->state[offset].isValid()) {
      // The block is valid

      tag_iterator t_block = (block_set->template get<by_tag>()).find(get_block_tag(tagset));
      tag_iterator t_end   = (block_set->template get<by_tag>()).end();

      DBG_Assert( t_block != t_end );

      way_iterator block = block_set->template project<by_way>(t_block);

      update_lru(block_set, block);

      lookup->region_allocated = true;
      lookup->block_allocated = true;
      lookup->rt_cache = this;
      lookup->block_set = block_set;
      lookup->tagset = tagset;
      lookup->rt_set = rt_set;
      lookup->offset = offset;
      lookup->region = entry;
      lookup->block = block;
      lookup->orig_state = block->state;

      DBG_Assert( isConsistent(tagset) );

      DBG_(Trace, Set( (CompName) << theName) Addr(tagset) ( << " Inserting block " << std::hex << tagset << " in set: " << std::hex << get_block_set(tagset, entry->way) << ", way: " << block->way << "(" << entry->ways[offset] << "), size: " << block_set->size()));

      // we return the victim, in this case there is none
      return LookupResult_p(new RTLookupResult(false, false, this, 0));
    } else {
      // The block is not currently valid, so let's allocate space for it

      // First, find a replacement block
      uint64_t block_tag = get_block_tag(tagset);
      way_iterator block = (block_set->template get<by_way>()).end();
      int32_t way = block_set->size();

      // Try to find a matching tag in the BST
      // This is a somewhat un-realistic optimization since the BST
      // doesn't contain tags, but it simplifies things in the code
      // if we re-use entries like this, and it doesn't result in any
      // un-realistic behaviour unless we care about the specific "way" being used
      // Thus, if we implement a Non-LRU block replacement policy, we might not
      // want to do this.

      LookupResult_p v_lookup( new RTLookupResult(false, false, this, 0));

      tag_iterator t_block = (block_set->template get<by_tag>()).find(block_tag);
      tag_iterator t_end   = (block_set->template get<by_tag>()).end();
      if (t_block != t_end) {
        block = block_set->template project<by_way>(t_block);
        way = block->way;
        (block_set->template get<by_way>()).modify(block, ChangeBlockState(_DefaultBState));
        update_lru(block_set, block);
        DBG_Assert(block->way == way);
        DBG_(Verb, ( << theName << "Found matching BST tag, new state is " << block->state));
      } else {
        // First, check if the set is full yet, if not simply add the block to the set.
        if (way < theAssociativity) {
          std::pair<order_iterator, bool> result = (block_set->template get<by_order>()).push_back(BlockEntry(block_tag, _DefaultBState, way));
          if (!result.second) {
            DBG_(Dev, ( << theName << " Failed to insert block. Existing block tag: " << std::hex << result.first->tag << std::dec << ", way: " << result.first->way ));
            DBG_(Dev, ( << theName << " Failed to insert block. New block tag: " << std::hex << block_tag << std::dec << ", way: " << way ));
            DBG_(Dev, ( << theName << " Was in state " << std::hex << result.first->state << std::dec ));
            DBG_(Dev, ( << theName << " size: " << (block_set->template get<by_way>()).size() ));
            DBG_(Dev, ( << theName << " set: " << std::hex << block_tag ));
            DBG_(Dev, ( << theName << " Inserting block " << std::hex << block_tag << " in set: " << get_block_set(tagset, entry->way) << ", way: " << way << ", size: " << block_set->size() ));
            DBG_Assert(false);
          }
          block = block_set->template project<by_way>(result.first);
          DBG_Assert(block->way == way);
          DBG_(Trace, ( << theName << " Allocated new block: " << std::hex << tagset << ", state is " << block->state));
        } else if (!(block_set->template get<by_order>()).front().state.isValid()) {
          // Next, look for an invalid block (that would naturally be at the bottom (front) of the LRU
          way = (block_set->template get<by_order>()).front().way;
          block = replace_lru_block(block_set, block_tag, _DefaultBState, v_lookup);
          DBG_Assert(block->way == way);
          DBG_(Verb, ( << theName << " Replaced Invalid Block, new state is " << block->state));
        } else {
          // Set is full of valid blocks, use replacement policy
          if (theReplPolicy == SET_LRU) {
            // The LRU block might be protected, so wait till we do the replacement to get the way
            // way = (block_set->template get<by_order>()).front().way;
            block = replace_lru_block(block_set, block_tag, _DefaultBState, v_lookup);
            way = block->way;
            DBG_(Verb, ( << theName << " Replaced LRU block, state is " << block->state));
          } else if (theReplPolicy == REGION_LRU) {
            DBG_Assert( false , ( << "RegionLRU Not Yet Supported !!!!" ));

            // This actually becomes complicated and needs additional support
            // if we want to implement it.
            // Each BST set contains blocks that map to multiple RVA sets
            // So we need an LRU order between all of the blocks in those RVA
            // sets, and then we need to pick the block that maps the the least
            // recently used RVA entry. Keeping in mind that there are more RVA
            // entries than there are blocks in the BST set.

          } else {
            // Don't know any other policies
            DBG_Assert( false );
          }
        }
      }

      DBG_Assert(block != (block_set->template get<by_way>()).end());
      DBG_Assert(way < theAssociativity);

      rt->modify(entry, AllocateBlock(offset, _DefaultBState, way));

      DBG_(Verb, ( << theName << " RVA state set to: " << entry->state[offset] ));

      entry = update_lru(rt_set, entry);

      lookup->region_allocated = true;
      lookup->block_allocated = true;
      lookup->rt_cache  = this;
      lookup->tagset  = tagset;
      lookup->rt_set  = rt_set;
      lookup->offset  = offset;
      lookup->region  = entry;
      lookup->block  = block;
      lookup->block_set = block_set;
      lookup->orig_state = _DefaultBState;

      DBG_(Trace, Set( (CompName) << theName) Addr(tagset) ( << " Inserting block " << std::hex << tagset << " in set: " << std::hex << get_block_set(tagset, entry->way) << ", way: " << block->way << "(" << entry->ways[offset] << "), size: " << block_set->size()));

      DBG_Assert( isConsistent(tagset) );
      return v_lookup;
    }
  }

  void setPrefetched(RTLookupResult & result, bool val) {
    (result.rt_set->template get<by_tag>()).modify(result.region, ChangeRTPrefetched(result.offset, val));
    (result.block_set->template get<by_way>()).modify(result.block, ChangeBlockPrefetched(val));
  }

  void setProtected(RTLookupResult & result, bool val) {
    (result.rt_set->template get<by_tag>()).modify(result.region, ChangeRTProtected(result.offset, val));
    (result.block_set->template get<by_way>()).modify(result.block, ChangeBlockProtected(val));
  }

  void updateState(RTLookupResult & result, const _BState & new_state, bool make_MRU, bool make_LRU) {
    DBG_Assert(result.block_allocated);

    DBG_(Trace, Set( (CompName) << theName ) Addr(result.tagset) ( << "Setting State of block: " << std::hex << result.tagset << " to " << new_state ));

    (result.rt_set->template get<by_tag>()).modify(result.region, ChangeRTState(result.offset, new_state));
    (result.block_set->template get<by_way>()).modify(result.block, ChangeBlockState(new_state));

    // Fixup LRU order
    if (make_MRU) {
      result.region = update_lru(result.rt_set, result.region);
      result.block = update_lru(result.block_set, result.block);
    } else if (make_LRU) {
      make_block_lru(result.block_set, result.block);
    }
    DBG_Assert(isConsistent(result.tagset));
  }

  void setLocked(RTLookupResult &result, bool val) {
    (result.rt_set->template get<by_tag>()).modify(result.region, ChangeRTLocked(result.offset, val));
    (result.block_set->template get<by_way>()).modify(result.block, ChangeBlockLocked(val));
  }

  int32_t regionProbeOwner(MemoryAddress anAddress) {

    uint64_t tagset = anAddress;
    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    int32_t owner = -1;
    if (entry != end) {
      owner =  entry->owner;
    }

    DBG_(Verb, ( << theName << ": regionProbeOwner(" << std::hex << tagset << std::dec << ") = " << owner ));
    return owner;
  }
  bool regionSetOwner(MemoryAddress anAddress, int32_t owner) {

    uint64_t tagset = anAddress;
    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      rt->modify(entry, ChangeRegionOwner(owner));
      return true;
    }

    DBG_(Verb, ( << theName << ": regionSetOwner(" << std::hex << tagset << std::dec << ") = " << owner << " FAILED!"));
    return false;
  }
  boost::dynamic_bitset<> regionProbePresence(MemoryAddress anAddress) {

    boost::dynamic_bitset<> ret(theBlocksPerRegion, false);

    uint64_t tagset = anAddress;
    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      entry->getPresence(ret);
    }

    DBG_(Verb, ( << theName << ": regionProbePresence(" << std::hex << tagset << std::dec << ") = " << ret ));
    return ret;
  }
  const std::vector<_BState>* regionProbeState(MemoryAddress anAddress) {

    uint64_t tagset = anAddress;
    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    //DBG_Assert(entry != end);
    if (entry == end) {
      return nullptr;
    }

    DBG_(Verb, ( << theName << ": regionProbeState(" << std::hex << tagset << std::dec << ")" ));
    return &(entry->state);
  }

  uint32_t blockScoutProbe( uint64_t tagset) {

    uint32_t ret = 0;

    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      ret = entry->validVector();
    }

    DBG_(Verb, ( << theName << ": blockSetProbe(" << std::hex << tagset << std::dec << ") = " << ret ));
    return ret;
  }

  uint32_t blockProbe( uint64_t tagset) {

    uint32_t ret = 0;

    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      int32_t offset = get_block_offset(tagset);
      ret = (uint32_t)entry->state[offset].isValid();
    }

    DBG_(Verb, ( << theName << ": blockProbe(" << std::hex << tagset << std::dec << ") = " << ret ));
    return ret;
  }

  void _setSharingState( uint64_t tagset, RegionState aState, uint32_t shared_blocks) {

    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      if (entry->region_state != aState) {
        rt->modify(entry, ChangeRegionState(aState));
      }
      if (theBlockScout) {
        rt->modify(entry, ChangeSharedBlocks(shared_blocks));
      }
    }
  }

  void setNonSharedRegion( uint64_t tagset) {
    DBG_(Verb, ( << theName << ": setNonSharedRegion(" << std::hex << tagset << std::dec << ")" ));
    _setSharingState(tagset, NON_SHARED_REGION, 0);
  }
  void setSharedRegion( uint64_t tagset) {
    DBG_(Verb, ( << theName << ": setSharedRegion(" << std::hex << tagset << std::dec << ")" ));
    _setSharingState(tagset, SHARED_REGION, ((1ULL << theBlocksPerRegion) - 1));
  }

  void setPartialSharedRegion( uint64_t tagset, uint32_t shared) {
    if (!theBlockScout) {
      return;
    }
    DBG_(Verb, ( << theName << ": setNonSharedRegion(" << std::hex << tagset << std::dec << ")" ));
    _setSharingState(tagset, PARTIAL_SHARED_REGION, shared);
  }

  bool isNonSharedRegion( uint64_t tagset) {
    bool ret = false;

    uint64_t rt_tag = get_rt_tag(tagset);
    int rt_set_index = get_rt_set(tagset);

    rt_set_t * rt_set = &(theRVA[rt_set_index]);
    rt_index * rt  = &(rt_set->template get<by_tag>());
    rt_iterator entry = rt->find(rt_tag);
    rt_iterator end  = rt->end();

    if (entry == end) {
      rt_set  = &theERB;
      rt  = &(theERB.template get<by_tag>());
      entry = rt->find(rt_tag);
      end  = rt->end();
    }

    if (entry != end) {
      if (entry->region_state == NON_SHARED_REGION) {
        ret = true;
      } else if (theBlockScout && (entry->region_state == PARTIAL_SHARED_REGION)) {
        int32_t offset = get_block_offset(tagset);
        ret = ! entry->shared[offset];
      }
    }
    DBG_(Verb, ( << theName << ": isNonSharedRegion(" << std::hex << tagset << std::dec << ") = " << ret ));
    return ret;
  }

  bool saveState( std::ostream & s ) {
    return true;
  }

  bool loadState( std::istream & s, int32_t theIndex ) {

    boost::archive::binary_iarchive ia(s);

    int32_t totalRTSets = theNumRTSets * theTotalBanks;
    int32_t global_set = 0;
    int32_t local_set = 0;
    MemoryAddress addr(0);

    uint64_t set_count = 0;
    uint32_t associativity = 0;

    ia >> set_count;
    ia >> associativity;

    DBG_Assert( set_count == (uint64_t)totalRTSets, ( << "Error loading cache state. Flexpoint contains " << set_count << " RT sets but simulator configured for " << totalRTSets << " RT sets." ));
    DBG_Assert( associativity == (uint64_t)theRTAssociativity, ( << "Error loading cache state. Flexpoint contains " << associativity << "-way RT sets but simulator configured for " << theRTAssociativity << "-way RT sets." ));

    for (; global_set < totalRTSets; global_set++, addr += theBlockSize) {
      if ((getBankIndex(addr) == theLocalBankIndex) && (getGroupIndex(addr) == theGroupIndex)) {
        for (int32_t way = 0; way < theRTAssociativity; way++) {
          RTSerializer serial;
          ia >> serial;

          DBG_Assert(getBankIndex(serial.tag) == theLocalBankIndex && getGroupIndex(serial.tag) == theGroupIndex);

          RegionState rstate = SHARED_REGION;
          switch (serial.state) {
            case 'N':
              rstate = NON_SHARED_REGION;
              break;
            case 'S':
              rstate = SHARED_REGION;
              break;
            case 'P':
              rstate = PARTIAL_SHARED_REGION;
              break;
            default:
              DBG_Assert(false, ( << "Unknown RegionState: " << serial.state));
              break;
          }
          if (serial.owner != -1) {
            DBG_(Verb, ( << "Loaded delegated region " << std::hex << (uint64_t)serial.tag << " Owner is: " << std::dec << (int)serial.owner ));
          }
          (theRVA[local_set].template get<by_order>()).push_back(RTEntry(theBlocksPerRegion, serial.tag, way, rstate, serial.owner));
        }
        local_set++;
      } else {
        for (int32_t way = 0; way < theRTAssociativity; way++) {
          RTSerializer serial;
          ia >> serial;
        }
      }
    }
    return false;
  }

};

};  // namespace nCMPCache

BOOST_CLASS_TRACKING(nCMPCache::RTSerializer, boost::serialization::track_never)
BOOST_CLASS_TRACKING(nCMPCache::BlockSerializer, boost::serialization::track_never)

#endif /* FLEXUS_CMP_CACHE_RTCMP_CACHE_HPP_INCLUDED */
