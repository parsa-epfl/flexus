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


#include "pageWalk.hpp"

#define DBG_DeclareCategories MMU
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()

namespace nMMU {
    using namespace Flexus::Qemu::API;
    using Flexus::SharedTypes::Translation;
    using Flexus::SharedTypes::TranslationTransport;




void PageWalk::preWalk( TranslationTransport &  aTranslation ) {
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
        PhysicalMemoryAddress magicPaddr(QEMU_logical_to_physical(*Flexus::Qemu::Processor::getProcessor( theNode ),QEMU_DI_Instruction,basicPointer->theVaddr));
        DBG_(Tmp,( <<" QEMU Translated: " << std::hex << basicPointer->theVaddr << std::dec << ", to: " << std::hex << magicPaddr << std::dec));
    }
    PhysicalMemoryAddress TTEDescriptor( statefulPointer->TTAddressResolver->resolve(basicPointer->theVaddr) );
    DBG_(Tmp,(<< "Current Translation Level: " << (unsigned int) statefulPointer->currentLookupLevel
                << ", Returned TTE Descriptor Address: " << TTEDescriptor << std::dec ));

    basicPointer->thePaddr = TTEDescriptor;
    uint64_t rawTTEValue = Flexus::Core::construct(QEMU_read_phys_memory(  TTEDescriptor, 8 ), 8).to_ulong();
    pushMemoryRequest(basicPointer);
}

bool PageWalk::walk( TranslationTransport &  aTranslation ) {
    boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
    boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

    DBG_(Tmp,(<< "Current Translation Level: " << (unsigned int) statefulPointer->currentLookupLevel
                << ", Read Raw TTE Desc. from QEMU : " << std::hex << basicPointer->rawTTEValue << std::dec ));
    /* Check Valid */
    bool validBit = extractSingleBitAsBool(basicPointer->rawTTEValue,0);
    DBG_Assert( validBit == true , ( << "Encountered INVALID entry in doTTEAccess: " << std::hex << basicPointer->rawTTEValue << std::dec << ", need to generate Page Fault."));
    /* distinguish between block and table entries, could cut pwalk early!!! */
    bool isNextLevelTableEntry = extractSingleBitAsBool(basicPointer->rawTTEValue,1);
    if( statefulPointer->currentLookupLevel == 3 ) { /* Last level for all granules */
        PhysicalMemoryAddress shiftedRegionBits = PhysicalMemoryAddress(statefulPointer->TTAddressResolver->getBlockOutputBits(basicPointer->rawTTEValue));
        basicPointer->thePaddr = shiftedRegionBits;
        statefulPointer->BlockSizeFromTTs = 1 << 12; // 4KB
        //FIXME: FIXME: other granule sizes

        PhysicalMemoryAddress PageOffsetMask( statefulPointer->granuleSize-1 );
        PhysicalMemoryAddress maskedVAddr( basicPointer->theVaddr & PageOffsetMask );
        basicPointer->thePaddr |= maskedVAddr;
        basicPointer->setDone();
        return true; // pwalk done
    } else { /* Intermediate level */
        if( isNextLevelTableEntry ) {
            /* Remake the next level resolver */
            statefulPointer->currentLookupLevel += 1;
            setupTTResolver(aTranslation, basicPointer->rawTTEValue);
            return false;
        } else { // block entry - mem. mapped
            /* Return size of mapped region, output address, and terminate PWalk */
            PhysicalMemoryAddress shiftedRegionBits = PhysicalMemoryAddress(statefulPointer->TTAddressResolver->getBlockOutputBits(basicPointer->rawTTEValue));
            basicPointer->thePaddr = shiftedRegionBits;
            DBG_( Dev, ( << "Encountered BLOCK ENTRY in TT level [" << (unsigned int) statefulPointer->currentLookupLevel << "], TTE = " << std::hex << basicPointer->rawTTEValue << std::dec << ", ShiftedRegionBits = " << std::hex << shiftedRegionBits << std::dec));
            switch( statefulPointer->currentLookupLevel ) {
                case 1:
                    statefulPointer->BlockSizeFromTTs = 1<<30; // 1GB block
                    break;
                case 2:
                    statefulPointer->BlockSizeFromTTs = 1<<21; // 2MB block
                    break;
                default:
                    DBG_Assert( false , ( << "Encountered Non-standard BLOCK entry, in intermediate TT walk stage " << (unsigned int) statefulPointer->currentLookupLevel << ", should have been a memory mapped region. TTE = " << std::hex << basicPointer->rawTTEValue << std::dec));
            }
            //PhysicalMemoryAddress PageOffsetMask( statefulPointer->granuleSize-1 );
            PhysicalMemoryAddress PageOffsetMask( statefulPointer->BlockSizeFromTTs -1 );
            PhysicalMemoryAddress maskedVAddr( basicPointer->theVaddr & PageOffsetMask );
            basicPointer->thePaddr |= maskedVAddr;

            DBG_( Dev, ( << " PageOffsetMask = " << std::hex << PageOffsetMask << std::dec << ", maskedVaddr = " << std::hex << maskedVAddr << std::dec << "PAddr to Return = " << std::hex << basicPointer->thePaddr << std::dec ));
            basicPointer->setDone();
            return true; // p walk done
        }
    } // end intermediate level block
} // end doTTAccess function

