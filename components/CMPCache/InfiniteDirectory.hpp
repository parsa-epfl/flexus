
#ifndef __INFINITE_DIRECTORY_HPP__
#define __INFINITE_DIRECTORY_HPP__

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <core/checkpoint/json.hpp>
#include <fstream>
using namespace boost::multi_index;

#include <components/CommonQEMU/Util.hpp>

using json = nlohmann::json;
using nCommonUtil::AddressHash;
using nCommonUtil::log_base2;

namespace nCMPCache {

template<typename _State, typename _EState = _State>
class InfiniteDirectory : public AbstractDirectory<_State, _EState>
{
  private:
    struct InfDirEntry
    {
        MemoryAddress theAddress;
        mutable _State theState;
        mutable bool theProtectedState;

        InfDirEntry(MemoryAddress anAddress, _State aState)
          : theAddress(anAddress)
          , theState(aState)
          , theProtectedState(false)
        {
        }

        InfDirEntry(MemoryAddress tag, uint32_t sharer_nb, _State sharers)
          : theAddress(tag)
          , theState(sharer_nb)
          , theProtectedState(false)
        {
            theState = sharers;
        }
    };

    typedef multi_index_container<InfDirEntry,
                                  indexed_by<hashed_unique<member<InfDirEntry, MemoryAddress, &InfDirEntry::theAddress>,
                                                           AddressHash // From Common/Util.hpp
                                                           >>>
      inf_dir_t;

    inf_dir_t theDirectory;

    typedef typename inf_dir_t::iterator iterator;

    class InfiniteLookupResult : public AbstractLookupResult<_State>
    {
      private:
        iterator theIterator;
        bool isValid;

        InfiniteLookupResult(iterator iter, bool valid)
          : theIterator(iter)
          , isValid(valid)
        {
        }

        friend class InfiniteDirectory<_State, _EState>;

      public:
        virtual bool found() { return isValid; }
        virtual bool isProtected() { return theIterator->theProtectedState; }
        virtual void setProtected(bool val) { theIterator->theProtectedState = val; }
        virtual const _State& state() const { return theIterator->theState; }
        virtual void addSharer(int32_t sharer) { theIterator->theState.addSharer(sharer); }
        virtual void removeSharer(int32_t sharer) { theIterator->theState.removeSharer(sharer); }
        virtual void setSharer(int32_t sharer) { theIterator->theState.setSharer(sharer); }
        virtual void setState(const _State& state) { theIterator->theState = state; }
    };

    class DummyLookupResult : public AbstractLookupResult<_State>
    {
      private:
        _State theState;

        DummyLookupResult(_State& s)
          : theState(s)
        {
        }

        friend class InfiniteDirectory<_State, _EState>;

      public:
        virtual bool found() { return true; }
        virtual bool isProtected() { return false; }
        virtual void setProtected(bool val) {}
        virtual const _State& state() const { return theState; }
        virtual void addSharer(int32_t sharer) { theState.addSharer(sharer); }
        virtual void removeSharer(int32_t sharer) { theState.removeSharer(sharer); }
        virtual void setSharer(int32_t sharer) { theState.setSharer(sharer); }
        virtual void setState(const _State& state) { theState = state; }
    };

    DirEvictBuffer<_EState> theEvictBuffer;

    int32_t theNumSharers;
    uint32_t theBlockSize;
    uint32_t theBlockShift;

    uint64_t theNodeId;
    uint64_t theNumNodes;

    bool theSameSetReturnValue;

    std::string theName;

  public:
    typedef InfiniteLookupResult LookupResult;
    typedef typename boost::intrusive_ptr<LookupResult> LookupResult_p;

