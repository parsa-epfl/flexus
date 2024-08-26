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
//
#pragma once

/* Author: @msutherl
 * - Implementation of the Arm FPSRs (FPSR and FPCR).
 *   These are read/reset by QEMU on resync, and then can be mapped to a phys
 *   register whenever an FP instruction uses them.
 * - References:
 *   ARMv8 FPCR/SR status register description - Section C5.2.7/8 of ISA Manual
 *   (on /home/parsacom/docs/qflex/arm_isa_reference_manual).
 */

#include "components/Decoder/BitManip.hpp"

/* Bit flags/definitions for all fields in FPSR
 */
#define FPSR_N   (1 << 31)
#define FPSR_Z   (1 << 30)
#define FPSR_C   (1 << 29)
#define FPSR_V   (1 << 28)
#define FPSR_QC  (1 << 27)
#define FPSR_IDC (1 << 7)
#define FPSR_IXC (1 << 4)
#define FPSR_UFC (1 << 3)
#define FPSR_OFC (1 << 2)
#define FPSR_DZC (1 << 1)
#define FPSR_IOC (1)

/* Bit flags/definitions for all fields in FPCR
 */
#define FPCR_AHP    (1 << 26)
#define FPCR_DN     (1 << 25)
#define FPCR_FZ     (1 << 24)
#define FPCR_RMODE  ((1ULL << 23) | (1ULL << 22))
#define FPCR_STRIDE ((1ULL << 21) | (1ULL << 20))
#define FPCR_FZ16   (1 << 19)
#define FPCR_LEN    ((1ULL << 18) | (1ULL << 17) | (1ULL << 16))
#define FPCR_IDE    (1 << 15)
#define FPCR_IXE    (1 << 12)
#define FPCR_UFE    (1 << 11)
#define FPCR_OFE    (1 << 10)
#define FPCR_DZE    (1 << 9)
#define FPCR_IOE    (1 << 8)

// using namespace nuArch;

class CImpl_FPSR
{
  public:
    CImpl_FPSR() { theVal = 0; }

    CImpl_FPSR(uint64_t src) { theVal = src; }

    const uint64_t get(void) const { return theVal; }

    void set(uint64_t aNewVal) { theVal = aNewVal; }

  private:
    uint64_t theVal;
};

class CImpl_FPCR
{
  public:
    CImpl_FPCR() { theVal = 0; }

    CImpl_FPCR(uint64_t src) { theVal = src; }

    const uint64_t get(void) const { return theVal; }

    void set(uint64_t aNewVal) { theVal = aNewVal; }

  private:
    uint64_t theVal;
};