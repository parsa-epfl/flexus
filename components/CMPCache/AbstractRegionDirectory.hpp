
#ifndef __ABSTRACT_REGION_DIRECTORY_HPP__
#define __ABSTRACT_REGION_DIRECTORY_HPP__

#include <components/CMPCache/AbstractDirectory.hpp>
#include <iostream>

namespace nCMPCache {

template<typename _State>
class AbstractRegionLookupResult : public AbstractLookupResult<_State>
{
  public:
    virtual ~AbstractRegionLookupResult() {}
    virtual int32_t owner() const = 0;

    virtual const _State& regionState(int32_t i) const     = 0;
    virtual const std::vector<_State>& regionState() const = 0;

    virtual MemoryAddress regionTag() const = 0;

    virtual void setRegionOwner(int32_t new_owner)                                = 0;
    virtual void setRegionState(const std::vector<_State>& new_state)             = 0;
    virtual void setRegionSharerState(int32_t sharer,
                                      boost::dynamic_bitset<uint64_t>& presence,
                                      boost::dynamic_bitset<uint64_t>& exclusive) = 0;

    virtual bool emptyRegion() = 0;
};

template<typename _State>
class AbstractRegionEvictBuffer : public DirEvictBuffer<_State>
{
  public:
    AbstractRegionEvictBuffer(int32_t size)
      : DirEvictBuffer<_State>(size)
    {
    }
    virtual ~AbstractRegionEvictBuffer() {}
};

template<typename _State, typename _EState = _State>
class AbstractRegionDirectory : public AbstractDirectory<_State, _EState>
{
  public:
    virtual ~AbstractRegionDirectory() {}

    typedef AbstractRegionLookupResult<_State> RegLookupResult;
    typedef typename boost::intrusive_ptr<RegLookupResult> RegLookupResult_p;

    virtual RegLookupResult_p nativeLookup(MemoryAddress address)      = 0;
    virtual AbstractRegionEvictBuffer<_EState>* getRegionEvictBuffer() = 0;

    virtual int32_t blocksPerRegion()                   = 0;
    virtual MemoryAddress getRegion(MemoryAddress addr) = 0;
};

}; // namespace nCMPCache

#endif // __ABSTRACT_REGION_DIRECTORY_HPP__
