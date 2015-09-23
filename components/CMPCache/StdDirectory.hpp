#ifndef __STD_DIRECTORY_HPP__
#define __STD_DIRECTORY_HPP__

#include <strings.h>

#include <components/Common/Util.hpp>

#include <components/Common/Serializers.hpp>

using nCommonUtil::log_base2;
using nCommonSerializers::StdDirEntrySerializer;

namespace nCMPCache {

typedef uint64_t Tag;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

template<typename _State, typename _EState = _State>
class StdDirectory : public AbstractDirectory<_State, _EState> {
private:

  class Block {
  public:
    Block() : theTag(0), theState(0) {}
    Tag tag() const {
      return (theTag & ~1ULL);
    }
    _State & state() {
      return theState;
    }

    void setTag(Tag aTag) {
      theTag = (theTag & 1ULL) | (aTag & ~1ULL);
    }

    void setState(const _State & aState) {
      theState = aState;
    }

    void setProtected(bool val) {
      if (val) {
        theTag |= 1ULL;
      } else {
        theTag &= ~1ULL;
      }
    }

    Block & operator=(const StdDirEntrySerializer & serializer) {
      setTag(serializer.tag);
      theState = serializer.state;
      return *this;
    }

    bool isProtected() {
      return ((theTag & 1ULL) != 0);
    }

  private:
    Tag theTag;
    _State theState;
  };

  class Set;

  class StdLookupResult : public AbstractLookupResult<_State> {
  public:
    virtual ~StdLookupResult() {}
    StdLookupResult(Set  *  aSet,
                    Block  * aBlock,
                    MemoryAddress anAddress,
                    bool   anIsValid)
      : theSet(aSet)
      , theBlock(aBlock)
      , theAddress(anAddress)
      , isValid(anIsValid)
    { }

    virtual bool found() {
      return isValid;
    }
    virtual bool isProtected() {
      return theBlock->isProtected();
    }
    virtual void setProtected(bool val) {
      theBlock->setProtected(val);
    }
    virtual const _State & state() const {
      return theBlock->state();
    }
    virtual void addSharer(int32_t sharer) {
      theBlock->state().addSharer(sharer);
    }
    virtual void removeSharer(int32_t sharer) {
      theBlock->state().removeSharer(sharer);
    }
    virtual void setSharer(int32_t sharer) {
      theBlock->state().setSharer(sharer);
    }
    virtual void setState(const _State & state) {
      theBlock->state() = state;
    }

  private:
    Set  *  theSet;
    Block  * theBlock;
    MemoryAddress theAddress;
    bool   isValid;

    friend class Set;
    friend class StdDirectory;
  };

  class Set {
  private:
    Block  * theBlocks;
    int    theAssociativity;

    friend class StdDirectory;

  public:
    Set(int32_t anAssociativity, int32_t aNumSharers) : theAssociativity(anAssociativity) {
      theBlocks = new Block[theAssociativity];
      for (int32_t way = 0; way < theAssociativity; way++) {
        theBlocks[way].state().reset(aNumSharers);
      }
    }
    virtual ~Set() {}

    typedef StdLookupResult LookupResult;
    typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

    LookupResult_p lookup(MemoryAddress anAddress) {
      int32_t i;
      for (i = 0; i < theAssociativity; i++) {
        if (theBlocks[i].tag() == (uint64_t)anAddress) {
          return LookupResult_p(new LookupResult(this, &(theBlocks[i]), anAddress, true));
        }
      }
      return LookupResult_p(new LookupResult(this, nullptr, anAddress, false));
    }

    virtual boost::tuple<bool, bool, MemoryAddress, _State> allocate(LookupResult_p lookup, MemoryAddress anAddress, const _State & aState) {
      int32_t i = pickVictim();
      if (i < 0) {
        return boost::make_tuple(false, false, anAddress, aState);
      }
      MemoryAddress v_addr(theBlocks[i].tag());
      _State v_state(theBlocks[i].state());
      theBlocks[i].setTag(anAddress);
      theBlocks[i].setState(aState);
      lookup->theBlock = &(theBlocks[i]);
      lookup->isValid = true;
      lookup->theAddress = anAddress;
      return boost::make_tuple(true, !v_state.noSharers(), v_addr, v_state);
    }

    virtual int32_t pickVictim() {
      // By default, look for the un-protected block with the fewest sharers
      int32_t min_sharers = INT_MAX;
      int32_t best = -1;
      for (int32_t i = 0; i < theAssociativity; i++) {
        if (!theBlocks[i].isProtected() && theBlocks[i].state().countSharers() < min_sharers) {
          min_sharers = theBlocks[i].state().countSharers();
          best = i;
        }
      }
      return best;
    }
    bool loadState(boost::archive::binary_iarchive & ia, const std::string & aName) {
      StdDirEntrySerializer serializer;
      for (int32_t way = 0; way < theAssociativity; way++) {
        ia >> serializer;
        theBlocks[way] = serializer;
        DBG_(Trace, Addr(serializer.tag) ( << aName << " Directory loading block " << serializer ));
      }
      return true;
    }
  };

