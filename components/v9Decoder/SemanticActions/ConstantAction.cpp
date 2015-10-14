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

struct ConstantAction : public PredicatedSemanticAction {
  uint64_t theConstant;
  eOperandCode theResult;
  boost::optional<eOperandCode> theBypass;

  ConstantAction( SemanticInstruction * anInstruction, uint64_t aConstant, eOperandCode aResult, boost::optional<eOperandCode> aBypass )
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theConstant(aConstant)
    , theResult( aResult )
    , theBypass( aBypass ) {
    setReady( 0, true );
  }

  void doEvaluate() {
    theInstruction->setOperand(theResult, static_cast<uint64_t>(theConstant));
    DBG_( Verb, ( << *this << " applied") );
    if (theBypass) {
      mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass);
      core()->bypass( name, theConstant );
    }
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " store constant " << theConstant << " in " << theResult;
  }

};

predicated_action constantAction
( SemanticInstruction * anInstruction
  , uint64_t aConstant
  , eOperandCode aResult
  , boost::optional<eOperandCode> aBypass
) {
  ConstantAction * act(new(anInstruction->icb()) ConstantAction( anInstruction, aConstant, aResult, aBypass) );
  return predicated_action( act, act->predicate() );
}

} //nv9Decoder
