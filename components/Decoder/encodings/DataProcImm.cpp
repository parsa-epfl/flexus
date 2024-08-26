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

#include "DataProcImm.hpp"

#include "SharedFunctions.hpp"
#include "Unallocated.hpp"

namespace nDecoder {
using namespace nuArch;

enum eMoveWideOp
{
    kMoveWideOp_N,
    kMoveWideOp_Z,
    kMoveWideOp_K,
};

archinst
ADR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    uint32_t rd    = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int64_t offset = sextract64(aFetchedOpcode.theOpcode, 5, 19);
    offset         = (offset << 2) | extract32(aFetchedOpcode.theOpcode, 29, 2);
    bool op        = extract32(aFetchedOpcode.theOpcode, 31, 1);
    uint64_t base  = aFetchedOpcode.thePC;

    if (op) {
        /* ADRP (page based) */
        base &= ~0xfff;
        offset <<= 12;
    }
    std::vector<std::list<InternalDependance>> rs_deps(2);

    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);

    addReadConstant(inst, 1, base, rs_deps[0]);
    addReadConstant(inst, 2, offset, rs_deps[1]);

    addDestination(inst, rd, exec, true);

    return inst;
}

archinst
EXTR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    bool sf          = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool n           = extract32(aFetchedOpcode.theOpcode, 22, 1);
    uint32_t rm      = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t imm     = extract32(aFetchedOpcode.theOpcode, 10, 6);
    uint32_t rn      = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rd      = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t op21    = extract32(aFetchedOpcode.theOpcode, 29, 2);
    bool op0         = extract32(aFetchedOpcode.theOpcode, 21, 1);
    uint32_t bitsize = sf ? 64 : 32;

    if (sf != n || op21 || op0 || imm >= bitsize) { return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo,
                                                      clsComputation,
                                                      codeALU));

    /* MARK: Refactored to deal with the case where there are 2 sources
     * (explicit use of EXTR) or only one (where it should disassemble as ROR)
     */
    if (rn == rm) {
        std::vector<std::list<InternalDependance>> rs_deps(2);
        predicated_action exec = rorAction(inst, rs_deps, kOperand1, kOperand2, sf);
        readRegister(inst, 1, rn, rs_deps[0], sf);
        addReadConstant(inst, 2, imm, rs_deps[1]);
        addDestination(inst, rd, exec, sf);
    } else {
        /* Explicit EXTR with rn != rm, and both must be read */
        std::vector<std::list<InternalDependance>> rs_deps(3);
        predicated_action exec = extractAction(inst, rs_deps, kOperand1, kOperand2, kOperand3, sf);

        readRegister(inst, 1, rn, rs_deps[0], sf);
        readRegister(inst, 2, rm, rs_deps[1], sf);
        addReadConstant(inst, 3, imm, rs_deps[2]);

        addDestination(inst, rd, exec, sf);
    }
    return inst;
}

static bool
logic_imm_decode_wmask(uint64_t* result,
                       uint64_t* result2,
                       unsigned int immn,
                       unsigned int imms,
                       unsigned int immr,
                       bool immediate)
{
    uint64_t mask, mask2;
    unsigned e, levels, s, r, d;
    int len;

    assert(immn < 2 && imms < 64 && immr < 64);

    /* The bit patterns we create here are 64 bit patterns which
     * are vectors of identical elements of size e = 2, 4, 8, 16, 32 or
     * 64 bits each. Each element contains the same value: a run
     * of between 1 and e-1 non-zero bits, rotated within the
     * element by between 0 and e-1 bits.
     *
     * The element size and run length are encoded into immn (1 bit)
     * and imms (6 bits) as follows:
     * 64 bit elements: immn = 1, imms = <length of run - 1>
     * 32 bit elements: immn = 0, imms = 0 : <length of run - 1>
     * 16 bit elements: immn = 0, imms = 10 : <length of run - 1>
     *  8 bit elements: immn = 0, imms = 110 : <length of run - 1>
     *  4 bit elements: immn = 0, imms = 1110 : <length of run - 1>
     *  2 bit elements: immn = 0, imms = 11110 : <length of run - 1>
     * Notice that immn = 0, imms = 11111x is the only combination
     * not covered by one of the above options; this is reserved.
     * Further, <length of run - 1> all-ones is a reserved pattern.
     *
     * In all cases the rotation is by immr % e (and immr is 6 bits).
     */

    /* First determine the element size */
    len = 31 - clz32((immn << 6) | (~imms & 0x3f));
    if (len < 1) {
        /* This is the immn == 0, imms == 0x11111x case */
        return false;
    }
    e = 1 << len;

    levels = e - 1;
    s      = imms & levels;
    r      = immr & levels;
    d      = s - r;
    d &= levels;

    if (immediate && s == levels) {
        /* <length of run - 1> mustn't be all-ones. */
        return false;
    }

    /* Create the value of one element: s+1 set bits rotated
     * by r within the element (which is e bits wide)...
     */
    mask  = bitmask64(s + 1);
    mask2 = bitmask64(d + 1);
    if (r) {
        mask = (mask >> r) | (mask << (e - r));
        mask &= bitmask64(e);
    }
    /* ...then replicate the element over the whole 64 bit value */
    mask     = bitfield_replicate(mask, e);
    mask2    = bitfield_replicate(mask2, e);
    *result  = mask;
    *result2 = mask2;
    return true;
}

