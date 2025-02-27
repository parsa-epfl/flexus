
#ifndef _CACHE_ARRAY_HPP
#define _CACHE_ARRAY_HPP

#include "components/CommonQEMU/Serializers.hpp"
#include "components/CommonQEMU/Util.hpp"
#include "core/checkpoint/json.hpp"
#include "core/debug/debug.hpp"
#include "core/target.hpp"
#include "core/types.hpp"

#include <iostream>

using nCommonSerializers::BlockSerializer;
using nCommonUtil::log_base2;
using json = nlohmann::json;

namespace nCache {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

typedef int32_t SetIndex;
typedef uint32_t BlockOffset;

enum ReplacementPolicy
{
    REPLACEMENT_LRU,
};

// This is a cache block.  The accessor functions are braindead simple.
template<typename _State, const _State& _DefaultState>
class Block
{
  public:
    Block(void)
      : theTag(0)
      , theState(_DefaultState)
    {
    }

    MemoryAddress tag(void) const { return theTag; }

    MemoryAddress& tag(void) { return theTag; }

    const _State& state(void) const { return theState; }
    _State& state(void) { return theState; }

    bool valid() { return theState != _DefaultState; }

  private:
    MemoryAddress theTag;
    _State theState;

}; // class Block

template<typename _State, const _State& _DefaultState>
class Set;
template<typename _State, const _State& _DefaultState>
class StdArray;

// The output of a cache lookup
template<typename _State, const _State& _DefaultState>
class StdLookupResult : public AbstractLookupResult<_State>
{
  public:
    virtual ~StdLookupResult() {}
    StdLookupResult(Set<_State, _DefaultState>* aSet,
                    Block<_State, _DefaultState>* aBlock,
                    MemoryAddress aBlockAddress,
                    bool aIsHit)
      : theSet(aSet)
      , theBlock(aBlock)
      , theBlockAddress(aBlockAddress)
      , isHit(aIsHit)
      , theOrigState(aIsHit ? aBlock->state() : _DefaultState)
    {
        DBG_Assert((aBlock == nullptr) || (aBlock->tag() == aBlockAddress),
                   (<< "Created Lookup where tag " << std::hex << (uint64_t)aBlock->tag() << " != address "
                    << (uint64_t)aBlockAddress));
    }

    const _State& state() const { return (isHit ? theBlock->state() : theOrigState); }
    void setState(const _State& aNewState) { theBlock->state() = aNewState; }
    void setProtected(bool val) { theBlock->state().setProtected(val); }
    void setPrefetched(bool val) { theBlock->state().setPrefetched(val); }

    bool hit(void) const { return isHit; }
    bool miss(void) const { return !isHit; }
    bool found(void) const { return theBlock != nullptr; }
    bool valid(void) const { return theBlock != nullptr; }

    MemoryAddress blockAddress(void) const { return theBlockAddress; }

  protected:
    Set<_State, _DefaultState>* theSet;
    Block<_State, _DefaultState>* theBlock;
    MemoryAddress theBlockAddress;
    bool isHit;
    _State theOrigState;

    friend class Set<_State, _DefaultState>;
    friend class StdArray<_State, _DefaultState>;
}; // class LookupResult

// The cache set, implements a set of cache blocks and applies a replacement
// policy (from a derived class)
template<typename _State, const _State& _DefaultState>
class Set
{
  public:
    Set(const int32_t aAssociativity)
    {
        theAssociativity = aAssociativity;
        theBlocks        = new Block<_State, _DefaultState>[theAssociativity];
        DBG_Assert(theBlocks);
    }
    virtual ~Set() {}

    typedef StdLookupResult<_State, _DefaultState> LookupResult;
    typedef boost::intrusive_ptr<LookupResult> LookupResult_p;

    LookupResult_p lookupBlock(const MemoryAddress anAddress)
    {
        int32_t i, t = -1;

        // Linearly search through the set for the matching block
        // This could be made faster for higher-associativity sets
        // Through other search methods
        for (i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].tag() == anAddress) {
                if (theBlocks[i].valid()) {
                    return LookupResult_p(new LookupResult(this, &(theBlocks[i]), anAddress, true));
                }
                t = i;
            }
        }
        if (t >= 0) { return LookupResult_p(new LookupResult(this, &(theBlocks[t]), anAddress, false)); }

