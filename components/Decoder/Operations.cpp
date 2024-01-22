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

#include <components/uArch/uArchInterfaces.hpp>

#include "SemanticActions.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

typedef struct OFFSET : public Operation {
  OFFSET() {
  }
  virtual ~OFFSET() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    if (theOperands.size()) {
      DBG_Assert(theOperands.size() == 1 || theOperands.size() == 2, (<< *theInstruction));
      uint64_t op1 = boost::get<uint64_t>(theOperands[0]);
      uint64_t op2 = boost::get<uint64_t>(theOperands[1]);
      return op1 + op2;
    } else {
      DBG_Assert(operands.size() == 2);
      uint64_t op1 = boost::get<uint64_t>(operands[0]);
      uint64_t op2 = boost::get<uint64_t>(operands[1]);
      return op1 + op2;
    }
  }
  virtual char const *describe() const {
    return "Add";
  }
} OFFSET_;

typedef struct ADD : public Operation {
  ADD() {
  }
  virtual ~ADD() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    std::vector<Operand> finalOperands = theOperands.size() ? theOperands : operands;
    SEMANTICS_DBG("ADDING with " << finalOperands.size());
    DBG_Assert(finalOperands.size() == 1 || finalOperands.size() == 2,
               (<< *theInstruction << "num operands: " << finalOperands.size()));

    if (finalOperands.size() > 1) {
      return boost::get<uint64_t>(finalOperands[0]) + boost::get<uint64_t>(finalOperands[1]);
    } else {
      return boost::get<uint64_t>(finalOperands[0]);
    }
  }
  virtual char const *describe() const {
    return "Add";
  }
} ADD_;

typedef struct SUB : public Operation {
  SUB() {
  }
  virtual ~SUB() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "SUB";
  }
} SUB_;

typedef struct CONCAT32 : public Operation {
  CONCAT32() {
  }
  virtual ~CONCAT32() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);

    the128 = true;

    op1 &= 0xffffffff;
    op2 &= 0xffffffff;
    return bits((op1 << 32) | op2);
  }
  virtual char const *describe() const {
    return "CONCAT32";
  }
} CONCAT32_;

typedef struct CONCAT64 : public Operation {
  CONCAT64() {
  }
  virtual ~CONCAT64() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);

    the128 = true;
    return concat_bits((bits)op1, (bits)op2);
  }
  virtual char const *describe() const {
    return "CONCAT64";
  }
} CONCAT64_;

typedef struct AND : public Operation {
  AND() {
  }
  virtual ~AND() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    DBG_(Iface, (<< "ANDING " << std::hex << boost::get<uint64_t>(operands[0]) << " & "
                 << boost::get<uint64_t>(operands[1]) << std::dec));
    return (boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1]));
  }
  virtual char const *describe() const {
    return "AND";
  }
} AND_;

typedef struct ORR : public Operation {
  ORR() {
  }
  virtual ~ORR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) | boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "ORR";
  }
} ORR_;

typedef struct XOR : public Operation {
  XOR() {
  }
  virtual ~XOR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) ^ boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "XOR";
  }
} XOR_;

typedef struct AndN : public Operation {

  AndN() {
  }
  virtual ~AndN() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) & ~boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "AndN";
  }
} AndN_;

typedef struct EoN : public Operation {
  EoN() {
  }
  virtual ~EoN() {
  }

  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) | ~boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "EoN";
  }
} EoN_;

typedef struct OrN : public Operation {
  OrN() {
  }
  virtual ~OrN() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) | ~boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "OrN";
  }
} OrN_;

typedef struct Not : public Operation {
  Not() {
  }
  virtual ~Not() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]);
  }
  virtual char const *describe() const {
    return "Invert";
  }
} Not_;

typedef struct ROR : public Operation {
  ROR() {
  }
  virtual ~ROR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return boost::get<uint64_t>(operands[0]);
    // HEHE
    /*
    DBG_Assert(operands.size() == 3);
    uint64_t input = boost::get<uint64_t>(operands[0]);
    uint64_t input_size = boost::get<uint64_t>(operands[2]);

    uint64_t shift_size = (input_size == 32) ? boost::get<uint64_t>(operands[1]) & ((32 - 1))
                                             : boost::get<uint64_t>(operands[1]) & ((64 - 1));
    return ror((uint64_t)input, (uint64_t)input_size, (uint64_t)shift_size);
    */
  }
  virtual char const *describe() const {
    return "ROR";
  }
} ROR_;

typedef struct LSL : public Operation {
  LSL() {
  }
  virtual ~LSL() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 3);
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    uint64_t data_size = boost::get<uint64_t>(operands[2]);

    uint64_t mask = (data_size == 32) ? 0xffffffff : 0xffffffffffffffff;
    return (op1 << (op2 % data_size) & mask);
  }
  virtual char const *describe() const {
    return "LSL";
  }
} LSL_;

