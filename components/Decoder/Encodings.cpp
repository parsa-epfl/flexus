#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <components/uArch/uArchInterfaces.hpp>

#include "BitManip.hpp"
#include "Conditions.hpp"
#include "Constraints.hpp"
#include "Instruction.hpp"
#include "SemanticActions.hpp"
#include "SemanticInstruction.hpp"
#include "Validations.hpp"

namespace nDecoder {

using namespace Flexus::SharedTypes;

enum eSrcType {
  kRegZero,
  kRegRA,
  kRegSP,

  kRegRS1,
  kRegRS1C,
  kRegRS1C_S,
  kRegRS1C_SW,
  kRegRS2,
  kRegRS2C,
  kRegRS2C_S,
  kRegRS2C_SW,

  kRegRD,
  kRegRDW,

  kPC,

  kImmI,
  kImmU,
  kImmS,
  kImmC,
  kImmC_12,
  kImmC_ADDI16SP,
  kImmC_ADDI4SPN,
  kImmC_LW,
  kImmC_LD,
  kImmC_LWSP,
  kImmC_LDSP,
  kImmC_SWSP,
  kImmC_SDSP,
  kImmC_J,
  kImmC_B,
  kImmC_Z,
  kImmSB,
  kImmUJ,
  kImmSHAMT,
  kImmH,
  kImmZero
};

class DecodedOpcode {
public:
  DecodedOpcode(uint32_t aOpcode): theOpcode(aOpcode) {
  }

  uint32_t cat() {
    return U( 0,  2);
  }

  uint32_t rs1() {
    return U(15,  5);
  }
  uint32_t rs1c() {
    return U( 7,  5);
  }
  uint32_t rs1c_s() {
    return U( 7,  3) + 8;
  }
  uint32_t rs2() {
    return U(20,  5);
  }
  uint32_t rs2c() {
    return U( 2,  5);
  }
  uint32_t rs2c_s() {
    return U( 2,  3) + 8;
  }
  uint32_t rd() {
    return U( 7,  5);
  }

  uint32_t reg(eSrcType st) {
    switch (st) {
    case kRegZero:
      return 0;
    case kRegRA:
      return 1;
    case kRegSP:
      return 2;
    case kRegRS1:
      return rs1();
    case kRegRS1C:
      return rs1c();
    case kRegRS1C_S:
    case kRegRS1C_SW:
      return rs1c_s();
    case kRegRS2:
      return rs2();
    case kRegRS2C:
      return rs2c();
    case kRegRS2C_S:
    case kRegRS2C_SW:
      return rs2c_s();
    case kRegRD:
    case kRegRDW:
      return rd();
    default:
      assert(false);
      return 0;
    }
  }

  int64_t imm_i() {
    return S(20, 12);
  }
  int64_t imm_u() {
    return S(12, 20) << 12;
  }
  int64_t imm_s() {
    return U( 7,  5) + (S(25, 7) << 5);
  }
  int64_t imm_c() {
    return U( 2,  5) + (S(12, 1) << 5);
  }
  int64_t imm_c_addi16sp() {
    return (U( 6,  1) <<  4) +
           (U( 2,  1) <<  5) +
           (U( 5,  1) <<  6) +
           (U( 3,  2) <<  7) +
           (S(12,  1) <<  9);
  }
  int64_t imm_c_addi4spn()  {
    return (U( 6,  1) <<  2) +
           (U( 5,  1) <<  3) +
           (U(11,  2) <<  4) +
           (U( 7,  4) <<  6);
  }
  int64_t imm_c_lw() {
    return (U( 6,  1) <<  2) +
           (U(10,  3) <<  3) +
           (U( 5,  1) <<  6);
  }
  int64_t imm_c_ld() {
    return (U( 5,  2) <<  6) +
           (U(10,  3) <<  3);
  }
  int64_t imm_c_lwsp() {
    return (U( 4,  3) <<  2) +
           (U(12,  1) <<  5) +
           (U( 2,  2) <<  6);
  }
  int64_t imm_c_ldsp() {
    return (U( 5,  2) <<  3) +
           (U(12,  1) <<  5) +
           (U( 2,  3) <<  6);
  }
  int64_t imm_c_swsp() {
    return (U( 9,  4) <<  2) +
           (U( 7,  2) <<  6);
  }
  int64_t imm_c_sdsp() {
    return (U(10,  3) <<  3) +
           (U( 7,  3) <<  6);
  }
  int64_t imm_c_j() {
    return (U( 3,  3) <<  1) +
           (U(11,  1) <<  4) +
           (U( 2,  1) <<  5) +
           (U( 7,  1) <<  6) +
           (U( 6,  1) <<  7) +
           (U( 9,  2) <<  8) +
           (U( 8,  1) << 10) +
           (S(12,  1) << 11);
  }
  int64_t imm_c_b() {
    return (U( 3,  2) <<  1) +
           (U(10,  2) <<  3) +
           (U( 2,  1) <<  5) +
           (U( 5,  2) <<  6) +
           (S(12,  1) <<  8);
  }
  int64_t imm_c_z() {
    return (U( 2,  5)      ) +
           (U(12,  1) <<  5);
  }
  int64_t imm_sb() {
    return (U( 8,  4) <<  1) +
           (U(25,  6) <<  5) +
           (U( 7,  1) << 11) +
           (S(31,  1) << 12);
  }
  int64_t imm_uj() {
    return (U(21, 10) <<  1) +
           (U(20,  1) << 11) +
           (U(12,  8) << 12) +
           (S(31,  1) << 20);
  }
  int64_t imm_shamt() {
    return  U(20,  6);
  }

  int64_t imm(eSrcType st) {
    switch (st) {
    case kImmI:
      return imm_i();
    case kImmU:
      return imm_u();
    case kImmS:
      return imm_s();
    case kImmC:
      return imm_c();
    case kImmC_12:
      return imm_c() << 12;
    case kImmC_ADDI16SP:
      return imm_c_addi16sp();
    case kImmC_ADDI4SPN:
      return imm_c_addi4spn();
    case kImmC_LW:
      return imm_c_lw();
    case kImmC_LD:
      return imm_c_ld();
    case kImmC_LWSP:
      return imm_c_lwsp();
    case kImmC_LDSP:
      return imm_c_ldsp();
    case kImmC_SWSP:
      return imm_c_swsp();
    case kImmC_SDSP:
      return imm_c_sdsp();
    case kImmC_J:
      return imm_c_j();
    case kImmC_B:
      return imm_c_b();
    case kImmC_Z:
      return imm_c_z();
    case kImmSB:
      return imm_sb();
    case kImmUJ:
      return imm_uj();
    case kImmSHAMT:
      return imm_shamt();
    case kImmZero:
      return 0;
    default:
      assert(false);
      return 0;
    }
  }

private:
  uint64_t U(int lo, int len) {
    return ((uint64_t)(theOpcode) >> lo) & ((1lu << len) - 1);
  }

  uint64_t S(int lo, int len) {
    auto x = 64 - len;

    return (( int64_t)(theOpcode) << (x - lo)) >> x;
  }

  uint32_t theOpcode;
};

void setRS(SemanticInstruction *inst, eOperandCode rs_code, uint32_t rs) {
  reg regRS;
  regRS.theType = xRegisters;
  regRS.theIndex = rs;
  inst->setOperand(rs_code, regRS);
}

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

  connectDependance(aWritebackDependance, annul);

  inst->addAnnulmentEffect(satisfy(inst, annul.predicate));
  inst->addAnnulmentEffect(squash(inst, exec.predicate));
  inst->addReinstatementEffect(squash(inst, annul.predicate));
  inst->addReinstatementEffect(satisfy(inst, exec.predicate));
}

