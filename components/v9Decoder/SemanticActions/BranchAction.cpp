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

#include <components/uArch/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

struct BranchCCAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  bool theAnnul;
  uint32_t theCondition;
  bool isFloating;
  uint32_t theFeedbackCount;

  BranchCCAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, bool floating)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , isFloating(floating)
    , theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {

        std::bitset<8> cc = theInstruction->operand< std::bitset<8> > (kOperand1);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        bool result;
        if (isFloating) {
          FCondition cond = fcondition( theCondition );
          result = cond(cc);
          DBG_( Verb, ( << *this << " Floating condition: " << theCondition << " cc: " << cc.to_ulong() << " result: " << result ) );
        } else {
          Condition cond = condition( theCondition );
          result = cond(cc);
        }

        if ( result ) {
          //Taken
          DBG_( Verb, ( << *this << " conditional branch CC: " << cc << " TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Reinstate Next Instruction") );
            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 4 );
          }

        } else {
          //Not Taken
          DBG_( Verb, ( << *this << " conditional branch CC: " << cc << " NOT TAKEN" ) );
          feedback->theActualDirection = kNotTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction, annulInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
          }
          core()->applyToNext( theInstruction, branchInteraction( theInstruction->pc() + 8  ) );

        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchCCAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, bool floating) {
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
        uint64_t val = theInstruction->operand< uint64_t > (kOperand1);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        RCondition cond( rcondition(theCondition) );

        if ( cond(val) ) {
          //Taken
          DBG_( Verb, ( << *this << " conditional branch val: " << val << " TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 4 );
          }

        } else {
          //Not Taken
          DBG_( Verb, ( << *this << " conditional branch val: " << val << " NOT TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction( VirtualMemoryAddress(0) ) );
          feedback->theActualDirection = kNotTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction, annulInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
          }
        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
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
        uint64_t target = theInstruction->operand< uint64_t > (theTarget);
        VirtualMemoryAddress target_addr(target);
        DBG_( Verb, ( << *this << " branc to reg target: " << target_addr ) );
        core()->applyToNext( theInstruction, branchInteraction(target_addr) );

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
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

} //nv9Decoder