        // Miss on this set
        return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));
    }

    virtual bool canAllocate(LookupResult_p lookup, MemoryAddress anAddress)
    {
        // First look for an invalid tag match
        if (lookup->theBlock != nullptr) {
            return true;
        } else {
            return victimAvailable();
        }
    }
    virtual LookupResult_p allocate(LookupResult_p lookup, MemoryAddress anAddress)
    {
        // First look for an invalid tag match
        if (lookup->theBlock != nullptr) {
            lookup->isHit = true;
            DBG_Assert(
              lookup->theBlock->tag() == anAddress,
              (<< "Lookup Tag " << std::hex << (uint64_t)lookup->theBlock->tag() << " != " << (uint64_t)anAddress));
            // don't need to change the lookup, just fix order and return no victim
            recordAccess(lookup->theBlock);
            return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));

        } else {
            Block<_State, _DefaultState>* victim = pickVictim();

            // Create lookup result now and remember the block state
            LookupResult_p v_lookup(new LookupResult(this, victim, blockAddress(victim), true));
            v_lookup->isHit = false;

            victim->tag() = anAddress;

            lookup->isHit        = true;
            lookup->theOrigState = _DefaultState;
            lookup->theBlock     = victim;
            victim->state()      = _DefaultState;

            recordAccess(victim);
            return v_lookup;
        }
    }

    virtual Block<_State, _DefaultState>* pickVictim()                 = 0;
    virtual bool victimAvailable()                                     = 0;
    virtual void recordAccess(Block<_State, _DefaultState>* aBlock)    = 0;
    virtual void invalidateBlock(Block<_State, _DefaultState>* aBlock) = 0;

    virtual void load_set_from_ckpt(json& is,
                                    int32_t core_idx,
                                    int32_t set_idx,
                                    int32_t tag_shift,
                                    int32_t set_shift) = 0;

    MemoryAddress blockAddress(const Block<_State, _DefaultState>* theBlock) { return theBlock->tag(); }

    int32_t count(const MemoryAddress& aTag)
    {
        int32_t i, res = 0;

        for (i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].tag() == aTag) { res++; }
        }

        return res;
    }

    std::list<MemoryAddress> getTags()
    {
        std::list<MemoryAddress> tag_list;
        for (int32_t i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].state().isValid()) { tag_list.push_back(theBlocks[i].tag()); }
        }
        return tag_list;
    }

  protected:
    Block<_State, _DefaultState>* theBlocks;
    int32_t theAssociativity;

}; // class Set

template<typename _State, const _State& _DefaultState>
class SetLRU : public Set<_State, _DefaultState>
{
  public:
    SetLRU(const int32_t aAssociativity)
      : Set<_State, _DefaultState>(aAssociativity)
    {
        theMRUOrder = new int[aAssociativity];

        for (int32_t i = 0; i < aAssociativity; i++) {
            theMRUOrder[i] = i;
        }
    }

