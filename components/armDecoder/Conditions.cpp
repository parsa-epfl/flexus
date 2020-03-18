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

#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArchARM/RegisterType.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>

#include "components/uArchARM/CoreModel/PSTATE.hpp"

#include <core/MakeUniqueWrapper.hpp>

#include "Conditions.hpp"
#include "OperandMap.hpp"
#include "SemanticActions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {
using namespace nuArchARM;

bool ConditionHolds(const PSTATE &pstate, int condcode) {

  bool result = false;
  switch (condcode >> 1) {
  case 0: // EQ or NE
    result = pstate.Z();
    break;
  case 1: // CS or CC
    result = pstate.C();
    break;
  case 2: // MI or PL
    result = pstate.N();
    break;
  case 3: // VS or VC
    result = pstate.V();
    break;
  case 4: // HI or LS
    result = (pstate.C() && pstate.Z() == 0);
    break;
  case 5: // GE or LT
    result = (pstate.N() == pstate.V());
    break;
  case 6: // GT or LE
    result = ((pstate.N() == pstate.V()) && (pstate.Z() == 0));
    break;
  case 7: // AL
    result = true;
    break;
  default:
    DBG_Assert(false);
    break;
  }

  // Condition flag values in the set '111x' indicate always true
  // Otherwise, invert condition if necessary.

  if (((condcode & 1) == 1) && (condcode != 0xf /*1111*/))
    return !result;

  return result;
}

typedef struct CMPBR : public Condition {
  CMPBR() {
  }
  virtual ~CMPBR() {
  }
  virtual bool operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    return boost::get<uint64_t>(operands[0]) == 0;
  }
  virtual char const *describe() const {
    return "Compare and Branch on Zero";
  }
} CMPBR;

typedef struct CBNZ : public Condition {
  CBNZ() {
  }
  virtual ~CBNZ() {
  }
  virtual bool operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    return boost::get<uint64_t>(operands[0]) != 0;
  }
  virtual char const *describe() const {
    return "Compare and Branch on Non Zero";
  }
} CBNZ;

typedef struct TBZ : public Condition {
  TBZ() {
  }
  virtual ~TBZ() {
  }
  virtual bool operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return (boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1])) == 0;
  }
  virtual char const *describe() const {
    return "Test and Branch on Zero";
  }
} TBZ;

typedef struct TBNZ : public Condition {
  TBNZ() {
  }
  virtual ~TBNZ() {
  }
  virtual bool operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return (boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1])) != 0;
  }
  virtual char const *describe() const {
    return "Test and Branch on Non Zero";
  }
} TBNZ;

typedef struct BCOND : public Condition {
  BCOND() {
  }
  virtual ~BCOND() {
  }
  virtual bool operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    PSTATE p = boost::get<uint64_t>(operands[0]);
    uint64_t test = boost::get<uint64_t>(operands[1]);

    // hacking  --- update pstate
    //    uint32_t pstate =
    //    Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readPSTATE();

    //    DBG_(Iface, (<< "Flexus pstate = " <<
    //    theInstruction->core()->getPSTATE())); DBG_(Iface, (<< "qemu pstate = "
    //    << pstate));

    return ConditionHolds(p, test);
  }
  virtual char const *describe() const {
    return "Branch Conditionally";
  }
} BCOND;

std::unique_ptr<Condition> condition(eCondCode aCond) {

  std::unique_ptr<Condition> ptr;
  switch (aCond) {
  case kCBZ_:
    DBG_(Iface, (<< "CBZ"));
    ptr.reset(new CMPBR());
    break;
  case kCBNZ_:
    DBG_(Iface, (<< "CBNZ"));
    ptr.reset(new CBNZ());
    break;
  case kTBZ_:
    DBG_(Iface, (<< "kTBZ_"));
    ptr.reset(new TBZ());
    break;
  case kTBNZ_:
    DBG_(Iface, (<< "TBNZ"));
    ptr.reset(new TBNZ());
    break;
  case kBCOND_:
    DBG_(Iface, (<< "BCOND"));
    ptr.reset(new BCOND());
    break;
  default:
    DBG_Assert(false);
    return nullptr;
  }
  return ptr;
}

} // namespace narmDecoder
