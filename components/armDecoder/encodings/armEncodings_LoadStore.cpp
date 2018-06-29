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
#include "armLoadStore.hpp"
#include "armUnallocated.hpp"
#include "armMagic.hpp"

namespace narmDecoder {



/* AdvSIMD load/store single structure
 *
 *  31  30  29           23 22 21 20       16 15 13 12  11  10 9    5 4    0
 * +---+---+---------------+-----+-----------+-----+---+------+------+------+
 * | 0 | Q | 0 0 1 1 0 1 0 | L R | 0 0 0 0 0 | opc | S | size |  Rn  |  Rt  |
 * +---+---+---------------+-----+-----------+-----+---+------+------+------+
 *
 * AdvSIMD load/store single structure (post-indexed)
 *
 *  31  30  29           23 22 21 20       16 15 13 12  11  10 9    5 4    0
 * +---+---+---------------+-----+-----------+-----+---+------+------+------+
 * | 0 | Q | 0 0 1 1 0 1 1 | L R |     Rm    | opc | S | size |  Rn  |  Rt  |
 * +---+---+---------------+-----+-----------+-----+---+------+------+------+
 *
 * Rt: first (or only) SIMD&FP register to be transferred
 * Rn: base address or SP
 * Rm (post-index only): post-index register (when !31) or size dependent #imm
 * index = encoded in Q:S:size dependent on size
 *
 * lane_size = encoded in R, opc
 * transfer width = encoded in opc, S, size
 */
arminst disas_ldst_single_struct(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    assert(false);
}

/* AdvSIMD load/store multiple structures
 *
 *  31  30  29           23 22  21         16 15    12 11  10 9    5 4    0
 * +---+---+---------------+---+-------------+--------+------+------+------+
 * | 0 | Q | 0 0 1 1 0 0 0 | L | 0 0 0 0 0 0 | opcode | size |  Rn  |  Rt  |
 * +---+---+---------------+---+-------------+--------+------+------+------+
 *
 * AdvSIMD load/store multiple structures (post-indexed)
 *
 *  31  30  29           23 22  21  20     16 15    12 11  10 9    5 4    0
 * +---+---+---------------+---+---+---------+--------+------+------+------+
 * | 0 | Q | 0 0 1 1 0 0 1 | L | 0 |   Rm    | opcode | size |  Rn  |  Rt  |
 * +---+---+---------------+---+---+---------+--------+------+------+------+
 *
 * Rt: first (or only) SIMD&FP register to be transferred
 * Rn: base address or SP
 * Rm (post-index only): post-index register (when !31) or size dependent #imm
 */
arminst disas_ldst_multiple_struct(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    assert(false);
}

/*
 * Load/store (immediate post-indexed)
 * Load/store (immediate pre-indexed)
 * Load/store (unscaled immediate)
 *
 * 31 30 29   27  26 25 24 23 22 21  20    12 11 10 9    5 4    0
 * +----+-------+---+-----+-----+---+--------+-----+------+------+
 * |size| 1 1 1 | V | 0 0 | opc | 0 |  imm9  | idx |  Rn  |  Rt  |
 * +----+-------+---+-----+-----+---+--------+-----+------+------+
 *
 * idx = 01 -> post-indexed, 11 pre-indexed, 00 unscaled imm. (no writeback)
         10 -> unprivileged
 * V = 0 -> non-vector
 * size: 00 -> 8 bit, 01 -> 16 bit, 10 -> 32 bit, 11 -> 64bit
 * opc: 00 -> store, 01 -> loadu, 10 -> loads 64, 11 -> loads 32
 */
arminst disas_ldst_reg_imm9(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                                int opc,
                                int size,
                                int rt,
                                bool is_vector)
{
    assert(false);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int imm9 = sextract32(aFetchedOpcode.theOpcode, 12, 9);
    int idx = extract32(aFetchedOpcode.theOpcode, 10, 2);
    bool is_signed = false;
    bool is_store = false;
    bool is_extended = false;
    bool is_unpriv = (idx == 2);
    bool iss_valid = !is_vector;
    bool post_index;
    bool writeback;

//    TCGv_i64 tcg_addr;

    if (is_vector) {
        assert(false);
        size |= (opc & 2) << 1;
        if (size > 4 || is_unpriv) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
        is_store = ((opc & 1) == 0);
//        if (!fp_access_check(s)) {
//            return;
//        }
    } else {
        if (size == 3 && opc == 2) {
            /* PRFM - prefetch */
            if (is_unpriv) {
                unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
                return;
            }
            return;
        }
        if (opc == 3 && size > 1) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
        is_store = (opc == 0);
        is_signed = extract32(opc, 1, 1);
        is_extended = (size < 3) && extract32(opc, 0, 1);
    }

    switch (idx) {
    case 0:
    case 2:
        post_index = false;
        writeback = false;
        break;
    case 1:
        post_index = true;
        writeback = true;
        break;
    case 3:
        post_index = false;
        writeback = true;
        break;
    }

    //if (rn == 31) {
        /*
         * The AArch64 architecture mandates that (if enabled via PSTATE
         * or SCTLR bits) there is a check that SP is 16-aligned on every
         * SP-relative load or store (with an exception generated if it is not).
         * In line with general QEMU practice regarding misaligned accesses
        */
        //gen_check_sp_alignment(s);

    //}

    //tcg_addr = read_cpu_reg_sp(s, rn, 1);
    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rn, rs_deps[0]);
    if (!post_index) {
        //tcg_gen_addi_i64(tcg_addr, tcg_addr, imm9);
        inst->setOperand(kUopAddressOffset, static_cast<uint64_t>(imm9));
        addAddressCompute(inst,rs_deps);
    }