typedef struct ASR : public Operation {
  ASR() {
  }
  virtual ~ASR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 3);
    return boost::get<uint64_t>(operands[0]);
    // HEHE
    /*
    uint64_t input = boost::get<uint64_t>(operands[0]);
    uint64_t shift_size = boost::get<uint64_t>(operands[1]);
    uint64_t input_size = boost::get<uint64_t>(operands[2]);

    return asr((uint64_t)input, (uint64_t)input_size, (uint64_t)shift_size);
    */
  }
  virtual char const *describe() const {
    return "ASR";
  }
} ASR_;

typedef struct LSR : public Operation {
  LSR() {
  }
  virtual ~LSR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 3);
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    uint64_t data_size = boost::get<uint64_t>(operands[2]);

    uint64_t mask = (data_size == 32) ? 0xffffffff : 0xffffffffffffffff;
    return (op1 >> (op2 % data_size) & mask);
  }
  virtual char const *describe() const {
    return "LSR";
  }
} LSR_;

typedef struct SextB : public Operation {
  SextB() {
  }
  virtual ~SextB() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    uint64_t op1 = (uint64_t)(boost::get<uint64_t>(operands[0]));
    if (op1 & (1 << (8 - 1)))
      op1 |= SIGNED_UPPER_BOUND_B;
    return op1;
  }
  virtual char const *describe() const {
    return "signed extend Byte";
  }
} SextB_;

typedef struct SextH : public Operation {
  SextH() {
  }
  virtual ~SextH() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    uint64_t op1 = (uint64_t)(boost::get<uint64_t>(operands[0]));
    if (op1 & (1 << (16 - 1)))
      op1 |= SIGNED_UPPER_BOUND_H;
    return op1;
  }
  virtual char const *describe() const {
    return "signed extend Half Word";
  }
} SextH_;

typedef struct SextW : public Operation {
  SextW() {
  }
  virtual ~SextW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    uint64_t op1 = (uint64_t)(boost::get<uint64_t>(operands[0]));
    if (op1 & (1 << (32 - 1)))
      op1 |= SIGNED_UPPER_BOUND_W;
    return op1;
  }
  virtual char const *describe() const {
    return "signed extend Word";
  }
} SextW_;

typedef struct SextX : public Operation {
  SextX() {
  }
  virtual ~SextX() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    return (uint64_t)(boost::get<uint64_t>(operands[0]) | SIGNED_UPPER_BOUND_X);
  }
  virtual char const *describe() const {
    return "signed extend Double Word";
  }
} SextX_;

typedef struct ZextB : public Operation {
  ZextB() {
  }
  virtual ~ZextB() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    uint64_t op = boost::get<uint64_t>(operands[0]);
    op &= ~SIGNED_UPPER_BOUND_B;
    return op;
  }
  virtual char const *describe() const {
    return "Zero extend Byte";
  }
} ZextB_;

typedef struct ZextH : public Operation {
  ZextH() {
  }
  virtual ~ZextH() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    uint64_t op = boost::get<uint64_t>(operands[0]);
    op &= ~SIGNED_UPPER_BOUND_H;
    return op;
  }
  virtual char const *describe() const {
    return "Zero extend Half Word";
  }
} ZextH_;

typedef struct ZextW : public Operation {
  ZextW() {
  }
  virtual ~ZextW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    uint64_t op = boost::get<uint64_t>(operands[0]);
    op &= ~SIGNED_UPPER_BOUND_W;
    return op;
  }
  virtual char const *describe() const {
    return "Zero extend Word";
  }
} ZextW_;

typedef struct ZextX : public Operation {
  ZextX() {
  }
  virtual ~ZextX() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    return boost::get<uint64_t>(operands[0]);
  }
  virtual char const *describe() const {
    return "Zero extend Double Word";
  }
} ZextX_;

typedef struct Xnor : public Operation {
  Xnor() {
  }
  virtual ~Xnor() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) ^ ~boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "Xnor";
  }
} Xnor_;

typedef struct UMul : public Operation {
  UMul() {
  }
  virtual ~UMul() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    bits op0 = boost::get<uint64_t>(operands[0]);
    bits op1 = boost::get<uint64_t>(operands[1]);
    uint64_t prod = uint64_t((op0 * op1) & (bits)0xFFFFFFFFFFFFFFFF);
    return prod;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "UMul";
  }
} UMul_;

