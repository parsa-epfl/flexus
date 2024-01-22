#include "BitManip.hpp"
#include "Instruction.hpp"
#include "SemanticActions.hpp"
#include "SemanticInstruction.hpp"
#include "Validations.hpp"

namespace nDecoder {

#if 0
void setRD(SemanticInstruction *inst, uint32_t rd) {
  reg regRD;
  regRD.theType = xRegisters;
  regRD.theIndex = rd;
  inst->setOperand(kRD, regRD);
  DECODER_DBG("Writing to x|w[" << rd << "]"
                                << "\e[0m");
}

void addAnnulment(SemanticInstruction *inst, predicated_action &exec,
                  InternalDependance const &aWritebackDependance) {
  predicated_action annul = annulAction(inst);
  // inst->addDispatchAction( annul );

  connectDependance(aWritebackDependance, annul);

  inst->addAnnulmentEffect(satisfy(inst, annul.predicate));
  inst->addAnnulmentEffect(squash(inst, exec.predicate));
  inst->addReinstatementEffect(squash(inst, annul.predicate));
  inst->addReinstatementEffect(satisfy(inst, exec.predicate));
}

void addWriteback(SemanticInstruction *inst, eOperandCode aRegisterCode,
                  eOperandCode aMappedRegisterCode, predicated_action &exec, bool a64,
                  bool setflags, bool addSquash) {
  if (addSquash) {
    inst->addDispatchEffect(mapDestination(inst));
  } else {
    inst->addDispatchEffect(mapDestination_NoSquashEffects(inst));
  }

  // Create the writeback action
  reg name(inst->operand<reg>(kRD));

  dependant_action wb =
      writebackAction(inst, aRegisterCode, aMappedRegisterCode, a64, name.theIndex == 31, setflags);
  addAnnulment(inst, exec, wb.dependance);

  // Make writeback depend on execute, make retirement depend on writeback
  connectDependance(wb.dependance, exec);
  connectDependance(inst->retirementDependance(), wb);
}

void addDestination(SemanticInstruction *inst, uint32_t rd, predicated_action &exec, bool is64,
                    bool setflags, bool addSquash) {

  setRD(inst, rd);
  addWriteback(inst, kResult, kPD, exec, is64, setflags, addSquash);
  inst->addPostvalidation(validateXRegister(rd, kResult, inst, is64));
}

// add
// WRITE_RD(sext_xlen(RS1 + RS2));

archinst ADD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    addDestination(inst, rd, exec, true);

    return inst;
}

// addi
// WRITE_RD(sext_xlen(RS1 + insn.i_imm()));

archinst ADDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    addDestination(inst, rd, exec, true);

    return inst;
}

// addiw
// require_rv64;
// WRITE_RD(sext32(insn.i_imm() + RS1));

archinst ADDIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    addDestination(inst, rd, exec, false);

    return inst;
}

// addw
// require_rv64;
// WRITE_RD(sext32(RS1 + RS2));

archinst ADDW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);
    addDestination(inst, rd, exec, false);

    return inst;
}

// and
// WRITE_RD(RS1 & RS2);

archinst AND(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kAND_), rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    addDestination(inst, rd, exec, true);

    return inst;
}

// andi
// WRITE_RD(insn.i_imm() & RS1);

archinst ANDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kAND_), rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    addDestination(inst, rd, exec, true);

    return inst;
}

// auipc
// WRITE_RD(sext_xlen(insn.u_imm() + pc));

archinst AUIPC(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_u(opcode);
    uint64_t pc  = aFetchedOpcode.thePC;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    predicated_action exec = addExecute(inst, operation(kAUIPC_, rs_deps));

    addReadConstant(inst, 1, imm, rs_deps[0]);
    addReadConstant(inst, 2, pc,  rs_deps[1]);
    addDestination(inst, rd, exec, true);

    return inst;
}

// beq
// if (RS1 == RS2)
//   set_pc(BRANCH_TARGET);

archinst BEQ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_sb(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchConditional);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    dependent_action exec = branchCondAction(inst, tgt, condition(kBEQ_), 1);
    connectDependance(inst->retirementDependance(), exec);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);

    rs_deps.push_back(exec.dependance);
    inst->addRetirementEffect(updateConditional(inst));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// bge
// if (sreg_t(RS1) >= sreg_t(RS2))
//   set_pc(BRANCH_TARGET);

archinst BGE(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_sb(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchConditional);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    dependent_action exec = branchCondAction(inst, tgt, condition(kBGE_), 1);
    connectDependance(inst->retirementDependance(), exec);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);

    rs_deps.push_back(exec.dependance);
    inst->addRetirementEffect(updateConditional(inst));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// bgeu
// if (RS1 >= RS2)
//   set_pc(BRANCH_TARGET);