    if (is_vector) {
        assert(false);
//        if (is_store) {
//            do_fp_st(s, rt, tcg_addr, size);
//        } else {
//            do_fp_ld(s, rt, tcg_addr, size);
//        }
    } else {
//        TCGv_i64 tcg_rt = cpu_reg(s, rt);
//        int memidx = is_unpriv ? get_a64_user_mem_index(s) : get_mem_index(s);
        int memidx = 0;
//        memidx = aFetchedCode.theMemidx;
        bool iss_sf = disas_ldst_compute_iss_sf(size, is_signed, opc);

        if (is_store) {
//            do_gpr_st_memidx(s, tcg_rt, tcg_addr, size, memidx,
//                             iss_valid, rt, iss_sf, false);
//            STR(inst, rt, rn, size);
        } else {
//            do_gpr_ld_memidx(s, tcg_rt, tcg_addr, size,
//                             is_signed, is_extended, memidx,
//                             iss_valid, rt, iss_sf, false);
            ldgpr(inst, rt, rn, size, false);
        }
    }

    if (writeback) {
//        TCGv_i64 tcg_rn = cpu_reg_sp(s, rn);
        if (post_index) {
            predicated_action add = addExecute(inst,operation(kADD_),rs_deps);
            addDestination(inst,rn,add);
            //tcg_gen_addi_i64(tcg_addr, tcg_addr, imm9);
        }
        predicated_action mov = addExecute(inst,operation(kMOV_),rs_deps);
        addDestination(inst,rn,mov);
//        tcg_gen_mov_i64(tcg_rn, tcg_addr);
    }
}


static void ext_and_shift_reg(uint_fast64_t rd, uint_fast64_t rs,
                              int option, unsigned int shift)
{
    int extsize = extract32(option, 0, 2);
    bool is_signed = extract32(option, 2, 1);

    if (is_signed) {
        switch (extsize) {
        case 0:
//            tcg_gen_ext8s_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 1:
//            tcg_gen_ext16s_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 2:
//            tcg_gen_ext32s_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 3:
//            tcg_gen_mov_i64(tcg_out, tcg_in);
            assert(false);
            break;
        }
    } else {
        switch (extsize) {
        case 0:
//            tcg_gen_ext8u_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 1:
//            tcg_gen_ext16u_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 2:
//            tcg_gen_ext32u_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 3:
//            tcg_gen_mov_i64(tcg_out, tcg_in);
            assert(false);
            break;
        }
    }

    if (shift) {
//        tcg_gen_shli_i64(tcg_out, tcg_out, shift);
        assert(false);
    }
}


