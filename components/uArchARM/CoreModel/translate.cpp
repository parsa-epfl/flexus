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

#include "coreModelImpl.hpp"
#include <core/qemu/ARMmmu.hpp>
#include <components/CommonQEMU/Transports/TranslationTransport.hpp>
#include <core/types.hpp>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArchARM {
    using namespace MMU;
    using namespace Flexus::Qemu::API;
    using Flexus::SharedTypes::Translation;
    using Flexus::SharedTypes::TranslationState;
    using Flexus::SharedTypes::TranslationTransport;

    void CoreImpl::InitMMU( std::shared_ptr<mmu_regs_t> regsFromQemu ) {
      this->theMMU = std::make_shared<mmu_t>();
      mmu_regs_t* rawRegs = reinterpret_cast<mmu_regs_t*>( regsFromQemu.get() );

      theMMU->initRegsFromQEMUObject( rawRegs );
      theMMU->setupAddressSpaceSizesAndGranules();
      this->mmuInitialized = true;
      DBG_(Tmp,( << "MMU object init'd, " << std::hex << theMMU << std::dec ));
    }

    bool CoreImpl::IsTranslationEnabledAtEL(uint8_t & anEL) {
        return theMMU->IsExcLevelEnabled(anEL);
    }

    void CoreImpl::translate(Flexus::SharedTypes::Translation& aTranslation) {

        TranslationTransport newTransport;
        boost::intrusive_ptr<TranslationState> statefulTranslation( new TranslationState() );
        boost::intrusive_ptr<Translation> basicTranslation( &aTranslation );
        newTransport.set(TranslationBasicTag, basicTranslation);
        newTransport.set(TranslationStatefulTag, statefulTranslation);
        /* Translation looks like this:
         * - Call into MMU to setup the translation parameter
         * - For each level, decode and create the TTE Access
         */
        InitialTranslationSetup(newTransport);
        // getting here only happens on a NEW translation req.
        for(int i = 0; i < newTransport[TranslationStatefulTag]->requiredTableLookups;i++) {
            bool done = doTTEAccess(newTransport);
            if( done ) break;
        } 
        // once we get here, output address is in the PhysicalAddress field of aTranslation, done
    }

    void CoreImpl::intermediateTranslationStep(Flexus::SharedTypes::Translation& aTr) { } 
    // TODO: this func, permissions check etc. // TODO: will need to change to TranslationTransport at some point

    // Private MMU internal functionality
    // - Msutherl: Oct'18
    void CoreImpl::InitialTranslationSetup( TranslationTransport& aTranslation ) {
        // setup stateful API that gets passed along with the tr.
        boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
        boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);
        int br = theMMU->checkBR0RangeForVAddr( basicPointer->theVaddr );
        if( br != -1 ) {
            if( br == 0 ) {
                statefulPointer->isBR0 = true;
            } else statefulPointer->isBR0 = false;
        }
        else DBG_Assert(false, ( << "FAULTING Vaddr, neither in BR0 or BR1: " << std::hex << basicPointer->theVaddr << std::dec ));
        uint8_t initialLevel = theMMU->getInitialLookupLevel(statefulPointer->isBR0);
        statefulPointer->requiredTableLookups = 4 - initialLevel;
        statefulPointer->currentLookupLevel = initialLevel;
        statefulPointer->granuleSize = theMMU->getGranuleSize(statefulPointer->isBR0);
        statefulPointer->ELRegime = currentEL();


        uint8_t EL = statefulPointer->ELRegime;
        uint64_t initialTTBR;
        if(statefulPointer->isBR0) initialTTBR = theMMU->mmu_regs.TTBR0[EL];
        else initialTTBR = theMMU->mmu_regs.TTBR1[EL];
        setupTTResolver(aTranslation, initialTTBR);
    }

    bool CoreImpl::doTTEAccess( TranslationTransport&  aTranslation ) {
        /* 
         * 1) Entire phys addr comes from the TTAddressResolver object
         * 2) Do access (right now, with magic QEMU)
         * 3) Assert validity etc.
         * 4) Setup next TTAddressResolver
         * - Foreach level of tablewalk
         *    - Calculate the bits that will be resolved. Depends on:
         *        - IA/OA sizes.
         *        - Granule of this translation regime
         *        - level of walk
         *    - Read base+offset of table (PAddr) and get TTE descriptor
         *    - Parse it, get the matching PA bits, and check if done
         *    - Raise fault if need be (TODO)
         */
        boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
        boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

        if( statefulPointer->currentLookupLevel == 0 ) {
            PhysicalMemoryAddress magicPaddr(QEMU_logical_to_physical(*theQEMUCPU,QEMU_DI_Instruction,basicPointer->theVaddr));
            DBG_(Tmp,( <<" QEMU Translated: " << std::hex << basicPointer->theVaddr << std::dec << ", to: " << std::hex << magicPaddr << std::dec));
        }
        PhysicalMemoryAddress TTEDescriptor( statefulPointer->TTAddressResolver->resolve(basicPointer->theVaddr) );
        DBG_(Tmp,(<< "Current Translation Level: " << (unsigned int) statefulPointer->currentLookupLevel
                    << ", Returned TTE Descriptor Address: " << std::hex <<  TTEDescriptor << std::dec ));
        unsigned long long rawTTEValue = Flexus::Core::construct(QEMU_read_phys_memory(  TTEDescriptor, 8 ), 8).to_ulong(); // TODO: check w mark
        DBG_(Tmp,(<< "Current Translation Level: " << (unsigned int) statefulPointer->currentLookupLevel
                    << ", Read Raw TTE Desc. from QEMU : " << std::hex << rawTTEValue << std::dec ));
        /* Check Valid */
        bool validBit = extractSingleBitAsBool(rawTTEValue,0);
        DBG_Assert( validBit == true , ( << "Encountered INVALID entry in doTTEAccess: " << std::hex << rawTTEValue << std::dec << ", need to generate Page Fault."));
        /* distinguish between block and table entries, could cut pwalk early!!! */
        bool isNextLevelTableEntry = extractSingleBitAsBool(rawTTEValue,1);
        if( statefulPointer->currentLookupLevel == 3 ) { /* Last level for all granules */
            PhysicalMemoryAddress shiftedRegionBits = PhysicalMemoryAddress(statefulPointer->TTAddressResolver->getBlockOutputBits(rawTTEValue));
            basicPointer->thePaddr = shiftedRegionBits;
            statefulPointer->BlockSizeFromTTs = 1 << 12; // 4KB
            //FIXME: FIXME: other granule sizes

            PhysicalMemoryAddress PageOffsetMask( statefulPointer->granuleSize-1 );
            PhysicalMemoryAddress maskedVAddr( basicPointer->theVaddr & PageOffsetMask );
            basicPointer->thePaddr |= maskedVAddr;
            return true; // pwalk done
        } else { /* Intermediate level */
            if( isNextLevelTableEntry ) {
                /* Remake the next level resolver */
                statefulPointer->currentLookupLevel += 1;
                setupTTResolver(aTranslation,rawTTEValue);
                return false;
            } else { // block entry - mem. mapped
                /* Return size of mapped region, output address, and terminate PWalk */
                PhysicalMemoryAddress shiftedRegionBits = PhysicalMemoryAddress(statefulPointer->TTAddressResolver->getBlockOutputBits(rawTTEValue));
                basicPointer->thePaddr = shiftedRegionBits;
                switch( statefulPointer->currentLookupLevel ) {
                    case 1:
                        statefulPointer->BlockSizeFromTTs = 1<<30; // 1GB block
                        break;
                    case 2:
                        statefulPointer->BlockSizeFromTTs = 1<<21; // 2MB block
                        break;
                    default:
                        DBG_Assert( false , ( << "Encountered Non-standard BLOCK entry, in intermediate TT walk stage " << (unsigned int) statefulPointer->currentLookupLevel << ", should have been a memory mapped region. TTE = " << std::hex << rawTTEValue << std::dec));
                }
                PhysicalMemoryAddress PageOffsetMask( statefulPointer->granuleSize-1 );
                PhysicalMemoryAddress maskedVAddr( basicPointer->theVaddr & PageOffsetMask );
                basicPointer->thePaddr |= maskedVAddr;
                return true; // p walk done
            }
        } // end intermediate level block
    } // end doTTAccess function

    /* Local helper function to set up a TTResolver based on level and TTBR */
    void CoreImpl::setupTTResolver( TranslationTransport& aTr, uint64_t TTDescriptor ) {
        boost::intrusive_ptr<TranslationState> statefulPointer(aTr[TranslationStatefulTag]);
        boost::intrusive_ptr<Translation> basicPointer(aTr[TranslationBasicTag]);
        uint8_t PAWidth = theMMU->getPAWidth(statefulPointer->isBR0);
        // Resolve TTBR base.
        switch( statefulPointer->currentLookupLevel ) {
            case 0:
                statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                        std::make_shared<MMU::L0Resolver>(statefulPointer->isBR0,theMMU->Gran0, TTDescriptor,PAWidth) :
                        std::make_shared<MMU::L0Resolver>(statefulPointer->isBR0,theMMU->Gran1,TTDescriptor,PAWidth) );
                break;
            case 1:
                statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                        std::make_shared<MMU::L1Resolver>(statefulPointer->isBR0,
                            theMMU->Gran0, TTDescriptor,PAWidth) :
                        std::make_shared<MMU::L1Resolver>(statefulPointer->isBR0,
                            theMMU->Gran1,TTDescriptor,PAWidth) );
                break;
            case 2:
                statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                        std::make_shared<MMU::L2Resolver>(statefulPointer->isBR0,
                            theMMU->Gran0, TTDescriptor ,PAWidth) :
                        std::make_shared<MMU::L2Resolver>(statefulPointer->isBR0,
                            theMMU->Gran1,TTDescriptor,PAWidth) );
                break;
            case 3:
                statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                        std::make_shared<MMU::L3Resolver>(statefulPointer->isBR0,
                            theMMU->Gran0, TTDescriptor,PAWidth) :
                        std::make_shared<MMU::L3Resolver>(statefulPointer->isBR0,
                            theMMU->Gran1,TTDescriptor,PAWidth) );
                break;
            default:
                DBG_Assert(false, ( << "Random lookup level in InitialTranslationSetup: " << statefulPointer->currentLookupLevel));
        }
    }


    //TODO??
    TTEDescriptor getNextTTDescriptor(Translation& aTr ) {DBG_Assert(false); }

} // end namespace nuArchARM
