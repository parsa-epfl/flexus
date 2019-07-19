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
#include <boost/lambda/bind.hpp>
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

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

void connect(std::list<InternalDependance> const &dependances, simple_action &aSource) {
  BaseSemanticAction &act = *(aSource.action);
  std::for_each(dependances.begin(), dependances.end(),
                boost::lambda::bind(&BaseSemanticAction::addDependance, ll::var(act), ll::_1));
}

void BaseSemanticAction::addRef() {
  /*
  ++theSemanticActionImbalance;
  if (theSemanticActionImbalance > theMaxSemanticActionImbalance  + 50) {
    theMaxSemanticActionImbalance = theSemanticActionImbalance;
    DBG_(Iface, ( << "Max outstanding semantic actions: " <<
  theSemanticActionImbalance) );
  }
  */
  boost::intrusive_ptr_add_ref(theInstruction);
}

void BaseSemanticAction::releaseRef() {
  //--theSemanticActionImbalance;
  boost::intrusive_ptr_release(theInstruction);
}

void BaseSemanticAction::Dep::satisfy(int32_t anArg) {
  SEMANTICS_DBG(theAction.theInstruction->identify() << " " << theAction << " " << anArg);
  theAction.satisfy(anArg);
}
void BaseSemanticAction::Dep::squash(int32_t anArg) {
  SEMANTICS_DBG(theAction.theInstruction->identify() << " " << theAction);
  theAction.squash(anArg);
}

void BaseSemanticAction::satisfyDependants() {
  if (!cancelled() && !signalled()) {
    for (int32_t i = 0; i < theEndOfDependances; ++i) {
      theDependances[i].satisfy();
    }
    theSignalled = true;
    theSquashed = false;
  } else {
    SEMANTICS_DBG(theInstruction << "NOTE: Dependants were canceled!");
  }
}

void BaseSemanticAction::satisfy(int32_t anArg) {
  if (!cancelled()) {
    bool was_ready = ready();
    setReady(anArg, true);
    theSquashed = false;
    if (!was_ready && ready() && core()) {
      setReady(anArg, true);
      reschedule();
    }
  } else {
    SEMANTICS_DBG(theInstruction << "NOTE: Action were canceled!");
  }
}

void BaseSemanticAction::squash(int32_t anOperand) {
  SEMANTICS_DBG(*theInstruction << *this);
  setReady(anOperand, false);
  squashDependants();
}

void BaseSemanticAction::squashDependants() {
  if (!theSquashed) {
    if (core()) {

      SEMANTICS_DBG(theInstruction);
      for (int32_t i = 0; i < theEndOfDependances; ++i) {
        theDependances[i].squash();
      }
    }
    theSignalled = false;
    theSquashed = true;
  }
}

void BaseSemanticAction::evaluate() {
  theScheduled = false;
  if (!cancelled()) {
    doEvaluate();
  }
}

void BaseSemanticAction::addDependance(InternalDependance const &aDependance) {
  DBG_Assert(theEndOfDependances < 4);
  theDependances[theEndOfDependances] = aDependance;
  ++theEndOfDependances;
}

void BaseSemanticAction::reschedule() {
  if (!theScheduled && core()) {
    DBG_(Iface, (<< *this << " rescheduled"));
    theScheduled = true;
    core()->reschedule(this);
  }
}

void connectDependance(InternalDependance const &aDependant, simple_action &aSource) {
  aSource.action->addDependance(aDependant);
}

uArchARM *BaseSemanticAction::core() {
  return theInstruction->core();
}

PredicatedSemanticAction::PredicatedSemanticAction(SemanticInstruction *anInstruction,
                                                   int32_t aNumArgs, bool anInitialPredicate)
    : BaseSemanticAction(anInstruction, aNumArgs), thePredicate(anInitialPredicate),
      thePredicateTarget(*this) {
}

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
  if (!cancelled() && thePredicate) {
    SEMANTICS_DBG(*theInstruction << *this);
    reschedule();
    thePredicate = false;
    squashDependants();
  }
}

void PredicatedSemanticAction::predicate_on(int) {
  if (!cancelled() && !thePredicate) {
    SEMANTICS_DBG(*theInstruction << *this);
    reschedule();
    thePredicate = true;
    squashDependants();
  }
}

} // namespace narmDecoder
