#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>

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

struct ReadRegisterAction : public BaseSemanticAction {
  eOperandCode theRegisterCode;
  eOperandCode theOperandCode;
  bool theConnected;

  ReadRegisterAction( SemanticInstruction * anInstruction, eOperandCode aRegisterCode, eOperandCode anOperandCode )
    : BaseSemanticAction( anInstruction, 1 )
    , theRegisterCode( aRegisterCode )
    , theOperandCode( anOperandCode )
    , theConnected(false)
  {}

  bool bypass(register_value aValue) {
    if ( cancelled() || theInstruction->isRetired() || theInstruction->isSquashed() ) {
      return true;
    }

    if ( !signalled() ) {
      theInstruction->setOperand(theOperandCode, aValue);
      DBG_( Verb, ( << *this << " bypassed " << theRegisterCode << " = " << aValue << " written to " << theOperandCode ) );
      satisfyDependants();
      setReady(0, true);
    }
    return false;
  }

  void doEvaluate() {
    if (! theConnected) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
      setReady( 0, core()->requestRegister( name, theInstruction->makeInstructionDependance(dependance()) ) == kReady );
      //theInstruction->addSquashEffect( disconnectRegister( theInstruction, theRegisterCode ) );
      //theInstruction->addRetirementEffect( disconnectRegister( theInstruction, theRegisterCode ) );
      core()->connectBypass( name, theInstruction, [this](auto x){ return this->bypass(x); });
      theConnected = true;
    }
    if (! signalled() ) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
      eResourceStatus status = core()->requestRegister( name );
      if (status == kReady) {
        mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
        DBG_( Verb, ( << *this << " read " << theRegisterCode << "(" << name << ")" ) );
        Operand aValue = core()->readRegister( name );
        theInstruction->setOperand(theOperandCode, aValue);
        DBG_( Verb, ( << *this << " read " << theRegisterCode << "(" << name << ") = " << aValue << " written to " << theOperandCode ) );
        satisfyDependants();
      } else {
        setReady( 0, false );
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " ReadRegister " << theRegisterCode << " store in " << theOperandCode;
  }
};

simple_action readRegisterAction
( SemanticInstruction * anInstruction
  , eOperandCode aRegisterCode
  , eOperandCode anOperandCode
) {
  return new(anInstruction->icb()) ReadRegisterAction( anInstruction, aRegisterCode, anOperandCode);
}

} //nv9Decoder
