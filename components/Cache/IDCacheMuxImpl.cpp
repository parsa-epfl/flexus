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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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