/*
 * Load/store (register offset)
 *
 * 31 30 29   27  26 25 24 23 22 21  20  16 15 13 12 11 10 9  5 4  0
 * +----+-------+---+-----+-----+---+------+-----+--+-----+----+----+
 * |size| 1 1 1 | V | 0 0 | opc | 1 |  Rm  | opt | S| 1 0 | Rn | Rt |
 * +----+-------+---+-----+-----+---+------+-----+--+-----+----+----+
 *
 * For non-vector:
 *   size: 00-> byte, 01 -> 16 bit, 10 -> 32bit, 11 -> 64bit
 *   opc: 00 -> store, 01 -> loadu, 10 -> loads 64, 11 -> loads 32
 * For vector:
 *   size is opc<1>:size<1:0> so 100 -> 128 bit; 110 and 111 unallocated
 *   opc<0>: 0 -> store, 1 -> load
 * V: 1 -> vector/simd
 * opt: extend encoding (see DecodeRegExtend)
 * S: if S=1 then scale (essentially index by sizeof(size))
 * Rt: register to transfer into/out of
 * Rn: address register or SP for base
 * Rm: offset register or ZR for offset
 */
arminst disas_ldst_reg_roffset(armcode const & aFetchedOpcode,uint32_t  aCPU, int64_t aSequenceNo,
                                   int opc,
                                   int size,
                                   int rt,
                                   bool is_vector)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int shift = extract32(aFetchedOpcode.theOpcode, 12, 1);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int opt = extract32(aFetchedOpcode.theOpcode, 13, 3);
    bool is_signed = false;
    bool is_store = false;
    bool is_extended = false;

//    TCGv_i64 tcg_rm;
//    TCGv_i64 tcg_addr;

    if (extract32(opt, 1, 1) == 0) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (is_vector) {
        size |= (opc & 2) << 1;
        if (size > 4) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = !extract32(opc, 0, 1);
//        if (!fp_access_check(s)) {
//            return;
//        }
    } else {
        if (size == 3 && opc == 2) {
            /* PRFM - prefetch */
            return;
        }
        if (opc == 3 && size > 1) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = (opc == 0);
        is_signed = extract32(opc, 1, 1);
        is_extended = (size < 3) && extract32(opc, 0, 1);
    }

//    if (rn == 31) {
//        gen_check_sp_alignment(s);
//    }

//    tcg_addr = read_cpu_reg_sp(s, rn, 1);
//    tcg_rm = read_cpu_reg(s, rm, 1);
//    ext_and_shift_reg(tcg_rm, tcg_rm, opt, shift ? size : 0);
//    tcg_gen_add_i64(tcg_addr, tcg_addr, tcg_rm);

    std::vector<std::list<InternalDependance> > rs_deps(2);
//    addReadXRegister(inst, 1, rn, rs_deps[0]);
//    addReadXRegister(inst, 2, rm, rs_deps[1]);
//    ext_and_shift_reg(rm, rm, opt, shift ? size : 0);



    if (is_vector) {
        if (is_store) {
//            do_fp_st(s, rt, tcg_addr, size);
            stfpr(inst, rn, rt, size);
        } else {
//            do_fp_ld(s, rt, tcg_addr, size);
            ldfpr(inst, rn, rt, size);
        }
    } else {
//        TCGv_i64 tcg_rt = cpu_reg(s, rt);
        bool iss_sf = disas_ldst_compute_iss_sf(size, is_signed, opc);
        if (is_store) {
//            do_gpr_st(s, tcg_rt, tcg_addr, size,
//                      true, rt, iss_sf, false);
//            STR(inst, rn, rt, size);
        } else {
//            do_gpr_ld(s, tcg_rt, tcg_addr, size,
//                      is_signed, is_extended,
//                      true, rt, iss_sf, false);
            ldgpr(inst, rn, rt, size, is_signed);
        }
    }
    return inst;
}

