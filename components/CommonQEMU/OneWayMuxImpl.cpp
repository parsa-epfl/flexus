
#include <components/CommonQEMU/OneWayMux.hpp>

#define FLEXUS_BEGIN_COMPONENT OneWayMux
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nOneWayMux {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

class FLEXUS_COMPONENT(OneWayMux)
{
    FLEXUS_COMPONENT_IMPL(OneWayMux);

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(OneWayMux)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

  public:
    // Initialization
    void initialize() {}

    // Added by PLotfi
    void finalize() {}
    // end PLotfi

    bool isQuiesced() const
    {
        return true; // Mux is always quiesced
    }

    // Ports
    // From the instruction cache
    bool available(interface::InI const&, index_t anIndex) { return true; }

    void push(interface::InI const&, index_t anIndex, MemoryMessage& aMessage) { FLEXUS_CHANNEL(Out) << aMessage; }

    // From the data cache
    bool available(interface::InD const&, index_t anIndex) { return true; }

    void push(interface::InD const&, index_t anIndex, MemoryMessage& aMessage) { FLEXUS_CHANNEL(Out) << aMessage; }
};

} // End Namespace nOneWayMux

FLEXUS_COMPONENT_INSTANTIATOR(OneWayMux, nOneWayMux);

FLEXUS_PORT_ARRAY_WIDTH(OneWayMux, InI)
{
    return 1;
}
FLEXUS_PORT_ARRAY_WIDTH(OneWayMux, InD)
{
    return 1;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT OneWayMux
