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

#include "armDataProcReg.hpp"
#include "armSharedFunctions.hpp"
#include "armUnallocated.hpp"
#include <components/uArchARM/uArchInterfaces.hpp>

namespace narmDecoder {
using namespace nuArchARM;

// Data-processing (1 source)
arminst RBIT(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo,
                                                    clsComputation, codeRev));

  std::vector<std::list<InternalDependance>> rs_deps(1);
  predicated_action act = reverseAction(inst, kOperand1, kResult, rs_deps, sf);

  readRegister(inst, 1, rn, rs_deps[0], sf);

  addDestination(inst, rd, act, sf);

  return inst;
}
arminst REV(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t opc = extract32(aFetchedOpcode.theOpcode, 10, 2);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

  std::vector<std::list<InternalDependance>> rs_deps(1);

  uint8_t container_size = 0;
  switch (opc) {
  case 0x0:
    DBG_Assert(false);
    break;
  case 0x1:
    container_size = 16;
    break;
  case 0x2:
    container_size = 32;
    break;
  case 0x3:
    if (!sf)
      return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    container_size = 64;
  }

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  predicated_action act = reorderAction(inst, kOperand1, kResult, container_size, rs_deps, sf);

  inst->setClass(clsComputation, codeALU);

  readRegister(inst, 1, rn, rs_deps[0], sf);

  addDestination(inst, rd, act, sf);

  return inst;
}

arminst CL(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  bool op = extract32(aFetchedOpcode.theOpcode, 10, 1);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  eCountOp opcode = op ? kCountOp_CLS : kCountOp_CLZ;
  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  std::vector<std::list<InternalDependance>> rs_deps(1);

  inst->setClass(clsComputation, codeALU);
  predicated_action act = countAction(inst, kOperand1, kResult, opcode, rs_deps, sf);

  readRegister(inst, 1, rn, rs_deps[0], sf);

  addDestination(inst, rd, act, sf);
  return inst;
}

// Data-processing (2 sources)
arminst DIV(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  bool is_signed = extract32(aFetchedOpcode.theOpcode, 10, 1);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsComputation, codeALU);

  std::vector<std::list<InternalDependance>> rs_deps(2);
  predicated_action exec =
      addExecute(inst, is_signed ? operation(kSDiv_) : operation(kUDiv_), rs_deps);

  readRegister(inst, 1, rn, rs_deps[0], sf);
  readRegister(inst, 2, rm, rs_deps[1], sf);

  addDestination(inst, rd, exec, sf);

  return inst;
}

arminst SHIFT(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {

  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  uint32_t shift_type = extract32(aFetchedOpcode.theOpcode, 10, 2);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsComputation, codeALU);

  std::vector<std::list<InternalDependance>> rs_deps(3);

  predicated_action exec = addExecute(inst, shift(shift_type), rs_deps);

  addReadXRegister(inst, 1, rn, rs_deps[0], sf);
  addReadXRegister(inst, 2, rm, rs_deps[1], sf);
  addReadConstant(inst, 3, sf ? 64 : 32, rs_deps[2]);

  addDestination(inst, rd, exec, sf);

  return inst;
}

arminst CRC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t sz = extract32(aFetchedOpcode.theOpcode, 10, 2);
  bool crc32c = extract32(aFetchedOpcode.theOpcode, 12, 1);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

  if (sf && sz != 3)
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  if (!sf && sz == 3)
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  std::vector<std::list<InternalDependance>> rs_deps(2);
  uint32_t poly = crc32c ? 0x1EDC6F41 : 0x04C11DB7;

  predicated_action act = crcAction(inst, poly, kOperand1, kOperand2, kResult, sf);

  readRegister(inst, 1, rn, rs_deps[0], false);
  readRegister(inst, 2, rm, rs_deps[1], sf);

  connect(rs_deps[0], act);
  connect(rs_deps[1], act);

  addDestination(inst, rd, act, sf);

  return inst;
}

