// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


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

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;
using nuArchARM::Instruction;

struct UpdateAddressAction : public BaseSemanticAction {

  eOperandCode theAddressCode, theAddressCode2;
  bool thePair, theVirtual;

  UpdateAddressAction ( SemanticInstruction * anInstruction, eOperandCode anAddressCode, bool isVirtual )
    : BaseSemanticAction ( anInstruction, 2 )
    , theAddressCode (anAddressCode)
    , theVirtual (isVirtual)
  { }


  void squash(int32_t anOperand) {
    if (! cancelled() ) {
      DBG_( VVerb, ( << *this << " Squashing vaddr." ) );
      core()->resolveVAddr( boost::intrusive_ptr<Instruction>(theInstruction), kUnresolved/*, 0x80*/);
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
        DBG_(Dev, (<< "Executing " << *this ));

        if (theVirtual){
          DBG_Assert(theInstruction->hasOperand( theAddressCode ) );

          uint64_t addr = theInstruction->operand< uint64_t > (theAddressCode);
          if (theInstruction->hasOperand( kUopAddressOffset ) ) {
            uint64_t offset = theInstruction->operand< uint64_t > (kUopAddressOffset);
            DECODER_DBG("UpdateAddressAction: adding offset " << offset << " to address "<< addr);
            addr +=  offset;
            theInstruction->setOperand(theAddressCode, addr);
          } else if (theInstruction->hasOperand( kSopAddressOffset ) ) {
              int64_t offset = theInstruction->operand< int64_t > (kSopAddressOffset);
              DECODER_DBG("UpdateAddressAction: adding offset " << offset << " to address "<< addr);
              addr +=  offset;
              theInstruction->setOperand(theAddressCode, addr);
            }
          VirtualMemoryAddress vaddr(addr);
          core()->resolveVAddr( boost::intrusive_ptr<Instruction>(theInstruction), vaddr );
          SEMANTICS_DBG(*this << " updating vaddr = " << vaddr);

        } else {
            DBG_Assert(theInstruction->hasOperand( theAddressCode ) );
            core()->resolvePAddr( boost::intrusive_ptr<Instruction>(theInstruction));
        }
        satisfyDependants();
     }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " UpdateAddressAction";
  }
};



multiply_dependant_action updateVirtualAddressAction
( SemanticInstruction * anInstruction, uint32_t aNumDependent, eOperandCode aCode ) {
  UpdateAddressAction * act(new(anInstruction->icb()) UpdateAddressAction( anInstruction, aCode, true ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );
  return multiply_dependant_action( act, dependances );
}

multiply_dependant_action updatePhysicalAddressAction
( SemanticInstruction * anInstruction, uint32_t aNumDependent, eOperandCode aCode ) {
  UpdateAddressAction * act(new(anInstruction->icb()) UpdateAddressAction( anInstruction, aCode, false ) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );
  return multiply_dependant_action( act, dependances );
}

multiply_dependant_action updateCASAddressAction
( SemanticInstruction * anInstruction, eOperandCode aCode ) {
  UpdateAddressAction * act(new(anInstruction->icb()) UpdateAddressAction( anInstruction, aCode, true) );
  std::vector<InternalDependance> dependances;
  dependances.push_back( act->dependance(0) );
  dependances.push_back( act->dependance(1) );

  return multiply_dependant_action( act, dependances );
}


} //narmDecoder
