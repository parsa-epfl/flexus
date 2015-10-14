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

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;
using nuArch::Instruction;

struct UpdateFloatingStoreValueAction : public BaseSemanticAction {
  eSize theSize;

  UpdateFloatingStoreValueAction ( SemanticInstruction * anInstruction, eSize aSize )
    : BaseSemanticAction ( anInstruction, 2 )
    , theSize( aSize )
  { }

  void doEvaluate() {
    switch (theSize) {
      case kWord: {
        uint64_t value = theInstruction->operand< uint64_t > (kfResult0);
        DBG_( Verb, ( << *this << " updating store value=" << value) );
        core()->updateStoreValue( boost::intrusive_ptr<Instruction>(theInstruction), value);
        break;
      }
      case kDoubleWord: {
        uint64_t value = theInstruction->operand< uint64_t > (kfResult0);
        value <<= 32;
        value |= theInstruction->operand< uint64_t > (kfResult1);
        DBG_( Verb, ( << *this << " updating store value=" << value) );
        core()->updateStoreValue( boost::intrusive_ptr<Instruction>(theInstruction), value);
        break;
      }
      default:
        DBG_Assert(false);
    }
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " UpdateFloatingStoreValue";
  }
};

multiply_dependant_action updateFloatingStoreValueAction
( SemanticInstruction * anInstruction, eSize aSize ) {
  UpdateFloatingStoreValueAction * act(new(anInstruction->icb()) UpdateFloatingStoreValueAction( anInstruction, aSize ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );
  return multiply_dependant_action( act, dependances );
}

} //nv9Decoder
