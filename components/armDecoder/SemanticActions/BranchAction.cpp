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
#include "../Validations.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct BranchCondAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  std::unique_ptr<Condition> theCondition;
  uint32_t theFeedbackCount;

  BranchCondAction(SemanticInstruction *anInstruction, VirtualMemoryAddress aTarget,
                   std::unique_ptr<Condition> &aCondition, size_t numOperands)
      : BaseSemanticAction(anInstruction, numOperands), theTarget(aTarget),
        theCondition(std::move(aCondition)), theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back(op(eOperandCode(kOperand1 + i)));
        }

        if (theInstruction->hasOperand(kCondition)) {
          operands.push_back(theInstruction->operand(kCondition));
        }

        boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        theCondition->setInstruction(theInstruction);

        bool result = theCondition->operator()(operands);

        if (result) {
          // Taken
          theInstruction->redirectPC(theTarget);
          core()->applyToNext(theInstruction, branchInteraction(theTarget));
          feedback->theActualDirection = kTaken;
          DBG_(Iface, (<< "Branch taken! " << *theInstruction));
        } else {
          theInstruction->redirectPC(theInstruction->pc() + 4);
          core()->applyToNext(theInstruction, branchInteraction(theInstruction->pc() + 4));
          feedback->theActualDirection = kNotTaken;
          DBG_(Iface, (<< "Branch Not taken! " << *theInstruction));
        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }

  Operand op(eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }
};
dependant_action branchCondAction(SemanticInstruction *anInstruction, VirtualMemoryAddress aTarget,
                                  std::unique_ptr<Condition> aCondition, size_t numOperands) {
  BranchCondAction *act = new BranchCondAction(anInstruction, aTarget, aCondition, numOperands);
  anInstruction->addNewComponent(act);

  return dependant_action(act, act->dependance());
}

struct BranchRegAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  eOperandCode theRegOperand;
  eBranchType theType;

  BranchRegAction(SemanticInstruction *anInstruction, eOperandCode aRegOperand, eBranchType aType)
      : BaseSemanticAction(anInstruction, 1), theRegOperand(aRegOperand), theType(aType) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        DBG_(Iface, (<< *this << " Branching to an address held in register " << theRegOperand));

        uint64_t target = boost::get<uint64_t>(theInstruction->operand<uint64_t>(kOperand1));

        theTarget = VirtualMemoryAddress(target);

        boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = theType;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();
        theInstruction->setBranchFeedback(feedback);

        DBG_(Iface, (<< *this << " Checking for redirection PC= " << theInstruction->pc()
                     << " target= " << theTarget));

        theInstruction->redirectPC(theTarget);
        core()->applyToNext(theInstruction, branchInteraction(theTarget));

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchRegAction(SemanticInstruction *anInstruction, eOperandCode aRegOperand,
                                 eBranchType type) {
  BranchRegAction *act = new BranchRegAction(anInstruction, aRegOperand, type);
  anInstruction->addNewComponent(act);
  return dependant_action(act, act->dependance());
}

struct BranchToCalcAddressAction : public BaseSemanticAction {
  eOperandCode theTarget;
  uint32_t theFeedbackCount;

  BranchToCalcAddressAction(SemanticInstruction *anInstruction, eOperandCode aTarget)
      : BaseSemanticAction(anInstruction, 1), theTarget(aTarget), theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        // Feedback is taken care of by the updateUncoditional effect at
        // retirement
        uint64_t target = theInstruction->operand<uint64_t>(theTarget);
        VirtualMemoryAddress target_addr(target);
        DBG_(Iface, (<< *this << " branc to mapped_reg target: " << target_addr));

        theInstruction->redirectPC(target_addr);
        core()->applyToNext(theInstruction, branchInteraction(target_addr));

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BranchToCalcAddress Action";
  }
};

dependant_action branchToCalcAddressAction(SemanticInstruction *anInstruction) {
  BranchToCalcAddressAction *act = new BranchToCalcAddressAction(anInstruction, kAddress);
  anInstruction->addNewComponent(act);

  return dependant_action(act, act->dependance());
}

} // namespace narmDecoder