archinst
BFM(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{

    DECODER_TRACE;

    uint32_t rd   = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn   = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t imms = extract32(aFetchedOpcode.theOpcode, 10, 6);
    uint32_t immr = extract32(aFetchedOpcode.theOpcode, 16, 6);
    bool n        = extract32(aFetchedOpcode.theOpcode, 22, 1);
    uint32_t opc  = extract32(aFetchedOpcode.theOpcode, 29, 2);
    bool sf       = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool inzero = false, extend = false;
    uint64_t wmask = 0, tmask = 0;

    uint64_t bitsize = sf ? 64 : 32;
    if (sf != n || immr >= bitsize || imms >= bitsize || opc > 2) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    switch (opc) {
        case 0: // SBFM
            inzero = true;
            extend = true;
            break;
        case 1: // BFM
            break;
        case 2: // UBFM
            inzero = true;
            break;
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); break;
    }

    if (!logic_imm_decode_wmask(&wmask, &tmask, n, imms, immr, false)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (!sf) { wmask &= 0xffffffff; }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps(2);
    predicated_action exec = bitFieldAction(inst, rs_deps, kOperand1, kOperand2, imms, immr, wmask, tmask, extend, sf);
    readRegister(inst, 1, rn, rs_deps[0], sf);
    if (inzero) {
        addReadConstant(inst, 2, 0, rs_deps[1]);
    } else {
        readRegister(inst, 2, rd, rs_deps[1], sf);
    }
    addDestination(inst, rd, exec, sf);

    return inst;
}

archinst
MOVE(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t rd  = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint64_t imm = extract32(aFetchedOpcode.theOpcode, 5, 16);
    bool sf      = extract32(aFetchedOpcode.theOpcode, 31, 1);
    uint32_t pos = extract32(aFetchedOpcode.theOpcode, 21, 2) << 4;
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 29, 2);

    if (!sf && (pos >= 32)) { return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); }

    eMoveWideOp opcode;
    switch (opc) {
        case 0: opcode = kMoveWideOp_N; break;
        case 2: opcode = kMoveWideOp_Z; break;
        case 3: opcode = kMoveWideOp_K; break;
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));
    inst->setClass(clsComputation, codeALU);
    std::vector<std::list<InternalDependance>> rs_deps(1);

    predicated_action act;
    uint64_t mask = ~((uint64_t)0xffff << pos);
    inst->setOperand(kOperand3, mask);

    if (opcode == kMoveWideOp_K) {
        DECODER_DBG("Wide Move K");
        rs_deps.resize(2);
        act = addExecute(inst, operation(kMOVK_), { kOperand1, kOperand2, kOperand3 }, rs_deps);
        inst->addDispatchEffect(satisfy(inst, act.action->dependance(2)));

        readRegister(inst, 2, rd, rs_deps[1], sf);
    } else {
        if (opcode == kMoveWideOp_N) {
            DECODER_DBG("Wide Move Neg");
            act = addExecute(inst, operation(kMOVN_), rs_deps);
        } else {
            DECODER_DBG("Wide Move");
            act = addExecute(inst, operation(kMOV_), rs_deps);
        }
    }

    addReadConstant(inst, 1, (imm << pos), rs_deps[0]);
    addDestination(inst, rd, act, sf);
    return inst;
}

archinst
LOGICALIMM(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t rd   = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn   = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t imms = extract32(aFetchedOpcode.theOpcode, 10, 6);
    uint32_t immr = extract32(aFetchedOpcode.theOpcode, 16, 6);
    bool n        = extract32(aFetchedOpcode.theOpcode, 22, 1);
    uint32_t opc  = extract32(aFetchedOpcode.theOpcode, 29, 2);
    bool sf       = extract32(aFetchedOpcode.theOpcode, 31, 1);
    std::unique_ptr<Operation> op;
    uint64_t wmask = 0, tmask = 0;
    bool setflags = false;

    switch (opc) {
        case 0: op = operation(kAND_); break;
        case 1: op = operation(kORR_); break;
        case 2: op = operation(kXOR_); break;
        case 3:
            setflags = true;
            op       = operation(kANDS_);
            break;
        default: break;
    }

    if (!logic_imm_decode_wmask(&wmask, &tmask, n, imms, immr, true)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    if (!sf) { wmask &= 0xffffffff; }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps(2);

    predicated_action exec = addExecute(inst, std::move(op), rs_deps);

    readRegister(inst, 1, rn, rs_deps[0], sf);
    addReadConstant(inst, 2, static_cast<int64_t>(wmask), rs_deps[1]);

    if (!setflags || rd != 31)
        addDestination(inst, rd, exec, sf, setflags);
    else if (setflags)
        addSetCC(inst, exec, sf);

    return inst;
}

archinst
ALUIMM(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t rd    = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn    = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t imm   = extract32(aFetchedOpcode.theOpcode, 10, 12);
    uint32_t shift = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool setflags  = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sub_op    = extract32(aFetchedOpcode.theOpcode, 30, 1);
    bool sf        = extract32(aFetchedOpcode.theOpcode, 31, 1);

    switch (shift) {
        case 0x0: break;
        case 0x1: imm <<= 12; break;
        default: return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rs_deps(2);
    predicated_action exec =
      addExecute(inst, operation(setflags ? (sub_op ? kSUBS_ : kADDS_) : (sub_op ? kSUB_ : kADD_)), rs_deps);
    addReadConstant(inst, 2, imm, rs_deps[1]);

    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
    if (!setflags || rd != 31)
        addDestination(inst, rd, exec, sf, setflags);
    else if (setflags)
        addSetCC(inst, exec, sf);

    return inst;
}

} // namespace nDecoder