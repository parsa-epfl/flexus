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

#include <core/qemu/api_wrappers.hpp>
#include <components/uArch/uArchInterfaces.hpp>

#include "SemanticActions.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

#define sext32(x) ({          \
  int64_t z = (int64_t)(x);   \
  if (z & 0x80000000l)        \
    z |= 0xffffffff00000000l; \
  else                        \
    z &= 0x00000000ffffffffl; \
  z;                          \
})

#define zext32(x) ({          \
  uint64_t z = (uint64_t)(x); \
  z &= 0xfffffffflu;          \
  z;                          \
})

// from spike
#define sext64(x) (( int64_t)(x))
#define zext64(x) ((uint64_t)(x))

typedef struct ADD : public Operation {
  ADD() {
  }
  virtual ~ADD() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return zext64(rs1 + rs2); // signed
  }
  virtual char const *describe() const {
    return "ADD";
  }
} ADD;

typedef struct ADDW : public Operation {
  ADDW() {
  }
  virtual ~ADDW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(sext32(rs2 + rs1)); // signed
  }
  virtual char const *describe() const {
    return "ADDW";
  }
} ADDW;

typedef struct SUB : public Operation {
  SUB() {
  }
  virtual ~SUB() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(rs1 - rs2); // signed
  }
  virtual char const *describe() const {
    return "SUB";
  }
} SUB;

typedef struct SUBW : public Operation {
  SUBW() {
  }
  virtual ~SUBW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(sext32(rs1 - rs2)); // signed
  }
  virtual char const *describe() const {
    return "SUBW";
  }
} SUBW;

typedef struct AND : public Operation {
  AND() {
  }
  virtual ~AND() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return rs1 & rs2;
  }
  virtual char const *describe() const {
    return "AND";
  }
} AND;

typedef struct ORR : public Operation {
  ORR() {
  }
  virtual ~ORR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return rs1 | rs2;
  }
  virtual char const *describe() const {
    return "ORR";
  }
} ORR;

typedef struct XOR : public Operation {
  XOR() {
  }
  virtual ~XOR() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return rs1 ^ rs2;
  }
  virtual char const *describe() const {
    return "XOR";
  }
} XOR;

typedef struct MOV : public Operation {
  MOV() {
  }
  virtual ~MOV() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 1);

    return boost::get<uint64_t>(operands[0]);
  }
  virtual char const *describe() const {
    return "MOV";
  }
} MOV;

typedef struct SLL : public Operation {
  SLL() {
  }
  virtual ~SLL() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return zext64(rs1 << (rs2 & 0x3f)); // signed
  }
  virtual char const *describe() const {
    return "SLL";
  }
} SLL;

typedef struct SLLW : public Operation {
  SLLW() {
  }
  virtual ~SLLW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return zext64(sext32(rs1 << (rs2 & 0x1f))); // signed
  }
  virtual char const *describe() const {
    return "SLLW";
  }
} SLLW;

typedef struct SLT : public Operation {
  SLT() {
  }
  virtual ~SLT() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(!!(rs1 < rs2)); // signed
  }
  virtual char const *describe() const {
    return "SLT";
  }
} SLT;

typedef struct SLTU : public Operation {
  SLTU() {
  }
  virtual ~SLTU() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return zext64(!!(rs1 < rs2)); // signed
  }
  virtual char const *describe() const {
    return "SLTU";
  }
} SLTU;

typedef struct SRA : public Operation {
  SRA() {
  }
  virtual ~SRA() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(rs1 >> (rs2 & 0x3f)); // signed
  }
  virtual char const *describe() const {
    return "SRA";
  }
} SRA;

typedef struct SRAW : public Operation {
  SRAW() {
  }
  virtual ~SRAW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int32_t rs1 = sext32(boost::get<uint64_t>(operands[0]));
    int32_t rs2 = sext32(boost::get<uint64_t>(operands[1]));

    return zext64(sext32(rs1 >> (rs2 & 0x1f))); // signed
  }
  virtual char const *describe() const {
    return "SRAW";
  }
} SRAW;

typedef struct SRL : public Operation {
  SRL() {
  }
  virtual ~SRL() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return zext64(rs1 >> (rs2 & 0x3f)); // signed
  }
  virtual char const *describe() const {
    return "SRL";
  }
} SRL;