/*
 * Load/store (unsigned immediate)
 *
 * 31 30 29   27  26 25 24 23 22 21        10 9     5
 * +----+-------+---+-----+-----+------------+-------+------+
 * |size| 1 1 1 | V | 0 1 | opc |   imm12    |  Rn   |  Rt  |
 * +----+-------+---+-----+-----+------------+-------+------+
 *
 * For non-vector:
 *   size: 00-> byte, 01 -> 16 bit, 10 -> 32bit, 11 -> 64bit
 *   opc: 00 -> store, 01 -> loadu, 10 -> loads 64, 11 -> loads 32
 * For vector:
 *   size is opc<1>:size<1:0> so 100 -> 128 bit; 110 and 111 unallocated
 *   opc<0>: 0 -> store, 1 -> load
 * Rn: base address register (inc SP)
 * Rt: target register
 */
arminst disas_ldst_reg_unsigned_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                                        int opc,
                                        int size,
                                        int rt,
                                        bool is_vector)
{
    SemanticInstruction * inst (new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,aFetchedOpcode.theBPState,aCPU,aSequenceNo));
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    unsigned int imm12 = extract32(aFetchedOpcode.theOpcode, 10, 12);
    unsigned int offset;

//    TCGv_i64 tcg_addr;

    uint64_t addr;
    bool is_store;
    bool is_signed = false;
    bool is_extended = false;
    int memidx = 0;

    if (is_vector) {
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register  VECTOR#1\033[0m"));

        size |= (opc & 2) << 1;
        if (size > 4) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = !extract32(opc, 0, 1);
//        if (!fp_access_check(s)) {
//            return;
//        }
    } else {
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register  non-VECTOR#1\033[0m"));

        if (size == 3 && opc == 2) {
            /* PRFM - prefetch */
        }
        if (opc == 3 && size > 1) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = (opc == 0);
        is_signed = extract32(opc, 1, 1);
        is_extended = (size < 3) && extract32(opc, 0, 1);
    }

//    if (rn == 31) {
//        gen_check_sp_alignment(s);
//    }

    //tcg_addr = read_cpu_reg_sp(s, rn, 1);
    //tcg_gen_addi_i64(tcg_addr, tcg_addr, offset);

    std::vector< std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst,1,rn,rs_deps[0]);
    offset = imm12 << size;

    inst->setOperand( kUopAddressOffset, offset );
    addAddressCompute(inst,rs_deps);



    if (is_vector) {

        if (is_store) {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-STF-VECTOR #2\033[0m"));

            stfpr(inst, rt, rn, size);
            //do_fp_st(s, rt, tcg_addr, size);
        } else {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-LDF-VECTOR #2\033[0m"));

            ldfpr(inst, rn, rt, size);
            //do_fp_ld(s, rt, tcg_addr, size);
        }
    } else {
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register nonVECTOR #2\033[0m"));

        //TCGv_i64 tcg_rt = cpu_reg(s, rt);

        bool iss_sf = disas_ldst_compute_iss_sf(eSize(1 << size), is_signed, opc);
        if (is_store) {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-ST nonVECTOR #2\033[0m"));

//            STR(inst, rt, size, memidx,
//                   true, rt, iss_sf, false);
            inst->setClass(clsStore, codeStoreFP);
            addReadXRegister(inst,2,rt, rs_deps[1]);
            addAddressCompute( inst, rs_deps ) ;
            inst->addSquashEffect( eraseLSQ(inst) );
//            inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
            inst->addRetirementEffect( retireMem(inst) );
//            inst->addPrevalidation( validateFPSR(inst) );
            inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false , kAccType_ORDERED) );
            inst->addCommitEffect( commitStore(inst) );
//            inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
//            inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