  int32_t theAssociativity;
  int32_t theBlockSize;
  int32_t theBanks;
  int32_t theTotalBanks;
  int32_t theLocalBankIndex;
  int32_t theGlobalBankIndex;
  int32_t theBankInterleaving;
  int32_t theGroups;
  int32_t theGroupIndex;
  int32_t theGroupInterleaving;
  int32_t theNumSharers;

  Tag tagMask;

  int32_t theNumSets;
  int32_t setHighShift;
  int32_t setMidShift;
  int32_t setLowShift;
  uint64_t setLowMask;
  uint64_t setMidMask;
  uint64_t setHighMask;

  bool theSkewSet;
  int32_t skewShift;

  int32_t theBankShift, theBankMask;
  int32_t theGroupShift, theGroupMask;

  Set ** theSets;

  DirEvictBuffer<_EState> theEvictBuffer;

  std::string theName;

  MemoryAddress makeTag(MemoryAddress anAddress) {
    return MemoryAddress(anAddress & tagMask);
  }

  int32_t makeSet(MemoryAddress addr) {
    if (theSkewSet) {
      uint64_t a = (uint64_t)addr ^ ((uint64_t)addr >> skewShift);
      return ((a >> setLowShift) & setLowMask) | ((a >> setMidShift) & setMidMask) | ((a >> setHighShift) & setHighMask);
    } else {
      return ((addr >> setLowShift) & setLowMask) | ((addr >> setMidShift) & setMidMask) | ((addr >> setHighShift) & setHighMask);
    }
  }

  inline int32_t getBank(MemoryAddress addr) {
    if (theSkewSet) {
      addr = MemoryAddress(addr ^ (addr >> skewShift));
    }
    return (addr >> theBankShift) & theBankMask;
  }

  inline int32_t getGroup(MemoryAddress addr) {
    if (theSkewSet) {
      addr = MemoryAddress(addr ^ (addr >> skewShift));
    }
    return (addr >> theGroupShift) & theGroupMask;
  }

public:
  typedef StdLookupResult LookupResult;
  typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

  virtual ~StdDirectory() {
    for (int32_t i = 0; i < theNumSets; i++) {
      delete theSets[i];
    }
    delete[] theSets;
  }

