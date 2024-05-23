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
#ifndef FLEXUS_FASTCACHE_ABSTRACT_CACHE_HPP_INCLUDED
#define FLEXUS_FASTCACHE_ABSTRACT_CACHE_HPP_INCLUDED

#include <boost/dynamic_bitset.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <components/FastCache/LookupResult.hpp>
#include <functional>

using namespace boost::multi_index;

#include <core/types.hpp>

using Flexus::SharedTypes::PhysicalMemoryAddress;

namespace nFastCache {

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

    virtual void getSetTags(uint64_t region, std::list<PhysicalMemoryAddress>& tags) = 0;

    virtual void saveState(std::ostream& s) = 0;

    virtual bool loadState(std::istream& s) = 0;
};

} // namespace nFastCache

#endif /* FLEXUS_FASTCACHE_ABSTRACT_CACHE_HPP_INCLUDED */
