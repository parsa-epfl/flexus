#ifndef FLEXUS_SIMICS_ARM_MMU_HPP_INCLUDED
#define FLEXUS_SIMICS_ARM_MMU_HPP_INCLUDED
#include <cmath>

namespace Flexus {
namespace Qemu {

namespace MMU {

typedef unsigned long long address_t;
typedef unsigned long long mmu_reg_t;
typedef unsigned char asi_t;

//    The MMU has the following features:

//    48-entry fully-associative L1 instruction TLB.
//    32-entry fully-associative L1 data TLB for data load and store pipelines.

//    4-way set-associative 1024-entry L2 TLB in each processor.
//    Intermediate table walk caches.
//    The TLB entries contain a global indicator or an Address Space Identifier (ASID) to permit context switches without TLB flushes.
//    The TLB entries contain a Virtual Machine Identifier (VMID) to permit virtual machine switches without TLB flushes.

//    Each TLB entry typically contains not just physical
//    and Virtual Addresses, but also attributes such as memory type,
//    cache policies, access permissions, the Address Space ID (ASID),
//    and the Virtual Machine ID (VMID).

typedef struct mmu_regs {
    /* Msutherl - june'18
     * Defined all registers, only coded MMU for stage 1 tablewalk (no monitor or hypervisor)
     */
    /* type      -      ARM_NAME            - funct. */
    mmu_reg_t           SCTLR_EL1;          // enables/disables Secure EL1/EL0 translation
    mmu_reg_t           SCTLR_EL2;          // enables/disables Non-Secure EL2 Stage 1, Non-Secure EL1/EL0 Stage 2
    mmu_reg_t           SCTLR_EL3;          // enables/disables Secure EL3 Stage 1
    // all SCTLRs also include cacheability bits for PTEs
    
    mmu_reg_t           TCR_EL1;            // controls Secure/NonSecure EL1/EL0
    mmu_reg_t           TCR_EL2;            // controls Secure EL2Stg1 and Non-Secure EL1/EL0 Stage 2
    mmu_reg_t           TCR_EL3;            // controls Secure EL3 Stage 1

    mmu_reg_t           TTBR0_EL1;          // controls lower address range for EL1 (default 0x0 - 0xffff ffff ffff), configurable through TCR_ELx.T0SZ
    mmu_reg_t           TTBR1_EL1;          // upper address range for EL1, size also configurable through TCR_ELx.T1SZ
    mmu_reg_t           TTBR0_EL3;          // controls single address range for EL3
    mmu_reg_t           TTBR0_EL2;          // controls lower address range for EL2 (default 0x0 - 0xffff ffff ffff), config. as above
    mmu_reg_t           TTBR1_EL2;          // upper address range for EL2
} mmu_regs_t;

/* Msutherl - june'18
 * - added definitions for granules and varying sizes
 */
class TranslationGranule
{
    private:
        unsigned KBSize;
        unsigned logKBSize;
        unsigned granuleShift;
        unsigned granuleEntries;