archinst BGEU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_sb(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchConditional);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    dependent_action exec = branchCondAction(inst, tgt, condition(kBGEU_), 1);
    connectDependance(inst->retirementDependance(), exec);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);

    rs_deps.push_back(exec.dependance);
    inst->addRetirementEffect(updateConditional(inst));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// blt
// if (sreg_t(RS1) < sreg_t(RS2))
//   set_pc(BRANCH_TARGET);

archinst BLT(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_sb(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchConditional);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    dependent_action exec = branchCondAction(inst, tgt, condition(kBLT_), 1);
    connectDependance(inst->retirementDependance(), exec);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);

    rs_deps.push_back(exec.dependance);
    inst->addRetirementEffect(updateConditional(inst));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// bltu
// if (RS1 < RS2)
//   set_pc(BRANCH_TARGET);

archinst BLTU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_sb(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchConditional);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    dependent_action exec = branchCondAction(inst, tgt, condition(kBLTU_), 1);
    connectDependance(inst->retirementDependance(), exec);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);

    rs_deps.push_back(exec.dependance);
    inst->addRetirementEffect(updateConditional(inst));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// bne
// if (RS1 != RS2)
//   set_pc(BRANCH_TARGET);

archinst BNE(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_sb(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchConditional);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    dependent_action exec = branchCondAction(inst, tgt, condition(kBNE_), 1);
    connectDependance(inst->retirementDependance(), exec);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);

    rs_deps.push_back(exec.dependance);
    inst->addRetirementEffect(updateConditional(inst));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// jal
// reg_t tmp = npc;
// set_pc(JUMP_TARGET);
// WRITE_RD(tmp);

archinst JAL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_uj(opcode);
    uint64_t npc = aFetchedOpcode.thePC + 4;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeCALL);

    VirtualMemoryAddress tgt(aFetchedOpcode.thePC + imm);

    std::vector<std::list<InternalDependance>> rs_deps[1];
    predicated_action exec = addExecute(inst, operation(kMOV_), rs_deps);

    addReadConstant(inst, 1, npc, rs_deps[0]);
    addDestination(inst, rd, exec, true);

    inst->addDispatchEffect(branch(inst, tgt));
    inst->addRetirementEffect(updateCall(inst, tgt));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// jalr
// reg_t tmp = npc;
// set_pc((RS1 + insn.i_imm()) & ~reg_t(1));
// WRITE_RD(tmp);

archinst JALR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    uint64_t npc = aFetchedOpcode.thePC + 4;
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsBranch, codeBranchIndirectCall);

    std::vector<std::list<InternalDependance>> rs_deps[1];
    simple_action tgr = calcAddressAction(inst, rs_deps);
    dependant_action br = branchRegAction(inst, kAddress, kIndirectCall);
    connectDependance(br.dependance, tgt);
    connectDependance(inst->retirementDependance(), br);

    std::unique_ptr<Operation> mov = operation(kMOV_);
    mov->setOperands(npc);
    predicated_action exec = addExecute(inst, std::move(mov), rs_deps)
    addDestination(inst, rd, exec, true);

    readRegister(inst, 1, rs1, rs_deps[0], true);

    inst->addRetirementEffect(updateIndirect(inst,kAddress, kIndirectCall));
    inst->addPostvalidation(validatePC(inst));

    return inst;
}

// lb
// WRITE_RD(MMU.load<int8_t>(RS1 + insn.i_imm()));

archinst LB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(8), kSignExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(8), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// lbu
// WRITE_RD(MMU.load<uint8_t>(RS1 + insn.i_imm()));

archinst LBU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(8), kZeroExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(8), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// ld
// require_rv64;
// WRITE_RD(MMU.load<int64_t>(RS1 + insn.i_imm()));

archinst LD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(64), kZeroExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(64), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// lh
// WRITE_RD(MMU.load<int16_t>(RS1 + insn.i_imm()));

archinst LH(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(16), kZeroExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(16), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// lhu
// WRITE_RD(MMU.load<uint16_t>(RS1 + insn.i_imm()));

archinst LHU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(16), kSignExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(16), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// lui
// WRITE_RD(insn.u_imm());

archinst LUI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_u(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    addReadConstant(inst, 1, imm, rs_deps[0]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// lw
// WRITE_RD(MMU.load<int32_t>(RS1 + insn.i_imm()));

archinst LW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(32), kSignExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(32), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// lwu
// require_rv64;
// WRITE_RD(MMU.load<uint32_t>(RS1 + insn.i_imm()));

archinst LWU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps[2];
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSopAddressOffset, imm);
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));

    predicated_dependent_action load = loadAction(inst, dbsize(32), kZeroExtend, kPD);

    addDestination(inst, rd, load, true);

    inst->addDispatchEffect(allocateLoad(inst, dbsize(32), load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    return inst;
}

// or
// WRITE_RD(RS1 | RS2);

archinst OR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// ori
// // prefetch.i/r/w hint when rd = 0 and i_imm[4:0] = 0/1/3
// WRITE_RD(insn.i_imm() | RS1);

archinst ORI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sb
// MMU.store<uint8_t>(RS1 + insn.s_imm(), RS2);

archinst SB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_s(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsStore, codeStore);

    std::vector<std::list<InternalDependance>> rs_deps[2], st_deps;
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSipAddressOffset, imm);

    predicated_dependant_action st = updateStoreValueAction(inst, kOprand5);
    st_deps.push_back(st.dependance);
    connectDependance(inst->retirementDependance(), st);

    readRegister(inst, 5, rs2, rs_deps[0], true);

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));
    inst->addCommitEffect(commitStore(inst));

    inst->addDispatchEffect(allocateStore(inst, dbsize(8), false, kAccType_NORMAL));
    inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    inst->addRetirementConstraint(sideEffectStoreConstrant(inst));
    inst->addPostvalidation(validateMemory(kAddress, kOprand5, dbsize(8), inst));

    return inst;
}

