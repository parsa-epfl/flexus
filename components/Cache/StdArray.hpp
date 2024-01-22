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

#ifndef _CACHE_ARRAY_HPP
#define _CACHE_ARRAY_HPP

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <exception>
#include <iostream>
#include <stdlib.h>

#include <boost/dynamic_bitset.hpp>
#include <boost/throw_exception.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/CommonQEMU/Serializers.hpp>
#include <components/CommonQEMU/Util.hpp>

using nCommonSerializers::BlockSerializer;
using nCommonUtil::log_base2;

namespace nCache {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

typedef int32_t SetIndex;
typedef uint32_t BlockOffset;

enum ReplacementPolicy { REPLACEMENT_LRU, REPLACEMENT_PLRU };

// This is a cache block.  The accessor functions are braindead simple.
template <typename _State, const _State &_DefaultState> class Block {
public:
  Block(void) : theTag(0), theState(_DefaultState) {
  }

  MemoryAddress tag(void) const {
    return theTag;
  }

  MemoryAddress &tag(void) {
    return theTag;
  }

  const _State &state(void) const {
    return theState;
  }
  _State &state(void) {
    return theState;
  }

  bool valid() {
    return theState != _DefaultState;
  }

private:
  MemoryAddress theTag;
  _State theState;

}; // class Block

template <typename _State, const _State &_DefaultState> class Set;
template <typename _State, const _State &_DefaultState> class StdArray;

// The output of a cache lookup
template <typename _State, const _State &_DefaultState>
class StdLookupResult : public AbstractLookupResult<_State> {
public:
  virtual ~StdLookupResult() {
  }
  StdLookupResult(Set<_State, _DefaultState> *aSet, Block<_State, _DefaultState> *aBlock,
                  MemoryAddress aBlockAddress, bool aIsHit)
      : theSet(aSet), theBlock(aBlock), theBlockAddress(aBlockAddress), isHit(aIsHit),
        theOrigState(aIsHit ? aBlock->state() : _DefaultState) {
    DBG_Assert((aBlock == nullptr) || (aBlock->tag() == aBlockAddress),
               (<< "Created Lookup where tag " << std::hex << (uint64_t)aBlock->tag()
                << " != address " << (uint64_t)aBlockAddress));
  }

  const _State &state() const {
    return (isHit ? theBlock->state() : theOrigState);
  }
  void setState(const _State &aNewState) {
    theBlock->state() = aNewState;
  }
  void setProtected(bool val) {
    theBlock->state().setProtected(val);
  }
  void setPrefetched(bool val) {
    theBlock->state().setPrefetched(val);
  }

  bool hit(void) const {
    return isHit;
  }
  bool miss(void) const {
    return !isHit;
  }
  bool found(void) const {
    return theBlock != nullptr;
  }
  bool valid(void) const {
    return theBlock != nullptr;
  }

  MemoryAddress blockAddress(void) const {
    return theBlockAddress;
  }

protected:
  Set<_State, _DefaultState> *theSet;
  Block<_State, _DefaultState> *theBlock;
  MemoryAddress theBlockAddress;
  bool isHit;
  _State theOrigState;

