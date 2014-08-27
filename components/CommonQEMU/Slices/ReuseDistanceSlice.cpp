#include <components/CommonQEMU/Slices/ReuseDistanceSlice.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator << (std::ostream & s, ReuseDistanceSlice & aReuseDistanceSlice) {
  char const * reuseDistSlice_types[] = {
    "Process Memory Message",
    "Get Mean Reuse Distance [Data]"
  };

  return s << "ReuseDistanceSlice[" << reuseDistSlice_types[aReuseDistanceSlice.type()]
         << "]: Addr:0x" << std::hex << aReuseDistanceSlice.address()
         << " Msg: " << aReuseDistanceSlice.memMsg();
}

} //namespace SharedTypes
} //namespace Flexus

