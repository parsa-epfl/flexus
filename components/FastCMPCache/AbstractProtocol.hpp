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
#ifndef FASTCMPCACHE_PROTOCOL_HPP
#define FASTCMPCACHE_PROTOCOL_HPP

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/FastCMPCache/AbstractFactory.hpp>

#include <components/FastCMPCache/CacheStats.hpp>
#include <components/FastCMPCache/CoherenceStates.hpp>

#include <core/types.hpp>

namespace nFastCMPCache {

using namespace Flexus::SharedTypes;

#define NoRequest NumMemoryMessageTypes

enum SharingState { ZeroSharers = 0, OneSharer = 1, ManySharers = 2 };

struct SharingStatePrinter {
  SharingStatePrinter(const SharingState &s) : state(s) {
  }
  const SharingState &state;
};

inline std::ostream &operator<<(std::ostream &os, const SharingStatePrinter &state) {
  switch (state.state) {
  case ZeroSharers:
    os << "ZeroSharers";
    break;
  case OneSharer:
    os << "OneSharer";
    break;
  case ManySharers:
    os << "ManySharers";
    break;
  }
  return os;
}

typedef Flexus::Stat::StatCounter CacheStats::*StatMemberPtr;

struct PrimaryAction {
  MemoryMessage::MemoryMessageType snoop_type;
  MemoryMessage::MemoryMessageType terminal_response[2];
  MemoryMessage::MemoryMessageType response[2];
  CoherenceState_t next_state[2];
  bool forward_request;
  bool multicast;
  bool allocate;
  bool update;
  StatMemberPtr stat;
  bool poison;

  PrimaryAction(MemoryMessage::MemoryMessageType s, MemoryMessage::MemoryMessageType t1,
                MemoryMessage::MemoryMessageType t2, MemoryMessage::MemoryMessageType r1,
                MemoryMessage::MemoryMessageType r2, CoherenceState_t ns1, CoherenceState_t ns2,
                bool m, bool fwd, bool a, bool u, StatMemberPtr s_ptr, bool p)
      : snoop_type(s), forward_request(fwd), multicast(m), allocate(a), update(u), stat(s_ptr),
        poison(p) {
    terminal_response[0] = t1;
    terminal_response[1] = t2;
    response[0] = r1;
    response[1] = r2;
    next_state[0] = ns1;
    next_state[1] = ns2;
  };

  PrimaryAction() : poison(true){};
};

class AbstractProtocol {
public:
  virtual ~AbstractProtocol() {
  }

  // Given a SharingState and message type, return the appropriate protocol
  // action to take
  virtual const PrimaryAction &getAction(CoherenceState_t cache_state, SharingState dir_state,
                                         MemoryMessage::MemoryMessageType type,
                                         PhysicalMemoryAddress address) = 0;

  virtual MemoryMessage::MemoryMessageType evict(uint64_t tagset, CoherenceState_t state) = 0;
};

#define REGISTER_PROTOCOL_TYPE(type, n)                                                            \
  const std::string type::name = n;                                                                \
  static ConcreteFactory<AbstractProtocol, type> type##_Factory
#define CREATE_PROTOCOL(args) AbstractFactory<AbstractProtocol>::createInstance(args)

}; // namespace nFastCMPCache

#endif // FASTCMPCACHE_PROTOCOL_HPP