    public:
        TranslationGranule() : KBSize(4096), logKBSize(12), granuleShift(9), granuleEntries(512) { }
        TranslationGranule(unsigned ksize) : KBSize(ksize), logKBSize(log2(ksize)),
            granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)) { }
            // -3 for all ARM specs (ARMv8 ref manual, D4-2021)
};

bool is4KSupported();
bool is16KGranuleSupported();
bool is64KGranuleSupported();
/* Support or lack thereof is coded in register:
 * ID_AA64MMFR0_EL1.TGRAN4 (TGRAN16, TGRAN64) // FIXME: add to libqflexAPI?
 */

class TTEDescriptor
{
    public:
        TTEDescriptor() {} 
        typedef unsigned long long tte_raw_t;
    private:
        tte_raw_t rawDescriptor;
        TranslationGranule matchingGranule;
};

// Msutherl - antiquated + orphaned, needs to go soon
typedef unsigned long long tte_tag;
typedef unsigned long long tte_data;

typedef struct mmu {

    /* Msutherl - jun'18
     * - Adds system registers for controlling table walk
     * - TLB tags/data are refactored into Flexus state 
     */
    mmu_regs_t mmu_regs;

  /* general MMU registers */
    /*
  mmu_reg_t primary_context;
  mmu_reg_t secondary_context;
  mmu_reg_t pa_watchpoint;
  mmu_reg_t va_watchpoint;
  */

  /* D- and I-TLB registers */
  /*tlb_regs_t d_regs;
  /tlb_regs_t i_regs;
  */

  //unsigned long long lfsr;   /* LFSR bits for replacement */

  /* D-TLB tags and data */
    /*
  tte_tag  dt16_tag[16];
  tte_data dt16_data[16];
  tte_tag  dt512_tag[512];
  tte_data dt512_data[512];
  */

  /* I-TLB tags and data */
    /*
  tte_tag  it16_tag[16];
  tte_data it16_data[16];
  tte_tag  it128_tag[128];
  tte_data it128_data[128];
  */

} mmu_t;

typedef enum {
  CLASS_ASI_PRIMARY        = 0,
  CLASS_ASI_SECONDARY      = 1,
  CLASS_ASI_NUCLEUS        = 2,
  CLASS_ASI_NONTRANSLATING = 4
} asi_class_t;

typedef enum {
  MMU_TRANSLATE = 0,  /* for 'normal' translations */
  MMU_TRANSLATE_PF,   /* translate but don't trap */
  MMU_DEMAP_PAGE
} mmu_translation_type_t;

typedef enum {
  MMU_TSB_8K_PTR = 0,
  MMU_TSB_64K_PTR
} mmu_ptr_type_t;

/* taken from simics arm_exception_type */
typedef enum {
  no_exception                     = 0x000,
  /* */
  instruction_access_exception     = 0x008,
  instruction_access_mmu_miss      = 0x009,
  instruction_access_error         = 0x00a,
  /* */
  data_access_exception            = 0x030,
  data_access_mmu_miss             = 0x031,
  data_access_error                = 0x032,
  data_access_protection           = 0x033,
  mem_address_not_aligned          = 0x034,
  /* */
  priveledged_action               = 0x037,
  /* */
  fast_instruction_access_MMU_miss = 0x064,
  fast_data_access_MMU_miss        = 0x068,
  fast_data_access_protection      = 0x06c
  /* */
} mmu_exception_t;

typedef enum {
  mmu_access_load  = 1,
  mmu_access_store = 2
} mmu_access_type_t;

typedef struct mmu_access {
  address_t va;     /* virtual address */
  asi_t asi;         /* ASI */
  mmu_access_type_t type;        /* load or store */
  mmu_reg_t val;  /* store value or load result */
} mmu_access_t;

/* prototypes */
bool mmu_is_cacheable(tte_data data);
bool mmu_is_sideeffect(tte_data data);
bool mmu_is_xendian(tte_data data);
address_t mmu_make_paddr(tte_data data, address_t va);

address_t mmu_translate(mmu_t * mmu,
                        unsigned int is_fetch,
                        address_t va,
                        unsigned int asi_class,
                        unsigned int asi,
                        unsigned int nofault,
                        unsigned int priv,
                        unsigned int access_type,
                        mmu_exception_t * except,
                        mmu_translation_type_t trans_type);

tte_data mmu_lookup(mmu_t * mmu,
                    unsigned int is_fetch,
                    address_t va,
                    unsigned int asi_class,
                    unsigned int asi,
                    unsigned int nofault,
                    unsigned int priv,
                    unsigned int access_type,
                    mmu_exception_t * except,
                    mmu_translation_type_t trans_type);

address_t mmu_generate_tsb_ptr(address_t va,
                               mmu_ptr_type_t type,
                               address_t tsb_base_in,
                               unsigned int tsb_split,
                               unsigned int tsb_size,
                               mmu_reg_t tsb_ext);

void mmu_access(mmu_t * mmu, mmu_access_t * access);

void fm_print_mmu_regs(mmu_t * mmu);
int fm_compare_regs(mmu_regs_t* a, mmu_regs_t * b, const char * who);
int fm_compare_mmus(mmu_t * a, mmu_t * b);

} //end namespace MMU
//#endif
} //end Namespace Qemu
} //end namespace Flexus

//#if FLEXUS_TARGET_IS(arm)
#include <core/qemu/mai_api.hpp>
namespace Flexus {
namespace Qemu {
namespace MMU {
}
}
}

#endif //FLEXUS_SIMICS_MAI_API_HPP_INCLUDED