typedef struct UMulH : public Operation {
  UMulH() {
  }
  virtual ~UMulH() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    bits op0 = boost::get<uint64_t>(operands[0]);
    bits op1 = boost::get<uint64_t>(operands[1]);
    uint64_t prod = (uint64_t)((op0 * op1) >> 64);
    return prod;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "UMulH";
  }
} UMulH_;

typedef struct UMulL : public Operation {
  UMulL() {
  }
  virtual ~UMulL() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op0 = (uint32_t)boost::get<uint64_t>(operands[0]);
    uint64_t op1 = (uint32_t)boost::get<uint64_t>(operands[1]);
    uint64_t prod = op0 * op1;
    return prod;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "UMulL";
  }
} UMulL_;

typedef struct SMul : public Operation {
  SMul() {
  }
  virtual ~SMul() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & (uint64_t)0xFFFFFFFFULL;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & (uint64_t)0xFFFFFFFFULL;
    uint64_t op0_s = op0 | (op0 & (uint64_t)0x80000000ULL ? (uint64_t)0xFFFFFFFF00000000ULL : 0ULL);
    uint64_t op1_s = op1 | (op1 & (uint64_t)0x80000000ULL ? (uint64_t)0xFFFFFFFF00000000ULL : 0ULL);
    uint64_t prod = op0_s * op1_s;
    return prod;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "SMul";
  }
} SMul_;

typedef struct SMulH : public Operation {
  SMulH() {
  }
  virtual ~SMulH() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    int64_t op0 = boost::get<uint64_t>(operands[0]);
    int64_t op1 = boost::get<uint64_t>(operands[1]);
    bits multi =
        (bits)((boost::multiprecision::int128_t)op0 * (boost::multiprecision::int128_t)op1);
    uint64_t prod = (uint64_t)((multi & ((bits)0xFFFFFFFFFFFFFFFF << 64)) >> 64);
    return prod;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "SMulH";
  }
} SMulH_;

typedef struct SMulL : public Operation {
  SMulL() {
  }
  virtual ~SMulL() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op0 = (int64_t)(int32_t)(boost::get<uint64_t>(operands[0]));
    uint64_t op1 = (int64_t)(int32_t)(boost::get<uint64_t>(operands[1]));
    uint64_t prod = op0 * op1;
    return prod;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "SMulL";
  }
} SMulL_;

typedef struct UDiv : public Operation {
  UDiv() {
  }
  virtual ~UDiv() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]);
    uint64_t op1 = boost::get<uint64_t>(operands[1]);

    if (op1 == 0)
      return uint64_t(0);

    return op0 / op1;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "UDiv";
  }
} UDiv_;

typedef struct SDiv : public Operation {
  SDiv() {
  }
  virtual ~SDiv() {
  }
  uint64_t calc(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]);
    uint64_t op1 = boost::get<uint64_t>(operands[1]);

    if (op1 == 0)
      return uint64_t(0);

    return op0 / op1;
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    return calc(operands);
  }
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return calc(operands) >> 32;
  }
  virtual char const *describe() const {
    return "SDiv";
  }
} SDiv_;

typedef struct MulX : public Operation {
  MulX() {
  }
  virtual ~MulX() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) * boost::get<uint64_t>(operands[1]);
  }
  virtual char const *describe() const {
    return "MulX";
  }
} MulX_;

typedef struct UDivX : public Operation {
  UDivX() {
  }
  virtual ~UDivX() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    if (boost::get<uint64_t>(operands[1]) != 0) {
      return boost::get<uint64_t>(operands[0]) / boost::get<uint64_t>(operands[1]);
    } else {
      return uint64_t(0ULL); // Will result in an exception, so it doesn't
                             // matter what we return
    }
  }
  virtual char const *describe() const {
    return "UDivX";
  }
} UDivX_;

typedef struct SDivX : public Operation {
  SDivX() {
  }
  virtual ~SDivX() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);
    uint64_t op0_s = boost::get<uint64_t>(operands[0]);
    uint64_t op1_s = boost::get<uint64_t>(operands[1]);
    if (op1_s != 0) {
      return op0_s / op1_s;
    } else {
      return uint64_t(0ULL); // Will result in an exception, so it doesn't
                             // matter what we return
    }
  }
  virtual char const *describe() const {
    return "SDivX";
  }
} SDivX_;

typedef struct MOV_ : public Operation {
  MOV_() {
  }
  virtual ~MOV_() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    if (theOperands.size()) {
      DBG_Assert(theOperands.size() == 1);
      return boost::get<uint64_t>(theOperands[0]);

    } else {
      DBG_Assert(operands.size() == 1);
      return boost::get<uint64_t>(operands[0]);
    }
  }
  virtual char const *describe() const {
    return "MOV";
  }
} MOV_;

