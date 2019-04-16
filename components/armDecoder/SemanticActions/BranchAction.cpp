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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
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


#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"
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

  BranchCondAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, std::unique_ptr<Condition> & aCondition, size_t numOperands)
    : BaseSemanticAction ( anInstruction, numOperands )
    , theTarget(aTarget)
    , theCondition(std::move(aCondition))
    , theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {


        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
        operands.push_back( op(eOperandCode( kOperand1 + i)) );
        }

        if (theInstruction->hasOperand(kCondition)){
            operands.push_back(theInstruction->operand(kCondition));
        }

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        theCondition->setInstruction(theInstruction);

        bool result = theCondition->operator ()(operands);

        if ( result ) {
          //Taken
            if (theInstruction->redirectPC(theInstruction->pc() + 4, theTarget)){
                if ( theInstruction->core()->squashAfter(theInstruction) ) {
                  theInstruction->core()->redirectFetch(theTarget);
                  theInstruction->addPostvalidation(validatePC((uint64_t)theTarget, theInstruction));
                }
            }
//          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

        } else {
          theInstruction->core()->redirectFetch(theInstruction->pc() + 4);
          feedback->theActualDirection = kNotTaken;
          DBG_(Dev, (<< "Branching Not taken! "));
        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( VVerb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }

  Operand op( eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }
};
dependant_action branchCondAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, std::unique_ptr<Condition> aCondition, size_t numOperands)
{
  BranchCondAction * act(new(anInstruction->icb()) BranchCondAction ( anInstruction, aTarget, aCondition, numOperands ) );

  return dependant_action( act, act->dependance() );
}

struct BranchRegAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  eOperandCode theRegOperand;


  BranchRegAction( SemanticInstruction * anInstruction, eOperandCode aRegOperand)
    : BaseSemanticAction ( anInstruction, 1 )
    , theRegOperand(aRegOperand) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {

      if (theInstruction->hasPredecessorExecuted()) {

        DBG_( Dev, ( << *this << " Branching to an address held in register " << theRegOperand) );

        uint64_t target = boost::get<uint64_t>(theInstruction->operand< uint64_t > (kOperand1));

        theTarget = VirtualMemoryAddress(target);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kUnconditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();
        theInstruction->setBranchFeedback(feedback);

        DBG_( Dev, ( << *this << " Checking for redirection PC= " << theInstruction->pc() << " target= " << theTarget) );

        if (theInstruction->redirectPC(theInstruction->pc() + 4, theTarget)){
            // must redirect
            if ( theInstruction->core()->squashAfter(theInstruction) ) {
              theInstruction->core()->redirectFetch(theTarget);
              theInstruction->addPostvalidation(validatePC((uint64_t)theTarget, theInstruction));
            }
        }

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( VVerb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchRegAction
( SemanticInstruction * anInstruction, eOperandCode aRegOperand) {
  BranchRegAction * act(new(anInstruction->icb()) BranchRegAction ( anInstruction, aRegOperand) );

  return dependant_action( act, act->dependance() );
}

struct BranchToCalcAddressAction : public BaseSemanticAction {
  eOperandCode theTarget;
  uint32_t theFeedbackCount;

  BranchToCalcAddressAction( SemanticInstruction * anInstruction, eOperandCode aTarget)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {

        //Feedback is taken care of by the updateUncoditional effect at retirement
        uint64_t target = theInstruction->operand< uint64_t > (theTarget);
        VirtualMemoryAddress target_addr(target);
        DBG_( Dev, ( << *this << " branc to mapped_reg target: " << target_addr ) );

        core()->applyToNext( theInstruction, branchInteraction(target_addr) );

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( VVerb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchToCalcAddress Action";
  }
};

dependant_action branchToCalcAddressAction
( SemanticInstruction * anInstruction) {
  BranchToCalcAddressAction * act(new(anInstruction->icb()) BranchToCalcAddressAction ( anInstruction, kAddress ) );

  return dependant_action( act, act->dependance() );
}

} //narmDecoder
