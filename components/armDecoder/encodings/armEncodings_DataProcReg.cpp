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
#include "armDataProcReg.hpp"
#include "armUnallocated.hpp"
#include "armMagic.hpp"

namespace narmDecoder {



/* Data-processing (2 source)
 *   31   30  29 28             21 20  16 15    10 9    5 4    0
 * +----+---+---+-----------------+------+--------+------+------+
 * | sf | 0 | S | 1 1 0 1 0 1 1 0 |  Rm  | opcode |  Rn  |  Rd  |
 * +----+---+---+-----------------+------+--------+------+------+
 */
arminst disas_data_proc_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, rm, opcode, rn, rd;
    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    opcode = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (extract32(aFetchedOpcode.theOpcode, 29, 1)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch (opcode) {
    case 2: /* UDIV */
//        handle_div(aFetchedOpcode, aCPU, aSequenceNo, false, sf, rm, rn, rd);
        break;
    case 3: /* SDIV */
//        handle_div(aFetchedOpcode, aCPU, aSequenceNo, true, sf, rm, rn, rd);
        break;
    case 8: /* LSLV */
//        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_LSL, sf, rm, rn, rd);
        break;
    case 9: /* LSRV */
//        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_LSR, sf, rm, rn, rd);
        break;
    case 10: /* ASRV */
//        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_ASR, sf, rm, rn, rd);
        break;
    case 11: /* RORV */
//        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_ROR, sf, rm, rn, rd);
        break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23: /* CRC32 */
    {
        int sz = extract32(opcode, 0, 2);
        bool crc32c = extract32(opcode, 2, 1);
//        handle_crc32(aFetchedOpcode, aCPU, aSequenceNo, sf, sz, crc32c, rm, rn, rd);
        break;
    }
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/* Data-processing (1 source)
 *   31  30  29  28             21 20     16 15    10 9    5 4    0
 * +----+---+---+-----------------+---------+--------+------+------+
 * | sf | 1 | S | 1 1 0 1 0 1 1 0 | opcode2 | opcode |  Rn  |  Rd  |
 * +----+---+---+-----------------+---------+--------+------+------+
 */
arminst disas_data_proc_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, opcode, rn, rd;

    if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 16, 5)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opcode = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    switch (opcode) {
    case 0: /* RBIT */
//        handle_rbit(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 1: /* REV16 */
//        handle_rev16(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 2: /* REV32 */
//        handle_rev32(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 3: /* REV64 */
//        handle_rev64(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 4: /* CLZ */
//        handle_clz(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 5: /* CLS */
//        handle_cls(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    }
}

/* Conditional select
 *   31   30  29  28             21 20  16 15  12 11 10 9    5 4    0
 * +----+----+---+-----------------+------+------+-----+------+------+
 * | sf | op | S | 1 1 0 1 0 1 0 0 |  Rm  | cond | op2 |  Rn  |  Rd  |
 * +----+----+---+-----------------+------+------+-----+------+------+
 */
arminst disas_cond_select(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, else_inv, rm, cond, else_inc, rn, rd;
//    TCGv_i64 tcg_rd, zero;
//    DisasCompare64 c;

    if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 11, 1)) {
        /* S == 1 or op2<1> == 1 */
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    else_inv = extract32(aFetchedOpcode.theOpcode, 30, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    else_inc = extract32(aFetchedOpcode.theOpcode, 10, 1);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

//    tcg_rd = cpu_reg(s, rd);

//    a64_test_cc(&c, cond);
//    zero = tcg_const_i64(0);

//    if (rn == 31 && rm == 31 && (else_inc ^ else_inv)) {
//        /* CSET & CSETM.  */
//        //tcg_gen_setcond_i64(tcg_invert_cond(c.cond), tcg_rd, c.value, zero);
//        if (else_inv) {
//            //tcg_gen_neg_i64(tcg_rd, tcg_rd);
//        }
//    } else {
//        TCGv_i64 t_true = cpu_reg(s, rn);
//        TCGv_i64 t_false = read_cpu_reg(s, rm, 1);
//        if (else_inv && else_inc) {
//            //tcg_gen_neg_i64(t_false, t_false);
//        } else if (else_inv) {
//            //tcg_gen_not_i64(t_false, t_false);
//        } else if (else_inc) {
//            //tcg_gen_addi_i64(t_false, t_false, 1);
//        }
//        //tcg_gen_movcond_i64(c.cond, tcg_rd, c.value, zero, t_true, t_false);
//    }

//    tcg_temp_free_i64(zero);
//    a64_free_cc(&c);

//    if (!sf) {
//        //tcg_gen_ext32u_i64(tcg_rd, tcg_rd);
//    }
}

/* Conditional compare (immediate / register)
 *  31 30 29 28 27 26 25 24 23 22 21  20    16 15  12  11  10  9   5  4 3   0
 * +--+--+--+------------------------+--------+------+----+--+------+--+-----+
 * |sf|op| S| 1  1  0  1  0  0  1  0 |imm5/rm | cond |i/r |o2|  Rn  |o3|nzcv |
 * +--+--+--+------------------------+--------+------+----+--+------+--+-----+
 *        [1]                             y                [0]       [0]
 */
arminst disas_cc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, op, y, cond, rn, nzcv, is_imm;
//    TCGv_i32 tcg_t0, tcg_t1, tcg_t2;
//    TCGv_i64 tcg_tmp, tcg_y, tcg_rn;
//    DisasCompare c;

    if (!extract32(aFetchedOpcode.theOpcode, 29, 1)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }
    if (aFetchedOpcode.theOpcode & (1 << 10 | 1 << 4)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }
    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    is_imm = extract32(aFetchedOpcode.theOpcode, 11, 1);
    y = extract32(aFetchedOpcode.theOpcode, 16, 5); /* y = rm (mapped_reg) or imm5 (imm) */
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    nzcv = extract32(aFetchedOpcode.theOpcode, 0, 4);

    /* Set T0 = !COND.  */
//    tcg_t0 = tcg_temp_new_i32();
//    arm_test_cc(&c, cond);
//    //tcg_gen_setcondi_i32(tcg_invert_cond(c.cond), tcg_t0, c.value, 0);
//    arm_free_cc(&c);

//    /* Load the arguments for the new comparison.  */
//    if (is_imm) {
//        tcg_y = new_tmp_a64(s);
//        //tcg_gen_movi_i64(tcg_y, y);
//    } else {
//        tcg_y = cpu_reg(s, y);
//    }
//    tcg_rn = cpu_reg(s, rn);

//    /* Set the flags for the new comparison.  */
//    tcg_tmp = tcg_temp_new_i64();
//    if (op) {
//        gen_sub_CC(sf, tcg_tmp, tcg_rn, tcg_y);
//    } else {
//        gen_add_CC(sf, tcg_tmp, tcg_rn, tcg_y);
//    }
//    tcg_temp_free_i64(tcg_tmp);

//    /* If COND was false, force the flags to #nzcv.  Compute two masks
//     * to help with this: T1 = (COND ? 0 : -1), T2 = (COND ? -1 : 0).
//     * For tcg hosts that support ANDC, we can make do with just T1.
//     * In either case, allow the tcg optimizer to delete any unused mask.
//     */
//    tcg_t1 = tcg_temp_new_i32();
//    tcg_t2 = tcg_temp_new_i32();
//    //tcg_gen_neg_i32(tcg_t1, tcg_t0);
//    //tcg_gen_subi_i32(tcg_t2, tcg_t0, 1);

//    if (nzcv & 8) { /* N */
//        //tcg_gen_or_i32(cpu_NF, cpu_NF, tcg_t1);
//    } else {
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_NF, cpu_NF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_NF, cpu_NF, tcg_t2);
//        }
//    }
//    if (nzcv & 4) { /* Z */
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_ZF, cpu_ZF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_ZF, cpu_ZF, tcg_t2);
//        }
//    } else {
//        //tcg_gen_or_i32(cpu_ZF, cpu_ZF, tcg_t0);
//    }
//    if (nzcv & 2) { /* C */
//        //tcg_gen_or_i32(cpu_CF, cpu_CF, tcg_t0);
//    } else {
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_CF, cpu_CF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_CF, cpu_CF, tcg_t2);
//        }
//    }
//    if (nzcv & 1) { /* V */
//        //tcg_gen_or_i32(cpu_VF, cpu_VF, tcg_t1);
//    } else {
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_VF, cpu_VF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_VF, cpu_VF, tcg_t2);
//        }
//    }
//    tcg_temp_free_i32(tcg_t0);
//    tcg_temp_free_i32(tcg_t1);
//    tcg_temp_free_i32(tcg_t2);
}

