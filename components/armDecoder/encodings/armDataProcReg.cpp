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

#include "armDataProcReg.hpp"
#include "armSharedFunctions.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {
using namespace nuArchARM;

// Data-processing (1 source)
arminst RBIT(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);

    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    std::vector<std::list<InternalDependance>> rs_deps(1);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);

    predicated_action act = reverseAction(inst, kOperand1, kResult, sf);
    connect(rs_deps[0], act);

    addDestination(inst, rd, act, sf);

    return inst;
}
arminst REV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t opc = extract32(aFetchedOpcode.thePC, 10, 2);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);


    std::vector<std::list<InternalDependance>> rs_deps(1);

    uint8_t container_size;
    switch(opc) {
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
            if (!sf) return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            container_size = 64;
    }

    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));


    addReadXRegister(inst, 1, rn, rs_deps[0], sf);

    predicated_action act = reorderAction(inst, kOperand1, kResult, container_size, sf);
    connect(rs_deps[0], act);

    addDestination(inst, rd, act, sf);

    return inst;
}

arminst CL(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    bool op = extract32(aFetchedOpcode.thePC, 10, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    eCountOp opcode = op ?  kCountOp_CLZ : kCountOp_CLS;
    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
    std::vector<std::list<InternalDependance>> rs_deps(1);

    addReadXRegister(inst, 1, rn, rs_deps[0], sf);

    predicated_action act = countAction(inst, kOperand1, kResult, opcode, sf);
    connect(rs_deps[0], act);

    addDestination(inst, rd, act, sf);
    return inst;
}

// Data-processing (2 sources)
arminst DIV(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){

    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    bool is_signed = extract32(aFetchedOpcode.thePC, 10, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);

    SemanticInstruction * inst (new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);


    predicated_action exec = addExecute(inst, is_signed ? operation(kSDiv_) : operation(kUDiv_), rs_deps);
    addDestination(inst, rd, exec, sf);

    return inst;
}

arminst SHIFT(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){

    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    uint32_t shift_type  = extract32(aFetchedOpcode.thePC, 11, 2);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);

    SemanticInstruction * inst (new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps(2), sec_deps(2);
    addReadXRegister(inst, 1, rn, sec_deps[0], sf);

    addReadXRegister(inst, 2, rm, rs_deps[1], sf);
    predicated_action e1 = constantAction(inst, sf ? 64 : 32, kOperand3, kRS3);
    inst->addDispatchAction(e1);

    predicated_action e2 = addExecute(inst, operation(kUDiv_), rs_deps);
    inst->addDispatchAction(e2);

    predicated_action e3 = operandAction(inst, kResult, kOperand2, 0, boost::none);
    connect(sec_deps[1], e3);

    predicated_action e4 = addExecute(inst, shift(shift_type), sec_deps);
    addDestination(inst, rd, e4, sf);

    return inst;
}

arminst CRC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t sz = extract32(aFetchedOpcode.thePC, 10, 2);
    bool crc32c = extract32(aFetchedOpcode.thePC, 12, 1);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);

    if (sf  && sz != 3) return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    if (!sf && sz == 3) return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);

    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    std::vector<std::list<InternalDependance>> rs_deps(2);

    addReadXRegister(inst, 1, rn, rs_deps[0], false);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);
    uint32_t poly = crc32c ? 0x1EDC6F41 : 0x04C11DB7;


    predicated_action act = crcAction(inst, poly, kOperand1, kOperand2, kResult, sf);
    connect(rs_deps[0], act);
    connect(rs_deps[1], act);


    addDestination(inst, rd, act, sf);

    return inst;
}


// Conditional select
arminst CSEL(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (extract32(aFetchedOpcode.thePC, 29, 1) || extract32(aFetchedOpcode.thePC, 11, 1)) {
        /* S == 1 or op2<1> == 1 */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));


    inst->setClass(clsComputation, codeALU);

    uint32_t rn = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t cond = extract32(aFetchedOpcode.thePC, 12, 4);
    uint32_t rd = extract32(aFetchedOpcode.thePC, 16, 5);
    bool op = extract32(aFetchedOpcode.thePC, 30, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    bool o2 = extract32(aFetchedOpcode.thePC, 10, 1);
    bool inv = (op == 1);
    bool inc = (o2 == 1);

    std::vector<std::list<InternalDependance>> rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf );
    addReadXRegister(inst, 2, rm, rs_deps[1], sf );


    std::unique_ptr<Condition> ptr = condition(kBCOND_);
    predicated_action act = conditionSelectAction(inst, ptr, cond, rs_deps, kResult, inv, inc, sf);

    addDestination(inst, rd, act, sf);

    return inst;
}

