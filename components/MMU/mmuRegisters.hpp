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
#ifndef FLEXUS_ARM_MMU_REGISTERS_H_INCLUDED
#define FLEXUS_ARM_MMU_REGISTERS_H_INCLUDED

#include <components/uArchARM/uArchInterfaces.hpp>

typedef unsigned long long mmu_reg_t;

typedef struct mmu_bit_configs {
  /* Msutherl - Aug'18
   * - used to extract WHERE certain bits are stored (such as EE and M in SCTLR)
   * in its emulated registers.
   */
  // enable and endian-ness
  // - in SCTLR_ELx
  uint8_t M_Bit;   // 0b1 = MMU enabled
  uint8_t EE_Bit;  // endianness of EL1, 0b0 is little-endian
  uint8_t EoE_Bit; // endianness of EL0, 0b0 is little-endian

  // Granule that this MMU supports
  // - in ID_AA64MMFR0_EL1
  uint8_t TGran4_Base;  // = 0b0000 IF supported, = 0b1111 if not
  uint8_t TGran16_Base; // = 0b0000 if NOT supported, 0b0001 IF supported (yes,
                        // this is correct)
  uint8_t TGran64_Base; // = 0b0000 IF supported, = 0b1111 if not
  uint8_t TGran_NumBits;

  // Granule configured for EL1 (TODO: add others if we ever care about them in
  // the future)
  // - in TCR_ELx
  uint8_t TG0_Base; /* 00 = 4KB
                            01 = 64KB
                            11 = 16KB */
  uint8_t TG0_NumBits;
  uint8_t TG1_Base; /* 01 = 16KB
                            10 = 4KB
                            11 = 64KB (yes, this is different than TG0) */
  uint8_t TG1_NumBits;

  // Physical, output, and input address sizes. ( in ID_AA64MMFR0_EL1 )
  uint8_t PARange_Base; /* 0b0000 = 32b
                               0b0001 = 36b
                               0b0010 = 40b
                               0b0011 = 42b
                               0b0100 = 44b
                               0b0101 = 48b
                               0b0110 = 52b */
  uint8_t PARange_NumBits;

  // translation range sizes, chooses which VA's map to TTBR0 and TTBR1
  // - in TCR_ELx
  uint8_t TG0_SZ_Base;
  uint8_t TG1_SZ_Base;
  uint8_t TGn_SZ_NumBits;
} bit_configs_t;

#define NUM_AARCH64_ELS 4

struct mmu_regs_t {

  /* Msutherl - june'18
   * Defined all registers, will only code MMU for stage 1 tablewalk (no monitor
   * or hypervisor)
   */
  mmu_reg_t SCTLR[NUM_AARCH64_ELS];
  mmu_reg_t TCR[NUM_AARCH64_ELS];

  mmu_reg_t TTBR0[NUM_AARCH64_ELS];
  mmu_reg_t TTBR1[2 + 1]; // only EL1/EL2 (indexes 1 and 2)

  mmu_reg_t ID_AA64MMFR0_EL1; // only implemented in EL1 as far as I know.

  mmu_regs_t(mmu_regs_t &copyMe) {
    for (unsigned i = 0; i < NUM_AARCH64_ELS; i++) {
      SCTLR[i] = copyMe.SCTLR[i];
      TCR[i] = copyMe.TCR[i];
      TTBR0[i] = copyMe.TTBR0[i];
    }
    TTBR1[EL1] = copyMe.TTBR1[EL1];
    TTBR1[EL2] = copyMe.TTBR1[EL2];
    ID_AA64MMFR0_EL1 = copyMe.ID_AA64MMFR0_EL1; // only implemented in EL1 as far as I know.
  }

  mmu_regs_t() {
  }

  /* type      -      ARM_NAME            - funct.
   * ----DO NOT REMOVE---- PEOPLE WILL NEED TO KNOW THIS *
   mmu_reg_t           SCTLR_EL1;          // enables/disables Secure EL1/EL0
  translation mmu_reg_t           SCTLR_EL2;          // enables/disables
  Non-Secure EL2 Stage 1, Non-Secure EL1/EL0 Stage 2 mmu_reg_t SCTLR_EL3; //
  enables/disables Secure EL3 Stage 1
  // all SCTLRs also include cacheability bits for PTEs

  mmu_reg_t           TCR_EL1;            // controls Secure/NonSecure EL1/EL0
  mmu_reg_t           TCR_EL2;            // controls Secure EL2Stg1 and
  Non-Secure EL1/EL0 Stage 2 mmu_reg_t           TCR_EL3;            // controls
  Secure EL3 Stage 1

  mmu_reg_t           TTBR0_EL1;          // controls lower address range for
  EL1 (default 0x0 - 0xffff ffff ffff), configurable through TCR_ELx.T0SZ
  mmu_reg_t           TTBR1_EL1;          // upper address range for EL1, size
  also configurable through TCR_ELx.T1SZ mmu_reg_t           TTBR0_EL3; //
  controls single address range for EL3 mmu_reg_t           TTBR0_EL2; //
  controls lower address range for EL2 (default 0x0 - 0xffff ffff ffff), config.
  as above mmu_reg_t           TTBR1_EL2;          // upper address range for
  EL2
   * ----DO NOT REMOVE---- PEOPLE WILL NEED TO KNOW THIS */
};

#endif // FLEXUS_ARM_MMU_REGISTERS_HPP_INCLUDED
