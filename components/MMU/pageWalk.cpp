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

#include "pageWalk.hpp"
#include <components/CommonQEMU/Translation.hpp>
#include <core/debug/debug.hpp>

#define DBG_DeclareCategories MMU
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()

using namespace Flexus::Qemu;
using Flexus::SharedTypes::Translation;
using Flexus::SharedTypes::TranslationPtr;
using Flexus::SharedTypes::TranslationTransport;

namespace nMMU {
void PageWalk::annulAll() {
  if (TheInitialized && theTranslationTransports.size() > 0)
    while (theTranslationTransports.size() > 0) {
      boost::intrusive_ptr<Translation> basicPointer(
          theTranslationTransports.front()[TranslationBasicTag]);
      DBG_(VVerb, (<< "Annulling PW entry " << basicPointer->theVaddr));
      theTranslationTransports.pop_back();
    }
}

void PageWalk::preWalk(TranslationTransport &aTranslation) {
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
  DBG_(VVerb, (<< "preWalking " << basicPointer->theVaddr));

  if (statefulPointer->currentLookupLevel == 0) {
    PhysicalMemoryAddress magicPaddr(API::qemu_api.translate_va2pa(theNode, basicPointer->theVaddr));
    DBG_(VVerb, (<< " QEMU Translated: " << std::hex << basicPointer->theVaddr << std::dec
                 << ", to: " << std::hex << magicPaddr << std::dec));
  }
  PhysicalMemoryAddress TTEDescriptor(
      statefulPointer->TTAddressResolver->resolve(basicPointer->theVaddr));
  DBG_(VVerb, (<< "Current Translation Level: " << (unsigned int)statefulPointer->currentLookupLevel
               << ", Returned TTE Descriptor Address: " << TTEDescriptor << std::dec));

  basicPointer->thePaddr = TTEDescriptor;
  trace_address = TTEDescriptor;
  //    uint64_t rawTTEValue = Flexus::Core::construct(QEMU_read_phys_memory(
  //    TTEDescriptor, 8 ), 8);
  if (!basicPointer->inTraceMode) {
    pushMemoryRequest(basicPointer);
  }
}

bool PageWalk::walk(TranslationTransport &aTranslation) {
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);
  DBG_(VVerb, (<< "preWalking " << basicPointer->theVaddr));

  DBG_(VVerb, (<< "Current Translation Level: " << (unsigned int)statefulPointer->currentLookupLevel
               << ", Read Raw TTE Desc. from QEMU : " << std::hex << basicPointer->rawTTEValue
               << std::dec));
  /* Check Valid */
  bool validBit = extractSingleBitAsBool(basicPointer->rawTTEValue, 0);
  if (basicPointer->theInstruction) {
    DBG_(VVerb, (<< "Walking " << *basicPointer->theInstruction));
  }
  if (validBit != true) {
    if (basicPointer->isData() && basicPointer->theInstruction) {
      basicPointer->theInstruction->forceResync();
    } else {
      basicPointer->setPagefault();
    }
    basicPointer->setDone();
    return false;
  }
  //    DBG_Assert( validBit == true , ( << "Encountered INVALID entry in
  //    doTTEAccess: " << std::hex << basicPointer->rawTTEValue << std::dec <<
  //    ", need to generate Page Fault."));
  /* distinguish between block and table entries, could cut pwalk early!!! */
  bool isNextLevelTableEntry = extractSingleBitAsBool(basicPointer->rawTTEValue, 1);
  if (statefulPointer->currentLookupLevel == 3) { /* Last level for all granules */
    PhysicalMemoryAddress shiftedRegionBits = PhysicalMemoryAddress(
        statefulPointer->TTAddressResolver->getBlockOutputBits(basicPointer->rawTTEValue));
    basicPointer->thePaddr = shiftedRegionBits;
    statefulPointer->BlockSizeFromTTs = 1 << 12; // 4KB
    // FIXME: FIXME: other granule sizes

    PhysicalMemoryAddress PageOffsetMask(statefulPointer->granuleSize - 1);
    PhysicalMemoryAddress maskedVAddr(basicPointer->theVaddr & PageOffsetMask);
    basicPointer->thePaddr |= maskedVAddr;
    basicPointer->setDone();
    return true; // pwalk done
  } else {       /* Intermediate level */
    if (isNextLevelTableEntry) {
      /* Remake the next level resolver */
      statefulPointer->currentLookupLevel += 1;
      setupTTResolver(aTranslation, basicPointer->rawTTEValue);
      return false;
    } else { // block entry - mem. mapped
      /* Return size of mapped region, output address, and terminate PWalk */
      PhysicalMemoryAddress shiftedRegionBits = PhysicalMemoryAddress(
          statefulPointer->TTAddressResolver->getBlockOutputBits(basicPointer->rawTTEValue));
      basicPointer->thePaddr = shiftedRegionBits;
      DBG_(VVerb, (<< "Encountered BLOCK ENTRY in TT level ["
                   << (unsigned int)statefulPointer->currentLookupLevel << "], TTE = " << std::hex
                   << basicPointer->rawTTEValue << std::dec << ", ShiftedRegionBits = " << std::hex
                   << shiftedRegionBits << std::dec));
      switch (statefulPointer->currentLookupLevel) {
      case 1:
        statefulPointer->BlockSizeFromTTs = 1 << 30; // 1GB block
        break;
      case 2:
        statefulPointer->BlockSizeFromTTs = 1 << 21; // 2MB block
        break;
      default:
        DBG_Assert(false, (<< "Encountered Non-standard BLOCK entry, in intermediate "
                              "TT walk stage "
                           << (unsigned int)statefulPointer->currentLookupLevel
                           << ", should have been a memory mapped region. TTE = " << std::hex
                           << basicPointer->rawTTEValue << std::dec));
      }
      // PhysicalMemoryAddress PageOffsetMask( statefulPointer->granuleSize-1 );
      PhysicalMemoryAddress PageOffsetMask(statefulPointer->BlockSizeFromTTs - 1);
      PhysicalMemoryAddress maskedVAddr(basicPointer->theVaddr & PageOffsetMask);
      basicPointer->thePaddr |= maskedVAddr;

      DBG_(VVerb, (<< " PageOffsetMask = " << std::hex << PageOffsetMask << std::dec
                   << ", maskedVaddr = " << std::hex << maskedVAddr << std::dec
                   << "PAddr to Return = " << std::hex << basicPointer->thePaddr << std::dec));
      basicPointer->setDone();
      return true; // p walk done
    }
  } // end intermediate level block
} // end doTTAccess function

