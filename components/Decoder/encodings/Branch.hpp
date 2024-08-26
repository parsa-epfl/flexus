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

#ifndef FLEXUS_armDECODER_armBRANCH_HPP_INCLUDED
#define FLEXUS_armDECODER_armBRANCH_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

// Unconditional branch (immediate)
archinst
UNCONDBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Unconditional branch (register)
archinst
BR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
BLR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
RET(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
ERET(archcode const& aFetchedOpcode,
     uint32_t aCPU,
     int64_t aSequenceNo); // TODO
archinst
DPRS(archcode const& aFetchedOpcode,
     uint32_t aCPU,
     int64_t aSequenceNo); // TODO

// Compare and branch (immediate)
archinst
CMPBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Test and branch (immediate)
archinst
TSTBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Conditional branch (immediate)
archinst
CONDBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// System TODO
archinst
HINT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SYNC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
MSR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SYS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Exception generation TODO
archinst
SVC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
HVC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SMC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
BRK(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
HLT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
DCPS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder
#endif // FLEXUS_armDECODER_armBRANCH_HPP_INCLUDED