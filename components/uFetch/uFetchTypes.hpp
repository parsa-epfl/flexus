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

#ifndef FLEXUS_uFETCH_TYPES_HPP_INCLUDED
#define FLEXUS_uFETCH_TYPES_HPP_INCLUDED

#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/qemu/mai_api.hpp>
#include <iostream>
#include <list>

namespace Flexus {
namespace SharedTypes {

using boost::counted_base;
using Flexus::SharedTypes::TransactionTracker;
using Flexus::SharedTypes::Translation;
using Flexus::SharedTypes::VirtualMemoryAddress;

enum eBranchType
{
    kNonBranch,
    kConditional,
    kUnconditional,
    kCall,
    kIndirectReg,
    kIndirectCall,
    kReturn,
    kLastBranchType
};
std::ostream&
operator<<(std::ostream& anOstream, eBranchType aType);

enum eDirection
{
    kStronglyTaken,
    kTaken // Bimodal
    ,
    kNotTaken // gShare
    ,
    kStronglyNotTaken
};
std::ostream&
operator<<(std::ostream& anOstream, eDirection aDir);

struct BPredState : boost::counted_base
{
    eBranchType thePredictedType;
    VirtualMemoryAddress thePredictedTarget;
    eDirection thePrediction;
    eDirection theBimodalPrediction;
    eDirection theMetaPrediction;
    eDirection theGSharePrediction;
    uint32_t theGShareShiftReg;
    uint32_t theSerial;
};

struct FetchAddr
{
    Flexus::SharedTypes::VirtualMemoryAddress theAddress;
    boost::intrusive_ptr<BPredState> theBPState;
    FetchAddr(Flexus::SharedTypes::VirtualMemoryAddress anAddress)
      : theAddress(anAddress)
    {
    }
};

struct FetchCommand : boost::counted_base
{
    std::list<FetchAddr> theFetches;
};

struct BranchFeedback : boost::counted_base
{
    VirtualMemoryAddress thePC;
    eBranchType theActualType;
    eDirection theActualDirection;
    VirtualMemoryAddress theActualTarget;
    boost::intrusive_ptr<BPredState> theBPState;
};

typedef uint32_t Opcode;

struct FetchedOpcode
{
    VirtualMemoryAddress thePC;
    Opcode theOpcode;
    boost::intrusive_ptr<BPredState> theBPState;
    boost::intrusive_ptr<TransactionTracker> theTransaction;

    FetchedOpcode(Opcode anOpcode)
      : theOpcode(anOpcode)
    {
    }
    FetchedOpcode(VirtualMemoryAddress anAddr,
                  Opcode anOpcode,
                  boost::intrusive_ptr<BPredState> aBPState,
                  boost::intrusive_ptr<TransactionTracker> aTransaction)
      : thePC(anAddr)
      , theOpcode(anOpcode)
      , theBPState(aBPState)
      , theTransaction(aTransaction)
    {
    }
};

struct FetchBundle : public boost::counted_base
{
    std::list<FetchedOpcode> theOpcodes;
    std::list<tFillLevel> theFillLevels;
    int32_t coreID;

    void updateOpcode(VirtualMemoryAddress anAddress, std::list<FetchedOpcode>::iterator it, Opcode anOpcode)
    {
        DBG_AssertSev(Crit,
                      it->thePC == anAddress,
                      (<< "ERROR: FetchedOpcode iterator did not match!! Iterator PC " << it->thePC
                       << ", translation returned vaddr " << anAddress));
        it->theOpcode = anOpcode;
    }

    void clear()
    {
        theOpcodes.clear();
        theFillLevels.clear();
    }
};

typedef boost::intrusive_ptr<FetchBundle> pFetchBundle;

} // end namespace SharedTypes
} // end namespace Flexus

#endif // FLEXUS_uFETCH_TYPES_HPP_INCLUDED
