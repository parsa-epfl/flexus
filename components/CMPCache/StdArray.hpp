
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

            DBG_(Trace, (<< "Replacing block " << std::hex << blockAddress(victim) << " in state " << victim->state()));

            // Create lookup result now and remember the block state
            LookupResult_p v_lookup(new LookupResult(this, victim, blockAddress(victim), true));
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

    virtual void load_set_from_ckpt(uint32_t index, uint64_t tag, bool dirty, bool writable) = 0;

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

    virtual bool almostFull(int max_locked) const
    {
        int locked_count    = 0;
        int unlocked_count  = 0;
        int unlocked_thresh = theAssociativity - max_locked;
        for (int32_t i = 0; i < theAssociativity; i++) {
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
        for (int32_t i = 0; i < theAssociativity; i++) {
            if (theBlocks[i].state().isLocked() && !theBlocks[i].state().isProtected()) { return true; }
        }
        return false;
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
            // Can't be protected or locked
            if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected() &&
                !Set<_State, _DefaultState>::theBlocks[index].state().isLocked()) {
                return &(Set<_State, _DefaultState>::theBlocks[index]);
            }
        }
        DBG_Assert(false, (<< "All blocks in the set are protected."));
        return nullptr;
    }

    virtual Block<_State, _DefaultState>* pickLockedVictim()
    {
        for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
            int32_t index = theMRUOrder[i];
            // Can't be protected or locked
            if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected() &&
                Set<_State, _DefaultState>::theBlocks[index].state().isLocked()) {
                return &(Set<_State, _DefaultState>::theBlocks[index]);
            }
        }
        DBG_Assert(false, (<< "All blocks in the set are protected."));
        return nullptr;
    }

    virtual bool victimAvailable()
    {
        for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
            int32_t index = theMRUOrder[i];
            if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected() &&
                !Set<_State, _DefaultState>::theBlocks[index].state().isLocked()) {
                return true;
            }
        }
        return false;
    }

    virtual bool recordAccess(Block<_State, _DefaultState>* aBlock)
    {
        int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
        // If it's already at the head, just return now
        if (theMRUOrder[0] == theBlockNum) { return false; }
        moveToHead(theBlockNum);
        return true;
    }

    virtual void invalidateBlock(Block<_State, _DefaultState>* aBlock)
    {
        int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
        moveToTail(theBlockNum);
    }

    virtual void load_set_from_ckpt(uint32_t index, uint64_t tag, bool dirty, bool writable)
    {
        _State state(_State::bool2state(dirty, writable));
        theMRUOrder[index]                                   = index;
        Set<_State, _DefaultState>::theBlocks[index].tag()   = MemoryAddress(tag);
        Set<_State, _DefaultState>::theBlocks[index].state() = state;
        DBG_(Trace, NoDefaultOps()(<< index << "-LLC: Loading block " << std::hex << tag << " in state " << state));
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
    CMPCacheInfo theInfo;

    // Masks to address portions of the cache
    int32_t theAssociativity;
    int32_t theReplacementPolicy;
    int32_t theBlockSize;
    int32_t theBanks;
    int32_t theTotalBanks;
    int32_t theGlobalBankIndex;
    int32_t theLocalBankIndex;
    int32_t theBankInterleaving;
    int32_t theGroups;
    int32_t theGroupIndex;
    int32_t theGroupInterleaving;

    int32_t theNumSets;
    MemoryAddress tagMask;
    int32_t setHighShift;
    int32_t setMidShift;
    int32_t setLowShift;
    uint64_t setLowMask;
    uint64_t setMidMask;
    uint64_t setHighMask;

    int32_t theBankMask, theBankShift;
    int32_t theGroupMask, theGroupShift;

    Set<_State, _DefaultState>** theSets;

  public:
    virtual ~StdArray() {}
    StdArray(CMPCacheInfo& aCacheInfo,
             const int32_t aBlockSize,
             const std::list<std::pair<std::string, std::string>>& theConfiguration)
      : theInfo(aCacheInfo)
    {
        theBlockSize         = aBlockSize;
        theBanks             = theInfo.theNumBanks;
        theBankInterleaving  = theInfo.theBankInterleaving;
        theGroups            = theInfo.theNumGroups;
        theGroupInterleaving = theInfo.theGroupInterleaving;
        DBG_(Dev, (<< "theGroupInterleaving = " << theGroupInterleaving));

        theGlobalBankIndex = theInfo.theNodeId;
        theLocalBankIndex  = theInfo.theNodeId % theBanks;
        theGroupIndex      = theInfo.theNodeId / theBanks;

        theTotalBanks = theBanks * theGroups;

        std::list<std::pair<std::string, std::string>>::const_iterator iter = theConfiguration.begin();
        for (; iter != theConfiguration.end(); iter++) {
            if (iter->first == "sets") {
                theNumSets = strtoll(iter->second.c_str(), nullptr, 0);
            } else if (iter->first == "total_sets" || iter->first == "global_sets") {
                int32_t global_sets = strtol(iter->second.c_str(), nullptr, 0);
                theNumSets          = global_sets / theTotalBanks;
                DBG_Assert((theNumSets * theTotalBanks) == global_sets,
                           (<< "global_sets (" << global_sets << ") is not divisible by number of banks ("
                            << theTotalBanks << ")"));
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
        DBG_Assert(((theBankInterleaving - 1) & theBankInterleaving) == 0);
        DBG_Assert(((theGroupInterleaving - 1) & theGroupInterleaving) == 0);

        DBG_Assert((theBankInterleaving * theBanks) <= theGroupInterleaving,
                   (<< "Invalid interleaving: BI = " << theBankInterleaving << ", Banks = " << theBanks
                    << ", GI = " << theGroupInterleaving << ", Groups = " << theGroups));

        int32_t blockOffsetBits      = log_base2(theBlockSize);
        int32_t indexBits            = log_base2(theNumSets);
        int32_t bankBits             = log_base2(theBanks);
        int32_t bankInterleavingBits = log_base2(theBankInterleaving);

        int32_t groupBits             = log_base2(theGroups);
        int32_t groupInterleavingBits = log_base2(theGroupInterleaving);

        int32_t lowBits  = bankInterleavingBits - blockOffsetBits;
        int32_t midBits  = groupInterleavingBits - bankInterleavingBits - bankBits;
        int32_t highBits = indexBits - midBits - lowBits;
        if ((midBits + lowBits) > indexBits) {
            midBits  = indexBits - lowBits;
            highBits = 0;
        }

        setLowMask  = (1 << lowBits) - 1;
        setMidMask  = ((1 << midBits) - 1) << lowBits;
        setHighMask = ((1 << highBits) - 1) << (midBits + lowBits);

        setLowShift  = blockOffsetBits;
        setMidShift  = bankBits + blockOffsetBits;
        setHighShift = groupBits + bankBits + blockOffsetBits;

        theBankMask   = theBanks - 1;
        theBankShift  = bankInterleavingBits;
        theGroupMask  = theGroups - 1;
        theGroupShift = groupInterleavingBits;

        DBG_(Dev,
             (<< "blockOffsetBits = " << std::dec << blockOffsetBits << ", indexBits = " << std::dec << indexBits
              << ", bankBits = " << std::dec << bankBits << ", bankInterleavingBits = " << std::dec
              << bankInterleavingBits << ", groupBits = " << std::dec << groupBits
              << ", groupInterleavingBits = " << std::dec << groupInterleavingBits << ", lowBits = " << std::dec
              << lowBits << ", midBits = " << std::dec << midBits << ", highBits = " << std::dec << highBits
              << ", setLowMask = " << std::hex << setLowMask << ", setMidMask = " << std::hex << setMidMask
              << ", setHighMask = " << std::hex << setHighMask << ", setLowShift = " << std::dec << setLowShift
              << ", setMidShift = " << std::dec << setMidShift << ", setHighShift = " << std::dec << setHighShift
              << ", theBankMask = " << std::hex << theBankMask << ", theBankShift = " << std::dec << theBankShift
              << ", theGroupMask = " << std::hex << theGroupMask << ", theGroupShift = " << std::dec << theGroupShift));

        tagMask = MemoryAddress(~0ULL & ~((uint64_t)(theBlockSize - 1)));

        // Allocate the sets
        theSets = new Set<_State, _DefaultState>*[theNumSets];
        DBG_Assert(theSets);

        for (int32_t i = 0; i < theNumSets; i++) {

            switch (theReplacementPolicy) {
                case REPLACEMENT_LRU: theSets[i] = new SetLRU<_State, _DefaultState>(theAssociativity); break;
                default: DBG_Assert(false);
            };
        }
    }

    // Main array lookup function
    virtual boost::intrusive_ptr<AbstractArrayLookupResult<_State>> operator[](const MemoryAddress& anAddress)
    {
        DBG_(Trace,
             (<< "Looking for block " << std::hex << anAddress << " in set " << makeSet(anAddress)
              << " theNumSets = " << theNumSets));
        boost::intrusive_ptr<AbstractArrayLookupResult<_State>> ret =
          theSets[makeSet(anAddress)]->lookupBlock(blockAddress(anAddress));
        DBG_(Trace,
             (<< "Found block " << std::hex << anAddress << " in set " << makeSet(anAddress) << " in state "
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

        return std_lookup->theSet->allocate(std_lookup, blockAddress(anAddress));
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

    virtual void load_cache_from_ckpt(std::string const& filename, int32_t theIndex)
    {

        std::ifstream ifs(filename.c_str(), std::ios::in);

        if (!ifs.good()) {
            DBG_(Dev, (<< "checkpoint file: " << filename << " not found."));
            DBG_Assert(false, (<< "FILE NOT FOUND"));
        }

        json checkpoint;
        ifs >> checkpoint;

        DBG_Assert((uint64_t)theAssociativity == checkpoint["associativity"]);
        DBG_Assert((uint64_t)theNumSets == checkpoint["tags"].size());

        for (int32_t i{ 0 }; i < theNumSets; i++) {
            // if ((getBank(addr) == theLocalBankIndex) && (getGroup(addr) == theGroupIndex))
            for (uint16_t j = 0; j < checkpoint["tags"].at(i).size(); j++) {
                uint64_t tag  = checkpoint["tags"].at(i).at(j)["tag"];
                bool dirty    = checkpoint["tags"].at(i).at(j)["dirty"];
                bool writable = checkpoint["tags"].at(i).at(j)["writable"];

                theSets[i]->load_set_from_ckpt(j, tag, dirty, writable);
            }
        }

        ifs.close();
    }

    // Addressing helper functions
    MemoryAddress blockAddress(MemoryAddress const& anAddress) const { return MemoryAddress(anAddress & tagMask); }

    SetIndex makeSet(const MemoryAddress& anAddress) const
    {
        return ((anAddress >> setLowShift) & setLowMask) | ((anAddress >> setMidShift) & setMidMask) |
               ((anAddress >> setHighShift) & setHighMask);
    }

    int32_t getBank(const MemoryAddress& anAddress) const { return ((anAddress >> theBankShift) & theBankMask); }

    int32_t getGroup(const MemoryAddress& anAddress) const { return ((anAddress >> theGroupShift) & theGroupMask); }

    virtual bool sameSet(MemoryAddress a, MemoryAddress b) { return (makeSet(a) == makeSet(b)); }

    virtual std::list<MemoryAddress> getSetTags(MemoryAddress addr) { return theSets[makeSet(addr)]->getTags(); }
    virtual bool setAlmostFull(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
                               MemoryAddress const& anAddress) const
    {
        return theSets[makeSet(anAddress)]->almostFull(AbstractArray<_State>::theLockedThreshold);
    }

    virtual bool lockedVictimAvailable(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
                                       MemoryAddress const& anAddress) const
    {
        return theSets[makeSet(anAddress)]->lockedVictimAvailable();
    }

    virtual bool victimAvailable(boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
                                 MemoryAddress const& anAddress) const
    {
        return theSets[makeSet(anAddress)]->victimAvailable();
    }

    virtual boost::intrusive_ptr<AbstractArrayLookupResult<_State>> replaceLockedBlock(
      boost::intrusive_ptr<AbstractArrayLookupResult<_State>> lookup,
      MemoryAddress const& anAddress)
    {

        StdLookupResult<_State, _DefaultState>* std_lookup =
          dynamic_cast<StdLookupResult<_State, _DefaultState>*>(lookup.get());
        DBG_Assert(std_lookup != nullptr);

        return std_lookup->theSet->replaceLocked(std_lookup, blockAddress(anAddress));
    }

    virtual void setLockedThreshold(int32_t threshold)
    {
        DBG_Assert(threshold > 0 && threshold < theAssociativity);
        AbstractArray<_State>::theLockedThreshold = threshold;
    }

    virtual uint32_t globalCacheSets() { return theNumSets * theTotalBanks; }

}; // class StdArray

}; // namespace nCMPCache

#endif /* _CMP_CACHE_STDARRAY_HPP */