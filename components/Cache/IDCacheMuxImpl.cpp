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

#include <components/Cache/IDCacheMux.hpp>

#define FLEXUS_BEGIN_COMPONENT IDCacheMux
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nIDCacheMux {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

class FLEXUS_COMPONENT(IDCacheMux) {
  FLEXUS_COMPONENT_IMPL(IDCacheMux);

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(IDCacheMux) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

public:
  // Initialization
  void initialize() {
  }

  void finalize() {
  }

  bool isQuiesced() const {
    return true; // Mux is always quiesced
  }

  // Ports
  // From the instruction cache
  bool available(interface::TopInI const &) {
    return FLEXUS_CHANNEL(BottomOut).available();
  }
  void push(interface::TopInI const &, MemoryTransport &aMemTransport) {
    intrusive_ptr<Mux> arb(new Mux('i'));
    aMemTransport.set(MuxTag, arb);
    FLEXUS_CHANNEL(BottomOut) << aMemTransport;
  }

  // From the data cache
  bool available(interface::TopInD const &) {
    return FLEXUS_CHANNEL(BottomOut).available();
  }
  void push(interface::TopInD const &, MemoryTransport &aMemTransport) {
    intrusive_ptr<Mux> arb(new Mux('d'));
    aMemTransport.set(MuxTag, arb);
    FLEXUS_CHANNEL(BottomOut) << aMemTransport;
  }

  bool available(interface::BottomIn const &) {
    return FLEXUS_CHANNEL(TopOutI).available() && FLEXUS_CHANNEL(TopOutD).available();
  }
  void push(interface::BottomIn const &, MemoryTransport &aMemTransport) {
    if (aMemTransport[MuxTag] == 0) {
      // slice doesn't exist in the transport - forward to the data cache
      FLEXUS_CHANNEL(TopOutD) << aMemTransport;
    } else {
      switch (aMemTransport[MuxTag]->source) {
      case 'i':
        FLEXUS_CHANNEL(TopOutI) << aMemTransport;
        break;
      case 'd':
        FLEXUS_CHANNEL(TopOutD) << aMemTransport;
        break;
      default:
        DBG_Assert(false, (<< "Invalid source"));
      }
    }
  }

  bool available(interface::TopIn_SnoopI const &) {
    return FLEXUS_CHANNEL(BottomOut_Snoop).available();
  }
  void push(interface::TopIn_SnoopI const &, MemoryTransport &aMemTransport) {
    intrusive_ptr<Mux> arb(new Mux('i'));
    aMemTransport.set(MuxTag, arb);
    aMemTransport[MemoryMessageTag]->dstream() = false;
    FLEXUS_CHANNEL(BottomOut_Snoop) << aMemTransport;
  }

  // From the data cache
  bool available(interface::TopIn_SnoopD const &) {
    return FLEXUS_CHANNEL(BottomOut_Snoop).available();
  }
  void push(interface::TopIn_SnoopD const &, MemoryTransport &aMemTransport) {
    intrusive_ptr<Mux> arb(new Mux('d'));
    aMemTransport.set(MuxTag, arb);
    aMemTransport[MemoryMessageTag]->dstream() = true;
    FLEXUS_CHANNEL(BottomOut_Snoop) << aMemTransport;
  }
};

} // End Namespace nIDCacheMux

FLEXUS_COMPONENT_INSTANTIATOR(IDCacheMux, nIDCacheMux);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT IDCacheMux
