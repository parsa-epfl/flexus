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

struct BitFieldAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode1, theOperandCode2;
  uint32_t theS, theR;
  uint32_t thewmask, thetmask;
  bool theExtend, the64;

  BitFieldAction ( SemanticInstruction * anInstruction
                  , eOperandCode anOperandCode1, eOperandCode anOperandCode2
                  , uint32_t imms, uint32_t immr
                  , uint32_t wmask, uint32_t tmask, bool anExtend, bool a64)
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theOperandCode1(anOperandCode1)
    , theOperandCode2 (anOperandCode2)
    , theS (imms)
    , theR (immr)
    , thewmask (wmask)
    , thetmask (tmask)
    , theExtend (anExtend)
    , the64 (a64)
  {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        bits src =  boost::get<bits>(theInstruction->operand(theOperandCode1));
        bits dst =  boost::get<bits>(theInstruction->operand(theOperandCode2));

        std::unique_ptr<Operation> ror = operation(kROR_);
        std::vector<Operand> operands = {src, theR, the64};
        bits res =  boost::get<bits>(ror->operator ()(operands));

        // perform bitfield move on low bits
        bits bot = (dst & ~thewmask) | (res & thewmask);
        // determine extension bits (sign, zero or dest register)
        bits top = theExtend ? src & (1 << theS) : dst;
        // combine extension bits and result bits
        theInstruction->setOperand(kResult, ((top & ~thetmask) | (bot & thetmask)));

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

predicated_action bitFieldAction
(SemanticInstruction * anInstruction
  , std::vector< std::list<InternalDependance> > & opDeps
  , eOperandCode anOperandCode1, eOperandCode anOperandCode2
  , uint32_t imms, uint32_t immr
  , uint32_t wmask, uint32_t tmask, bool anExtend, bool a64
 ){
  BitFieldAction * act(new(anInstruction->icb()) BitFieldAction( anInstruction, anOperandCode1,
                                                                 anOperandCode2, imms, immr, wmask, tmask, anExtend, a64) );

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back( act->dependance(i) );
    return predicated_action( act, act->predicate() );

  }

  return predicated_action( act, act->predicate() );

}

} //narmDecoder
