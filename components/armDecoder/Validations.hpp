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

struct validateXRegister {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateXRegister( uint32_t aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  { }

  bool operator () ();
};


struct SysReg_access{

    SemanticInstruction * theInstruction;
    uint8_t theOpc0;
    uint8_t theOpc1;
    uint8_t theOpc2;
    uint8_t theCRn;
    uint8_t theCRm;
    uint8_t theIsRead;

    SysReg_access ( SemanticInstruction * anInstruction,
                    uint8_t opc0,
                    uint8_t opc1,
                    uint8_t opc2,
                    uint8_t crn,
                    uint8_t crm,
                    uint8_t is_read)
            : theInstruction(anInstruction)
            , theOpc0 (opc0)
            , theOpc1 (opc1)
            , theOpc2 (opc2)
            , theCRn (crn)
            , theCRm (crm)
            , theIsRead (is_read)
          {}

    bool operator () ();
};


struct checkSystemAccess {
  SemanticInstruction * theInstruction;
  uint8_t theOpc0;
  uint8_t theOpc1;
  uint8_t theOpc2;
  uint8_t theCRn;
  uint8_t theCRm;
  checkSystemAccess( SemanticInstruction * anInstruction,
                     uint8_t opc0,
                     uint8_t opc1,
                     uint8_t opc2,
                     uint8_t crn,
                     uint8_t crm)
    : theInstruction(anInstruction)
    , theOpc0 (opc0)
    , theOpc1 (opc1)
    , theOpc2 (opc2)
    , theCRn (crn)
    , theCRm (crm)
  {}

  bool operator () ();
};

struct validatePC {
  uint64_t theAddr;
  SemanticInstruction * theInstruction;

  validatePC( uint64_t anAddr, SemanticInstruction * anInstruction )
    : theAddr(anAddr)
    , theInstruction(anInstruction)
  { }

  bool operator () ();
};

struct validateVRegister {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateVRegister( uint32_t aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
    : theReg(aReg)
    , theOperandCode(anOperand)
    , theInstruction(anInstruction)
  { }

  bool operator () ();
};

struct validateMemory {
  eOperandCode theAddressCode;
  int32_t theAddrOffset;
  eOperandCode theValueCode;
  nuArchARM::eSize theSize;
  SemanticInstruction * theInstruction;

  validateMemory( eOperandCode anAddressCode, eOperandCode aValueCode, nuArchARM::eSize aSize, SemanticInstruction * anInstruction )
    : theAddressCode(anAddressCode)
    , theAddrOffset(0)
    , theValueCode(aValueCode)
    , theSize(aSize)
    , theInstruction(anInstruction)
  {}

  validateMemory( eOperandCode anAddressCode, int32_t anAddrOffset, eOperandCode aValueCode, nuArchARM::eSize aSize, SemanticInstruction * anInstruction )
    : theAddressCode(anAddressCode)
    , theAddrOffset(anAddrOffset)
    , theValueCode(aValueCode)
    , theSize(aSize)
    , theInstruction(anInstruction)
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

struct AArch64ExclusiveMonitorsPass {
  SemanticInstruction * theInstruction;
  unsigned int theSize;
  unsigned int theCPU;
  AArch64ExclusiveMonitorsPass( SemanticInstruction * anInstruction, unsigned int size, unsigned int cpu)
    : theInstruction(anInstruction)
    , theSize(size)
    , theCPU (cpu)
  {}
  bool operator () ();
};

} //narmDecoder

#endif //FLEXUS_armDECODER_VALIDATIONS_HPP_INCLUDED
