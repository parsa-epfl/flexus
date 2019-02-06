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
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

void connect( std::list<InternalDependance> const & dependances, simple_action & aSource) {
  BaseSemanticAction & act = *(aSource.action);
  std::for_each
  ( dependances.begin()
    , dependances.end()
    , ll::bind( &BaseSemanticAction::addDependance, ll::var(act), ll::_1)
  );
}

void BaseSemanticAction::addRef() {
  /*
  ++theSemanticActionImbalance;
  if (theSemanticActionImbalance > theMaxSemanticActionImbalance  + 50) {
    theMaxSemanticActionImbalance = theSemanticActionImbalance;
    DBG_(Dev, ( << "Max outstanding semantic actions: " << theSemanticActionImbalance) );
  }
  */
  boost::intrusive_ptr_add_ref(theInstruction);
}

void BaseSemanticAction::releaseRef() {
  //--theSemanticActionImbalance;
  boost::intrusive_ptr_release(theInstruction);
}

void BaseSemanticAction::Dep::satisfy(int32_t anArg) {
  theAction.satisfy(anArg);
}
void BaseSemanticAction::Dep::squash(int32_t anArg) {
  theAction.squash(anArg);
}

void BaseSemanticAction::satisfyDependants() {
  if (! cancelled() && ! signalled() )  {
    for ( int32_t i = 0; i < theEndOfDependances; ++i) {
      theDependances[i].satisfy();
    }
    theSignalled = true;
    theSquashed = false;
  }
}

void BaseSemanticAction::satisfy(int32_t anArg) {
  if ( !cancelled() ) {
    bool was_ready = ready();
    setReady(anArg, true);
    theSquashed = false;
    if (! was_ready && ready() && core()) {
      setReady(anArg, true);
      reschedule();
    }
  }
}

void BaseSemanticAction::squash(int32_t anOperand) {
  DBG_(Verb, ( << *this << " squashed" ));
  setReady(anOperand, false);
  squashDependants();
}

void BaseSemanticAction::squashDependants() {
  if (! theSquashed) {
    if (core()) {
      for ( int32_t i = 0; i < theEndOfDependances; ++i) {
        theDependances[i].squash();
      }
    }
    theSignalled = false;
    theSquashed = true;
  }
}

void BaseSemanticAction::evaluate() {
  theScheduled = false;
  if ( !cancelled() ) {
    DBG_( VVerb, ( <<  *this << " Evaluated") );
    doEvaluate();
  }
}

void BaseSemanticAction::addDependance( InternalDependance const & aDependance) {
  DBG_Assert( theEndOfDependances < 4);
  theDependances[theEndOfDependances] = aDependance;
  ++theEndOfDependances;
}

void BaseSemanticAction::reschedule() {
  if (!theScheduled && core()) {
    DBG_(Verb, ( << *this << " rescheduled" ));
    theScheduled = true;
    core()->reschedule(this);
  }
}

void connectDependance( InternalDependance const & aDependant, simple_action & aSource) {
  aSource.action->addDependance( aDependant );
}

uArch * BaseSemanticAction::core() {
  return theInstruction->core();
}

PredicatedSemanticAction::PredicatedSemanticAction ( SemanticInstruction * anInstruction, int32_t aNumArgs, bool anInitialPredicate )
  : BaseSemanticAction( anInstruction, aNumArgs )
  , thePredicate(anInitialPredicate)
  , thePredicateTarget(*this)
{ }

void PredicatedSemanticAction::evaluate() {
  theScheduled = false;
  if (thePredicate) {
    BaseSemanticAction::evaluate();
  }
}

void PredicatedSemanticAction::squash(int32_t anOperand) {
  setReady(anOperand, false);
  if (thePredicate) {
    squashDependants();
  }
}

void PredicatedSemanticAction::predicate_off(int) {
  if ( !cancelled() && thePredicate ) {
    DBG_( Verb, ( << *this << " predicated off. ") );
    reschedule();
    thePredicate = false;
    squashDependants();
  }
}

void PredicatedSemanticAction::predicate_on(int) {
  if (!cancelled() && ! thePredicate ) {
    DBG_( Verb, ( << *this << " predicated on. ") );
    reschedule();
    thePredicate = true;
    squashDependants();
  }
}

} //nv9Decoder