void addWriteback(SemanticInstruction *inst, eOperandCode aRegisterCode,
                  eOperandCode aMappedRegisterCode, predicated_action &exec, eSignCode aSign,
                  bool addSquash) {
  if (addSquash) {
    inst->addDispatchEffect(mapDestination(inst));
  } else {
    inst->addDispatchEffect(mapDestination_NoSquashEffects(inst));
  }

  dependant_action wb =
      writebackAction(inst, aRegisterCode, aMappedRegisterCode, aSign);
  addAnnulment(inst, exec, wb.dependance);

  // Make writeback depend on execute, make retirement depend on writeback
  connectDependance(wb.dependance, exec);
  connectDependance(inst->retirementDependance(), wb);
}

void addReadConstant(SemanticInstruction *inst, int32_t anOpNumber, uint64_t val,
                     std::list<InternalDependance> &dependances) {

  DBG_Assert(anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3 || anOpNumber == 4 ||
             anOpNumber == 5);
  eOperandCode cOperand = eOperandCode(kOperand1 + anOpNumber - 1);

  DECODER_DBG("Reading constant "
              << "0x" << std::hex << val << std::dec << " to " << cOperand);

  simple_action act = readConstantAction(inst, val, cOperand);
  connect(dependances, act);
  inst->addDispatchAction(act);
}

simple_action addReadRegister(SemanticInstruction *inst, int32_t anOpNumber, uint32_t rs,
                     std::list<InternalDependance> &dependances) {

  // Calculate operand codes from anOpNumber
  DBG_Assert(anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3 || anOpNumber == 4 ||
             anOpNumber == 5);
  eOperandCode cOperand = eOperandCode(kOperand1 + anOpNumber - 1);
  eOperandCode cRS = eOperandCode(kRS1 + anOpNumber - 1);
  eOperandCode cPS = eOperandCode(kPS1 + anOpNumber - 1);

  DECODER_DBG("Reading x[" << rs << "] and mapping it [ " << cRS << " -> " << cPS << " ]");

  setRS(inst, cRS, rs);
  inst->addDispatchEffect(mapSource(inst, cRS, cPS));
  simple_action act = readRegisterAction(inst, cPS, cOperand);
  connect(dependances, act);
  inst->addDispatchAction(act);
  inst->addPrevalidation(validateXRegister(rs, cOperand, inst));
  return act;
}

void addDestination(SemanticInstruction *inst, uint32_t rd, predicated_action &exec, eSignCode sign,
                    bool addSquash = true) {
  if (rd) {
    setRD(inst, rd);
    addWriteback(inst, kResult, kPD, exec, sign, addSquash);
    inst->addPostvalidation(validateXRegister(rd, kResult, inst));

  } else {
    predicated_action annul = annulAction(inst);

    inst->addAnnulmentEffect    (satisfy(inst, annul.predicate));
    inst->addAnnulmentEffect    (squash (inst,  exec.predicate));
    inst->addReinstatementEffect(satisfy(inst,  exec.predicate));
    inst->addReinstatementEffect(squash (inst, annul.predicate));

    connectDependance(inst->retirementDependance(), exec);
  }
}

predicated_action addExecute(SemanticInstruction *inst, std::unique_ptr<Operation> anOperation,
                             std::vector<std::list<InternalDependance>> &rs_deps,
                             int operandOffSet = 0,
                             eOperandCode aResult = kResult,
                             boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none)) {
  return executeAction(inst, anOperation, rs_deps, aResult, aBypass, operandOffSet);
}

predicated_action addExecute(SemanticInstruction *inst, std::unique_ptr<Operation> anOperation,
                             std::vector<eOperandCode> anOperands,
                             std::vector<std::list<InternalDependance>> &rs_deps,
                             eOperandCode aResult = kResult,
                             boost::optional<eOperandCode> aBypass = boost::optional<eOperandCode>(boost::none)) {
  return executeAction(inst, anOperation, anOperands, rs_deps, aResult, aBypass);
}

simple_action addAddressCompute(SemanticInstruction *inst,
                                std::vector<std::list<InternalDependance>> &rs_deps) {
  DECODER_TRACE;

  simple_action tr = translationAction(inst);
  multiply_dependant_action update_address = updateVirtualAddressAction(inst);
  inst->addDispatchEffect(satisfy(inst, update_address.dependances[1]));
  simple_action exec = calcAddressAction(inst, rs_deps);

  connectDependance(update_address.dependances[0], exec);
  connectDependance(tr.action->dependance(0), exec);
  connectDependance(inst->retirementDependance(), update_address);

  inst->addRetirementConstraint(paddrResolutionConstraint(inst));

  return exec;
}

struct BlackBoxInstruction : public ArchInstruction {

  BlackBoxInstruction(VirtualMemoryAddress aPC, Opcode anOpcode,
                      boost::intrusive_ptr<BPredState> aBPState, uint32_t aCPU, int64_t aSequenceNo)
      : ArchInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo) {
    setClass(clsSynchronizing, codeBlackBox);
    forceResync();
  }

  virtual bool mayRetire() const {
    return true;
  }

  virtual bool postValidate() {
    return false;
  }

  virtual void setOperand(uint32_t idx, uint64_t val) {
  }

  virtual void describe(std::ostream &anOstream) const {
    ArchInstruction::describe(anOstream);
    anOstream << instCode();
  }
};

archinst blackBox(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return archinst(new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                          aFetchedOpcode.theBPState, aCPU, aSequenceNo));
}

// see: ADDSUB_CARRY
static archinst ALX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                    eOpType   op,
                    eSrcType  st1,
                    eSrcType  st2,
                    eSrcType  dt,
                    eSignCode sc) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsComputation, codeALU);

  std::vector<std::list<InternalDependance>> rs_deps(1 + (st2 != kRegZero));
  predicated_action alu = addExecute(inst, operation(op), rs_deps);

  switch (st1) {
  case kRegSP:
  case kRegRS1:
  case kRegRS1C:
  case kRegRS1C_S:
  case kRegRS2C:
  case kRegRS2C_S:
    addReadRegister(inst, 1, opcode.reg(st1), rs_deps[0]);
    break;
  case kPC:
    addReadConstant(inst, 1, aFetchedOpcode.thePC, rs_deps[0]);
    break;
  default:
    addReadConstant(inst, 1, opcode.imm(st1), rs_deps[0]);
  }

  switch (st2) {
  case kRegZero:
    break;
  case kRegRS2:
  case kRegRS2C:
  case kRegRS2C_S:
    addReadRegister(inst, 2, opcode.reg(st2), rs_deps[1]);
    break;
  case kImmH:
    addReadConstant(inst, 2, aCPU,            rs_deps[1]);
    break;
  default:
    addReadConstant(inst, 2, opcode.imm(st2), rs_deps[1]);
  }

  switch (dt) {
  case kRegSP:
  case kRegRD:
  case kRegRS1C_S:
  case kRegRS2C_S:
    addDestination(inst, opcode.reg(dt), alu, kNoExtension);
    break;
  case kRegRDW:
  case kRegRS1C_SW:
  case kRegRS2C_SW:
    addDestination(inst, opcode.reg(dt), alu, sc);
    break;
  default:
    assert(false);
  }

  return inst;
}

// add
// WRITE_RD(sext_xlen(RS1 + RS2));

archinst ADD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// addi
// WRITE_RD(sext_xlen(RS1 + insn.i_imm()));

archinst ADDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegRS1, kImmI, kRegRD, kNoExtension);
}

// addiw
// require_rv64;
// WRITE_RD(sext32(insn.i_imm() + RS1));

archinst ADDIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADDW, kRegRS1, kImmI, kRegRDW, kSignExtend);
}

// addw
// require_rv64;
// WRITE_RD(sext32(RS1 + RS2));

archinst ADDW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
}

// and
// WRITE_RD(RS1 & RS2);

archinst AND(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kAND, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// andi
// WRITE_RD(insn.i_imm() & RS1);

archinst ANDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kAND, kRegRS1, kImmI, kRegRD, kNoExtension);
}

