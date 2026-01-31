
#include "../SemanticActions.hpp"

#include <components/uArch/uArchInterfaces.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

namespace nDecoder {

using namespace nuArch;

struct register_value_extractor : boost::static_visitor<register_value>
{
    register_value operator()(uint64_t v) const { return v; }

    register_value operator()(bits v) const { return v; }

    register_value operator()(int64_t v) const { return v; }

    template<class T>
    register_value operator()(T aT) const
    {
        DBG_Assert(false,
                   (<< "Attempting to store a non-register value operand "
                       "into a register"));
        return uint64_t(0ULL);
    }
};

} // namespace nDecoder