    virtual Block<_State, _DefaultState>* pickVictim()
    {
        for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
            int32_t index = theMRUOrder[i];
            if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected()) {
                return &(Set<_State, _DefaultState>::theBlocks[index]);
            } else if (Set<_State, _DefaultState>::theBlocks[index].state() == _DefaultState) {
                DBG_(Dev,
                     (<< "picking Protected victim in Invalid state: " << std::hex
                      << (Set<_State, _DefaultState>::theBlocks[index].tag())));
                Set<_State, _DefaultState>::theBlocks[index].state().setProtected(false);
                return &(Set<_State, _DefaultState>::theBlocks[index]);
            }
        }
        DBG_Assert(false, (<< "All blocks in the set are protected and valid."));
        return nullptr;
    }
    virtual bool victimAvailable()
    {
        for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
            int32_t index = theMRUOrder[i];
            if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected()) {
                // Found unprotected block
                return true;
            } else if (Set<_State, _DefaultState>::theBlocks[index].state() == _DefaultState) {
                // Found Invalid block
                DBG_(Dev, (<< "Found Invalid block that can be evicted."));
                return true;
            }
        }
        return false;
    }

    virtual void recordAccess(Block<_State, _DefaultState>* aBlock)
    {
        int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
        moveToHead(theBlockNum);
    }

    virtual void invalidateBlock(Block<_State, _DefaultState>* aBlock)
    {
        int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
        moveToTail(theBlockNum);
    }

    virtual void load_set_from_ckpt(json& checkpoint,
                                    int32_t core_idx,
                                    int32_t set_idx,
                                    int32_t tag_shift,
                                    int32_t set_shift)
    {
        for (uint32_t i{ 0 }; i < checkpoint["tags"].at(set_idx).size(); i++) {
            bool dirty    = checkpoint["tags"].at(set_idx).at(i)["dirty"];
            bool writable = checkpoint["tags"].at(set_idx).at(i)["writable"];
            uint64_t tag  = checkpoint["tags"].at(set_idx).at(i)["tag"];

            Set<_State, _DefaultState>::theBlocks[i].tag() = MemoryAddress((tag << tag_shift) | (set_idx << set_shift));
            Set<_State, _DefaultState>::theBlocks[i].state() = _State::bool2state(dirty, writable);
        }

        // reset the MRU order. Least recently used cache line is in the beginning.
        for (int32_t i = 0; i < this->theAssociativity; i++) {
            theMRUOrder[i] = this->theAssociativity - i - 1;
        }
    }

  protected:
    inline int32_t lruListHead(void) { return theMRUOrder[0]; }
    inline int32_t lruListTail(void) { return theMRUOrder[Set<_State, _DefaultState>::theAssociativity - 1]; }

    inline void moveToHead(const SetIndex aBlock)
    {
        int32_t i = 0;
        while (theMRUOrder[i] != aBlock)
            i++;
        while (i > 0) {
            theMRUOrder[i] = theMRUOrder[i - 1];
            i--;
        }
        theMRUOrder[0] = aBlock;
    }

    inline void moveToTail(const SetIndex aBlock)
    {
        int32_t i = 0;
        while (theMRUOrder[i] != aBlock)
            i++;
        while (i < (Set<_State, _DefaultState>::theAssociativity - 1)) {
            theMRUOrder[i] = theMRUOrder[i + 1];
            i++;
        }
        theMRUOrder[Set<_State, _DefaultState>::theAssociativity - 1] = aBlock;
    }

    SetIndex* theMRUOrder;

}; // class SetLRU

template<typename _State, const _State& _DefaultState>
class StdArray : public AbstractArray<_State>
{
  protected:
    int32_t theCacheSize;
    int32_t theAssociativity;
    int32_t theBlockSize;
    int32_t theReplacementPolicy;

    // Masks to address portions of the cache
    int32_t setCount;
    int32_t setIndexShift;
    int32_t setIndexMask;

    int32_t theTagShift; // Used when loading Text flexpoints

    // provided by AbstractArray<_State>
    // MemoryAddress blockOffsetMask;

    Set<_State, _DefaultState>** theSets;

  public:
    virtual ~StdArray() {}
    StdArray(const int32_t aBlockSize, const std::list<std::pair<std::string, std::string>>& theConfiguration)
    {
        theBlockSize = aBlockSize;

        std::list<std::pair<std::string, std::string>>::const_iterator iter = theConfiguration.begin();
        for (; iter != theConfiguration.end(); iter++) {
            if (strcasecmp(iter->first.c_str(), "size") == 0) {
                theCacheSize = strtoull(iter->second.c_str(), nullptr, 0);
            } else if (strcasecmp(iter->first.c_str(), "assoc") == 0 ||
                       strcasecmp(iter->first.c_str(), "associativity") == 0) {
                theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
            } else if (strcasecmp(iter->first.c_str(), "repl") == 0 ||
                       strcasecmp(iter->first.c_str(), "replacement") == 0) {
                if (strcasecmp(iter->second.c_str(), "lru") == 0) {
                    theReplacementPolicy = REPLACEMENT_LRU;
                } else {
                    DBG_Assert(false, (<< "Invalid replacement policy type " << iter->second));
                }
            } else {
                DBG_Assert(false,
                           (<< "Unknown configuration parameter '" << iter->first
                            << "' while creating StdArray. Valid params are: "
                               "size,assoc,repl"));
            }
        }

        init();
    }

    void init()
    {
        int32_t indexBits, blockOffsetBits, i;

        DBG_Assert(theCacheSize > 0);
        DBG_Assert(theAssociativity > 0);
        DBG_Assert(theBlockSize > 0);

        //  Physical address layout:
        //
        //  +---------+-------+-------------------+
        //  | Tag     | Index |    BlockOffset    |
        //  +---------+-------+-------------------+
        //                    |<- setIndexShift ->|
        //            |<----->|
        //             setIndexMask (shifted right)
        //  |<------->|
        //    tagMask
        //            |<------ tagShift --------->|

        // Set indexes and masks
        setCount = theCacheSize / theAssociativity / theBlockSize;

        DBG_Assert((setCount & (setCount - 1)) == 0);

        indexBits       = log_base2(setCount);
        blockOffsetBits = log_base2(theBlockSize);

        AbstractArray<_State>::blockOffsetMask = MemoryAddress((1ULL << blockOffsetBits) - 1);

        setIndexShift = blockOffsetBits;
        setIndexMask  = (1 << indexBits) - 1;

        theTagShift = blockOffsetBits + indexBits;

        // Allocate the sets
        theSets = new Set<_State, _DefaultState>*[setCount];
        DBG_Assert(theSets);

        for (i = 0; i < setCount; i++) {

            switch (theReplacementPolicy) {
                case REPLACEMENT_LRU: theSets[i] = new SetLRU<_State, _DefaultState>(theAssociativity); break;
                default: DBG_Assert(0);
            };
        }
    }

    // Main array lookup function
    virtual boost::intrusive_ptr<AbstractLookupResult<_State>> operator[](const MemoryAddress& anAddress)
    {
        boost::intrusive_ptr<AbstractLookupResult<_State>> ret =
          theSets[makeSet(anAddress)]->lookupBlock(blockAddress(anAddress));
        DBG_(VVerb,
             (<< "Found block " << std::hex << blockAddress(anAddress) << " (" << anAddress << ") in set "
              << makeSet(anAddress) << " in state " << ret->state()));
        return ret;
    }

    virtual bool canAllocate(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup, const MemoryAddress& anAddress)
    {
        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);

        return std_lookup->theSet->canAllocate(std_lookup, blockAddress(anAddress));
    }
    virtual boost::intrusive_ptr<AbstractLookupResult<_State>> allocate(
      boost::intrusive_ptr<AbstractLookupResult<_State>> lookup,
      const MemoryAddress& anAddress)
    {
        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);

        return std_lookup->theSet->allocate(std_lookup, blockAddress(anAddress));
    }

    virtual void recordAccess(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup)
    {
        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);
        DBG_Assert(std_lookup->valid());

        std_lookup->theSet->recordAccess(std_lookup->theBlock);
    }

    virtual void invalidateBlock(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup)
    {
        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);
        DBG_Assert(std_lookup->valid());

        std_lookup->theSet->invalidateBlock(std_lookup->theBlock);
    }

    virtual std::pair<_State, MemoryAddress> getPreemptiveEviction()
    {
        return std::make_pair(_DefaultState, MemoryAddress(0));
    }

    virtual void load_from_ckpt(std::istream& is, int32_t theIndex)
    {
        json checkpoint;
        is >> checkpoint;

        DBG_Assert((uint64_t)theAssociativity == checkpoint["associativity"]);
        DBG_Assert((uint64_t)setCount == checkpoint["tags"].size());

        for (int32_t i{ 0 }; i < setCount; i++) {
            theSets[i]->load_set_from_ckpt(checkpoint, theIndex, i, theTagShift, setIndexShift);
        }
    }

    // Addressing helper functions
    MemoryAddress blockAddress(MemoryAddress const& anAddress) const
    {
        return MemoryAddress(anAddress & ~(AbstractArray<_State>::blockOffsetMask));
    }

    BlockOffset blockOffset(MemoryAddress const& anAddress) const
    {
        return BlockOffset(anAddress & AbstractArray<_State>::blockOffsetMask);
    }

    SetIndex makeSet(const MemoryAddress& anAddress) const { return ((anAddress >> setIndexShift) & setIndexMask); }

    virtual bool sameSet(MemoryAddress a, MemoryAddress b) const { return (makeSet(a) == makeSet(b)); }

    virtual std::list<MemoryAddress> getSetTags(MemoryAddress addr) { return theSets[makeSet(addr)]->getTags(); }

    virtual std::function<bool(MemoryAddress a, MemoryAddress b)> setCompareFn() const
    {
        return std::bind(&StdArray<_State, _DefaultState>::sameSet,
                         *this,
                         std::placeholders::_1,
                         std::placeholders::_2);
    }

    virtual uint64_t getSet(MemoryAddress const& addr) const { return (uint64_t)makeSet(addr); }

    virtual int32_t requestsPerSet() const { return theAssociativity; }

}; // class StdArray

}; // namespace nCache

#endif /* _CACHE_ARRAY_HPP */
