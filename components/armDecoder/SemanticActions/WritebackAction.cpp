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

#include <boost/dynamic_bitset.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "RegisterValueExtractor.hpp"

#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct WritebackAction : public BaseSemanticAction {
  eOperandCode theResult;
  eOperandCode theRd;
  bool the64, theSetflags, theSP;

  WritebackAction(SemanticInstruction *anInstruction, eOperandCode aResult, eOperandCode anRd,
                  bool a64, bool aSP, bool setflags = false)
      : BaseSemanticAction(anInstruction, 1), theResult(aResult), theRd(anRd), the64(a64),
        theSetflags(setflags), theSP(aSP) {
  }

  void squash(int32_t anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand<mapped_reg>(theRd);
      core()->squashRegister(name);
      DBG_(VVerb, (<< *this << " squashing register rd=" << name));
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {

    if (ready()) {
      DBG_(Iface, (<< "Writing " << theResult << " to " << theRd));

      register_value result =
          boost::apply_visitor(register_value_extractor(), theInstruction->operand(theResult));
      uint64_t res = boost::get<uint64_t>(result);

      mapped_reg name = theInstruction->operand<mapped_reg>(theRd);
      char r = the64 ? 'X' : 'W';
      if (!the64) {
        res &= 0xFFFFFFFF;
        result = res;
        theInstruction->setOperand(theResult, res);
      }
      DBG_(Iface,
           (<< "Writing " << std::hex << result << std::dec << " to " << r << "-REG ->" << name));
      core()->writeRegister(name, result, !the64);
      DBG_(VVerb, (<< *this << " rd= " << name << " result=" << result));
      core()->bypass(name, result);
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " WritebackAction to" << theRd;
  }
};

struct WriteccAction : public BaseSemanticAction {
  eOperandCode theCC;
  bool the64;

  WriteccAction(SemanticInstruction *anInstruction, eOperandCode anCC, bool an64)
      : BaseSemanticAction(anInstruction, 1), theCC(anCC), the64(an64) {
  }

  void squash(int32_t anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand<mapped_reg>(theCC);
      core()->squashRegister(name);
      DBG_(VVerb, (<< *this << " squashing register cc=" << name));
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {

    if (ready()) {
      mapped_reg name = theInstruction->operand<mapped_reg>(theCC);
      uint64_t ccresult =
          Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readPSTATE();
      uint64_t flags = boost::get<uint64_t>(
          boost::apply_visitor(register_value_extractor(), theInstruction->operand(kResultCC)));
      if (!the64) {
        uint64_t result = boost::get<uint64_t>(
            boost::apply_visitor(register_value_extractor(), theInstruction->operand(kResult)));
        flags &= ~PSTATE_N;
        flags |= (result & ((uint64_t)1 << 31)) ? PSTATE_N : 0;
        flags |= !(result & 0xFFFFFFFF) ? PSTATE_Z : 0;
        flags |= ((result >> 32) == 1) ? PSTATE_C : 0;
      }
      ccresult &= ~(0xF << 28);
      ccresult |= flags;
      DBG_(Iface,
           (<< "Writing to CC: " << std::hex << ccresult << ", " << std::dec << *theInstruction));
      core()->writeRegister(name, ccresult, false);
      core()->bypass(name, ccresult);
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " WriteCCAction to" << theCC;
  }
};

dependant_action writebackAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                                 eOperandCode aMappedRegisterCode, bool is64, bool aSP,
                                 bool setflags) {
  WritebackAction *act =
      new WritebackAction(anInstruction, aRegisterCode, aMappedRegisterCode, is64, aSP, setflags);
  anInstruction->addNewComponent(act);
  return dependant_action(act, act->dependance());
}

dependant_action writeccAction(SemanticInstruction *anInstruction, eOperandCode aMappedRegisterCode,
                               bool is64) {
  WriteccAction *act = new WriteccAction(anInstruction, aMappedRegisterCode, is64);
  anInstruction->addNewComponent(act);
  return dependant_action(act, act->dependance());
}

} // namespace narmDecoder
