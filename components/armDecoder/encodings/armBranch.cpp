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

#include "armBranch.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {

using namespace nuArchARM;

static void branch_always(SemanticInstruction *inst, bool immediate, VirtualMemoryAddress target) {
  DECODER_TRACE;

  inst->setClass(clsBranch, codeBranchUnconditional);
  inst->addDispatchEffect(branch(inst, target));
  inst->addRetirementEffect(updateUnconditional(inst, target));
}

static void branch_cond(SemanticInstruction *inst, VirtualMemoryAddress target, eCondCode aCode,
                        std::list<InternalDependance> &rs_deps) {
  DECODER_TRACE;

  inst->setClass(clsBranch, codeBranchConditional);

  dependant_action br = branchCondAction(inst, target, condition(aCode), 1);
  connectDependance(inst->retirementDependance(), br);

  rs_deps.push_back(br.dependance);

  //  inst->addDispatchAction( br );
  inst->addRetirementEffect(updateConditional(inst));
}

/*
 * Branch with Link branches to a PC-relative offset, setting the register X30
 * to PC+4. It provides a hint that this is a subroutine call.
 *
 * Operation:
 *  X[30] = PC[] + 4;
 *  BranchTo(PC[] + offset, BranchType_CALL);
 */
arminst UNCONDBR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  bool op = extract32(aFetchedOpcode.theOpcode, 31, 1);
  inst->setClass(clsBranch, codeBranchUnconditional);

  int64_t offset = sextract32(aFetchedOpcode.theOpcode, 0, 26) << 2;
  uint64_t addr = (uint64_t)aFetchedOpcode.thePC + offset;
  VirtualMemoryAddress target(addr);

  DBG_(Iface, (<< "branching to " << std::hex << target << " with an offset of 0x" << std::hex
               << offset << std::dec));

  inst->addPostvalidation(validatePC(inst));

  if (op) {
    inst->setClass(clsBranch, codeCALL);
    std::vector<std::list<InternalDependance>> rs_deps(1);
    predicated_action exec = addExecute(inst, operation(kMOV_), rs_deps);

    addReadConstant(inst, 1, (uint64_t)(aFetchedOpcode.thePC) + 4, rs_deps[0]);
    addDestination(inst, 30, exec, true);
  }

  inst->addDispatchEffect(branch(inst, target));
  return inst;
}

/*
 * Compare and Branch on Zero/ Non-Zero compares the value in a register with
 * zero, and conditionally branches to a label at a PC-relative offset if the
 * comparison is equal. It provides a hint that this is not a subroutine call or
 * return. This instruction does not affect condition flags.
 *
 * Operation:
 * bits(datasize) operand1 = X[t];
 * if IsZero(operand1) == TRUE then
 * BranchTo(PC[] + offset, BranchType_JMP);
 */
arminst CMPBR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
  bool iszero = !(extract32(aFetchedOpcode.theOpcode, 24, 1));
  int64_t offset = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
  uint64_t addr = (uint64_t)aFetchedOpcode.thePC + offset;
  VirtualMemoryAddress target(addr);

  DBG_(Iface, (<< "cmp/br to " << std::hex << target << " with an offset of 0x" << std::hex
               << offset << std::dec));

  std::vector<std::list<InternalDependance>> rs_deps(1);
  branch_cond(inst, target, iszero ? kCBZ_ : kCBNZ_, rs_deps[0]);
  addReadXRegister(inst, 1, rt, rs_deps[0], sf);
  inst->addPostvalidation(validatePC(inst));

  return inst;
}

/*
 * Test bit and Branch if Zero compares the value of a test bit with zero,
 * and conditionally branches to a label at a PC-relative offset if the
 * comparison is equal. It provides a hint that this is not a subroutine call
 * or return. This instruction does not affect condition flags.
 *
 * Operation:
 * bits(datasize) operand = X[t];
 * if operand<bit_pos> == op then
 * BranchTo(PC[] + offset, BranchType_JMP);
 *
 */
