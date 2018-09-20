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

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU,aSequenceNo));


    inst->setClass(clsBranch, codeBranchUnconditional);
    uint32_t  rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint64_t offset = sextract64(aFetchedOpcode.theOpcode, 5, 19);
    offset = offset << 2 | extract32(aFetchedOpcode.theOpcode, 29, 2);
    uint32_t op = extract32(aFetchedOpcode.theOpcode, 31, 1);
    uint64_t base = aFetchedOpcode.thePC;


    if (op) {
        /* ADRP (page based) */
        base &= ~0xfff;
        offset <<= 12;
    }

    ADR(inst, base, offset, rd);

    return inst;
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

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    uint32_t sf, opc, is_n, immr, imms, rn, rd;
    uint64_t wmask;
    bool is_and = false;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    is_n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    immr = extract32(aFetchedOpcode.theOpcode, 16, 6);
    imms = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (!logic_imm_decode_wmask(&wmask, is_n, imms, immr)) {
        /* some immediate field values are reserved */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (!sf) {
        wmask &= 0xffffffff;
    }



    switch (opc) {
    case 0x3: /* ANDS */
        AND(inst, rd, rn, wmask, sf, true);
    case 0x0: /* AND */
        //    if sf == '0' && N != '0' then ReservedValue();
        //    ReservedValue()
        //    if UsingAArch32() && !AArch32.GeneralExceptionsToAArch64() then
        //    AArch32.TakeUndefInstrException();
        //    else
        //    AArch64.UndefinedFault();
            if (sf == 0 && is_n != 0 ){
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }

         AND(inst, rd, rn, wmask, sf, false);
         break;
    case 0x1: /* ORR */
        ORR(inst, rd, rn, wmask, sf, false);
        break;
    case 0x2: /* EOR */
        XOR(inst, rd, rn, wmask, sf, false);
        break;
    default:
        DBG_Assert(false); /* must handle all above */
        break;
    }

    return arminst(inst);
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

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint64_t imm = extract32(aFetchedOpcode.theOpcode, 5, 16);
    int sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    int opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    int pos = extract32(aFetchedOpcode.theOpcode, 21, 2) << 4;

    if (!sf && (pos >= 32)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    switch (opc) {
    case 0: /* MOVN */
    case 2: /* MOVZ */
        imm <<= pos;
        if (opc == 0) { // MOVN
            imm = ~imm;
        }
        if (!sf) {
            imm &= 0xffffffffu;
        }
//        MOV(inst, rd, rn, imm, sf, (opc == 0) ? false : true);
        break;
    case 3: /* MOVK */
        MOVK(inst, rd, rd, imm ,sf);
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }

    return arminst(inst);
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

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint32_t sf, n, opc, ri, si, rn, rd, bitsize, pos, len;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    ri = extract32(aFetchedOpcode.theOpcode, 16, 6); // immr
    si = extract32(aFetchedOpcode.theOpcode, 10, 6); // imms
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    bitsize = sf ? 64 : 32;

    if (!sf && n != 1){
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); // ReservedValue()
    }
    if (!sf && n != 0 || ri != 0 || si != 0){
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); // ReservedValue()
    }


    switch (opc) {
    case 0:
        SBFM(inst, rd, rd, si, ri, sf);
        break;
    case 1:
        break;
    case 2:
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

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

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU,aSequenceNo));

    uint32_t sf, n, rm, imm, rn, rd, bitsize, op21, op0;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    imm = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    op21 = extract32(aFetchedOpcode.theOpcode, 29, 2);
    op0 = extract32(aFetchedOpcode.theOpcode, 21, 1);
    bitsize = sf ? 64 : 32;

    if (sf != n || op21 || op0 || imm >= bitsize) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        EXTR(inst, rd, rn, rm, imm, sf);
    }

    return arminst(inst);
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

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU,aSequenceNo));
    uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint64_t imm = extract32(aFetchedOpcode.theOpcode, 10, 12);
    int shift = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool S = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);


    switch (shift) {
    case 0x0:
        break;
    case 0x1:
        imm <<= 12;
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }


    if (op) // sub
    {
        SUB(inst, rd, rn, imm, sf, S);
    }
    else { // add
        ADD(inst, rd, rn, imm, sf, S);

    }

    return arminst(inst);
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