typedef struct MOVN_ : public Operation {
  MOVN_() {
  }
  virtual ~MOVN_() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]);
  }
  virtual char const *describe() const {
    return "MOVN";
  }
} MOVN_;

typedef struct MOVK_ : public Operation {
  MOVK_() {
  }
  virtual ~MOVK_() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 3);
    uint64_t imm = boost::get<uint64_t>(operands[0]);
    uint64_t rd = boost::get<uint64_t>(operands[1]);
    uint64_t mask = boost::get<uint64_t>(operands[2]);

    rd &= mask;
    rd |= imm;

    return rd;
  }
  virtual char const *describe() const {
    return "MOVK";
  }
} MOVK_;

typedef struct OVERWRITE_ : public Operation {
  OVERWRITE_() {
  }
  virtual ~OVERWRITE_() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 3);
    uint64_t lhs = boost::get<uint64_t>(operands[0]);
    uint64_t rhs = boost::get<uint64_t>(operands[1]);
    uint64_t mask = boost::get<uint64_t>(operands[2]);

    return ((lhs & mask) | rhs);
  }
  virtual char const *describe() const {
    return "OVERWRITE_";
  }
} OVERWRITE_;

std::unique_ptr<Operation> shift(uint32_t aType) {
  std::unique_ptr<Operation> ptr;

  switch (aType) {
  case 0:
    ptr.reset(new LSL_());
    break;
  case 1:
    ptr.reset(new LSR_());
    break;
  case 2:
    ptr.reset(new ASR_());
    break;
  case 3:
    ptr.reset(new ROR_());
    break;
  default:
    DBG_Assert(false);
  }
  return ptr;
}

std::unique_ptr<Operation> operation(eOpType aType) {

  std::unique_ptr<Operation> ptr;
  switch (aType) {
  case kADD_:
    ptr.reset(new ADD_());
    break;
  case kAND_:
    ptr.reset(new AND_());
    break;
  case kORR_:
    ptr.reset(new ORR_());
    break;
  case kXOR_:
    ptr.reset(new XOR_());
    break;
  case kSUB_:
    ptr.reset(new SUB_());
    break;
  case kAndN_:
    ptr.reset(new AndN_());
    break;
  case kOrN_:
    ptr.reset(new OrN_());
    break;
  case kEoN_:
    ptr.reset(new EoN_());
    break;
  case kXnor_:
    ptr.reset(new Xnor_());
    break;
  case kCONCAT32_:
    ptr.reset(new CONCAT32_());
    break;
  case kCONCAT64_:
    ptr.reset(new CONCAT64_());
    break;
  case kMulX_:
    ptr.reset(new MulX_());
    break;
  case kUMul_:
    ptr.reset(new UMul_());
    break;
  case kUMulH_:
    ptr.reset(new UMulH_());
    break;
  case kUMulL_:
    ptr.reset(new UMulL_());
    break;
  case kSMul_:
    ptr.reset(new SMul_());
    break;
  case kSMulH_:
    ptr.reset(new SMulH_());
    break;
  case kSMulL_:
    ptr.reset(new SMulL_());
    break;
  case kUDivX_:
    ptr.reset(new UDivX_());
    break;
  case kUDiv_:
    ptr.reset(new UDiv_());
    break;
  case kSDiv_:
    ptr.reset(new SDiv_());
    break;
  case kSDivX_:
    ptr.reset(new SDivX_());
    break;
  case kMOV_:
    ptr.reset(new MOV_());
    break;
  case kMOVN_:
    ptr.reset(new MOVN_());
    break;
  case kMOVK_:
    ptr.reset(new MOVK_());
    break;
  case kSextB_:
    ptr.reset(new SextB_());
    break;
  case kSextH_:
    ptr.reset(new SextH_());
    break;
  case kSextW_:
    ptr.reset(new SextW_());
    break;
  case kSextX_:
    ptr.reset(new SextX_());
    break;
  case kZextB_:
    ptr.reset(new ZextB_());
    break;
  case kZextH_:
    ptr.reset(new ZextH_());
    break;
  case kZextW_:
    ptr.reset(new ZextW_());
    break;
  case kZextX_:
    ptr.reset(new ZextX_());
    break;
  case kNot_:
    ptr.reset(new Not_());
    break;
  case kROR_:
    ptr.reset(new ROR_());
    break;
  case kASR_:
    ptr.reset(new ASR_());
    break;
  case kLSR_:
    ptr.reset(new LSR_());
    break;
  case kLSL_:
    ptr.reset(new LSL_());
    break;
  case kOVERWRITE_:
    ptr.reset(new OVERWRITE_());
    break;
  default:
    DBG_Assert(false, (<< "Unimplemented operation type: " << aType));
  }
  return ptr;
}

} // namespace nDecoder