// Conditional compare (immediate / register)
arminst CCMP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (!extract32(aFetchedOpcode.thePC, 29, 1)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    if (aFetchedOpcode.thePC & (1 << 10 | 1 << 4)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));


    uint32_t flags = extract32(aFetchedOpcode.thePC, 0, 4);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t cond = extract32(aFetchedOpcode.thePC, 12, 4);
    bool sub_op = extract32(aFetchedOpcode.thePC, 30, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);

    inst->setOperand(kResult1, flags);

    std::vector<std::list<InternalDependance>> rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);

    std::unique_ptr<Condition> ptr = condition(kBCOND_);
    predicated_action act = conditionCompareAction(inst, ptr, cond, rs_deps, kResult, sub_op, sf);
    connect(rs_deps[0], act);

    inst->addDispatchEffect(writePR(inst, kNZCV));

    return inst;

}

// Add/subtract (with carry)
arminst ADDSUB_CARRY(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    bool sub_op = extract32(aFetchedOpcode.thePC, 30, 1);
    bool setflags = extract32(aFetchedOpcode.thePC, 29, 1);

    SemanticInstruction * inst (new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));

    std::vector<std::list<InternalDependance>> rs_deps(3);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);

    simple_action nzcv =  readNZCVAction ( inst, kC, kOperand3);
    connect(rs_deps[2], nzcv);
    inst->addDispatchAction(nzcv);

    predicated_action exec = addExecute(inst, operation(setflags ? (sub_op ? kSUBS_ : kADDS_) : (sub_op ? kSUB_ : kADD_)), rs_deps);

    addDestination(inst, rd, exec, sf);
    return inst;
}

// Data-processing (3 source)
arminst DP_3_SRC(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    uint32_t op_id = (extract32(aFetchedOpcode.thePC, 29, 3) << 4) |
                     (extract32(aFetchedOpcode.thePC, 21, 3) << 1) |
                      extract32(aFetchedOpcode.thePC, 15, 1);
    bool is_signed = false;

    /* Note that op_id is sf:op54:op31:o0 so it includes the 32/64 size flag */
    switch (op_id) {
    case 0x42: /* SMADDL */
    case 0x43: /* SMSUBL */
    case 0x44: /* SMULH */
        is_signed = true;
        break;
    case 0x0: /*  MADD (32bit) */
    case 0x1: /*  MSUB (32bit) */
    case 0x40: /* MADD (64bit) */
    case 0x41: /* MSUB (64bit) */
    case 0x4a: /* UMADDL */
    case 0x4b: /* UMSUBL */
    case 0x4c: /* UMULH */
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    uint32_t ra = extract32(aFetchedOpcode.thePC, 10, 5);
    bool sub_op = extract32(aFetchedOpcode.thePC, 15, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    bool is_long = extract32(aFetchedOpcode.thePC, 21, 1);
    bool is_high = extract32(aFetchedOpcode.thePC, 22, 1);


    SemanticInstruction * inst = new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

    inst->setClass(clsComputation, codeALU);
    std::vector<std::list<InternalDependance>> rs_deps(2), rs2_deps(1);

    addReadXRegister(inst, 1, rn, rs_deps[0], is_long ? false : sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], is_long ? false : sf);

    predicated_action mul = addExecute(inst, operation(is_high ? (is_signed ? kSMulH_ : kUMulH_) : (is_long ? (is_signed ? kSMulL_ : kUMulL_) : (is_signed ? kSMul_ : kUMul_) )), rs_deps);
    inst->addDispatchAction(mul);

    if (!is_high){
        connect(rs2_deps[1], mul);
        addReadXRegister(inst, 3, ra, rs2_deps[0], sf);

        predicated_action act = addExecute(inst, operation(sub_op ? kSUB_ : kADD_), rs2_deps);
        inst->addDispatchAction(act);
        addDestination(inst, rd, act, sf);

    } else {
        addDestination(inst, rd, mul, sf);
    }

    return inst;
}

// Add/subtract (shifted register)
arminst ADDSUB_SHIFTED(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    bool setflags = extract32(aFetchedOpcode.thePC, 29, 1);
    bool sub_op = extract32(aFetchedOpcode.thePC, 30, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    uint32_t shift_amount = extract32(aFetchedOpcode.thePC, 10, 6);
    uint32_t shift_type = extract32(aFetchedOpcode.thePC, 22, 2);


    if (shift_amount == 3) return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); // FIXME reservedvvalue
    if (!sf && (shift_amount & (1 << 5))) return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); // ReservedValue();

    SemanticInstruction * inst = new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

    inst->setClass(clsComputation, codeALU);
    std::vector<std::list<InternalDependance>> rs_deps(3);

    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);

    std::unique_ptr<Operation> shift_op = shift(shift_type);
    predicated_action sh = shiftAction(inst, kOperand2, shift_op, shift_amount, sf);
    connect(rs_deps[1], sh);
    inst->addDispatchAction( sh );

    predicated_action res = addExecute(inst, operation( (sub_op ? (setflags ? kSUBS_ : kSUB_) : (setflags ? kADDS_ : kADD_) ) ), rs_deps);
    addDestination(inst, rd, res, sf);

    return inst;
}

