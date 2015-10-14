#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <functional>

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

struct ReadFPRSAction : public PredicatedSemanticAction {
  eOperandCode theResult;

  ReadFPRSAction( SemanticInstruction * anInstruction, eOperandCode aResult )
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theResult( aResult ) {
    setReady( 0, true );
  }

  void doEvaluate() {
    uint64_t fprs = theInstruction->core()->getFPRS() ;
    theInstruction->setOperand(theResult, fprs);
    DBG_( Verb, ( << *this << " read FPRS") );
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " Read FPRS store in " << theResult;
  }

};

predicated_action readFPRSAction
( SemanticInstruction * anInstruction
) {
  ReadFPRSAction * act(new(anInstruction->icb()) ReadFPRSAction( anInstruction, kResult) );

  return predicated_action( act, act->predicate() );
}

struct ReadTICKAction : public PredicatedSemanticAction {
  eOperandCode theResult;

  ReadTICKAction( SemanticInstruction * anInstruction, eOperandCode aResult )
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theResult( aResult ) {
    setReady( 0, true );
  }

  void doEvaluate() {
    uint64_t tick = theInstruction->core()->getTICK() ;
    theInstruction->setOperand(theResult, tick);
    DBG_( Verb, ( << *this << " read TICK") );
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " Read TICK store in " << theResult;
  }

};

predicated_action readTICKAction
( SemanticInstruction * anInstruction
) {
  ReadTICKAction * act(new(anInstruction->icb()) ReadTICKAction( anInstruction, kResult) );

  return predicated_action( act, act->predicate() );
}

struct ReadSTICKAction : public PredicatedSemanticAction {
  eOperandCode theResult;

  ReadSTICKAction( SemanticInstruction * anInstruction, eOperandCode aResult )
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theResult( aResult ) {
    setReady( 0, true );
  }

  void doEvaluate() {
    uint64_t stick = theInstruction->core()->getSTICK() ;
    theInstruction->setOperand(theResult, stick);
    DBG_( Verb, ( << *this << " read STICK") );
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " Read STICK store in " << theResult;
  }

};

predicated_action readSTICKAction
( SemanticInstruction * anInstruction
) {
  ReadSTICKAction * act(new(anInstruction->icb()) ReadSTICKAction( anInstruction, kResult) );

  return predicated_action( act, act->predicate() );
}

} //nv9Decoder
