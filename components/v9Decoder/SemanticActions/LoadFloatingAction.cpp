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

struct LoadFloatingAction : public PredicatedSemanticAction {
  eSize theSize;
  boost::optional<eOperandCode> theBypass0;
  boost::optional<eOperandCode> theBypass1;

  LoadFloatingAction( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> const & aBypass0, boost::optional<eOperandCode> const & aBypass1  )
    : PredicatedSemanticAction ( anInstruction, 1, true )
    , theSize(aSize)
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
    if (ready() && thePredicate) {
      DBG_(Verb, ( << *this << " received floating load value" ));
      switch (theSize) {
        case kWord: {
          uint64_t value = core()->retrieveLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
          theInstruction->setOperand(kfResult0, value & 0xFFFFFFFFULL );
          if (theBypass0) {
            mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass0);
            DBG_(Verb, ( << *this << " bypassing value=" << (value & 0xFFFFFFFFULL) << " to " << name));
            core()->bypass( name, value & 0xFFFFFFFFULL);
          }

          break;
        }
        case kDoubleWord: {
          uint64_t value = core()->retrieveLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
          theInstruction->setOperand(kfResult1, value & 0xFFFFFFFFULL );
          theInstruction->setOperand(kfResult0, value >> 32 );
          if (theBypass1) {
            mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass1);
            DBG_(Verb, ( << *this << " bypassing value=" << (value & 0xFFFFFFFFULL) << " to " << name));
            core()->bypass( name, value & 0xFFFFFFFFULL);
          }
          if (theBypass0) {
            mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass0);
            DBG_(Verb, ( << *this << " bypassing value=" << (value >> 32) << " to " << name));
            core()->bypass( name, value >> 32);
          }

          break;
        }
        default:
          DBG_Assert(false);
      }

      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " LoadFloatingAction";
  }
};

predicated_dependant_action loadFloatingAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1) {
  LoadFloatingAction * act(new(anInstruction->icb()) LoadFloatingAction( anInstruction, aSize, aBypass0, aBypass1) );
  return predicated_dependant_action( act, act->dependance(), act->predicate()  );
}

} //nv9Decoder