// auipc
// WRITE_RD(sext_xlen(insn.u_imm() + pc));

archinst AUIPC(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kPC, kImmU, kRegRD, kNoExtension);
}

// see: CMPBR/branch_cond
static archinst BX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                   eInstructionCode ic,
                   eCondCode        cc,
                   eSrcType         st1,
                   eSrcType         st2,
                   eSrcType         sti) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsBranch, ic);

  VirtualMemoryAddress tgt((uint64_t)(aFetchedOpcode.thePC) + opcode.imm(sti));

  std::vector<std::list<InternalDependance>> rs_deps(2);
  dependant_action br = branchCondAction(inst, tgt, condition(cc), rs_deps);

  switch (st1) {
  case kRegRS1:
  case kRegRS1C_S:
    addReadRegister(inst, 1, opcode.reg(st1), rs_deps[0]);
    break;
  default:
    assert(false);
  }

  switch (st2) {
  case kRegRS2:
    addReadRegister(inst, 2, opcode.reg(st2), rs_deps[1]);
    break;
  case kRegZero:
    addReadConstant(inst, 2, 0, rs_deps[1]);
    break;
  default:
    assert(false);
  }

  connectDependance(inst->retirementDependance(), br);

  inst->addRetirementEffect(updateConditional(inst));
  inst->addPostvalidation  (validatePC(inst));

  return inst;
}

// beq
// if (RS1 == RS2)
//   set_pc(BRANCH_TARGET);

archinst BEQ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBEQ, kRegRS1, kRegRS2, kImmSB);
}

// bge
// if (sreg_t(RS1) >= sreg_t(RS2))
//   set_pc(BRANCH_TARGET);

archinst BGE(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBGE, kRegRS1, kRegRS2, kImmSB);
}

// bgeu
// if (RS1 >= RS2)
//   set_pc(BRANCH_TARGET);

archinst BGEU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBGEU, kRegRS1, kRegRS2, kImmSB);
}

// blt
// if (sreg_t(RS1) < sreg_t(RS2))
//   set_pc(BRANCH_TARGET);

archinst BLT(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBLT, kRegRS1, kRegRS2, kImmSB);
}

// bltu
// if (RS1 < RS2)
//   set_pc(BRANCH_TARGET);

archinst BLTU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBLTU, kRegRS1, kRegRS2, kImmSB);
}

// bne
// if (RS1 != RS2)
//   set_pc(BRANCH_TARGET);

archinst BNE(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBNE, kRegRS1, kRegRS2, kImmSB);
}

// see: UNCONDBR
static archinst JX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                   eInstructionCode ic,
                   eSrcType         sti,
                   eSrcType         dt) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsBranch, ic);

  VirtualMemoryAddress tgt((uint64_t)(aFetchedOpcode.thePC) + opcode.imm(sti));

  switch (dt) {
  case kRegRD: {
    if (opcode.rd() == 0)
      break;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    predicated_action mov = addExecute(inst, operation(kMOV), rs_deps);

    addReadConstant(inst, 1, aFetchedOpcode.thePC + (opcode.cat() == 3 ? 4 : 2), rs_deps[0]);
    addDestination (inst, opcode.rd(), mov, kNoExtension);
    break;
  }
  case kRegZero:
    break;
  default:
    assert(false);
  }

  inst->addDispatchEffect  (branch(inst, tgt));
  inst->addRetirementEffect(updateUnconditional(inst, tgt));
  inst->addPostvalidation  (validatePC(inst));

  return inst;
}

// jal
// reg_t tmp = npc;
// set_pc(JUMP_TARGET);
// WRITE_RD(tmp);

archinst JAL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return JX(aFetchedOpcode, aCPU, aSequenceNo, codeCALL, kImmUJ, kRegRD);
}

// see: BLR
static archinst JRX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                    eSrcType st1,
                    eSrcType sti,
                    eSrcType dt) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsBranch, codeBranchIndirectCall);

  std::vector<std::list<InternalDependance>> rs_deps(1 + (sti != kImmZero));
  predicated_action alu = addExecute(inst, operation(sti != kImmZero ? kADD : kMOV), rs_deps, 0, kAddress);

  switch (st1) {
  case kRegRS1:
  case kRegRS1C:
    addReadRegister(inst, 1, opcode.reg(st1), rs_deps[0]);
    break;
  default:
    assert(false);
  }

  switch (sti) {
  case kImmZero:
    break;
  default:
    addReadConstant(inst, 2, opcode.imm(sti), rs_deps[1]);
    break;
  }

  dependant_action br = branchRegAction(inst, kAddress, kIndirectCall);
  connectDependance(br.dependance, alu);

  switch (dt) {
  case kRegRD:
  case kRegRA: {
    std::vector<std::list<InternalDependance>> rd_deps(1);
    predicated_action mov = addExecute(inst, operation(kMOV), rd_deps, 2);

    addReadConstant(inst, 3, aFetchedOpcode.thePC + (opcode.cat() == 3 ? 4 : 2), rd_deps[0]);
    addDestination (inst, opcode.reg(dt), mov, kNoExtension);
    break;
  }
  case kRegZero:
    break;
  default:
    assert(false);
  }

  connectDependance(inst->retirementDependance(), br);

  inst->addRetirementEffect(updateIndirect(inst, kAddress, kIndirectCall));
  inst->addPostvalidation  (validatePC(inst));

  return inst;
}

// jalr
// reg_t tmp = npc;
// set_pc((RS1 + insn.i_imm()) & ~reg_t(1));
// WRITE_RD(tmp);

archinst JALR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return JRX(aFetchedOpcode, aCPU, aSequenceNo, kRegRS1, kImmI, kRegRD);
}

// see: LDR
static archinst LX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                   eSize     sz,
                   eSignCode sc,
                   eSrcType  st1,
                   eSrcType  sti,
                   eSrcType  dt) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass  (clsLoad, codeLoad);
  inst->setOperand(kSopAddressOffset, opcode.imm(sti));

  std::vector<std::list<InternalDependance>> rs_deps(1);
  addAddressCompute(inst, rs_deps);

  switch (st1) {
  case kRegSP:
  case kRegRS1C_S: 
  case kRegRS1:
    addReadRegister(inst, 1, opcode.reg(st1), rs_deps[0]);
    break;
  default:
    assert(false);
  }

  boost::optional<eOperandCode> byp = boost::none;

  if (opcode.reg(dt))
    byp = kPD;

  predicated_dependant_action ld = loadAction(inst, sz, sc, byp);

  switch (dt) {
  case kRegRD:
  case kRegRS2C_S:
    addDestination(inst, opcode.reg(dt), ld, kNoExtension);
    break;
  case kRegRDW:
  case kRegRS2C_SW:
    addDestination(inst, opcode.reg(dt), ld, sc);
    break;
  default:
    assert(false);
    break;
  }

  inst->addDispatchEffect      (allocateLoad(inst, sz, ld.dependance, kAccType_NORMAL));
  inst->addCheckTrapEffect     (mmuPageFaultCheck(inst));
  inst->addCommitEffect        (accessMem(inst));
  inst->addRetirementConstraint(loadMemoryConstraint(inst));
  inst->addRetirementEffect    (retireMem(inst));
  inst->addSquashEffect        (eraseLSQ (inst));

  return inst;
}

// lb
// WRITE_RD(MMU.load<int8_t>(RS1 + insn.i_imm()));

archinst LB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kByte, kSignExtend, kRegRS1, kImmI, kRegRD);
}

// lbu
// WRITE_RD(MMU.load<uint8_t>(RS1 + insn.i_imm()));

archinst LBU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kByte, kZeroExtend, kRegRS1, kImmI, kRegRD);
}

// ld
// require_rv64;
// WRITE_RD(MMU.load<int64_t>(RS1 + insn.i_imm()));

archinst LD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kNoExtension, kRegRS1, kImmI, kRegRD);
}

