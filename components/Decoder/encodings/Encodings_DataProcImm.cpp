
#include "Branch.hpp"
#include "DataProcImm.hpp"
#include "DataProcReg.hpp"
#include "Encodings.hpp"
#include "LoadStore.hpp"
#include "Unallocated.hpp"

namespace nDecoder {

/* PC-rel. addressing
 *   31  30   29 28       24 23                5 4    0
 * +----+-------+-----------+-------------------+------+
 * | op | immlo | 1 0 0 0 0 |       immhi       |  Rd  |
 * +----+-------+-----------+-------------------+------+
 */
archinst
disas_pc_rel_adr(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
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
archinst
disas_logic_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
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
archinst
disas_movw_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
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
archinst
disas_bitfield(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
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
archinst
disas_extract(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
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
archinst
disas_add_sub_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return ALUIMM(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Data processing - immediate */
archinst
disas_data_proc_imm(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 23, 6)) {
        case 0x20:
        case 0x21: /* PC-rel. addressing */ return disas_pc_rel_adr(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x22:
        case 0x23: /* Add/subtract (immediate) */ return disas_add_sub_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x24: /* Logical (immediate) */ return disas_logic_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x25: /* Move wide (immediate) */ return disas_movw_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x26: /* Bitfield */ return disas_bitfield(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x27: /* Extract */ return disas_extract(aFetchedOpcode, aCPU, aSequenceNo);
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    DBG_Assert(false);
}

} // namespace nDecoder
