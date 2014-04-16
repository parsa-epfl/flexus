#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>
#include "../SemanticActions.hpp"

namespace nv9Decoder {

using namespace nuArch;

struct register_value_extractor : boost::static_visitor<register_value> {
  register_value operator()(uint64_t v) const {
    return v;
  }

  register_value operator()(std::bitset<8> v) const {
    return v;
  }

  template <class T>
  register_value operator()(T aT) const {
    DBG_Assert( false, ( << "Attempting to store a non-register value operand into a register"));
    return 0ULL;
  }
};

} //nv9Decoder
