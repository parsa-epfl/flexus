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
    if (extract32(aFetchedOpcode.theOpcode, 29, 1)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    switch (extract32(aFetchedOpcode.theOpcode, 10, 6)) {
    case 2: /* UDIV */
        return UDIV(aFetchedOpcode, aCPU, aSequenceNo);
    case 3: /* SDIV */
        return SDIV(aFetchedOpcode, aCPU, aSequenceNo);

    case 8: /* LSLV */
        return LSLV(aFetchedOpcode, aCPU, aSequenceNo);

    case 9: /* LSRV */
        return LSRV(aFetchedOpcode, aCPU, aSequenceNo);

    case 10: /* ASRV */
        return ASRV(aFetchedOpcode, aCPU, aSequenceNo);

    case 11: /* RORV */
        return RORV(aFetchedOpcode, aCPU, aSequenceNo);

    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23: /* CRC32 */
        return CRC32(aFetchedOpcode, aCPU, aSequenceNo);
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
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
    if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 16, 5)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    switch (extract32(aFetchedOpcode.theOpcode, 10, 6)) {
    case 0: /* RBIT */
        return RBIT(aFetchedOpcode, aCPU, aSequenceNo);
    case 1: /* REV16 */
        return REV16(aFetchedOpcode, aCPU, aSequenceNo);
    case 2: /* REV32 */
        return REV32(aFetchedOpcode, aCPU, aSequenceNo);
    case 3: /* REV64 */
        return REV64(aFetchedOpcode, aCPU, aSequenceNo);
    case 4: /* CLZ */
        return CLZ(aFetchedOpcode, aCPU, aSequenceNo);
    case 5: /* CLS */
        return CLS(aFetchedOpcode, aCPU, aSequenceNo);
    default:
        DBG_Assert(false);
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
    if (extract32(aFetchedOpcode.thePC, 29, 1) || extract32(aFetchedOpcode.thePC, 11, 1)) {
        /* S == 1 or op2<1> == 1 */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (!extract32(aFetchedOpcode.thePC, 30, 1)) {
        return extract32(aFetchedOpcode.thePC, 10, 1) ? CSINC(aFetchedOpcode, aCPU, aSequenceNo) : CSEL(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        return extract32(aFetchedOpcode.thePC, 10, 1) ? CSNEG(aFetchedOpcode, aCPU, aSequenceNo) : CSINV(aFetchedOpcode, aCPU, aSequenceNo);
    }

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
    if (!extract32(aFetchedOpcode.thePC, 29, 1)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    if (aFetchedOpcode.thePC & (1 << 10 | 1 << 4)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    return extract32(aFetchedOpcode.thePC, 30, 1) ?
        CCMP(aFetchedOpcode, aCPU, aSequenceNo) : CCMN(aFetchedOpcode, aCPU, aSequenceNo);

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
    if (extract32(aFetchedOpcode.thePC, 10, 6) != 0) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (extract32(aFetchedOpcode.thePC, 30, 1)) {
        return extract32(aFetchedOpcode.thePC,29,1) ? ADC(aFetchedOpcode, aCPU, aSequenceNo) : ADCS(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        return extract32(aFetchedOpcode.thePC,29,1) ? SBC(aFetchedOpcode, aCPU, aSequenceNo) : SBCS(aFetchedOpcode, aCPU, aSequenceNo);
    }
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
    int op_id = (extract32(aFetchedOpcode.thePC, 29, 3) << 4) |
                (extract32(aFetchedOpcode.thePC, 21, 3) << 1) |
                 extract32(aFetchedOpcode.thePC, 15, 1);
    bool is_signed = false;

    /* Note that op_id is sf:op54:op31:o0 so it includes the 32/64 size flag */
    switch (op_id) {
    case 0x42: /* SMADDL */
    case 0x43: /* SMSUBL */
    case 0x44: /* SMULH */
        is_signed = true;
        break;
    case 0x0: /* MADD (32bit) */
    case 0x1: /* MSUB (32bit) */
    case 0x40: /* MADD (64bit) */
    case 0x41: /* MSUB (64bit) */
    case 0x4a: /* UMADDL */
    case 0x4b: /* UMSUBL */
    case 0x4c: /* UMULH */
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

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
    DECODER_TRACE;

    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

    if ((extract32(aFetchedOpcode.thePC,15,1) && ! extract32(aFetchedOpcode.thePC, 31, 1)) || extract32(aFetchedOpcode.thePC,22,2) == 0x4  ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (extract32(aFetchedOpcode.thePC, 30, 1) == 0){
        return (extract32(aFetchedOpcode.thePC,29,1) ? ADDS(aFetchedOpcode, aCPU, aSequenceNo) : ADD(aFetchedOpcode, aCPU, aSequenceNo));
    } else {
        return (extract32(aFetchedOpcode.thePC,29,1) ? SUBS(aFetchedOpcode, aCPU, aSequenceNo) : SUB(aFetchedOpcode, aCPU, aSequenceNo));
    }
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

    if (extract32(aFetchedOpcode.thePC,10,3) > 0x4 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    if (extract32(aFetchedOpcode.thePC,10,3) != 0x0 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }


    if (!extract32(aFetchedOpcode.thePC, 30, 1)){
        return extract32(aFetchedOpcode.thePC,29,1) ? ADDS(aFetchedOpcode, aCPU, aSequenceNo) : ADD(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        return extract32(aFetchedOpcode.thePC,29,1) ? SUBS(aFetchedOpcode, aCPU, aSequenceNo) : SUB(aFetchedOpcode, aCPU, aSequenceNo);
    }

}


/* Logical (shifted register)
 *   31  30 29 28       24 23   22 21  20  16 15    10 9    5 4    0
 * +----+-----+-----------+-------+---+------+--------+------+------+
 * | sf | opc | 0 1 0 1 0 | shift | N |  Rm  |  imm6  |  Rn  |  Rd  |
 * +----+-----+-----------+-------+---+------+--------+------+------+
 */
arminst disas_logic_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int N = extract32(aFetchedOpcode.thePC, 21, 1);

    switch (extract32(aFetchedOpcode.thePC, 29, 2)) {
    case 0x0:
        return N ? BIC(aFetchedOpcode, aCPU, aSequenceNo) : AND(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x1:
        return N ? ORN(aFetchedOpcode, aCPU, aSequenceNo) : ORR(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x2:
        return N ? EON(aFetchedOpcode, aCPU, aSequenceNo) : EOR(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x3:
        return N ? BICS(aFetchedOpcode, aCPU, aSequenceNo) : ANDS(aFetchedOpcode, aCPU, aSequenceNo);
    default:
        DBG_Assert(false);
        break;
    }
}

/* Data processing - register */
arminst disas_data_proc_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.thePC, 24, 5)) {
        case 0x0a: /* Logical (shifted register) */
            return disas_logic_reg(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x0b: /* Add/subtract */
            if (aFetchedOpcode.thePC & (1 << 21)) { /* (extended register) */
                return disas_add_sub_ext_reg(aFetchedOpcode, aCPU, aSequenceNo);
            } else {
                return disas_add_sub_reg(aFetchedOpcode, aCPU, aSequenceNo);
            }
        case 0x1b: /* Data-processing (3 source) */
            return disas_data_proc_3src(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1a:
            switch (extract32(aFetchedOpcode.thePC, 21, 3)) {
            case 0x0: /* Add/subtract (with carry) */
                return disas_adc_sbc(aFetchedOpcode, aCPU, aSequenceNo);
            case 0x2: /* Conditional compare */
                return disas_cc(aFetchedOpcode, aCPU, aSequenceNo); /* both imm and reg forms */
            case 0x4: /* Conditional select */
                return disas_cond_select(aFetchedOpcode, aCPU, aSequenceNo);
            case 0x6: /* Data-processing */
                if (aFetchedOpcode.thePC & (1 << 30)) { /* (1 source) */
                    return disas_data_proc_1src(aFetchedOpcode, aCPU, aSequenceNo);
                } else {            /* (2 source) */
                    return disas_data_proc_2src(aFetchedOpcode, aCPU, aSequenceNo);
                }
                break;
            default:
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }
            break;
        default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
}

} // narmDecoder