arminst TSTBR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t bit_pos = ((extract32(aFetchedOpcode.theOpcode, 31, 1) << 5) |
                      extract32(aFetchedOpcode.theOpcode, 19, 5));
  bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
  bool bit_val = extract32(aFetchedOpcode.theOpcode, 24, 1);
  int64_t offset = sextract32(aFetchedOpcode.theOpcode, 5, 14) << 2;
  VirtualMemoryAddress target((uint64_t)aFetchedOpcode.thePC + offset);

  DBG_(Iface, (<< "offest is set to #" << offset));
  DBG_(Iface, (<< "Branch address is set to " << target));

  DBG_(Iface, (<< "Testing bit value (" << bit_val << ") and bit pos (" << bit_pos << ")  -- "
               << "Branching to address " << std::hex << (uint64_t)aFetchedOpcode.thePC
               << " with an offset of 0x" << offset << std::dec << " -->> " << target));

  std::vector<std::list<InternalDependance>> rs_deps(1);
  branch_cond(inst, target, bit_val ? kTBNZ_ : kTBZ_, rs_deps[0]);

  readRegister(inst, 1, rt, rs_deps[0], sf);
  inst->setOperand(kCondition, uint64_t(1ULL << bit_pos));
  inst->addPostvalidation(validatePC(inst));

  return inst;
}

/*
 * Branch conditionally to a label at a PC-relative offset,
 * with a hint that this is not a subroutine call or return.
 *
 * Operation:
 * if ConditionHolds(condition) then
 * BranchTo(PC[] + offset, BranchType_JMP);
 */
arminst CONDBR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  uint64_t cond = extract32(aFetchedOpcode.theOpcode, 0, 4);

  //    program label to be conditionally branched to. Its offset from the
  //    address of this instruction,
  // in the range +/-1MB, is encoded as "imm19" times 4.
  int64_t offset = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
  uint64_t addr = (uint64_t)aFetchedOpcode.thePC + offset;

  VirtualMemoryAddress target(addr);

  if (cond < 0x0e) {
    DBG_(Iface, (<< "conditionally branching to " << std::hex << target << " with an offset of 0x"
                 << std::hex << offset << std::dec));

    /* genuinely conditional branches */
    std::vector<std::list<InternalDependance>> rs_deps(1);
    branch_cond(inst, target, kBCOND_, rs_deps[0]);
    inst->setOperand(kCondition, cond);
    addReadCC(inst, 1, rs_deps[0], true);
  } else {
    DBG_(Iface, (<< "unconditionally branching to " << std::hex << target << " with an offset of 0x"
                 << std::hex << offset << std::dec));
    inst->addPostvalidation(validatePC(inst));

    /* 0xe and 0xf are both "always" conditions */
    branch_always(inst, false, target);
  }
  inst->addPostvalidation(validatePC(inst));

  return inst;
}

/*
 * Branch to Register branches unconditionally to an address in a register,
 * with a hint that this is not a subroutine return.
 *
 * Operation:
 * bits(64) target = X[n];
 * BranchTo(target, BranchType_JMP);
 */
arminst BR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsBranch, codeBranchUnconditional);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  std::vector<std::list<InternalDependance>> rs_deps(1);

  addReadXRegister(inst, 1, rn, rs_deps[0], true);

  simple_action target = calcAddressAction(inst, rs_deps);
  dependant_action br = branchToCalcAddressAction(inst);
  connectDependance(br.dependance, target);
  connectDependance(inst->retirementDependance(), br);
  inst->addRetirementEffect(updateUnconditional(inst, kAddress));

  return inst;
}

/* >> -- same as RET
 * Branch with Link to Register calls a subroutine at an address in a register,
 * setting register X30 to PC+
 *
 * Operation:
 * bits(64) target = X[n];
 * if branch_type == BranchType_CALL then X[30] = PC[] + 4;
 * BranchTo(target, branch_type);
 */
