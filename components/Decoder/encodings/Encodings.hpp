
/*
 * The instruction disassembly implemented here matches
 * the instruction encoding classifications documented in
 * the 00bet3.1 release of the A64 ISA XML for ARMv8.2.
 * classification names and decode diagrams here should generally
 * match up with those in the manual.
 */

#ifndef FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED
#define FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

//<<--Data Processing -- Immediate
archinst
disas_add_sub_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_pc_rel_adr(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_logic_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_movw_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_bitfield(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_extract(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

//<<--Branches, Exception Generating and System instructions
archinst
disas_uncond_b_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_exc(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_system(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_cond_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_test_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_comp_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_uncond_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_b_exc_sys(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

//<<--Loads and Stores
archinst
disas_ldst_single_struct(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_multiple_struct(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_reg_imm9(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_reg_roffset(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_reg_unsigned_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_pair(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop, bool& aLastUop);
archinst
disas_ld_lit(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_excl(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop, bool& aLastUop);

//<<--Data Processing -- Register
archinst
disas_data_proc_2src(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_1src(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_cond_select(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_cc(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_adc_sbc(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_3src(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_add_sub_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_add_sub_ext_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_logic_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

//<<--Data Processing -- FP/SIMD
archinst
disas_fp_int_conv(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_ccomp(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_csel(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_fixed_conv(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_1src(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_compare(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_2src(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_3src(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_fp_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_fp(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_simd(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_data_proc_simd_fp(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

/* C3.1 A64 instruction index by encoding */
archinst
disas_a64_insn(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop, bool& aLastUop);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED