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

struct ExecuteBase : public PredicatedSemanticAction {
  eOperandCode theOperands[5];
  eOperandCode theResult;
  std::unique_ptr<Operation> theOperation;

  ExecuteBase(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands,
              eOperandCode aResult, std::unique_ptr<Operation> &anOperation)
      : PredicatedSemanticAction(anInstruction, anOperands.size(), true), theResult(aResult),
        theOperation(std::move(anOperation)) {

    for (int32_t i = 0; i < numOperands(); ++i) {
      theOperands[i] = anOperands[i];
    }
  }

  Operand op(eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }
};

struct OperandPrintHelper {
  std::vector<Operand> &theOperands;
  OperandPrintHelper(std::vector<Operand> &operands) : theOperands(operands) {
  }
  friend std::ostream &operator<<(std::ostream &anOstream, OperandPrintHelper const &aHelper) {
    int32_t i = 0;
    std::for_each(aHelper.theOperands.begin(), aHelper.theOperands.end(),
                  ll::var(anOstream)
                      << " op" << ++ll::var(i) << "=" << std::hex << ll::_1 << std::dec);
    return anOstream;
  }
};

struct ExecuteAction : public ExecuteBase {
  boost::optional<eOperandCode> theBypass;
  bool theFlags;

  ExecuteAction(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands,
                eOperandCode aResult, std::unique_ptr<Operation> &anOperation,
                boost::optional<eOperandCode> aBypass)
      : ExecuteBase(anInstruction, anOperands, aResult, anOperation), theBypass(aBypass) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    DBG_(Iface, (<< "Trying to Execute " << *this));

    if (ready()) {

      DBG_(Iface, (<< "Executing " << *this));

      if (theInstruction->hasPredecessorExecuted()) {
        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back(op(theOperands[i]));
        }

        theOperation->theInstruction = theInstruction;
        Operand result = theOperation->operator()(operands);
        if (theOperation->hasNZCVFlags()) {
          uint64_t nzcv = theOperation->getNZCVbits();
          theInstruction->setOperand(kResultCC, nzcv);
        }

        if (theOperation->is128()) {
          bits res = boost::get<bits>(result);
          theInstruction->setOperand(theResult, res);
          DBG_(Iface, (<< "Writing " << res << " in [ " << result << " -> " << theResult << " ]"));
        } else {
          uint64_t res = boost::get<uint64_t>(result);
          theInstruction->setOperand(theResult, res);
          DBG_(Iface, (<< "Writing " << std::hex << res << std::dec << " in " << theResult));
        }
        DBG_(Iface, (<< *this << " operands: " << OperandPrintHelper(operands) << std::hex
                     << " result=" << result << std::dec));
        if (theBypass) {
          mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
          register_value val = boost::apply_visitor(register_value_extractor(), result);
          core()->bypass(name, val);
        }
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(Iface, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    } else {
      DBG_(Iface, (<< "cant Execute " << *this << " yet"));
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ExecuteAction " << theOperation->describe();
  }
};

struct ExecuteAction_WithXTRA : public ExecuteBase {
  eOperandCode theXTRA;
  boost::optional<eOperandCode> theBypass;
  boost::optional<eOperandCode> theBypassXTRA;

  ExecuteAction_WithXTRA(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands,
                         eOperandCode aResult, eOperandCode anXTRA,
                         std::unique_ptr<Operation> &anOperation,
                         boost::optional<eOperandCode> aBypassResult,
                         boost::optional<eOperandCode> aBypassXTRA)
      : ExecuteBase(anInstruction, anOperands, aResult, anOperation), theXTRA(anXTRA),
        theBypass(aBypassResult), theBypassXTRA(aBypassXTRA) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {
        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back(op(theOperands[i]));
        }

        Operand result = theOperation->operator()(operands);
        Operand xtra = theOperation->evalExtra(operands);
        theInstruction->setOperand(theResult, result);
        theInstruction->setOperand(theXTRA, xtra);
        DBG_(VVerb, (<< *this << " operands: " << OperandPrintHelper(operands)
                     << " result=" << result << " xtra=" << xtra));
        if (theBypass) {
          mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
          register_value val = boost::apply_visitor(register_value_extractor(), result);
          core()->bypass(name, val);
        }
        if (theBypassXTRA) {
          mapped_reg name = theInstruction->operand<mapped_reg>(*theBypassXTRA);
          register_value val = boost::apply_visitor(register_value_extractor(), xtra);
          core()->bypass(name, val);
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
    anOstream << theInstruction->identify() << " ExecuteAction " << theOperation->describe();
  }
};

struct FPExecuteAction : public ExecuteBase {
  eSize theSize;

  FPExecuteAction(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands,
                  std::unique_ptr<Operation> &anOperation, eSize aSize)
      : ExecuteBase(anInstruction, anOperands, (aSize == kByte ? kResultCC : kfResult0),
                    anOperation),
        theSize(aSize) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {
        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back(op(theOperands[i]));
        }