//            do_gpr_st(s, tcg_rt, tcg_addr, size,
//                      true, rt, iss_sf, false);
        } else {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-LD nonVECTOR #2\033[0m"));

//            ldgpr(inst, rt, size, is_signed, is_extended, memidx,
//                  true, rt, iss_sf, false);
            inst->setClass(clsLoad, codeLoad);
            addAddressCompute( inst, rs_deps ) ;
            inst->addSquashEffect( eraseLSQ(inst) );
//            inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
            inst->addRetirementEffect( retireMem(inst) );
            predicated_dependant_action load;
            load = loadAction( inst, eSize(1 << size), is_extended, kPD );
            inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_ORDERED ) );
            inst->addCommitEffect( accessMem(inst) );
            inst->addRetirementConstraint( loadMemoryConstraint(inst) );
            addDestination( inst, rt, load);
//            do_gpr_ld(s, tcg_rt, tcg_addr, size, is_signed, is_extended,
//                      true, rt, iss_sf, false);
        }
    }
    return inst;
}

/* Load/store register (all forms) */
arminst disas_ldst_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
    int size = extract32(aFetchedOpcode.theOpcode, 30, 2);

    switch (extract32(aFetchedOpcode.theOpcode, 24, 2)) {
    case 0:
        if (extract32(aFetchedOpcode.theOpcode, 21, 1) == 1 && extract32(aFetchedOpcode.theOpcode, 10, 2) == 2) {
            DBG_(Tmp,(<< "\033[1;31m disas_ldst_reg_roffset\033[0m"));
            return disas_ldst_reg_roffset(aFetchedOpcode, aCPU,aSequenceNo, opc, size, rt, is_vector);
        } else {
            /* Load/store register (unscaled immediate)
             * Load/store immediate pre/post-indexed
             * Load/store register unprivileged
             */
            DBG_(Tmp,(<< "\033[1;31m Load/store - register (unscaled immediate)"
                      " - immediate pre/post-indexed"
                      " - register unprivilege\033[0m"));

            return disas_ldst_reg_imm9( aFetchedOpcode, aCPU,aSequenceNo,opc, size, rt, is_vector);
        }
        break;
    case 1:
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register\033[0m"));
        return disas_ldst_reg_unsigned_imm(aFetchedOpcode, aCPU,aSequenceNo,opc, size, rt, is_vector);
        break;
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/*
 * LDNP (Load Pair - non-temporal hint)
 * LDP (Load Pair - non vector)
 * LDPSW (Load Pair Signed Word - non vector)
 * STNP (Store Pair - non-temporal hint)
 * STP (Store Pair - non vector)
 * LDNP (Load Pair of SIMD&FP - non-temporal hint)
 * LDP (Load Pair of SIMD&FP)
 * STNP (Store Pair of SIMD&FP - non-temporal hint)
 * STP (Store Pair of SIMD&FP)
 *
 *  31 30 29   27  26  25 24   23  22 21   15 14   10 9    5 4    0
 * +-----+-------+---+---+-------+---+-----------------------------+
 * | opc | 1 0 1 | V | 0 | index | L |  imm7 |  Rt2  |  Rn  | Rt   |
 * +-----+-------+---+---+-------+---+-------+-------+------+------+
 *
 * opc: LDP/STP/LDNP/STNP        00 -> 32 bit, 10 -> 64 bit
 *      LDPSW                    01
 *      LDP/STP/LDNP/STNP (SIMD) 00 -> 32 bit, 01 -> 64 bit, 10 -> 128 bit
 *   V: 0 -> GPR, 1 -> Vector
 * idx: 00 -> signed offset with non-temporal hint, 01 -> post-index,
 *      10 -> signed offset, 11 -> pre-index
 *   L: 0 -> Store 1 -> Load
 *
 * Rt, Rt2 = GPR or SIMD registers to be stored
 * Rn = general purpose register containing address
 * imm7 = signed offset (multiple of 4 or 8 depending on size)
 */
arminst disas_ldst_pair(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,aFetchedOpcode.theBPState,
                                                      aCPU,aSequenceNo));
    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint64_t offset = sextract64(aFetchedOpcode.theOpcode, 15, 7);
    int index = extract32(aFetchedOpcode.theOpcode, 23, 2);
    bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
    bool is_load = extract32(aFetchedOpcode.theOpcode, 22, 1);
    int opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    int memidx = 0;
    bool is_signed = false;
    bool postindex = false;
    bool wback = false;

    //TCGv_i64 tcg_addr; /* calculated address */
    int size;

    if (opc == 3) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (is_vector) {
        size = 2 + opc;
    } else {
        size = 2 + extract32(opc, 1, 1);
        is_signed = extract32(opc, 0, 1);
        if (!is_load && is_signed) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
    }

    switch (index) {
    case 1: /* post-index */
        postindex = true;
        wback = true;
        break;
    case 0:
        /* signed offset with "non-temporal" hint. Since we don't emulate
         * caches we don't care about hints to the cache system about
         * data access patterns, and handle this identically to plain
         * signed offset.
         */
        if (is_signed) {
            /* There is no non-temporal-hint version of LDPSW */
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        postindex = false;
        break;
    case 2: /* signed offset, rn not updated */
        postindex = false;
        break;
    case 3: /* pre-index */
        postindex = false;
        wback = true;
        break;
    }
//    if (is_vector && !fp_access_check(s)) {
//        return;
//    }
    offset <<= size;
//    if (rn == 31) {
//        gen_check_sp_alignment(s);
//    }
//    tcg_addr = read_cpu_reg_sp(s, rn, 1);

    std::vector<std::list<InternalDependance>> rs_deps(3);
    addReadXRegister(inst, 1, rn, rs_deps[0]);

    if (!postindex) {
//        //tcg_gen_addi_i64(tcg_addr, tcg_addr, offset);
        inst->setOperand( kUopAddressOffset, offset );
    }

    addAddressCompute(inst, rs_deps);
    satisfyAtDispatch(inst, rs_deps[0]);

    if (is_vector) {
        assert(false);
//        if (is_load) {
//            do_fp_ld(s, rt, tcg_addr, size);
//        } else {
//            do_fp_st(s, rt, tcg_addr, size);
//        }
//        //tcg_gen_addi_i64(tcg_addr, tcg_addr, 1 << size);
//        if (is_load) {
//            do_fp_ld(s, rt2, tcg_addr, size);
//        } else {
//            do_fp_st(s, rt2, tcg_addr, size);
//        }
    } else {
        //TCGv_i64 tcg_rt = cpu_reg(s, rt);
        //TCGv_i64 tcg_rt2 = cpu_reg(s, rt2);
//        addReadXRegister(inst, 1, rt, rs_deps[1]);
//        addReadXRegister(inst, 2, rt2, rs_deps[2]);
        if (is_load) {
            //TCGv_i64 tmp = tcg_temp_new_i64();
            /* Do not modify tcg_rt before recognizing any exception
             * from the second load.
             */
//            do_gpr_ld(s, tmp, tcg_addr, size, is_signed, false,
//                      false, 0, false, false);
//            //tcg_gen_addi_i64(tcg_addr, tcg_addr, 1 << size);
//            do_gpr_ld(s, tcg_rt2, tcg_addr, size, is_signed, false,
//                      false, 0, false, false);
//            //tcg_gen_mov_i64(tcg_rt, tmp);
//            tcg_temp_free_i64(tmp);

            ldgpr(inst, rt, rn, size,false);
            inst->setOperand( kUopAddressOffset, static_cast<uint64_t>(1 << size) );
            addAddressCompute(inst,rs_deps);
//            satisfyAtDispatch(inst, rs_deps[1]);

//            ldgpr(inst, rt2, rn, size, false);
//            predicated_action mov = addExecute(inst, operation(kMOV_),rs_deps);
//            addDestination(inst, rt, mov);
//            satisfyAtDispatch(inst, rs_deps[2]);


        } else {
//            do_gpr_st(s, tcg_rt, tcg_addr, size,
//                      false, 0, false, false);
//            //tcg_gen_addi_i64(tcg_addr, tcg_addr, 1 << size);
//            do_gpr_st(s, tcg_rt2, tcg_addr, size,
//                      false, 0, false, false);

//            STR(inst, rt, rn, size);
            DBG_(Tmp,(<< "\033[1;31m DECODER: Store General Reg \033[0m"));
            inst->setClass(clsStore, codeStore);

//            inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
            inst->addRetirementEffect( retireMem(inst) );
            inst->addSquashEffect( eraseLSQ(inst) );

            inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false, kAccType_ORDERED) );
            inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
            inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

            addReadRD( inst, rt );
//            inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, eSize(1<<size), inst ) );
            inst->addCommitEffect( commitStore(inst) );

//            inst->setOperand( kUopAddressOffset, static_cast<uint64_t>(1 << size) );
//            addAddressCompute(inst,rs_deps);

//            satisfyAtDispatch(inst, rs_deps[0]);

//            STR(inst, rt2, rn, size);
        }
    }

    if (wback) {
        predicated_action e;
        if (postindex) {
//            //tcg_gen_addi_i64(tcg_addr, tcg_addr, offset - (1 << size));
            inst->setOperand(kOperand2, offset - (1 << size));
            e = addExecute(inst, operation(kADD_),rs_deps);
        } else {
//            //tcg_gen_subi_i64(tcg_addr, tcg_addr, 1 << size);
            e = addExecute(inst, operation(kSUB_),rs_deps);
        }
//        //tcg_gen_mov_i64(cpu_reg_sp(s, rn), tcg_addr);
        addDestination(inst, rn, e);
    }
    return inst;
}