// sd
// require_rv64;
// MMU.store<uint64_t>(RS1 + insn.s_imm(), RS2);

archinst SD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_s(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsStore, codeStore);

    std::vector<std::list<InternalDependance>> rs_deps[2], st_deps;
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSipAddressOffset, imm);

    predicated_dependant_action st = updateStoreValueAction(inst, kOprand5);
    st_deps.push_back(st.dependance);
    connectDependance(inst->retirementDependance(), st);

    readRegister(inst, 5, rs2, rs_deps[0], true);

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));
    inst->addCommitEffect(commitStore(inst));

    inst->addDispatchEffect(allocateStore(inst, dbsize(64), false, kAccType_NORMAL));
    inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    inst->addRetirementConstraint(sideEffectStoreConstrant(inst));
    inst->addPostvalidation(validateMemory(kAddress, kOprand5, dbsize(64), inst));

    return inst;
}

// sh
// MMU.store<uint16_t>(RS1 + insn.s_imm(), RS2);

archinst SH(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_s(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsStore, codeStore);

    std::vector<std::list<InternalDependance>> rs_deps[2], st_deps;
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSipAddressOffset, imm);

    predicated_dependant_action st = updateStoreValueAction(inst, kOprand5);
    st_deps.push_back(st.dependance);
    connectDependance(inst->retirementDependance(), st);

    readRegister(inst, 5, rs2, rs_deps[0], true);

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));
    inst->addCommitEffect(commitStore(inst));

    inst->addDispatchEffect(allocateStore(inst, dbsize(16), false, kAccType_NORMAL));
    inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    inst->addRetirementConstraint(sideEffectStoreConstrant(inst));
    inst->addPostvalidation(validateMemory(kAddress, kOprand5, dbsize(16), inst));

    return inst;
}

// sll
// WRITE_RD(sext_xlen(RS1 << (RS2 & (xlen-1))));

archinst SLL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// slli
// require(SHAMT < xlen);
// WRITE_RD(sext_xlen(RS1 << SHAMT));

archinst SLLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_shamt(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// slliw
// require_rv64;
// WRITE_RD(sext32(RS1 << SHAMT));

archinst SLLIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_shamt(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sllw
// require_rv64;
// WRITE_RD(sext32(RS1 << (RS2 & 0x1F)));

archinst SLLW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// slt
// WRITE_RD(sreg_t(RS1) < sreg_t(RS2));

archinst SLT(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// slti
// WRITE_RD(sreg_t(RS1) < sreg_t(insn.i_imm()));

archinst SLTI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sltiu
// WRITE_RD(RS1 < reg_t(insn.i_imm()));

archinst SLTIU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sltu
// WRITE_RD(RS1 < RS2);

archinst SLTU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sra
// WRITE_RD(sext_xlen(sext_xlen(RS1) >> (RS2 & (xlen-1))));

archinst SRA(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// srai
// require(SHAMT < xlen);
// WRITE_RD(sext_xlen(sext_xlen(RS1) >> SHAMT));

archinst SRAI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_shamt(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sraiw
// require_rv64;
// WRITE_RD(sext32(int32_t(RS1) >> SHAMT));

archinst SRAIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_shamt(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sraw
// require_rv64;
// WRITE_RD(sext32(int32_t(RS1) >> (RS2 & 0x1F)));

archinst SRAW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// srl
// WRITE_RD(sext_xlen(zext_xlen(RS1) >> (RS2 & (xlen-1))));

archinst SRL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// srli
// require(SHAMT < xlen);
// WRITE_RD(sext_xlen(zext_xlen(RS1) >> SHAMT));

archinst SRLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_shamt(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// srliw
// require_rv64;
// WRITE_RD(sext32((uint32_t)RS1 >> SHAMT));

archinst SRLIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_shamt(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// srlw
// require_rv64;
// WRITE_RD(sext32((uint32_t)RS1 >> (RS2 & 0x1F)));

archinst SRLW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sub
// WRITE_RD(sext_xlen(RS1 - RS2));

archinst SUB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// subw
// require_rv64;
// WRITE_RD(sext32(RS1 - RS2));

archinst SUBW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sw
// MMU.store<uint32_t>(RS1 + insn.s_imm(), RS2);

archinst SW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    int64_t  imm = gen_imm_s(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsStore, codeStore);

    std::vector<std::list<InternalDependance>> rs_deps[2], st_deps;
    simple_action agu = addAddressCompute(inst, rs_deps);

    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    inst->setOperand(kSipAddressOffset, imm);

    predicated_dependant_action st = updateStoreValueAction(inst, kOprand5);
    st_deps.push_back(st.dependance);
    connectDependance(inst->retirementDependance(), st);

    readRegister(inst, 5, rs2, rs_deps[0], true);

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSeuqshEffect(eraseLSQ(inst));
    inst->addCommitEffect(commitStore(inst));

    inst->addDispatchEffect(allocateStore(inst, dbsize(32), false, kAccType_NORMAL));
    inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    inst->addRetirementConstraint(sideEffectStoreConstrant(inst));
    inst->addPostvalidation(validateMemory(kAddress, kOprand5, dbsize(32), inst));

    return inst;
}

// xor
// WRITE_RD(RS1 ^ RS2);

archinst XOR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// xori
// WRITE_RD(insn.i_imm() ^ RS1);

archinst XORI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_i(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// fence

archinst FENCE(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    // for simplicity now
    inst->setClass(classMEMBAR, codeMEMBARSync);

    inst->addRetirementConstraint(membarSyncConstraint(inst));
    inst->addDispatchEffect(allocateMEMBAR(inst, kAddType_ATOMIC));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(eraseLSQ(inst));
    inst->setMayCommit(false);

    return inst;
}

// fence_i
// MMU.flush_icache();

archinst FENCE_I(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    return inst;
}

// amoadd_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs + RS2; }));

archinst AMOADD_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsAtomic, codeCAS);
    inst->setExclusive();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    addAddressCompute(inst, rs_deps);
    addRedXRegister(inst, 3, rs1, rs_deps[0], true);

    predicated_dependant_action amo = amoAction(inst, kDoubleWord, kNoExtension, kPD);

    inst->addDispatchEffect(allocateCAS(inst, kDoubleWord, amo.dependance, kAccType_ATOMICRW));
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addCommitEffect(accessMem(inst));
    inst->setMayCommit(false);

    multiply_dependant_action upd = updateAMOValueAction(inst, kOperand1, kOperand2, kAMOADD_);
    connectDependance(inst->retirementDependance(), upd);

    std::vector<std::list<InternalDependance>> st_dep(1);
    st_dep[0].push_back(upd.dependances[0]);
    addReadXRegister(inst, 2, rs1, st_dep[0], true);

    std::vector<std::list<InternalDependance>> am_dep(1);
    am_dep[0].push_back(upd.dependances[1]);
    addReadXRegister(inst, 1, rs2, am_dep[0], true);

    return inst;
}

// amoadd_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs + RS2; })));

archinst AMOADD_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoand_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs & RS2; }));

archinst AMOAND_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoand_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs & RS2; })));

archinst AMOAND_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amomax_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](int64_t lhs) { return std::max(lhs, int64_t(RS2)); }));

archinst AMOMAX_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amomaxu_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return std::max(lhs, RS2); }));

archinst AMOMAXU_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amomaxu_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return std::max(lhs, uint32_t(RS2)); })));

archinst AMOMAXU_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amomax_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](int32_t lhs) { return std::max(lhs, int32_t(RS2)); })));

archinst AMOMAX_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amomin_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](int64_t lhs) { return std::min(lhs, int64_t(RS2)); }));

archinst AMOMIN_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amominu_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return std::min(lhs, RS2); }));

archinst AMOMINU_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amominu_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return std::min(lhs, uint32_t(RS2)); })));

archinst AMOMINU_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amomin_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](int32_t lhs) { return std::min(lhs, int32_t(RS2)); })));

archinst AMOMIN_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoor_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs | RS2; }));

archinst AMOOR_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoor_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs | RS2; })));

archinst AMOOR_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoswap_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t UNUSED lhs) { return RS2; }));

archinst AMOSWAP_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoswap_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t UNUSED lhs) { return RS2; })));

archinst AMOSWAP_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoxor_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs ^ RS2; }));

archinst AMOXOR_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// amoxor_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs ^ RS2; })));

archinst AMOXOR_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// lr_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.load_reserved<int64_t>(RS1));

archinst LR_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// lr_w
// require_extension('A');
// WRITE_RD(MMU.load_reserved<int32_t>(RS1));

archinst LR_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sc_d
// require_extension('A');
// require_rv64;
// bool have_reservation = MMU.store_conditional<uint64_t>(RS1, RS2);
// WRITE_RD(!have_reservation);

archinst SC_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// sc_w
// require_extension('A');
// bool have_reservation = MMU.store_conditional<uint32_t>(RS1, RS2);
// WRITE_RD(!have_reservation);

archinst SC_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// c_add
// require_extension(EXT_ZCA);
// require(insn.rvc_rs2() != 0);
// WRITE_RD(sext_xlen(RVC_RS1 + RVC_RS2));

archinst C_ADD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  5);
    uint32_t rs2 = extract32(opcode,  2,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_addi
// require_extension(EXT_ZCA);
// WRITE_RD(sext_xlen(RVC_RS1 + insn.rvc_imm()));

archinst C_ADDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_c(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_addi4spn
// require_extension(EXT_ZCA);
// require(insn.rvc_addi4spn_imm() != 0);
// WRITE_RVC_RS2S(sext_xlen(RVC_SP + insn.rvc_addi4spn_imm()));

archinst C_ADDI4SPN(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs2 = extract32(opcode,  2,  3) + 8;
    int64_t  imm = gen_imm_c_addi4spn(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs2, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    return inst;
}

// c_addw
// require_extension(EXT_ZCA);
// require_rv64;
// WRITE_RVC_RS1S(sext32(RVC_RS1S + RVC_RS2S));

archinst C_ADDW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);

    return inst;
}

// c_and
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S & RVC_RS2S);

archinst C_AND(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);

    return inst;
}

// c_andi
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S & insn.rvc_imm());

archinst C_ANDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    int64_t  imm = gen_imm_c(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    return inst;
}

// c_beqz
// require_extension(EXT_ZCA);
// if (RVC_RS1S == 0)
//   set_pc(pc + insn.rvc_b_imm());

archinst C_BEQZ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    int64_t  imm = gen_imm_c_b(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    return inst;
}

// c_bnez
// require_extension(EXT_ZCA);
// if (RVC_RS1S != 0)
//   set_pc(pc + insn.rvc_b_imm());

archinst C_BNEZ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    int64_t  imm = gen_imm_c_b(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    return inst;
}

// c_ebreak
// require_extension(EXT_ZCA);
// if (!STATE.debug_mode && (
//         (!STATE.v && STATE.prv == PRV_M && STATE.dcsr->ebreakm) ||
//         (!STATE.v && STATE.prv == PRV_S && STATE.dcsr->ebreaks) ||
//         (!STATE.v && STATE.prv == PRV_U && STATE.dcsr->ebreaku) ||
//         (STATE.v && STATE.prv == PRV_S && STATE.dcsr->ebreakvs) ||
//         (STATE.v && STATE.prv == PRV_U && STATE.dcsr->ebreakvu))) {
// 	throw trap_debug_mode();
// } else {
// 	throw trap_breakpoint(STATE.v, pc);
// }

archinst C_EBREAK(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// c_j
// require_extension(EXT_ZCA);
// set_pc(pc + insn.rvc_j_imm());

archinst C_J(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    int64_t  imm = gen_imm_c_j(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    addReadConstant(inst, 1, imm, rs_deps[0]);

    return inst;
}

// c_jal
// require_extension(EXT_ZCA);
// if (xlen == 32) {
//   reg_t tmp = tgt;
//   set_pc(pc + insn.rvc_j_imm());
//   WRITE_REG(X_RA, tmp);
// } else { // c.addiw
//   require(insn.rvc_rd() != 0);
//   WRITE_RD(sext32(RVC_RS1 + insn.rvc_imm()));
// }

archinst C_JAL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  5);
    uint32_t rd  = extract32(opcode,  7,  5);
    uint64_t tgt = 0;
    int64_t  imm = gen_imm_c(opcode);
    int64_t  imm = gen_imm_c_j(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_jalr
// require_extension(EXT_ZCA);
// require(insn.rvc_rs1() != 0);
// reg_t tmp = tgt;
// set_pc(RVC_RS1 & ~reg_t(1));
// WRITE_REG(X_RA, tmp);

archinst C_JALR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  5);
    uint64_t tgt = 0;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], false);

    return inst;
}

// c_jr
// require_extension(EXT_ZCA);
// require(insn.rvc_rs1() != 0);
// set_pc(RVC_RS1 & ~reg_t(1));

archinst C_JR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], false);

    return inst;
}

// c_li
// require_extension(EXT_ZCA);
// WRITE_RD(insn.rvc_imm());

