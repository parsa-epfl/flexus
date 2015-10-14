#ifndef FASTCMPDIRECTORY_PROTOCOL_HPP
#define FASTCMPDIRECTORY_PROTOCOL_HPP

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/FastCMPDirectory/AbstractFactory.hpp>
#include <unordered_map>

#include <core/types.hpp>

namespace nFastCMPDirectory {

using namespace Flexus::SharedTypes;

#define NoRequest NumMemoryMessageTypes

enum SharingState {
  ZeroSharers,
  OneSharer,
  ExclSharer,
  ManySharers,
  Migratory  // Used for migratory sharing optimization
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
    case ExclSharer:
      os << "ExclSharer";
      break;
    case ManySharers:
      os << "ManySharers";
      break;
    case Migratory:
      os << "Migratory";
      break;
  }
  return os;
}

struct PrimaryAction {
  typedef Flexus::SharedTypes::MemoryMessage::MemoryMessageType MMType;

  MMType snoop_type;
  MMType terminal_response[2];
  MMType response[2];
  bool forward_request;
  bool multicast;
  bool reply_from_snoop;
  bool poison;

  PrimaryAction(MMType s, MMType t1, MMType t2, MMType r1, MMType r2, bool fwd, bool m, bool rfs, bool p) :
    snoop_type(s), forward_request(fwd),
    multicast(m), reply_from_snoop(rfs), poison(p) {
    terminal_response[0] = t1;
    terminal_response[1] = t2;
    response[0] = r1;
    response[1] = r2;
  };

  PrimaryAction() : poison(true) {};
};

class AbstractProtocol {
public:
  typedef Flexus::SharedTypes::MemoryMessage::MemoryMessageType MMType;

  virtual ~AbstractProtocol() {}

  // Given a SharingState and message type, return the appropriate protocol action to take
  virtual const PrimaryAction & getAction(SharingState state, MMType type, PhysicalMemoryAddress address) = 0;
};

#define REGISTER_PROTOCOL_TYPE(type,n) const std::string type::name = n; static ConcreteFactory<AbstractProtocol,type> type ## _Factory
#define CREATE_PROTOCOL(args)   AbstractFactory<AbstractProtocol>::createInstance(args)

}; // namespace

#endif // FASTCMPDIRECTORY_PROTOCOL_HPP

