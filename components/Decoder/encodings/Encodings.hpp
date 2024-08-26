//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

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
disas_ldst_pair(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ld_lit(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst_excl(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
disas_ldst(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

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
disas_a64_insn(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_armENCODINGS_HPP_INCLUDED