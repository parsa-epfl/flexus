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

struct BranchCCAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  bool theAnnul;
  std::unique_ptr<Condition> theCondition;
  bool isFloating;
  uint32_t theFeedbackCount;

  BranchCCAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, std::unique_ptr<Condition> & aCondition, bool floating)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(std::move(aCondition))
    , isFloating(floating)
    , theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {


      std::vector< Operand > operands;
      for (int32_t i = 0; i < numOperands(); ++i) {
        operands.push_back( op( kOperand1 + i ) );
      }

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

//        theCondition.setInstruction(theInstruction);

        bool result/* = theCondition(operands)*/;

//        if (isFloating) {
//          FCondition cond = fcondition( theCondition );
//          result = cond(cc);
//          DBG_( Tmp, ( << *this << " Floating condition: " << theCondition << " cc: " << cc.to_ulong() << " result: " << result ) );
//        } else {
//          Condition cond = condition( theCondition );
//          result = cond(cc);
//        }

        if ( result ) {
          //Taken
//          DBG_( Tmp, ( << *this << " conditional branch CC: " << cc << " TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

          if (theAnnul) {
            DBG_( Tmp, ( << *this << " Reinstate Next Instruction") );
            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
//            theInstruction->redirectNPC( theInstruction->pc() + 4 );
          }

        } else {
          //Not Taken
//          DBG_( Tmp, ( << *this << " conditional branch CC: " << cc << " NOT TAKEN" ) );
          feedback->theActualDirection = kNotTaken;

          if (theAnnul) {
            DBG_( Tmp, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction, annulInstructionInteraction() );
//            theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
          }
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

struct BranchCondAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  bool theAnnul;
  uint32_t theCondition;
  bool isFloating;
  uint32_t theFeedbackCount;
  bool theOnZero;

  BranchCondAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool onZero)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theFeedbackCount(0)
    , theOnZero(onZero)
  {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {

        bits cond = theInstruction->operand< bits> (kOperand1);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();


//        Condition cond = condition( theCondition );


        bool cc;
//        if (theOnZero) {
//            cc = cond == 0;
//        }
//        else
//        {
//            cc = cond != 0;
//        }

        if ( cc ) {
          //Taken
//          DBG_( Tmp, ( << *this << " conditional branch Cond: " << cc << " TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

          if (theAnnul) {
//            DBG_( Tmp, ( << *this << " Reinstate Next Instruction") );
            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
//            theInstruction->redirectNPC( theInstruction->pc() + 4 );
          }

        } else {
          //Not Taken
//          DBG_( Tmp, ( << *this << " conditional branch CC: " << cc << " NOT TAKEN" ) );
          feedback->theActualDirection = kNotTaken;

          if (theAnnul) {
//            DBG_( Tmp, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction, annulInstructionInteraction() );
//            theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
          }
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
};

dependant_action branchCCAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, std::unique_ptr<Condition> aCondition, bool floating)
{
  BranchCCAction * act(new(anInstruction->icb()) BranchCCAction ( anInstruction, aTarget, anAnnul, aCondition, floating ) );

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
