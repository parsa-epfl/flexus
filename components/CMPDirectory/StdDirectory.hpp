#ifndef __STD_DIRECTORY_HPP__
#define __STD_DIRECTORY_HPP__

#include <strings.h>

#include <components/Common/Util.hpp>

#include <components/Common/Serializers.hpp>

using nCommonUtil::log_base2;
using nCommonSerializers::StdDirEntrySerializer;

namespace nCMPDirectory {

typedef uint64_t Tag;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

template < typename _State, typename _EState = _State >
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
      return LookupResult_p(new LookupResult(this, NULL, anAddress, false));
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
    bool loadState(boost::archive::binary_iarchive & ia) {
      StdDirEntrySerializer serializer;
      for (int32_t way = 0; way < theAssociativity; way++) {
        ia >> serializer;
        theBlocks[way] = serializer;
        DBG_(Crit, ( << "Directory loading block " << serializer ));
      }
      return true;
    }
  };

  int32_t theAssociativity;
  int32_t theBlockSize;
  int32_t theBanks;
  int32_t theBankIndex;
  int32_t theBankInterleaving;
  int32_t theNumSharers;

  Tag tagMask;

  int32_t theNumSets;
  int32_t setHighShift;
  int32_t setLowShift;
  uint64_t setLowMask;
  uint64_t setHighMask;

  bool theSkewSet;
  int32_t skewShift;

  Set ** theSets;

  EvictBuffer<_EState> theEvictBuffer;

  std::string theName;

  MemoryAddress makeTag(MemoryAddress anAddress) {
    return MemoryAddress(anAddress & tagMask);
  }

  int32_t makeSet(MemoryAddress addr) {
    if (theSkewSet) {
      uint64_t a = (uint64_t)addr ^ ((uint64_t)addr >> skewShift);
      return ((a >> setLowShift) & setLowMask) | ((a >> setHighShift) & setHighMask);
    } else {
      return ((addr >> setLowShift) & setLowMask) | ((addr >> setHighShift) & setHighMask);
    }
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

  StdDirectory(const DirectoryInfo & theInfo, const std::list< std::pair< std::string, std::string> > &theConfiguration)
    : theEvictBuffer(theInfo.theEBSize)
    , theName(theInfo.theName) {
    theBlockSize = theInfo.theBlockSize;
    theBanks = theInfo.theNumBanks;
    theBankInterleaving = theInfo.theBankInterleaving;
    theBankIndex = theInfo.theNodeId;
    theNumSharers = theInfo.theCores;

    std::list< std::pair< std::string, std::string> >::const_iterator iter = theConfiguration.begin();
    for (; iter != theConfiguration.end(); iter++) {
      if (iter->first == "sets") {
        theNumSets = strtoll(iter->second.c_str(), NULL, 0);
      } else if (iter->first == "total_sets" || iter->first == "global_sets") {
        int32_t global_sets = strtol(iter->second.c_str(), NULL, 0);
        theNumSets = global_sets / theBanks;
        DBG_Assert( (theNumSets * theBanks) == global_sets, ( << "global_sets (" << global_sets << ") is not divisible by number of banks (" << theBanks << ")" ));
      } else if (iter->first == "assoc" || iter->first == "associativity") {
        theAssociativity = strtol(iter->second.c_str(), NULL, 0);
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
    //  +---------+------------+------+-----------+-----------------+
    //  | Tag     | Index High | Bank | Index Low |  BlockOffset    |
    //  +---------+------------+------+-----------+-----------------+
    //                                            |<- setLowShift ->|

    DBG_Assert( ((theBlockSize - 1) & theBlockSize) == 0);
    DBG_Assert( ((theBankInterleaving - 1) & theBankInterleaving) == 0);

    int32_t blockOffsetBits = log_base2(theBlockSize);
    int32_t bankBits = log_base2(theBanks);
    int32_t indexBits = log_base2(theNumSets);
    int32_t interleavingBits = log_base2(theBankInterleaving);

    int32_t lowBits = interleavingBits - blockOffsetBits;
    int32_t highBits = indexBits - lowBits;

    setLowMask = (1 << lowBits) - 1;
    setHighMask = ((1 << highBits) - 1) << lowBits;

    setLowShift = blockOffsetBits;
    setHighShift = bankBits + blockOffsetBits;

    skewShift = 34 - (indexBits + bankBits + blockOffsetBits);

    tagMask = ~0ULL & ~((uint64_t)(theBlockSize - 1));

    theSets = new Set*[theNumSets];
    DBG_Assert( theSets);

    for (int32_t i = 0; i < theNumSets; i++) {
      theSets[i] = new Set(theAssociativity, theNumSharers);
      DBG_Assert( theSets[i] );
    }

    DBG_(Dev, ( << "Created directory " << theBankIndex << ": Lshift " << setLowShift << ", Hshift " << setHighShift << ", Sshift " << skewShift << ", Lmask 0x" << std::hex << setLowMask << ", Hmask 0x" << setHighMask << ", NumSharers = " << theNumSharers ));
  }

  virtual bool allocate(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, MemoryAddress address, const _State & state) {
    StdLookupResult * std_lookup = dynamic_cast<StdLookupResult *>(lookup.get());
    DBG_Assert(std_lookup != NULL, ( << "allocate() was not passed a valid StdLookupResult"));
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

  virtual EvictBuffer<_EState>* getEvictBuffer() {
    return &theEvictBuffer;
  }

  virtual bool loadState(std::istream & is) {
    boost::archive::binary_iarchive ia(is);
    int32_t setsPerGroup = theBankInterleaving / theBlockSize;
    int32_t numGroups = theNumSets / setsPerGroup;
    DBG_(Dev, ( << theName << " - Loading " << numGroups << " groups of " << setsPerGroup << " sets"));
    int32_t set = 0;
    StdDirEntrySerializer serializer;
    int32_t global_set = 0;
    for (int32_t group = 0; group < numGroups; group++) {
      for (int32_t bank = 0; bank < theBanks; bank++) {
        if (bank == theBankIndex) {
          for (int32_t local_set = 0; local_set < setsPerGroup; local_set++, set++, global_set++) {
            DBG_Assert(set < theNumSets);
            if (!theSets[set]->loadState(ia)) {
              DBG_Assert(false, ( << "failed to load dir state for set " << set ));
              return false;
            }
            for (int32_t way = 0; way < theAssociativity; way++) {
              DBG_(Trace, ( << theName << " - Bank[" << bank << "] - Set[" << set << "][" << way << "] = 0x" << std::hex << theSets[set]->theBlocks[way].tag() << " : " << theSets[set]->theBlocks[way].state().getSharers() ));
              int32_t calc_set = makeSet(MemoryAddress(theSets[set]->theBlocks[way].tag()));
              if ((calc_set != set) && (theSets[set]->theBlocks[way].tag() != 0)) {
                DBG_Assert(false, ( << theName << " - block " << std::hex << theSets[set]->theBlocks[way].tag() << std::dec << " maps to set " << calc_set << " but found in set " << set ));
              }
            }
          }
        } else {
          for (int32_t local_set = 0; local_set < setsPerGroup; local_set++, global_set++) {
            for (int32_t way = 0; way < theAssociativity; way++) {
              // Skip over entries that belong to other banks
              ia >> serializer;
            }
          }
        }
      }
    }
    return true;
  }

};

}; // namespace nCMPDirectory

#endif // __STD_DIRECTORY_HPP__