arminst BLR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t rm = extract32(aFetchedOpcode.theOpcode, 0, 5);
  uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
  bool pac = extract32(aFetchedOpcode.theOpcode, 11, 1);
  uint32_t op = extract32(aFetchedOpcode.theOpcode, 21, 2);
  eBranchType branch_type;

  if (!pac && (rm != 0))
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  else if (pac) // armv8.3
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  switch (op) {
  case 0:
    branch_type = kIndirectReg;
    break;
  case 1:
    branch_type = kIndirectCall;
    break;
  case 2:
    branch_type = kReturn;
    break;
  default:
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    break;
  }

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  std::vector<std::list<InternalDependance>> rs_deps(1);

  simple_action target = calcAddressAction(inst, rs_deps);
  dependant_action br = branchRegAction(inst, kAddress, branch_type);
  connectDependance(br.dependance, target);
  connectDependance(inst->retirementDependance(), br);

  switch (branch_type) {
  case kIndirectCall:
    inst->setClass(clsBranch, codeBranchIndirectCall);
    break;
  case kIndirectReg:
    inst->setClass(clsBranch, codeBranchIndirectReg);
    break;
  case kReturn:
    inst->setClass(clsBranch, codeRETURN);
    break;
  default:
    break;
  }
  inst->addRetirementEffect(updateIndirect(inst, kAddress, branch_type));

  // Link
  if (branch_type == kIndirectCall) {
    std::unique_ptr<Operation> addop = operation(kMOV_);
    addop->setOperands((uint64_t)aFetchedOpcode.thePC + 4);
    predicated_action exec = addExecute(inst, std::move(addop), rs_deps);
    addDestination(inst, 30, exec, true);
  }

  addReadXRegister(inst, 1, rn, rs_deps[0], true);
  inst->addPostvalidation(validatePC(inst));

  return inst;
}

arminst ERET(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}
arminst DPRS(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

// System
arminst HINT(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  uint32_t op1 = extract32(aFetchedOpcode.theOpcode, 16, 3);
  uint32_t crm = extract32(aFetchedOpcode.theOpcode, 8, 4);
  uint32_t op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);
  uint32_t selector = crm << 3 | op2;

  if (op1 != 3) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  switch (selector) {
  case 0: /* NOP */
  case 3: /* WFI */
  case 1: /* YIELD */
  case 2: /* WFE */
  case 4: /* SEV */
  case 5: /* SEVL */
  default:
    return nop(aFetchedOpcode, aCPU, aSequenceNo);
  }
}
arminst SYNC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  // uint32_t rt = extract32(aFetchedOpcode.theOpcode, 16, 3);
  uint32_t crm = extract32(aFetchedOpcode.theOpcode, 8, 4);
  uint32_t op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);

  if (op2 == 0x0 || op2 == 0x1 || op2 == 0x7 || op2 == 0x3) {
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsSynchronizing, codeCLREX);

  uint32_t bar = 0;

  switch (op2) {
  case 2: /* CLREX */
    inst->setClass(clsSynchronizing, codeCLREX);
    inst->addRetirementEffect(clearExclusiveMonitor(inst));
    break;
    ;
  case 4: /* DSB */
  case 5: /* DMB */
    switch (crm & 3) {
    case 1: /* MBReqTypes_Reads */
      bar = kBAR_SC | kMO_LD_LD | kMO_LD_ST;
      break;
    case 2: /* MBReqTypes_Writes */
      bar = kBAR_SC | kMO_ST_ST;
      break;
    default: /* MBReqTypes_All */
      bar = kBAR_SC | kMO_ALL;
      break;
    }
    MEMBAR(inst, bar);
    break;
  case 6: /* ISB */
    /* We need to break the TB after this insn to execute
     * a self-modified code correctly and also to take
     * any pending interrupts immediately.
     */
    inst->setHaltDispatch();
    delete inst;
    return nop(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    delete inst;
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  }

  return inst;
}

