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

#include <core/debug/debug.hpp>
#include <core/qemu/bitUtilities.hpp>
#include <core/qemu/mai_api.hpp>

#include "PageWalk.hpp"

#define DBG_DeclareCategories MMU
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()

namespace nMMU {

using namespace Flexus::Qemu;
using namespace Flexus::SharedTypes;

void PageWalk::annulAll() {
  if (TheInitialized && theTranslationTransports.size() > 0)
    while (theTranslationTransports.size() > 0) {
      boost::intrusive_ptr<Translation> basicPointer(
          theTranslationTransports.front()[TranslationBasicTag]);
      DBG_(VVerb, (<< "Annulling PW entry " << basicPointer->theVaddr));
      theTranslationTransports.pop_back();
    }
}

void PageWalk::preWalkUat(TranslationTransport &aTranslation) {
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

  PhysicalMemoryAddress addr(uatBase(basicPointer->theVaddr, statefulPointer.get()));

  basicPointer->thePaddr = addr;
  trace_address = addr;

  if (!basicPointer->inTraceMode)
    pushMemoryRequest(basicPointer);
}

void PageWalk::preWalk(TranslationTransport &aTranslation) {
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

  if (basicPointer->isUat()) {
    preWalkUat(aTranslation);
    return;
  }

  DBG_(VVerb, (<< "preWalking " << basicPointer->theVaddr));

  PhysicalMemoryAddress TTEDescriptor;

  // sv39
  switch (statefulPointer->currentLookupLevel) {
  case 0:
    TTEDescriptor =  statefulPointer->ppn_root |
                    (extractBitsWithBounds(basicPointer->theVaddr,    38, 30) <<  3);
    break;
  case 1:
    TTEDescriptor = (extractBitsWithBounds(basicPointer->rawTTEValue, 53, 10) << 12) |
                    (extractBitsWithBounds(basicPointer->theVaddr,    29, 21) <<  3);
    break;
  case 2:
    TTEDescriptor = (extractBitsWithBounds(basicPointer->rawTTEValue, 53, 10) << 12) |
                    (extractBitsWithBounds(basicPointer->theVaddr,    20, 12) <<  3);
    break;
  }

  DBG_(VVerb, (<< "Current Translation Level: " << (unsigned int)statefulPointer->currentLookupLevel
               << ", Returned TTE Descriptor Address: " << TTEDescriptor << std::dec));

  basicPointer->thePaddr = TTEDescriptor;
  trace_address = TTEDescriptor;

  if (!basicPointer->inTraceMode) {
    pushMemoryRequest(basicPointer);
  }
}

//static uint64_t uat_get_bound(uint64_t *uat) {
//  return extractBitsWithBounds(uat[0], 51,  0);
//}

static uint64_t uat_get_attr(uint64_t *uat) {
  return extractBitsWithBounds(uat[0], 63, 56);
}

//static uint64_t uat_get_offs(uint64_t *uat) {
//  return extractBitsWithBounds(uat[1], 51, 0);
//}

static uint16_t uat_get_sub(uint64_t *uat, int n) {
  return ((uint16_t *)(uat))[n + 12];
}

static uint64_t uat_get_sub_attr(uint16_t sub) {
  return extractBitsWithBounds(sub, 15, 12);
}

static uint64_t uat_get_sub_csid(uint16_t sub) {
  return extractBitsWithBounds(sub, 11, 0);
}

bool PageWalk::walkUat(TranslationTransport &aTranslation) {
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

  uint64_t attr = uat_get_attr(basicPointer->rawUatValue);
  uint64_t csid = 0;

  if (!(attr & 0x1))
    goto fault;

  if (!(attr & 0x20)) {
    bool found = false;

    for (int i = 0; i < 20; i++) {
      uint16_t sub = uat_get_sub(basicPointer->rawUatValue, i);

      if (uat_get_sub_csid(sub) == csid) {
        attr |= uat_get_sub_attr(sub);

        found = true;
        break;
      }

      if (!found)
        goto fault;
    }
  }

  switch (basicPointer->theType) {
  case Translation::eLoad:
    if (!(attr & 0x1))
      goto fault;
    break;
  case Translation::eStore:
    if (!(attr & 0x2))
      goto fault;
    break;
  case Translation::eFetch:
    if (!(attr & 0x4))
      goto fault;
    break;
  }

  basicPointer->setDone();
  return true;

fault:
  if (basicPointer->isData() && basicPointer->theInstruction)
    basicPointer->theInstruction->forceResync();
  else
    basicPointer->setPagefault();

  basicPointer->setDone();
  return true;
}

bool PageWalk::walk(TranslationTransport &aTranslation) {
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

  if (basicPointer->isUat())
    return walkUat(aTranslation);

  DBG_(VVerb, (<< "Walking " << basicPointer->theVaddr));
  DBG_(VVerb, (<< "Current Translation Level: " << (unsigned int)statefulPointer->currentLookupLevel
               << ", Read Raw TTE Desc. from QEMU : " << std::hex << basicPointer->rawTTEValue
               << std::dec));

  /* Check Valid */
  if (basicPointer->theInstruction)
    DBG_(VVerb, (<< "Walking " << *basicPointer->theInstruction));

  // sv39
  int vld = extractSingleBitAsBool(basicPointer->rawTTEValue, 0);
  int rwx = extractBitsWithBounds (basicPointer->rawTTEValue, 3, 1);

  if (vld == false)
    goto fault;

  if (rwx == 0) {
    if (statefulPointer->currentLookupLevel == 2)
      goto fault;

    statefulPointer->currentLookupLevel += 1;
    return false;

  } else {
    switch (basicPointer->theType) {
    case Translation::eLoad:
      if (!(rwx & 0x1))
        goto fault;
      break;
    case Translation::eStore:
      if (!(rwx & 0x2))
        goto fault;
      break;
    case Translation::eFetch:
      if (!(rwx & 0x4))
        goto fault;
      break;
    }

    switch (statefulPointer->currentLookupLevel) {
    case 0:
      basicPointer->thePaddr = (extractBitsWithRange(basicPointer->rawTTEValue, 53, 28) << 30) |
                               (extractBitsWithRange(basicPointer->theVaddr,    29,  0));
      break;
    case 1:
      basicPointer->thePaddr = (extractBitsWithRange(basicPointer->rawTTEValue, 53, 19) << 21) |
                               (extractBitsWithRange(basicPointer->theVaddr,    20,  0));
      break;
    case 2:
      basicPointer->thePaddr = (extractBitsWithRange(basicPointer->rawTTEValue, 53, 10) << 12) |
                               (extractBitsWithRange(basicPointer->theVaddr,    11,  0));
      break;
    }

    basicPointer->setDone();
    return true;
  }

fault:
  if (basicPointer->isData() && basicPointer->theInstruction)
    basicPointer->theInstruction->forceResync();
  else
    basicPointer->setPagefault();

  basicPointer->setDone();
  return true;
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

    if (basicTranslation->isUat())
      for (int i = 0; i < 8; i++)
        basicTranslation->rawUatValue[i] = (uint64_t)(theCPU->readPhysicalAddress(trace_address + i * 8, 8));
    else
      basicTranslation->rawTTEValue = (uint64_t)theCPU->readPhysicalAddress(trace_address, 8);

    aTranslation->trace_addresses.push(trace_address);
    newTransport.set(TranslationBasicTag, basicTranslation);
    translate(newTransport);
    if (basicTranslation->isDone())
      break;
  }

