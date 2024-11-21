
#ifndef _CMP_CACHE_STDARRAY_HPP
#define _CMP_CACHE_STDARRAY_HPP

#include "components/CMPCache/CMPCacheInfo.hpp"
#include "components/CommonQEMU/Serializers.hpp"
#include "components/CommonQEMU/Util.hpp"
#include "core/checkpoint/json.hpp"
#include "core/debug/debug.hpp"
#include "core/target.hpp"
#include "core/types.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/throw_exception.hpp>
#include <fstream>

using nCommonSerializers::BlockSerializer;
using nCommonUtil::log_base2;
using json = nlohmann::json;

namespace nCMPCache {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

typedef uint32_t SetIndex;
typedef uint64_t BlockOffset;

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
class StdLookupResult : public AbstractArrayLookupResult<_State>
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
    bool setState(const _State& aNewState)
    {
        if (theBlock->state() != aNewState) {
            theBlock->state() = aNewState;
            return true;
        }
        return false;
    }
    bool isProtected() const { return (isHit ? theBlock->state().isProtected() : false); }
    void setProtected(bool val) { theBlock->state().setProtected(val); }
    void setPrefetched(bool val) { theBlock->state().setPrefetched(val); }
    void setLocked(bool val) { theBlock->state().setLocked(val); }

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
    Set(const uint64_t aAssociativity)
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
        uint64_t i, t = 0xffffffffffffffffULL;

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
        if (t != 0xffffffffffffffffULL) { return LookupResult_p(new LookupResult(this, &(theBlocks[t]), anAddress, false)); }

        // Miss on this set
        return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));
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
            LookupResult_p v_lookup(new LookupResult(this, victim, this->blockAddress(victim), true));
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

    virtual LookupResult_p replaceLocked(LookupResult_p lookup, MemoryAddress anAddress)
    {
        // First look for an invalid tag match
        if (lookup->theBlock != nullptr && lookup->theBlock->state().isLocked()) {
            lookup->isHit = true;
            DBG_Assert(
              lookup->theBlock->tag() == anAddress,
              (<< "Lookup Tag " << std::hex << (uint64_t)lookup->theBlock->tag() << " != " << (uint64_t)anAddress));
            // don't need to change the lookup, just fix order and return no victim
            recordAccess(lookup->theBlock);
            return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));
        } else {
            bool swap_locked_victim = false;
            if (lookup->theBlock != nullptr) {
                // there's already a block with the right tag, but it's unlocked and we
                // can't go over our threshold so change the address of the current
                // block to avoid problems
                swap_locked_victim = true;
            }

            Block<_State, _DefaultState>* victim = pickLockedVictim();

            DBG_(Trace, (<< "Replacing block " << std::hex << this->blockAddress(victim) << " in state " << victim->state()));

            // Create lookup result now and remember the block state
            LookupResult_p v_lookup(new LookupResult(this, victim, this->blockAddress(victim), true));
            v_lookup->isHit = false;

            if (swap_locked_victim) { lookup->theBlock->tag() = victim->tag(); }
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
    virtual Block<_State, _DefaultState>* pickLockedVictim()           = 0;
    virtual bool victimAvailable()                                     = 0;
    virtual bool recordAccess(Block<_State, _DefaultState>* aBlock)    = 0;
    virtual void invalidateBlock(Block<_State, _DefaultState>* aBlock) = 0;

    virtual void load_set_from_ckpt(uint64_t index, uint64_t mru_index, uint64_t tag, bool dirty, bool writable) = 0;

    MemoryAddress blockAddress(const Block<_State, _DefaultState>* theBlock)
    { 
        return theBlock->tag(); 
    }

    uint64_t count(const MemoryAddress& aTag)
    {
        uint64_t i, res = 0;

        for (i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].tag() == aTag) { res++; }
        }

        return res;
    }

    std::list<MemoryAddress> getTags()
    {
        std::list<MemoryAddress> tag_list;
        for (uint64_t i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].state().isValid()) { tag_list.push_back(theBlocks[i].tag()); }
        }
        return tag_list;
    }

    virtual bool almostFull(int max_locked) const
    {
        int locked_count    = 0;
        int unlocked_count  = 0;
        int unlocked_thresh = theAssociativity - max_locked;
        for (uint64_t i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].state().isLocked()) {
                locked_count++;
                if (locked_count >= max_locked) { return true; }
            } else {
                unlocked_count++;
                if (unlocked_count > unlocked_thresh) { return false; }
            }
        }
        return false;
    }

    virtual bool lockedVictimAvailable() const
    {
        // Look for a block that is Locked, but not protected
        for (uint64_t i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].state().isLocked() && !theBlocks[i].state().isProtected()) { return true; }
        }
        return false;
    }

  protected:
    Block<_State, _DefaultState>* theBlocks;
    SetIndex theAssociativity;

}; // class Set

template<typename _State, const _State& _DefaultState>
class SetLRU : public Set<_State, _DefaultState>
{
  public:
    SetLRU(const uint64_t aAssociativity)
      : Set<_State, _DefaultState>(aAssociativity)
    {
        theMRUOrder = new SetIndex[aAssociativity];

        for (uint64_t i = 0; i < aAssociativity; i++) {
            theMRUOrder[i] = i;
        }
    }

    virtual Block<_State, _DefaultState>* pickVictim()
    {
        for (SetIndex i = this->theAssociativity - 1; i >= 0; i--) {
            SetIndex index = theMRUOrder[i];
            // Can't be protected or locked
            if (!this->theBlocks[index].state().isProtected() &&
                !this->theBlocks[index].state().isLocked()) {
                return &(this->theBlocks[index]);
            }
        }
        DBG_Assert(false, (<< "All blocks in the set are protected."));
        return nullptr;
    }

    virtual Block<_State, _DefaultState>* pickLockedVictim()
    {
        for (SetIndex i = this->theAssociativity - 1; i >= 0; i--) {
            SetIndex index = theMRUOrder[i];
            // Can't be protected or locked
            if (!this->theBlocks[index].state().isProtected() &&
                this->theBlocks[index].state().isLocked()) {
                return &(this->theBlocks[index]);
            }
        }
        DBG_Assert(false, (<< "All blocks in the set are protected."));
        return nullptr;
    }

    virtual bool victimAvailable()
    {
        for (SetIndex i = this->theAssociativity - 1; i >= 0; i--) {
            SetIndex index = theMRUOrder[i];
            if (!this->theBlocks[index].state().isProtected() &&
                !this->theBlocks[index].state().isLocked()) {
                return true;
            }
        }
        return false;
    }

    virtual bool recordAccess(Block<_State, _DefaultState>* aBlock)
    {
        SetIndex theBlockNum = aBlock - this->theBlocks;
        // If it's already at the head, just return now
        if (theMRUOrder[0] == theBlockNum) { return false; }
        moveToHead(theBlockNum);
        return true;
    }

    virtual void invalidateBlock(Block<_State, _DefaultState>* aBlock)
    {
        SetIndex theBlockNum = aBlock - this->theBlocks;
        moveToTail(theBlockNum);
    }

    virtual void load_set_from_ckpt(uint64_t index, uint64_t mru_order_index, uint64_t tag, bool dirty, bool writable)
    {
        
        DBG_Assert(index < uint64_t(this->theAssociativity));
        _State state(_State::bool2state(dirty, writable));
        theMRUOrder[index]                                   = mru_order_index;
        this->theBlocks[index].tag()   = MemoryAddress(tag);
        this->theBlocks[index].state() = state;
        DBG_(Trace, NoDefaultOps()(<< index << "-LLC: Loading block " << std::hex << tag << " in state " << state));
    }

  protected:
    inline SetIndex lruListHead(void) { return theMRUOrder[0]; }
    inline SetIndex lruListTail(void) { return theMRUOrder[this->theAssociativity - 1]; }

    inline void moveToHead(const SetIndex aBlock)
    {
        SetIndex i = 0;
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
        SetIndex i = 0;
        while (theMRUOrder[i] != aBlock)
            i++;
        while (i < (this->theAssociativity - 1)) {
            theMRUOrder[i] = theMRUOrder[i + 1];
            i++;
        }
        theMRUOrder[this->theAssociativity - 1] = aBlock;
    }

    SetIndex* theMRUOrder;

}; // class SetLRU

template<typename _State, const _State& _DefaultState>
class StdArray : public AbstractArray<_State>
{
  protected:
    CMPCacheInfo theInfo;

    // Masks to address portions of the cache
    uint64_t theAssociativity;
    uint64_t theReplacementPolicy;
    uint64_t theBlockSize;

    uint64_t theNumSets;
    uint64_t theSetIndexShift;
    uint64_t theSetIndexMask;

    uint64_t theNumNodes;

    MemoryAddress theTagMask;

    Set<_State, _DefaultState>** theSets;

  public:
    virtual ~StdArray() {}
    StdArray(CMPCacheInfo& aCacheInfo,
             const uint64_t aBlockSize,
             const std::list<std::pair<std::string, std::string>>& theConfiguration)
      : theInfo(aCacheInfo)
    {
        theBlockSize         = aBlockSize;

        // Currently, we only test with 64-byte blocks
        // A larger block can cause wrong interleaving in the LLC.
        DBG_Assert(theBlockSize == 64);

        theNumNodes = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
        DBG_Assert(theNumNodes > 0);

        std::list<std::pair<std::string, std::string>>::const_iterator iter = theConfiguration.begin();
        for (; iter != theConfiguration.end(); iter++) {
            if (iter->first == "sets") {
                theNumSets = strtoll(iter->second.c_str(), nullptr, 0);
            } else if (iter->first == "total_sets") {
                uint64_t total_sets = strtoll(iter->second.c_str(), nullptr, 0);
                DBG_Assert(total_sets % theNumNodes == 0);
                theNumSets = total_sets / theNumNodes;
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
                               "sets, total_sets, assoc, repl"));
            }
        }

        init();
    }

    void init()
    {
        DBG_Assert(theNumSets > 0);
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
        DBG_Assert((theNumSets & (theNumSets - 1)) == 0);
        DBG_Assert(((theBlockSize - 1) & theBlockSize) == 0);

        uint64_t blockOffsetBits      = log_base2(theBlockSize);
        // int32_t indexBits            = log_base2(theNumSets);
        this->theSetIndexShift       = blockOffsetBits;
        this->theSetIndexMask        = (theNumSets - 1); // mask is applied after shift.

        this->theTagMask = MemoryAddress(~0ULL & ~((uint64_t)(theBlockSize - 1)));

        // Allocate the sets
        theSets = new Set<_State, _DefaultState>*[theNumSets];
        DBG_Assert(theSets);

        for (uint64_t i = 0; i < theNumSets; i++) {

            switch (theReplacementPolicy) {
                case REPLACEMENT_LRU: theSets[i] = new SetLRU<_State, _DefaultState>(theAssociativity); 
                break;
                default: DBG_Assert(false);
            };
        }

        DBG_(Crit, (<< "Created CMP Cache StdArray with " << theNumSets << " sets, " << theAssociativity << " ways"));
    }

    // Main array lookup function
    virtual boost::intrusive_ptr<AbstractArrayLookupResult<_State>> operator[](const MemoryAddress& anAddress)
    {
        uint64_t set_number = this->makeSet(anAddress);

        DBG_(Trace,
             (<< "Looking for block " << std::hex << anAddress << " in set " << set_number
              << " theNumSets = " << theNumSets));

        DBG_Assert(set_number < theNumSets && set_number >= 0);
        boost::intrusive_ptr<AbstractArrayLookupResult<_State>> ret =
          theSets[set_number]->lookupBlock(this->blockAddress(anAddress));
        DBG_(Trace,
             (<< "Found block " << std::hex << anAddress << " in set " << this->makeSet(anAddress) << " in state "
              << ret->state()));
        return ret;
    }

    virtual boost::intrusive_ptr<AbstractArrayLookupResult<_State>> allocate(
      boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
      const MemoryAddress& anAddress)
    {
        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);

        return std_lookup->theSet->allocate(std_lookup, this->blockAddress(anAddress));
    }

    virtual bool recordAccess(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup)
    {
        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);
        DBG_Assert(std_lookup->valid());

        return std_lookup->theSet->recordAccess(std_lookup->theBlock);
    }

    virtual void invalidateBlock(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup)
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

    // Checkpoint reading/writing functions

    virtual void load_cache_from_ckpt(std::string const& filename, uint64_t theIndex)
    {

        std::ifstream ifs(filename.c_str(), std::ios::in);

        if (!ifs.good()) {
            DBG_(Dev, (<< "checkpoint file: " << filename << " not found."));
            DBG_Assert(false, (<< "FILE NOT FOUND"));
        }

        json checkpoint;
        ifs >> checkpoint;

        DBG_Assert((uint64_t)theAssociativity == checkpoint["associativity"]);
        DBG_Assert((uint64_t)theNumSets == (checkpoint["tags"].size() / theNumNodes)); // Only load one slice.

        for (uint64_t i = 0; i < theNumSets; i++) {
            // Only load the set that corresponds to this slice.
            if (i % theNumNodes != theIndex) {
                continue;
            }

            assert(checkpoint["tags"].at(i).size() <= (uint64_t)theAssociativity);
            for (uint64_t j = 0; j < checkpoint["tags"].at(i).size(); j++) {
                uint64_t tag  = checkpoint["tags"].at(i).at(j)["tag"];
                bool dirty    = checkpoint["tags"].at(i).at(j)["dirty"];
                bool writable = checkpoint["tags"].at(i).at(j)["writable"];

                theSets[i]->load_set_from_ckpt(j, theAssociativity - j - 1,tag, dirty, writable); // the last element is the most recently used cacheline.
            }
        }

        ifs.close();
    }

    // Addressing helper functions
    MemoryAddress blockAddress(MemoryAddress const& anAddress) const 
    { 
        return MemoryAddress(anAddress & this->theTagMask); 
    }

    SetIndex makeSet(const MemoryAddress& anAddress) const
    {
        return ((anAddress >> this->theSetIndexShift) & this->theSetIndexMask);
    }

    virtual bool sameSet(MemoryAddress a, MemoryAddress b) { return (this->makeSet(a) == this->makeSet(b)); }

    virtual std::list<MemoryAddress> getSetTags(MemoryAddress addr) { return theSets[this->makeSet(addr)]->getTags(); }
    virtual bool setAlmostFull(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
                               MemoryAddress const& anAddress) const
    {
        return theSets[this->makeSet(anAddress)]->almostFull(AbstractArray<_State>::theLockedThreshold);
    }

    virtual bool lockedVictimAvailable(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
                                       MemoryAddress const& anAddress) const
    {
        return theSets[this->makeSet(anAddress)]->lockedVictimAvailable();
    }

    virtual bool victimAvailable(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
                                 MemoryAddress const& anAddress) const
    {
        return theSets[this->makeSet(anAddress)]->victimAvailable();
    }

    virtual boost::intrusive_ptr<AbstractArrayLookupResult<_State>> replaceLockedBlock(
      boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
      MemoryAddress const& anAddress)
    {

        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);

        return std_lookup->theSet->replaceLocked(std_lookup, this->blockAddress(anAddress));
    }

    virtual void setLockedThreshold(uint64_t threshold)
    {
        DBG_Assert(threshold > 0 && threshold < theAssociativity);
        AbstractArray<_State>::theLockedThreshold = threshold;
    }
}; // class StdArray

}; // namespace nCMPCache

#endif /* _CMP_CACHE_STDARRAY_HPP */