// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#ifndef FLEXUS_MMU_UTIL_HPP_INCLUDED
#define FLEXUS_MMU_UTIL_HPP_INCLUDED

#include <core/types.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include "TranslationState.hpp"
#include "mmuRegisters.hpp"
#include "ARMTranslationGranules.hpp"
#include "TTResolvers.hpp"
#include <core/qemu/bitUtilities.hpp>

using Flexus::SharedTypes::Translation;
using Flexus::SharedTypes::VirtualMemoryAddress;

typedef uint64_t address_t;
typedef unsigned char asi_t;
typedef unsigned long long tte_raw_t;


namespace nMMU {

#define VADDR_WIDTH 48


class TTEDescriptor
{
    public:
        // helper functions for reading ARM64 descriptors
        bool isValid();
        bool isTableEntry();
        bool isBlockEntry();

    private:
        tte_raw_t rawDescriptor;
};

/* RAW DESCRIPTOR FORMATS ON AARCH64 REFERENCE MANUAL - SECTION D4.3 - D4-2061
 * INVALID ENTRY bit[0] = 0
 * VALID BLOCK ENTRY: For level 1, n = 42, level 2, n = 29.
 * [ 63:51 | ---- | 50:48 | 47:n     | n-1:16 | 15:12  -- | 11:2 ----- | 1 | 0 ] 
 * [ upper attrs. | res0  | OA[47:n] |  res0  | OA[51:48] | low. attrs | 0 | 1 ]
 *
 * VALID TABLE ENTRY:
 * [ 63 ---- | 62:61 - | 60 ---- | 59 ----- | 58:51  | 50:48 |-----  47:16  ----- |----- 15:12 ------ | 11:2 - | 1 | 0 ]
 * [ NSTable | APTable | XNTable | PXNTable | IGNORE | res0  | Next-level [47:16] | Next-level[51:48] | IGNORE | 1 | 1 ]
 * - entries for NS,AP,XN,PXN only valid in Stage1, res0 in Stage 2
 */

// Msutherl - antiquated + orphaned, needs to go soon
typedef uint64_t tte_tag;
typedef uint64_t tte_data;

class mmu_t {
    public:
        /* Msutherl - jun'18
         * - Adds system registers for controlling table walk
         * - TLB tags/data are refactored into Flexus state 
         */
        mmu_regs_t mmu_regs;
        bit_configs_t aarch64_bit_configs;
        std::shared_ptr<TranslationGranule> Gran0;
        std::shared_ptr<TranslationGranule> Gran1;

        /* Accessors that get higher data about this MMU *
         * e.g., is it enabled, what sizes does it implement....
         * - ALL the references for the magic numbers come from the AARCH64 Manual,
         *      and have page #s documented re in ARM-mmu.txt (parsa wiki).
         * - Some of them are in mmuRegisters.h as well.
         */
        bool is4KGranuleSupported();
        bool is16KGranuleSupported();
        bool is64KGranuleSupported();
        uint32_t getGranuleSize(uint32_t granuleNum);
        uint32_t parsePASizeFromRegs();
        uint32_t getIAOffsetValue(bool isBRO);

        int checkBR0RangeForVAddr(VirtualMemoryAddress& theVA) const ;
        uint8_t getInitialLookupLevel( bool& isBR0 ) const ;
        uint8_t getIASize(bool isBR0) const ;
        uint8_t getPAWidth(bool isBR0) const ;

    public: /* Functions that are called externally from TLBs, uArch, etc.... */
        void initRegsFromQEMUObject(std::shared_ptr<mmu_regs_t> qemuRegs);
        bool IsExcLevelEnabled(uint8_t elToValidate) const ;
        void setupAddressSpaceSizesAndGranules(void);

    private:
        void setupBitConfigs();
};

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

// Msutherl - june'18
// - added smaller MMU interface (resolving walks + memory accesses resolved in Flexus components)
std::shared_ptr<mmu_regs_t> getMMURegsFromQEMU();

void fm_print_mmu_regs(mmu_regs_t* mmu);
} //end namespace nMMU


#endif //FLEXUS_MMU_UTIL_HPP_INCLUDED
