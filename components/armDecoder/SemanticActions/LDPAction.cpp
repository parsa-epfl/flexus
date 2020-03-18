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

#include <iomanip>
#include <iostream>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;
using nuArchARM::Instruction;

struct LDPAction : public PredicatedSemanticAction {
  eSize theSize;
  eSignCode theSignExtend;
  boost::optional<eOperandCode> theBypass0;
  boost::optional<eOperandCode> theBypass1;
  bool theLoadExtended;

  LDPAction(SemanticInstruction *anInstruction, eSize aSize, eSignCode aSigncode,
            boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1)
      : PredicatedSemanticAction(anInstruction, 1, true), theSize(aSize), theSignExtend(aSigncode),
        theBypass0(aBypass0), theBypass1(aBypass1) {
  }

  void satisfy(int32_t anArg) {
    BaseSemanticAction::satisfy(anArg);
    SEMANTICS_DBG(*theInstruction);
    if (!cancelled() && ready() && thePredicate) {
      doLoad();
    }
  }

  void predicate_on(int32_t anArg) {
    PredicatedSemanticAction::predicate_on(anArg);
    if (!cancelled() && ready() && thePredicate) {
      doLoad();
    }
  }

  void doLoad() {
    SEMANTICS_DBG(*this);
    bits value;
    uint64_t size;
    value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
    switch (theSize) {
    case kQuadWord:
      size = 128;
      break;
    default:
      size = 64;
      break;
    }

    Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
    uint64_t addr = theInstruction->operand<uint64_t>(kAddress);
    uint64_t addr_final = addr + (size >> 3) - 1;
    bits value_new = c->readVirtualAddress(VirtualMemoryAddress(addr), size >> 3);
    if (((addr & 0x1000) != (addr_final & 0x1000)) && value != value_new) {
      DBG_(Iface, (<< theInstruction->identify()
                   << " Correcting LDP access across pages at address: " << std::hex << addr
                   << ", size: " << size << ", original: " << value << ", final: " << value_new));
      core()->setLoadValue(boost::intrusive_ptr<Instruction>(theInstruction), value_new);
      value = value_new;
    }
    std::pair<uint64_t, uint64_t> pairValues = splitBits(value, size);

    theInstruction->setOperand(kResult, pairValues.second);
    theInstruction->setOperand(kResult1, pairValues.first);

    DBG_(Iface, (<< *this << " received pair load value = " << std::hex << value << std::dec));
    DBG_(Iface, (<< *this << " received load values = " << std::hex << pairValues.first << " and "
                 << pairValues.second << std::dec));

    if (theBypass0) {
      mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
      core()->bypass(name, pairValues.second);
    }
    if (theBypass1) {
      mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass1);
      core()->bypass(name, pairValues.first);
    }
    satisfyDependants();
  }

  void doEvaluate() {
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " LDPAction";
  }
};

predicated_dependant_action ldpAction(SemanticInstruction *anInstruction, eSize aSize,
                                      eSignCode aSignCode, boost::optional<eOperandCode> aBypass0,
                                      boost::optional<eOperandCode> aBypass1) {
  LDPAction *act = new LDPAction(anInstruction, aSize, aSignCode, aBypass0, aBypass1);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}
predicated_dependant_action caspAction(SemanticInstruction *anInstruction, eSize aSize,
                                       eSignCode aSignCode, boost::optional<eOperandCode> aBypass0,
                                       boost::optional<eOperandCode> aBypass1) {
  LDPAction *act = new LDPAction(anInstruction, aSize, aSignCode, aBypass0, aBypass1);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}
} // namespace narmDecoder
