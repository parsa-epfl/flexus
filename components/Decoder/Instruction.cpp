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

#include <components/uArch/uArchInterfaces.hpp>

#include "Instruction.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

ArchInstruction::ArchInstruction(VirtualMemoryAddress aPC, Opcode anOpcode,
                                 boost::intrusive_ptr<BPredState> bp_state, uint32_t aCPU,
                                 int64_t aSequenceNo, eInstructionClass aClass,
                                 eInstructionCode aCode)
    : thePC(aPC), thePCReg(aPC + 4), theOpcode(anOpcode), theBPState(bp_state), theCPU(aCPU),
      theSequenceNo(aSequenceNo), theuArch(0), theRaisedException(kException_None),
      theResync(false), theWillRaise(kException_None), theAnnulled(false), theRetired(false),
      theSquashed(false), theExecuted(true), thePageFault(false), theInstructionClass(aClass),
      theInstructionCode(aCode), theHaltDispatch(false), theHasCheckpoint(false),
      theRetireStallCycles(0), theMayCommit(true), theResolved(false), theUsesIntAlu(true),
      theUsesIntMult(false), theUsesIntDiv(false), theUsesFpAdd(false), theUsesFpCmp(false),
      theUsesFpCvt(false), theUsesFpMult(false), theUsesFpDiv(false), theUsesFpSqrt(false),
      theInsnSourceLevel(eL1I), thePriv(false) {
}

void ArchInstruction::describe(std::ostream &anOstream) const {
  Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
  anOstream << "#" << std::dec << theSequenceNo << "[" << std::setfill('0') << std::right
            << std::setw(2) << cpu.id()
            << "] "
            << printInstClass() << " QEMU disas: " << cpu.disassemble(thePC);
  if (theRaisedException) {
    anOstream << " {raised}";
  }
  if (theResync) {
    anOstream << " {force-resync}";
  }
  if (haltDispatch()) {
    anOstream << " {halt-dispatch}";
  }
}

std::string ArchInstruction::disassemble() const {
  return Flexus::Qemu::Processor::getProcessor(theCPU).disassemble(thePC);
}

void ArchInstruction::setWillRaise(eExceptionType aSetting) {
  theWillRaise = aSetting;
}

void ArchInstruction::doDispatchEffects() {
  if (bpState() && !isBranch()) {
    // Branch predictor identified an instruction that is not a branch as a
    // branch.
    DBG_(VVerb, (<< *this << " predicted as a branch, but is a non-branch.  Fixing"));

    boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
    feedback->thePC = pc();
    feedback->theActualType = kNonBranch;
    feedback->theActualDirection = kNotTaken;
    feedback->theActualTarget = VirtualMemoryAddress(0);
    feedback->theBPState = bpState();
    core()->branchFeedback(feedback);
  }
}

bool ArchInstruction::usesIntAlu() const {
  return theUsesIntAlu;
}

bool ArchInstruction::usesIntMult() const {
  return theUsesIntMult;
}

bool ArchInstruction::usesIntDiv() const {
  return theUsesIntDiv;
}

bool ArchInstruction::usesFpAdd() const {
  return theUsesFpAdd;
}

bool ArchInstruction::usesFpCmp() const {
  return theUsesFpCmp;
}

bool ArchInstruction::usesFpCvt() const {
  return theUsesFpCvt;
}

bool ArchInstruction::usesFpMult() const {
  return theUsesFpMult;
}

bool ArchInstruction::usesFpDiv() const {
  return theUsesFpDiv;
}

bool ArchInstruction::usesFpSqrt() const {
  return theUsesFpSqrt;
}

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
       int32_t aUop) {
  DBG_(VVerb, (<< "\033[1;31m DECODER: Decoding " << std::hex << aFetchedOpcode.theOpcode
               << std::dec << "\033[0m"));

  bool last_uop = true;
  boost::intrusive_ptr<AbstractInstruction> ret_val; // HEHE
  return std::make_pair(ret_val, last_uop);
}

} // namespace nDecoder
