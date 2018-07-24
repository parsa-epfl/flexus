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

/*
 * The instruction disassembly implemented here matches
 * the instruction encoding classifications documented in
 * the 00bet3.1 release of the A64 ISA XML for ARMv8.2.
 * classification names and decode diagrams here should generally
 * match up with those in the manual.
 */

#ifndef FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED
#define FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED

#include "armSharedFunctions.hpp"


namespace narmDecoder{

enum a64_shift_type {
    A64_SHIFT_TYPE_LSL = 0,
    A64_SHIFT_TYPE_LSR = 1,
    A64_SHIFT_TYPE_ASR = 2,
    A64_SHIFT_TYPE_ROR = 3
};

//<<--Data Processing -- Immediate
arminst disas_add_sub_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_pc_rel_adr(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_logic_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_movw_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_bitfield(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_extract(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);


//<<--Branches, Exception Generating and System instructions
arminst disas_uncond_b_reg( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_exc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_system(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_cond_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_test_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_comp_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_uncond_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_b_exc_sys(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);


//<<--Loads and Stores
arminst disas_ldst_single_struct(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_multiple_struct(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_reg_imm9(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_reg_roffset(armcode const & aFetchedOpcode,uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_reg_unsigned_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_pair(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ld_lit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst_excl(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_ldst(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

//<<--Data Processing -- Register
arminst disas_data_proc_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_cond_select(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_cc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_adc_sbc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_add_sub_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_add_sub_ext_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_logic_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);


//<<--Data Processing -- FP/SIMD
arminst disas_fp_int_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_ccomp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_csel(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_fixed_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_compare(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_fp_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo );
arminst disas_fp_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_simd(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst disas_data_proc_simd_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);

/* C3.1 A64 instruction index by encoding */
arminst disas_a64_insn( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop );

} // narmDecoder


#endif // FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED
