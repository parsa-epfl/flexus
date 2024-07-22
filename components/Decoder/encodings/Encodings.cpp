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

#include "Encodings.hpp"
#include "Unallocated.hpp"

namespace nDecoder {

/* C3.1 A64 instruction index by encoding */
archinst disas_a64_insn(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo,
                       int32_t aUop) {
  if (aFetchedOpcode.theOpcode == 1) { // instruction fetch page fault
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
  }
  DECODER_DBG("#" << aSequenceNo << ": opcode = " << std::hex << aFetchedOpcode.theOpcode
                  << std::dec);

  switch (extract32(aFetchedOpcode.theOpcode, 25, 4)) {
  case 0x0:
  case 0x1:
  case 0x2:
  case 0x3: /* UNALLOCATED */
    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x8:
  case 0x9: /* Data processing - immediate */
    return disas_data_proc_imm(aFetchedOpcode, aCPU, aSequenceNo);
  case 0xa:
  case 0xb: /* Branch, exception generation and system insns */
    return disas_b_exc_sys(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x4:
  case 0x6:
  case 0xc:
  case 0xe: /* Loads and stores */
    return disas_ldst(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x5:
  case 0xd: /* Data processing - register */
    return disas_data_proc_reg(aFetchedOpcode, aCPU, aSequenceNo);
  case 0x7:
  case 0xf: /* Data processing - SIMD and floating point */
    return disas_data_proc_simd_fp(aFetchedOpcode, aCPU, aSequenceNo);
  default:
    DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should
                                                                    be handled above */
    break;
  }
  DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should
                                                                  be handled above */
  return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

} // namespace nDecoder