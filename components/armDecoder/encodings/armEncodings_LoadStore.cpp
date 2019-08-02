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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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
#include "armMagic.hpp"
#include "armUnallocated.hpp"

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
arminst disas_ldst_single_struct(armcode const &aFetchedOpcode, uint32_t aCPU,
                                 int64_t aSequenceNo) {
  DECODER_TRACE;
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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
arminst disas_ldst_multiple_struct(armcode const &aFetchedOpcode, uint32_t aCPU,
                                   int64_t aSequenceNo) {
  DECODER_TRACE;
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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
arminst disas_ldst_reg_imm9(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
  bool V = extract32(aFetchedOpcode.theOpcode, 26, 1);
  uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
  bool is_store = (opc == 0);

  if (size == 3 && opc == 2) {
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
  }
  if (opc == 3 && size > 1) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  if (V) {
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    //         if (is_store){
    //             return STRF(aFetchedOpcode, aCPU, aSequenceNo);
    //         } else {
    //             return LDRF(aFetchedOpcode, aCPU, aSequenceNo);
    //         }
  } else {
    if (is_store) {
      return STR(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return LDR(aFetchedOpcode, aCPU, aSequenceNo);
    }
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
arminst disas_ldst_reg_roffset(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
  uint32_t option = extract32(aFetchedOpcode.theOpcode, 13, 3);
  bool V = extract32(aFetchedOpcode.theOpcode, 26, 1);
  uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
  bool is_store = (opc == 0);

  if (extract32(option, 1, 1) == 0) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  if (!V && opc == 3 && size > 1) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  if (V) {
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    //        if (is_store){
    //            return STRF(aFetchedOpcode, aCPU, aSequenceNo);
    //        } else {
    //            DBG_Assert(false);
    //            return unallocated_encoding(aFetchedOpcode, aCPU,
    //            aSequenceNo);
    //        }
  } else {
    if (is_store) {
      return STR(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return LDR(aFetchedOpcode, aCPU, aSequenceNo);
    }
  }
}

/* Atomic memory operations
 *
 *  31  30      27  26    24    22  21   16   15    12    10    5     0
 * +------+-------+---+-----+-----+---+----+----+-----+-----+----+-----+
 * | size | 1 1 1 | V | 0 0 | A R | 1 | Rs | o3 | opc | 0 0 | Rn |  Rt |
 * +------+-------+---+-----+-----+--------+----+-----+-----+----+-----+
 *
 * Rt: the result register
 * Rn: base address or SP
 * Rs: the source register for the operation
 * V: vector flag (always 0 as of v8.3)
 * A: acquire flag
 * R: release flag
 */
arminst disas_ldst_atomic(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {

  // QEMU doesnt model these and nor they get invoked when running flexus...
  // will add them as soon as they're needed.
  DECODER_TRACE;
  uint32_t o3_opc = extract32(aFetchedOpcode.theOpcode, 12, 4);
  bool V = extract32(aFetchedOpcode.theOpcode, 26, 1);
  if (V) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
  switch (o3_opc) {
  case 000: /* LDADD */
    return LDADD(aFetchedOpcode, aCPU, aSequenceNo);
  case 001: /* LDCLR */
    return LDCLR(aFetchedOpcode, aCPU, aSequenceNo);
  case 002: /* LDEOR */
    return LDEOR(aFetchedOpcode, aCPU, aSequenceNo);
  case 003: /* LDSET */
    return LDSET(aFetchedOpcode, aCPU, aSequenceNo);
  case 004: /* LDSMAX */
    return LDSMAX(aFetchedOpcode, aCPU, aSequenceNo);
  case 005: /* LDSMIN */
    return LDSMIN(aFetchedOpcode, aCPU, aSequenceNo);
  case 006: /* LDUMAX */
    return LDUMAX(aFetchedOpcode, aCPU, aSequenceNo);
  case 007: /* LDUMIN */
    return LDUMIN(aFetchedOpcode, aCPU, aSequenceNo);
  case 010: /* SWP */
    return SWP(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
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
arminst disas_ldst_reg_unsigned_imm(armcode const &aFetchedOpcode, uint32_t aCPU,
                                    int64_t aSequenceNo) {
  DECODER_TRACE;
  uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
  bool V = extract32(aFetchedOpcode.theOpcode, 26, 1);
  uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
  bool is_store = ((opc == 0) || ((opc == 2) && (size == 0)));

  if (size == 3 && opc == 2) {
    return nop(aFetchedOpcode, aCPU, aSequenceNo); // PRFM
  }
  if (opc == 3 && size > 1) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  if (V) {
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    //        if (is_store){
    //            return STRF(aFetchedOpcode, aCPU, aSequenceNo);
    //        } else {
    //            return LDRF(aFetchedOpcode, aCPU, aSequenceNo);
    //        }
  } else {
    if (is_store) {
      return STR(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return LDR(aFetchedOpcode, aCPU, aSequenceNo);
    }
  }
}

/* Load/store register (all forms) */
arminst disas_ldst_reg(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  switch (extract32(aFetchedOpcode.theOpcode, 24, 2)) {
  case 0:
    if (extract32(aFetchedOpcode.theOpcode, 21, 1) == 1 &&
        extract32(aFetchedOpcode.theOpcode, 10, 2) == 2) {
      return disas_ldst_reg_roffset(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      /* Load/store register (unscaled immediate)
       * Load/store immediate pre/post-indexed
       * Load/store register unprivileged
       */
      return disas_ldst_reg_imm9(aFetchedOpcode, aCPU, aSequenceNo);
    }
  case 1:
    return disas_ldst_reg_unsigned_imm(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
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
arminst disas_ldst_pair(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
  bool is_load = extract32(aFetchedOpcode.theOpcode, 22, 1);

  if (extract32(aFetchedOpcode.theOpcode, 30, 2) == 3) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  if (is_vector) {
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    //        if (is_load) {
    //            return LDFP(aFetchedOpcode, aCPU, aSequenceNo);
    //        } else {
    //            return STFP(aFetchedOpcode, aCPU, aSequenceNo);
    //        }
  } else {
    if (is_load) {
      return LDP(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return STP(aFetchedOpcode, aCPU, aSequenceNo);
    }
  }
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
arminst disas_ld_lit(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  bool V = extract32(aFetchedOpcode.theOpcode, 26, 1);
  uint32_t opc = extract32(aFetchedOpcode.theOpcode, 30, 2);

  switch (V + (opc << 1)) {
  case 0:
  case 2:
  case 4:
  case 6:
    return LDR_lit(aFetchedOpcode, aCPU, aSequenceNo);
    //    case 1: case 3: case 5:
    //        return LDRF_lit(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
  }
}

/* Load/storeexclusive
 * 31 30             24 23 22 21 20    16 15       10     5    0 0
 * +----+-------------+---+--+--+--------+--+--------+-----+----+
 * |size| 0 0 1 0 0 0 | o2|L |o1|  Rs    |o0|  Rt2   |  Rn | Rt |
 * +----+-------------+---+--+--+--------+--+--------+-----+----+
 */
arminst disas_ldst_excl(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
  bool o0 = extract32(aFetchedOpcode.theOpcode, 15, 1); // is_lasr
  bool o1 = extract32(aFetchedOpcode.theOpcode, 21, 1); // is_pair
  bool L = extract32(aFetchedOpcode.theOpcode, 22, 1);  // is_store
  bool o2 = extract32(aFetchedOpcode.theOpcode, 23, 1); // is_excl

  if (((o2 && o1) || (!o2 && o1)) && rt2 != 31) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
  unsigned int decision = o0 | (o1 << 1) | (L << 2) | (o2 << 3);

  switch (decision) {
  case 0:
  case 1:
    return STXR(aFetchedOpcode, aCPU, aSequenceNo);
    break;
  case 2:
  case 3:
  case 6:
  case 7:
  case 10:
  case 11:
  case 14:
  case 15:
    return CAS(aFetchedOpcode, aCPU, aSequenceNo);
    break;
  case 4:
  case 5:
    return LDXR(aFetchedOpcode, aCPU, aSequenceNo);
    break;
  case 8:
  case 9:
    return STRL(aFetchedOpcode, aCPU, aSequenceNo);
    break;
  case 12:
  case 13:
    return LDAQ(aFetchedOpcode, aCPU, aSequenceNo);
    break;
  default:
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
}

/* Loads and stores */
arminst disas_ldst(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  switch (extract32(aFetchedOpcode.theOpcode, 24, 6)) {
  case 0x08: /* Load/store exclusive */
    return disas_ldst_excl(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x18:
  case 0x1c: /* Load register (literal) */
    return disas_ld_lit(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x28:
  case 0x29:
  case 0x2c:
  case 0x2d: /* Load/store pair (all forms) */
    return disas_ldst_pair(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x38:
  case 0x39:
  case 0x3c:
  case 0x3d: /* Load/store register (all forms) */
    return disas_ldst_reg(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x0c: /* AdvSIMD load/store multiple structures */
    return disas_ldst_multiple_struct(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x0d: /* AdvSIMD load/store single structure */
    return disas_ldst_single_struct(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
}

} // namespace narmDecoder