  return true;
}

bool PageWalk::InitialTranslationSetup(TranslationTransport &aTranslation) {
  // setup stateful API that gets passed along with the tr.
  boost::intrusive_ptr<TranslationState> statefulPointer(aTranslation[TranslationStatefulTag]);
  boost::intrusive_ptr<Translation> basicPointer(aTranslation[TranslationBasicTag]);

  Processor cpu = Processor::getProcessor(theNode);

  if (basicPointer->isUat()) {
    uint64_t suatp = API::qemu_api.get_csr(cpu, CSR_SUATP);
    uint64_t suatc = API::qemu_api.get_csr(cpu, CSR_SUATC);

    statefulPointer->uat      = true;
    statefulPointer->uat_root = extractBitsWithBounds(suatp, 62,  0) << 12;
    statefulPointer->uat_idx  = extractBitsWithBounds(suatc,  5,  0);
    statefulPointer->uat_vsc  = extractBitsWithBounds(suatc, 13,  8);
    statefulPointer->uat_top  = extractBitsWithBounds(suatc, 21, 16);
    statefulPointer->uat_siz  = extractBitsWithBounds(suatc, 29, 24);

  } else {
    // sv39
    statefulPointer->uat      = false;
    statefulPointer->ppn_root = extractBitsWithBounds(API::qemu_api.get_csr(cpu, CSR_SATP), 53, 10) << 12;

    statefulPointer->requiredTableLookups = 2;
    statefulPointer->currentLookupLevel   = 0;
  }

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

void PageWalk::cycle() {
  if (theTranslationTransports.size() > 0) {
    auto i = theTranslationTransports.begin();

    TranslationTransport &item = *i;
    TranslationPtr basicPointer(item[TranslationBasicTag]);
    (*basicPointer)++;
    DBG_(VVerb, (<< "processing translation entry " << basicPointer->theVaddr));

    if ((theTranslationTransports.begin() == i) && basicPointer->isDone()) {
      DBG_(VVerb, (<< "translation is done for " << basicPointer->theVaddr));
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

} // end namespace nMMU
