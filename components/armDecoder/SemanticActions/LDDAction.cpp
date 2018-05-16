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

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;
using nuArchARM::Instruction;

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
    DBG_(Tmp, (<<"After the stisfy of BaseSemanticAction"));
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

      DBG_(Tmp, ( << *this << " Quad LDD received load value=" << value << " value1=" << value1));
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

      DBG_(Tmp, ( << *this << " received load value=" << value));
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

} //narmDecoder
