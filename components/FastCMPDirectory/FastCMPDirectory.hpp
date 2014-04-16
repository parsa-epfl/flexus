#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/RegionScoutMessage.hpp>

#define FLEXUS_BEGIN_COMPONENT FastCMPDirectory
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPWidth", 16 )

  PARAMETER( DirectoryType, std::string, "Directory Type", "directory_type", "Infinite:Loc=Distributed:Interleaving=64")
  PARAMETER( TopologyType, std::string, "Topology Type", "topology_type", "Mesh:MemLoc=Edges:NumMem=4:MemInterleaving=64")
  PARAMETER( Protocol, std::string, "Protocol Type", "protocol", "SingleCMP")
  PARAMETER( NoLatency, bool, "Don't perform latency calculations", "no_latency", true)
  PARAMETER( AlwaysMulticast, bool, "Perform multicast instead of serial snooping", "always_multicast", false)

  PARAMETER( CoherenceUnit, uint64_t, "Coherence Unit", "coherence_unit", 64)

);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestIn )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FetchRequestIn )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOut )
  DYNAMIC_PORT_ARRAY( PushInput, RegionScoutMessage, RegionNotify )
  DYNAMIC_PORT_ARRAY( PushOutput, RegionScoutMessage, RegionProbe )

  // From Directory OUT to Memory Controller
  PORT( PushInput, MemoryMessage, SnoopIn ) // From MemController IN to topology
  PORT( PushOutput, MemoryMessage, RequestOut ) // From topology OUT to memcontroller

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastCMPDirectory
