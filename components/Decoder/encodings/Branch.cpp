
#include "Branch.hpp"

#include "../Effects.hpp"
#include "Unallocated.hpp"
#include "components/Decoder/Conditions.hpp"
#include "components/Decoder/OperandCode.hpp"
#include "components/Decoder/SemanticActions.hpp"
#include "components/uArch/systemRegister.hpp"

namespace nDecoder {

using namespace nuArch;

static void
branch_always(SemanticInstruction* inst, bool immediate, VirtualMemoryAddress target)
{
    DECODER_TRACE;

    inst->setClass(clsBranch, codeBranchUnconditional);
    inst->addDispatchEffect(branch(inst, target));

    inst->bpState()->theActualType = kUnconditional;
    inst->bpState()->theActualDirection = kTaken;
    inst->bpState()->theActualTarget = target;
}

static void
branch_cond(SemanticInstruction* inst,
            VirtualMemoryAddress target,
            eCondCode aCode,
            std::list<InternalDependance>& rs_deps)
{
    DECODER_TRACE;

    inst->setClass(clsBranch, codeBranchConditional);

    dependant_action br = branchCondAction(inst, target, condition(aCode), 1);
    connectDependance(inst->retirementDependance(), br);

    rs_deps.push_back(br.dependance);
}

/*
 * Branch with Link branches to a PC-relative offset, setting the register X30
 * to PC+4. It provides a hint that this is a subroutine call.
 *
 * Operation:
 *  X[30] = PC[] + 4;
 *  BranchTo(PC[] + offset, BranchType_CALL);
 */
archinst
UNCONDBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    bool op        = extract32(aFetchedOpcode.theOpcode, 31, 1);
    int64_t offset = sextract32(aFetchedOpcode.theOpcode, 0, 26) << 2;
    uint64_t addr  = (uint64_t)aFetchedOpcode.thePC + offset;
    VirtualMemoryAddress target(addr);

    DBG_(VVerb,
         (<< "branching to " << std::hex << target << " with an offset of 0x" << std::hex << offset << std::dec));

    inst->addPostvalidation(validatePC(inst));

    if (op) {
        inst->setClass(clsBranch, codeCALL);
        std::vector<std::list<InternalDependance>> rs_deps(1);
        predicated_action exec = addExecute(inst, operation(kMOV_), rs_deps);

        addReadConstant(inst, 1, (uint64_t)(aFetchedOpcode.thePC) + 4, rs_deps[0]);
        addDestination(inst, 30, exec, true);

        inst->bpState()->theActualType = kCall;
        inst->bpState()->theActualDirection = kTaken;
        inst->bpState()->theActualTarget = target;

        // update call after
        inst->addDispatchEffect(branch(inst, target));
    } else {

        inst->bpState()->theActualType = kUnconditional;

        branch_always(inst, 0, target);
    }
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
archinst
CMPBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    bool sf        = extract32(aFetchedOpcode.theOpcode, 31, 1);
    uint32_t rt    = extract32(aFetchedOpcode.theOpcode, 0, 5);
    bool iszero    = !(extract32(aFetchedOpcode.theOpcode, 24, 1));
    int64_t offset = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
    uint64_t addr  = (uint64_t)aFetchedOpcode.thePC + offset;
    VirtualMemoryAddress target(addr);

    DBG_(VVerb, (<< "cmp/br to " << std::hex << target << " with an offset of 0x" << std::hex << offset << std::dec));

    std::vector<std::list<InternalDependance>> rs_deps(1);
    branch_cond(inst, target, iszero ? kCBZ_ : kCBNZ_, rs_deps[0]);
    addReadXRegister(inst, 1, rt, rs_deps[0], sf);
    inst->addPostvalidation(validatePC(inst));
    inst->bpState()->theActualType = kConditional;

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
archinst
TSTBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    uint32_t rt      = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t bit_pos = ((extract32(aFetchedOpcode.theOpcode, 31, 1) << 5) | extract32(aFetchedOpcode.theOpcode, 19, 5));
    bool sf          = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool bit_val     = extract32(aFetchedOpcode.theOpcode, 24, 1);
    int64_t offset   = sextract32(aFetchedOpcode.theOpcode, 5, 14) << 2;
    VirtualMemoryAddress target((uint64_t)aFetchedOpcode.thePC + offset);

