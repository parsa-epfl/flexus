#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryNetwork
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NetworkTopologyFile, std::string, "Network topology file", "topology-file", "" )
  FLEXUS_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 2)
  FLEXUS_PARAMETER( VChannels, int, "Number of virtual channels", "virtual-channels", 3)
  FLEXUS_PARAMETER( Topology, std::string, "Topology", "topology", "mesh" )
  FLEXUS_PARAMETER( RoutingFunc, std::string, "Routing function", "routing-func", "dim_order" )
  FLEXUS_PARAMETER( NetworkRadix, int, "Network radix", "network-radix", 4 )
  FLEXUS_PARAMETER( NetworkDimension, int, "Network dimension", "network-dimension", 2 )
  FLEXUS_PARAMETER( Concentration, int, "Concentration", "concentration", 1 )
  FLEXUS_PARAMETER( NumRoutersX, int, "Number of routers in X", "routers-x", 4 )
  FLEXUS_PARAMETER( NumRoutersY, int, "Number of routers in Y", "routers-y", 4 )
  FLEXUS_PARAMETER( NumNodesPerRouterX, int, "Number of nodes per router in X only of concentration > 1", "nodes-per-router-x", 1 )
  FLEXUS_PARAMETER( NumNodesPerRouterY, int, "Number of nodes per router in Y only of concentration > 1", "nodes-per-router-y", 1 )
  FLEXUS_PARAMETER( NumInPorts, int, "Number of input ports", "in-ports", 5 )
  FLEXUS_PARAMETER( NumOutPorts, int, "Number of output ports", "out-ports", 5 )
  FLEXUS_PARAMETER( Speculation, int, "Speculative routing", "speculation", 0 )
  FLEXUS_PARAMETER( VCBuffSize, int, "Virtual channel buffer size", "vc-buff-size", 4 )
  FLEXUS_PARAMETER( RoutingDelay, int, "Routing delay", "routing-delay", 1 )
  FLEXUS_PARAMETER( VCAllocDelay, int, "VC allocation delay", "vc-alloc-delay", 1 )
  FLEXUS_PARAMETER( SWAllocDelay, int, "Switch allocation delay", "sw-alloc-delay", 1 )
  FLEXUS_PARAMETER( STPrepDelay, int, "Switch transportation: preparation delay", "st-prep-delay", 1 )
  FLEXUS_PARAMETER( STFinalDelay, int, "Switch tranportation: final delay", "st-final-delay", 1 )
  FLEXUS_PARAMETER( ChannelWidth, int, "Channel width", "cwidth", 128 )
  FLEXUS_PARAMETER( DataPackLength, int, "Data packet length", "data-length", 4 )
  FLEXUS_PARAMETER( CtrlPackLength, int, "Control packet length", "ctrl-length", 1 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, ToNode )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNode )
  DRIVE( NetworkDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MemoryNetwork
