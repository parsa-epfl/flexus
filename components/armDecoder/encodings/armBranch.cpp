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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
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

#include "armBranch.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {

using namespace nuArchARM;


static void branch_always( SemanticInstruction * inst, bool immediate, VirtualMemoryAddress target) {
    DECODER_TRACE;

    inst->setClass(clsBranch, codeBranchUnconditional);
    inst->addDispatchEffect( branch( inst, target ) );
    inst->addRetirementEffect( updateUnconditional( inst, target ) );
}

static void branch_cond( SemanticInstruction * inst, VirtualMemoryAddress target, eCondCode aCode, std::list<InternalDependance> & rs_deps) {
    DECODER_TRACE;

  inst->setClass(clsBranch, codeBranchConditional);

  dependant_action br = branchCondAction( inst, target, condition(aCode), 1) ;
  connectDependance( inst->retirementDependance(), br );

    rs_deps.push_back( br.dependance );

//  inst->addDispatchAction( br );
  inst->addRetirementEffect( updateConditional(inst) );
}

/*
* Branch with Link branches to a PC-relative offset, setting the register X30 to PC+4.
* It provides a hint that this is a subroutine call.
*
* Operation:
*  X[30] = PC[] + 4;
*  BranchTo(PC[] + offset, BranchType_CALL);
*/
arminst UNCONDBR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    bool op = extract32(aFetchedOpcode.thePC, 31, 1);
    inst->setClass(clsBranch, codeBranchUnconditional);

    VirtualMemoryAddress target(aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 0, 26) * 4 - 4);


    if (op){
        std::vector< std::list<InternalDependance> > rs_deps(1);
        addReadConstant(inst, 1, aFetchedOpcode.thePC+4, rs_deps[0]);
        predicated_action exec = addExecute(inst, operation(kMOV_),rs_deps);
        addDestination(inst, 30, exec, true);
    }

    inst->addDispatchEffect( branch( inst, target ) );
    return inst;
}

/*
* Compare and Branch on Zero/ Non-Zero compares the value in a register with zero, and conditionally branches to a label
* at a PC-relative offset if the comparison is equal. It provides a hint that this is not a subroutine call
* or return. This instruction does not affect condition flags.
*
* Operation:
* bits(datasize) operand1 = X[t];
* if IsZero(operand1) == TRUE then
* BranchTo(PC[] + offset, BranchType_JMP);
*/
arminst CMPBR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    bool iszero = !(extract32(aFetchedOpcode.theOpcode, 24, 1));
    VirtualMemoryAddress target = VirtualMemoryAddress(aFetchedOpcode.thePC + (sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4) - 4);

    std::vector<std::list<InternalDependance>>  rs_deps(1);
    branch_cond(inst, target, iszero ? kCBZ_ : kCBNZ_, rs_deps[0]);
    addReadXRegister(inst, 1, rt, rs_deps[0], sf);

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
arminst TSTBR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t bit_pos = (extract32(aFetchedOpcode.theOpcode, 31, 1) << 5) | extract32(aFetchedOpcode.theOpcode, 19, 5);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool bit_val = ! (extract32(aFetchedOpcode.theOpcode, 24, 1));
    VirtualMemoryAddress target(aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 14) * 4 - 4);

    std::vector<std::list<InternalDependance> > rs_deps(1);
    branch_cond(inst, target, bit_val ? kTBZ_ : kTBNZ_, rs_deps[0]);

    addReadXRegister(inst, 1, rt, rs_deps[0], sf);
    inst->setOperand(kCondition, uint64_t(1ULL << bit_pos));



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
arminst CONDBR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint32_t cond = extract32(aFetchedOpcode.theOpcode, 0, 4);
    VirtualMemoryAddress target(aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4 -4);


    if (cond < 0x0e) {
        /* genuinely conditional branches */
        std::vector<std::list<InternalDependance>> rs_deps(1);
        branch_cond(inst, target, kBCOND_, rs_deps[0]);
        addReadConstant(inst, 1, cond, rs_deps[0]);

    } else {
        /* 0xe and 0xf are both "always" conditions */
        branch_always(inst, false, target);
    }

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
arminst BR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    std::vector<std::list<InternalDependance> > rs_deps(1);

    addReadXRegister(inst, 1, rn, rs_deps[0], true);

    simple_action target = calcAddressAction( inst, rs_deps);
    dependant_action br = branchToCalcAddressAction( inst );
    connectDependance( br.dependance, target );
    connectDependance( inst->retirementDependance(), br );
    inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

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
arminst BLR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
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
        branch_type = kUnconditional;
        break;
    case 1:
        branch_type = kCall;
        break;
    case 2:
        branch_type = kReturn;
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    inst->setClass(clsBranch, codeBranchUnconditional);
    std::vector<std::list<InternalDependance> > rs_deps(1);

    simple_action target = calcAddressAction( inst, rs_deps);
    dependant_action br = branchToCalcAddressAction( inst );
    connectDependance( br.dependance, target );
    connectDependance( inst->retirementDependance(), br );
    inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

    // Link
    if (branch_type == kCall){
        std::unique_ptr<Operation> addop = operation(kMOV_);
        addop->setOperands((uint64_t)aFetchedOpcode.thePC + 4);
        predicated_action exec = addExecute(inst, std::move(addop), rs_deps);
        addDestination(inst, 30, exec, true);
    }

    addReadXRegister(inst, 1, rn, rs_deps[0], true);


    return inst;
}

