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

struct UpdateAddressAction : public BaseSemanticAction {

  eOperandCode theAddressCode;

  UpdateAddressAction ( SemanticInstruction * anInstruction, eOperandCode anAddressCode )
    : BaseSemanticAction ( anInstruction, 2 )
    , theAddressCode (anAddressCode)
  { }

  void squash(int32_t anOperand) {
    if (! cancelled() ) {
      DBG_( Verb, ( << *this << " Squashing vaddr." ) );
      core()->resolveVAddr( boost::intrusive_ptr<Instruction>(theInstruction), kUnresolved, 0x80);
    }
    BaseSemanticAction::squash(anOperand);
  }

  void satisfy(int32_t anOperand) {
    //updateAddress as soon as dependence is satisfied
    BaseSemanticAction::satisfy(anOperand);
    updateAddress();
  }

  void doEvaluate() {
    //Address is now updated when satisfied.
  }

  void updateAddress() {
    if (ready()) {
      uint64_t addr = theInstruction->operand< uint64_t > (theAddressCode);
      if (theInstruction->hasOperand( kUopAddressOffset ) ) {
        uint64_t offset = theInstruction->operand< uint64_t > (kUopAddressOffset);
        addr += offset;
      }
      int32_t asi = theInstruction->operand< uint64_t > (kOperand3);
      VirtualMemoryAddress vaddr(addr);
      DBG_( Verb, ( << *this << " updating vaddr=" << vaddr << " asi=" << std::hex << asi << std::dec) );
      core()->resolveVAddr( boost::intrusive_ptr<Instruction>(theInstruction), vaddr, asi);
      theInstruction->setASI(asi);

      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " UpdateAddressAction";
  }
};

multiply_dependant_action updateAddressAction
( SemanticInstruction * anInstruction ) {
  UpdateAddressAction * act(new(anInstruction->icb()) UpdateAddressAction( anInstruction, kAddress ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );

  return multiply_dependant_action( act, dependances );
}

multiply_dependant_action updateCASAddressAction
( SemanticInstruction * anInstruction ) {
  UpdateAddressAction * act(new(anInstruction->icb()) UpdateAddressAction( anInstruction, kOperand1 ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );

  return multiply_dependant_action( act, dependances );
}

} //nv9Decoder
