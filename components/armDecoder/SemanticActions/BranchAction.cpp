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

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct BranchCondAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  std::unique_ptr<Condition> theCondition;
  uint32_t theFeedbackCount;

  BranchCondAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, std::unique_ptr<Condition> & aCondition)
    : BaseSemanticAction ( anInstruction, 1 )
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

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        bool result = theCondition->operator ()(operands);

        if ( result ) {
          //Taken
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

        } else {
          feedback->theActualDirection = kNotTaken;
          core()->applyToNext( theInstruction, branchInteraction( theInstruction->pc() + 8  ) );

        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Tmp, ( << *this << " waiting for predecessor ") );
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
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, std::unique_ptr<Condition> aCondition)
{
  BranchCondAction * act(new(anInstruction->icb()) BranchCondAction ( anInstruction, aTarget, aCondition ) );

  return dependant_action( act, act->dependance() );
}

struct BranchRegAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  bool theAnnul;
  uint32_t theCondition;
  uint32_t theFeedbackCount;

  BranchRegAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {
        bits val = theInstruction->operand< bits > (kOperand1);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

//        RCondition cond( rcondition(theCondition) );

//        if ( cond(val) ) {
//          //Taken
//          DBG_( Tmp, ( << *this << " conditional branch val: " << val << " TAKEN" ) );
//          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
//          feedback->theActualDirection = kTaken;

//          if (theAnnul) {
//            DBG_( Tmp, ( << *this << " Annul Next Instruction") );
//            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
//  //          theInstruction->redirectNPC( theInstruction->pc() + 4 );
//          }

//        } else {
//          //Not Taken
//          DBG_( Tmp, ( << *this << " conditional branch val: " << val << " NOT TAKEN" ) );
//          core()->applyToNext( theInstruction, branchInteraction( VirtualMemoryAddress(0) ) );
//          feedback->theActualDirection = kNotTaken;

//          if (theAnnul) {
//            DBG_( Tmp, ( << *this << " Annul Next Instruction") );
//            core()->applyToNext( theInstruction, annulInstructionInteraction() );
// //           theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
//          }
//        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Tmp, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchRegAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition) {
  BranchRegAction * act(new(anInstruction->icb()) BranchRegAction ( anInstruction, aTarget, anAnnul, aCondition) );

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
        bits target = theInstruction->operand< bits > (theTarget);
        VirtualMemoryAddress target_addr(target.to_ulong());
        DBG_( Tmp, ( << *this << " branc to mapped_reg target: " << target_addr ) );
        core()->applyToNext( theInstruction, branchInteraction(target_addr) );

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Tmp, ( << *this << " waiting for predecessor ") );
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
