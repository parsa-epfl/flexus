#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryNetwork
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NetworkTopologyFile, std::string, "Network topology file", "topology-file", "" )
  FLEXUS_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 2)
  FLEXUS_PARAMETER( VChannels, int, "Number of virtual channels", "virtual-channels", 3)
  FLEXUS_PARAMETER( DirYXRouting, bool, "Different Routing or Directory-sourced traffic", "directory_YX_routing", false)	//ALEX
  FLEXUS_PARAMETER( randomDirection, bool, "Pick X-Y or Y-X routing at random", "random_direction", false)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, ToNode )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNode )
  DRIVE( NetworkDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MemoryNetwork
