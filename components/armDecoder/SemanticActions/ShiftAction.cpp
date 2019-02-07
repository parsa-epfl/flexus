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
#include <boost/lambda/bind.hpp>
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

struct ShiftRegisterAction : public BaseSemanticAction
{
  eOperandCode theRegisterCode;
  bool the64;
  std::unique_ptr<Operation> theShiftOperation;
  uint64_t theShiftAmount;

  ShiftRegisterAction( SemanticInstruction * anInstruction, eOperandCode aRegisterCode, std::unique_ptr<Operation> & aShiftOperation, uint64_t aShiftAmount, bool is64)
    : BaseSemanticAction( anInstruction, 1 )
    , theRegisterCode( aRegisterCode )
    , the64(is64)
    , theShiftOperation (std::move(aShiftOperation))
    , theShiftAmount (aShiftAmount)
  {}

  void doEvaluate()
  {
    SEMANTICS_DBG(*this);

    if (! signalled() ) {
      SEMANTICS_DBG("Signalling");

      Operand aValue = theInstruction->operand(theRegisterCode);

      aValue = theShiftOperation->operator ()({aValue, theShiftAmount});

      uint64_t val = boost::get<uint64_t>(aValue);

      if (!the64) {
        val &= 0xffffffff;
      }

      theInstruction->setOperand(theRegisterCode, val);
      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const
  {
    anOstream << theInstruction->identify() << " ShiftRegisterAction " << theRegisterCode;
  }
};

simple_action shiftRegisterAction ( SemanticInstruction * anInstruction, eOperandCode aRegisterCode,
                                   std::unique_ptr<Operation> & aShiftOp, uint64_t aShiftAmount, bool is64)
{
  return new(anInstruction->icb()) ShiftRegisterAction( anInstruction, aRegisterCode, aShiftOp, aShiftAmount, is64);
}



} //narmDecoder