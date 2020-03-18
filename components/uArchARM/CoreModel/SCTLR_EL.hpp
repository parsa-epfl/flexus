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

#ifndef FLEXUS_uARCHARM_SCTLR_EL_HPP_INCLUDED
#define FLEXUS_uARCHARM_SCTLR_EL_HPP_INCLUDED

/* SCTLR bit meanings. Several bits have been reused in newer
 * versions of the architecture; in that case we define constants
 * for both old and new bit meanings. Code which tests against those
 * bits should probably check or otherwise arrange that the CPU
 * is the architectural version it expects.
 */
#define SCTLR_M (1U << 0)
#define SCTLR_A (1U << 1)
#define SCTLR_C (1U << 2)
#define SCTLR_W (1U << 3) /* up to v6; RAO in v7 */
#define SCTLR_SA (1U << 3)
#define SCTLR_P (1U << 4)       /* up to v5; RAO in v6 and v7 */
#define SCTLR_SA0 (1U << 4)     /* v8 onward, AArch64 only */
#define SCTLR_D (1U << 5)       /* up to v5; RAO in v6 */
#define SCTLR_CP15BEN (1U << 5) /* v7 onward */
#define SCTLR_L (1U << 6)       /* up to v5; RAO in v6 and v7; RAZ in v8 */
#define SCTLR_B (1U << 7)       /* up to v6; RAZ in v7 */
#define SCTLR_ITD (1U << 7)     /* v8 onward */
#define SCTLR_S (1U << 8)       /* up to v6; RAZ in v7 */
#define SCTLR_SED (1U << 8)     /* v8 onward */
#define SCTLR_R (1U << 9)       /* up to v6; RAZ in v7 */
#define SCTLR_UMA (1U << 9)     /* v8 onward, AArch64 only */
#define SCTLR_F (1U << 10)      /* up to v6 */
#define SCTLR_SW (1U << 10)     /* v7 onward */
#define SCTLR_Z (1U << 11)
#define SCTLR_I (1U << 12)
#define SCTLR_V (1U << 13)
#define SCTLR_RR (1U << 14)   /* up to v7 */
#define SCTLR_DZE (1U << 14)  /* v8 onward, AArch64 only */
#define SCTLR_L4 (1U << 15)   /* up to v6; RAZ in v7 */
#define SCTLR_UCT (1U << 15)  /* v8 onward, AArch64 only */
#define SCTLR_DT (1U << 16)   /* up to ??, RAO in v6 and v7 */
#define SCTLR_nTWI (1U << 16) /* v8 onward */
#define SCTLR_HA (1U << 17)
#define SCTLR_BR (1U << 17)   /* PMSA only */
#define SCTLR_IT (1U << 18)   /* up to ??, RAO in v6 and v7 */
#define SCTLR_nTWE (1U << 18) /* v8 onward */
#define SCTLR_WXN (1U << 19)
#define SCTLR_ST (1U << 20)   /* up to ??, RAZ in v6 */
#define SCTLR_UWXN (1U << 20) /* v7 onward */
#define SCTLR_FI (1U << 21)
#define SCTLR_U (1U << 22)
#define SCTLR_XP (1U << 23)  /* up to v6; v7 onward RAO */
#define SCTLR_VE (1U << 24)  /* up to v7 */
#define SCTLR_E0E (1U << 24) /* v8 onward, AArch64 only */
#define SCTLR_EE (1U << 25)
#define SCTLR_L2 (1U << 26)  /* up to v6, RAZ in v7 */
#define SCTLR_UCI (1U << 26) /* v8 onward, AArch64 only */
#define SCTLR_NMFI (1U << 27)
#define SCTLR_TRE (1U << 28)
#define SCTLR_AFE (1U << 29)
#define SCTLR_TE (1U << 30)

using namespace nuArchARM;

typedef struct SCTLR_EL {

  SCTLR_EL(uint64_t src) {
    theVal = src;
  }
  uint32_t UCL() const {
    return theVal & SCTLR_UCI;
  }
  uint32_t DZE() const {
    return theVal & SCTLR_DZE;
  }

  uint32_t UMA() const {
    return theVal & SCTLR_UMA;
  }

private:
  uint64_t theVal;

} SCTLR_EL;

#endif // FLEXUS_uARCHARM_SCTLR_EL_HPP_INCLUDED
