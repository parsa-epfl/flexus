#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT NetShim
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/NetworkTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NetworkTopologyFile, std::string, "Network topology file", "topology-file", "" )
  FLEXUS_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 2)
  FLEXUS_PARAMETER( VChannels, int, "Number of virtual channels", "virtual-channels", 3)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, NetworkTransport, ToNode )
  DYNAMIC_PORT_ARRAY( PushInput, NetworkTransport, FromNode )
  DRIVE( NetworkDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NetShim
