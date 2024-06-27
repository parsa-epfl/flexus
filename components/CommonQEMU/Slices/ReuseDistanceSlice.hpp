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
#ifndef FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED
#define FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED

#ifdef FLEXUS_ReuseDistanceSlice_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ReuseDistanceSlice data type"
#endif
#define FLEXUS_ReuseDistanceSlice_TYPE_PROVIDED

#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/exception.hpp>
#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

using namespace Flexus::Core;
using boost::intrusive_ptr;

static MemoryMessage theDummyMemMsg(MemoryMessage::LoadReq);

struct ReuseDistanceSlice : public boost::counted_base
{ /*, public FastAlloc */
    typedef PhysicalMemoryAddress MemoryAddress;

    // enumerated message type
    enum ReuseDistanceSliceType
    {
        ProcessMemMsg,
        GetMeanReuseDist_Data
    };

    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType)
      : theType(aType)
      , theAddress(0)
      , theMemoryMessage(theDummyMemMsg)
      , theReuseDist(0LL)
    {
    }
    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType, MemoryAddress anAddress)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(theDummyMemMsg)
      , theReuseDist(0LL)
    {
    }
    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType, MemoryAddress anAddress, MemoryMessage& aMemMsg)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(aMemMsg)
      , theReuseDist(0LL)
    {
    }

    static intrusive_ptr<ReuseDistanceSlice> newProcessMemMsg(MemoryMessage& aMemMsg)
    {
        intrusive_ptr<ReuseDistanceSlice> slice = new ReuseDistanceSlice(ProcessMemMsg, aMemMsg.address(), aMemMsg);
        return slice;
    }
    static intrusive_ptr<ReuseDistanceSlice> newMeanReuseDistData(MemoryAddress anAddress)
    {
        intrusive_ptr<ReuseDistanceSlice> slice = new ReuseDistanceSlice(GetMeanReuseDist_Data, anAddress);
        return slice;
    }

    const ReuseDistanceSliceType type() const { return theType; }
    const MemoryAddress address() const { return theAddress; }
    const int64_t meanReuseDist() const { return theReuseDist; }

    ReuseDistanceSliceType& type() { return theType; }
    MemoryAddress& address() { return theAddress; }
    MemoryMessage& memMsg() { return theMemoryMessage; }
    int64_t& meanReuseDist() { return theReuseDist; }

  private:
    ReuseDistanceSliceType theType;
    MemoryAddress theAddress;
    MemoryMessage& theMemoryMessage;
    int64_t theReuseDist;
};

std::ostream&
operator<<(std::ostream& s, ReuseDistanceSlice const& aReuseDistSlice);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED
