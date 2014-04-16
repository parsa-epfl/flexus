#ifndef FASTCMPCACHE_PROTOCOL_HPP
#define FASTCMPCACHE_PROTOCOL_HPP

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/FastCMPCache/AbstractFactory.hpp>

#include <components/FastCMPCache/CoherenceStates.hpp>
#include <components/FastCMPCache/CacheStats.hpp>

#include <ext/hash_map>

#include <core/types.hpp>

namespace nFastCMPCache {

using namespace Flexus::SharedTypes;

#define NoRequest NumMemoryMessageTypes

enum SharingState {
  ZeroSharers = 0,
  OneSharer = 1,
  ManySharers = 2
};

struct SharingStatePrinter {
  SharingStatePrinter(const SharingState & s) : state(s) {}
  const SharingState & state;
};

inline std::ostream & operator<<(std::ostream & os, const SharingStatePrinter & state) {
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

typedef Flexus::Stat::StatCounter CacheStats::* StatMemberPtr;

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

  PrimaryAction(MemoryMessage::MemoryMessageType s, MemoryMessage::MemoryMessageType t1, MemoryMessage::MemoryMessageType t2, MemoryMessage::MemoryMessageType r1, MemoryMessage::MemoryMessageType r2, CoherenceState_t ns1, CoherenceState_t ns2, bool m, bool fwd, bool a, bool u, StatMemberPtr s_ptr, bool p) :
    snoop_type(s), forward_request(fwd),
    multicast(m), allocate(a), update(u), stat(s_ptr), poison(p) {
    terminal_response[0] = t1;
    terminal_response[1] = t2;
    response[0] = r1;
    response[1] = r2;
    next_state[0] = ns1;
    next_state[1] = ns2;
  };

  PrimaryAction() : poison(true) {};
};

class AbstractProtocol {
public:

  virtual ~AbstractProtocol() {}

  // Given a SharingState and message type, return the appropriate protocol action to take
  virtual const PrimaryAction & getAction(CoherenceState_t cache_state, SharingState dir_state, MemoryMessage::MemoryMessageType type, PhysicalMemoryAddress address) = 0;

  virtual MemoryMessage::MemoryMessageType evict(uint64_t tagset, CoherenceState_t state) = 0;
};

#define REGISTER_PROTOCOL_TYPE(type,n) const std::string type::name = n; static ConcreteFactory<AbstractProtocol,type> type ## _Factory
#define CREATE_PROTOCOL(args)   AbstractFactory<AbstractProtocol>::createInstance(args)

}; // namespace

#endif // FASTCMPCACHE_PROTOCOL_HPP

