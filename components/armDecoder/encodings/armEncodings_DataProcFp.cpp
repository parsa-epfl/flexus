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
#include "armDataProcFp.hpp"
#include "armUnallocated.hpp"
#include "armMagic.hpp"

namespace narmDecoder {




/* Floating point <-> integer conversions
 *   31   30  29 28       24 23  22  21 20   19 18 16 15         10 9  5 4  0
 * +----+---+---+-----------+------+---+-------+-----+-------------+----+----+
 * | sf | 0 | S | 1 1 1 1 0 | type | 1 | rmode | opc | 0 0 0 0 0 0 | Rn | Rd |
 * +----+---+---+-----------+------+---+-------+-----+-------------+----+----+
 */
arminst disas_fp_int_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int opcode = extract32(aFetchedOpcode.theOpcode, 16, 3);
    int rmode = extract32(aFetchedOpcode.theOpcode, 19, 2);
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool sbit = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

    if (sbit) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    if (opcode > 5) {
        /* FMOV */
        bool itof = opcode & 1;

        if (rmode >= 2) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }

        switch (sf << 3 | type << 1 | rmode) {
        case 0x0: /* 32 bit */
        case 0xa: /* 64 bit */
        case 0xd: /* 64 bit to top half of quad */
            break;
        default:
            /* all other sf/type/rmode combinations are invalid */
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }

//        if (!fp_access_check(s)) {
//            return;
//        }
//        handle_fmov(s, rd, rn, type, itof);
    } else {
        /* actual FP conversions */
        bool itof = extract32(opcode, 1, 1);

        if (type > 1 || (rmode != 0 && opcode > 1)) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }

//        if (!fp_access_check(s)) {
//            return;
//        }
//        handle_fpfpcvt(s, rd, rn, opcode, itof, rmode, 64, sf, type);
    }
}


/* Floating point conditional compare
 *   31  30  29 28       24 23  22  21 20  16 15  12 11 10 9    5  4   3    0
 * +---+---+---+-----------+------+---+------+------+-----+------+----+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | cond | 0 1 |  Rn  | op | nzcv |
 * +---+---+---+-----------+------+---+------+------+-----+------+----+------+
 */
arminst disas_fp_ccomp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int mos, type, rm, cond, rn, op, nzcv;
//    TCGv_i64 tcg_flags;
//    TCGLabel *label_continue = NULL;

    mos = extract32(aFetchedOpcode.theOpcode, 29, 3);
    type = extract32(aFetchedOpcode.theOpcode, 22, 2); /* 0 = single, 1 = double */
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    op = extract32(aFetchedOpcode.theOpcode, 4, 1);
    nzcv = extract32(aFetchedOpcode.theOpcode, 0, 4);

    if (mos || type > 1) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

//    if (cond < 0x0e) { /* not always */
//        TCGLabel *label_match = gen_new_label();
//        label_continue = gen_new_label();
//        arm_gen_test_cc(cond, label_match);
//        /* nomatch: */
//        tcg_flags = tcg_const_i64(nzcv << 28);
//        gen_set_nzcv(tcg_flags);
//        tcg_temp_free_i64(tcg_flags);
//        tcg_gen_br(label_continue);
//        gen_set_label(label_match);
//    }

//    handle_fp_compare(s, type, rn, rm, false, op);

    if (cond < 0x0e) {
//        gen_set_label(label_continue);
    }
}

/* Floating point conditional select
 *   31  30  29 28       24 23  22  21 20  16 15  12 11 10 9    5 4    0
 * +---+---+---+-----------+------+---+------+------+-----+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | cond | 1 1 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+------+------+-----+------+------+
 */
arminst disas_fp_csel(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int mos, type, rm, cond, rn, rd;
//    TCGv_i64 t_true, t_false, t_zero;
//    DisasCompare64 c;

    mos = extract32(aFetchedOpcode.theOpcode, 29, 3);
    type = extract32(aFetchedOpcode.theOpcode, 22, 2); /* 0 = single, 1 = double */
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (mos || type > 1) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

    /* Zero extend sreg inputs to 64 bits now.  */
//    t_true = tcg_temp_new_i64();
//    t_false = tcg_temp_new_i64();
//    read_vec_element(s, t_true, rn, 0, type ? MO_64 : MO_32);
//    read_vec_element(s, t_false, rm, 0, type ? MO_64 : MO_32);

//    a64_test_cc(&c, cond);
//    t_zero = tcg_const_i64(0);
//    tcg_gen_movcond_i64(c.cond, t_true, c.value, t_zero, t_true, t_false);
//    tcg_temp_free_i64(t_zero);
//    tcg_temp_free_i64(t_false);
//    a64_free_cc(&c);

    /* Note that sregs write back zeros to the high bits,
       and we've already done the zero-extension.  */
//    write_fp_dreg(s, rd, t_true);
//    tcg_temp_free_i64(t_true);
}