/*
 * Load register (literal)
 *
 *  31 30 29   27  26 25 24 23                5 4     0
 * +-----+-------+---+-----+-------------------+-------+
 * | opc | 0 1 1 | V | 0 0 |     imm19         |  Rt   |
 * +-----+-------+---+-----+-------------------+-------+
 *
 * V: 1 -> vector (simd/fp)
 * opc (non-vector): 00 -> 32 bit, 01 -> 64 bit,
 *                   10-> 32 bit signed, 11 -> prefetch
 * opc (vector): 00 -> 32 bit, 01 -> 64 bit, 10 -> 128 bit (11 unallocated)
 */
arminst disas_ld_lit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
//    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

//    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
//    int64_t imm = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
//    bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
//    int opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
//    bool is_signed = false;
//    int size = 2;
//    //TCGv_i64 tcg_rt, tcg_addr;

//    if (is_vector) {
//        if (opc == 3) {
//            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//            return;
//        }
//        size = 2 + opc;
////        if (!fp_access_check(s)) {
////            return;
////        }
////    } else {
//        if (opc == 3) {
//            /* PRFM (literal) : prefetch */
//            return;
//        }
//        size = 2 + extract32(opc, 0, 1);
//        is_signed = extract32(opc, 1, 1);
//    }

//    std::vector< std::list<InternalDependance> > rs_deps(2);
//    addAddressCompute( inst, rs_deps ) ;
//    ArmFormatOperands( inst, operands.rn(), rs_deps );

