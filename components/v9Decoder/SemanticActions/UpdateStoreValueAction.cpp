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
using nuArch::Instruction;

struct UpdateStoreValueAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode;

  UpdateStoreValueAction ( SemanticInstruction * anInstruction, eOperandCode anOperandCode )
    : PredicatedSemanticAction ( anInstruction, 1, true )
    , theOperandCode(anOperandCode)
  { }

  void predicate_off(int) {
    if ( !cancelled() && thePredicate ) {
      DBG_( Verb, ( << *this << " predicated off. ") );
      DBG_Assert(core());
      DBG_( Verb, ( << *this << " anulling store" ) );
      core()->annulStoreValue( boost::intrusive_ptr<Instruction>(theInstruction));
      thePredicate = false;
      satisfyDependants();
    }
  }

  void predicate_on(int) {
    if (!cancelled() && ! thePredicate ) {
      DBG_( Verb, ( << *this << " predicated on. ") );
      reschedule();
      thePredicate = true;
      squashDependants();
    }
  }

  void doEvaluate() {
    //Should ensure that we got execution units here
    if (! cancelled() ) {
      if ( thePredicate && ready() ) {
        uint64_t value = theInstruction->operand< uint64_t > (theOperandCode);
        DBG_( Verb, ( << *this << " updating store value=" << value) );
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
( SemanticInstruction * anInstruction ) {
  UpdateStoreValueAction * act(new(anInstruction->icb()) UpdateStoreValueAction( anInstruction, kResult ) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}

predicated_dependant_action updateRMWValueAction
( SemanticInstruction * anInstruction ) {
  UpdateStoreValueAction * act(new(anInstruction->icb()) UpdateStoreValueAction( anInstruction, kOperand4 ) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}

struct UpdateCASValueAction : public BaseSemanticAction {

  UpdateCASValueAction ( SemanticInstruction * anInstruction )
    : BaseSemanticAction ( anInstruction, 3 )
  { }

  void doEvaluate() {
    if (ready()) {
      uint64_t store_value = theInstruction->operand< uint64_t > (kOperand4);
      uint64_t cmp_value = theInstruction->operand< uint64_t > (kOperand2);
      DBG_( Verb, ( << *this << " updating CAS write=" << store_value << " cmp=" << cmp_value) );
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
      uint64_t value = (theInstruction->operand< uint64_t > (kResult) << 32) | (theInstruction->operand< uint64_t > (kResult1) & 0xFFFFFFFFULL);
      DBG_( Verb, ( << *this << " updating store value=" << value) );
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

} //nv9Decoder
