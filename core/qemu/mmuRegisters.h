#ifndef FLEXUS_ARM_MMU_REGISTERS_H_INCLUDED
#define FLEXUS_ARM_MMU_REGISTERS_H_INCLUDED
typedef unsigned long long mmu_reg_t;

typedef struct mmu_bit_configs {
    /* Msutherl - Aug'18
     * - used to extract WHERE certain bits are stored (such as EE and M in SCTLR) in its emulated registers.
     */
    // enable and endian-ness
    // - in SCTLR_ELx
    uint8_t M_Bit; // 0b1 = MMU enabled
    uint8_t EE_Bit; // endianness of EL1, 0b0 is little-endian
    uint8_t EoE_Bit; // endianness of EL0, 0b0 is little-endian

    // Granule that this MMU supports
    // - in ID_AA64MMFR0_EL1
    uint8_t TGran4_Base;  // = 0b0000 IF supported, = 0b1111 if not
    uint8_t TGran16_Base; // = 0b0000 if NOT supported, 0b0001 IF supported (yes, this is correct)
    uint8_t TGran64_Base; // = 0b0000 IF supported, = 0b1111 if not
    uint8_t TGran_NumBits;

    // Granule configured for EL1 (TODO: add others if we ever care about them in the future)
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

typedef struct mmu_regs {
    /* Msutherl - june'18
     * Defined all registers, will only code MMU for stage 1 tablewalk (no monitor or hypervisor)
     */

    mmu_reg_t SCTLR[4];
    mmu_reg_t TCR[4];

    mmu_reg_t TTBR0[4];
    mmu_reg_t TTBR1[3]; // only EL1/EL2

    mmu_reg_t ID_AA64MMFR0_EL1; // only implemented in EL1 as far as I know.
    /* type      -      ARM_NAME            - funct.
     * ----DO NOT REMOVE---- PEOPLE WILL NEED TO KNOW THIS *
    mmu_reg_t           SCTLR_EL1;          // enables/disables Secure EL1/EL0 translation
    mmu_reg_t           SCTLR_EL2;          // enables/disables Non-Secure EL2 Stage 1, Non-Secure EL1/EL0 Stage 2
    mmu_reg_t           SCTLR_EL3;          // enables/disables Secure EL3 Stage 1
    // all SCTLRs also include cacheability bits for PTEs
    //
    mmu_reg_t           TCR_EL1;            // controls Secure/NonSecure EL1/EL0
    mmu_reg_t           TCR_EL2;            // controls Secure EL2Stg1 and Non-Secure EL1/EL0 Stage 2
    mmu_reg_t           TCR_EL3;            // controls Secure EL3 Stage 1

    mmu_reg_t           TTBR0_EL1;          // controls lower address range for EL1 (default 0x0 - 0xffff ffff ffff), configurable through TCR_ELx.T0SZ
    mmu_reg_t           TTBR1_EL1;          // upper address range for EL1, size also configurable through TCR_ELx.T1SZ
    mmu_reg_t           TTBR0_EL3;          // controls single address range for EL3
    mmu_reg_t           TTBR0_EL2;          // controls lower address range for EL2 (default 0x0 - 0xffff ffff ffff), config. as above
    mmu_reg_t           TTBR1_EL2;          // upper address range for EL2
     * ----DO NOT REMOVE---- PEOPLE WILL NEED TO KNOW THIS */
} mmu_regs_t;

#endif // FLEXUS_ARM_MMU_REGISTERS_HPP_INCLUDED
