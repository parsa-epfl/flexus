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
#ifndef FLEXUS_SLICES__PERFECTPLACEMENTSLICE_HPP_INCLUDED
#define FLEXUS_SLICES__PERFECTPLACEMENTSLICE_HPP_INCLUDED

#ifdef FLEXUS_PerfectPlacementSlice_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::PerfectPlacementSlice data type"
#endif
#define FLEXUS_PerfectPlacementSlice_TYPE_PROVIDED

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

static MemoryMessage thePerfPlcDummyMemMsg(MemoryMessage::LoadReq);

struct PerfectPlacementSlice : public boost::counted_base
{ /*, public FastAlloc */
    typedef PhysicalMemoryAddress MemoryAddress;

    // enumerated message type
    enum PerfectPlacementSliceType
    {
        ProcessMsg,
        MakeBlockWritable
    };

    explicit PerfectPlacementSlice(PerfectPlacementSliceType aType)
      : theType(aType)
      , theAddress(0)
      , theMemoryMessage(thePerfPlcDummyMemMsg)
    {
    }
    explicit PerfectPlacementSlice(PerfectPlacementSliceType aType, MemoryAddress anAddress)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(thePerfPlcDummyMemMsg)
    {
    }
    explicit PerfectPlacementSlice(PerfectPlacementSliceType aType, MemoryAddress anAddress, MemoryMessage& aMemMsg)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(aMemMsg)
    {
    }

    static intrusive_ptr<PerfectPlacementSlice> newMakeBlockWritable(MemoryAddress anAddress)
    {
        intrusive_ptr<PerfectPlacementSlice> slice = new PerfectPlacementSlice(MakeBlockWritable, anAddress);
        return slice;
    }
    static intrusive_ptr<PerfectPlacementSlice> newProcessMsg(MemoryMessage& aMemMsg)
    {
        intrusive_ptr<PerfectPlacementSlice> slice = new PerfectPlacementSlice(ProcessMsg, aMemMsg.address(), aMemMsg);
        return slice;
    }

    const PerfectPlacementSliceType type() const { return theType; }
    const MemoryAddress address() const { return theAddress; }

    PerfectPlacementSliceType& type() { return theType; }
    MemoryAddress& address() { return theAddress; }
    MemoryMessage& memMsg() { return theMemoryMessage; }

  private:
    PerfectPlacementSliceType theType;
    MemoryAddress theAddress;
    MemoryMessage& theMemoryMessage;
};

std::ostream&
operator<<(std::ostream& s, PerfectPlacementSlice const& aPerfPlcSlice);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__PERFECTPLACEMENTSLICE_HPP_INCLUDED
