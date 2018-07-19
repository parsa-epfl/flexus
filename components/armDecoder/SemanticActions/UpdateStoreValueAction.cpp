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
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;
using nuArchARM::Instruction;

struct UpdateStoreValueAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode,theOperandCode1,theOperandCode2;
  bool isPair;

  UpdateStoreValueAction ( SemanticInstruction * anInstruction, eOperandCode anOperandCode )
    : PredicatedSemanticAction ( anInstruction, 1, true )
    , theOperandCode(anOperandCode)
  { }

  void predicate_off(int) {
    if ( !cancelled() && thePredicate ) {
      DBG_( Tmp, ( << *this << " predicated off. ") );
      DBG_Assert(core());
      DBG_( Tmp, ( << *this << " anulling store" ) );
      core()->annulStoreValue( boost::intrusive_ptr<Instruction>(theInstruction));
      thePredicate = false;
      satisfyDependants();
    }
  }

  void predicate_on(int) {
    if (!cancelled() && ! thePredicate ) {
      DBG_( Tmp, ( << *this << " predicated on. ") );
      reschedule();
      thePredicate = true;
      squashDependants();
    }
  }

  void doEvaluate() {
    //Should ensure that we got execution units here
    if (! cancelled() ) {
      if ( thePredicate && ready() ) {
            bits value = theInstruction->operand< bits > (theOperandCode);
            DBG_( Tmp, ( << *this << " updating store value=" << value) );
            core()->updateStoreValue( boost::intrusive_ptr<Instruction>(theInstruction), value);
            satisfyDependants();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " UpdateStoreValue";
  }
};

predicated_dependant_action updateStoreValueAction
( SemanticInstruction * anInstruction , eOperandCode data) {
  UpdateStoreValueAction * act(new(anInstruction->icb()) UpdateStoreValueAction( anInstruction, data ) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}

predicated_dependant_action updateRMWValueAction
( SemanticInstruction * anInstruction ) {
  UpdateStoreValueAction * act(new(anInstruction->icb()) UpdateStoreValueAction( anInstruction, kOperand4 ) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}

struct UpdateCASValueAction : public BaseSemanticAction {

  bool thePair;

  UpdateCASValueAction ( SemanticInstruction * anInstruction )
    : BaseSemanticAction ( anInstruction, 3 )
  { }

  UpdateCASValueAction ( SemanticInstruction * anInstruction, bool aPair )
    : BaseSemanticAction ( anInstruction, 5 )
    , thePair(aPair)
  { }

  void doEvaluate() {
    if (ready()) {
            bits store_value = theInstruction->operand< bits > (kOperand1);
            bits cmp_value = theInstruction->operand< bits > (kOperand2);
            DBG_( Tmp, ( << *this << " updating CAS write=" << store_value << " cmp=" << cmp_value) );
            core()->updateCASValue( boost::intrusive_ptr<Instruction>(theInstruction), store_value, cmp_value);
            satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " UpdateCASValue";
  }
};




multiply_dependant_action updateCASValueAction
( SemanticInstruction * anInstruction ) {
  UpdateCASValueAction  * act(new(anInstruction->icb()) UpdateCASValueAction( anInstruction ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );
  dependances.push_back( act->dependance(2) );

  return multiply_dependant_action( act, dependances );
}

struct UpdateSTDValueAction : public BaseSemanticAction {

  UpdateSTDValueAction ( SemanticInstruction * anInstruction )
    : BaseSemanticAction ( anInstruction, 2 )
  { }

  void doEvaluate() {
    if (ready()) {
      bits value = (theInstruction->operand< bits > (kResult) << 32) | (theInstruction->operand< bits > (kResult1) & bits(0xFFFFFFFFULL));
      DBG_( Tmp, ( << *this << " updating store value=" << value) );
      core()->updateStoreValue( boost::intrusive_ptr<Instruction>(theInstruction), value);
      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " UpdateStoreValue";
  }
};

multiply_dependant_action updateSTDValueAction
( SemanticInstruction * anInstruction ) {
  UpdateSTDValueAction * act(new(anInstruction->icb()) UpdateSTDValueAction( anInstruction ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );

  return multiply_dependant_action( act, dependances );
}

} //narmDecoder
