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

#include <iomanip>
#include <iostream>
#include <list>

#include <boost/none.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/debug/debug.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include "Constraints.hpp"
#include "Effects.hpp"
#include "Interactions.hpp"
#include "SemanticActions.hpp"
#include "SemanticInstruction.hpp"
#include "Validations.hpp"
#include "armBitManip.hpp"
#include "armInstruction.hpp"

#include <bitset> // std::bitset

#include "encodings/armEncodings.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

namespace Stat = Flexus::Stat;

using nuArchARM::ccBits;
using nuArchARM::vRegisters;
using nuArchARM::xRegisters;

using namespace nuArchARM;

/* MARK: Added constructor with explicit instruction class/code to simplify/improve accounting */
armInstruction::armInstruction(VirtualMemoryAddress aPC, Opcode anOpcode,
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

void armInstruction::describe(std::ostream &anOstream) const {
  Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
  anOstream << "#" << std::dec << theSequenceNo << "[" << std::setfill('0') << std::right
            << std::setw(2) << cpu->id()
            << "] "
            // << " PC: @" << thePC  << " OPC: | " << std::hex << theOpcode <<
            // std::dec << " | "
            << printInstClass() << " QEMU disas: " << cpu->disassemble(thePC);
  if (theRaisedException) {
    anOstream << " {raised}" /* << cpu->describeException(theRaisedException) <<
                                "(" << theRaisedException << ")"*/
        ;
  }
  if (theResync) {
    anOstream << " {force-resync}";
  }
  if (haltDispatch()) {
    anOstream << " {halt-dispatch}";
  }
}

std::string armInstruction::disassemble() const {
  return Flexus::Qemu::Processor::getProcessor(theCPU)->disassemble(thePC);
}

void armInstruction::setWillRaise(eExceptionType aSetting) {
  theWillRaise = aSetting;
}

void armInstruction::doDispatchEffects() {
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
    //    core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >(
    //    this ) , new BranchInteraction(VirtualMemoryAddress(0)) );
  }
}

bool armInstruction::usesIntAlu() const {
  return theUsesIntAlu;
}

bool armInstruction::usesIntMult() const {
  return theUsesIntMult;
}

bool armInstruction::usesIntDiv() const {
  return theUsesIntDiv;
}

bool armInstruction::usesFpAdd() const {
  return theUsesFpAdd;
}

bool armInstruction::usesFpCmp() const {
  return theUsesFpCmp;
}

bool armInstruction::usesFpCvt() const {
  return theUsesFpCvt;
}

bool armInstruction::usesFpMult() const {
  return theUsesFpMult;
}

bool armInstruction::usesFpDiv() const {
  return theUsesFpDiv;
}

bool armInstruction::usesFpSqrt() const {
  return theUsesFpSqrt;
}

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
       int32_t aUop) {
  DBG_(VVerb, (<< "\033[1;31m DECODER: Decoding " << std::hex << aFetchedOpcode.theOpcode
               << std::dec << "\033[0m"));

  bool last_uop = true;
  boost::intrusive_ptr<AbstractInstruction> ret_val =
      disas_a64_insn(aFetchedOpcode, aCPU, aSequenceNo, aUop);
  return std::make_pair(ret_val, last_uop);
}

} // namespace narmDecoder