archinst C_LI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_c(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    addReadConstant(inst, 1, imm, rs_deps[0]);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_lui
// require_extension(EXT_ZCA);
// if (insn.rvc_rd() == 2) { // c.addi16sp
//   require(insn.rvc_addi16sp_imm() != 0);
//   WRITE_REG(X_SP, sext_xlen(RVC_SP + insn.rvc_addi16sp_imm()));
// } else if (insn.rvc_imm() != 0) { // c.lui
//   WRITE_RD(insn.rvc_imm() << 12);
// } else if ((insn.rvc_rd() & 1) != 0) { // c.mop.N
//   #include "c_mop_N.h"
// } else {
//   require(false);
// }

archinst C_LUI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_c(opcode);
    int64_t  imm = gen_imm_c_addi16sp(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    addReadConstant(inst, 1, imm, rs_deps[0]);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_lw
// require_extension(EXT_ZCA);
// WRITE_RVC_RS2S(MMU.load<int32_t>(RVC_RS1S + insn.rvc_lw_imm()));

archinst C_LW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;
    int64_t  imm = gen_imm_c_lw(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[3];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);
    addReadConstant(inst, 3, imm, rs_deps[2]);

    return inst;
}

// c_lwsp
// require_extension(EXT_ZCA);
// require(insn.rvc_rd() != 0);
// WRITE_RD(MMU.load<int32_t>(RVC_SP + insn.rvc_lwsp_imm()));

archinst C_LWSP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);
    int64_t  imm = gen_imm_c_lwsp(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    addReadConstant(inst, 1, imm, rs_deps[0]);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_mv
// require_extension(EXT_ZCA);
// require(insn.rvc_rs2() != 0);
// WRITE_RD(RVC_RS2);

archinst C_MV(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs2 = extract32(opcode,  2,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs2, rs_deps[0], false);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_or
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S | RVC_RS2S);

archinst C_OR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);

    return inst;
}

// c_slli
// require_extension(EXT_ZCA);
// require(insn.rvc_zimm() < xlen);
// WRITE_RD(sext_xlen(RVC_RS1 << insn.rvc_zimm()));

archinst C_SLLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    predicated_action exec;
    addDestination(inst, rd, exec, false);

    return inst;
}

// c_srai
// require_extension(EXT_ZCA);
// require(insn.rvc_zimm() < xlen);
// WRITE_RVC_RS1S(sext_xlen(sext_xlen(RVC_RS1S) >> insn.rvc_zimm()));

archinst C_SRAI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], false);

    return inst;
}

// c_srli
// require_extension(EXT_ZCA);
// require(insn.rvc_zimm() < xlen);
// WRITE_RVC_RS1S(sext_xlen(zext_xlen(RVC_RS1S) >> insn.rvc_zimm()));

archinst C_SRLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], false);

    return inst;
}

// c_sub
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(sext_xlen(RVC_RS1S - RVC_RS2S));

archinst C_SUB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);

    return inst;
}

// c_subw
// require_extension(EXT_ZCA);
// require_rv64;
// WRITE_RVC_RS1S(sext32(RVC_RS1S - RVC_RS2S));

archinst C_SUBW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);

    return inst;
}

// c_sw
// require_extension(EXT_ZCA);
// MMU.store<uint32_t>(RVC_RS1S + insn.rvc_lw_imm(), RVC_RS2S);

archinst C_SW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;
    int64_t  imm = gen_imm_c_lw(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[3];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);
    addReadConstant(inst, 3, imm, rs_deps[2]);

    return inst;
}

// c_swsp
// require_extension(EXT_ZCA);
// MMU.store<uint32_t>(RVC_SP + insn.rvc_swsp_imm(), RVC_RS2);

archinst C_SWSP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs2 = extract32(opcode,  2,  5);
    int64_t  imm = gen_imm_c_swsp(opcode);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs2, rs_deps[0], false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    return inst;
}

// c_xor
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S ^ RVC_RS2S);

archinst C_XOR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode,  7,  3) + 8;
    uint32_t rs2 = extract32(opcode,  2,  3) + 8;

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], false);
    readRegister(inst, 2, rs2, rs_deps[1], false);

    return inst;
}

// div
// require_extension('M');
// sreg_t lhs = sext_xlen(RS1);
// sreg_t rhs = sext_xlen(RS2);
// if (rhs == 0)
//   WRITE_RD(UINT64_MAX);
// else if (lhs == INT64_MIN && rhs == -1)
//   WRITE_RD(lhs);
// else
//   WRITE_RD(sext_xlen(lhs / rhs));