arminst ERET(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo);}
arminst DPRS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo);}

// System
arminst HINT(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t op1 = extract32(aFetchedOpcode.thePC, 16, 3);
    uint32_t crm = extract32(aFetchedOpcode.thePC, 8, 4);
    uint32_t op2 = extract32(aFetchedOpcode.thePC, 5, 3);
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
arminst SYNC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;

    uint32_t rt = extract32(aFetchedOpcode.thePC, 16, 3);
    uint32_t crm = extract32(aFetchedOpcode.thePC, 8, 4);
    uint32_t op2 = extract32(aFetchedOpcode.thePC, 5, 3);


    if (op2 == 0x0 || op2 == 0x1 || op2 == 0x7 || op2 == 0x3 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (rt != 0x1F){
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsSynchronizing, codeCLREX);

    uint32_t bar = 0;

    switch (op2) {
    case 2: /* CLREX */
        inst->setClass(clsSynchronizing, codeCLREX);
        inst->addRetirementEffect(clearExclusiveMonitor(inst));
        break;;
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


arminst MSR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;

    uint32_t op1, crm, op2;
    op1 = extract32(aFetchedOpcode.thePC, 16, 3);
    crm = extract32(aFetchedOpcode.thePC, 8, 4); //imm
    op2 = extract32(aFetchedOpcode.thePC, 5, 3);


    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    inst->setClass(clsComputation, codeWRPR);

    // need to halt dispatch for writes
    inst->setHaltDispatch();
    inst->addCheckTrapEffect( checkSystemAccess(inst, 0, op1, op2, 0x4, crm, 0x1f, 0) );
    inst->addCheckTrapEffect( checkDAIFAccess(inst,  op1) );
    inst->setOperand(kResult, uint64_t(crm));

    inst->addRetirementEffect( writePSTATE(inst, op1, op2) );
//        inst->addPostvalidation( validateXRegister( rt, kResult, inst  ) );
//        FIXME - validate PR

    return inst;
}

arminst SYS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;

    uint32_t op0, op1, crn, crm, op2, rt;
    bool l;
    l = extract32(aFetchedOpcode.thePC, 21, 1);
    op0 = extract32(aFetchedOpcode.thePC, 19, 2);
    op1 = extract32(aFetchedOpcode.thePC, 16, 3);
    crn = extract32(aFetchedOpcode.thePC, 12, 4);
    crm = extract32(aFetchedOpcode.thePC, 8, 4);
    op2 = extract32(aFetchedOpcode.thePC, 5, 3);
    rt = extract32(aFetchedOpcode.thePC, 0, 5);

    //Check for supported PR's
    ePrivRegs pr = getPrivRegType(op0,op1,op2,crn,crm);
    if (pr == kLastPrivReg ) {
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo); //resynchronize on all other PRs
    }



    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );




    if (l) {
      inst->setClass(clsComputation, codeRDPR);

      inst->addCheckTrapEffect( checkSystemAccess(inst, op0, op1, op2, crn, crm,rt, l) );
      setRD( inst, rt);
      inst->addDispatchEffect( mapDestination( inst ) );
      inst->addRetirementEffect( readPR(inst, pr) );
      inst->addPostvalidation( validateXRegister( rt, kResult, inst  ) );
    } else {
        inst->setClass(clsComputation, codeWRPR);

        // need to halt dispatch for writes
        inst->setHaltDispatch();
        inst->addCheckTrapEffect( checkSystemAccess(inst, op0, op1, op2, crn, crm,rt, l) );
        setRS(inst, kResult, rt);
        inst->addRetirementEffect( writePR(inst, pr) );
//        inst->addPostvalidation( validateXRegister( rt, kResult, inst  ) );
//        FIXME - validate PR
    }

    return inst;
}

// Exception generation
arminst SVC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){ DECODER_TRACE; return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_SVC);}
arminst HVC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_HVC);}
arminst SMC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_SMC);}
arminst BRK(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_AA64_BKPT);}
arminst HLT(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_UNCATEGORIZED); } // not supported - do we care about semihosting exception? if yes, then raise it here
arminst DCPS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return generateException(aFetchedOpcode, aCPU, aSequenceNo, kException_UNCATEGORIZED);} // not supported

} // narmDecoder
