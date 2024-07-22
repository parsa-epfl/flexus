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

#ifndef FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED
#define FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

// Load/store exclusive
// archinst CASP(archcode const & aFetchedOpcode, uint32_t  aCPU, int64_t
// aSequenceNo);
archinst CAS(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst STXR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst STRL(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDAQ(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDXR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Load register (literal)
archinst LDR_lit(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Load/store pair (all forms)
archinst LDP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst STP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

/* Load/store register (all forms) */
archinst LDR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst STR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

/* atomic memory operations */ // TODO
archinst LDADD(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDCLR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDEOR(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDSET(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDSMAX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDSMIN(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDUMAX(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst LDUMIN(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst SWP(archcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED