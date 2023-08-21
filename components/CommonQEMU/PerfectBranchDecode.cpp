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
#include <components/CommonQEMU/PerfectBranchDecode.hpp>

using Flexus::SharedTypes::kCall;
using Flexus::SharedTypes::kConditional;
using Flexus::SharedTypes::kIndirectCall;
using Flexus::SharedTypes::kIndirectReg;
using Flexus::SharedTypes::kNonBranch;
using Flexus::SharedTypes::kReturn;
using Flexus::SharedTypes::kUnconditional;

int32_t sextract32(uint32_t value, int start, int length) {
  assert(start >= 0 && length > 0 && length <= 32 - start);
  /* Note that this implementation relies on right shift of signed
   * integers being an arithmetic shift.
   */
  return ((int32_t)(value << (32 - length - start))) >> (32 - length);
}

uint32_t extract32(uint32_t value, int start, int length) {
  assert(start >= 0 && length > 0 && length <= 32 - start);
  return (value >> start) & (~0U >> (32 - length));
}

std::pair<Flexus::SharedTypes::eBranchType, Flexus::SharedTypes::VirtualMemoryAddress>
targetDecode(uint32_t opcode) {
  VirtualMemoryAddress aTarget(0); // This is actually displacement
  eBranchType aType = kNonBranch;
  int64_t offset = 0;
  uint64_t cond = 0;
  switch (extract32(opcode, 25, 7)) {
  case 0x0a:
  case 0x0b:
  case 0x4a:
  case 0x4b: /* Unconditional branch (immediate) */
    offset = sextract32(opcode, 0, 26) << 2;
    aTarget = VirtualMemoryAddress(offset);
    aType = kUnconditional;
    break;
  case 0x1a:
  case 0x5a: /* Compare & branch (immediate) */
    offset = sextract32(opcode, 5, 19) << 2;
    aTarget = VirtualMemoryAddress(offset);
    aType = kConditional;
    break;
  case 0x1b:
  case 0x5b: /* Test & branch (immediate) */
    offset = sextract32(opcode, 5, 14) << 2;
    aTarget = VirtualMemoryAddress(offset);
    aType = kConditional;
    break;
  case 0x2a: /* Conditional branch (immediate) */
    cond = extract32(opcode, 0, 4);
    // program label to be conditionally branched to. Its offset from the address of this
    // instruction, in the range +/-1MB, is encoded as "imm19" times 4.
    offset = sextract32(opcode, 5, 19) << 2;
    aTarget = VirtualMemoryAddress(offset);

    if (cond < 0x0e) {
      /* genuinely conditional branches */
      aType = kConditional;
    } else {
      /* 0xe and 0xf are both "always" conditions */
      aType = kUnconditional;
    }
    break;
  case 0x6b: /* Unconditional branch (register) */
    switch (extract32(opcode, 21, 2)) {
    case 0:
      aType = kIndirectReg;
      break;
    case 1:
      aType = kCall;
      break;
    case 2:
      aType = kReturn;
      break;
    default:
      aType = kNonBranch;
      break;
    }
    break;
  default:
    break;
  }
  return std::make_pair(aType, aTarget);
}
