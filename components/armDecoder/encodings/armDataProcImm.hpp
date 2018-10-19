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

/*
 * The instruction disassembly implemented here matches
 * the instruction encoding classifications documented in
 * the 00bet3.1 release of the A64 ISA XML for ARMv8.2.
 * classification names and decode diagrams here should generally
 * match up with those in the manual.
 */

#ifndef FLEXUS_armDECODER_armDATAPROCIMM_HPP_INCLUDED
#define FLEXUS_armDECODER_armDATAPROCIMM_HPP_INCLUDED

#include "armSharedFunctions.hpp"

namespace narmDecoder {


//TODO
void ADR(SemanticInstruction* inst, uint64_t base, uint64_t offset, uint64_t rd);
void EXTR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint32_t rm  , uint64_t imm, bool sf);
arminst BFM(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
void MOVK(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf);
void MOV(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool is_not);
void XOR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S);
void ORR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S);
void AND(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S);
void ADD(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S);
void SUB(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S);

} // narmDecoder
#endif // FLEXUS_armDECODER_armDATAPROCIMM_HPP_INCLUDED
