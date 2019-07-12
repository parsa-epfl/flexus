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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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

#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"
#include <components/CommonQEMU/Slices/MemOp.hpp>

namespace narmDecoder {

struct SemanticInstruction;

struct validateXRegister {
  uint32_t theReg;
  eOperandCode theOperandCode;
  SemanticInstruction *theInstruction;
  bool the_64;

  validateXRegister(uint32_t aReg, eOperandCode anOperand, SemanticInstruction *anInstruction,
                    bool is_64)
      : theReg(aReg), theOperandCode(anOperand), theInstruction(anInstruction), the_64(is_64) {
  }

  bool operator()();
};

struct validatePC {
  SemanticInstruction *theInstruction;
  bool thePreValidation;

  validatePC(SemanticInstruction *anInstruction, bool prevalidation = false)
      : theInstruction(anInstruction), thePreValidation(prevalidation) {
  }

  bool operator()();
};

struct validateMemory {
  eOperandCode theAddressCode;
  eOperandCode theValueCode;
  nuArchARM::eSize theSize;
  SemanticInstruction *theInstruction;

  validateMemory(eOperandCode anAddressCode, eOperandCode aValueCode, nuArchARM::eSize aSize,
                 SemanticInstruction *anInstruction)
      : theAddressCode(anAddressCode), theValueCode(aValueCode), theSize(aSize),
        theInstruction(anInstruction) {
  }

  bool operator()();
};

} // namespace narmDecoder

#endif // FLEXUS_armDECODER_VALIDATIONS_HPP_INCLUDED
