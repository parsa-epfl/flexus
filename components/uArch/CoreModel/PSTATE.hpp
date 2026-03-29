
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
