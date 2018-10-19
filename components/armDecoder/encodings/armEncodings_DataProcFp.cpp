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
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}


/* Floating point conditional compare
 *   31  30  29 28       24 23  22  21 20  16 15  12 11 10 9    5  4   3    0
 * +---+---+---+-----------+------+---+------+------+-----+------+----+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | cond | 0 1 |  Rn  | op | nzcv |
 * +---+---+---+-----------+------+---+------+------+-----+------+----+------+
 */
arminst disas_fp_ccomp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Floating point conditional select
 *   31  30  29 28       24 23  22  21 20  16 15  12 11 10 9    5 4    0
 * +---+---+---+-----------+------+---+------+------+-----+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | cond | 1 1 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+------+------+-----+------+------+
 */
arminst disas_fp_csel(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}


/* Floating point <-> fixed point conversions
 *   31   30  29 28       24 23  22  21 20   19 18    16 15   10 9    5 4    0
 * +----+---+---+-----------+------+---+-------+--------+-------+------+------+
 * | sf | 0 | S | 1 1 1 1 0 | type | 0 | rmode | opcode | scale |  Rn  |  Rd  |
 * +----+---+---+-----------+------+---+-------+--------+-------+------+------+
 */
arminst disas_fp_fixed_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}



/* Floating-point data-processing (2 source) - single precision */
arminst handle_fp_2src_single(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Floating-point data-processing (2 source) - double precision */
arminst handle_fp_2src_double(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

}

/* Floating point data-processing (1 source)
 *   31  30  29 28       24 23  22  21 20    15 14       10 9    5 4    0
 * +---+---+---+-----------+------+---+--------+-----------+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 | opcode | 1 0 0 0 0 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+--------+-----------+------+------+
 */
arminst disas_fp_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Floating point compare
 *   31  30  29 28       24 23  22  21 20  16 15 14 13  10    9    5 4     0
 * +---+---+---+-----------+------+---+------+-----+---------+------+-------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | op  | 1 0 0 0 |  Rn  |  op2  |
 * +---+---+---+-----------+------+---+------+-----+---------+------+-------+
 */
arminst disas_fp_compare(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Floating point data-processing (2 source)
 *   31  30  29 28       24 23  22  21 20  16 15    12 11 10 9    5 4    0
 * +---+---+---+-----------+------+---+------+--------+-----+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | opcode | 1 0 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+------+--------+-----+------+------+
 */
arminst disas_fp_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}



/* Floating point data-processing (3 source)
 *   31  30  29 28       24 23  22  21  20  16  15  14  10 9    5 4    0
 * +---+---+---+-----------+------+----+------+----+------+------+------+
 * | M | 0 | S | 1 1 1 1 1 | type | o1 |  Rm  | o0 |  Ra  |  Rn  |  Rd  |
 * +---+---+---+-----------+------+----+------+----+------+------+------+
 */
arminst disas_fp_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo )
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Floating point immediate
 *   31  30  29 28       24 23  22  21 20        13 12   10 9    5 4    0
 * +---+---+---+-----------+------+---+------------+-------+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |    imm8    | 1 0 0 | imm5 |  Rd  |
 * +---+---+---+-----------+------+---+------------+-------+------+------+
 */
arminst disas_fp_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

}


/* FP-specific subcases of table C3-6 (SIMD and FP data processing)
 *   31  30  29 28     25 24                          0
 * +---+---+---+---------+-----------------------------+
 * |   | 0 |   | 1 1 1 1 |                             |
 * +---+---+---+---------+-----------------------------+
 */
arminst disas_data_proc_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (extract32(aFetchedOpcode.thePC, 24, 1)) {
        /* Floating point data-processing (3 source) */
        return disas_fp_3src(aFetchedOpcode, aCPU, aSequenceNo);
    } else if (!extract32(aFetchedOpcode.thePC, 21, 1)) {
        /* Floating point to fixed point conversions */
        return disas_fp_fixed_conv(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        switch (extract32(aFetchedOpcode.thePC, 10, 2)) {
        case 1:
            /* Floating point conditional compare */
            return disas_fp_ccomp(aFetchedOpcode, aCPU, aSequenceNo);
        case 2:
            /* Floating point data-processing (2 source) */
            return disas_fp_2src(aFetchedOpcode, aCPU, aSequenceNo);
        case 3:
            /* Floating point conditional select */
            return disas_fp_csel(aFetchedOpcode, aCPU, aSequenceNo);
        case 0:
            switch (ctz32(extract32(aFetchedOpcode.thePC, 12, 4))) {
            case 0: /* [15:12] == xxx1 */
                /* Floating point immediate */
                return disas_fp_imm(aFetchedOpcode, aCPU, aSequenceNo);
            case 1: /* [15:12] == xx10 */
                /* Floating point compare */
                return disas_fp_compare(aFetchedOpcode, aCPU, aSequenceNo);
            case 2: /* [15:12] == x100 */
                /* Floating point data-processing (1 source) */
                return disas_fp_1src(aFetchedOpcode, aCPU, aSequenceNo);
            case 3: /* [15:12] == 1000 */
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            default: /* [15:12] == 0000 */
                /* Floating point <-> integer conversions */
                return disas_fp_int_conv(aFetchedOpcode, aCPU, aSequenceNo);
            }
        default:
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
    }
}

arminst disas_data_proc_simd(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

/* C3.6 Data processing - SIMD and floating point */
arminst disas_data_proc_simd_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (extract32(aFetchedOpcode.thePC, 28, 1) == 1 && extract32(aFetchedOpcode.thePC, 30, 1) == 0) {
        return disas_data_proc_fp(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        /* SIMD, including crypto */
        return disas_data_proc_simd(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

} // narmDecoder
