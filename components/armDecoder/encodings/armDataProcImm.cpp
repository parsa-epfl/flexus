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

#include "armDataProcImm.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {
using namespace nuArchARM;



void ADR(SemanticInstruction* inst, uint64_t base, uint64_t offset, uint64_t rd)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);

    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);

    addReadConstant(inst, 1, base, rs_deps[0]);
    addReadConstant(inst, 2, offset, rs_deps[1]);

    addDestination(inst, rd, exec, true);
}

void EXTR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint32_t rm  , uint64_t imm, bool sf)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(3);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf);

    addReadConstant(inst, 3, imm, rs_deps[2]);

    predicated_action exec = extractAction(inst, rs_deps, kOperand1, kOperand2, kOperand3, sf);

    addDestination(inst, rd, exec, sf);
}

arminst BFM(armcode const & aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{

    DECODER_TRACE;

    uint32_t rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t imms = extract32(aFetchedOpcode.theOpcode, 10, 6);
    uint32_t immr = extract32(aFetchedOpcode.theOpcode, 16, 6);
    bool n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool inzero, extend;

    if ((!sf && n) || (sf && !n) || (opc == 2)){//FIXME ReservedValue();
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    if (!sf && (n || ((immr & (1 << 5)) != 0) || ((imms & (1 << 5)) != 0))) {//FIXME ReservedValue();
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }


    switch (opc) {
    case 0: // SBFM
        inzero = true;
        extend = true;
        break;
    case 1: // BFM
        break;
    case 2: // SBFM
        inzero = true;
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint64_t wmask, tmask;
    decodeBitMasks(tmask, wmask, n, imms, immr, false, sf ? 64 : 32);

    std::vector<std::list<InternalDependance>> rs_deps(1);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    if (inzero){
        addReadConstant(inst, 2, 0, rs_deps[1]);
    } else {
        addReadXRegister(inst, 2, rd, rs_deps[1], sf);
    }


    predicated_action exec = bitFieldAction(inst, rs_deps, kOperand1, kOperand2, imms, immr, wmask, tmask, extend, sf);
    addDestination(inst, rd, exec, sf);

    return inst;
}

void MOVK(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    predicated_action exec = addExecute(inst, operation(kMOVK_),rs_deps);
    addDestination(inst, rd, exec, sf);
}

void MOV(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool is_not)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    predicated_action exec = addExecute(inst,operation(is_not ? kMOVN_: kMOV_),rs_deps);
    addDestination(inst, rd, exec, sf);
}

void XOR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    predicated_action exec = addExecute(inst, operation(kXOR_) ,rs_deps);
    addDestination(inst, rd, exec, sf);
}

void ORR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    predicated_action exec = addExecute(inst,  operation(kORR_) ,rs_deps);
    addDestination(inst, rd, exec, sf);
}

void AND(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    predicated_action exec = addExecute(inst, operation(S ? kANDS_ : kAND_), rs_deps);

    if (rd == 31 && !S){
        addDestination(inst, rd, exec, sf);
    } else {
        inst->addReinstatementEffect(writePR(inst, kSPSel));
    }
}

void ADD(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadConstant(inst, 2, imm, rs_deps[1]);
    if (S) { // ADDS
        inst->setOperand(kOperand3, 0); // carry in bit
    }

    predicated_action exec = addExecute(inst, operation(S ? kADDS_ : kADD_) ,rs_deps);
    addDestination(inst, rd, exec, sf);
}

void SUB(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    DECODER_TRACE;
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    predicated_action exec = addExecute(inst, operation(S ? kSUBS_ : kSUB_) ,rs_deps);
    addDestination(inst, rd, exec, sf);
}

}

