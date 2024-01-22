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

#ifndef FLEXUS_PAGEWALK_HPP_INCLUDED
#define FLEXUS_PAGEWALK_HPP_INCLUDED

#define CSR_SATP  0x080
#define CSR_SUATP 0x090
#define CSR_SUATC 0x091
#define CSR_SUATL 0x092
#define CSR_SUATH 0x093
#define CSR_UCID  0x081
#define CSR_UCSP  0x082

#include <core/types.hpp>

#include <components/CommonQEMU/Transports/TranslationTransport.hpp>
#include <components/uArch/CoreImpl.hpp>

namespace nMMU {

using namespace Flexus::SharedTypes;

typedef uint64_t PTEDescriptor;

uint64_t uatIdx (uint64_t vaddr, TranslationState *s);
uint64_t uatBase(uint64_t vaddr, TranslationState *s);
uint64_t uatOffs(uint64_t vaddr, TranslationState *s);

class PageWalk {
  std::list<TranslationTransport> theTranslationTransports;
  std::queue<boost::intrusive_ptr<Translation>> theDoneTranslations;
  std::queue<boost::intrusive_ptr<Translation>> theMemoryTranslations;
  PhysicalMemoryAddress trace_address;
  bool TheInitialized;

public:
  PageWalk(uint32_t aNode) : TheInitialized(false), theNode(aNode) {
  }
  ~PageWalk() {
  }
  void translate(TranslationTransport &aTransport);
  void preTranslate(TranslationTransport &aTransport);
  void cycle();
  bool push_back(boost::intrusive_ptr<Translation> aTranslation);
  bool push_back_trace(boost::intrusive_ptr<Translation> aTranslation,
                       Flexus::Qemu::Processor theCPU);
  TranslationPtr popMemoryRequest();
  bool hasMemoryRequest();
  void annulAll();

private:
  uint64_t uatBase(uint64_t vaddr, TranslationState *s);
  uint64_t uatAddr(uint64_t vaddr, TranslationState *s);

  void preWalk(TranslationTransport &aTranslation);
  void preWalkUat(TranslationTransport &aTranslation);
  bool walk(TranslationTransport &aTranslation);
  bool walkUat(TranslationTransport &aTranslation);
  bool InitialTranslationSetup(TranslationTransport &aTranslation);
  void pushMemoryRequest(TranslationPtr aTranslation);

  PTEDescriptor getNextTTDescriptor(TranslationTransport &aTranslation);

  uint32_t theNode;
};

} // end namespace nMMU

#endif // FLEXUS_PAGEWALK_HPP_INCLUDED
