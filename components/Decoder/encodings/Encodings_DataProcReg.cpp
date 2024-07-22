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

#include "DataProcReg.hpp"
#include "Encodings.hpp"
#include "SharedFunctions.hpp"
#include "Unallocated.hpp"

namespace nDecoder {

/* Data-processing (2 source)
 *   31   30  29 28             21 20  16 15    10 9    5 4    0
 * +----+---+---+-----------------+------+--------+------+------+
 * | sf | 0 | S | 1 1 0 1 0 1 1 0 |  Rm  | opcode |  Rn  |  Rd  |
 * +----+---+---+-----------------+------+--------+------+------+
 */
archinst disas_data_proc_2src(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  if (extract32(aFetchedOpcode.theOpcode, 29, 1)) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  switch (extract32(aFetchedOpcode.theOpcode, 10, 6)) {
  case 2: /* UDIV */
  case 3: /* SDIV */
    return DIV(aFetchedOpcode, aCPU, aSequenceNo);

  case 8:  /* LSLV */
  case 9:  /* LSRV */
  case 10: /* ASRV */
  case 11: /* RORV */
    return SHIFT(aFetchedOpcode, aCPU, aSequenceNo);

  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
  case 21:
  case 22:
  case 23: /* CRC32 */
    return CRC(aFetchedOpcode, aCPU, aSequenceNo);
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
archinst disas_data_proc_1src(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 16, 5)) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  switch (extract32(aFetchedOpcode.theOpcode, 10, 6)) {
  case 0: /* RBIT */
    return RBIT(aFetchedOpcode, aCPU, aSequenceNo);
  case 1: /* REV16 */
  case 2: /* REV32 */
  case 3: /* REV64 */
    return REV(aFetchedOpcode, aCPU, aSequenceNo);
  case 4: /* CLZ */
  case 5: /* CLS */
    return CL(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    DBG_Assert(false);
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
}

/* Conditional select
 *   31   30  29  28             21 20  16 15  12 11 10 9    5 4    0
 * +----+----+---+-----------------+------+------+-----+------+------+
 * | sf | op | S | 1 1 0 1 0 1 0 0 |  Rm  | cond | op2 |  Rn  |  Rd  |
 * +----+----+---+-----------------+------+------+-----+------+------+
 */
archinst disas_cond_select(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return CSEL(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Conditional compare (immediate / register)
 *  31 30 29 28 27 26 25 24 23 22 21  20    16 15  12  11  10  9   5  4 3   0
 * +--+--+--+------------------------+--------+------+----+--+------+--+-----+
 * |sf|op| S| 1  1  0  1  0  0  1  0 |imm5/rm | cond |i/r |o2|  Rn  |o3|nzcv |
 * +--+--+--+------------------------+--------+------+----+--+------+--+-----+
 *        [1]                             y                [0]       [0]
 */
archinst disas_cc(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return CCMP(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Add/subtract (with carry)
 *  31 30 29 28 27 26 25 24 23 22 21  20  16  15   10  9    5 4   0
 * +--+--+--+------------------------+------+---------+------+-----+
 * |sf|op| S| 1  1  0  1  0  0  0  0 |  rm  | opcode2 |  Rn  |  Rd |
 * +--+--+--+------------------------+------+---------+------+-----+
 *                                            [000000]
 */
archinst disas_adc_sbc(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return ADDSUB_CARRY(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Data-processing (3 source)
 *
 *    31 30  29 28       24 23 21  20  16  15  14  10 9    5 4    0
 *  +--+------+-----------+------+------+----+------+------+------+
 *  |sf| op54 | 1 1 0 1 1 | op31 |  Rm  | o0 |  Ra  |  Rn  |  Rd  |
 *  +--+------+-----------+------+------+----+------+------+------+
 */
archinst disas_data_proc_3src(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return DP_3_SRC(aFetchedOpcode, aCPU, aSequenceNo);
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
archinst disas_add_sub_reg(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return ADDSUB_SHIFTED(aFetchedOpcode, aCPU, aSequenceNo);
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
archinst disas_add_sub_ext_reg(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  return ADDSUB_EXTENDED(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Logical (shifted register)
 *   31  30 29 28       24 23   22 21  20  16 15    10 9    5 4    0
 * +----+-----+-----------+-------+---+------+--------+------+------+
 * | sf | opc | 0 1 0 1 0 | shift | N |  Rm  |  imm6  |  Rn  |  Rd  |
 * +----+-----+-----------+-------+---+------+--------+------+------+
 */
archinst disas_logic_reg(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  return LOGICAL(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Data processing - register */
archinst disas_data_proc_reg(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  switch (extract32(aFetchedOpcode.theOpcode, 24, 5)) {
  case 0x0a: /* Logical (shifted register) */
    return disas_logic_reg(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x0b:                                    /* Add/subtract */
    if (aFetchedOpcode.theOpcode & (1 << 21)) { /* (extended register) */
      return disas_add_sub_ext_reg(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return disas_add_sub_reg(aFetchedOpcode, aCPU, aSequenceNo);
    }
  case 0x1b: /* Data-processing (3 source) */
    return disas_data_proc_3src(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x1a:
    switch (extract32(aFetchedOpcode.theOpcode, 21, 3)) {
    case 0x0: /* Add/subtract (with carry) */
      return disas_adc_sbc(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x2:                                             /* Conditional compare */
      return disas_cc(aFetchedOpcode, aCPU, aSequenceNo); /* both imm and reg forms */
    case 0x4:                                             /* Conditional select */
      return disas_cond_select(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x6:                                     /* Data-processing */
      if (aFetchedOpcode.theOpcode & (1 << 30)) { /* (1 source) */
        return disas_data_proc_1src(aFetchedOpcode, aCPU, aSequenceNo);
      } else { /* (2 source) */
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

} // namespace nDecoder