// lh
// WRITE_RD(MMU.load<int16_t>(RS1 + insn.i_imm()));

archinst LH(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kHalfWord, kSignExtend, kRegRS1, kImmI, kRegRD);
}

// lhu
// WRITE_RD(MMU.load<uint16_t>(RS1 + insn.i_imm()));

archinst LHU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kHalfWord, kZeroExtend, kRegRS1, kImmI, kRegRD);
}

// lui
// WRITE_RD(insn.u_imm());

archinst LUI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMOV, kImmU, kRegZero, kRegRD, kNoExtension);
}

// lw
// WRITE_RD(MMU.load<int32_t>(RS1 + insn.i_imm()));

archinst LW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kSignExtend, kRegRS1, kImmI, kRegRD);
}

// lwu
// require_rv64;
// WRITE_RD(MMU.load<uint32_t>(RS1 + insn.i_imm()));

archinst LWU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kZeroExtend, kRegRS1, kImmI, kRegRD);
}

// or
// WRITE_RD(RS1 | RS2);

archinst OR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kORR, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// ori
// // prefetch.i/r/w hint when rd = 0 and i_imm[4:0] = 0/1/3
// WRITE_RD(insn.i_imm() | RS1);

archinst ORI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kORR, kRegRS1, kImmI, kRegRD, kNoExtension);
}

// see: STR
static archinst SX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                   eSize    sz,
                   eSrcType st1,
                   eSrcType st2,
                   eSrcType sti) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass  (clsStore, codeStore);
  inst->setOperand(kSopAddressOffset, opcode.imm(sti));

  std::vector<std::list<InternalDependance>> rs_deps(1), st_deps(1);
  addAddressCompute(inst, rs_deps);

  switch (st1) {
  case kRegSP:
  case kRegRS1:
  case kRegRS1C_S:
    addReadRegister(inst, 1, opcode.reg(st1), rs_deps[0]);
    break;
  default:
    assert(false);
  }

  predicated_dependant_action st = updateStoreValueAction(inst, kOperand2);
  st_deps[0].push_back(st.dependance);

  switch (st2) {
  case kRegRS2:
  case kRegRS2C:
  case kRegRS2C_S:
    addReadRegister(inst, 2, opcode.reg(st2), st_deps[0]);
    break;
  default:
    assert(false);
  }

  connectDependance(inst->retirementDependance(), st);

  inst->addDispatchEffect      (allocateStore(inst, sz, false, kAccType_NORMAL));
  inst->addCheckTrapEffect     (mmuPageFaultCheck(inst));
  inst->addCommitEffect        (commitStore(inst));
  inst->addRetirementEffect    (retireMem(inst));
  inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
  inst->addRetirementConstraint(sideEffectStoreConstraint(inst));
  inst->addSquashEffect        (eraseLSQ(inst));
  inst->addPostvalidation      (validateMemory(kAddress, kOperand2, sz, inst));

  return inst;
}

// sb
// MMU.store<uint8_t>(RS1 + insn.s_imm(), RS2);

archinst SB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kByte, kRegRS1, kRegRS2, kImmS);
}

// sd
// require_rv64;
// MMU.store<uint64_t>(RS1 + insn.s_imm(), RS2);

archinst SD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kRegRS1, kRegRS2, kImmS);
}

// sh
// MMU.store<uint16_t>(RS1 + insn.s_imm(), RS2);

archinst SH(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kHalfWord, kRegRS1, kRegRS2, kImmS);
}

// sll
// WRITE_RD(sext_xlen(RS1 << (RS2 & (xlen-1))));

archinst SLL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLL, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// slli
// require(SHAMT < xlen);
// WRITE_RD(sext_xlen(RS1 << SHAMT));

archinst SLLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLL, kRegRS1, kImmSHAMT, kRegRD, kNoExtension);
}

// slliw
// require_rv64;
// WRITE_RD(sext32(RS1 << SHAMT));

archinst SLLIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLLW, kRegRS1, kImmSHAMT, kRegRDW, kSignExtend);
}

// sllw
// require_rv64;
// WRITE_RD(sext32(RS1 << (RS2 & 0x1F)));

archinst SLLW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLLW, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
}

// slt
// WRITE_RD(sreg_t(RS1) < sreg_t(RS2));

archinst SLT(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLT, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// slti
// WRITE_RD(sreg_t(RS1) < sreg_t(insn.i_imm()));

archinst SLTI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLT, kRegRS1, kImmI, kRegRD, kNoExtension);
}

// sltiu
// WRITE_RD(RS1 < reg_t(insn.i_imm()));

archinst SLTIU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLTU, kRegRS1, kImmI, kRegRD, kNoExtension);
}

// sltu
// WRITE_RD(RS1 < RS2);

archinst SLTU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLTU, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// sra
// WRITE_RD(sext_xlen(sext_xlen(RS1) >> (RS2 & (xlen-1))));

archinst SRA(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRA, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// srai
// require(SHAMT < xlen);
// WRITE_RD(sext_xlen(sext_xlen(RS1) >> SHAMT));

archinst SRAI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRA, kRegRS1, kImmSHAMT, kRegRD, kNoExtension);
}

// sraiw
// require_rv64;
// WRITE_RD(sext32(int32_t(RS1) >> SHAMT));

archinst SRAIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRAW, kRegRS1, kImmSHAMT, kRegRDW, kSignExtend);
}

// sraw
// require_rv64;
// WRITE_RD(sext32(int32_t(RS1) >> (RS2 & 0x1F)));

archinst SRAW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRAW, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
}

// srl
// WRITE_RD(sext_xlen(zext_xlen(RS1) >> (RS2 & (xlen-1))));

archinst SRL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRL, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// srli
// require(SHAMT < xlen);
// WRITE_RD(sext_xlen(zext_xlen(RS1) >> SHAMT));

archinst SRLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRL, kRegRS1, kImmSHAMT, kRegRD, kNoExtension);
}

// srliw
// require_rv64;
// WRITE_RD(sext32((uint32_t)RS1 >> SHAMT));

archinst SRLIW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRLW, kRegRS1, kImmSHAMT, kRegRDW, kSignExtend);
}

// srlw
// require_rv64;
// WRITE_RD(sext32((uint32_t)RS1 >> (RS2 & 0x1F)));

archinst SRLW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRLW, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
}

// sub
// WRITE_RD(sext_xlen(RS1 - RS2));

archinst SUB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSUB, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// subw
// require_rv64;
// WRITE_RD(sext32(RS1 - RS2));

archinst SUBW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSUBW, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
}

// sw
// MMU.store<uint32_t>(RS1 + insn.s_imm(), RS2);

archinst SW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kRegRS1, kRegRS2, kImmS);
}

// xor
// WRITE_RD(RS1 ^ RS2);

archinst XOR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kXOR, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// xori
// WRITE_RD(insn.i_imm() ^ RS1);

archinst XORI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kXOR, kRegRS1, kImmI, kRegRD, kNoExtension);
}

// fence

// see: MEMBAR
archinst FENCE(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  // for simplicity now
  inst->setClass(clsMEMBAR, codeMEMBARSync);

  inst->setMayCommit(false);

  inst->addDispatchEffect      (allocateMEMBAR(inst, kAccType_ATOMIC));
  inst->addCommitEffect        (eraseLSQ(inst));
  inst->addRetirementConstraint(membarSyncConstraint(inst));
  inst->addRetirementEffect    (retireMem(inst));
  inst->addSquashEffect        (eraseLSQ(inst));

  return inst;
}

// fence_i
// MMU.flush_icache();

