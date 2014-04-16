#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT VMMapper
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER(PhysicalAddrBits, int, "Number of bits used by physical addresses.", "PhysicalAddrBits", 32)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut )
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_D )
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_I )
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Snoop)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Request)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Prefetch)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, BackSideOut_Snoop)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, BackSideOut_Request)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, BackSideOut_Prefetch)

  // Same components supports either unified or split cache. Aren't I so clever?
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, BackSideIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, BackSideIn_D)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, BackSideIn_I)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT VMMapper