  friend class Set<_State, _DefaultState>;
  friend class StdArray<_State, _DefaultState>;
}; // class LookupResult

// The cache set, implements a set of cache blocks and applies a replacement
// policy (from a derived class)
template <typename _State, const _State &_DefaultState> class Set {
public:
  Set(const int32_t aAssociativity) {
    theAssociativity = aAssociativity;
    theBlocks = new Block<_State, _DefaultState>[theAssociativity];
    DBG_Assert(theBlocks);
  }
  virtual ~Set() {
  }

  typedef StdLookupResult<_State, _DefaultState> LookupResult;
  typedef boost::intrusive_ptr<LookupResult> LookupResult_p;

  LookupResult_p lookupBlock(const MemoryAddress anAddress) {
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
    if (t >= 0) {
      return LookupResult_p(new LookupResult(this, &(theBlocks[t]), anAddress, false));
    }

    // Miss on this set
    return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));
  }

  virtual bool canAllocate(LookupResult_p lookup, MemoryAddress anAddress) {
    // First look for an invalid tag match
    if (lookup->theBlock != nullptr) {
      return true;
    } else {
      return victimAvailable();
    }
  }
  virtual LookupResult_p allocate(LookupResult_p lookup, MemoryAddress anAddress) {
    // First look for an invalid tag match
    if (lookup->theBlock != nullptr) {
      lookup->isHit = true;
      DBG_Assert(lookup->theBlock->tag() == anAddress,
                 (<< "Lookup Tag " << std::hex << (uint64_t)lookup->theBlock->tag()
                  << " != " << (uint64_t)anAddress));
      // don't need to change the lookup, just fix order and return no victim
      recordAccess(lookup->theBlock);
      return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));

    } else {
      Block<_State, _DefaultState> *victim = pickVictim();

      // Create lookup result now and remember the block state
      LookupResult_p v_lookup(new LookupResult(this, victim, blockAddress(victim), true));
      v_lookup->isHit = false;

      victim->tag() = anAddress;

      lookup->isHit = true;
      lookup->theOrigState = _DefaultState;
      lookup->theBlock = victim;
      victim->state() = _DefaultState;

      recordAccess(victim);
      return v_lookup;
    }
  }

  virtual Block<_State, _DefaultState> *pickVictim() = 0;
  virtual bool victimAvailable() = 0;
  virtual void recordAccess(Block<_State, _DefaultState> *aBlock) = 0;
  virtual void invalidateBlock(Block<_State, _DefaultState> *aBlock) = 0;

  virtual bool saveState(std::ostream &s) = 0;
  virtual bool loadState(boost::archive::binary_iarchive &ia, int32_t theIndex, int32_t theSet) = 0;
  virtual bool loadTextState(std::istream &is, int32_t theIndex, int32_t theSet,
                             int32_t theTagShift, int32_t theSetShift) = 0;

  MemoryAddress blockAddress(const Block<_State, _DefaultState> *theBlock) {
    return theBlock->tag();
  }

  int32_t count(const MemoryAddress &aTag) {
    int32_t i, res = 0;

    for (i = 0; i < theAssociativity; i++) {
      if (theBlocks[i].tag() == aTag) {
        res++;
      }
    }

    return res;
  }

  std::list<MemoryAddress> getTags() {
    std::list<MemoryAddress> tag_list;
    for (int32_t i = 0; i < theAssociativity; i++) {
      if (theBlocks[i].state().isValid()) {
        tag_list.push_back(theBlocks[i].tag());
      }
    }
    return tag_list;
  }

protected:
  Block<_State, _DefaultState> *theBlocks;
  int32_t theAssociativity;

}; // class Set

template <typename _State, const _State &_DefaultState>
class SetLRU : public Set<_State, _DefaultState> {
public:
  SetLRU(const int32_t aAssociativity) : Set<_State, _DefaultState>(aAssociativity) {
    theMRUOrder = new int[aAssociativity];

    for (int32_t i = 0; i < aAssociativity; i++) {
      theMRUOrder[i] = i;
    }
  }

  virtual Block<_State, _DefaultState> *pickVictim() {
    for (int32_t i = Set<_State, _DefaultState>::theAssociativity - 1; i >= 0; i--) {
      int32_t index = theMRUOrder[i];
      if (!Set<_State, _DefaultState>::theBlocks[index].state().isProtected()) {
        return &(Set<_State, _DefaultState>::theBlocks[index]);
      } else if (Set<_State, _DefaultState>::theBlocks[index].state() == _DefaultState) {
        DBG_(Dev, (<< "picking Protected victim in Invalid state: " << std::hex
                   << (Set<_State, _DefaultState>::theBlocks[index].tag())));
        Set<_State, _DefaultState>::theBlocks[index].state().setProtected(false);
        return &(Set<_State, _DefaultState>::theBlocks[index]);
      }
    }
    DBG_Assert(false, (<< "All blocks in the set are protected and valid."));
    return nullptr;
  }
  virtual bool victimAvailable() {
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

  virtual void recordAccess(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    moveToHead(theBlockNum);
  }

  virtual void invalidateBlock(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    moveToTail(theBlockNum);
  }

  virtual bool saveState(std::ostream &s) {
    return true;
  }

  virtual bool loadState(boost::archive::binary_iarchive &ia, int32_t theIndex, int32_t theSet) {
    BlockSerializer bs;
    for (int32_t i = (Set<_State, _DefaultState>::theAssociativity - 1); i >= 0; i--) {
      ia >> bs;
      DBG_(Trace, Addr(bs.tag)(<< theIndex << "-L2: Loading block " << std::hex << bs.tag
                               << " in state " << (uint16_t)bs.state << " in way "
                               << (uint16_t)bs.way << " in set " << theSet));
      _State state(_State::char2State(bs.state));
      // int32_t way = bs.way;
      MemoryAddress tag = MemoryAddress(bs.tag);
      theMRUOrder[i] = i;
      Set<_State, _DefaultState>::theBlocks[i].tag() = tag;
      Set<_State, _DefaultState>::theBlocks[i].state() = state;
    }
    return false;
  }

  virtual bool loadTextState(std::istream &is, int32_t theIndex, int32_t theSet,
                             int32_t theTagShift, int32_t theSetShift) {

    char paren;
    int32_t dummy;
    //    int32_t load_state;
    //    uint64_t load_tag;
    is >> paren; // {
    if (paren != '{') {
      DBG_(Crit, (<< "Expected '{' when loading checkpoint, read '" << paren << "' instead"));
      return true;
    }

    for (int32_t i = 0; i < Set<_State, _DefaultState>::theAssociativity; i++) {
      uint64_t tag;
      int32_t state;
      is >> paren >> state >> tag >> paren;
      Set<_State, _DefaultState>::theBlocks[i].tag() =
          MemoryAddress((tag << theTagShift) | (theSet << theSetShift));
      Set<_State, _DefaultState>::theBlocks[i].state() = _State::int2State(state);
      DBG_(Trace, (<< theIndex << "-L?: Loading block " << std::hex << tag << ", in state "
                   << (Set<_State, _DefaultState>::theBlocks[i].state())));
    }

    is >> paren; // }
    if (paren != '}') {
      DBG_(Crit, (<< "Expected '}' when loading checkpoint"));
      return true;
    }

    // useless associativity information
    is >> paren; // <
    if (paren != '<') {
      DBG_(Crit, (<< "Expected '<' when loading checkpoint"));
      return false;
    }
    for (int32_t j = 0; j < Set<_State, _DefaultState>::theAssociativity; j++) {
      is >> dummy;
    }
    is >> paren; // >
    if (paren != '>') {
      DBG_(Crit, (<< "Expected '>' when loading checkpoint"));
      return false;
    }

    return false;
  }

protected:
  inline int32_t lruListHead(void) {
    return theMRUOrder[0];
  }
  inline int32_t lruListTail(void) {
    return theMRUOrder[Set<_State, _DefaultState>::theAssociativity - 1];
  }

  inline void moveToHead(const SetIndex aBlock) {
    int32_t i = 0;
    while (theMRUOrder[i] != aBlock)
      i++;
    while (i > 0) {
      theMRUOrder[i] = theMRUOrder[i - 1];
      i--;
    }
    theMRUOrder[0] = aBlock;
  }

  inline void moveToTail(const SetIndex aBlock) {
    int32_t i = 0;
    while (theMRUOrder[i] != aBlock)
      i++;
    while (i < (Set<_State, _DefaultState>::theAssociativity - 1)) {
      theMRUOrder[i] = theMRUOrder[i + 1];
      i++;
    }
    theMRUOrder[Set<_State, _DefaultState>::theAssociativity - 1] = aBlock;
  }

  SetIndex *theMRUOrder;

}; // class SetLRU

template <typename _State, const _State &_DefaultState>
class SetPseudoLRU : public Set<_State, _DefaultState> {
public:
  SetPseudoLRU(const int32_t anAssociativity)
      : Set<_State, _DefaultState>(anAssociativity),
        theMRUBits(anAssociativity, ((1ULL << anAssociativity) - 1)) {
  }

  virtual Block<_State, _DefaultState> *pickVictim() {
    DBG_Assert(false);
    // TODO: Fix this to check protected bits!!
    // First look for "invalid block", ie. block in DefaultState
    for (int32_t i = 0; i < Set<_State, _DefaultState>::theAssociativity; i++) {
      if (Set<_State, _DefaultState>::theBlocks[i].state() == _DefaultState) {
        return &Set<_State, _DefaultState>::theBlocks[i];
      }
    }

    int32_t theBlockNum = theMRUBits.find_first();
    return Set<_State, _DefaultState>::theBlocks + theBlockNum;
  }

