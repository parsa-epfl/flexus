#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT OneWayMux
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( InIWidth, int, "Number of InI input ports", "i_width", 1 )
  PARAMETER( InDWidth, int, "Number of InD input ports", "d_width", 1 )
);

COMPONENT_INTERFACE(
  PORT( PushOutput, MemoryMessage, Out )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, InI )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, InD )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT OneWayMux
// clang-format on