// Conditional select
arminst CSEL(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 11, 1)) {
    /* S == 1 or op2<1> == 1 */
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsComputation, codeALU);

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  bool inv = extract32(aFetchedOpcode.theOpcode, 30, 1);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  bool inc = extract32(aFetchedOpcode.theOpcode, 10, 1);

  std::vector<std::list<InternalDependance>> rs_deps(3);
  inst->setOperand(kOperand5, (uint64_t)1);

  std::unique_ptr<Condition> ptr = condition(kBCOND_);
  predicated_action act = conditionSelectAction(inst, ptr, cond, rs_deps, kResult, inv, inc, sf);
  inst->setOperand(kCondition, (uint64_t)cond);

  readRegister(inst, 1, rn, rs_deps[0], sf);
  readRegister(inst, 2, rm, rs_deps[1], sf);
  addReadCC(inst, 3, rs_deps[2], true);

  addDestination(inst, rd, act, sf);

  return inst;
}

// Conditional compare (immediate / register)
arminst CCMP(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  if (!extract32(aFetchedOpcode.theOpcode, 29, 1)) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }
  if (aFetchedOpcode.theOpcode & (1 << 10 | 1 << 4)) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsComputation, codeALU);
  uint64_t flags = extract32(aFetchedOpcode.theOpcode, 0, 4);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  uint32_t cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
  bool sub_op = extract32(aFetchedOpcode.theOpcode, 30, 1);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  bool imm_provided = extract32(aFetchedOpcode.theOpcode, 11, 1);

  std::vector<std::list<InternalDependance>> rs_deps(3);
  std::unique_ptr<Condition> condOp = condition(kBCOND_);
  predicated_action act = conditionCompareAction(inst, condOp, cond, rs_deps, kResult, sub_op, sf);
  inst->setOperand(kCondition, (uint64_t)cond);
  inst->setOperand(kOperand4, flags);

  readRegister(inst, 1, rn, rs_deps[0], sf);
  if (imm_provided)
    addReadConstant(inst, 2, (uint64_t)rm, rs_deps[1]);
  else
    readRegister(inst, 2, rm, rs_deps[1], sf);
  addReadCC(inst, 3, rs_deps[2], true);

  addSetCC(inst, act, true);

  return inst;
}

// Add/subtract (with carry)
arminst ADDSUB_CARRY(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  bool sub_op = extract32(aFetchedOpcode.theOpcode, 30, 1);
  bool setflags = extract32(aFetchedOpcode.theOpcode, 29, 1);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  std::vector<std::list<InternalDependance>> rs_deps(3);
  predicated_action exec = addExecute(
      inst, operation(setflags ? (sub_op ? kSUBS_ : kADDS_) : (sub_op ? kSUB_ : kADD_)), rs_deps);

  readRegister(inst, 1, rn, rs_deps[0], sf);
  readRegister(inst, 2, rm, rs_deps[1], sf);

  simple_action nzcv = readNZCVAction(inst, kC, kOperand3);
  connect(rs_deps[2], nzcv);
  inst->addDispatchAction(nzcv);

  if (!setflags || rd != 31)
    addDestination(inst, rd, exec, sf, setflags);
  else if (setflags)
    addSetCC(inst, exec, sf);
  return inst;
}