arminst MSR(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t op1, crm, op2;
  op1 = extract32(aFetchedOpcode.theOpcode, 16, 3);
  crm = extract32(aFetchedOpcode.theOpcode, 8, 4); // imm
  op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  inst->setClass(clsComputation, codeWRPR);

  /* Msutherl: Added check for instructions with special encodings (e.g., DAIFSet and DAIFClr)
   * - for relevant encodings, see section C5.2.2 "DAIF, Interrupt Mask Bits", page C5-357 of the
   *   ARMv8 ISA manual */
  bool isShorthandDAIFEncoding = false;
  if (op1 == 0x3 && (op2 == 0x7 || op2 == 0x6)) {
    isShorthandDAIFEncoding = true;
  }

  // Check for supported PR's
  ePrivRegs pr = getPrivRegType(0, op1, op2, 0x4, crm);
  if (pr == kLastPrivReg && !isShorthandDAIFEncoding) {
    return blackBox(aFetchedOpcode, aCPU,
                    aSequenceNo); // resynchronize on all other PRs
  }

  // need to halt dispatch for writes
  inst->setHaltDispatch();
  if (!isShorthandDAIFEncoding) {
    // check general system register
    inst->addCheckTrapEffect(checkSystemAccess(inst, 0, op1, op2, 0x4, crm, 0x1f, 0));
  } else {
    inst->addCheckTrapEffect(checkDAIFAccess(inst, op1));
  }
  inst->setOperand(kResult, uint64_t(crm));

  // FIXME: This code never actually writes the register.

  // inst->addRetirementEffect( writePSTATE(inst, op1, op2) );
  // inst->addPostvalidation( validateXRegister( rt, kResult, inst  ) );
  // FIXME - validate PR

  return inst;
}

arminst SYS(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  uint32_t op0, op1, crn, crm, op2, rt;
  bool l;
  l = extract32(aFetchedOpcode.theOpcode, 21, 1);
  op0 = extract32(aFetchedOpcode.theOpcode, 19, 2);
  op1 = extract32(aFetchedOpcode.theOpcode, 16, 3);
  crn = extract32(aFetchedOpcode.theOpcode, 12, 4);
  crm = extract32(aFetchedOpcode.theOpcode, 8, 4);
  op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);
  rt = extract32(aFetchedOpcode.theOpcode, 0, 5);

  // Check for supported PR's
  ePrivRegs pr = getPrivRegType(op0, op1, op2, crn, crm);

  if (pr == kLastPrivReg) {
    pr = kAbstractSysReg;
  } else if (pr == kDC_ZVA) {
    return nop(aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));

  if (l) {
    inst->setClass(clsComputation, codeRDPR);
    inst->addCheckTrapEffect(checkSystemAccess(inst, op0, op1, op2, crn, crm, rt, l));
    setRD(inst, rt);
    inst->addDispatchEffect(mapDestination(inst));

    std::unique_ptr<SysRegInfo> ri = getPriv(op0, op1, op2, crn, crm);
    ri->setSystemRegisterEncodingValues(op0, op1, op2, crn, crm);
    inst->addRetirementEffect(readPR(inst, pr, std::move(ri)));
    inst->addPostvalidation(validateXRegister(rt, kResult, inst, true));
  } else {
    inst->setClass(clsComputation, codeWRPR);
    return inst; // FIXME: This will never actually write the register

    std::vector<std::list<InternalDependance>> rs_dep(1);
    // need to halt dispatch for writes
    inst->setHaltDispatch();
    inst->addCheckTrapEffect(checkSystemAccess(inst, op0, op1, op2, crn, crm, rt, l));
    addReadXRegister(inst, 1, rt, rs_dep[0], true);
    addExecute(inst, operation(kMOV_), rs_dep);
    std::unique_ptr<SysRegInfo> ri = getPriv(op0, op1, op2, crn, crm);
    ri->setSystemRegisterEncodingValues(op0, op1, op2, crn, crm);
    inst->addRetirementEffect(writePR(inst, pr, std::move(ri)));
    //        inst->addPostvalidation( validateXRegister( rt, kResult, inst  )
    //        );
    //        FIXME - validate PR
  }

  return inst;
}

// Exception generation
arminst SVC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
  return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_SVC);
}
arminst HVC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_HVC);
}
arminst SMC(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_SMC);
}
arminst BRK(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_BKPT);
}
arminst HLT(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_UNCATEGORIZED);
} // not supported - do we care about semihosting exception? if yes, then raise
  // it here
arminst DCPS(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_UNCATEGORIZED);
} // not supported

} // namespace narmDecoder
