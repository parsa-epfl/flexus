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

#ifndef FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED
#define FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED

#include "armSharedFunctions.hpp"

namespace narmDecoder {


// Load/store exclusive
arminst CASP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst CAS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STLR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDAR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

// Load register (literal)
arminst LDR_lit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDRF_lit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDRSW(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst PRFM(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

// Load/store pair (all forms)
arminst LDFP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STFP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDPSW(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

/* Load/store register (all forms) */
arminst LDR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDRF(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STRF(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

/* atomic memory operations */ // TODO
arminst LDADD(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDCLR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDEOR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDSET(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDSMAX(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDSMIN(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDUMAX(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDUMIN(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst SWP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

} // narmDecoder

#endif // FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED
