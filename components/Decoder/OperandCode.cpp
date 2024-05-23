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

#include "OperandCode.hpp"

namespace nDecoder {

std::ostream&
operator<<(std::ostream& anOstream, eOperandCode aCode)
{
    const char* operand_codes[] = { "rs1",
                                    "rs2",
                                    "rs3" // Y for div
                                    ,
                                    "rs4",
                                    "rs5",
                                    "fs1_0",
                                    "fs1_1",
                                    "fs2_0",
                                    "fs2_1",
                                    "CCs",
                                    "rd",
                                    "rd1",
                                    "rd2",
                                    "XTRA",
                                    "fd0",
                                    "fd1",
                                    "CCd",
                                    "ps1",
                                    "ps2",
                                    "ps3",
                                    "ps4",
                                    "ps5",
                                    "pfs1_0",
                                    "pfs1_1",
                                    "pfs2_0",
                                    "pfs2_1",
                                    "CCps",
                                    "pd",
                                    "pd1",
                                    "pd2",
                                    "XTRApd",
                                    "pfd0",
                                    "pfd1",
                                    "CCpd",
                                    "ppd",
                                    "ppd1",
                                    "ppd2",
                                    "XTRAppd",
                                    "ppfd0",
                                    "ppfd1",
                                    "pCCpd",
                                    "operand1",
                                    "operand2",
                                    "operand3",
                                    "operand4",
                                    "operand5",
                                    "foperand1_0",
                                    "foperand1_1",
                                    "foperand2_0",
                                    "foperand2_1",
                                    "cc",
                                    "result",
                                    "result1",
                                    "result2",
                                    "fresult0",
                                    "fresult1",
                                    "XTRAout",
                                    "resultcc",
                                    "address",
                                    "status",
                                    "condition",
                                    "storedvalue",
                                    "fpsr",
                                    "fpcr",
                                    "uop_address_offset",
                                    "sop_address_offset" };
    if (aCode >= kLastOperandCode) {
        anOstream << "InvalidOperandCode(" << static_cast<int>(aCode) << ")";
    } else {
        anOstream << operand_codes[aCode];
    }
    return anOstream;
}

} // namespace nDecoder