  virtual bool victimAvailable() {
    DBG_Assert(false);
    // First look for "invalid block", ie. block in DefaultState
    for (int32_t i = 0; i < Set<_State, _DefaultState>::theAssociativity; i++) {
      if (Set<_State, _DefaultState>::theBlocks[i].state() == _DefaultState) {
        return true;
      }
    }

    return true;
    // TODO: Fix this to check protected bits!!
  }

  virtual void recordAccess(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    theMRUBits[theBlockNum] = 0;
    if (theMRUBits.none()) {
      theMRUBits.flip();
      theMRUBits[theBlockNum] = 0;
    }
  }

  virtual void invalidateBlock(Block<_State, _DefaultState> *aBlock) {
    int32_t theBlockNum = aBlock - Set<_State, _DefaultState>::theBlocks;
    theMRUBits[theBlockNum] = 1;
  }

  virtual bool saveState(std::ostream &s) {
    return true;
  }

  virtual bool loadState(boost::archive::binary_iarchive &ia, int32_t theIndex, int32_t theSet) {
    return true;
  }

  virtual bool loadTextState(std::istream &is, int32_t theIndex, int32_t theSet, int32_t theTgShift,
                             int32_t theSetShift) {
    return true;
  }

protected:
  // This uses one bit per bool, pretty space-efficient
  boost::dynamic_bitset<> theMRUBits;

}; // class SetPseudoLRU

template <typename _State, const _State &_DefaultState>
class StdArray : public AbstractArray<_State> {
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

