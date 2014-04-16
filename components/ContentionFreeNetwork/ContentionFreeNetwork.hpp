#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT ContentionFreeNetwork
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 2)
  FLEXUS_PARAMETER( NumCores, int, "Number of Cores", "cores", 2)
  FLEXUS_PARAMETER( VChannels, int, "Number of virtual channels", "virtual-channels", 3)
  FLEXUS_PARAMETER( RouterDelay, int, "Number of cycles of router delay (add 1 cycle per flit for transmission)", "RouterDelay", 3)
  FLEXUS_PARAMETER( ChannelBandwidth, int, "Width of channel in bytes (default = 8)", "ChannelBandwidth", 8)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, ToNode )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNode )
  DRIVE( NetworkDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT ContentionFreeNetwork
