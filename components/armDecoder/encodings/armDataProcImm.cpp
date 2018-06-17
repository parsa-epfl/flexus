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

namespace narmDecoder {
using namespace nuArchARM;

void ADR(SemanticInstruction* inst, uint64_t base, uint64_t offset, uint64_t rd)
{
    std::vector<std::list<InternalDependance> > rs_deps;
    inst->setOperand(kOperand1, base);
    inst->setOperand(kOperand2, offset);
    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);
    addDestination(inst, rd, exec);
}

void EXTR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint32_t rm  , uint64_t imm, bool sf)
{
    std::vector<std::list<InternalDependance> > rs_deps(3);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf ? true : false);
    inst->setOperand(kOperand3, imm);
    predicated_action exec = addExecute(inst, operation(sf ? kCONCAT64_ : kCONCAT32_),rs_deps);
    addDestination(inst, rd, exec);
}

void SBFM(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint32_t imms, uint64_t immr, bool sf)
{

//    if (sf){
//        uint64_t *wmask, *tmask;
//        if (!logic_imm_decode_wmask_tmask(&wmask, &tmask, n, imms, immr)) {
//            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//        }
//    }

//    bits(datasize) src = X[n];
//    // perform bitfield move on low bits
//    bits(datasize) bot = ROR(src, R) AND wmask;
//    // determine extension bits (sign, zero or dest register)
//    bits(datasize) top = Replicate(src<S>);
//    // combine extension bits and result bits
//    X[d] = (top AND NOT(tmask)) OR (bot AND tmask);

//    std::vector<std::list<InternalDependance> > rs_deps(2);
//    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
//    inst->setOperand(kOperand2, immr);
//    predicated_action e1 = addExecute(inst, operation(kROR_),rs_deps);

//    Operation * op = operation(kAND_);
//    op->setOperands(kOperand1, wmask);



//    predicated_action e2 = addExecute(inst, operation(kAND_),rs_deps);
//    uint64_t top = src & (1 << imms);

//    (top & ~tmask) | (bot & tmask);


}

void MOVK(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, operation(kMOVK_),rs_deps);
    addDestination(inst, rd, exec);
}

void MOV(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool is_not)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, is_not ? operation(kMOVN_) : operation(kMOV_),rs_deps);
    addDestination(inst, rd, exec);
}

void XOR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, operation(kXOR_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void ORR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, operation(kORR_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void AND(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, S ? operation(kANDS_) : operation(kAND_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void ADD(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    if (S) { // ADDS
        inst->setOperand(kOperand3, 0); // carry in bit
    }
    predicated_action exec = addExecute(inst, S ? operation(kADDS_) : operation(kADD_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void SUB(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    if (S) { // SUBS
        inst->setOperand(kOperand3, 1); // carry in bit
    }
    predicated_action exec = addExecute(inst, S ? operation(kSUBS_) : operation(kSUB_) ,rs_deps);
    addDestination(inst, rd, exec);
}

}