    InfiniteDirectory(const CMPCacheInfo& theInfo, std::list<std::pair<std::string, std::string>>& args)
      : theEvictBuffer(theInfo.theDirEBSize)
      , theSameSetReturnValue(false)
      , theName(theInfo.theName)
    {
        theNumSharers        = theInfo.theCores;
        theBlockSize         = theInfo.theBlockSize;

        // The current interleaving scheme requires a block size of 64 bytes
        DBG_Assert(theBlockSize == 64);

        theBlockShift = log_base2(theBlockSize);
        theNodeId  = theInfo.theNodeId;
        theNumNodes = Flexus::Core::ComponentManager::getComponentManager().systemWidth();;
        
    }

    virtual bool allocate(boost::intrusive_ptr<AbstractLookupResult<_State>> lookup,
                          MemoryAddress address,
                          const _State& state)
    {
        InfiniteLookupResult* inf_lookup = dynamic_cast<InfiniteLookupResult*>(lookup.get());
        DBG_Assert(inf_lookup != nullptr, (<< "allocate() was not passed a valid InfiniteLookupResult"));
        std::pair<iterator, bool> ret = theDirectory.insert(InfDirEntry(address, state));
        inf_lookup->theIterator       = ret.first;
        return ret.second;
    }
    virtual boost::intrusive_ptr<AbstractLookupResult<_State>> lookup(MemoryAddress address)
    {
        // Make sure this address is in the right range.
        uint64_t node_index = (address >> theBlockShift) % theNumNodes;

        DBG_Assert(node_index == theNodeId, (<< "Address " << std::hex << address << " is not in the correct node. Expected node " << theNodeId << " but got node " << node_index));

        iterator iter = theDirectory.find(address);
        return LookupResult_p(new LookupResult(iter, (iter != theDirectory.end())));
    }

    virtual void remove(MemoryAddress address) { theDirectory.erase(address); }

    virtual bool sameSet(MemoryAddress a, MemoryAddress b) { return theSameSetReturnValue; }

    virtual void setSameSetReturn(bool ret) { theSameSetReturnValue = ret; }

    virtual DirEvictBuffer<_EState>* getEvictBuffer() { return &theEvictBuffer; }

    virtual boost::intrusive_ptr<AbstractLookupResult<_State>> getDummyResult(_State& state)
    {
        return boost::intrusive_ptr<AbstractLookupResult<_State>>(new DummyLookupResult(state));
    }

    virtual void load_dir_from_ckpt(const std::string& filename)
    {
        std::ifstream ifs(filename.c_str(), std::ios::in);

        if (!ifs.good()) {
            DBG_(Dev, (<< "checkpoint file: " << filename << " not found."));
            DBG_Assert(false, (<< "FILE NOT FOUND"));
        }

        json checkpoint;
        ifs >> checkpoint;

        // empty the directory
        theDirectory.clear();

        // for length of the dir
        uint32_t cache_size = checkpoint.size();
        DBG_(Trace, (<< "Directory loading " << cache_size << " entries."));

        for (uint32_t i{ 0 }; i < cache_size; i++) {
            uint64_t address = checkpoint.at(i)["tag"];

            uint64_t node_idx_of_cacheline = (address >> theBlockShift) % theNumNodes;

            DBG_Assert(node_idx_of_cacheline == theNodeId, (<< "Address " << std::hex << address << " is not in the correct node. Expected node " << theNodeId << " but got node " << node_idx_of_cacheline));

            std::bitset<MAX_NUM_SHARERS> sharers(checkpoint.at(i)["sharers"].get<std::string>());

            DBG_Assert(sharers.size() <= MAX_NUM_SHARERS, (<< "Sharers size mismatch"));

            // It's stupid but that's the only way to workaround this object
            SimpleDirectoryState state(theNumSharers);
            state = sharers;
            theDirectory.insert(InfDirEntry(PhysicalMemoryAddress(address), state));
        }

        DBG_(Trace, (<< "Directory loaded"));
        ifs.close();
    }
};

}; // namespace nCMPCache

#endif // __INFINITE_DIRECTORY_HPP__