    DBG_(VVerb, (<< "offest is set to #" << offset));
    DBG_(VVerb, (<< "Branch address is set to " << target));

    DBG_(VVerb,
         (<< "Testing bit value (" << bit_val << ") and bit pos (" << bit_pos << ")  -- "
          << "Branching to address " << std::hex << (uint64_t)aFetchedOpcode.thePC << " with an offset of 0x" << offset
          << std::dec << " -->> " << target));

    std::vector<std::list<InternalDependance>> rs_deps(1);
    branch_cond(inst, target, bit_val ? kTBNZ_ : kTBZ_, rs_deps[0]);

    readRegister(inst, 1, rt, rs_deps[0], sf);
    inst->setOperand(kCondition, uint64_t(1ULL << bit_pos));
    inst->addPostvalidation(validatePC(inst));
    inst->bpState()->theActualType = kConditional;

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
archinst
CONDBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    uint64_t cond = extract32(aFetchedOpcode.theOpcode, 0, 4);

    //    program label to be conditionally branched to. Its offset from the
    //    address of this instruction,
    // in the range +/-1MB, is encoded as "imm19" times 4.
    int64_t offset = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
    uint64_t addr  = (uint64_t)aFetchedOpcode.thePC + offset;

    VirtualMemoryAddress target(addr);

    if (cond < 0x0e) {
        DBG_(VVerb,
             (<< "conditionally branching to " << std::hex << target << " with an offset of 0x" << std::hex << offset
              << std::dec));

        /* genuinely conditional branches */
        std::vector<std::list<InternalDependance>> rs_deps(1);
        branch_cond(inst, target, kBCOND_, rs_deps[0]);
        inst->setOperand(kCondition, cond);
        addReadCC(inst, 1, rs_deps[0], true);

        inst->bpState()->theActualType = kConditional;
    } else {
        DBG_(VVerb,
             (<< "unconditionally branching to " << std::hex << target << " with an offset of 0x" << std::hex << offset
              << std::dec));
        inst->addPostvalidation(validatePC(inst));

        /* 0xe and 0xf are both "always" conditions */
        branch_always(inst, false, target);

        inst->bpState()->theActualType = kUnconditional;
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
archinst
BR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsBranch, codeBranchUnconditional);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    std::vector<std::list<InternalDependance>> rs_deps(1);

    addReadXRegister(inst, 1, rn, rs_deps[0], true);

    simple_action target = calcAddressAction(inst, rs_deps);
    dependant_action br  = branchToCalcAddressAction(inst);
    connectDependance(br.dependance, target);
    connectDependance(inst->retirementDependance(), br);
    inst->bpState()->theActualType = kIndirectReg;

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
archinst
BLR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t rm = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    bool pac    = extract32(aFetchedOpcode.theOpcode, 11, 1);
    uint32_t op = extract32(aFetchedOpcode.theOpcode, 21, 2);
    eBranchType branch_type;

    if (!pac && (rm != 0))
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    else if (pac) // armv8.3
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    switch (op) {
        case 0: branch_type = kIndirectReg; break;
        case 1: branch_type = kIndirectCall; break;
        case 2: branch_type = kReturn; break;
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); break;
    }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));
    std::vector<std::list<InternalDependance>> rs_deps(1);

    simple_action target = calcAddressAction(inst, rs_deps);
    dependant_action br  = branchRegAction(inst, kAddress, branch_type);
    connectDependance(br.dependance, target);
    connectDependance(inst->retirementDependance(), br);

    switch (branch_type) {
        case kIndirectCall: {
            inst->setClass(clsBranch, codeBranchIndirectCall);
            inst->bpState()->theActualType = kIndirectCall;
            break;
        }
        case kIndirectReg: {
            inst->setClass(clsBranch, codeBranchIndirectReg);
            inst->bpState()->theActualType = kIndirectReg;
            break;
        }
        case kReturn: {
            inst->setClass(clsBranch, codeRETURN);
            inst->bpState()->theActualType = kReturn;
            break;
        }
        default: DBG_Assert(false, (<< "Not setting a class is weird, what happend ?"));
    }

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

archinst
ERET(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}
archinst
DPRS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