archinst FENCE_I(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

// see: CAS
static archinst AMO(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                    eRMWOperation op,
                    eSize         sz,
                    eSrcType      dt) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsAtomic, codeRMW);
  inst->setExclusive();

  std::vector<std::list<InternalDependance>> rs1_dep(1);
  std::vector<std::list<InternalDependance>> rs2_dep(1);
  addAddressCompute(inst, rs1_dep);

  boost::optional<eOperandCode> byp = boost::none;

  if (opcode.rd())
    byp = kPD;

  predicated_dependant_action ld  = amoAction(inst, sz, kSignExtend, byp);
  predicated_dependant_action rmw = updateRMWValueAction(inst, kOperand2, kResult1, kRMW, op);

  addReadRegister(inst, 1, opcode.rs1(), rs1_dep[0]);
  addReadRegister(inst, 2, opcode.rs2(), rs2_dep[0]);

  switch (dt) {
  case kRegRD:
    addDestination(inst, opcode.rd(), ld, kNoExtension);
    break;
  case kRegRDW:
    addDestination(inst, opcode.rd(), ld, kSignExtend);
    break;
  default:
    assert(false);
  }

  connectDependance(rmw.dependance, ld);
  connectDependance(inst->retirementDependance(), rmw);

  inst->setMayCommit(false);

  inst->addDispatchEffect  (allocateRMW(inst, sz, ld.dependance, kAccType_ATOMICRW));
  inst->addCheckTrapEffect (mmuPageFaultCheck(inst));
  inst->addCommitEffect    (accessMem(inst));
  inst->addRetirementEffect(retireMem(inst));
  inst->addSquashEffect    (eraseLSQ(inst));
  inst->addPostvalidation  (validateMemory(kAddress, kResult1, sz, inst));

  return inst;
}

// amoadd_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs + RS2; }));

archinst AMOADD_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOADD, kDoubleWord, kRegRD);
}

// amoadd_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs + RS2; })));

archinst AMOADD_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOADDW, kWord, kRegRDW);
}

// amoand_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs & RS2; }));

archinst AMOAND_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOAND, kDoubleWord, kRegRD);
}

// amoand_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs & RS2; })));

archinst AMOAND_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOANDW, kWord, kRegRDW);
}

// amomax_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](int64_t lhs) { return std::max(lhs, int64_t(RS2)); }));

archinst AMOMAX_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMAX, kDoubleWord, kRegRD);
}

// amomaxu_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return std::max(lhs, RS2); }));

archinst AMOMAXU_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMAXU, kDoubleWord, kRegRD);
}

// amomaxu_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return std::max(lhs, uint32_t(RS2)); })));

archinst AMOMAXU_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMAXUW, kWord, kRegRDW);
}

// amomax_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](int32_t lhs) { return std::max(lhs, int32_t(RS2)); })));

archinst AMOMAX_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMAXW, kWord, kRegRDW);
}

// amomin_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](int64_t lhs) { return std::min(lhs, int64_t(RS2)); }));

archinst AMOMIN_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMIN, kDoubleWord, kRegRD);
}

// amominu_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return std::min(lhs, RS2); }));

archinst AMOMINU_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMINU, kDoubleWord, kRegRD);
}

// amominu_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return std::min(lhs, uint32_t(RS2)); })));

archinst AMOMINU_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMINUW, kWord, kRegRDW);
}

// amomin_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](int32_t lhs) { return std::min(lhs, int32_t(RS2)); })));

archinst AMOMIN_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOMINW, kWord, kRegRDW);
}

// amoor_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs | RS2; }));

archinst AMOOR_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOOR, kDoubleWord, kRegRD);
}

// amoor_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs | RS2; })));

archinst AMOOR_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOORW, kWord, kRegRDW);
}

// amoswap_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t UNUSED lhs) { return RS2; }));

archinst AMOSWAP_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOSWAP, kDoubleWord, kRegRD);
}

// amoswap_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t UNUSED lhs) { return RS2; })));

archinst AMOSWAP_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOSWAPW, kWord, kRegRD);
}

// amoxor_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.amo<uint64_t>(RS1, [&](uint64_t lhs) { return lhs ^ RS2; }));

archinst AMOXOR_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOXOR, kDoubleWord, kRegRD);
}

// amoxor_w
// require_extension('A');
// WRITE_RD(sext32(MMU.amo<uint32_t>(RS1, [&](uint32_t lhs) { return lhs ^ RS2; })));

archinst AMOXOR_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return AMO(aFetchedOpcode, aCPU, aSequenceNo, kAMOXORW, kWord, kRegRDW);
}

// see: LDXR
static archinst LRX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                    eSize     sz,
                    eSignCode sc) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsAtomic, codeLoad);
  inst->setExclusive();

  std::vector<std::list<InternalDependance>> rs_deps(1);
  addAddressCompute(inst, rs_deps);

  boost::optional<eOperandCode> byp = boost::none;

  if (opcode.rd())
    byp = kPD;

  predicated_dependant_action ld = loadAction(inst, sz, kSignExtend, byp);

  addReadRegister(inst, 1, opcode.rs1(), rs_deps[0]);
  addDestination (inst, opcode.rd(), ld, sc);

  inst->addDispatchEffect      (allocateLoad(inst, sz, ld.dependance, kAccType_ORDERED));
  inst->addCheckTrapEffect     (mmuPageFaultCheck(inst));
  inst->addCommitEffect        (accessMem(inst));
  inst->addRetirementConstraint(loadMemoryConstraint(inst));
  inst->addRetirementEffect    (retireMem(inst));
  inst->addRetirementEffect    (markExclusiveMonitor(inst, kAddress, sz));
  inst->addSquashEffect        (eraseLSQ (inst));

  return inst;
}

// lr_d
// require_extension('A');
// require_rv64;
// WRITE_RD(MMU.load_reserved<int64_t>(RS1));

archinst LR_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LRX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kNoExtension);
}

// lr_w
// require_extension('A');
// WRITE_RD(MMU.load_reserved<int32_t>(RS1));

archinst LR_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LRX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kSignExtend);
}

// see: STXR
static archinst SCX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                    eSize sz) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsAtomic, codeStore);
  inst->setExclusive();

  std::vector<std::list<InternalDependance>> rs1_dep(1);
  std::vector<std::list<InternalDependance>> rs2_dep(1);
  simple_action agu = addAddressCompute(inst, rs1_dep);
  simple_action rs2 = addReadRegister  (inst, 2, opcode.rs2(), rs2_dep[0]);

  boost::optional<eOperandCode> byp = boost::none;

  if (opcode.rd())
    byp = kPD;

  predicated_action           mon = exclusiveMonitorAction(inst, kAddress, sz, byp);
  predicated_dependant_action st  = updateStoreValueAction(inst, kOperand2);

  addReadRegister(inst, 1, opcode.rs1(), rs1_dep[0]);
  addDestination (inst, opcode.rd(), mon, kNoExtension);

  connectDependance(mon.action->dependance(0), agu);
  connectDependance(mon.action->dependance(1), rs2);

  connectDependance(st.dependance, mon);
  connectDependance(inst->retirementDependance(), st);

  inst->addDispatchEffect  (allocateStore(inst, sz, false, kAccType_ORDERED));
  inst->addCheckTrapEffect (mmuPageFaultCheck(inst));
  inst->addCommitEffect    (commitStore(inst));
  inst->addCommitEffect    (clearExclusiveMonitor(inst));
  inst->addRetirementEffect(retireMem(inst));
  inst->addSquashEffect    (eraseLSQ(inst));
  inst->addPostvalidation  (validateMemory(kAddress, kOperand2, sz, inst));

  return inst;
}

// sc_d
// require_extension('A');
// require_rv64;
// bool have_reservation = MMU.store_conditional<uint64_t>(RS1, RS2);
// WRITE_RD(!have_reservation);

archinst SC_D(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SCX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord);
}

// sc_w
// require_extension('A');
// bool have_reservation = MMU.store_conditional<uint32_t>(RS1, RS2);
// WRITE_RD(!have_reservation);

archinst SC_W(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SCX(aFetchedOpcode, aCPU, aSequenceNo, kWord);
}

