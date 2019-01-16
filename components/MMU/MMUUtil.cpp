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

#include <boost/throw_exception.hpp>
#include <memory>
#include <functional>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/qemu/api_wrappers.hpp>

#include <core/qemu/mai_api.hpp>
#include "MMUUtil.hpp"
#include <stdio.h>
#include <core/MakeUniqueWrapper.hpp>

#define DBG_DefineCategories MMUCat
#define DBG_SetDefaultOps AddCat(MMUCat)
#include DBG_Control()

namespace nMMU {

using namespace Flexus;


    /* Msutherl - June'18
     * - Add code for TTE Descriptor classes
     */
    bool TTEDescriptor::isValid() {
            return extractSingleBitAsBool(rawDescriptor,0);
    }
    bool TTEDescriptor::isTableEntry(){
            return extractSingleBitAsBool( rawDescriptor, 1 );
    }
    bool TTEDescriptor::isBlockEntry() {
                return !(extractSingleBitAsBool( rawDescriptor, 1 ));
        }

    void fm_print_mmu_regs(mmu_regs_t* r) {
        DBG_(VVerb,( << "SCTLR_EL1: " << std::hex << r->SCTLR[EL1] << std::dec << std::endl
            << "SCTLR_EL2: " << std::hex << r->SCTLR[EL2] << std::dec << std::endl
            << "SCTLR_EL3: " << std::hex << r->SCTLR[EL3] << std::dec << std::endl
            << "TCR_EL1: " << std::hex << r->TCR[EL1] << std::dec << std::endl
            << "TCR_EL2: " << std::hex << r->TCR[EL2] << std::dec << std::endl
            << "TCR_EL3: " << std::hex << r->TCR[EL3] << std::dec << std::endl
            << "TTBR0_EL1: " << std::hex << r->TTBR0[EL1] << std::dec << std::endl
            << "TTBR1_EL1: " << std::hex << r->TTBR1[EL1] << std::dec << std::endl
            << "TTBR0_EL2: " << std::hex << r->TTBR0[EL2] << std::dec << std::endl
            << "TTBR1_EL2: " << std::hex << r->TTBR1[EL2] << std::dec << std::endl
            << "TTBR0_EL3: " << std::hex << r->TTBR0[EL3] << std::dec << std::endl
            << "ID_AA64MMFR0_EL1: " << std::hex << r->ID_AA64MMFR0_EL1 << std::dec));
        ;
    }
    void mmu_t::setupBitConfigs() {
        // enable and endian-ness
        // - in SCTLR_ELx
        aarch64_bit_configs.M_Bit = 0; // 0b1 = MMU enabled
        aarch64_bit_configs.EE_Bit = 25; // endianness of EL1, 0b0 is little-endian
        aarch64_bit_configs.EoE_Bit = 24; // endianness of EL0, 0b0 is little-endian

        // Granule that this MMU supports
        // - in ID_AA64MMFR0_EL1
        aarch64_bit_configs.TGran4_Base = 28;  // = 0b0000 IF supported, = 0b1111 if not
        aarch64_bit_configs.TGran16_Base = 20; // = 0b0000 if NOT supported, 0b0001 IF supported (yes, this is correct)
        aarch64_bit_configs.TGran64_Base = 24; // = 0b0000 IF supported, = 0b1111 if not
        aarch64_bit_configs.TGran_NumBits = 4;

        // Granule configured for EL1 (TODO: add others if we ever care about them in the future)
        // - in TCR_ELx
        aarch64_bit_configs.TG0_Base = 14; /* 00 = 4KB
                                              01 = 64KB
                                              11 = 16KB */
        aarch64_bit_configs.TG0_NumBits = 2;
        aarch64_bit_configs.TG1_Base = 30; /* 01 = 16KB
                                              10 = 4KB
                                              11 = 64KB (yes, this is different than TG0) */
        aarch64_bit_configs.TG1_NumBits = 2;

        // Physical, output, and input address sizes. ( in ID_AA64MMFR0_EL1 )
        aarch64_bit_configs.PARange_Base = 0; /* 0b0000 = 32b
                                                 0b0001 = 36b
                                                 0b0010 = 40b
                                                 0b0011 = 42b
                                                 0b0100 = 44b
                                                 0b0101 = 48b
                                                 0b0110 = 52b */
        aarch64_bit_configs.PARange_NumBits = 4;

        // translation range sizes, chooses which VA's map to TTBR0 and TTBR1
        // - in TCR_ELx
        aarch64_bit_configs.TG0_SZ_Base = 0;
        aarch64_bit_configs.TG1_SZ_Base = 16;
        aarch64_bit_configs.TGn_SZ_NumBits = 6;
    }
    void mmu_t::initRegsFromQEMUObject(std::shared_ptr<mmu_regs_t>  qemuRegs) {
        setupBitConfigs(); // initialize AARCH64 bit locations for masking

        mmu_regs.SCTLR[EL1] = qemuRegs->SCTLR[EL1];
        mmu_regs.SCTLR[EL2] = qemuRegs->SCTLR[EL2];
        mmu_regs.SCTLR[EL3] = qemuRegs->SCTLR[EL3];
        mmu_regs.TCR[EL1] = qemuRegs->TCR[EL1];
        mmu_regs.TCR[EL2] = qemuRegs->TCR[EL2];
        mmu_regs.TCR[EL3] = qemuRegs->TCR[EL3];
        mmu_regs.TTBR0[EL1] = qemuRegs->TTBR0[EL1];
        mmu_regs.TTBR1[EL1] = qemuRegs->TTBR1[EL1];
        mmu_regs.TTBR0[EL2] = qemuRegs->TTBR0[EL2];
        mmu_regs.TTBR1[EL2] = qemuRegs->TTBR1[EL2];
        mmu_regs.TTBR0[EL3] = qemuRegs->TTBR0[EL3];
        mmu_regs.ID_AA64MMFR0_EL1 = qemuRegs->ID_AA64MMFR0_EL1;
        DBG_(VVerb , ( << "Initializing mmu registers from QEMU...."));
//        fm_print_mmu_regs(&(this->mmu_regs));
    }
    bool mmu_t::IsExcLevelEnabled(uint8_t EL) const {
        DBG_Assert( EL > 0 && EL <= 3,
                ( << "ERROR, ARM MMU: Transl. Request Not Supported at Invalid EL = " << EL ) );
        return extractSingleBitAsBool ( mmu_regs.SCTLR[EL], aarch64_bit_configs.M_Bit );
    }
    void mmu_t::setupAddressSpaceSizesAndGranules(void)
    {
        unsigned TG0_Size = getGranuleSize(0);
        unsigned TG1_Size = getGranuleSize(1);
        unsigned PASize = parsePASizeFromRegs();
        PASize = 48; // FIXME: ID_AAMMFR0 REGISTER ALWAYS 0 FROM QEMU????
        unsigned BR0_Offset = getIAOffsetValue(true);
        unsigned BR1_Offset = getIAOffsetValue(false);
        Gran0 = std::make_shared<TG0_Granule>(TG0_Size,PASize,BR0_Offset);
        Gran1 = std::make_shared<TG1_Granule>(TG1_Size,PASize,BR1_Offset);
    }

