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

struct LoadAction : public PredicatedSemanticAction {
  eSize theSize;
  bool theSignExtend;
  boost::optional<eOperandCode> theBypass;
  bool theLoadExtended;

  LoadAction( SemanticInstruction * anInstruction, eSize aSize, bool aSignExtend,  boost::optional<eOperandCode> aBypass, bool aLoadExtended)
    : PredicatedSemanticAction ( anInstruction, 1, true )
    , theSize(aSize)
    , theSignExtend(aSignExtend)
    , theBypass(aBypass)
    , theLoadExtended(aLoadExtended)
  { }

  void satisfy(int32_t anArg) {
    BaseSemanticAction::satisfy(anArg);
    if ( !cancelled() && ready() && thePredicate ) {
      doLoad();
    }
  }

  void predicate_on(int32_t anArg) {
    PredicatedSemanticAction::predicate_on(anArg);
    if (!cancelled() && ready() && thePredicate ) {
      doLoad();
    }
  }

  void doLoad() {
    uint64_t value;
    if (theLoadExtended) {
      value = core()->retrieveExtendedLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
    } else {
      value = core()->retrieveLoadValue( boost::intrusive_ptr<Instruction>(theInstruction) );
    }
    switch (theSize) {
      case kByte:
        value &= 0xFFULL;
        if (theSignExtend && (value & 0x80ULL)) {
          value |= 0xFFFFFFFFFFFFFF00ULL;
        }
        break;
      case kHalfWord:
        value &= 0xFFFFULL;
        if (theSignExtend && (value & 0x8000ULL)) {
          value |= 0xFFFFFFFFFFFF0000ULL;
        }
        break;
      case kWord:
        value &= 0xFFFFFFFFULL;
        if (theSignExtend && (value & 0x80000000ULL)) {
          value |= 0xFFFFFFFF00000000ULL;
        }
        break;
      case kDoubleWord:
        break;
    }

    theInstruction->setOperand(kResult, value);
    DBG_(Verb, ( << *this << " received load value=" << value));
    if (theBypass) {
      mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass);
      DBG_(Verb, ( << *this << " bypassing value=" << value << " to " << name));
      core()->bypass( name, value );
    }
    satisfyDependants();
  }

  void doEvaluate() {}

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " LoadAction";
  }
};

predicated_dependant_action loadAction
( SemanticInstruction * anInstruction, eSize aSize, bool aSignExtend, boost::optional<eOperandCode> aBypass ) {
  LoadAction * act(new(anInstruction->icb()) LoadAction( anInstruction, aSize, aSignExtend, aBypass, false ) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}
predicated_dependant_action casAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass ) {
  LoadAction * act(new(anInstruction->icb()) LoadAction( anInstruction, aSize, false, aBypass, true) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}
predicated_dependant_action rmwAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass ) {
  LoadAction * act(new(anInstruction->icb()) LoadAction( anInstruction, aSize, false, aBypass, true) );
  return predicated_dependant_action( act, act->dependance(), act->predicate() );
}

} //nv9Decoder
