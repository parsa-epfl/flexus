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

#ifndef _ABSTRACT_ARRAY_HPP
#define _ABSTRACT_ARRAY_HPP

#include <boost/throw_exception.hpp>
#include <components/CMPCache/CMPCacheInfo.hpp>
#include <components/CMPCache/CacheState.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <exception>
#include <iostream>
#include <limits.h>
#include <stdlib.h>

namespace nCMPCache {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

typedef uint32_t BlockOffset;

class InvalidCacheAccessException : public std::exception
{};

// The output of a cache lookup
// derived from counted_base so we work with intrusive_ptr
template<typename _State>
class AbstractArrayLookupResult : public boost::counted_base
{
  public:
    virtual ~AbstractArrayLookupResult() {}
    AbstractArrayLookupResult() {}

    virtual const _State& state(void) const        = 0;
    virtual bool setState(const _State& aNewState) = 0;
    virtual void setProtected(bool val)            = 0;
    virtual bool isProtected() const               = 0;
    virtual void setPrefetched(bool val)           = 0;
    virtual void setLocked(bool val)               = 0;

    virtual bool hit(void) const   = 0;
    virtual bool miss(void) const  = 0;
    virtual bool found(void) const = 0;
    virtual bool valid(void) const = 0;

    virtual MemoryAddress blockAddress(void) const = 0;

}; // class AbstractArrayLookupResult

template<typename _State>
class AbstractArray
{
  public:
    virtual ~AbstractArray() {}
    AbstractArray() {}

    typedef boost::intrusive_ptr<AbstractArrayLookupResult<_State>> LookupResult_p;

    // Main array lookup function
    virtual LookupResult_p operator[](const MemoryAddress& anAddress) = 0;

    virtual LookupResult_p allocate(LookupResult_p lookup, const MemoryAddress& anAddress) = 0;

    virtual bool recordAccess(LookupResult_p lookup)    = 0;
    virtual void invalidateBlock(LookupResult_p lookup) = 0;

    // Checkpoint reading/writing functions
    virtual bool saveState(std::ostream& s)                  = 0;
    virtual bool loadState(std::istream& s, int32_t anIndex) = 0;

    // Addressing helper functions
    MemoryAddress blockAddress(MemoryAddress const& anAddress) const
    {
        return MemoryAddress(anAddress & ~(blockOffsetMask));
    }

    BlockOffset blockOffset(MemoryAddress const& anAddress) const { return BlockOffset(anAddress & blockOffsetMask); }

    virtual uint32_t freeEvictionResources() const { return UINT_MAX; }
    virtual bool evictionResourcePressure() const { return false; }
    virtual bool evictionResourcesAvailable() const { return true; }
    virtual bool evictionResourcesEmpty() const { return true; }
    virtual void reserveEvictionResource(int32_t n) {}
    virtual void unreserveEvictionResource(int32_t n) {}
    virtual std::pair<_State, MemoryAddress> getPreemptiveEviction() = 0;

    virtual bool sameSet(MemoryAddress a, MemoryAddress b) { return false; }
    virtual std::list<MemoryAddress> getSetTags(MemoryAddress addr)
    {
        DBG_Assert(false, (<< "Derived class does not implement getSetTags() function."));
        return std::list<MemoryAddress>();
    }

    virtual bool setAlmostFull(LookupResult_p lookup, MemoryAddress const& anAddress) const
    {
        return true; // Default always returns true. Proper behaviour can be
                     // definied in derived classes when required
    }

    virtual bool lockedVictimAvailable(LookupResult_p lookup, MemoryAddress const& anAddress) const
    {
        return false; // Default always returns false. Proper behaviour can be
                      // definied in derived classes when required
    }

    virtual bool victimAvailable(LookupResult_p lookup, MemoryAddress const& anAddress) const
    {
        return false; // Default always returns false. Proper behaviour can be
                      // definied in derived classes when required
    }

    virtual LookupResult_p replaceLockedBlock(LookupResult_p lookup, MemoryAddress const& anAddress)
    {
        DBG_Assert(false, (<< "Derived class does not implement replaceLockedBlock() function."));
        return nullptr;
    }

    virtual void setLockedThreshold(int32_t threshold)
    {
        DBG_Assert(theLockedThreshold > 0);
        theLockedThreshold = threshold;
    }

    virtual uint32_t globalCacheSets()
    {
        DBG_Assert(false, (<< "Derived class does not implement globalCacheSets() function."));
        return 0;
    }

  protected:
    MemoryAddress blockOffsetMask;
    int32_t theLockedThreshold;

}; // class AbstractArray

}; // namespace nCMPCache

#include <components/CMPCache/RTArray.hpp>
#include <components/CMPCache/StdArray.hpp>

namespace nCMPCache {

/*
 * Construct an array based on the configuration given
 *
 * The configuraion has the following format:
 *
 *      <Type>[:[<key>=<value>[:<key>=<value>]]
 *
 * <Type> is the name of the array type to be created
 * and it is followed by an optional series of key,value pairs
 * that describe a configuration appropriate for that type
 *
 */
template<typename _State, const _State& _Default>
AbstractArray<_State>*
constructArray(std::string& anArrayConfiguration, CMPCacheInfo& theInfo, int32_t theBlockSize)
{
    std::string& args = anArrayConfiguration;

    // First, extract the Name of the type of array to consruct
    std::string::size_type loc = args.find(':', 0);
    std::string name           = args.substr(0, loc);
    if (loc != std::string::npos) {
        args = args.substr(loc + 1);
    } else {
        args = "";
    }

    // Now create a list of key,value pairs
    std::list<std::pair<std::string, std::string>> arg_list;
    std::string key;
    std::string value;
    std::string::size_type pos = 0;
    do {
        pos                              = args.find(':', 0);
        std::string cur_arg              = args.substr(0, pos);
        std::string::size_type equal_pos = cur_arg.find('=', 0);
        if (equal_pos == std::string::npos) {
            key   = cur_arg;
            value = "1";
        } else {
            key   = cur_arg.substr(0, equal_pos);
            value = cur_arg.substr(equal_pos + 1);
        }
        arg_list.push_back(std::make_pair(key, value));

        if (pos != std::string::npos) { args = args.substr(pos + 1); }
    } while (pos != std::string::npos);

    // Now construct an array of the appropriate type
    // BlockSize is always passed separately to avoid specifying it more than once
    if (name == "std" || name == "Std" || name == "STD") {
        return new StdArray<_State, _Default>(theInfo, theBlockSize, arg_list);
    } else if (name == "RegionTracker" || name == "RT" || name == "rt") {
        return new RTArray<_State, _Default>(theInfo, theBlockSize, arg_list);
    }

    DBG_Assert(false, (<< "Failed to create Instance of '" << name << "'"));
    return nullptr;
}

}; // namespace nCMPCache

#endif /* _ABSTRACT_ARRAY_HPP */