/* Floating point <-> fixed point conversions
 *   31   30  29 28       24 23  22  21 20   19 18    16 15   10 9    5 4    0
 * +----+---+---+-----------+------+---+-------+--------+-------+------+------+
 * | sf | 0 | S | 1 1 1 1 0 | type | 0 | rmode | opcode | scale |  Rn  |  Rd  |
 * +----+---+---+-----------+------+---+-------+--------+-------+------+------+
 */
arminst disas_fp_fixed_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int scale = extract32(aFetchedOpcode.theOpcode, 10, 6);
    int opcode = extract32(aFetchedOpcode.theOpcode, 16, 3);
    int rmode = extract32(aFetchedOpcode.theOpcode, 19, 2);
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool sbit = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool itof;

    if (sbit || (type > 1)
        || (!sf && scale < 32)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch ((rmode << 3) | opcode) {
    case 0x2: /* SCVTF */
    case 0x3: /* UCVTF */
        itof = true;
        break;
    case 0x18: /* FCVTZS */
    case 0x19: /* FCVTZU */
        itof = false;
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

//    handle_fpfpcvt(s, rd, rn, opcode, itof, FPROUNDING_ZERO, scale, sf, type);
}



/* Floating-point data-processing (2 source) - single precision */
arminst handle_fp_2src_single(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int opcode,
                                  int rd, int rn, int rm)
{
//    TCGv_i32 tcg_op1;
//    TCGv_i32 tcg_op2;
//    TCGv_i32 tcg_res;
//    TCGv_ptr fpst;

//    tcg_res = tcg_temp_new_i32();
//    fpst = get_fpstatus_ptr();
//    tcg_op1 = read_fp_sreg(s, rn);
//    tcg_op2 = read_fp_sreg(s, rm);

    switch (opcode) {
    case 0x0: /* FMUL */
//        gen_helper_vfp_muls(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x1: /* FDIV */
//        gen_helper_vfp_divs(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x2: /* FADD */
//        gen_helper_vfp_adds(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x3: /* FSUB */
//        gen_helper_vfp_subs(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x4: /* FMAX */
//        gen_helper_vfp_maxs(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x5: /* FMIN */
//        gen_helper_vfp_mins(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x6: /* FMAXNM */
//        gen_helper_vfp_maxnums(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x7: /* FMINNM */
//        gen_helper_vfp_minnums(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x8: /* FNMUL */
//        gen_helper_vfp_muls(tcg_res, tcg_op1, tcg_op2, fpst);
//        gen_helper_vfp_negs(tcg_res, tcg_res);
        break;
    }

//    write_fp_sreg(s, rd, tcg_res);

//    tcg_temp_free_ptr(fpst);
//    tcg_temp_free_i32(tcg_op1);
//    tcg_temp_free_i32(tcg_op2);
//    tcg_temp_free_i32(tcg_res);
}

/* Floating-point data-processing (2 source) - double precision */
arminst handle_fp_2src_double(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int opcode,
                                  int rd, int rn, int rm)
{
//    TCGv_i64 tcg_op1;
//    TCGv_i64 tcg_op2;
//    TCGv_i64 tcg_res;
//    TCGv_ptr fpst;

//    tcg_res = tcg_temp_new_i64();
//    fpst = get_fpstatus_ptr();
//    tcg_op1 = read_fp_dreg(s, rn);
//    tcg_op2 = read_fp_dreg(s, rm);

    switch (opcode) {
    case 0x0: /* FMUL */
//        gen_helper_vfp_muld(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x1: /* FDIV */
//        gen_helper_vfp_divd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x2: /* FADD */
//        gen_helper_vfp_addd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x3: /* FSUB */
//        gen_helper_vfp_subd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x4: /* FMAX */
//        gen_helper_vfp_maxd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x5: /* FMIN */
//        gen_helper_vfp_mind(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x6: /* FMAXNM */
//        gen_helper_vfp_maxnumd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x7: /* FMINNM */
//        gen_helper_vfp_minnumd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x8: /* FNMUL */
//        gen_helper_vfp_muld(tcg_res, tcg_op1, tcg_op2, fpst);
//        gen_helper_vfp_negd(tcg_res, tcg_res);
        break;
    }

//    write_fp_dreg(s, rd, tcg_res);

//    tcg_temp_free_ptr(fpst);
//    tcg_temp_free_i64(tcg_op1);
//    tcg_temp_free_i64(tcg_op2);
//    tcg_temp_free_i64(tcg_res);
}

/* Floating point data-processing (1 source)
 *   31  30  29 28       24 23  22  21 20    15 14       10 9    5 4    0
 * +---+---+---+-----------+------+---+--------+-----------+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 | opcode | 1 0 0 0 0 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+--------+-----------+------+------+
 */
arminst disas_fp_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    int opcode = extract32(aFetchedOpcode.theOpcode, 15, 6);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    switch (opcode) {
    case 0x4: case 0x5: case 0x7:
    {
        /* FCVT between half, single and double precision */
        int dtype = extract32(opcode, 0, 2);
        if (type == 2 || dtype == type) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
//        if (!fp_access_check(s)) {
//            return;
//        }

//        handle_fp_fcvt(s, opcode, rd, rn, dtype, type);
        break;
    }
    case 0x0 ... 0x3:
    case 0x8 ... 0xc:
    case 0xe ... 0xf:
        /* 32-to-32 and 64-to-64 ops */
        switch (type) {
        case 0:
//            if (!fp_access_check(s)) {
//                return;
//            }

//            handle_fp_1src_single(s, opcode, rd, rn);
            break;
        case 1:
//            if (!fp_access_check(s)) {
//                return;
//            }

//            handle_fp_1src_double(s, opcode, rd, rn);
//            break;
        default:
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        break;
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/* Floating point compare
 *   31  30  29 28       24 23  22  21 20  16 15 14 13  10    9    5 4     0
 * +---+---+---+-----------+------+---+------+-----+---------+------+-------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | op  | 1 0 0 0 |  Rn  |  op2  |
 * +---+---+---+-----------+------+---+------+-----+---------+------+-------+
 */
arminst disas_fp_compare(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int mos, type, rm, op, rn, opc, op2r;

    mos = extract32(aFetchedOpcode.theOpcode, 29, 3);
    type = extract32(aFetchedOpcode.theOpcode, 22, 2); /* 0 = single, 1 = double */
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    op = extract32(aFetchedOpcode.theOpcode, 14, 2);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    opc = extract32(aFetchedOpcode.theOpcode, 3, 2);
    op2r = extract32(aFetchedOpcode.theOpcode, 0, 3);

    if (mos || op || op2r || type > 1) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

//    handle_fp_compare(s, type, rn, rm, opc & 1, opc & 2);
}

/* Floating point data-processing (2 source)
 *   31  30  29 28       24 23  22  21 20  16 15    12 11 10 9    5 4    0
 * +---+---+---+-----------+------+---+------+--------+-----+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | opcode | 1 0 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+------+--------+-----+------+------+
 */
arminst disas_fp_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int opcode = extract32(aFetchedOpcode.theOpcode, 12, 4);

    if (opcode > 8) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch (type) {
    case 0:
//        if (!fp_access_check(s)) {
//            return;
//        }
        handle_fp_2src_single(aFetchedOpcode,  aCPU, aSequenceNo, opcode, rd, rn, rm);
        break;
    case 1:
//        if (!fp_access_check(s)) {
//            return;
//        }
        handle_fp_2src_double(aFetchedOpcode, aCPU, aSequenceNo, opcode, rd, rn, rm);
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}



/* Floating point data-processing (3 source)
 *   31  30  29 28       24 23  22  21  20  16  15  14  10 9    5 4    0
 * +---+---+---+-----------+------+----+------+----+------+------+------+
 * | M | 0 | S | 1 1 1 1 1 | type | o1 |  Rm  | o0 |  Ra  |  Rn  |  Rd  |
 * +---+---+---+-----------+------+----+------+----+------+------+------+
 */
arminst disas_fp_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo )
{
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int ra = extract32(aFetchedOpcode.theOpcode, 10, 5);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    bool o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    bool o1 = extract32(aFetchedOpcode.theOpcode, 21, 1);

    switch (type) {
    case 0:
//        if (!fp_access_check(s)) {
//            return;
//        }
//        handle_fp_3src_single(o0, o1, rd, rn, rm, ra);
        break;
    case 1:
//        if (!fp_access_check(s)) {
//            return;
//        }
//        handle_fp_3src_double(o0, o1, rd, rn, rm, ra);
        break;
    default:
       return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

/* Floating point immediate
 *   31  30  29 28       24 23  22  21 20        13 12   10 9    5 4    0
 * +---+---+---+-----------+------+---+------------+-------+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |    imm8    | 1 0 0 | imm5 |  Rd  |
 * +---+---+---+-----------+------+---+------------+-------+------+------+
 */
arminst disas_fp_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int imm8 = extract32(aFetchedOpcode.theOpcode, 13, 8);
    int is_double = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint64_t imm;
//    TCGv_i64 tcg_res;

    if (is_double > 1) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

    /* The imm8 encodes the sign bit, enough bits to represent
     * an exponent in the range 01....1xx to 10....0xx,
     * and the most significant 4 bits of the mantissa; see
     * VFPExpandImm() in the v8 ARM ARM.
     */
    if (is_double) {
        imm = (extract32(imm8, 7, 1) ? 0x8000 : 0) |
            (extract32(imm8, 6, 1) ? 0x3fc0 : 0x4000) |
            extract32(imm8, 0, 6);
        imm <<= 48;
    } else {
        imm = (extract32(imm8, 7, 1) ? 0x8000 : 0) |
            (extract32(imm8, 6, 1) ? 0x3e00 : 0x4000) |
            (extract32(imm8, 0, 6) << 3);
        imm <<= 16;
    }

//    tcg_res = tcg_const_i64(imm);
//    write_fp_dreg(s, rd, tcg_res);
//    tcg_temp_free_i64(tcg_res);
}


/* FP-specific subcases of table C3-6 (SIMD and FP data processing)
 *   31  30  29 28     25 24                          0
 * +---+---+---+---------+-----------------------------+
 * |   | 0 |   | 1 1 1 1 |                             |
 * +---+---+---+---------+-----------------------------+
 */
arminst disas_data_proc_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    assert(false);
    if (extract32(aFetchedOpcode.theOpcode, 24, 1)) {
        /* Floating point data-processing (3 source) */
        disas_fp_3src(aFetchedOpcode, aCPU, aSequenceNo);
    } else if (extract32(aFetchedOpcode.theOpcode, 21, 1) == 0) {
        /* Floating point to fixed point conversions */
        disas_fp_fixed_conv(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        switch (extract32(aFetchedOpcode.theOpcode, 10, 2)) {
        case 1:
            /* Floating point conditional compare */
            disas_fp_ccomp(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        case 2:
            /* Floating point data-processing (2 source) */
            disas_fp_2src(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        case 3:
            /* Floating point conditional select */
            disas_fp_csel(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        case 0:
            switch (ctz32(extract32(aFetchedOpcode.theOpcode, 12, 4))) {
            case 0: /* [15:12] == xxx1 */
                /* Floating point immediate */
                disas_fp_imm(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 1: /* [15:12] == xx10 */
                /* Floating point compare */
                disas_fp_compare(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 2: /* [15:12] == x100 */
                /* Floating point data-processing (1 source) */
                disas_fp_1src(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 3: /* [15:12] == 1000 */
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            default: /* [15:12] == 0000 */
                /* Floating point <-> integer conversions */
                return disas_fp_int_conv(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            }
            break;
        }
    }
}

arminst disas_data_proc_simd(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* C3.6 Data processing - SIMD and floating point */
arminst disas_data_proc_simd_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (extract32(aFetchedOpcode.theOpcode, 28, 1) == 1 && extract32(aFetchedOpcode.theOpcode, 30, 1) == 0) {
        return disas_data_proc_fp(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        /* SIMD, including crypto */
        return disas_data_proc_simd(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

} // narmDecoder
