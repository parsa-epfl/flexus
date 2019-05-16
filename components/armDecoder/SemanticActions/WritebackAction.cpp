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
#include "RegisterValueExtractor.hpp"

#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct WritebackAction : public BaseSemanticAction {
  eOperandCode theResult;
  eOperandCode theRd;
  bool the64, theSetflags, theSP;


  WritebackAction ( SemanticInstruction * anInstruction, eOperandCode aResult, eOperandCode anRd, bool a64, bool aSP, bool setflags = false )
    : BaseSemanticAction ( anInstruction, 1 )
    , theResult( aResult )
    , theRd( anRd )
    , the64 (a64)
    , theSetflags (setflags)
    , theSP (aSP)
  { }

  void squash(int32_t anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRd);
      core()->squashRegister( name );
      DBG_( VVerb, ( << *this << " squashing register rd=" << name) );
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {

    if (ready()) {
      DBG_(Dev, (<< "Writing " << theResult << " to " << theRd));

      register_value result = boost::apply_visitor( register_value_extractor(), theInstruction->operand( theResult ) );
      uint64_t res = boost::get<uint64_t>(result);

      mapped_reg name = theInstruction->operand< mapped_reg > (theRd);
      char r = the64 ? 'X' : 'W';
      if(!the64){
        res &= 0xFFFFFFFF;
        result = res;
        theInstruction->setOperand(theResult,res );
      }
      DBG_(Dev, (<< "Writing " << std::hex << result << std::dec << " to " << r << "-REG ->"<< name));
      core()->writeRegister( name, result, !the64 );
      DBG_( VVerb, ( << *this << " rd= " << name << " result=" << result ) );
      core()->bypass( name, result );
      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " WritebackAction to" << theRd;
  }
};

struct WriteccAction : public BaseSemanticAction {
  eOperandCode theCC;
  bool the64;

  WriteccAction ( SemanticInstruction * anInstruction, eOperandCode anCC, bool an64 )
    : BaseSemanticAction ( anInstruction, 1 )
    , theCC( anCC )
    , the64( an64 )
  { }

  void squash(int32_t anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theCC);
      core()->squashRegister( name );
      DBG_( VVerb, ( << *this << " squashing register cc=" << name) );
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {

    if (ready()) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theCC);
      uint64_t ccresult = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readPSTATE();
      uint64_t flags = boost::get<uint64_t>(boost::apply_visitor( register_value_extractor(), theInstruction->operand( kResultCC ) ));
      if(!the64){
        uint64_t result = boost::get<uint64_t>(boost::apply_visitor( register_value_extractor(), theInstruction->operand( kResult ) ));
        flags &= ~PSTATE_N;
        flags |= (result & ((uint64_t)1 << 31)) ? PSTATE_N : 0;
        flags |= !(result & 0xFFFFFFFF) ? PSTATE_Z : 0;
        flags |= ((result >> 32) == 1) ? PSTATE_C : 0;
      }
      ccresult &= ~(0xF << 28);
      ccresult |= flags;
      DBG_(Dev, (<< "Writing to CC: " << std::hex << ccresult << ", " << std::dec << *theInstruction));
      core()->writeRegister( name, ccresult, false );
      core()->bypass( name, ccresult );
      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " WriteCCAction to" << theCC;
  }
};

dependant_action writebackAction
( SemanticInstruction * anInstruction
  , eOperandCode aRegisterCode
  , eOperandCode aMappedRegisterCode
  , bool is64
  , bool aSP
  , bool setflags
) {
  WritebackAction * act;
    act = new(anInstruction->icb()) WritebackAction( anInstruction, aRegisterCode, aMappedRegisterCode, is64, aSP, setflags);
    return dependant_action( act, act->dependance() );
}

dependant_action writeccAction
( SemanticInstruction * anInstruction
  , eOperandCode aMappedRegisterCode
  , bool is64
) {
  WriteccAction * act;
    act = new(anInstruction->icb()) WriteccAction( anInstruction, aMappedRegisterCode, is64);
    return dependant_action( act, act->dependance() );
}

} //narmDecoder