        //        theOperation.setContext( core()->getRoundingMode() )
        Operand result = theOperation->operator()(operands);
        if (theSize == kDoubleWord) {
          bits val = boost::get<bits>(result);
          theInstruction->setOperand(theResult, val >> 32);
          mapped_reg name0 = theInstruction->operand<mapped_reg>(kPFD0);
          core()->bypass(name0, val >> 32);
          theInstruction->setOperand(eOperandCode(theResult + 1), val & (bits)0xFFFFFFFFULL);
          mapped_reg name1 = theInstruction->operand<mapped_reg>(kPFD1);
          core()->bypass(name1, val & (bits)0xFFFFFFFFULL);

        } else if (theSize == kWord) {
          // Word
          theInstruction->setOperand(theResult, result);
          mapped_reg name = theInstruction->operand<mapped_reg>(kPFD0);
          core()->bypass(name, boost::apply_visitor(register_value_extractor(), result));
        } else if (theSize == kByte) {
          // Indicates the CC output of an fp_cmp
          theInstruction->setOperand(theResult, result);
          mapped_reg name = theInstruction->operand<mapped_reg>(kCCpd);
          core()->bypass(name, boost::apply_visitor(register_value_extractor(), result));
        }
        DBG_(VVerb, (<< *this << " operands: " << OperandPrintHelper(operands) << " val=" << result
                     << " theResult=" << theResult));

        //        bits fpsr = core()->readFPSR();
        //        if (fpsr & 0xf) {
        //          core()->writeFPSR(fpsr & ~0xf);
        //        }

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " FPExecuteAction " << theOperation->describe();
  }
};

predicated_action executeAction(SemanticInstruction *anInstruction,
                                std::unique_ptr<Operation> &anOperation,
                                std::vector<std::list<InternalDependance>> &opDeps,
                                eOperandCode aResult, boost::optional<eOperandCode> aBypass) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back(eOperandCode(kOperand1 + i));
  }
  ExecuteAction *act = new ExecuteAction(anInstruction, operands, aResult, anOperation, aBypass);
  anInstruction->addNewComponent(act);
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

predicated_action executeAction(SemanticInstruction *anInstruction,
                                std::unique_ptr<Operation> &anOperation,
                                std::vector<eOperandCode> &anOperands,
                                std::vector<std::list<InternalDependance>> &opDeps,
                                eOperandCode aResult, boost::optional<eOperandCode> aBypass) {

  ExecuteAction *act = new ExecuteAction(anInstruction, anOperands, aResult, anOperation, aBypass);
  anInstruction->addNewComponent(act);
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

predicated_action executeAction_XTRA(SemanticInstruction *anInstruction,
                                     std::unique_ptr<Operation> &anOperation,
                                     std::vector<std::list<InternalDependance>> &opDeps,
                                     boost::optional<eOperandCode> aBypass,
                                     boost::optional<eOperandCode> aBypassXTRA) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back(eOperandCode(kOperand1 + i));
  }
  ExecuteAction_WithXTRA *act = new ExecuteAction_WithXTRA(
      anInstruction, operands, kResult, kXTRAout, anOperation, aBypass, aBypassXTRA);
  anInstruction->addNewComponent(act);

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

predicated_action fpExecuteAction(SemanticInstruction *anInstruction,
                                  std::unique_ptr<Operation> &anOperation, int32_t num_ops,
                                  eSize aDestSize, eSize aSrcSize,
                                  std::vector<std::list<InternalDependance>> &deps) {
  std::vector<eOperandCode> operands;
  if (aSrcSize == kWord) {
    operands.push_back(kFOperand1_0);
    if (num_ops == 2) {
      operands.push_back(kFOperand2_0);
    }
  } else {
    // DoubleWord
    operands.push_back(kFOperand1_0);
    operands.push_back(kFOperand1_1);
    if (num_ops == 2) {
      operands.push_back(kFOperand2_0);
      operands.push_back(kFOperand2_1);
    }
  }
  FPExecuteAction *act = new FPExecuteAction(anInstruction, operands, anOperation, aDestSize);
  anInstruction->addNewComponent(act);

  if (aSrcSize == kWord) {
    deps.resize(2);
    deps[0].push_back(act->dependance(0));
    if (num_ops == 2) {
      deps[1].push_back(act->dependance(1));
    }
  } else {
    // DoubleWord
    deps.resize(4);
    deps[0].push_back(act->dependance(0));
    deps[1].push_back(act->dependance(1));
    if (num_ops == 2) {
      deps[2].push_back(act->dependance(2));
      deps[3].push_back(act->dependance(3));
    }
  }

  return predicated_action(act, act->predicate());
}

simple_action calcAddressAction(SemanticInstruction *anInstruction,
                                std::vector<std::list<InternalDependance>> &opDeps) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back(eOperandCode(kOperand1 + i));
  }
  std::unique_ptr<Operation> add = operation(kADD_);
  ExecuteAction *act = new ExecuteAction(anInstruction, operands, kAddress, add, boost::none);
  anInstruction->addNewComponent(act);

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return act;
}

predicated_action visOp(SemanticInstruction *anInstruction, std::unique_ptr<Operation> &anOperation,
                        std::vector<std::list<InternalDependance>> &deps) {
  std::vector<eOperandCode> operands;
  // DoubleWord
  operands.push_back(kFOperand1_0);
  operands.push_back(kFOperand1_1);
  operands.push_back(kFOperand2_0);
  operands.push_back(kFOperand2_1);
  operands.push_back(kOperand5);

  FPExecuteAction *act = new FPExecuteAction(anInstruction, operands, anOperation, kDoubleWord);
  anInstruction->addNewComponent(act);

  // DoubleWord
  deps.resize(5);
  deps[0].push_back(act->dependance(0));
  deps[1].push_back(act->dependance(1));
  deps[2].push_back(act->dependance(2));
  deps[3].push_back(act->dependance(3));
  deps[4].push_back(act->dependance(4));

  return predicated_action(act, act->predicate());
}

} // namespace narmDecoder
