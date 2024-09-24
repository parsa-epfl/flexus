
#ifndef FLEXUS_uARCHARM_SCTLR_EL_HPP_INCLUDED
#define FLEXUS_uARCHARM_SCTLR_EL_HPP_INCLUDED

/* SCTLR bit meanings. Several bits have been reused in newer
 * versions of the architecture; in that case we define constants
 * for both old and new bit meanings. Code which tests against those
 * bits should probably check or otherwise arrange that the CPU
 * is the architectural version it expects.
 */
#define SCTLR_M       (1U << 0)
#define SCTLR_A       (1U << 1)
#define SCTLR_C       (1U << 2)
#define SCTLR_W       (1U << 3) /* up to v6; RAO in v7 */
#define SCTLR_SA      (1U << 3)
#define SCTLR_P       (1U << 4)  /* up to v5; RAO in v6 and v7 */
#define SCTLR_SA0     (1U << 4)  /* v8 onward, AArch64 only */
#define SCTLR_D       (1U << 5)  /* up to v5; RAO in v6 */
#define SCTLR_CP15BEN (1U << 5)  /* v7 onward */
#define SCTLR_L       (1U << 6)  /* up to v5; RAO in v6 and v7; RAZ in v8 */
#define SCTLR_B       (1U << 7)  /* up to v6; RAZ in v7 */
#define SCTLR_ITD     (1U << 7)  /* v8 onward */
#define SCTLR_S       (1U << 8)  /* up to v6; RAZ in v7 */
#define SCTLR_SED     (1U << 8)  /* v8 onward */
#define SCTLR_R       (1U << 9)  /* up to v6; RAZ in v7 */
#define SCTLR_UMA     (1U << 9)  /* v8 onward, AArch64 only */
#define SCTLR_F       (1U << 10) /* up to v6 */
#define SCTLR_SW      (1U << 10) /* v7 onward */
#define SCTLR_Z       (1U << 11)
#define SCTLR_I       (1U << 12)
#define SCTLR_V       (1U << 13)
#define SCTLR_RR      (1U << 14) /* up to v7 */
#define SCTLR_DZE     (1U << 14) /* v8 onward, AArch64 only */
#define SCTLR_L4      (1U << 15) /* up to v6; RAZ in v7 */
#define SCTLR_UCT     (1U << 15) /* v8 onward, AArch64 only */
#define SCTLR_DT      (1U << 16) /* up to ??, RAO in v6 and v7 */
#define SCTLR_nTWI    (1U << 16) /* v8 onward */
#define SCTLR_HA      (1U << 17)
#define SCTLR_BR      (1U << 17) /* PMSA only */
#define SCTLR_IT      (1U << 18) /* up to ??, RAO in v6 and v7 */
#define SCTLR_nTWE    (1U << 18) /* v8 onward */
#define SCTLR_WXN     (1U << 19)
#define SCTLR_ST      (1U << 20) /* up to ??, RAZ in v6 */
#define SCTLR_UWXN    (1U << 20) /* v7 onward */
#define SCTLR_FI      (1U << 21)
#define SCTLR_U       (1U << 22)
#define SCTLR_XP      (1U << 23) /* up to v6; v7 onward RAO */
#define SCTLR_VE      (1U << 24) /* up to v7 */
#define SCTLR_E0E     (1U << 24) /* v8 onward, AArch64 only */
#define SCTLR_EE      (1U << 25)
#define SCTLR_L2      (1U << 26) /* up to v6, RAZ in v7 */
#define SCTLR_UCI     (1U << 26) /* v8 onward, AArch64 only */
#define SCTLR_NMFI    (1U << 27)
#define SCTLR_TRE     (1U << 28)
#define SCTLR_AFE     (1U << 29)
#define SCTLR_TE      (1U << 30)

typedef struct SCTLR_EL
{

    SCTLR_EL(uint64_t src) { theVal = src; }
    uint32_t UCL() const { return theVal & SCTLR_UCI; }
    uint32_t DZE() const { return theVal & SCTLR_DZE; }

    uint32_t UMA() const { return theVal & SCTLR_UMA; }

  private:
    uint64_t theVal;

} SCTLR_EL;

#endif // FLEXUS_uARCHARM_SCTLR_EL_HPP_INCLUDED