// Add/subtract (extended register)
arminst ADDSUB_EXTENDED(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t option = extract32(aFetchedOpcode.thePC, 13, 3);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    bool setflags = extract32(aFetchedOpcode.thePC, 29, 1);
    bool sub_op = extract32(aFetchedOpcode.thePC, 30, 1);
    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    eExtendType extend_type = DecodeRegExtend(option);
    uint32_t shift_amount = extract32(aFetchedOpcode.thePC, 10, 3);


    SemanticInstruction * inst = new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

    inst->setClass(clsComputation, codeALU);
    std::vector<std::list<InternalDependance>> rs_deps(3);


    if (shift_amount > 4) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);

    std::unique_ptr<Operation> extend_op = extend(extend_type);
    predicated_action ex = extendAction(inst, kOperand2, extend_op, sf);
    connect(rs_deps[1], ex);
    inst->addDispatchAction( ex );

    std::unique_ptr<Operation> shift_op = shift(0/*kLSL_*/);
    predicated_action sh = shiftAction(inst, kOperand2, shift_op, shift_amount, sf);
    connect(rs_deps[1], sh);
    inst->addDispatchAction( sh );


    predicated_action res = addExecute(inst, operation( (sub_op ? (setflags ? kSUBS_ : kSUB_) : (setflags ? kADDS_ : kADD_) ) ), rs_deps);
    addDestination(inst, rd, res, sf);

    return inst;
}

// Logical (shifted register)
arminst LOGICAL(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    SemanticInstruction * inst = new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo);

    bool sf = extract32(aFetchedOpcode.thePC, 31, 1);
    uint32_t opc = extract32(aFetchedOpcode.thePC, 29, 2);
    uint32_t shift_type = extract32(aFetchedOpcode.thePC, 22, 2);
    uint32_t rm = extract32(aFetchedOpcode.thePC, 16, 5);
    uint32_t shift_amount = extract32(aFetchedOpcode.thePC, 10, 6);
    uint32_t rn = extract32(aFetchedOpcode.thePC, 5, 5);
    uint32_t rd = extract32(aFetchedOpcode.thePC, 0, 5);
    bool setflags;


    if (!sf && (shift_amount & (1 << 5))) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    std::vector<std::list<InternalDependance>> rs_deps(2);

    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[2], sf);

    std::unique_ptr<Operation> shift_op = shift(shift_type);
    predicated_action shift_act = shiftAction(inst, kOperand2, shift_op, shift_amount, sf);
    connect(rs_deps[1], shift_act);


    eOpType op;
    std::unique_ptr<Operation> ptr ;
    switch (opc) {
    case 0:
        op = kAND_;
        break;
    case 1:
        op = kORR_;
        break;
    case 2:
        op = kXOR_;
        break;
    case 3:
        op = kAND_;
        setflags = true;
        break;
    }

    predicated_action act = addExecute(inst, operation(op), rs_deps);

    if (setflags){
     inst->addRetirementEffect(writeNZCV(inst));
    }

    addDestination(inst, rd, act, sf);

    return inst;
}

} // narmDecoder
