// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

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