typedef struct SRLW : public Operation {
  SRLW() {
  }
  virtual ~SRLW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint32_t rs1 = zext32(boost::get<uint64_t>(operands[0]));
    uint32_t rs2 = zext32(boost::get<uint64_t>(operands[1]));

    return zext64(sext32(rs1 >> (rs2 & 0x1f))); // signed
  }
  virtual char const *describe() const {
    return "SRLW";
  }
} SRLW;

typedef struct DIV : public Operation {
  DIV() {
  }
  virtual ~DIV() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    if (rs2 == 0)
      return UINT64_MAX;
    else if ((rs1 == INT64_MIN) && (rs2 == -1))
      return zext64(rs1); // signed
    else
      return zext64(rs1 / rs2); // signed
  }
  virtual char const *describe() const {
    return "DIV";
  }
} DIV;

typedef struct DIVU : public Operation {
  DIVU() {
  }
  virtual ~DIVU() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    if (rs2 == 0)
      return UINT64_MAX;
    else
      return zext64(rs1 / rs2); // signed
  }
  virtual char const *describe() const {
    return "DIVU";
  }
} DIVU;

typedef struct DIVUW : public Operation {
  DIVUW() {
  }
  virtual ~DIVUW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint32_t rs1 = zext32(boost::get<uint64_t>(operands[0]));
    uint32_t rs2 = zext32(boost::get<uint64_t>(operands[1]));

    if (rs2 == 0)
      return UINT64_MAX;
    else
      return zext64(sext32(rs1 / rs2)); // signed
  }
  virtual char const *describe() const {
    return "DIVUW";
  }
} DIVUW;

typedef struct DIVW : public Operation {
  DIVW() {
  }
  virtual ~DIVW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int32_t rs1 = sext32(boost::get<uint64_t>(operands[0]));
    int32_t rs2 = sext32(boost::get<uint64_t>(operands[1]));

    if (rs2 == 0)
      return UINT64_MAX;
    else
      return zext64(sext32(rs1 / rs2)); // signed
  }
  virtual char const *describe() const {
    return "DIVW";
  }
} DIVW;

// from spike
static uint64_t mulhu(uint64_t a, uint64_t b) {
  uint64_t t;
  uint32_t y1;
  uint32_t y2;
  uint32_t y3;
  uint64_t a0 = zext32(a);
  uint64_t a1 = a >> 32;
  uint64_t b0 = zext32(b);
  uint64_t b1 = b >> 32;

  t  = a1 * b0 + ((a0 * b0) >> 32);
  y1 = t;
  y2 = t >> 32;

  t  = a0 * b1 + y1;
  t  = a1 * b1 + y2 + (t >> 32);
  y2 = t;
  y3 = t >> 32;

  return ((uint64_t)(y3) << 32) | y2;
}

static int64_t mulh(int64_t a, int64_t b) {
  int negate = (a < 0) != (b < 0);

  uint64_t res = mulhu(a < 0 ? -a : a, b < 0 ? -b : b);

  return negate ? ~res + (((uint64_t)(a) * (uint64_t)(b)) == 0) : res;
}

static int64_t mulhsu(int64_t a, uint64_t b) {
  int negate = a < 0;

  uint64_t res = mulhu(a < 0 ? -a : a, b);

  return negate ? ~res + ((a * b) == 0) : res;
}

typedef struct MUL : public Operation {
  MUL() {
  }
  virtual ~MUL() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(rs1 * rs2); // signed
  }
  virtual char const *describe() const {
    return "MUL";
  }
} MUL;

typedef struct MULH : public Operation {
  MULH() {
  }
  virtual ~MULH() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(sext64(mulh(rs1, rs2))); // signed
  }
  virtual char const *describe() const {
    return "MULH";
  }
} MULH;

typedef struct MULHSU : public Operation {
  MULHSU() {
  }
  virtual ~MULHSU() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    return zext64(mulhsu(rs1, rs2)); // signed
  }
  virtual char const *describe() const {
    return "MULHSU";
  }
} MULHSU;

typedef struct MULHU : public Operation {
  MULHU() {
  }
  virtual ~MULHU() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    return mulhu(rs1, rs2);
  }
  virtual char const *describe() const {
    return "MULHU";
  }
} MULHU;

typedef struct MULW : public Operation {
  MULW() {
  }
  virtual ~MULW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int32_t rs1 = sext32(boost::get<uint64_t>(operands[0]));
    int32_t rs2 = sext32(boost::get<uint64_t>(operands[1]));

    return zext64(sext32(rs1 * rs2)); // signed
  }
  virtual char const *describe() const {
    return "MULW";
  }
} MULW;

