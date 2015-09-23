#ifndef __TAGLESS_DIRECTORY_HPP__
#define __TAGLESS_DIRECTORY_HPP__

#include <boost/archive/binary_iarchive.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

#include <components/Common/GlobalHasher.hpp>
#include <components/Common/Util.hpp>
#include <components/Common/Serializers.hpp>
using nCommonUtil::log_base2;
using nCommonUtil::AddressHash;
using nCommonSerializers::StdDirEntrySerializer;

namespace nCMPDirectory {

template < typename _State, typename _EState = _State >
class TaglessDirectory : public AbstractDirectory<_State, _EState> {
public:

  class TaglessLookupResult : public AbstractLookupResult<_State> {
  private:
    MemoryAddress  theAddress;
    std::set<int>  theBucketList;
    std::vector<_State> &theSet;
    _State    theAggregateState;

    TaglessLookupResult(MemoryAddress addr, const std::set<int>& buckets, std::vector<_State> &set, int32_t num_sharers)
      : theAddress(addr), theBucketList(buckets), theSet(set), theAggregateState(num_sharers) {
      std::set<int>::iterator bucket = theBucketList.begin();
      theAggregateState = theSet[*bucket];
      bucket++;
      for (; bucket != theBucketList.end(); bucket++) {
        theAggregateState &= theSet[*bucket];
      }
    }

    friend class TaglessDirectory<_State, _EState>;
  public:
    virtual ~TaglessLookupResult() {}
    virtual bool found() {
      return true;
    }
    virtual void setProtected(bool val) { }
    virtual const _State & state() const {
      return theAggregateState;
    }

    virtual void addSharer( int32_t sharer) {
      theAggregateState.addSharer(sharer);
      std::set<int>::iterator bucket = theBucketList.begin();
      for (; bucket != theBucketList.end(); bucket++) {
        theSet[*bucket].addSharer(sharer);
      }

    }
    virtual void removeSharer( int32_t sharer) {
      DBG_Assert( false, ( << "Interface not supported by TaglessDirectory. Use remove(sharer,msg) instead." ));
    }
    void removeSharer( int32_t sharer, TaglessDirMsg_p msg) {
      if (msg->getConflictFreeBuckets().size() == 0) {
        return;
      }
      DBG_Assert(msg->getConflictFreeBuckets().size() == 1);
      const std::set<int> &conflict_free(msg->getConflictFreeBuckets().begin()->second);
      std::set<int>::const_iterator bucket = conflict_free.begin();
      for (; bucket != conflict_free.end(); bucket++) {
        theSet[*bucket].removeSharer(sharer);
      }
    }
    virtual void setSharer( int32_t sharer) {
      DBG_Assert( false, ( << "Interface not supported by TaglessDirectory. Use set(sharer,msg) instead." ));
    }
    void setSharer( int32_t sharer, TaglessDirMsg_p msg) {
      addSharer(sharer);
      const std::map<int, std::set<int> >& free_map = msg->getConflictFreeBuckets();
      std::map<int, std::set<int> >::const_iterator map_iter = free_map.begin();
      for (; map_iter != free_map.end(); map_iter++) {
        int32_t old_sharer = map_iter->first;
        std::set<int>::const_iterator bucket = map_iter->second.begin();
        for (; bucket != map_iter->second.end(); bucket++) {
          theSet[*bucket].removeSharer(old_sharer);
        }
      }
    }
    virtual void setState( const _State & state ) {
      DBG_Assert( false, ( << "Interface not supported by TaglessDirectory. No way to set state with a tagles directory." ));
    }

  };

private:
  std::vector<std::vector<_State> > theDirectory;

  int32_t theNumSharers;
  int32_t theBlockSize;
  int32_t theBanks;
  int32_t theBankIndex;
  int32_t theBankInterleaving;

  int32_t theBankShift;
  int32_t theBankMask;

  int32_t theNumLocalSets;
  int32_t theTotalNumSets;
  int32_t theNumBuckets;
  int32_t theTotalNumBuckets;
  bool thePartitioned;

  int32_t setLowShift, setHighShift, setLowMask, setHighMask;

  int32_t getBank(uint64_t addr) {
    return (addr >> theBankShift) & theBankMask;
  }

  int32_t get_set(uint64_t addr) {
    return ((addr >> setLowShift) & setLowMask) | ((addr >> setHighShift) & setHighMask);
  }

public:
  typedef TaglessLookupResult LookupResult;
  typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

