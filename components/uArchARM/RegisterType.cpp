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

#include <iostream>

#include "RegisterType.hpp"
#include "uArchInterfaces.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArchARM {

std::ostream &operator<<(std::ostream &anOstream, eRegisterType aCode) {
  const char *map_tables[] = {"x-Registers", "v-Registers", "cc-Bits"};
  //  if (aCode >= kLastMapTableCode) {
  //    anOstream << "Invalid Map Table(" << static_cast<int>(aCode) << ")";
  //  } else {
  anOstream << map_tables[aCode];
  //  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, eResourceStatus aCode) {
  const char *statuses[] = {"Ready", "Unmapped", "NotReady"};
  if (aCode >= kLastStatus) {
    anOstream << "Invalid Status(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << statuses[aCode];
  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, condBits aBit) {
  const char *cc_bits[] = {"N", "Z", "V", "C"};
  if (aBit >= kLastcondBit) {
    anOstream << "Invalid condBit(" << static_cast<int>(aBit) << ")";
  } else {
    anOstream << cc_bits[aBit];
  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, fccVals aVal) {
  const char *fcc_vals[] = {"fccE", "fccL", "fccG", "fccU"};
  if (aVal >= kLastfccVal) {
    anOstream << "Invalid fccVal(" << static_cast<int>(aVal) << ")";
  } else {
    anOstream << fcc_vals[aVal];
  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, eInstructionCode aCode) {
  const char *insn_codes[] =
      // Special cases
      {"BlackBox", "NOP", "MAGIC",
       // ALU
       "ALU", "Reverse", "Mul", "Div", "RDPR", "WRPR",
       // FP
       "FP", "ALIGN",
       // Memory
       "Load", "LoadEX", "LoadAQ", "LDP", "LoadFP", "LDD", "LDALU", "Store", "StoreEX", "STP",
       "STRL", "StoreFP", "STD",
       // Atomics
       "CAS", "CASP",
       // Branches
       "BranchUnconditional", "BranchConditional", "BranchFPConditional", "CALL", "RETURN",
       // MEMBARs
       "MEMBARSync", "MEMBARStLd", "MEMBARStSt", "MEMBARLdSt", "MEMBARLdLd",
       // Unsupported Instructions
       "RDPRUnsupported", "WRPRUnsupported", "RETRYorDONE", "ExceptionUnsupported", "Exception",
       "SideEffectLoad", "SideEffectStore", "SideEffectAtomic", "DeviceAccess", "MMUAccess",
       "ITLBMiss", "CLREX"};
  if (aCode >= codeLastCode) {
    anOstream << "Invalid code(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << insn_codes[aCode];
  }
  return anOstream;
}

} // namespace nuArchARM