typedef struct REM : public Operation {
  REM() {
  }
  virtual ~REM() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int64_t rs1 = sext64(boost::get<uint64_t>(operands[0]));
    int64_t rs2 = sext64(boost::get<uint64_t>(operands[1]));

    if (rs2 == 0)
      return zext64(rs1); // signed
    else if ((rs1 == INT64_MIN) && (rs2 == -1))
      return zext64(0); // signed
    else
      return zext64(rs1 % rs2); // signed
  }
  virtual char const *describe() const {
    return "REM";
  }
} REM;

typedef struct REMU : public Operation {
  REMU() {
  }
  virtual ~REMU() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint64_t rs1 = boost::get<uint64_t>(operands[0]);
    uint64_t rs2 = boost::get<uint64_t>(operands[1]);

    if (rs2 == 0)
      return zext64(rs1); // signed
    else
      return zext64(rs1 % rs2); // signed
  }
  virtual char const *describe() const {
    return "REMU";
  }
} REMU;

typedef struct REMUW : public Operation {
  REMUW() {
  }
  virtual ~REMUW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    uint32_t rs1 = zext32(boost::get<uint64_t>(operands[0]));
    uint32_t rs2 = zext32(boost::get<uint64_t>(operands[1]));

    if (rs2 == 0)
      return zext64(zext32(rs1)); // signed
    else
      return zext64(zext32(rs1 % rs2)); // signed
  }
  virtual char const *describe() const {
    return "REMUW";
  }
} REMUW;

typedef struct REMW : public Operation {
  REMW() {
  }
  virtual ~REMW() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) {
    DBG_Assert(operands.size() == 2);

    int32_t rs1 = sext32(boost::get<uint64_t>(operands[0]));
    int32_t rs2 = sext32(boost::get<uint64_t>(operands[1]));

    if (rs2 == 0)
      return zext32(rs1); // signed
    else
      return zext32(rs1 % rs2); // signed
  }
  virtual char const *describe() const {
    return "REMW";
  }
} REMW;

std::unique_ptr<Operation> operation(eOpType aType) {

  std::unique_ptr<Operation> ptr;
  switch (aType) {
  case kADD:
    ptr.reset(new ADD());
    break;
  case kADDW:
    ptr.reset(new ADDW());
    break;
  case kSUB:
    ptr.reset(new SUB());
    break;
  case kSUBW:
    ptr.reset(new SUBW());
    break;
  case kAND:
    ptr.reset(new AND());
    break;
  case kORR:
    ptr.reset(new ORR());
    break;
  case kXOR:
    ptr.reset(new XOR());
    break;
  case kMOV:
    ptr.reset(new MOV());
    break;
  case kSLL:
    ptr.reset(new SLL());
    break;
  case kSLLW:
    ptr.reset(new SLLW());
    break;
  case kSLT:
    ptr.reset(new SLT());
    break;
  case kSLTU:
    ptr.reset(new SLTU());
    break;
  case kSRA:
    ptr.reset(new SRA());
    break;
  case kSRAW:
    ptr.reset(new SRAW());
    break;
  case kSRL:
    ptr.reset(new SRL());
    break;
  case kSRLW:
    ptr.reset(new SRLW());
    break;
  case kDIV:
    ptr.reset(new DIV());
    break;
  case kDIVU:
    ptr.reset(new DIVU());
    break;
  case kDIVUW:
    ptr.reset(new DIVUW());
    break;
  case kDIVW:
    ptr.reset(new DIVW());
    break;
  case kMUL:
    ptr.reset(new MUL());
    break;
  case kMULH:
    ptr.reset(new MULH());
    break;
  case kMULHSU:
    ptr.reset(new MULHSU());
    break;
  case kMULHU:
    ptr.reset(new MULHU());
    break;
  case kMULW:
    ptr.reset(new MULW());
    break;
  case kREM:
    ptr.reset(new REM());
    break;
  case kREMU:
    ptr.reset(new REMU());
    break;
  case kREMUW:
    ptr.reset(new REMUW());
    break;
  case kREMW:
    ptr.reset(new REMW());
    break;
  default:
    DBG_Assert(false, (<< "Unimplemented operation type: " << aType));
  }
  return ptr;
}

} // namespace nDecoder
