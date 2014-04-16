#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

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

struct LDDAction : public PredicatedSemanticAction {
  boost::optional<eOperandCode> theBypass0;
  boost::optional<eOperandCode> theBypass1;

  LDDAction( SemanticInstruction * anInstruction, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1 )
    : PredicatedSemanticAction ( anInstruction, 1, true )
    , theBypass0(aBypass0)
    , theBypass1(aBypass1)
  { }

  void satisfy(int32_t anArg) {
    BaseSemanticAction::satisfy(anArg);
    if ( !cancelled() && ready() && thePredicate) {
      //Bypass
      doLoad();
    }
  }

  void predicate_on(int32_t anArg) {
    PredicatedSemanticAction::predicate_on(anArg);
    if (!cancelled() && ready() && thePredicate ) {
      doLoad();
    }
  }

  void doEvaluate() {}

  void doLoad() {
    int32_t asi = theInstruction->operand< uint64_t > (kOperand3);
    if (asi == 0x24 || asi == 0x34) {
      //Quad LDD
      uint64_t value = core()->retrieveLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
      uint64_t value1 = core()->retrieveExtendedLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
      theInstruction->setOperand(kResult, value );
      theInstruction->setOperand(kResult1, value1 );

      DBG_(Verb, ( << *this << " Quad LDD received load value=" << value << " value1=" << value1));
      if (theBypass0) {
        mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass0);
        core()->bypass( name, value );
      }
      if (theBypass1) {
        mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass1);
        core()->bypass( name, value1 );
      }
    } else {
      //Normal LDD
      uint64_t value = core()->retrieveLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
      theInstruction->setOperand(kResult, value >> 32);
      theInstruction->setOperand(kResult1, value & 0xFFFFFFFFULL );

      DBG_(Verb, ( << *this << " received load value=" << value));
      if (theBypass0) {
        mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass0);
        core()->bypass( name, value >> 32 );
      }
      if (theBypass1) {
        mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass1);
        core()->bypass( name, value & 0xFFFFFFFFULL );
      }
    }
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " LDDAction";
  }
};

predicated_dependant_action lddAction
( SemanticInstruction * anInstruction, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1  ) {
  LDDAction * act(new(anInstruction->icb()) LDDAction( anInstruction, aBypass0, aBypass1 ) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}

} //nv9Decoder