archinst DIV(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// divu
// require_extension('M');
// reg_t lhs = zext_xlen(RS1);
// reg_t rhs = zext_xlen(RS2);
// if (rhs == 0)
//   WRITE_RD(UINT64_MAX);
// else
//   WRITE_RD(sext_xlen(lhs / rhs));

archinst DIVU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// divuw
// require_extension('M');
// require_rv64;
// reg_t lhs = zext32(RS1);
// reg_t rhs = zext32(RS2);
// if (rhs == 0)
//   WRITE_RD(UINT64_MAX);
// else
//   WRITE_RD(sext32(lhs / rhs));

archinst DIVUW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// divw
// require_extension('M');
// require_rv64;
// sreg_t lhs = sext32(RS1);
// sreg_t rhs = sext32(RS2);
// if (rhs == 0)
//   WRITE_RD(UINT64_MAX);
// else
//   WRITE_RD(sext32(lhs / rhs));

archinst DIVW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// mul
// require_either_extension('M', EXT_ZMMUL);
// WRITE_RD(sext_xlen(RS1 * RS2));

archinst MUL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// mulh
// require_either_extension('M', EXT_ZMMUL);
// if (xlen == 64)
//   WRITE_RD(mulh(RS1, RS2));
// else
//   WRITE_RD(sext32((sext32(RS1) * sext32(RS2)) >> 32));

archinst MULH(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// mulhsu
// require_either_extension('M', EXT_ZMMUL);
// if (xlen == 64)
//   WRITE_RD(mulhsu(RS1, RS2));
// else
//   WRITE_RD(sext32((sext32(RS1) * reg_t((uint32_t)RS2)) >> 32));

archinst MULHSU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// mulhu
// require_either_extension('M', EXT_ZMMUL);
// if (xlen == 64)
//   WRITE_RD(mulhu(RS1, RS2));
// else
//   WRITE_RD(sext32(((uint64_t)(uint32_t)RS1 * (uint64_t)(uint32_t)RS2) >> 32));

archinst MULHU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// mulw
// require_either_extension('M', EXT_ZMMUL);
// require_rv64;
// WRITE_RD(sext32(RS1 * RS2));

archinst MULW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// rem
// require_extension('M');
// sreg_t lhs = sext_xlen(RS1);
// sreg_t rhs = sext_xlen(RS2);
// if (rhs == 0)
//   WRITE_RD(lhs);
// else if (lhs == INT64_MIN && rhs == -1)
//   WRITE_RD(0);
// else
//   WRITE_RD(sext_xlen(lhs % rhs));

archinst REM(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// remu
// require_extension('M');
// reg_t lhs = zext_xlen(RS1);
// reg_t rhs = zext_xlen(RS2);
// if (rhs == 0)
//   WRITE_RD(sext_xlen(RS1));
// else
//   WRITE_RD(sext_xlen(lhs % rhs));

archinst REMU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// remuw
// require_extension('M');
// require_rv64;
// reg_t lhs = zext32(RS1);
// reg_t rhs = zext32(RS2);
// if (rhs == 0)
//   WRITE_RD(sext32(lhs));
// else
//   WRITE_RD(sext32(lhs % rhs));

archinst REMUW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// remw
// require_extension('M');
// require_rv64;
// sreg_t lhs = sext32(RS1);
// sreg_t rhs = sext32(RS2);
// if (rhs == 0)
//   WRITE_RD(lhs);
// else
//   WRITE_RD(sext32(lhs % rhs));

archinst REMW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rs2 = extract32(opcode, 20,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[2];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    readRegister(inst, 2, rs2, rs_deps[1], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// csrrc
// bool write = insn.rs1() != 0;
// int csr = validate_csr(insn.csr(), write);
// reg_t old = p->get_csr(csr, insn, write);
// if (write) {
//   p->put_csr(csr, old & ~RS1);
// }
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRC(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// csrrci
// bool write = insn.rs1() != 0;
// int csr = validate_csr(insn.csr(), write);
// reg_t old = p->get_csr(csr, insn, write);
// if (write) {
//   p->put_csr(csr, old & ~(reg_t)insn.rs1());
// }
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRCI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// csrrs
// bool write = insn.rs1() != 0;
// int csr = validate_csr(insn.csr(), write);
// reg_t old = p->get_csr(csr, insn, write);
// if (write) {
//   p->put_csr(csr, old | RS1);
// }
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRS(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// csrrsi
// bool write = insn.rs1() != 0;
// int csr = validate_csr(insn.csr(), write);
// reg_t old = p->get_csr(csr, insn, write);
// if (write) {
//   p->put_csr(csr, old | insn.rs1());
// }
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRSI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// csrrw
// int csr = validate_csr(insn.csr(), true);
// reg_t old = p->get_csr(csr, insn, true);
// p->put_csr(csr, RS1);
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rs1 = extract32(opcode, 15,  5);
    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    std::vector<std::list<InternalDependance>> rs_deps[1];
    readRegister(inst, 1, rs1, rs_deps[0], true);
    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// csrrwi
// int csr = validate_csr(insn.csr(), true);
// reg_t old = p->get_csr(csr, insn, true);
// p->put_csr(csr, insn.rs1());
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRWI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;

    uint32_t rd  = extract32(opcode,  7,  5);

    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();

    predicated_action exec;
    addDestination(inst, rd, exec, true);

    return inst;
}

// dret
// require(STATE.debug_mode);
// set_pc_and_serialize(STATE.dpc->read());
// p->set_privilege(STATE.dcsr->prv, STATE.dcsr->v);
// if (STATE.prv < PRV_M)
//   STATE.mstatus->write(STATE.mstatus->read() & ~MSTATUS_MPRV);
// /* We're not in Debug Mode anymore. */
// STATE.debug_mode = false;
// if (STATE.dcsr->step)
//   STATE.single_step = STATE.STEP_STEPPING;

archinst DRET(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// ebreak
// if (!STATE.debug_mode && (
//         (!STATE.v && STATE.prv == PRV_M && STATE.dcsr->ebreakm) ||
//         (!STATE.v && STATE.prv == PRV_S && STATE.dcsr->ebreaks) ||
//         (!STATE.v && STATE.prv == PRV_U && STATE.dcsr->ebreaku) ||
//         (STATE.v && STATE.prv == PRV_S && STATE.dcsr->ebreakvs) ||
//         (STATE.v && STATE.prv == PRV_U && STATE.dcsr->ebreakvu))) {
// 	throw trap_debug_mode();
// } else {
// 	throw trap_breakpoint(STATE.v, pc);
// }

archinst EBREAK(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// ecall
// switch (STATE.prv)
// {
//   case PRV_U: throw trap_user_ecall();
//   case PRV_S:
//     if (STATE.v)
//       throw trap_virtual_supervisor_ecall();
//     else
//       throw trap_supervisor_ecall();
//   case PRV_M: throw trap_machine_ecall();
//   default: abort();
// }

archinst ECALL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// mret
// require_privilege(PRV_M);
// set_pc_and_serialize(p->get_state()->mepc->read());
// reg_t s = STATE.mstatus->read();
// reg_t prev_prv = get_field(s, MSTATUS_MPP);
// reg_t prev_virt = get_field(s, MSTATUS_MPV);
// if (prev_prv != PRV_M)
//   s = set_field(s, MSTATUS_MPRV, 0);
// s = set_field(s, MSTATUS_MIE, get_field(s, MSTATUS_MPIE));
// s = set_field(s, MSTATUS_MPIE, 1);
// s = set_field(s, MSTATUS_MPP, p->extension_enabled('U') ? PRV_U : PRV_M);
// s = set_field(s, MSTATUS_MPV, 0);
// p->put_csr(CSR_MSTATUS, s);
// p->set_privilege(prev_prv, prev_virt);

archinst MRET(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// sfence_vma
// require_extension('S');
// require_impl(IMPL_MMU);
// if (STATE.v) {
//   if (STATE.prv == PRV_U || get_field(STATE.hstatus->read(), HSTATUS_VTVM))
//     require_novirt();
// } else {
//   require_privilege(get_field(STATE.mstatus->read(), MSTATUS_TVM) ? PRV_M : PRV_S);
// }
// MMU.flush_tlb();

archinst SFENCE_VMA(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// sret
// require_extension('S');
// reg_t prev_hstatus = STATE.hstatus->read();
// if (STATE.v) {
//   if (STATE.prv == PRV_U || get_field(prev_hstatus, HSTATUS_VTSR))
//     require_novirt();
// } else {
//   require_privilege(get_field(STATE.mstatus->read(), MSTATUS_TSR) ? PRV_M : PRV_S);
// }
// reg_t next_pc = p->get_state()->sepc->read();
// set_pc_and_serialize(next_pc);
// reg_t s = STATE.sstatus->read();
// reg_t prev_prv = get_field(s, MSTATUS_SPP);
// s = set_field(s, MSTATUS_SIE, get_field(s, MSTATUS_SPIE));
// s = set_field(s, MSTATUS_SPIE, 1);
// s = set_field(s, MSTATUS_SPP, PRV_U);
// STATE.sstatus->write(s);
// bool prev_virt = STATE.v;
// if (!STATE.v) {
//   if (p->extension_enabled('H')) {
//     prev_virt = get_field(prev_hstatus, HSTATUS_SPV);
//     reg_t new_hstatus = set_field(prev_hstatus, HSTATUS_SPV, 0);
//     STATE.hstatus->write(new_hstatus);
//   }
//   STATE.mstatus->write(set_field(STATE.mstatus->read(), MSTATUS_MPRV, 0));
// }
// p->set_privilege(prev_prv, prev_virt);

archinst SRET(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}

// wfi
// if (STATE.v && STATE.prv == PRV_U) {
//   require_novirt();
// } else if (get_field(STATE.mstatus->read(), MSTATUS_TW)) {
//   require_privilege(PRV_M);
// } else if (STATE.v) { // VS-mode
//   if (get_field(STATE.hstatus->read(), HSTATUS_VTW))
//     require_novirt();
// } else if (p->extension_enabled('S')) {
//   // When S-mode is implemented, then executing WFI in
//   // U-mode causes an illegal instruction exception.
//   require_privilege(PRV_S);
// }
// wfi();

archinst WFI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    uint32_t opcode = aFetchedOpcode.theOpcode;



    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass();



    return inst;
}
#endif

} // namespace nDecoder