/* Local helper function to set up a TTResolver based on level and TTBR */
void PageWalk::setupTTResolver(TranslationTransport &aTranslation, uint64_t TTDescriptor) {
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);
  uint8_t PAWidth = theMMU->getPAWidth(statefulPointer->isBR0);
  // Resolve TTBR base.
  switch (statefulPointer->currentLookupLevel) {
  case 0:
    statefulPointer->TTAddressResolver =
        (statefulPointer->isBR0
             ? std::make_shared<L0Resolver>(statefulPointer->isBR0, theMMU->Gran0, TTDescriptor,
                                            PAWidth)
             : std::make_shared<L0Resolver>(statefulPointer->isBR0, theMMU->Gran1, TTDescriptor,
                                            PAWidth));
    break;
  case 1:
    statefulPointer->TTAddressResolver =
        (statefulPointer->isBR0
             ? std::make_shared<L1Resolver>(statefulPointer->isBR0, theMMU->Gran0, TTDescriptor,
                                            PAWidth)
             : std::make_shared<L1Resolver>(statefulPointer->isBR0, theMMU->Gran1, TTDescriptor,
                                            PAWidth));
    break;
  case 2:
    statefulPointer->TTAddressResolver =
        (statefulPointer->isBR0
             ? std::make_shared<L2Resolver>(statefulPointer->isBR0, theMMU->Gran0, TTDescriptor,
                                            PAWidth)
             : std::make_shared<L2Resolver>(statefulPointer->isBR0, theMMU->Gran1, TTDescriptor,
                                            PAWidth));
    break;
  case 3:
    statefulPointer->TTAddressResolver =
        (statefulPointer->isBR0
             ? std::make_shared<L3Resolver>(statefulPointer->isBR0, theMMU->Gran0, TTDescriptor,
                                            PAWidth)
             : std::make_shared<L3Resolver>(statefulPointer->isBR0, theMMU->Gran1, TTDescriptor,
                                            PAWidth));
    break;
  default:
    DBG_Assert(false, (<< "Random lookup level in InitialTranslationSetup: "
                       << statefulPointer->currentLookupLevel));
  }
}

bool PageWalk::push_back(TranslationPtr aTranslation) {

  TranslationTransport newTransport;
  boost::intrusive_ptr<TranslationState> statefulTranslation(new TranslationState());
  TranslationPtr basicTranslation = aTranslation;

  newTransport.set(TranslationBasicTag, basicTranslation);
  newTransport.set(TranslationStatefulTag, statefulTranslation);
  /* Translation looks like this:
   * - Call into nMMU to setup the translation parameter
   * - For each level, decode and create the TTE Access
   */
  if (!InitialTranslationSetup(newTransport))
    return false;
  theTranslationTransports.push_back(newTransport);
  if (!TheInitialized)
    TheInitialized = true;
  return true;
}

