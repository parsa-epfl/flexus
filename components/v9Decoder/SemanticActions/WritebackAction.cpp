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
#include "RegisterValueExtractor.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

struct WritebackAction : public BaseSemanticAction {
  eOperandCode theResult;
  eOperandCode theRd;

  WritebackAction ( SemanticInstruction * anInstruction, eOperandCode aResult, eOperandCode anRd )
    : BaseSemanticAction ( anInstruction, 1 )
    , theResult( aResult )
    , theRd( anRd )
  { }

  void squash(int32_t anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRd);
      core()->squashRegister( name );
      DBG_( Verb, ( << *this << " squashing register rd=" << name) );
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {
    if (ready()) {
      DBG_( Verb, ( << *this << " rd: " << theRd << " result: " << theResult) );
      mapped_reg name = theInstruction->operand< mapped_reg > (theRd);
      register_value result = boost::apply_visitor( register_value_extractor(), theInstruction->operand( theResult ) );
      core()->writeRegister( name, result );
      DBG_( Verb, ( << *this << " rd= " << name << " result=" << result ) );
      core()->bypass( name, result );
      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " WritebackAction";
  }
};

dependant_action writebackAction
( SemanticInstruction * anInstruction
  , eRegisterType aType
) {
  WritebackAction * act;
  if (aType == ccBits) {
    act = new(anInstruction->icb()) WritebackAction( anInstruction, kResultCC, kCCpd);
  } else {
    act = new(anInstruction->icb()) WritebackAction( anInstruction, kResult, kPD);
  }
  return dependant_action( act, act->dependance() );
}

dependant_action writebackRD1Action ( SemanticInstruction * anInstruction ) {
  WritebackAction * act = new(anInstruction->icb()) WritebackAction( anInstruction, kResult1, kPD1);
  return dependant_action( act, act->dependance() );
}

dependant_action writeXTRA
( SemanticInstruction * anInstruction ) {
  WritebackAction  * act = new(anInstruction->icb()) WritebackAction( anInstruction, kXTRAout, kXTRApd);
  return dependant_action( act, act->dependance() );
}

dependant_action floatingWritebackAction
( SemanticInstruction * anInstruction
  , int32_t anIndex
) {
  WritebackAction  * act(new(anInstruction->icb()) WritebackAction( anInstruction, eOperandCode(kfResult0 + anIndex), eOperandCode(kPFD0 + anIndex) ) );
  return dependant_action( act, act->dependance() );
}

} //nv9Decoder
