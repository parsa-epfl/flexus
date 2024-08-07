#ifndef FLEXUS_FASTCACHE_STANDARD_CACHE_HPP_INCLUDED
#define FLEXUS_FASTCACHE_STANDARD_CACHE_HPP_INCLUDED

#include <boost/dynamic_bitset.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <components/CommonQEMU/Util.hpp>
#include <core/checkpoint/json.hpp>
#include <core/types.hpp>
#include <functional>

using namespace boost::multi_index;
using Flexus::SharedTypes::PhysicalMemoryAddress;
using nCommonUtil::log_base2;
using json = nlohmann::json;
namespace nFastCache {

typedef std::function<void(uint64_t tagset, CoherenceState_t state)> evict_function_t;
typedef std::function<void(uint64_t tagset, int32_t owner)> region_evict_function_t;
typedef std::function<bool(uint64_t tagset, bool icache, bool dcache)> invalidate_function_t;

class StdCache //: public AbstractCache
{
  private:
    // Structur to mimic traditional tag array (BST like structure)
    struct BlockEntry
    {
        uint64_t tag;
        CoherenceState_t state;
        uint16_t way;

        BlockEntry(uint64_t t, CoherenceState_t s, uint16_t w)
          : tag(t)
          , state(s)
          , way(w)
        {
        }
    };

    // Modifier Objects to update BlockEntry objects in multi-index containers
    struct ChangeBlockState
    {
        ChangeBlockState(CoherenceState_t state)
          : state(state)
        {
        }
        void operator()(BlockEntry& entry)
        {
            DBG_(Verb,
                 Addr(entry.tag)(<< "Changing state of block " << std::hex << entry.tag << " from " << entry.state
                                 << " to " << state << std::dec));
            entry.state = state;
        }

      private:
        CoherenceState_t state;
    };

    struct ReplaceBlock
    {
        ReplaceBlock(uint64_t tag, CoherenceState_t state)
          : tag(tag)
          , state(state)
        {
        }
        void operator()(BlockEntry& entry)
        {
            DBG_(Verb,
                 Addr(entry.tag)(<< "Replacing block " << std::hex << entry.tag << " (" << entry.state << ") with "
                                 << tag << " (" << state << ")" << std::dec));
            entry.state = state;
            entry.tag   = tag;
        }

      private:
        uint64_t tag;
        CoherenceState_t state;
    };

    struct by_order
    {};
    struct by_tag
    {};
    struct by_way
    {};

    struct Int64Hash
    {
        const std::size_t operator()(int64_t x) const { return (std::size_t)(x >> 6); }
    };

    typedef multi_index_container<
      BlockEntry,
      indexed_by<sequenced<tag<by_order>>,
                 hashed_unique<tag<by_tag>, member<BlockEntry, uint64_t, &BlockEntry::tag>, Int64Hash>,
                 hashed_unique<tag<by_way>, member<BlockEntry, uint16_t, &BlockEntry::way>>>>
      block_set_t;

    typedef block_set_t::index<by_order>::type::iterator order_iterator;
    typedef block_set_t::index<by_tag>::type::iterator tag_iterator;
    typedef block_set_t::index<by_way>::type::iterator way_iterator;

    typedef block_set_t::index<by_way>::type way_index;

    class StdLookupResult : public LookupResult
    {
      public:
        PhysicalMemoryAddress theAddress;
        CoherenceState_t theState;
        block_set_t* theSet;
        tag_iterator theBlock;
        StdCache* theCache;

        StdLookupResult(PhysicalMemoryAddress addr,
                        CoherenceState_t state,
                        block_set_t* set,
                        tag_iterator block,
                        StdCache* cache)
          : theAddress(addr)
          , theState(state)
          , theSet(set)
          , theBlock(block)
          , theCache(cache)
        {
        }

        ~StdLookupResult() {}

        void allocate(CoherenceState_t new_state) { theCache->allocate(theAddress, new_state, *this); }

        void changeState(CoherenceState_t new_state, bool make_MRU, bool make_LRU)
        {
            theCache->updateState(*this, new_state, make_MRU, make_LRU);
        }

        void updateLRU() { theBlock = theCache->update_lru(theSet, theBlock); }

        CoherenceState_t getState()
        {
            if (theBlock->tag == theAddress) {
                return theBlock->state;
            } else {
                return theState;
            }
        }

        PhysicalMemoryAddress address() { return theAddress; }
    };

    typedef boost::intrusive_ptr<StdLookupResult> StdLookupResult_p;