/* Add/subtract (with carry)
 *  31 30 29 28 27 26 25 24 23 22 21  20  16  15   10  9    5 4   0
 * +--+--+--+------------------------+------+---------+------+-----+
 * |sf|op| S| 1  1  0  1  0  0  0  0 |  rm  | opcode2 |  Rn  |  Rd |
 * +--+--+--+------------------------+------+---------+------+-----+
 *                                            [000000]
 */
arminst disas_adc_sbc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

}

/* Data-processing (3 source)
 *
 *    31 30  29 28       24 23 21  20  16  15  14  10 9    5 4    0
 *  +--+------+-----------+------+------+----+------+------+------+
 *  |sf| op54 | 1 1 0 1 1 | op31 |  Rm  | o0 |  Ra  |  Rn  |  Rd  |
 *  +--+------+-----------+------+------+----+------+------+------+
 */
arminst disas_data_proc_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/*
 * Add/subtract (shifted register)
 *
 *  31 30 29 28       24 23 22 21 20   16 15     10 9    5 4    0
 * +--+--+--+-----------+-----+--+-------+---------+------+------+
 * |sf|op| S| 0 1 0 1 1 |shift| 0|  Rm   |  imm6   |  Rn  |  Rd  |
 * +--+--+--+-----------+-----+--+-------+---------+------+------+
 *
 *    sf: 0 -> 32bit, 1 -> 64bit
 *    op: 0 -> add  , 1 -> sub
 *     S: 1 -> set flags
 * shift: 00 -> LSL, 01 -> LSR, 10 -> ASR, 11 -> RESERVED
 *  imm6: Shift amount to apply to Rm before the add/sub
 */
