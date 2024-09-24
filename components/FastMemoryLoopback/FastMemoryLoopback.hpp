#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT FastMemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
);

COMPONENT_INTERFACE(
  PORT( PushOutput, MemoryMessage, ToCache)
  PORT( PushInput, MemoryMessage, FromCache)
  PORT( PushInput, MemoryMessage, DMA )
  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastMemoryLoopback
// clang-format on
