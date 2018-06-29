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


#ifndef FLEXUS_armDECODER_VALIDATIONS_HPP_INCLUDED
#define FLEXUS_armDECODER_VALIDATIONS_HPP_INCLUDED

#include <components/CommonQEMU/Slices/MemOp.hpp>
#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"

namespace narmDecoder {

struct SemanticInstruction;

struct overrideRegister {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  overrideRegister( uint32_t aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  { }

  void operator () ();
};

struct overrideFloatSingle {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  overrideFloatSingle( uint32_t aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  { }

  void operator () ();
};

struct overrideFloatDouble {
  uint32_t theReg;
  eOperandCode theOperandCodeHi;
  eOperandCode theOperandCodeLo;
  SemanticInstruction * theInstruction;

  overrideFloatDouble( uint32_t aReg, eOperandCode anOperandHi, eOperandCode anOperandLo, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCodeHi(anOperandHi)
    , theOperandCodeLo(anOperandLo)
    , theInstruction(anInstruction)
  { }

  void operator () ();
};

struct validateRegister {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateRegister( uint32_t aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  { }

  bool operator () ();
};

struct validateFRegister {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateFRegister( uint32_t aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  { }

  bool operator () ();
};

struct validateCC {
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateCC( eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theOperandCode(anOperand)
    , theInstruction(anInstruction)
  {}

  bool operator () ();
};

struct validateFCC {
  uint32_t theFCC;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateFCC( uint32_t anFCC, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theFCC(anFCC)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  {}

  bool operator () ();
};

struct validateMemory {
  eOperandCode theAddressCode;
  eOperandCode theASICode;
  int32_t theAddrOffset;
  eOperandCode theValueCode;
  nuArchARM::eSize theSize;
  SemanticInstruction * theInstruction;

  validateMemory( eOperandCode anAddressCode, eOperandCode anASICode, eOperandCode aValueCode, nuArchARM::eSize aSize, SemanticInstruction * anInstruction )
    : theAddressCode(anAddressCode)
    , theASICode(anASICode)
    , theAddrOffset(0)
    , theValueCode(aValueCode)
    , theSize(aSize)
    , theInstruction(anInstruction)
  {}

  validateMemory( eOperandCode anAddressCode, int32_t anAddrOffset, eOperandCode anASICode, eOperandCode aValueCode, nuArchARM::eSize aSize, SemanticInstruction * anInstruction )
    : theAddressCode(anAddressCode)
    , theASICode(anASICode)
    , theAddrOffset(anAddrOffset)
    , theValueCode(aValueCode)
    , theSize(aSize)
    , theInstruction(anInstruction)
  {}

  bool operator () ();
};

struct validatePR {
  SemanticInstruction * theInstruction;
  uint32_t thePR;
  eOperandCode theOperandCode;
  validatePR( SemanticInstruction * anInstruction, uint32_t aPR, eOperandCode anOperandCode)
    : theInstruction(anInstruction)
    , thePR(aPR)
    , theOperandCode(anOperandCode)
  {}
  bool operator () ();

};

struct validateFPSR {
  SemanticInstruction * theInstruction;
  validateFPSR( SemanticInstruction * anInstruction)
    : theInstruction(anInstruction)
  {}
  bool operator () ();
};

struct validateFPCR {
  SemanticInstruction * theInstruction;
  validateFPCR( SemanticInstruction * anInstruction)
    : theInstruction(anInstruction)
  {}
  bool operator () ();
};

struct validateTPR {
  SemanticInstruction * theInstruction;
  uint32_t thePR;
  eOperandCode theOperandCode;
  eOperandCode theTLOperandCode;
  validateTPR( SemanticInstruction * anInstruction, uint32_t aPR, eOperandCode anOperandCode, eOperandCode aTLOperandCode)
    : theInstruction(anInstruction)
    , thePR(aPR)
    , theOperandCode(anOperandCode)
    , theTLOperandCode(aTLOperandCode)
  {}
  bool operator () ();

};

struct validateSaveRestore {
  SemanticInstruction * theInstruction;
  validateSaveRestore( SemanticInstruction * anInstruction)
    : theInstruction(anInstruction)
  {}
  bool operator () ();
};

struct validateLegalReturn {
  SemanticInstruction * theInstruction;
  validateLegalReturn( SemanticInstruction * anInstruction)
    : theInstruction(anInstruction)
  {}
  bool operator () ();
};

} //narmDecoder

#endif //FLEXUS_armDECODER_VALIDATIONS_HPP_INCLUDED
