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
#include "PredicatedSemanticAction.hpp"
#include "RegisterValueExtractor.hpp"
#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct ConditionSelectAction : public PredicatedSemanticAction {
  std::unique_ptr<Condition> theOperation;
  eOperandCode theResult;
  uint64_t theCondCode;
  bool theInvert, theIncrement, the64;

  Operand op(eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }

  ConditionSelectAction(SemanticInstruction *anInstruction, uint32_t aCode, eOperandCode aResult,
                        std::unique_ptr<Condition> &anOperation, bool anInvert, bool anIncrement,
                        bool a64)
      : PredicatedSemanticAction(anInstruction, 3, true), theOperation(std::move(anOperation)),
        theResult(aResult), theCondCode(aCode), theInvert(anInvert), theIncrement(anIncrement),
        the64(a64) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        theOperation->setInstruction(theInstruction);
        std::vector<Operand> operands;
        operands.push_back(op(kOperand3));
        operands.push_back(op(kCondition));
        bool result = theOperation->operator()(operands);

        if (result) {
          theInstruction->setOperand(theResult, op(kOperand1));
        } else {
          theInstruction->setOperand(theResult, op(kOperand2));

          if (theInvert) {
            std::unique_ptr<Operation> op = operation(kNot_);
            std::vector<Operand> operands = {theInstruction->operand(kResult)};
            Operand res = op->operator()(operands);
            theInstruction->setOperand(theResult, res);
          }
          if (theIncrement) {
            std::unique_ptr<Operation> op = operation(kADD_);
            std::vector<Operand> operands = {theInstruction->operand(kResult),
                                             theInstruction->operand(kOperand5)};
            Operand res = op->operator()(operands);
            theInstruction->setOperand(theResult, res);
          }
        }
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ConditionSelectAction "
              << theOperation->describe();
  }
};

struct ConditionCompareAction : public PredicatedSemanticAction {
  std::unique_ptr<Condition> theOperation;
  eOperandCode theResult;
  uint64_t theCondCode;
  bool theSub_op;
  bool the64;

  Operand op(eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }

  ConditionCompareAction(SemanticInstruction *anInstruction, uint32_t aCode, eOperandCode aResult,
                         std::unique_ptr<Condition> &anOperation, bool sub_op, bool a64)
      : PredicatedSemanticAction(anInstruction, 3, true), theOperation(std::move(anOperation)),
        theResult(aResult), theCondCode(aCode), theSub_op(sub_op), the64(a64) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        theOperation->setInstruction(theInstruction);
        std::vector<Operand> operands;
        operands.push_back(op(kOperand3));
        operands.push_back(op(kCondition));
        bool result = theOperation->operator()(operands);
        std::unique_ptr<Operation> op;
        uint64_t nzcv = boost::get<uint64_t>(theInstruction->operand(kOperand4)) << 28;
        Operand res = (nzcv & PSTATE_N) ? 0xFFFFFFFFFFFFFFFF : 0;
        if (result) {
          if (theSub_op) {
            op = operation(kSUBS_);
          } else {
            op = operation(kADDS_);
          }

          std::vector<Operand> operands = {theInstruction->operand(kOperand1),
                                           theInstruction->operand(kOperand2)};
          res = op->operator()(operands);
          nzcv = op->getNZCVbits();
          if (!the64) {
            uint64_t result = boost::get<uint64_t>(res);
            nzcv &= ~PSTATE_N;
            nzcv |= (result & ((uint64_t)1 << 31)) ? PSTATE_N : 0;
            nzcv |= !(result & 0xFFFFFFFF) ? PSTATE_Z : 0;
            nzcv |= ((result >> 32) == 1) ? PSTATE_C : 0;
          }
        }
        theInstruction->setOperand(theResult, res);
        theInstruction->setOperand(kResultCC, nzcv);
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ConditionCompareAction "
              << theOperation->describe();
  }
};

predicated_action conditionSelectAction(SemanticInstruction *anInstruction,
                                        std::unique_ptr<Condition> &anOperation, uint64_t aCode,
                                        std::vector<std::list<InternalDependance>> &opDeps,
                                        eOperandCode aResult, bool anInvert, bool anIncrement,
                                        bool a64) {
  ConditionSelectAction *act = new ConditionSelectAction(anInstruction, aCode, aResult, anOperation,
                                                         anInvert, anIncrement, a64);
  anInstruction->addNewComponent(act);
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

predicated_action conditionCompareAction(SemanticInstruction *anInstruction,
                                         std::unique_ptr<Condition> &anOperation, uint32_t aCode,
                                         std::vector<std::list<InternalDependance>> &opDeps,
                                         eOperandCode aResult, bool aSub_op, bool a64) {
  ConditionCompareAction *act =
      new ConditionCompareAction(anInstruction, aCode, aResult, anOperation, aSub_op, a64);
  anInstruction->addNewComponent(act);
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

} // namespace narmDecoder
