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

#include "MMUImpl.hpp"
#include "PageWalk.hpp"

#define DBG_DeclareCategories MMU
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()

namespace nMMU {

using namespace Flexus::Qemu;
using namespace Flexus::SharedTypes;

void PageWalk::cycle() {
  for (auto &tr: trans) {
    DBG_(VVerb, (<< "walk " << tr->theVaddr
                 << ":"     << tr->theID
                 << ":"     << tr->isReady()
                 << ":"     << tr->isWaiting()
                 << ":"     << tr->theCurrentTranslationLevel));

    if (tr->isDone())
      continue;

    tr->incr();

    if (tr->isReady()) {
      if (tr->isWaiting())
        walk(tr);

      else {
        preWalk(tr);

        if (tr->isDone())
          continue;

        tr->toggleReady();
      }

      tr->toggleWaiting();
    }
  }
}

void PageWalk::annul() {
  while (trans.size()) {
    auto &tr = trans.front();

    DBG_(VVerb, (<< "walk " << tr->theVaddr
                 << ":"     << tr->theID
                 << ": annulled"));

    trans.pop_back();
  }
}

void PageWalk::preWalk(TranslationPtr &tr) {
  switch (tr->theCurrentTranslationLevel) {
  case 0:
    tr->theAddr =  mmu->ppnRoot |
                  (extractBitsWithBounds(tr->theVaddr,    38, 30) <<  3);
    break;
  case 1:
    tr->theAddr = (extractBitsWithBounds(tr->rawTTEValue, 53, 10) << 12) |
                  (extractBitsWithBounds(tr->theVaddr,    29, 21) <<  3);
    break;
  case 2:
    tr->theAddr = (extractBitsWithBounds(tr->rawTTEValue, 53, 10) << 12) |
                  (extractBitsWithBounds(tr->theVaddr,    20, 12) <<  3);
    break;
  }

  if (!tr->inTraceMode)
    pushWalk(tr);
}

bool PageWalk::walk(TranslationPtr &tr) {
  auto attr = extractBitsWithBounds(tr->rawTTEValue, 7, 0);

  if (!(attr & 0x1))
    return mmu->fault(tr);

  if (!(attr & 0xe)) {
    if (tr->theCurrentTranslationLevel == 3)
      return mmu->fault(tr);

    tr->theCurrentTranslationLevel++;
    return false;
  }

  tr->theAttr = attr;

  switch (tr->theCurrentTranslationLevel) {
  case 0:
    tr->thePaddr = (extractBitsWithBounds(tr->rawTTEValue, 53, 28) << 30) |
                   (extractBitsWithBounds(tr->theVaddr,    29,  0));
    break;
  case 1:
    tr->thePaddr = (extractBitsWithBounds(tr->rawTTEValue, 53, 19) << 21) |
                   (extractBitsWithBounds(tr->theVaddr,    20,  0));
    break;
  case 2:
    tr->thePaddr = (extractBitsWithBounds(tr->rawTTEValue, 53, 10) << 12) |
                   (extractBitsWithBounds(tr->theVaddr,    11,  0));
    break;
  }

  if (mmu->check(tr))
    return mmu->fault(tr, false);

  DBG_(VVerb, (<< "walk " << tr->theVaddr
               << ":"     << tr->theID
               << ": sv39"
               << ":"     << tr->thePaddr << std::hex
               << ":"     << tr->theAttr));

  tr->setDone();
  return true;
}

void PageWalk::push(TranslationPtr &tr) {
  tr->theCurrentTranslationLevel = 0;

  trans.push_back(tr);
}

void PageWalk::pop() {
  trans.pop_front();
}

size_t PageWalk::size() {
  return trans.size();
}

TranslationPtr &PageWalk::front() {
  return trans.front();
}

bool PageWalk::pushTrace(TranslationPtr &tr) {
  tr->theCurrentTranslationLevel = 0;

  while (true) {
    preWalk(tr);

    if (tr->isDone())
      break;

    tr->trace_addresses.push(PhysicalMemoryAddress(tr->theAddr));

    switch (tr->theLine) {
      case 0:
        if (API::qemu_api.get_mem((uint8_t *)(&tr->rawTTEValue), tr->theAddr, 8))
          return mmu->fault(tr);
        break;
    }

    if (walk(tr))
      break;
  }

  return false;
}

void PageWalk::pushWalk(TranslationPtr &tr) {
  walks.push(tr);
}

void PageWalk::popWalk() {
  assert(walks.size());
  walks.pop();
}

size_t PageWalk::walkSize() {
  return walks.size();
}

TranslationPtr &PageWalk::walkFront() {
  return walks.front();
}

void PageWalk::base(TranslationPtr &tr) {
  tr->theBase = tr->theVaddr & ~mask(12);
  tr->theSet  = tr->theVaddr >> 12;
}

} // end namespace nMMU