  StdDirectory(const CMPCacheInfo & theInfo, const std::list< std::pair< std::string, std::string> > &theConfiguration)
    : theEvictBuffer(theInfo.theDirEBSize)
    , theName(theInfo.theName) {
    theBlockSize = theInfo.theBlockSize;
    theBanks = theInfo.theNumBanks;
    theBankInterleaving = theInfo.theBankInterleaving;
    theGroups = theInfo.theNumGroups;
    theGroupInterleaving = theInfo.theGroupInterleaving;
    theGlobalBankIndex = theInfo.theNodeId;
    theLocalBankIndex = theInfo.theNodeId % theBanks;
    theGroupIndex = theInfo.theNodeId / theBanks;
    theNumSharers = theInfo.theCores;
    theTotalBanks = theBanks * theGroups;

    std::list< std::pair< std::string, std::string> >::const_iterator iter = theConfiguration.begin();
    for (; iter != theConfiguration.end(); iter++) {
      if (iter->first == "sets") {
        theNumSets = strtoll(iter->second.c_str(), nullptr, 0);
      } else if (iter->first == "total_sets" || iter->first == "global_sets") {
        int32_t global_sets = strtol(iter->second.c_str(), nullptr, 0);
        theNumSets = global_sets / theTotalBanks;
        DBG_Assert( (theNumSets * theTotalBanks) == global_sets, ( << "global_sets (" << global_sets
                    << ") is not divisible by number of banks (" << theTotalBanks << ")" ));
      } else if (iter->first == "assoc" || iter->first == "associativity") {
        theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if (iter->first == "skew" || iter->first == "skew_set") {
        theSkewSet = (strcasecmp(iter->second.c_str(), "true") == 0);
      } else {
        DBG_Assert( false, ( << "Unknown configuration parameter '" << iter->first << "' while creating StdArray" ));
      }
    }

    init();
  }

  void init() {
    DBG_Assert ( theNumSets > 0 );
    DBG_Assert ( theAssociativity > 0 );
    DBG_Assert ( theBlockSize     > 0 );

    //  Physical address layout:
    //
    //  +---------+------------+-------+-----+------+-----------+-----------------+
    //  | Tag     | Index High | Group | Mid | Bank | Index Low |  BlockOffset    |
    //  +---------+------------+-------+-----+------+-----------+-----------------+
    //                                                          |<- setLowShift ->|
    //                                       |<-------- setMidShift ------------->|
    //                         |<---------------- setHighShift ------------------>|

    DBG_Assert( ((theBlockSize - 1) & theBlockSize) == 0);
    DBG_Assert( ((theBankInterleaving - 1) & theBankInterleaving) == 0);
    DBG_Assert( ((theGroupInterleaving - 1) & theGroupInterleaving) == 0);

    DBG_Assert( (theBankInterleaving * theBanks) <= theGroupInterleaving, ( << "Invalid interleaving: BI = "
                << theBankInterleaving << ", Banks = " << theBanks << ", GI = " << theGroupInterleaving << ", Groups = " << theGroups ));

    int32_t blockOffsetBits = log_base2(theBlockSize);
    int32_t indexBits = log_base2(theNumSets);
    int32_t bankBits = log_base2(theBanks);
    int32_t bankInterleavingBits = log_base2(theBankInterleaving);
    int32_t groupBits = log_base2(theGroups);
    int32_t groupInterleavingBits = log_base2(theGroupInterleaving);

    int32_t lowBits = bankInterleavingBits - blockOffsetBits;
    int32_t midBits = groupInterleavingBits - bankInterleavingBits - bankBits;
    int32_t highBits = indexBits - midBits - lowBits;
    if ((midBits + lowBits) > indexBits) {
      midBits = indexBits - lowBits;
      highBits = 0;
    }

    setLowMask = (1 << lowBits) - 1;
    setMidMask = ((1 << midBits) - 1) << lowBits;
    setHighMask = ((1 << highBits) - 1) << (midBits + lowBits);

    setLowShift = blockOffsetBits;
    setMidShift = bankBits + blockOffsetBits;
    setHighShift = groupBits + bankBits + blockOffsetBits;

    skewShift = 34 - (indexBits + bankBits + blockOffsetBits);

    theBankMask = theBanks - 1;
    theBankShift = bankInterleavingBits;
    theGroupMask = theGroups - 1;
    theGroupShift = groupInterleavingBits;

    tagMask = ~0ULL & ~((uint64_t)(theBlockSize - 1));

    theSets = new Set*[theNumSets];
    DBG_Assert( theSets);

    for (int32_t i = 0; i < theNumSets; i++) {
      theSets[i] = new Set(theAssociativity, theNumSharers);
      DBG_Assert( theSets[i] );
    }

    DBG_(Dev, ( << "Created directory " << theGlobalBankIndex << ": Lshift " << setLowShift << ", Hshift " << setHighShift << ", Sshift " << skewShift << ", Lmask 0x" << std::hex << setLowMask << ", Hmask 0x" << setHighMask << ", NumSharers = " << theNumSharers ));
  }

  virtual bool allocate(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, MemoryAddress address, const _State & state) {
    StdLookupResult * std_lookup = dynamic_cast<StdLookupResult *>(lookup.get());
    DBG_Assert(std_lookup != nullptr, ( << "allocate() was not passed a valid StdLookupResult"));
    bool success, has_victim;
    _State v_state(0);
    MemoryAddress v_addr;
    boost::tie(success, has_victim, v_addr, v_state) =
      std_lookup->theSet->allocate(std_lookup, address, state);

    if (has_victim) {
      theEvictBuffer.insert(v_addr, v_state);
    }

    return success;
  }

  virtual boost::intrusive_ptr<AbstractLookupResult<_State> > lookup(MemoryAddress address) {
    DBG_(Trace, ( << "StdDirectory::lookup(0x" << std::hex << (uint64_t)address << ") in set 0x" << std::hex << makeSet(address) ));
    return theSets[makeSet(address)]->lookup(makeTag(address));
  }

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) {
    return (makeSet(a) == makeSet(b));
  }

  virtual DirEvictBuffer<_EState>* getEvictBuffer() {
    return &theEvictBuffer;
  }

  virtual bool loadState(std::istream & is) {
    boost::archive::binary_iarchive ia(is);

    // Loop through ALL of the sets and select the ones that belong to this set

    MemoryAddress addr(0);
    int32_t theTotalSets = theNumSets * theBanks * theGroups;
    int32_t local_set = 0;
    int32_t global_set = 0;

    uint64_t set_count = 0;
    uint32_t associativity = 0;

    ia >> set_count;
    ia >> associativity;

    DBG_Assert( set_count == (uint64_t)theTotalSets, ( << "Error loading directory state. Flexpoint contains " << set_count << " sets but simulator configured for " << theTotalSets << " sets." ));
    DBG_Assert( associativity == (uint64_t)theAssociativity, ( << "Error loading directory state. Flexpoint contains " << associativity << "-way sets but simulator configured for " << theAssociativity << "-way sets." ));

    StdDirEntrySerializer serializer;
    for (; global_set < theTotalSets; global_set++, addr += theBlockSize) {
      DBG_(Trace, ( << theName << ": Loading global set " << global_set << ", bank = " << getBank(addr) << ", local bank = " << theLocalBankIndex << ", group = " << getGroup(addr) << ", local group = " << theGroupIndex ));
      if ((getBank(addr) == theLocalBankIndex) && (getGroup(addr) == theGroupIndex)) {
        if (!theSets[local_set]->loadState(ia, theName)) {
          DBG_Assert(false, ( << "failed to load dir state for set " << local_set ));
          return false;
        }
        local_set++;
      } else {
        for (int32_t way = 0; way < theAssociativity; way++) {
          // Skip over entries that belong to other banks
          ia >> serializer;
        }
      }
    }

    return true;
  }

};

}; // namespace nCMPCache

#endif // __STD_DIRECTORY_HPP__