//    //tcg_rt = cpu_reg(s, rt);
//    //tcg_addr = tcg_const_i64((s->pc - 4) + imm);

//    predicated_dependant_action load = loadAction( inst, (operands.rd() == 0 ? kWord : kDoubleWord), false, boost::none );

//    if (is_vector) {
//        inst->setClass(clsLoad, codeLoadFP);
//        //do_fp_ld(s, rt, tcg_addr, size);
//        inst->addSquashEffect( eraseLSQ(inst) );
////        inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
//        inst->addRetirementEffect( retireMem(inst) );


//        inst->addDispatchEffect( allocateLoad( inst, (operands.rd() == 0 ? kWord : kDoubleWord), load.dependance, kAccType_ORDERED ) );
//        inst->addCommitEffect( accessMem(inst) );
//        inst->addRetirementConstraint( loadMemoryConstraint(inst) );

//        connectDependance( inst->retirementDependance(), load );
//        inst->addRetirementEffect( writeFPSR(inst, (operands.rd() == 0 ? kWord : kDoubleWord)) );
//        inst->setHaltDispatch();
////        inst->addPostvalidation( validateFPSR(inst) );

//    } else {
//        inst->setClass(clsLoad, codeLoad);
////        inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
//        inst->addRetirementEffect( retireMem(inst) );
//        inst->addSquashEffect( eraseLSQ(inst) );
//        /* Only unsigned 32bit loads target 32bit registers.  */
//        bool iss_sf = opc != 0;
//        //do_gpr_ld(s, tcg_rt, tcg_addr, size, is_signed, false, true, rt, iss_sf, false);

