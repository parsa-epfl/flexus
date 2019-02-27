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

#include "armEncodings.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {

/* C3.1 A64 instruction index by encoding */
arminst disas_a64_insn( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop )
{
    if (aFetchedOpcode.theOpcode == 1){ // instruction fetch page fault
        return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    }
    DECODER_DBG(   "#" << aSequenceNo << ": opcode = " << std::hex << aFetchedOpcode.theOpcode << std::dec);

    switch (extract32(aFetchedOpcode.theOpcode, 25, 4)) {
    case 0x0: case 0x1: case 0x2: case 0x3: /* UNALLOCATED */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x8: case 0x9: /* Data processing - immediate */
        return disas_data_proc_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0xa: case 0xb: /* Branch, exception generation and system insns */
         return disas_b_exc_sys(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x4:
    case 0x6:
    case 0xc:
    case 0xe:      /* Loads and stores */
        return disas_ldst(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x5:
    case 0xd:      /* Data processing - register */
        return disas_data_proc_reg(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x7:
    case 0xf:      /* Data processing - SIMD and floating point */
        return disas_data_proc_simd_fp(aFetchedOpcode,  aCPU, aSequenceNo);
    default:
        DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should be handled above */
        break;
    }
    DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should be handled above */
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

} // narmDecoder