// System
archinst
HINT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t op1      = extract32(aFetchedOpcode.theOpcode, 16, 3);
    uint32_t crm      = extract32(aFetchedOpcode.theOpcode, 8, 4);
    uint32_t op2      = extract32(aFetchedOpcode.theOpcode, 5, 3);
    uint32_t selector = crm << 3 | op2;

    if (op1 != 3) { return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); }

    switch (selector) {
        case 0: /* NOP */
        case 3: /* WFI */
        case 1: /* YIELD */
        case 2: /* WFE */
        case 4: /* SEV */
        case 5: /* SEVL */
        default: return nop(aFetchedOpcode, aCPU, aSequenceNo);
    }
}
archinst
SYNC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    // uint32_t rt = extract32(aFetchedOpcode.theOpcode, 16, 3);
    uint32_t crm = extract32(aFetchedOpcode.theOpcode, 8, 4);
    uint32_t op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);

    if (op2 == 0x0 || op2 == 0x1 || op2 == 0x7 || op2 == 0x3) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

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
                case 1: /* MBReqTypes_Reads */ bar = kBAR_SC | kMO_LD_LD | kMO_LD_ST; break;
                case 2: /* MBReqTypes_Writes */ bar = kBAR_SC | kMO_ST_ST; break;
                default: /* MBReqTypes_All */ bar = kBAR_SC | kMO_ALL; break;
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
        default: delete inst; return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    return inst;
}

archinst
MSR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t op1, crm, op2;
    op1 = extract32(aFetchedOpcode.theOpcode, 16, 3);
    crm = extract32(aFetchedOpcode.theOpcode, 8, 4); // imm
    op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsComputation, codeWRPR);

    /* Msutherl: Added check for instructions with special encodings (e.g., DAIFSet and DAIFClr)
     * - for relevant encodings, see section C5.2.2 "DAIF, Interrupt Mask Bits", page C5-357 of the
     *   ARMv8 ISA manual */
    bool isShorthandDAIFEncoding = false;
    if (op1 == 0x3 && (op2 == 0x7 || op2 == 0x6)) { isShorthandDAIFEncoding = true; }

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
    inst->setOperand(kResult, uint64_t(crm << 6));

    // Confusing name writePSTATE, but it it implemented correctly for this very specific DAIFset and DAIFclr case
    inst->addRetirementEffect( writePSTATE(inst, op1, op2) );
    // inst->addPostvalidation( validateXRegister( rt, kResult, inst  ) );
    // FIXME - validate PR

    return inst;
}

archinst
SYS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t op0, op1, crn, crm, op2, rt;
    bool l;
    l   = extract32(aFetchedOpcode.theOpcode, 21, 1);
    op0 = extract32(aFetchedOpcode.theOpcode, 19, 2);
    op1 = extract32(aFetchedOpcode.theOpcode, 16, 3);
    crn = extract32(aFetchedOpcode.theOpcode, 12, 4);
    crm = extract32(aFetchedOpcode.theOpcode, 8, 4);
    op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);
    rt  = extract32(aFetchedOpcode.theOpcode, 0, 5);

    ePrivRegs pr = getPrivRegType(op0, op1, op2, crn, crm);

    if (pr == kLastPrivReg) {
        pr = kAbstractSysReg;
        return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    } else if (pr == kDC_ZVA) {
        return nop(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

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

        std::vector<std::list<InternalDependance>> rs_dep(1);
        // need to halt dispatch for writes
        inst->setHaltDispatch();
        inst->addCheckTrapEffect(checkSystemAccess(inst, op0, op1, op2, crn, crm, rt, l));
        predicated_action exec = addExecute(inst, (operation(kMOV_)), {kOperand1}, rs_dep, kResult);
        addReadXRegister(inst, 1, rt, rs_dep[0], true);
        connectDependance(inst->retirementDependance(), exec);
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
archinst
SVC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    // return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_SVC);
}
archinst
HVC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_HVC);
}
archinst
SMC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_SMC);
}
archinst
BRK(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_BKPT);
}
archinst
HLT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_UNCATEGORIZED);
} // TODO: not supported - do we care about semihosting exception? if yes, then raise
  // it here
archinst
DCPS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_UNCATEGORIZED);
} // TODO not supported

} // namespace nDecoder
