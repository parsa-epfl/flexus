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

#ifndef FLEXUS_uARCHARM_PSTATE_HPP_INCLUDED
#define FLEXUS_uARCHARM_PSTATE_HPP_INCLUDED

#include "components/Decoder/BitManip.hpp"

/* Bit definitions for ARMv8 SPSR (PSTATE) format.
 * Only these are valid when in AArch64 mode; in
 * AArch32 mode SPSRs are basically CPSR-format.
 */
#define PSTATE_SP          (1U)
#define PSTATE_M           (0xFU)
#define PSTATE_nRW         (1U << 4)
#define PSTATE_F           (1U << 6)
#define PSTATE_I           (1U << 7)
#define PSTATE_A           (1U << 8)
#define PSTATE_D           (1U << 9)
#define PSTATE_EL          ((1U << 2) | (1U << 3))
#define PSTATE_IL          (1U << 20)
#define PSTATE_SS          (1U << 21)
#define PSTATE_V           (1U << 28)
#define PSTATE_C           (1U << 29)
#define PSTATE_Z           (1U << 30)
#define PSTATE_N           (1U << 31)
#define PSTATE_NZCV        (PSTATE_N | PSTATE_Z | PSTATE_C | PSTATE_V)
#define PSTATE_DAIF        (PSTATE_D | PSTATE_A | PSTATE_I | PSTATE_F)
#define CACHED_PSTATE_BITS (PSTATE_NZCV | PSTATE_DAIF)
/* Mode values for AArch64 */
#define PSTATE_MODE_EL3h 13
#define PSTATE_MODE_EL3t 12
#define PSTATE_MODE_EL2h 9
#define PSTATE_MODE_EL2t 8
#define PSTATE_MODE_EL1h 5
#define PSTATE_MODE_EL1t 4
#define PSTATE_MODE_EL0t 0

typedef struct PSTATE
{

    PSTATE(uint64_t src) { theVal = src; }
    bool N() const { return (theVal & PSTATE_N) != 0; }
    bool Z() const { return (theVal & PSTATE_Z) != 0; }
    bool C() const { return (theVal & PSTATE_C) != 0; }
    bool V() const { return (theVal & PSTATE_V) != 0; }

    const uint32_t d() const { return theVal; }

    const uint32_t EL() const { return extract32(theVal, 2, 2); }

    const uint32_t SP() const { return theVal & PSTATE_SP; }

    const uint32_t M() const { return extract32(theVal, 0, 5); }

    const uint32_t DAIF() const { return theVal & PSTATE_DAIF; }

    const void setDAIF(const uint32_t aVal)
    {
        uint32_t mask = theVal & ~PSTATE_DAIF;
        theVal        = mask | (aVal & PSTATE_DAIF);
    }

    const uint32_t NZCV() const { return theVal & PSTATE_NZCV; }

  private:
    uint32_t theVal;

} PSTATE;

#endif // FLEXUS_uARCHARM_PSTATE_HPP_INCLUDED
