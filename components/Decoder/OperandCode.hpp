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

#ifndef FLEXUS_DECODER_OPERANDCODE_HPP_INCLUDED
#define FLEXUS_DECODER_OPERANDCODE_HPP_INCLUDED

#include <iostream>

namespace nDecoder {

#define SIGNED_UPPER_BOUND_B 0xFFFFFFFFFFFFFF00ULL
#define SIGNED_UPPER_BOUND_H 0xFFFFFFFFFFFF0000ULL
#define SIGNED_UPPER_BOUND_W 0xFFFFFFFF00000000ULL
#define SIGNED_UPPER_BOUND_X 0x0000000000000000ULL

enum eOpType {
  kADD,
  kADDW,
  kSUB,
  kSUBW,
  kAND,
  kORR,
  kXOR,
  kMOV,

  kSLL,
  kSLLW,
  kSLT,
  kSLTU,
  kSRA,
  kSRAW,
  kSRL,
  kSRLW,

  kDIV,
  kDIVU,
  kDIVUW,
  kDIVW,
  kMUL,
  kMULH,
  kMULHSU,
  kMULHU,
  kMULW,
  kREM,
  kREMU,
  kREMUW,
  kREMW,
};

enum eOperandCode {
  kRS1 = 0,
  kRS2,
  kRS3,
  kRS4,
  kRS5,
  kFS1_0,
  kFS1_1,
  kFS2_0,
  kFS2_1,
  kCCs,
  kRD,
  kRD1,
  kRD2,
  kXTRAr,
  kFD0,
  kFD1,
  kCCd,
  kPS1,
  kPS2,
  kPS3,
  kPS4,
  kPS5,
  kPFS1_0,
  kPFS1_1,
  kPFS2_0,
  kPFS2_1,
  kCCps,
  kPD,
  kPD1,
  kPD2,
  kXTRApd,
  kPFD0,
  kPFD1,
  kCCpd,
  kPPD,
  kPPD1,
  kPPD2,
  kXTRAppd,
  kPPFD0,
  kPPFD1,
  kPCCpd,
  kOperand1,
  kOperand2,
  kOperand3,
  kOperand4,
  kOperand5,
  kFOperand1_0,
  kFOperand1_1,
  kFOperand2_0,
  kFOperand2_1,
  kCC,
  kResult,
  kResult1,
  kResult2,
  kfResult0,
  kfResult1,
  kXTRAout,
  kResultCC,
  kAddress,
  kStatus,
  kCondition,
  kStoredValue,
  kocFPSR,
  kocFPCR,
  kUopAddressOffset,
  kSopAddressOffset,
  kLastOperandCode
};

std::ostream &operator<<(std::ostream &anOstream, eOperandCode aCode);

} // namespace nDecoder

#endif // FLEXUS_DECODER_OPERANDCODE_HPP_INCLUDED