bool PageWalk::push_back_trace(TranslationPtr aTranslation, Flexus::Qemu::Processor theCPU) {
  TranslationTransport newTransport;
  boost::intrusive_ptr<TranslationState> statefulTranslation(new TranslationState());
  TranslationPtr basicTranslation = aTranslation;

  newTransport.set(TranslationBasicTag, basicTranslation);
  newTransport.set(TranslationStatefulTag, statefulTranslation);
  if (!InitialTranslationSetup(newTransport))
    return false;

  while (1) {
    preTranslate(newTransport);
    basicTranslation->rawTTEValue = (uint64_t)theCPU.read_pa(trace_address, 8);
    aTranslation->trace_addresses.push(trace_address);
    newTransport.set(TranslationBasicTag, basicTranslation);
    translate(newTransport);
    if (basicTranslation->isDone())
      break;
  }

  return true;
}

// Private nMMU internal functionality
// - Msutherl: Oct'18
bool PageWalk::InitialTranslationSetup(TranslationTransport &aTranslation) {
  // setup stateful API that gets passed along with the tr.
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);
  int br = theMMU->checkBR0RangeForVAddr(basicPointer->theVaddr);
  if (br != -1) {
    if (br == 0) {
      statefulPointer->isBR0 = true;
    } else
      statefulPointer->isBR0 = false;
  } else {
    DBG_(Dev, (<< "FAULTING Vaddr, neither in BR0 or BR1: " << std::hex << basicPointer->theVaddr
               << std::dec << ", Dropping Request"));
    return false;
  }
  uint8_t initialLevel = theMMU->getInitialLookupLevel(statefulPointer->isBR0);
  statefulPointer->requiredTableLookups = 4 - initialLevel;
  statefulPointer->currentLookupLevel = initialLevel;
  statefulPointer->granuleSize = theMMU->getGranuleSize(statefulPointer->isBR0);
  statefulPointer->ELRegime = 1; /*currentEL();*/

  uint8_t EL = statefulPointer->ELRegime;
  uint64_t initialTTBR;
  if (statefulPointer->isBR0)
    initialTTBR = theMMU->mmu_regs.TTBR0[EL];
  else
    initialTTBR = theMMU->mmu_regs.TTBR1[EL];
  setupTTResolver(aTranslation, initialTTBR);
  return true;
}

void PageWalk::preTranslate(TranslationTransport &aTransport) {

  boost::intrusive_ptr<TranslationState> statefulPointer(aTransport[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTransport[TranslationBasicTag]);

  DBG_(VVerb, (<< "preTranslating " << basicPointer->theVaddr));
  // getting here only happens on a NEW translation req.
  if (basicPointer->theCurrentTranslationLevel < statefulPointer->requiredTableLookups) {
    ++basicPointer->theCurrentTranslationLevel;
    preWalk(aTransport);

  } else {
    DBG_(VVerb, (<< "theCurrentTranslationLevel >= "
                    "statefulPointer->requiredTableLookups "
                 << basicPointer->theVaddr));
  }
  // once we get here, output address is in the PhysicalAddress field of
  // aTranslation, done
}

void PageWalk::translate(TranslationTransport &aTransport) {
  walk(aTransport);
}

//    void
//    CoreImpl::intermediateTranslationStep(boost::intrusive_ptr<Translation>&
//    aTr) { }
// TODO: this func, permissions check etc. // TODO: will need to change to
// TranslationTransport at some point

void PageWalk::cycle() {

  //        for (auto i = theTranslationTransports.begin(); i !=
  //        theTranslationTransports.end() && theTranslationTransports.size() >
  //        0; ++i) {

  if (theTranslationTransports.size() > 0) {

    auto i = theTranslationTransports.begin();

    TranslationTransport &item = *i;
    TranslationPtr basicPointer(item[TranslationBasicTag]);
    (*basicPointer)++;
    DBG_(VVerb, (<< "processing translation entry " << basicPointer->theVaddr));

    if ((theTranslationTransports.begin() == i) && basicPointer->isDone()) {
      DBG_(VVerb, (<< "translation is done for " << basicPointer->theVaddr));

      //                theDoneTranslations.push(basicPointer);
      theTranslationTransports.erase(i);
      return;
    }

    if (basicPointer->isReady()) {
      DBG_(VVerb, (<< "translation entry is ready " << basicPointer->theVaddr));

      if (!basicPointer->isWaiting()) {
        DBG_(VVerb, (<< "translation entry is not waiting " << basicPointer->theVaddr));

        preTranslate(item);
        basicPointer->toggleReady();
        if (basicPointer->isDone()) {
          return;
        }
      } else {
        translate(item);
      }
      basicPointer->toggleWaiting();
    }
  }
}

void PageWalk::pushMemoryRequest(TranslationPtr aTranslation) {
  theMemoryTranslations.push(aTranslation);
}

TranslationPtr PageWalk::popMemoryRequest() {
  assert(!theMemoryTranslations.empty());
  TranslationPtr tmp = theMemoryTranslations.front();
  theMemoryTranslations.pop();
  return tmp;
}

bool PageWalk::hasMemoryRequest() {
  return !theMemoryTranslations.empty();
}

// TODO??
//    TTEDescriptor PageWalk::getNextTTDescriptor(TranslationTransport & aTr ) {
//    DBG_Assert(false); }

} // end namespace nMMU