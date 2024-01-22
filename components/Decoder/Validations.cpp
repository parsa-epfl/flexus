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

#include "Validations.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

bool validateXRegister::operator()() {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; // Don't check
  }
  if (theInstruction->raised() != kException_None) {
    DBG_(VVerb, (<< " Not performing register validation for " << theReg
                 << " because of exception. " << *theInstruction));
    return true;
  }

  uint64_t flexus = theInstruction->operand<uint64_t>(theOperandCode);
  uint64_t qemu =
      (Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readXRegister(theReg)) &
      (the_64 ? -1LL : 0xFFFFFFFF);

  DBG_(Dev, Condition(flexus != qemu)(<< "flexus value in " << std::setw(10) << theOperandCode
                                      << "  = " << std::hex << flexus << std::dec));
  DBG_(Dev, Condition(flexus != qemu)(<< "qemu value in   " << std::setw(10) << theReg << "  = "
                                      << std::hex << qemu << std::dec));

  return (flexus == qemu);
}

bool validatePC::operator()() {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; // Don't check
  }
  if (theInstruction->raised() != kException_None) {
    DBG_(VVerb, (<< " Not performing  validation because of exception. " << *theInstruction));
    return true;
  }

  uint64_t flexus = thePreValidation ? theInstruction->pc() : theInstruction->pcNext();
  uint64_t qemu = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readPC();

  DBG_(Dev, Condition(flexus != qemu)(<< "flexus PC value " << std::hex << flexus << std::dec));
  DBG_(Dev, Condition(flexus != qemu)(<< "qemu PC value   " << std::hex << qemu << std::dec));

  return flexus == qemu;
}

bool validateMemory::operator()() {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; // Don't check
  }

  bits flexus;
  if (theValueCode == kResult) {
    flexus = theInstruction->operand<bits>(theValueCode);
  } else {
    flexus = theInstruction->operand<uint64_t>(theValueCode);
  }

  Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
  VirtualMemoryAddress vaddr(theInstruction->operand<uint64_t>(theAddressCode));
  int theSize_orig = theSize, theSize_extra = 0;
  if (theSize < 8)
    flexus &= 0xFFFFFFFF >> (32 - theSize * 8);
  VirtualMemoryAddress vaddr_final = vaddr + theSize_orig - 1;
  if ((vaddr & 0x1000) != (vaddr_final & 0x1000)) {
    theSize_extra = (vaddr_final & 0xFFF) + 1;
    DBG_Assert(theSize_extra < 16);
    theSize_orig -= theSize_extra;
    vaddr_final = (VirtualMemoryAddress)(vaddr_final & ~0xFFFULL);
  }
  PhysicalMemoryAddress paddr = c->translateVirtualAddress(vaddr);
  bits qemu = c->readPhysicalAddress(paddr, theSize_orig);
  if (theSize_extra) {
    DBG_Assert((qemu >> (theSize_orig * 8)) == 0);
    PhysicalMemoryAddress paddr_spill = c->translateVirtualAddress(vaddr_final);
    qemu |= c->readPhysicalAddress(paddr_spill, theSize_extra) << (theSize_orig * 8);
  }

  if (flexus == qemu)
    return true;
  DBG_(Dev, Condition(flexus != qemu)(<< "flexus value: " << std::hex << flexus));
  DBG_(Dev, Condition(flexus != qemu)(<< "qemu value:   " << std::hex << qemu));

  return (flexus == qemu);
}

} // namespace nDecoder