/* Local helper function to set up a TTResolver based on level and TTBR */
void PageWalk::setupTTResolver( TranslationTransport & aTranslation, uint64_t TTDescriptor ) {
    boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
    boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);
    uint8_t PAWidth = theMMU->getPAWidth(statefulPointer->isBR0);
    // Resolve TTBR base.
    switch( statefulPointer->currentLookupLevel ) {
        case 0:
            statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                    std::make_shared<L0Resolver>(statefulPointer->isBR0,theMMU->Gran0, TTDescriptor,PAWidth) :
                    std::make_shared<L0Resolver>(statefulPointer->isBR0,theMMU->Gran1,TTDescriptor,PAWidth) );
            break;
        case 1:
            statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                    std::make_shared<L1Resolver>(statefulPointer->isBR0,
                        theMMU->Gran0, TTDescriptor,PAWidth) :
                    std::make_shared<L1Resolver>(statefulPointer->isBR0,
                        theMMU->Gran1,TTDescriptor,PAWidth) );
            break;
        case 2:
            statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                    std::make_shared<L2Resolver>(statefulPointer->isBR0,
                        theMMU->Gran0, TTDescriptor ,PAWidth) :
                    std::make_shared<L2Resolver>(statefulPointer->isBR0,
                        theMMU->Gran1,TTDescriptor,PAWidth) );
            break;
        case 3:
            statefulPointer->TTAddressResolver = ( statefulPointer->isBR0 ?
                    std::make_shared<L3Resolver>(statefulPointer->isBR0,
                        theMMU->Gran0, TTDescriptor,PAWidth) :
                    std::make_shared<L3Resolver>(statefulPointer->isBR0,
                        theMMU->Gran1,TTDescriptor,PAWidth) );
            break;
        default:
            DBG_Assert(false, ( << "Random lookup level in InitialTranslationSetup: " << statefulPointer->currentLookupLevel));
    }
}

    void PageWalk::push_back(boost::intrusive_ptr<Translation> aTranslation){

        TranslationTransport newTransport;
        boost::intrusive_ptr<TranslationState> statefulTranslation( new TranslationState());
        boost::intrusive_ptr<Translation> basicTranslation = aTranslation;

        newTransport.set(TranslationBasicTag, basicTranslation);
        newTransport.set(TranslationStatefulTag, statefulTranslation);
        /* Translation looks like this:
         * - Call into nMMU to setup the translation parameter
         * - For each level, decode and create the TTE Access
         */
        InitialTranslationSetup(newTransport);
        theTranlationTransports.push_back(newTransport);
    }

    // Private nMMU internal functionality
    // - Msutherl: Oct'18
    void PageWalk::InitialTranslationSetup( TranslationTransport & aTranslation ) {
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
        statefulPointer->ELRegime = 1;/*currentEL();*/


        uint8_t EL = statefulPointer->ELRegime;
        uint64_t initialTTBR;
        if(statefulPointer->isBR0) initialTTBR = theMMU->mmu_regs.TTBR0[EL];
        else initialTTBR = theMMU->mmu_regs.TTBR1[EL];
        setupTTResolver(aTranslation, initialTTBR);
    }

    void PageWalk::preTranslate(TranslationTransport & aTransport) {

        boost::intrusive_ptr<TranslationState> statefulPointer(aTransport[TranslationStatefulTag]);
        boost::intrusive_ptr<Translation> basicPointer(aTransport[TranslationBasicTag]);

        // getting here only happens on a NEW translation req.
        if(basicPointer->theCurrentTranslationLevel < statefulPointer->requiredTableLookups) {
            ++basicPointer->theCurrentTranslationLevel;
            preWalk(aTransport);

        }
        // once we get here, output address is in the PhysicalAddress field of aTranslation, done
    }

    void PageWalk::translate(TranslationTransport & aTransport) {
        walk(aTransport);
    }

//    void CoreImpl::intermediateTranslationStep(boost::intrusive_ptr<Translation>& aTr) { }
    // TODO: this func, permissions check etc. // TODO: will need to change to TranslationTransport at some point

    void PageWalk::cycle(){

//        for (auto i = theTranlationTransports.begin(); i < theTranlationTransports.end(); i++) {
            auto i = theTranlationTransports.begin();
            if (i == theTranlationTransports.end()) return;
            TranslationTransport& item = *i;
            boost::intrusive_ptr<Translation> basicPointer(item[TranslationBasicTag]);


            if (basicPointer->isReady()) {
                if (! basicPointer->isWaiting()){

                    preTranslate(item);
                    basicPointer->toggleReady();

                    if ((theTranlationTransports.begin() == i) && basicPointer->isDone()){
                        theDoneTranlations.push(basicPointer);
                        theTranlationTransports.erase(i);
                    }
                } else
                    translate(item);

                basicPointer->toggleWaiting();
            }
//        }
    }

    void PageWalk::pushMemoryRequest(TranslationPtr aTranslation){
        theMemoryTranlations.push(aTranslation);
    }

    TranslationPtr PageWalk::popMemoryRequest(){
        assert(!theMemoryTranlations.empty());
        TranslationPtr tmp = theMemoryTranlations.front();
        theMemoryTranlations.pop();
        return tmp;
    }


    bool PageWalk::hasMemoryRequest(){
        return !theMemoryTranlations.empty();
    }




    //TODO??
    TTEDescriptor PageWalk::getNextTTDescriptor(TranslationTransport & aTr ) { DBG_Assert(false); }

} // end namespace nMMU
