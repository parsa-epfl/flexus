#include <iostream>
#include <list>

#include <components/CommonQEMU/Slices/PrefetchMessage.hpp>

#include <core/debug/debug.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator << (std::ostream & s, PrefetchMessage const & aMsg) {
  char const * prefetch_types[] = {
    "Prefetch Request",
    "Discard Request",
    "Watch Request",
    "Prefetch Complete",
    "Prefetch Redundant",
    "Line Hit",
    "Line Hit - Partial",
    "Line Replaced",
    "Line Removed",
    "Watch Present",
    "Watch Redundant",
    "Watch Requested",
    "Watch Removed",
    "Watch Replaced"
  };
  DBG_Assert(aMsg.type() < static_cast<int>(sizeof(prefetch_types)) );

  s << "PrefetchMessage[" << prefetch_types[aMsg.type()]
    << "]: ";
  if (aMsg.streamID() > 0) {
    s << " Stream[" <<  aMsg.streamID()  << "] ";
  }
  s  << " Addr:0x" << std::hex << aMsg.address() << std::dec;
  return s;
}

} //End SharedTypes
} //End Flexus