arminst disas_add_sub_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

}

/*
 * Add/subtract (extended register)
 *
 *  31|30|29|28       24|23 22|21|20   16|15  13|12  10|9  5|4  0|
 * +--+--+--+-----------+-----+--+-------+------+------+----+----+
 * |sf|op| S| 0 1 0 1 1 | opt | 1|  Rm   |option| imm3 | Rn | Rd |
 * +--+--+--+-----------+-----+--+-------+------+------+----+----+
 *
 *  sf: 0 -> 32bit, 1 -> 64bit
 *  op: 0 -> add  , 1 -> sub
 *   S: 1 -> set flags
 * opt: 00
 * option: extension type (see DecodeRegExtend)
 * imm3: optional shift to Rm
 *
 * Rd = Rn + LSL(extend(Rm), amount)
 */
arminst disas_add_sub_ext_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

}


arminst disas_logic_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

}

/* Data processing - register */
arminst disas_data_proc_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 24, 5))
    {
        case 0x0a: /* Logical (shifted register) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: disas_logic_reg  \033[0m"));
            return disas_logic_reg(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x0b: /* Add/subtract */
            if (aFetchedOpcode.theOpcode & (1 << 21)) { /* (extended register) */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: disas_add_sub_ext_reg  \033[0m"));
                return disas_add_sub_ext_reg(aFetchedOpcode, aCPU, aSequenceNo);
            } else {
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: disas_add_sub_reg  \033[0m"));
                return disas_add_sub_reg(aFetchedOpcode, aCPU, aSequenceNo);
            }
            break;
        case 0x1b: /* Data-processing (3 source) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: Data-processing (3 source)  \033[0m"));
            return disas_data_proc_3src(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1a:
            switch (extract32(aFetchedOpcode.theOpcode, 21, 3))
            {
                case 0x0: /* Add/subtract (with carry) */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - Add/subtract (with carry)  \033[0m"));
                    return disas_adc_sbc(aFetchedOpcode, aCPU, aSequenceNo);
                case 0x2: /* Conditional compare */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - Conditional compare  \033[0m"));
                    return disas_cc(aFetchedOpcode, aCPU, aSequenceNo); /* both imm and reg forms */
                case 0x4: /* Conditional select */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - Conditional select  \033[0m"));
                    return disas_cond_select(aFetchedOpcode, aCPU, aSequenceNo);
                case 0x6: /* Data-processing */
                    if (aFetchedOpcode.theOpcode & (1 << 30)) { /* (1 source) */
                        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - (1 source)  \033[0m"));
                        return disas_data_proc_1src(aFetchedOpcode, aCPU, aSequenceNo);
                    } else {            /* (2 source) */
                        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - (2 source)   \033[0m"));
                        return disas_data_proc_2src(aFetchedOpcode, aCPU, aSequenceNo);
                    }
                    break;
                default:
                    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }
        default:
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

} // narmDecoder
