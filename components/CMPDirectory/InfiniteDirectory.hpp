#ifndef __INFINITE_DIRECTORY_HPP__
#define __INFINITE_DIRECTORY_HPP__

#include <boost/archive/binary_iarchive.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

#include <components/Common/Util.hpp>
#include <components/Common/Serializers.hpp>
using nCommonUtil::log_base2;
using nCommonUtil::AddressHash;
using nCommonSerializers::StdDirEntrySerializer;

namespace nCMPDirectory {

template < typename _State, typename _EState = _State >
class InfiniteDirectory : public AbstractDirectory<_State, _EState> {
private:
  struct InfDirEntry {
    MemoryAddress theAddress;
    mutable _State theState;

    InfDirEntry(MemoryAddress anAddress, _State aState)
      : theAddress(anAddress), theState(aState)
    { }

    InfDirEntry(const StdDirEntrySerializer & serializer, int32_t aNumSharers)
      : theAddress(serializer.tag), theState(aNumSharers) {
      theState = serializer.state;
    }
  };

  typedef multi_index_container
  < InfDirEntry
  , indexed_by
  < hashed_unique
  < member<InfDirEntry, MemoryAddress, &InfDirEntry::theAddress>
  , AddressHash // From Common/Util.hpp
  >
  >
  > inf_dir_t;

  inf_dir_t theDirectory;

  typedef typename inf_dir_t::iterator iterator;

  class InfiniteLookupResult : public AbstractLookupResult<_State> {
  private:
    iterator theIterator;
    bool  isValid;

    InfiniteLookupResult(iterator iter, bool valid)
      : theIterator(iter), isValid(valid) { }

    friend class InfiniteDirectory<_State, _EState>;
  public:
    virtual bool found() {
      return isValid;
    }
    virtual void setProtected(bool val) { }
    virtual const _State & state() const {
      return theIterator->theState;
    }
    virtual void addSharer(int32_t sharer) {
      theIterator->theState.addSharer(sharer);
    }
    virtual void removeSharer(int32_t sharer) {
      theIterator->theState.removeSharer(sharer);
    }
    virtual void setSharer(int32_t sharer) {
      theIterator->theState.setSharer(sharer);
    }
    virtual void setState(const _State & state) {
      theIterator->theState = state;
    }
  };

  EvictBuffer<_EState> theEvictBuffer;

  int32_t theNumSharers;
  int32_t theBlockSize;
  int32_t theBanks;
  int32_t theBankIndex;
  int32_t theBankInterleaving;

  int32_t theBankShift;
  int32_t theBankMask;
  int32_t theSkewShift;

  std::string theName;

  int32_t getBank(uint64_t addr) {
    if (theSkewShift >= 0) {
      return ((addr >> theBankShift) ^ (addr >> theSkewShift)) & theBankMask;
    } else {
      return (addr >> theBankShift) & theBankMask;
    }
  }

public:
  typedef InfiniteLookupResult LookupResult;
  typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

  InfiniteDirectory(const DirectoryInfo & theInfo, std::list<std::pair<std::string, std::string> > &args)
    : theEvictBuffer(theInfo.theEBSize)
    , theName(theInfo.theName) {
    theNumSharers = theInfo.theCores;
    theBlockSize = theInfo.theBlockSize;
    theBanks = theInfo.theNumBanks;
    theBankInterleaving = theInfo.theBankInterleaving;
    theBankIndex = theInfo.theNodeId;

    theBankShift = log_base2(theBankInterleaving);
    theBankMask = theBanks - 1;

    theSkewShift = -1;

    std::list< std::pair< std::string, std::string> >::const_iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (strcasecmp(iter->first.c_str(), "skew_shift") == 0) {
        theSkewShift = boost::lexical_cast<int>(iter->second);
      } else {
        DBG_Assert( false, ( << "Unrecognized parameter '" << iter->first << "' passed to InfiniteDirectory." ));
      }
    }

  }

  virtual bool allocate(boost::intrusive_ptr<AbstractLookupResult<_State> > lookup, MemoryAddress address, const _State & state) {
    InfiniteLookupResult * inf_lookup = dynamic_cast<InfiniteLookupResult *>(lookup.get());
    DBG_Assert(inf_lookup != NULL, ( << "allocate() was not passed a valid InfiniteLookupResult" ));
    std::pair<iterator, bool> ret = theDirectory.insert(InfDirEntry(address, state));
    inf_lookup->theIterator = ret.first;
    return ret.second;
  }
  virtual boost::intrusive_ptr<AbstractLookupResult<_State> > lookup(MemoryAddress address) {
    iterator iter = theDirectory.find(address);
    return LookupResult_p(new LookupResult(iter, (iter != theDirectory.end()) ) );
  }

  virtual bool sameSet(MemoryAddress a, MemoryAddress b) {
    return false;
  }

  virtual EvictBuffer<_EState>* getEvictBuffer() {
    return &theEvictBuffer;
  }

  virtual bool loadState(std::istream & is) {
    boost::archive::binary_iarchive ia(is);
    uint32_t count;
    ia >> count;
    StdDirEntrySerializer serializer;
    DBG_(Dev, ( << theName << " - Directory loading " << count << " entries." ));
    for (; count > 0; count--) {
      ia >> serializer;
      // only store entries for this bank to save memory
      if (getBank(serializer.tag) == theBankIndex) {
        DBG_(Trace, ( << theName << " - Directory loading block " << serializer));
        theDirectory.insert( InfDirEntry(serializer, theNumSharers) );
      }
    }
    return true;
  }
};

}; // namespace nCMPDirectory

#endif // __INFINITE_DIRECTORY_HPP__

