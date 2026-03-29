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