    std::string theName;
    int32_t theNumSets;
    int32_t theAssoc;
    int32_t theBlockSize;

    int32_t blockShift;
    int32_t blockSetMask;
    int64_t blockTagMask;

    evict_function_t evict;
    invalidate_function_t sendInvalidate;
    int32_t theIndex;
    Flexus::SharedTypes::tFillLevel theLevel;

    block_set_t* theBlocks;

    bool theAllocateInProgress;
    int64_t theAllocateAddr;

    int32_t get_set(uint64_t addr) { return (addr >> blockShift) & blockSetMask; }

    uint64_t get_tag(uint64_t addr) { return (addr & blockTagMask); }

  public:
    StdCache(const std::string& aName,
             int32_t aBlockSize,
             int32_t aNumSets,
             int32_t anAssociativity,
             evict_function_t anEvict,
             invalidate_function_t aSendInvalidate,
             int32_t anIndex,
             Flexus::SharedTypes::tFillLevel aLevel)
    {
        theName      = aName;
        theNumSets   = aNumSets;
        theAssoc     = anAssociativity;
        theBlockSize = aBlockSize;

        theAllocateInProgress = false;

        theBlocks = new block_set_t[theNumSets];

        evict          = anEvict;
        sendInvalidate = aSendInvalidate;

        theIndex = anIndex;
        theLevel = aLevel;

        DBG_Assert(theAssoc > 0);
        DBG_Assert(theNumSets > 0);
        DBG_Assert(theBlockSize > 0);
        DBG_Assert((theNumSets & (theNumSets - 1)) == 0);
        DBG_Assert((theBlockSize & (theBlockSize - 1)) == 0);

        blockShift   = log_base2(theBlockSize);
        blockTagMask = ~(aBlockSize - 1);
        blockSetMask = (theNumSets - 1);
    }

    ~StdCache() { delete[] theBlocks; }

    void allocate(PhysicalMemoryAddress addr, CoherenceState_t new_state, StdLookupResult& lookup)
    {
        uint64_t new_tag = get_tag(addr);

        theAllocateInProgress = true;
        theAllocateAddr       = new_tag;

        tag_iterator block = lookup.theBlock;
        block_set_t* set   = lookup.theSet;

        if (block->tag != new_tag) {
            if (isValid(block->state)) { evict(block->tag, block->state); }
            set->get<by_tag>().modify(block, ReplaceBlock(new_tag, new_state));
        } else {
            set->get<by_tag>().modify(block, ChangeBlockState(new_state));
        }

        order_iterator tail = set->get<by_order>().end();
        set->get<by_order>().relocate(tail, set->project<by_order>(block));

        theAllocateInProgress = false;
    }

    inline tag_iterator make_block_lru(block_set_t* set, tag_iterator entry)
    {
        order_iterator cur  = set->project<by_order>(entry);
        order_iterator head = set->get<by_order>().begin();
        set->get<by_order>().relocate(head, cur);
        return set->project<by_tag>(cur);
    }

    tag_iterator update_lru(block_set_t* set, tag_iterator block)
    {
        order_iterator cur  = set->project<by_order>(block);
        order_iterator tail = set->get<by_order>().end();
        set->get<by_order>().relocate(tail, cur);
        return set->project<by_tag>(cur);
    }

    void updateState(StdLookupResult& lookup, CoherenceState_t new_state, bool make_MRU, bool make_LRU)
    {

        tag_iterator block = lookup.theBlock;
        block_set_t* set   = lookup.theSet;

        set->get<by_tag>().modify(block, ChangeBlockState(new_state));

        if (make_MRU) {
            lookup.theBlock = update_lru(set, block);
        } else if (make_LRU) {
            lookup.theBlock = make_block_lru(set, block);
        }
    }

    LookupResult_p lookup(uint64_t tagset)
    {
        int64_t tag       = get_tag(tagset);
        int32_t set_index = get_set(tagset);

        block_set_t* set = &(theBlocks[set_index]);

        tag_iterator t_block = set->get<by_tag>().find(tag);
        tag_iterator t_end   = set->get<by_tag>().end();

        // if we didn't find the tag we're looking for
        if (t_block == t_end) {
            if (set->size() < (uint32_t)theAssoc) {
                std::pair<order_iterator, bool> ret =
                  set->get<by_order>().push_front(BlockEntry(tag, kInvalid, set->size()));
                t_block = set->project<by_tag>(ret.first);
            } else {
                t_block = set->project<by_tag>(set->get<by_order>().begin());
            }

            return StdLookupResult_p(new StdLookupResult(PhysicalMemoryAddress(tag), kInvalid, set, t_block, this));
        }

        return StdLookupResult_p(new StdLookupResult(PhysicalMemoryAddress(tag), t_block->state, set, t_block, this));
    }

    void getSetTags(uint64_t address, std::list<PhysicalMemoryAddress>& tags)
    {
        int32_t set_index = get_set(address);

        // First, check if we're in the process of allocating a new block to this
        // set (This happens if this was called as a result of evicting a block
        // during the allocation process)
        if (theAllocateInProgress) {
            int32_t alloc_set = get_set(theAllocateAddr);
            if (alloc_set == set_index) { tags.push_back(PhysicalMemoryAddress(theAllocateAddr)); }
        }

        // Now check all the Valid blocks in the given set
        block_set_t* block_set = &(theBlocks[set_index]);
        tag_iterator block     = block_set->get<by_tag>().begin();
        tag_iterator bend      = block_set->get<by_tag>().end();

        for (; block != bend; block++) {
            if (isValid(block->state)) { tags.push_back(PhysicalMemoryAddress(block->tag)); }
        }
    }

    void saveState(std::ostream& s)
    {
        json checkpoint;
        checkpoint["associativity"] = (uint64_t)theAssoc;

        uint32_t shift = blockShift + log_base2(theNumSets);

        for (size_t set = 0; set < (size_t)theNumSets; set++) {

            checkpoint["tags"][set] = json::array();

            order_iterator block = theBlocks[set].get<by_order>().begin();
            order_iterator end   = theBlocks[set].get<by_order>().end();

            uint16_t i         = 0;
            bool dirty_flag    = false;
            bool writable_flag = false;

            for (; block != end; block++) {

                DBG_Assert(i < theAssoc, (<< "The number of way saved is greater than the associativity... weird"));
                uint64_t tag = block->tag >> shift;

                switch (block->state) {
                    case kModified:
                        writable_flag = true;
                        dirty_flag    = true;
                        break;
                    case kOwned:
                        writable_flag = false;
                        dirty_flag    = true;
                        break;
                    case kExclusive:
                        writable_flag = true;
                        dirty_flag    = false;
                        break;
                    case kShared:
                        writable_flag = false;
                        dirty_flag    = false;
                        break;
                    case kInvalid: continue;
                    default: DBG_Assert(false, (<< "Don't know how to save state " << block->state)); break;
                }

                checkpoint["tags"][set][i++] = { { "tag", tag },
                                                 { "writable", writable_flag },
                                                 { "dirty", dirty_flag } };

                DBG_(Trace,
                     (<< theName << " - Saving block " << std::hex << block->tag << " with state "
                      << state2String(block->state) << " in way " << block->way));
            }
        }

        s << std::setw(4) << checkpoint << std::endl;
    }

    //   bool loadState(std::istream &s) {
    //    json checkpoint;
    //    s >> checkpoint;
    //
    //    uint32_t shift = blockShift + log_base2(theNumSets);
    //
    //    for (size_t set = 0; set < (size_t)theNumSets; set++) {
    //
    //      size_t blockSize = checkpoint["tags"].at(set).size();
    //
    //      //empty the cache set
    //      theBlocks[set].get<by_order>().clear();
    //
    //      for(size_t block = 0; block < blockSize; block++){
    //        bool dirty = checkpoint["tags"].at(set).at(block)["dirty"];
    //        bool writable = checkpoint["tags"].at(set).at(block)["writable"];
    //        uint64_t tag = checkpoint["tags"].at(set).at(block)["tag"];
    //
    //        CoherenceState_t state(kInvalid);
    //        if(dirty){
    //          if(writable) state = kModified;
    //          else state = kOwned;
    //        } else{
    //          if(writable) state = kExclusive;
    //          else state = kShared;
    //        }
    //
    //        DBG_(Trace, (<< theName << " - Loading block " << std::hex
    //                       << ((tag << shift) | (set << blockShift)) << " with state "
    //                       << state2String(state) << " in way " << block));
    //
    //        //fill the empty set
    //        theBlocks[set].get<by_order>().push_back(
    //          BlockEntry(((tag << shift) | (set << blockShift)), state, (uint16_t)(block)));
    //      }
    //
    //    }
    //
    //    return true;
    //  }
};

} // namespace nFastCache

#endif /* FLEXUS_FASTCACHE_STANDARD_CACHE_HPP_INCLUDED */