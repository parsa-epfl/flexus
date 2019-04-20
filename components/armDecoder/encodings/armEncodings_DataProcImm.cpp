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
#include "armDataProcImm.hpp"
#include "armDataProcReg.hpp"
#include "armBranch.hpp"
#include "armLoadStore.hpp"
#include "armUnallocated.hpp"
#include "armMagic.hpp"

namespace narmDecoder {


/* PC-rel. addressing
 *   31  30   29 28       24 23                5 4    0
 * +----+-------+-----------+-------------------+------+
 * | op | immlo | 1 0 0 0 0 |       immhi       |  Rd  |
 * +----+-------+-----------+-------------------+------+
 */
arminst disas_pc_rel_adr(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return ADR(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Logical (immediate)
 *   31  30 29 28         23 22  21  16 15  10 9    5 4    0
 * +----+-----+-------------+---+------+------+------+------+
 * | sf | opc | 1 0 0 1 0 0 | N | immr | imms |  Rn  |  Rd  |
 * +----+-----+-------------+---+------+------+------+------+
 */
arminst disas_logic_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return LOGICALIMM(aFetchedOpcode, aCPU, aSequenceNo);
}

/*
 * Move wide (immediate)
 *
 *  31 30 29 28         23 22 21 20             5 4    0
 * +--+-----+-------------+-----+----------------+------+
     * |sf| opc | 1 0 0 1 0 1 |  hw |  imm16         |  Rd  |
 * +--+-----+-------------+-----+----------------+------+
 *
 * sf: 0 -> 32 bit, 1 -> 64 bit
 * opc: 00 -> N, 10 -> Z, 11 -> K
 * hw: shift/16 (0,16, and sf only 32, 48)
 */
arminst disas_movw_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return MOVE(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Bitfield
 *   31  30 29 28         23 22  21  16 15  10 9    5 4    0
 * +----+-----+-------------+---+------+------+------+------+
 * | sf | opc | 1 0 0 1 1 0 | N | immr | imms |  Rn  |  Rd  |
 * +----+-----+-------------+---+------+------+------+------+
 */
arminst disas_bitfield(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return BFM(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Extract
 *   31  30  29 28         23 22   21  20  16 15    10 9    5 4    0
 * +----+------+-------------+---+----+------+--------+------+------+
 * | sf | op21 | 1 0 0 1 1 1 | N | o0 |  Rm  |  imms  |  Rn  |  Rd  |
 * +----+------+-------------+---+----+------+--------+------+------+
 */
arminst disas_extract(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return EXTR(aFetchedOpcode, aCPU, aSequenceNo);
}

/*
 * Add/subtract (immediate)
 *
 *  31 30 29 28       24 23 22 21         10 9   5 4   0
 * +--+--+--+-----------+-----+-------------+-----+-----+
 * |sf|op| S| 1 0 0 0 1 |shift|    imm12    |  Rn | Rd  |
 * +--+--+--+-----------+-----+-------------+-----+-----+
 *
 *    sf: 0 -> 32bit, 1 -> 64bit
 *    op: 0 -> add  , 1 -> sub
 *     S: 1 -> set flags
 * shift: 00 -> LSL imm by 0, 01 -> LSL imm by 12
 */
arminst disas_add_sub_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return ALUIMM(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Data processing - immediate */
arminst disas_data_proc_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 23, 6)) {
    case 0x20: case 0x21: /* PC-rel. addressing */
        return disas_pc_rel_adr(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x22: case 0x23: /* Add/subtract (immediate) */
        return disas_add_sub_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x24: /* Logical (immediate) */
        return disas_logic_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x25: /* Move wide (immediate) */
        return disas_movw_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x26: /* Bitfield */
        return disas_bitfield(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x27: /* Extract */
        return disas_extract(aFetchedOpcode,  aCPU, aSequenceNo);
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    DBG_Assert(false);
}

} // narmDecoder