  Set<_State, _DefaultState> **theSets;

public:
  virtual ~StdArray() {
  }
  StdArray(const int32_t aBlockSize,
           const std::list<std::pair<std::string, std::string>> &theConfiguration) {
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
        } else if (strcasecmp(iter->second.c_str(), "plru") == 0 ||
                   strcasecmp(iter->second.c_str(), "pseudoLRU") == 0) {
          theReplacementPolicy = REPLACEMENT_PLRU;
        } else {
          DBG_Assert(false, (<< "Invalid replacement policy type " << iter->second));
        }
      } else {
        DBG_Assert(false, (<< "Unknown configuration parameter '" << iter->first
                           << "' while creating StdArray. Valid params are: "
                              "size,assoc,repl"));
      }
    }

    init();
  }

  void init() {
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

    indexBits = log_base2(setCount);
    blockOffsetBits = log_base2(theBlockSize);

    AbstractArray<_State>::blockOffsetMask = MemoryAddress((1ULL << blockOffsetBits) - 1);

    setIndexShift = blockOffsetBits;
    setIndexMask = (1 << indexBits) - 1;

    theTagShift = blockOffsetBits + indexBits;

    // Allocate the sets
    theSets = new Set<_State, _DefaultState> *[setCount];
    DBG_Assert(theSets);

    for (i = 0; i < setCount; i++) {

      switch (theReplacementPolicy) {
      case REPLACEMENT_LRU:
        theSets[i] = new SetLRU<_State, _DefaultState>(theAssociativity);
        break;
      case REPLACEMENT_PLRU:
        theSets[i] = new SetPseudoLRU<_State, _DefaultState>(theAssociativity);
        break;
      default:
        DBG_Assert(0);
      };
    }
  }

  // Main array lookup function
  virtual boost::intrusive_ptr<AbstractLookupResult<_State>>
  operator[](const MemoryAddress &anAddress) {
    boost::intrusive_ptr<AbstractLookupResult<_State>> ret =
        theSets[makeSet(anAddress)]->lookupBlock(blockAddress(anAddress));
    DBG_(Trace, (<< "Found block " << std::hex << blockAddress(anAddress) << " (" << anAddress
                 << ") in set " << makeSet(anAddress) << " in state " << ret->state()));
    return ret;
  }

  virtual bool canAllocate(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup,
                           const MemoryAddress &anAddress) {
    StdLookupResult<_State, _DefaultState> *std_lookup =
        dynamic_cast<StdLookupResult<_State, _DefaultState> *>(lookup.get());
    DBG_Assert(std_lookup != nullptr);

    return std_lookup->theSet->canAllocate(std_lookup, blockAddress(anAddress));
  }
  virtual boost::intrusive_ptr<AbstractLookupResult<_State>>
  allocate(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup,
           const MemoryAddress &anAddress) {
    StdLookupResult<_State, _DefaultState> *std_lookup =
        dynamic_cast<StdLookupResult<_State, _DefaultState> *>(lookup.get());
    DBG_Assert(std_lookup != nullptr);

    return std_lookup->theSet->allocate(std_lookup, blockAddress(anAddress));
  }

  virtual void recordAccess(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup) {
    StdLookupResult<_State, _DefaultState> *std_lookup =
        dynamic_cast<StdLookupResult<_State, _DefaultState> *>(lookup.get());
    DBG_Assert(std_lookup != nullptr);
    DBG_Assert(std_lookup->valid());

    std_lookup->theSet->recordAccess(std_lookup->theBlock);
  }

  virtual void invalidateBlock(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup) {
    StdLookupResult<_State, _DefaultState> *std_lookup =
        dynamic_cast<StdLookupResult<_State, _DefaultState> *>(lookup.get());
    DBG_Assert(std_lookup != nullptr);
    DBG_Assert(std_lookup->valid());

    std_lookup->theSet->invalidateBlock(std_lookup->theBlock);
  }

  virtual std::pair<_State, MemoryAddress> getPreemptiveEviction() {
    return std::make_pair(_DefaultState, MemoryAddress(0));
  }

  // Checkpoint reading/writing functions
  virtual bool saveState(std::ostream &s) {

    // NOT IMPLEMENTED!
    return true;
  }

  virtual bool loadState(std::istream &s, int32_t theIndex, bool text) {

    if (text) {
      for (int32_t i = 0; i < setCount; i++) {
        if (theSets[i]->loadTextState(s, theIndex, i, theTagShift, setIndexShift)) {
          DBG_(Crit, (<< " Error loading state for set: line number " << i));
          return true; // true == error
        }
      }
    } else {
      boost::archive::binary_iarchive ia(s);

      uint64_t set_count = 0;
      uint32_t associativity = 0;

      ia >> set_count;
      ia >> associativity;

      DBG_Assert(set_count == (uint64_t)setCount,
                 (<< "Error loading cache state. Flexpoint contains " << set_count
                  << " sets but simulator configured for " << setCount << " sets."));
      DBG_Assert(associativity == (uint64_t)theAssociativity,
                 (<< "Error loading cache state. Flexpoint contains " << associativity
                  << "-way sets but simulator configured for " << theAssociativity
                  << "-way sets."));

      for (int32_t i = 0; i < setCount; i++) {
        if (theSets[i]->loadState(ia, theIndex, i)) {
          DBG_(Crit, (<< " Error loading state for set: line number " << i));
          return true; // true == error
        }
      }
    }
    return false; // false == no errors
  }

  // Addressing helper functions
  MemoryAddress blockAddress(MemoryAddress const &anAddress) const {
    return MemoryAddress(anAddress & ~(AbstractArray<_State>::blockOffsetMask));
  }

  BlockOffset blockOffset(MemoryAddress const &anAddress) const {
    return BlockOffset(anAddress & AbstractArray<_State>::blockOffsetMask);
  }

  SetIndex makeSet(const MemoryAddress &anAddress) const {
    return ((anAddress >> setIndexShift) & setIndexMask);
  }

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) const {
    return (makeSet(a) == makeSet(b));
  }

  virtual std::list<MemoryAddress> getSetTags(MemoryAddress addr) {
    return theSets[makeSet(addr)]->getTags();
  }

  virtual std::function<bool(MemoryAddress a, MemoryAddress b)> setCompareFn() const {
    // return [this](auto a, auto b){ return this->sameSet(a, b); };//std::bind(
    // &StdArray<_State, _DefaultState>::sameSet, *this, _1, _2);
    return std::bind(&StdArray<_State, _DefaultState>::sameSet, *this, std::placeholders::_1, std::placeholders::_2);
  }

  virtual uint64_t getSet(MemoryAddress const &addr) const {
    return (uint64_t)makeSet(addr);
  }

  virtual int32_t requestsPerSet() const {
    return theAssociativity;
  }

}; // class StdArray

}; // namespace nCache

#endif /* _CACHE_ARRAY_HPP */
