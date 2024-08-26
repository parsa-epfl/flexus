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

#include "Branch.hpp"
#include "Encodings.hpp"
#include "Unallocated.hpp"

namespace nDecoder {

/* Unconditional branch (register)
 *  31           25 24   21 20   16 15   10 9    5 4     0
 * +---------------+-------+-------+-------+------+-------+
 * | 1 1 0 1 0 1 1 |  opc  |  op2  |  op3  |  Rn  |  op4  |
 * +---------------+-------+-------+-------+------+-------+
 */
archinst
disas_uncond_b_reg(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 21, 4);
    uint32_t op2 = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t op3 = extract32(aFetchedOpcode.theOpcode, 10, 6);
    uint32_t op4 = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (op4 != 0x0 || op3 != 0x0 || op2 != 0x1f) { return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); }

    switch (opc) {
        case 0: /* BR */
        case 1: /* BLR */
        case 2: /* RET */ DECODER_TRACE; return BLR(aFetchedOpcode, aCPU, aSequenceNo);
        case 4: /* ERET */ return ERET(aFetchedOpcode, aCPU, aSequenceNo);

        case 5: /* DRPS */ return DPRS(aFetchedOpcode, aCPU, aSequenceNo);

        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

/* Exception generation
 *
 *  31             24 23 21 20                     5 4   2 1  0
 * +-----------------+-----+------------------------+-----+----+
 * | 1 1 0 1 0 1 0 0 | opc |          imm16         | op2 | LL |
 * +-----------------------+------------------------+----------+
 */
archinst
disas_exc(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t ll  = extract32(aFetchedOpcode.theOpcode, 0, 2);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 21, 3) << 2;

    uint32_t res = opc | ll;

    switch (res) {
        case 0x1: return SVC(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x2: return HVC(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x3: return SMC(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x4: return BRK(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x5: return HLT(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x15:
        case 0x16: return DCPS(aFetchedOpcode, aCPU, aSequenceNo);
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); break;
    }
}

/* System
 *  31                 22 21  20 19 18 16 15   12 11    8 7   5 4    0
 * +---------------------+---+-----+-----+-------+-------+-----+------+
 * | 1 1 0 1 0 1 0 1 0 0 | L | op0 | op1 |  CRn  |  CRm  | op2 |  Rt  |
 * +---------------------+---+-----+-----+-------+-------+-----+------+
 */
archinst
disas_system(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    bool l       = extract32(aFetchedOpcode.theOpcode, 21, 1);
    uint32_t op0 = extract32(aFetchedOpcode.theOpcode, 19, 2);
    uint32_t crn = extract32(aFetchedOpcode.theOpcode, 12, 4);
    uint32_t rt  = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (op0 == 0) {
        if (l || rt != 31) { return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); }
        switch (crn) {
            case 2: /* HINT (including allocated hints like NOP, YIELD, etc) */
                return HINT(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 3: /* CLREX, DSB, DMB, ISB */ return SYNC(aFetchedOpcode, aCPU, aSequenceNo); break;
            case 4: /* MSR (immediate) */ return MSR(aFetchedOpcode, aCPU, aSequenceNo); break;
            default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
    }

    /* MRS - move from system register
     * MSR (register) - move to system register
     * SYS
     * SYSL
     * These are all essentially the same insn in 'read' and 'write'
     * versions, with varying op0 fields.
     */
    return SYS(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Conditional branch (immediate)
 *  31           25  24  23                  5   4  3    0
 * +---------------+----+---------------------+----+------+
 * | 0 1 0 1 0 1 0 | o1 |         imm19       | o0 | cond |
 * +---------------+----+---------------------+----+------+
 */
archinst
disas_cond_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    if ((aFetchedOpcode.theOpcode & (1 << 4)) || (aFetchedOpcode.theOpcode & (1 << 24))) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    return CONDBR(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Test and branch (immediate)
 *   31  30         25  24  23   19 18          5 4    0
 * +----+-------------+----+-------+-------------+------+
 * | b5 | 0 1 1 0 1 1 | op |  b40  |    imm14    |  Rt  |
 * +----+-------------+----+-------+-------------+------+
 */
archinst
disas_test_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return TSTBR(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Compare and branch (immediate)
 *   31  30         25  24  23                  5 4      0
 * +----+-------------+----+---------------------+--------+
 * | sf | 0 1 1 0 1 0 | op |         imm19       |   Rt   |
 * +----+-------------+----+---------------------+--------+
 */
archinst
disas_comp_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return CMPBR(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Unconditional branch (immediate)
 *   31  30       26 25                                  0
 * +----+-----------+-------------------------------------+
 * | op | 0 0 1 0 1 |                 imm26               |
 * +----+-----------+-------------------------------------+
 */
archinst
disas_uncond_b_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return UNCONDBR(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Branches, exception generating and system instructions

 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3
2 1 0
+--------+--------+-----------+------------------------------------------------------
|  op0   |   101  |    op1    |
+--------+--------+-----------+-------------------------------------------------------


+--------+-------+
|  op0   |  op1  |
+--------+-------+
|  010   | 0xxx  | Conditional branch (immediate)
|  010   | 1xxx  | UNALLOCATED
|  110   | 00xx  | Exception generation
|  110   | 0100  | System
|  110   | 0101  | UNALLOCATED
|  110   | 011x  | UNALLOCATED
|  110   | 1xxx  | Unconditional branch (register)
|  x00   |       | Unconditional branch (Immediate)
|  x01   | 0xxx  | Compare and branch (immediate)
|  x01   | 1xxx  | Test and branch (immediate)
|  x11   |       | UNALLOCATED

*/

archinst
disas_b_exc_sys(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    switch (extract32(aFetchedOpcode.theOpcode, 25, 7)) {
        case 0x0a:
        case 0x0b:
        case 0x4a:
        case 0x4b: /* Unconditional branch (immediate) */ return disas_uncond_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1a:
        case 0x5a: /* Compare & branch (immediate) */ return disas_comp_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1b:
        case 0x5b: /* Test & branch (immediate) */ return disas_test_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x2a: /* Conditional branch (immediate) */ return disas_cond_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x6a: /* Exception generation / System */
            if (aFetchedOpcode.theOpcode & (1 << 24)) {
                return disas_system(aFetchedOpcode, aCPU, aSequenceNo);
            } else {
                return disas_exc(aFetchedOpcode, aCPU, aSequenceNo);
            }
            break;
        case 0x6b: /* Unconditional branch (register) */ return disas_uncond_b_reg(aFetchedOpcode, aCPU, aSequenceNo);
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

} // namespace nDecoder