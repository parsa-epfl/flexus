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
#include <boost/multiprecision/cpp_int.hpp>

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

struct UpdateStoreValueAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode, theOperandCode1, theOperandCode2;
  bool isPair;

  UpdateStoreValueAction(SemanticInstruction *anInstruction, eOperandCode anOperandCode)
      : PredicatedSemanticAction(anInstruction, 1, true), theOperandCode(anOperandCode) {
  }

  void predicate_off(int) {
    if (!cancelled() && thePredicate) {
      DBG_(VVerb, (<< *this << " predicated off. "));
      DBG_Assert(core());
      DBG_(VVerb, (<< *this << " anulling store"));
      core()->annulStoreValue(boost::intrusive_ptr<Instruction>(theInstruction));
      thePredicate = false;
      satisfyDependants();
    }
  }

  void predicate_on(int) {
    if (!cancelled() && !thePredicate) {
      DBG_(VVerb, (<< *this << " predicated on. "));
      reschedule();
      thePredicate = true;
      squashDependants();
    }
  }

  void doEvaluate() {
    // Should ensure that we got execution units here
    if (!cancelled()) {
      if (thePredicate && ready()) {
        uint64_t value = theInstruction->operand<uint64_t>(theOperandCode);
        DBG_(Iface, (<< *this << " updating store value=" << value));
        core()->updateStoreValue(boost::intrusive_ptr<Instruction>(theInstruction), value);
        satisfyDependants();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " UpdateStoreValue";
  }
};

struct UpdateCASValueAction : public BaseSemanticAction {

  eOperandCode theCompareCode, theCompareCode1, theNewCode, theNewCode1;
  bool thePair;

  UpdateCASValueAction(SemanticInstruction *anInstruction, eOperandCode aCompareCode,
                       eOperandCode aNewCode)
      : BaseSemanticAction(anInstruction, 3), theCompareCode(aCompareCode), theNewCode(aNewCode),
        thePair(false) {
  }

  UpdateCASValueAction(SemanticInstruction *anInstruction, eOperandCode aCompareCode1,
                       eOperandCode aCompareCode2, eOperandCode aNewCode1, eOperandCode aNewCode2,
                       bool aPair)
      : BaseSemanticAction(anInstruction, 5), theCompareCode(aCompareCode1),
        theCompareCode1(aCompareCode2), theNewCode(aNewCode1), theNewCode1(aNewCode2),
        thePair(aPair) {
  }

  void doEvaluate() {
    if (ready()) {
      if (!thePair) {
        uint64_t store_value = theInstruction->operand<uint64_t>(theNewCode);
        uint64_t cmp_value = theInstruction->operand<uint64_t>(theCompareCode);
        if ((cmp_value & (uint64_t)kMaxStore) == (uint64_t)kCheckAndStore) {
          uint64_t result = theInstruction->operand<uint64_t>(kResult);
          DBG_(Iface, (<< *this << " Read result as " << std::hex << result));
          cmp_value = (result << 32) | kAlwaysStore;
        }
        DBG_(Iface, (<< *this << " updating CAS write=" << std::hex << store_value
                     << " cmp=" << cmp_value));
        core()->updateCASValue(boost::intrusive_ptr<Instruction>(theInstruction), store_value,
                               cmp_value);
        satisfyDependants();
      } else {
        bits store_value = theInstruction->operand<bits>(theNewCode);
        bits store_value1 = theInstruction->operand<bits>(theNewCode1);
        bits cmp_value = theInstruction->operand<bits>(theCompareCode);
        bits cmp_value1 = theInstruction->operand<bits>(theCompareCode1);

        bits store = concat_bits(store_value, store_value1);
        bits cmp = concat_bits(cmp_value, cmp_value1);

        core()->updateCASValue(boost::intrusive_ptr<Instruction>(theInstruction), store, cmp);
        satisfyDependants();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " UpdateCASValue";
  }
};

struct UpdateSTPValueAction : public BaseSemanticAction {

  eOperandCode theOperand;
  UpdateSTPValueAction(SemanticInstruction *anInstruction, eOperandCode anOperandCode)
      : BaseSemanticAction(anInstruction, 2), theOperand(anOperandCode) {
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasOperand(theOperand)) {
        bits value = theInstruction->operand<bits>(theOperand);
        DBG_(Iface,
             (<< *this << " updating store value " << value << " from operand " << theOperand));
        core()->updateStoreValue(boost::intrusive_ptr<Instruction>(theInstruction), value);
        satisfyDependants();
      } else {
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " UpdateSTPValueAction";
  }
};

predicated_dependant_action updateStoreValueAction(SemanticInstruction *anInstruction,
                                                   eOperandCode data) {
  UpdateStoreValueAction *act = new UpdateStoreValueAction(anInstruction, data);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

multiply_dependant_action updateSTPValueAction(SemanticInstruction *anInstruction,
                                               eOperandCode data) {
  UpdateSTPValueAction *act = new UpdateSTPValueAction(anInstruction, data);
  anInstruction->addNewComponent(act);
  std::vector<InternalDependance> dependances;
  dependances.push_back(act->dependance(0));
  dependances.push_back(act->dependance(1));

  return multiply_dependant_action(act, dependances);
}

multiply_dependant_action updateCASValueAction(SemanticInstruction *anInstruction,
                                               eOperandCode aCompareCode, eOperandCode aNewCode) {
  UpdateCASValueAction *act = new UpdateCASValueAction(anInstruction, aCompareCode, aNewCode);
  anInstruction->addNewComponent(act);
  std::vector<InternalDependance> dependances;
  dependances.push_back(act->dependance(0));
  dependances.push_back(act->dependance(1));
  dependances.push_back(act->dependance(2));

  return multiply_dependant_action(act, dependances);
}

multiply_dependant_action updateCASPValueAction(SemanticInstruction *anInstruction,
                                                eOperandCode aCompareCode1,
                                                eOperandCode aCompareCode2, eOperandCode aNewCode1,
                                                eOperandCode aNewCode2) {
  UpdateCASValueAction *act = new UpdateCASValueAction(anInstruction, aCompareCode1, aCompareCode2,
                                                       aNewCode1, aNewCode2, true);
  anInstruction->addNewComponent(act);
  std::vector<InternalDependance> dependances;
  dependances.push_back(act->dependance(0));
  dependances.push_back(act->dependance(1));
  dependances.push_back(act->dependance(2));
  dependances.push_back(act->dependance(3));
  dependances.push_back(act->dependance(4));

  return multiply_dependant_action(act, dependances);
}

} // namespace narmDecoder
