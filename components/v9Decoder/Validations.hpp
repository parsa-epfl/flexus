#ifndef FLEXUS_v9DECODER_VALIDATIONS_HPP_INCLUDED
#define FLEXUS_v9DECODER_VALIDATIONS_HPP_INCLUDED

#include <components/Common/Slices/MemOp.hpp>
#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"

namespace nv9Decoder {

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
  nuArch::eSize theSize;
  SemanticInstruction * theInstruction;

  validateMemory( eOperandCode anAddressCode, eOperandCode anASICode, eOperandCode aValueCode, nuArch::eSize aSize, SemanticInstruction * anInstruction )
    : theAddressCode(anAddressCode)
    , theASICode(anASICode)
    , theAddrOffset(0)
    , theValueCode(aValueCode)
    , theSize(aSize)
    , theInstruction(anInstruction)
  {}

  validateMemory( eOperandCode anAddressCode, int32_t anAddrOffset, eOperandCode anASICode, eOperandCode aValueCode, nuArch::eSize aSize, SemanticInstruction * anInstruction )
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

struct validateFPRS {
  SemanticInstruction * theInstruction;
  validateFPRS( SemanticInstruction * anInstruction)
    : theInstruction(anInstruction)
  {}
  bool operator () ();
};

struct validateFSR {
  SemanticInstruction * theInstruction;
  validateFSR( SemanticInstruction * anInstruction)
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

} //nv9Decoder

#endif //FLEXUS_v9DECODER_VALIDATIONS_HPP_INCLUDED