    /* Magic numbers taken from:
     * ARMv8-A ref manual, Page D7-2335
     */
    bool mmu_t::is4KGranuleSupported() {
        address_t TGran4 = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                aarch64_bit_configs.TGran4_Base,
                aarch64_bit_configs.TGran_NumBits);
        return ( TGran4 == 0b0000 );
    }
    bool mmu_t::is16KGranuleSupported() {
        address_t TGran16 = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                aarch64_bit_configs.TGran16_Base,
                aarch64_bit_configs.TGran_NumBits);
        return ( TGran16 == 0b0001 );
    }
    bool mmu_t::is64KGranuleSupported() {
        address_t TGran64 = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                aarch64_bit_configs.TGran64_Base,
                aarch64_bit_configs.TGran_NumBits);

        return ( TGran64 == 0b0000 );
    }

    /* Magic numbers taken from:
     * ARMv8-A ref manual, Page D7-2487.
     */
    uint32_t mmu_t::getGranuleSize(uint32_t granuleNum) {
        if( granuleNum == 0 ) { // TG0
            address_t TG_SZ = extractBitsWithRange(mmu_regs.TCR[EL1],
                    aarch64_bit_configs.TG0_Base,
                    aarch64_bit_configs.TG0_NumBits);
            switch ( TG_SZ ) {
                case 0b00:
                    return 1<<12; // 4KB
                case 0b01:
                    return 1<<16; // 64KB
                case 0b11:
                    return 1<<14; // 16KB
                default:
                    DBG_Assert(false, ( << "Unknown value in getting TG0 Granule Size. TG_SZ = " << TG_SZ ));
                    return 0;
            }
        } else { // TG1
            address_t TG_SZ = extractBitsWithRange(mmu_regs.TCR[EL1],
                    aarch64_bit_configs.TG1_Base,
                    aarch64_bit_configs.TG1_NumBits);
            switch ( TG_SZ ) {
                case 0b10:
                    return 1<<12; // 4KB
                case 0b11:
                    return 1<<16; // 64KB
                case 0b01:
                    return 1<<14; // 16KB
                default:
                    DBG_Assert(false, ( << "Unknown value in getting TG1 Granule Size. TG_SZ = " << TG_SZ ));
                    return 0;
            }
        }
    }
    uint32_t mmu_t::parsePASizeFromRegs() {
        address_t pRange_Config = extractBitsWithRange(mmu_regs.ID_AA64MMFR0_EL1,
                aarch64_bit_configs.PARange_Base,
                aarch64_bit_configs.PARange_NumBits);
        /* Magic numbers taken from:
         * ARMv8-A ref manual, Page D4-2014.
         */
        switch( pRange_Config ) {
            case 0b0000:
                return 32;
            case 0b0001:
                return 36;
            case 0b0010:
                return 40;
            case 0b0011:
                return 42;
            case 0b0100:
                return 44;
            case 0b0101:
                return 48;
            case 0b0110:
                return 52;
            default:
                DBG_Assert(false, ( << "Unknown value in getting PAddr Width. Raw Value from MMU = " << pRange_Config ));
                return 0;
        }
    }
    uint32_t mmu_t::getIAOffsetValue(bool isBRO) {
        address_t ret = 16;
        if( isBRO ) { // brooooooooooo, chahh!
            ret = extractBitsWithRange(mmu_regs.TCR[EL1],
                    aarch64_bit_configs.TG0_SZ_Base,
                    aarch64_bit_configs.TGn_SZ_NumBits);
        } else { // cheerio old boy!
            ret = extractBitsWithRange(mmu_regs.TCR[EL1],
                    aarch64_bit_configs.TG1_SZ_Base,
                    aarch64_bit_configs.TGn_SZ_NumBits);
        }
        return ret;
    }
    int mmu_t::checkBR0RangeForVAddr( VirtualMemoryAddress& theVaddr) const {
        uint64_t upperBR0Bound = Gran0->GetUpperAddressRangeLimit();
        uint64_t lowerBR1Bound = Gran1->GetLowerAddressRangeLimit();
        /*
           std::cout << "UBound for BR0: " << std::hex << upperBR0Bound << std::dec << ", "
           << "LBound for BR1: " << std::hex << lowerBR1Bound << std::dec << ", "
           << "vAddr: " << std::hex << aTr.theVaddr << std::dec << std::endl;
           */
        if( (uint64_t)theVaddr <= upperBR0Bound ) {
            return 0; // br0
        } else if ( (uint64_t)theVaddr >= lowerBR1Bound ) {
            return 1; // br1
        } else return -1; // fault
    }
    uint8_t  mmu_t::getInitialLookupLevel( bool& isBR0 ) const {
        return ( isBR0 ? Gran0->getInitialLookupLevel()
                : Gran1->getInitialLookupLevel() );
    }
    uint8_t mmu_t::getIASize(bool isBR0) const {
        return ( isBR0 ? Gran0->getIAddrSize()
                : Gran1->getIAddrSize() );
    }
    uint8_t mmu_t::getPAWidth(bool isBR0) const {
        return ( isBR0 ? Gran0->getPAddrWidth()
                : Gran1->getPAddrWidth() );
    }




} // end namespace nMMU

