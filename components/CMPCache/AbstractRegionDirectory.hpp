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
