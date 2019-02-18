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
#include "PredicatedSemanticAction.hpp"
#include "RegisterValueExtractor.hpp"
#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct ExtractAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode1, theOperandCode2, theOperandCode3;
  bool the64;

  ExtractAction ( SemanticInstruction * anInstruction
                  , eOperandCode anOperandCode1, eOperandCode anOperandCode2
                  , eOperandCode anOperandCode3, bool a64)
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theOperandCode1(anOperandCode1)
    , theOperandCode2 (anOperandCode2)
    , theOperandCode3 (anOperandCode3)
    , the64 (a64)
  {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        bits src =  boost::get<bits>(theInstruction->operand(theOperandCode1));
        bits src2 =  boost::get<bits>(theInstruction->operand(theOperandCode2));
        uint64_t imm =  (uint64_t)boost::get<bits>(theInstruction->operand(theOperandCode2));

        std::unique_ptr<Operation> op = operation(the64 ? kCONCAT64_ : kCONCAT32_);
        std::vector<Operand> operands = {src, src2, (uint64_t)the64};
        uint64_t res =  (uint64_t)boost::get<bits>(op->operator ()(operands));

        res >>= imm;

        theInstruction->setOperand(kResult, res);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( VVerb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BitFieldAction ";
  }
};

predicated_action extractAction
(SemanticInstruction * anInstruction
 , std::vector< std::list<InternalDependance> > & opDeps
 , eOperandCode anOperandCode1, eOperandCode anOperandCode2
 , eOperandCode anOperandCode3, bool a64
){
  ExtractAction * act(new(anInstruction->icb()) ExtractAction( anInstruction, anOperandCode1,
                                                                 anOperandCode2, anOperandCode3, a64) );

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back( act->dependance(i) );
  }

  return predicated_action( act, act->predicate() );

}

} //narmDecoder
