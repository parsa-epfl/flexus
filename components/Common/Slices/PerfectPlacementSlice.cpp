#include <components/Common/Slices/PerfectPlacementSlice.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator << (std::ostream & s, PerfectPlacementSlice & aPerfectPlacementSlice) {
  char const * perfPlcSlice_types[] = {
    "Process Msg",
    "Make Block Writable"
  };

  return s << "PerfectPlacementSlice[" << perfPlcSlice_types[aPerfectPlacementSlice.type()]
         << "]: Addr:0x" << std::hex << aPerfectPlacementSlice.address()
         << " Msg: " << aPerfectPlacementSlice.memMsg();
}

} //namespace SharedTypes
} //namespace Flexus

