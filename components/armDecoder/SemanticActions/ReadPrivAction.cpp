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

#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct ReadPrivAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode;
  uint8_t theOp0, theOp1, theOp2, theCRn, theCRm;
  bool thehasCP;

  ReadPrivAction( SemanticInstruction * anInstruction,
                  eOperandCode anOperandCode,
                  uint8_t op0,
                  uint8_t op1,
                  uint8_t crn,
                  uint8_t crm,
                  uint8_t op2,
                  bool hasCP )
    : PredicatedSemanticAction( anInstruction, 1, true )
    , theOperandCode( anOperandCode )
    , theOp0( op0 )
    , theOp1( op1 )
    , theOp2( op2 )
    , theCRn( crn )
    , theCRm( crm )
    , thehasCP (hasCP)
  {
    setReady( 0, true );
  }

  void doEvaluate() {

      // further access checks
      SysRegInfo* ri = theInstruction->core()->getSysRegInfo(theOp0, theOp1, theOp2, theCRn, theCRm, thehasCP);
      if (ri->accessfn(theInstruction->core()) == kACCESS_OK){
          Operand val = ri->readfn(theInstruction->core());
            theInstruction->setOperand(theOperandCode, val);
      }
    satisfyDependants();
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " Read SYS store in " << theOperandCode;
  }

};

predicated_action readPrivAction
( SemanticInstruction * anInstruction, eOperandCode anOperandCode,
  uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm, bool hasCP
) {
  ReadPrivAction * act(new(anInstruction->icb()) ReadPrivAction( anInstruction, anOperandCode, op0, op1, op2, crn, crm, hasCP) );

  return predicated_action( act, act->predicate() );
}



} //narmDecoder