// c_add
// require_extension(EXT_ZCA);
// require(insn.rvc_rs2() != 0);
// WRITE_RD(sext_xlen(RVC_RS1 + RVC_RS2));

archinst C_ADD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegRS1C, kRegRS2C, kRegRD, kNoExtension);
}

// c_addi
// require_extension(EXT_ZCA);
// WRITE_RD(sext_xlen(RVC_RS1 + insn.rvc_imm()));

archinst C_ADDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegRS1C, kImmC, kRegRD, kNoExtension);
}

archinst C_ADDI16SP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegSP, kImmC_ADDI16SP, kRegSP, kNoExtension);
}

// c_addi4spn
// require_extension(EXT_ZCA);
// require(insn.rvc_addi4spn_imm() != 0);
// WRITE_RVC_RS2S(sext_xlen(RVC_SP + insn.rvc_addi4spn_imm()));

archinst C_ADDI4SPN(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADD, kRegSP, kImmC_ADDI4SPN, kRegRS2C_S, kNoExtension);
}

// c_addw
// require_extension(EXT_ZCA);
// require_rv64;
// WRITE_RVC_RS1S(sext32(RVC_RS1S + RVC_RS2S));

archinst C_ADDW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADDW, kRegRS1C_S, kRegRS2C_S, kRegRS1C_SW, kSignExtend);
}

// c_and
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S & RVC_RS2S);

archinst C_AND(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kAND, kRegRS1C_S, kRegRS2C_S, kRegRS1C_S, kNoExtension);
}

// c_andi
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S & insn.rvc_imm());

archinst C_ANDI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kAND, kRegRS1C_S, kImmC, kRegRS1C_S, kNoExtension);
}

// c_beqz
// require_extension(EXT_ZCA);
// if (RVC_RS1S == 0)
//   set_pc(pc + insn.rvc_b_imm());

archinst C_BEQZ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBEQ, kRegRS1C_S, kRegZero, kImmC_B);
}

// c_bnez
// require_extension(EXT_ZCA);
// if (RVC_RS1S != 0)
//   set_pc(pc + insn.rvc_b_imm());

archinst C_BNEZ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return BX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchConditional, kBNE, kRegRS1C_S, kRegZero, kImmC_B);
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
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

// c_j
// require_extension(EXT_ZCA);
// set_pc(pc + insn.rvc_j_imm());

archinst C_J(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return JX(aFetchedOpcode, aCPU, aSequenceNo, codeBranchUnconditional, kImmC_J, kRegZero);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kADDW, kRegRS1C, kImmC, kRegRD, kSignExtend);
}

// c_jalr
// require_extension(EXT_ZCA);
// require(insn.rvc_rs1() != 0);
// reg_t tmp = npc;
// set_pc(RVC_RS1 & ~reg_t(1));
// WRITE_REG(X_RA, tmp);

archinst C_JALR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return JRX(aFetchedOpcode, aCPU, aSequenceNo, kRegRS1C, kImmZero, kRegRA);
}

// c_jr
// require_extension(EXT_ZCA);
// require(insn.rvc_rs1() != 0);
// set_pc(RVC_RS1 & ~reg_t(1));

archinst C_JR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return JRX(aFetchedOpcode, aCPU, aSequenceNo, kRegRS1C, kImmZero, kRegZero);
}

// c_ld

archinst C_LD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kNoExtension, kRegRS1C_S, kImmC_LD, kRegRS2C_S);
}

// c_li
// require_extension(EXT_ZCA);
// WRITE_RD(insn.rvc_imm());

archinst C_LI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMOV, kImmC, kRegZero, kRegRD, kNoExtension);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMOV, kImmC_12, kRegZero, kRegRD, kNoExtension);
}

// c_lw
// require_extension(EXT_ZCA);
// WRITE_RVC_RS2S(MMU.load<int32_t>(RVC_RS1S + insn.rvc_lw_imm()));

archinst C_LW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kSignExtend, kRegRS1C_S, kImmC_LW, kRegRS2C_SW);
}

// c_lwsp
// require_extension(EXT_ZCA);
// require(insn.rvc_rd() != 0);
// WRITE_RD(MMU.load<int32_t>(RVC_SP + insn.rvc_lwsp_imm()));

archinst C_LWSP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kSignExtend, kRegSP, kImmC_LWSP, kRegRD);
}

// c_ldsp
// require_extension(EXT_ZCA);
// require(insn.rvc_rd() != 0);
// WRITE_RD(MMU.load<int64_t>(RVC_SP + insn.rvc_ldsp_imm()));

archinst C_LDSP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return LX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kNoExtension, kRegSP, kImmC_LDSP, kRegRD);
}

// c_mv
// require_extension(EXT_ZCA);
// require(insn.rvc_rs2() != 0);
// WRITE_RD(RVC_RS2);

archinst C_MV(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMOV, kRegRS2C, kRegZero, kRegRD, kNoExtension);
}

// c_nop

archinst C_NOP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsComputation, codeALU);

  return inst;
}

// c_or
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S | RVC_RS2S);

archinst C_OR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kORR, kRegRS1C_S, kRegRS2C_S, kRegRS1C_S, kNoExtension);
}

// c_sd

archinst C_SD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kRegRS1C_S, kRegRS2C_S, kImmC_LD);
}

// c_slli
// require_extension(EXT_ZCA);
// require(insn.rvc_zimm() < xlen);
// WRITE_RD(sext_xlen(RVC_RS1 << insn.rvc_zimm()));

archinst C_SLLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSLL, kRegRS1C, kImmC_Z, kRegRD, kNoExtension);
}

// c_srai
// require_extension(EXT_ZCA);
// require(insn.rvc_zimm() < xlen);
// WRITE_RVC_RS1S(sext_xlen(sext_xlen(RVC_RS1S) >> insn.rvc_zimm()));

archinst C_SRAI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRA, kRegRS1C_S, kImmC_Z, kRegRS1C_S, kNoExtension);
}

// c_srli
// require_extension(EXT_ZCA);
// require(insn.rvc_zimm() < xlen);
// WRITE_RVC_RS1S(sext_xlen(zext_xlen(RVC_RS1S) >> insn.rvc_zimm()));

archinst C_SRLI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSRL, kRegRS1C_S, kImmC_Z, kRegRS1C_S, kNoExtension);
}

// c_sub
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(sext_xlen(RVC_RS1S - RVC_RS2S));

archinst C_SUB(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSUB, kRegRS1C_S, kRegRS2C_S, kRegRS1C_S, kNoExtension);
}

// c_subw
// require_extension(EXT_ZCA);
// require_rv64;
// WRITE_RVC_RS1S(sext32(RVC_RS1S - RVC_RS2S));

archinst C_SUBW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kSUBW, kRegRS1C_S, kRegRS2C_S, kRegRS1C_SW, kSignExtend);
}

// c_sw
// require_extension(EXT_ZCA);
// MMU.store<uint32_t>(RVC_RS1S + insn.rvc_lw_imm(), RVC_RS2S);

archinst C_SW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kRegRS1C_S, kRegRS2C_S, kImmC_LW);
}

// c_swsp
// require_extension(EXT_ZCA);
// MMU.store<uint32_t>(RVC_SP + insn.rvc_swsp_imm(), RVC_RS2);

archinst C_SWSP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kWord, kRegSP, kRegRS2C, kImmC_SWSP);
}

// c_sdsp
// require_extension(EXT_ZCA);
// MMU.store<uint32_t>(RVC_SP + insn.rvc_swsp_imm(), RVC_RS2);

archinst C_SDSP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return SX(aFetchedOpcode, aCPU, aSequenceNo, kDoubleWord, kRegSP, kRegRS2C, kImmC_SDSP);
}

// c_xor
// require_extension(EXT_ZCA);
// WRITE_RVC_RS1S(RVC_RS1S ^ RVC_RS2S);

archinst C_XOR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kXOR, kRegRS1C_S, kRegRS2C_S, kRegRS1C_S, kNoExtension);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kDIV, kRegRS1, kRegRS2, kRegRD, kNoExtension);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kDIVU, kRegRS1, kRegRS2, kRegRD, kNoExtension);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kDIVUW, kRegRS1, kRegRS2, kRegRD, kSignExtend);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kDIVW, kRegRS1, kRegRS2, kRegRD, kSignExtend);
}

// mul
// require_either_extension('M', EXT_ZMMUL);
// WRITE_RD(sext_xlen(RS1 * RS2));

archinst MUL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMUL, kRegRS1, kRegRS2, kRegRD, kNoExtension);
}

// mulh
// require_either_extension('M', EXT_ZMMUL);
// if (xlen == 64)
//   WRITE_RD(mulh(RS1, RS2));
// else
//   WRITE_RD(sext32((sext32(RS1) * sext32(RS2)) >> 32));

archinst MULH(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMULH, kRegRS1, kRegRS2, kRegRD, kSignExtend);
}

// mulhsu
// require_either_extension('M', EXT_ZMMUL);
// if (xlen == 64)
//   WRITE_RD(mulhsu(RS1, RS2));
// else
//   WRITE_RD(sext32((sext32(RS1) * reg_t((uint32_t)RS2)) >> 32));

archinst MULHSU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMULHSU, kRegRS1, kRegRS2, kRegRD, kSignExtend);
}

// mulhu
// require_either_extension('M', EXT_ZMMUL);
// if (xlen == 64)
//   WRITE_RD(mulhu(RS1, RS2));
// else
//   WRITE_RD(sext32(((uint64_t)(uint32_t)RS1 * (uint64_t)(uint32_t)RS2) >> 32));

archinst MULHU(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMULHU, kRegRS1, kRegRS2, kRegRD, kSignExtend);
}

// mulw
// require_either_extension('M', EXT_ZMMUL);
// require_rv64;
// WRITE_RD(sext32(RS1 * RS2));

archinst MULW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kMULW, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kREM, kRegRS1, kRegRS2, kRegRD, kNoExtension);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kREMU, kRegRS1, kRegRS2, kRegRD, kNoExtension);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kREMUW, kRegRS1, kRegRS2, kRegRDW, kSignExtend);
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
  return ALX(aFetchedOpcode, aCPU, aSequenceNo, kREMW, kRegRS1, kRegRS2, kRegRD, kSignExtend);
}

archinst CSRX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int mode) {
  DECODER_TRACE;

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  auto csr = opcode.imm_i() & 0xffflu;

  // time/timeh
  if ((csr == 0xc01) || (csr == 0xc81)) {
    SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU, aSequenceNo));
    inst->setClass(clsComputation, codeALU);
    inst->setWillRaise(kException_IllegalInstr);
    inst->forceResync();

    return inst;
  }

  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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
  return CSRX(aFetchedOpcode, aCPU, aSequenceNo, 0);
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
  return CSRX(aFetchedOpcode, aCPU, aSequenceNo, 1);
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
  return CSRX(aFetchedOpcode, aCPU, aSequenceNo, 2);
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
  return CSRX(aFetchedOpcode, aCPU, aSequenceNo, 3);
}

// csrrw
// int csr = validate_csr(insn.csr(), true);
// reg_t old = p->get_csr(csr, insn, true);
// p->put_csr(csr, RS1);
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRW(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return CSRX(aFetchedOpcode, aCPU, aSequenceNo, 4);
}

// csrrwi
// int csr = validate_csr(insn.csr(), true);
// reg_t old = p->get_csr(csr, insn, true);
// p->put_csr(csr, insn.rs1());
// WRITE_RD(sext_xlen(old));
// serialize();

archinst CSRRWI(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return CSRX(aFetchedOpcode, aCPU, aSequenceNo, 5);
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
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  // TODO
  inst->setClass(clsComputation, codeALU);

  simple_action ecall = ecallAction(inst);

  connectDependance(inst->retirementDependance(), ecall);

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
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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

  DecodedOpcode opcode(aFetchedOpcode.theOpcode);

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsSynchronizing, codeMEMBARSync);

  std::vector<std::list<InternalDependance>> rs_deps(2);
  predicated_action tlbi = tlbiAction(inst, rs_deps);

  addReadRegister(inst, 1, opcode.rs1(), rs_deps[0]);
  addReadRegister(inst, 2, opcode.rs2(), rs_deps[1]);

  connectDependance(inst->retirementDependance(), tlbi);

  inst->addCommitEffect(tlbiEffect(inst, kOperand1, kOperand2));

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
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
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

  SemanticInstruction *inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                    aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  // TODO
  inst->setClass(clsComputation, codeALU);

  simple_action wfi = wfiAction(inst);

  inst->haltDispatch();

  connectDependance(inst->retirementDependance(), wfi);

  return inst;
}

