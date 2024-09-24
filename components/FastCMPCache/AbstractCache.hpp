#ifndef FLEXUS_FASTCMPCACHE_ABSTRACT_CACHE_HPP_INCLUDED
#define FLEXUS_FASTCMPCACHE_ABSTRACT_CACHE_HPP_INCLUDED

#include "core/debug/debug.hpp"
#include "core/types.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <components/FastCMPCache/CoherenceStates.hpp>
#include <components/FastCMPCache/LookupResult.hpp>
#include <functional>

using namespace boost::multi_index;

using Flexus::SharedTypes::PhysicalMemoryAddress;

namespace nFastCMPCache {

typedef std::function<void(uint64_t tagset, CoherenceState_t state)> evict_function_t;
typedef std::function<void(uint64_t tagset, int32_t owner)> region_evict_function_t;
typedef std::function<bool(uint64_t tagset, bool icache, bool dcache)> invalidate_function_t;

class AbstractCache
{
  public:
    virtual ~AbstractCache() {}

    virtual LookupResult_p lookup(uint64_t tagset) = 0;

    virtual uint32_t regionProbe(uint64_t tagset)
    {
        DBG_Assert(false, (<< "regionProbe() function not supported by this Cache structure."));
        return 0;
    }

    virtual uint32_t blockScoutProbe(uint64_t tagset)
    {
        DBG_Assert(false,
                   (<< "blockScoutProbe() function not supported by this "
                       "Cache structure."));
        return 0;
    }

    virtual uint32_t blockProbe(uint64_t tagset)
    {
        DBG_Assert(false, (<< "blockProbe() function not supported by this Cache structure."));
        return 0;
    }

    virtual void setNonSharedRegion(uint64_t tagset)
    {
        DBG_Assert(false,
                   (<< "setNonSharedRegion() function not supported by this "
                       "Cache structure."));
    }

    virtual void setPartialSharedRegion(uint64_t tagset, uint32_t shared)
    {
        DBG_Assert(false,
                   (<< "setPartialSharedRegion() function not supported by "
                       "this Cache structure."));
    }

    virtual bool isNonSharedRegion(uint64_t tagset)
    {
        DBG_Assert(false,
                   (<< "isNonSharedRegion() function not supported by this "
                       "Cache structure."));
        return false;
    }

    virtual int32_t getOwner(LookupResult_p result, uint64_t tagset)
    {
        DBG_Assert(false, (<< "getOwner() function not supported by this Cache structure."));
        return -1;
    }

    virtual void updateOwner(LookupResult_p result, int32_t owner, uint64_t tagset, bool shared = true)
    {
        DBG_Assert(false, (<< "updateOwner() function not supported by this Cache structure."));
    }

    virtual void finalize() { DBG_Assert(false, (<< "finalize() function not supported by this Cache structure.")); }

    virtual void getSetTags(uint64_t region, std::list<PhysicalMemoryAddress>& tags) = 0;

    virtual void saveState(std::ostream& s) = 0;

    virtual bool loadState(std::istream& s) = 0;
};

} // namespace nFastCMPCache

#endif /* FLEXUS_FASTCMPCACHE_ABSTRACT_CACHE_HPP_INCLUDED */