  TaglessDirectory(const DirectoryInfo & theInfo, std::list<std::pair<std::string, std::string> > &args)
    : theNumLocalSets(-1), theTotalNumSets(-1), theNumBuckets(-1), thePartitioned(false) {
    theNumSharers = theInfo.theCores;
    theBlockSize = theInfo.theBlockSize;
    theBanks = theInfo.theNumBanks;
    theBankInterleaving = theInfo.theBankInterleaving;
    theBankIndex = theInfo.theNodeId;

    theBankShift = log_base2(theBankInterleaving);
    theBankMask = theBanks - 1;

    std::list<std::string> theHashFns;

    std::list<std::pair<std::string, std::string> >::iterator arg = args.begin();
    for (; arg != args.end(); arg++) {
      if (strcasecmp(arg->first.c_str(), "hash") == 0) {
        theHashFns.push_back(arg->second);
      } else if (strcasecmp(arg->first.c_str(), "total_sets") == 0) {
        theTotalNumSets = boost::lexical_cast<int>(arg->second);
        theNumLocalSets = -1;
      } else if (strcasecmp(arg->first.c_str(), "local_sets") == 0) {
        theNumLocalSets = boost::lexical_cast<int>(arg->second);
        theTotalNumSets = -1;
      } else if (strcasecmp(arg->first.c_str(), "buckets") == 0) {
        theNumBuckets = boost::lexical_cast<int>(arg->second);
      } else if (strcasecmp(arg->first.c_str(), "partitioned") == 0) {
        thePartitioned = boost::lexical_cast<bool>(arg->second);
      } else {
        DBG_Assert(false, ( << "Unexpected parameter '" << arg->first << "' passed to Tagless Directory." ));
      }
    }

    if (theNumLocalSets > 0) {
      theTotalNumSets = theNumLocalSets * theBanks;
    } else if (theTotalNumSets > 0) {
      theNumLocalSets = theTotalNumSets / theBanks;
      DBG_Assert( (theNumLocalSets * theBanks) == theTotalNumSets, ( << "Invalid number of sets. Total Sets = " << theTotalNumSets << ", Num Banks = " << theBanks ));
    } else {
      DBG_Assert( false, ( << "Failed to set number of sets." ));
    }

    int32_t theHashShift = log_base2(theTotalNumSets) + log_base2(theBlockSize);

    DBG_Assert( theNumBuckets > 0 );
    DBG_Assert( theHashFns.size() > 0 );

    nGlobalHasher::GlobalHasher::theHasher().initialize(theHashFns, theHashShift, theNumBuckets, thePartitioned);

    theTotalNumBuckets = theNumBuckets;
    if (thePartitioned) {
      theTotalNumBuckets *= theHashFns.size();
    }

    DBG_(Dev, ( << "Creating directory with " << theNumLocalSets << " sets and " << theTotalNumBuckets << " total buckets." ));

    theDirectory.resize(theNumLocalSets, std::vector<_State>(theTotalNumBuckets, _State(theNumSharers) ));

    DBG_Assert( ((theNumLocalSets - 1) & theNumLocalSets) == 0 );

    int32_t blockOffsetBits = log_base2(theBlockSize);
    int32_t bankBits = log_base2(theBanks);
    int32_t indexBits = log_base2(theNumLocalSets);
    int32_t interleavingBits = log_base2(theBankInterleaving);

    int32_t lowBits = interleavingBits - blockOffsetBits;
    int32_t highBits = indexBits - lowBits;

    setLowMask = (1 << lowBits) - 1;
    setHighMask = ((1 << highBits) - 1) << lowBits;

    setLowShift = blockOffsetBits;
    setHighShift = bankBits + blockOffsetBits;
  }

  virtual bool allocate(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, MemoryAddress address, const _State & state) {
    DBG_Assert(false, ( << "Tagless Directory does not support 'allocate()' function."  ));
    return false;
  }

  virtual boost::intrusive_ptr<AbstractLookupResult<_State> > lookup(MemoryAddress address) {
    return nativeLookup(address);
  }

  LookupResult_p nativeLookup(MemoryAddress address) {
    int32_t set_index = get_set(address);
    std::set<int> bucket_list(nGlobalHasher::GlobalHasher::theHasher().hashAddr(address));

    return LookupResult_p(new LookupResult(address, bucket_list, theDirectory[set_index], theNumSharers));
  }

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) {
    return (get_set(a) == get_set(b));
  }

  // We don't have an evict buffer, just return null
  // (the policy should be aware of this and provide it's own InfiniteEvictBuffer to make everything work nicely
  virtual EvictBuffer<_EState>* getEvictBuffer() {
    return nullptr;
  }

  virtual bool loadState(std::istream & is) {
    boost::archive::binary_iarchive ia(is);
    uint32_t count;
    ia >> count;
    StdDirEntrySerializer serializer;
    for (; count > 0; count--) {
      ia >> serializer;
      // only store entries for this bank to save memory
      if (getBank(serializer.tag) == theBankIndex) {
        int32_t set_index = get_set(serializer.tag);
        std::set<int> bucket_list(nGlobalHasher::GlobalHasher::theHasher().hashAddr(MemoryAddress(serializer.tag)));

        std::set<int>::iterator bucket = bucket_list.begin();
        for (; bucket != bucket_list.end(); bucket++) {
          theDirectory[set_index][*bucket] |= serializer.state;
        }
      }
    }
    return true;
  }
};

}; // namespace nCMPDirectory

#endif // __TAGLESS_DIRECTORY_HPP__