archinst decode_riscv(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop) {
  auto opcode = aFetchedOpcode.theOpcode;

  if (opcode == 0xffffffff) {
    auto inst = blackBox(aFetchedOpcode, aCPU, aSequenceNo);

    inst->setWillRaise(kException_InstrPageFault);
    return inst;
  }

  DECODER_DBG("#" << aSequenceNo << ": opcode = " << std::hex << aFetchedOpcode.theOpcode
                  << std::dec);

  switch (extract32(opcode, 0, 2)) {
  case 0:
    switch (extract32(opcode, 13, 3)) {
    case 0:
      return C_ADDI4SPN(aFetchedOpcode, aCPU, aSequenceNo);
    case 2:
      return C_LW(aFetchedOpcode, aCPU, aSequenceNo);
    case 3:
      return C_LD(aFetchedOpcode, aCPU, aSequenceNo);
    case 6:
      return C_SW(aFetchedOpcode, aCPU, aSequenceNo);
    case 7:
      return C_SD(aFetchedOpcode, aCPU, aSequenceNo);
    default:
      goto err;
    }

  case 1:
    switch (extract32(opcode, 13, 3)) {
    case 0:
      switch (extract32(opcode, 7, 5)) {
      case 0:
        return C_NOP(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        return C_ADDI(aFetchedOpcode, aCPU, aSequenceNo);
      }
    case 1:
      return C_JAL(aFetchedOpcode, aCPU, aSequenceNo);
    case 2:
      switch (extract32(opcode, 7, 5)) {
      case 0:
        goto err;
      default:
        return C_LI(aFetchedOpcode, aCPU, aSequenceNo);
      }
    case 3:
      switch (extract32(opcode, 7, 5)) {
      case 0:
        goto err;
      case 2:
        return C_ADDI16SP(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        return C_LUI(aFetchedOpcode, aCPU, aSequenceNo);
      }
    case 4:
      switch (extract32(opcode, 10, 2)) {
      case 0:
        return C_SRLI(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        return C_SRAI(aFetchedOpcode, aCPU, aSequenceNo);
      case 2:
        return C_ANDI(aFetchedOpcode, aCPU, aSequenceNo);
      case 3:
        switch (extract32(opcode, 12, 1)) {
        case 0:
          switch (extract32(opcode, 5, 2)) {
          case 0:
            return C_SUB(aFetchedOpcode, aCPU, aSequenceNo);
          case 1:
            return C_XOR(aFetchedOpcode, aCPU, aSequenceNo);
          case 2:
            return C_OR(aFetchedOpcode, aCPU, aSequenceNo);
          case 3:
            return C_AND(aFetchedOpcode, aCPU, aSequenceNo);
          default:
            goto err;
          }
        default:
          switch (extract32(opcode, 5, 2)) {
          case 0:
            return C_SUBW(aFetchedOpcode, aCPU, aSequenceNo);
          case 1:
            return C_ADDW(aFetchedOpcode, aCPU, aSequenceNo);
          default:
            goto err;
          }
        }
      }
    case 5:
      return C_J(aFetchedOpcode, aCPU, aSequenceNo);
    case 6:
      return C_BEQZ(aFetchedOpcode, aCPU, aSequenceNo);
    case 7:
      return C_BNEZ(aFetchedOpcode, aCPU, aSequenceNo);
    default:
      goto err;
    }

  case 2:
    switch (extract32(opcode, 13, 3)) {
    case 0:
      switch (extract32(opcode, 7, 5)) {
      case 0:
        goto err;
      default:
        return C_SLLI(aFetchedOpcode, aCPU, aSequenceNo);
      }
    case 2:
      switch (extract32(opcode, 7, 5)) {
      case 0:
        goto err;
      default:
        return C_LWSP(aFetchedOpcode, aCPU, aSequenceNo);
      }
    case 3:
      switch (extract32(opcode, 7, 5)) {
      case 0:
        goto err;
      default:
        return C_LDSP(aFetchedOpcode, aCPU, aSequenceNo);
      }
    case 4:
      switch (extract32(opcode, 12, 1)) {
      case 0:
        switch (extract32(opcode, 2, 5)) {
        case 0:
          switch (extract32(opcode, 7, 5)) {
          case 0:
            goto err;
          default:
            return C_JR(aFetchedOpcode, aCPU, aSequenceNo);
          }
        default:
          switch (extract32(opcode, 7, 5)) {
          case 0:
            goto err;
          default:
            return C_MV(aFetchedOpcode, aCPU, aSequenceNo);
          }
        }
      case 1:
        switch (extract32(opcode, 2, 5)) {
        case 0:
          switch (extract32(opcode, 7, 5)) {
          case 0:
            return C_EBREAK(aFetchedOpcode, aCPU, aSequenceNo);
          default:
            return C_JALR(aFetchedOpcode, aCPU, aSequenceNo);
          }
        default:
          switch (extract32(opcode, 7, 5)) {
          case 0:
            goto err;
          default:
            return C_ADD(aFetchedOpcode, aCPU, aSequenceNo);
          }
        }
      default:
        goto err;
      }
    case 6:
      return C_SWSP(aFetchedOpcode, aCPU, aSequenceNo);
    case 7:
      return C_SDSP(aFetchedOpcode, aCPU, aSequenceNo);
    default:
      goto err;
    }

  case 3:
    switch (extract32(opcode, 2, 5)) {
    case 0x0d:
      return LUI(aFetchedOpcode, aCPU, aSequenceNo);

    case 0x05:
      return AUIPC(aFetchedOpcode, aCPU, aSequenceNo);

    case 0x1b:
      return JAL(aFetchedOpcode, aCPU, aSequenceNo);

    case 0x19:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return JALR(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x18:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return BEQ(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        return BNE(aFetchedOpcode, aCPU, aSequenceNo);
      case 4:
        return BLT(aFetchedOpcode, aCPU, aSequenceNo);
      case 5:
        return BGE(aFetchedOpcode, aCPU, aSequenceNo);
      case 6:
        return BLTU(aFetchedOpcode, aCPU, aSequenceNo);
      case 7:
        return BGEU(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x00:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return LB(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        return LH(aFetchedOpcode, aCPU, aSequenceNo);
      case 2:
        return LW(aFetchedOpcode, aCPU, aSequenceNo);
      case 3:
        return LD(aFetchedOpcode, aCPU, aSequenceNo);
      case 4:
        return LBU(aFetchedOpcode, aCPU, aSequenceNo);
      case 5:
        return LHU(aFetchedOpcode, aCPU, aSequenceNo);
      case 6:
        return LWU(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x08:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return SB(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        return SH(aFetchedOpcode, aCPU, aSequenceNo);
      case 2:
        return SW(aFetchedOpcode, aCPU, aSequenceNo);
      case 3:
        return SD(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x04:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return ADDI(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        switch (extract32(opcode, 26, 6)) {
        case 0x00:
          return SLLI(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 2:
        return SLTI(aFetchedOpcode, aCPU, aSequenceNo);
      case 3:
        return SLTIU(aFetchedOpcode, aCPU, aSequenceNo);
      case 4:
        return XORI(aFetchedOpcode, aCPU, aSequenceNo);
      case 5:
        switch (extract32(opcode, 26, 6)) {
        case 0x00:
          return SRLI(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x10:
          return SRAI(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 6:
        return ORI(aFetchedOpcode, aCPU, aSequenceNo);
      case 7:
        return ANDI(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x0c:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return ADD(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return MUL(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x20:
          return SUB(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 1:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SLL(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return MULH(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 2:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SLT(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return MULHSU(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 3:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SLTU(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return MULHU(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 4:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return XOR(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return DIV(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 5:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SRL(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return DIVU(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x20:
          return SRA(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 6:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return OR(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return REM(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 7:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return AND(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return REMU(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      default:
        goto err;
      }

    case 0x03:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return FENCE(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        return FENCE_I(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x1c:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        switch (extract32(opcode, 20, 12)) {
        case 0x000:
          return ECALL(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x001:
          return EBREAK(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x102:
          return SRET(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x302:
          return MRET(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x105:
          return WFI(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          switch (extract32(opcode, 25, 7)) {
          case 0x09:
            return SFENCE_VMA(aFetchedOpcode, aCPU, aSequenceNo);
          default:
            goto err;
          }
          goto err;
        }
      case 1:
        return CSRRW(aFetchedOpcode, aCPU, aSequenceNo);
      case 2:
        return CSRRS(aFetchedOpcode, aCPU, aSequenceNo);
      case 3:
        return CSRRC(aFetchedOpcode, aCPU, aSequenceNo);
      case 5:
        return CSRRWI(aFetchedOpcode, aCPU, aSequenceNo);
      case 6:
        return CSRRSI(aFetchedOpcode, aCPU, aSequenceNo);
      case 7:
        return CSRRCI(aFetchedOpcode, aCPU, aSequenceNo);
      default:
        goto err;
      }

    case 0x06:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        return ADDIW(aFetchedOpcode, aCPU, aSequenceNo);
      case 1:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SLLIW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 5:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SRLIW(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x20:
          return SRAIW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      default:
        goto err;
      }

    case 0x0e:
      switch (extract32(opcode, 12, 3)) {
      case 0:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return ADDW(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return MULW(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x20:
          return SUBW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 1:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SLLW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 4:
        switch (extract32(opcode, 25, 7)) {
        case 0x01:
          return DIVW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 5:
        switch (extract32(opcode, 25, 7)) {
        case 0x00:
          return SRLW(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return DIVUW(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x20:
          return SRAW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 6:
        switch (extract32(opcode, 25, 7)) {
        case 0x01:
          return REMW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 7:
        switch (extract32(opcode, 25, 7)) {
        case 0x01:
          return REMUW(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      default:
        goto err;
      }

    case 0x0b:
      switch (extract32(opcode, 12, 3)) {
      case 2:
        switch (extract32(opcode, 27, 5)) {
        case 0x02:
          return LR_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x03:
          return SC_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return AMOSWAP_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x00:
          return AMOADD_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x04:
          return AMOXOR_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x0c:
          return AMOAND_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x08:
          return AMOOR_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x10:
          return AMOMIN_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x14:
          return AMOMAX_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x18:
          return AMOMINU_W(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1c:
          return AMOMAXU_W(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      case 3:
        switch (extract32(opcode, 27, 5)) {
        case 0x02:
          return LR_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x03:
          return SC_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x01:
          return AMOSWAP_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x00:
          return AMOADD_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x04:
          return AMOXOR_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x0c:
          return AMOAND_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x08:
          return AMOOR_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x10:
          return AMOMIN_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x14:
          return AMOMAX_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x18:
          return AMOMINU_D(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1c:
          return AMOMAXU_D(aFetchedOpcode, aCPU, aSequenceNo);
        default:
          goto err;
        }
      default:
        goto err;
      }

    default:
      goto err;
    }
  default:
    goto err;
  }

err:
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

} // namespace nDecoder
