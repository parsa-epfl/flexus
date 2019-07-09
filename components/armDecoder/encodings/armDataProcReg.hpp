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

#ifndef FLEXUS_armDECODER_armDATAPROCREG_HPP_INCLUDED
#define FLEXUS_armDECODER_armDATAPROCREG_HPP_INCLUDED

#include "armSharedFunctions.hpp"
namespace narmDecoder {

// Logical (shifted register)
arminst LOGICAL(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Add/subtract (shifted register / extended register)
arminst ADDSUB_SHIFTED(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst ADDSUB_EXTENDED(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Data-processing (3 source)
arminst DP_1_SRC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst DP_2_SRC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst DP_3_SRC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Add/subtract (with carry)
arminst ADDSUB_CARRY(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Conditional compare (immediate / register)
arminst CCMP(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Conditional select
arminst CSEL(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Data-processing (1 source)
arminst RBIT(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst REV(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst CL(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Data-processing (2 sources)
arminst DIV(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst SHIFT(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
arminst CRC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace narmDecoder

#endif // FLEXUS_armDECODER_armDATAPROCREG_HPP_INCLUDED
