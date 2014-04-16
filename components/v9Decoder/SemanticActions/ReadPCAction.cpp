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

static const uint64_t kAM = 0x8;

struct ReadPCAction : public PredicatedSemanticAction {
  eOperandCode theResult;

  ReadPCAction( SemanticInstruction * anInstruction, eOperandCode aResult )
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theResult( aResult ) {
    setReady( 0, true );
  }

  void doEvaluate() {
    uint64_t pstate = theInstruction->core()->getPSTATE() ;
    if (pstate & kAM ) {
      //Need to mask upper 32 bits when AM is set
      theInstruction->setOperand(theResult, static_cast<uint64_t>(theInstruction->pc()) & 0xFFFFFFFFULL);

    } else {
      theInstruction->setOperand(theResult, static_cast<uint64_t>(theInstruction->pc()));
    }
    DBG_( Verb, ( << *this << " read PC") );
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " Read PC store in " << theResult;
  }

};

predicated_action readPCAction
( SemanticInstruction * anInstruction
) {
  ReadPCAction * act(new(anInstruction->icb()) ReadPCAction( anInstruction, kResult) );

  return predicated_action( act, act->predicate() );
}

} //nv9Decoder