// Data-processing (3 source)
arminst DP_3_SRC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  uint32_t op_id = (extract32(aFetchedOpcode.theOpcode, 29, 3) << 4) |
                   (extract32(aFetchedOpcode.theOpcode, 21, 3) << 1) |
                   extract32(aFetchedOpcode.theOpcode, 15, 1);
  bool is_signed = false;

  /* Note that op_id is sf:op54:op31:o0 so it includes the 32/64 size flag */
  switch (op_id) {
  case 0x42: /* SMADDL */
  case 0x43: /* SMSUBL */
  case 0x44: /* SMULH */
    is_signed = true;
    break;
  case 0x0:  /*  MADD (32bit) */
  case 0x1:  /*  MSUB (32bit) */
  case 0x40: /* MADD (64bit) */
  case 0x41: /* MSUB (64bit) */
  case 0x4a: /* UMADDL */
  case 0x4b: /* UMSUBL */
  case 0x4c: /* UMULH */
    break;
  default:
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  uint32_t ra = extract32(aFetchedOpcode.theOpcode, 10, 5);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  bool is_long = extract32(aFetchedOpcode.theOpcode, 21, 1);
  bool is_sub = extract32(op_id, 0, 1);
  bool is_high = extract32(op_id, 2, 1);

  SemanticInstruction *inst = new SemanticInstruction(
      aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

  inst->setClass(clsComputation, codeALU);
  std::vector<std::list<InternalDependance>> rs_deps(2), rs2_deps(2);

  predicated_action mul = addExecute(inst,
                                     operation(is_high ? (is_signed ? kSMulH_ : kUMulH_)
                                                       : (is_long ? (is_signed ? kSMulL_ : kUMulL_)
                                                                  : (is_signed ? kSMul_ : kUMul_))),
                                     rs_deps);

  readRegister(inst, 1, rn, rs_deps[0], is_long ? false : sf);
  readRegister(inst, 2, rm, rs_deps[1], is_long ? false : sf);

  if (!is_high) {
    predicated_action act = addExecute(inst, operation(is_sub ? kSUB_ : kADD_),
                                       {kOperand3, kResult}, rs2_deps, kResult1);
    // Store output in kResult1 to not overwrite mul output
    readRegister(inst, 3, ra, rs2_deps[0], sf);
    connect(rs2_deps[1], mul);
    addDestination1(inst, rd, act, sf);
  } else {
    addDestination(inst, rd, mul, sf);
  }

  return inst;
}

// Add/subtract (shifted register)
arminst ADDSUB_SHIFTED(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {

  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  bool setflags = extract32(aFetchedOpcode.theOpcode, 29, 1);
  bool sub_op = extract32(aFetchedOpcode.theOpcode, 30, 1);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  uint32_t shift_amount = extract32(aFetchedOpcode.theOpcode, 10, 6);
  uint32_t shift_type = extract32(aFetchedOpcode.theOpcode, 22, 2);

  if ((shift_type == 3) || (!sf && (shift_amount > 31))) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst = new SemanticInstruction(
      aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

  inst->setClass(clsComputation, codeALU);
  std::vector<std::list<InternalDependance>> rs_deps(1), rs2_deps(2);

  inst->setOperand(kOperand4, (uint64_t)shift_amount);
  inst->setOperand(kOperand5, sf ? (uint64_t)64 : (uint64_t)32);
  predicated_action sh =
      addExecute(inst, shift(shift_type), {kOperand3, kOperand4, kOperand5}, rs_deps, kOperand2);
  inst->addDispatchEffect(satisfy(inst, sh.action->dependance(1)));
  inst->addDispatchEffect(satisfy(inst, sh.action->dependance(2)));

  predicated_action res = addExecute(
      inst, operation((sub_op ? (setflags ? kSUBS_ : kSUB_) : (setflags ? kADDS_ : kADD_))),
      rs2_deps);
  connect(rs2_deps[1], sh);

  readRegister(inst, 1, rn, rs2_deps[0], sf);
  readRegister(inst, 3, rm, rs_deps[0], sf);

  if (!setflags || rd != 31)
    addDestination(inst, rd, res, sf, setflags);
  else if (setflags)
    addSetCC(inst, res, sf);

  return inst;
}

// Add/subtract (extended register)
arminst ADDSUB_EXTENDED(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t option = extract32(aFetchedOpcode.theOpcode, 13, 3);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  bool setflags = extract32(aFetchedOpcode.theOpcode, 29, 1);
  bool sub_op = extract32(aFetchedOpcode.theOpcode, 30, 1);
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  eExtendType extend_type = DecodeRegExtend(option);
  uint32_t shift_amount = extract32(aFetchedOpcode.theOpcode, 10, 3);
  bool sf_rm = ((option & 0x3) == 0x3) ? true : false;
  if (shift_amount > 4) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst = new SemanticInstruction(
      aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

  inst->setClass(clsComputation, codeALU);
  std::vector<std::list<InternalDependance>> rs_deps(1), rs2_deps(2);

  predicated_action ex = addExecute(inst, extend(extend_type), {kOperand3}, rs_deps, kOperand2);
  rs_deps.resize(2);
  inst->setOperand(kOperand4, (uint64_t)shift_amount);
  inst->setOperand(kOperand5, sf ? (uint64_t)64 : (uint64_t)32);
  predicated_action sh =
      addExecute(inst, shift(0 /*kLSL_*/), {kOperand2, kOperand4, kOperand5}, rs_deps, kOperand2);
  connect(rs_deps[1], ex);
  inst->addDispatchEffect(satisfy(inst, sh.action->dependance(2)));

  predicated_action res = addExecute(
      inst, operation((sub_op ? (setflags ? kSUBS_ : kSUB_) : (setflags ? kADDS_ : kADD_))),
      rs2_deps);
  connect(rs2_deps[1], sh);

  if (rn == 31)
    addReadXRegister(inst, 1, rn, rs2_deps[0], sf);
  else
    readRegister(inst, 1, rn, rs2_deps[0], sf);
  readRegister(inst, 3, rm, rs_deps[0], sf ? sf_rm : sf);

  if (!setflags || rd != 31)
    addDestination(inst, rd, res, sf, setflags);
  else if (setflags)
    addSetCC(inst, res, sf);

  return inst;
}

// Logical (shifted register)
arminst LOGICAL(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {

  DECODER_TRACE;
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  uint32_t opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
  uint32_t shift_type = extract32(aFetchedOpcode.theOpcode, 22, 2);
  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
  uint32_t shift_amount = extract32(aFetchedOpcode.theOpcode, 10, 6);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
  bool setflags = false;
  uint32_t n = extract32(aFetchedOpcode.theOpcode, 21, 1);

  if (!sf && (shift_amount & (1 << 5))) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst = new SemanticInstruction(
      aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);
  inst->setClass(clsComputation, codeALU);
  std::vector<std::list<InternalDependance>> rs_deps(1), rs2_deps(2);
  inst->setOperand(kOperand4, (uint64_t)shift_amount);
  inst->setOperand(kOperand5, sf ? (uint64_t)64 : (uint64_t)32);
  predicated_action sh =
      addExecute(inst, shift(shift_type), {kOperand3, kOperand4, kOperand5}, rs_deps, kOperand2);
  inst->addDispatchEffect(satisfy(inst, sh.action->dependance(1)));
  inst->addDispatchEffect(satisfy(inst, sh.action->dependance(2)));

  eOpType op;
  std::unique_ptr<Operation> ptr;
  switch (opc) {
  case 0:
    op = n ? kAndN_ : kAND_;
    break;
  case 1:
    op = n ? kOrN_ : kORR_;
    break;
  case 2:
    op = n ? kXnor_ : kXOR_;
    break;
  case 3:
    op = n ? kANDSN_ : kANDS_;
    setflags = true;
    break;
  }

  predicated_action act = addExecute(inst, operation(op), rs2_deps);
  connect(rs2_deps[1], sh);

  readRegister(inst, 1, rn, rs2_deps[0], sf);
  readRegister(inst, 3, rm, rs_deps[0], sf);

  if (!setflags || rd != 31)
    addDestination(inst, rd, act, sf, setflags);
  else if (setflags)
    addSetCC(inst, act, sf);

  return inst;
}

} // namespace narmDecoder
