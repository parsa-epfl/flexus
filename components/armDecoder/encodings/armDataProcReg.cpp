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

#include "armDataProcReg.hpp"
#include "armSharedFunctions.hpp"

namespace narmDecoder {
using namespace nuArchARM;


// Logical (shifted register)
arminst AND(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst ANDS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst BIC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst BICS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst ORN(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst ORR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst EON(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst EOR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}


// Add/subtract (shifted register / extended register)
arminst ADD(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst ADDS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst SUB(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst SUBS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}


// Data-processing (3 source)


// Add/subtract (with carry)
arminst ADC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst ADCS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst SBC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst SBCS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}


// Conditional compare (immediate / register)
arminst CCMP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CCMN(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}

// Conditional select
arminst CSINC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CSNEG(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CSEL(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CSINV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}

// Data-processing (1 source)
arminst RBIT(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst REV16(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst REV32(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst REV64(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CLZ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CLS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}

// Data-processing (2 sources)
arminst UDIV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst SDIV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst LSLV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst LSRV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst ASRV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst RORV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}
arminst CRC32(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){}


}