//        if (operands.rd() == 0) {
//          load = loadAction( inst, eSize(1<<size), is_signed, boost::none );
//        } else {
//          load = loadAction( inst, eSize(1<<size), is_signed, kPD );
//        }
//    }
//    inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_ORDERED ) );
//    inst->addCommitEffect( accessMem(inst) );
//    inst->addRetirementConstraint( loadMemoryConstraint(inst) );
//    addDestination( inst, operands.rd(), load);
//    //tcg_temp_free_i64(tcg_addr);
//    return boost::intrusive_ptr<armInstruction>(inst);

}

/* Load/storeexclusive
 * 31 30             24 23 22 21 20    16 15       10     5    0                    0
 * +----+-------------+---+--+--+--------+--+--------+-----+----+
 * |size| 0 0 1 0 0 0 | o2|L |o1|  Rs    |o0|  Rt2   |  Rn | Rt |
 * +----+-------------+---+--+--+--------+--+--------+-----+----+
 */
arminst disas_ldst_excl(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1); // is_lasr
    int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int o1 = extract32(aFetchedOpcode.theOpcode, 21, 1);  // is_pair
    int L = extract32(aFetchedOpcode.theOpcode, 22, 1); // is_store
    int o2 = extract32(aFetchedOpcode.theOpcode, 23, 1); // is_excl
    int size = extract32(aFetchedOpcode.theOpcode, 30, 2);

    if ( ((o2 && o1) || (!o2 && o1)) && rt2 != 31 && size != 0) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    int decision = o0 | (o1<<1) | (L<<2) | (o2<<3);

    switch (decision) {
    case 0:
        STXRB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 1:
        STLXRB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 2:case 3:
    case 6:case 7:
        CASP(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 4:
        LDXRB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 5:
        LDAXRB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 8:
        STLLRB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 9:
        STLRB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 10:case 11:
    case 14:case 15:
        CASB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 12:
        LDLARB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 13:
        LDARB(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    default:
        DBG_Assert(false);
        break;
    }

    return inst;
}

/* Loads and stores */
arminst disas_ldst(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 24, 6)) {
    case 0x08: /* Load/store exclusive */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Load/store exclusiv \033[0m"));

        return disas_ldst_excl(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x18: case 0x1c: /* Load register (literal) */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Load register (literal) \033[0m"));

        return disas_ld_lit(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x28: case 0x29:
    case 0x2c: case 0x2d: /* Load/store pair (all forms) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/store pair (all forms) \033[0m"));
        return disas_ldst_pair(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x38: case 0x39:
    case 0x3c: case 0x3d: /* Load/store register (all forms) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/store register (all forms) \033[0m"));
        return disas_ldst_reg(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x0c: /* AdvSIMD load/store multiple structures */
        DBG_(Tmp,(<< "\033[1;31mDECODER: AdvSIMD load/store multiple structures \033[0m"));
        disas_ldst_multiple_struct(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x0d: /* AdvSIMD load/store single structure */
        DBG_(Tmp,(<< "\033[1;31mDECODER: AdvSIMD load/store single structure \033[0m"));
        disas_ldst_single_struct(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

} // narmDecoder
