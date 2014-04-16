#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPNetworkControl
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NumCores, uint32_t, "Number of cores (0 = sys width)", "numCores", 0 )
  FLEXUS_PARAMETER( NumL2Tiles, uint32_t, "Number of L2 tiles in the CMP network", "numL2Tiles", 1)
  FLEXUS_PARAMETER( NumMemControllers, uint32_t, "Number of memory controllers in the CMP", "numMemControllers", 1)
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )

  FLEXUS_PARAMETER( PageSize, int, "Page size in bytes", "page_size", 8192 )
  FLEXUS_PARAMETER( CacheLineSize, int, "Cahe line size in bytes", "cache_line_size", 64 )
  FLEXUS_PARAMETER( L2InterleavingGranularity, uint32_t, "Granularity in bytes at which the L2 tiles are interleaved", "l2InterleavingGranularity", 64)

  FLEXUS_PARAMETER( Floorplan, std::string, "Floorplan; determines network connectivity [woven-torus, tiled-torus, mesh, 1-1, 4-4-line, 4-4-block, cores-top-mesh]", "floorplan", "woven-torus")
  FLEXUS_PARAMETER( Placement, std::string, "Placement policy [shared, private, R-NUCA]", "placement_policy", "shared" )
  FLEXUS_PARAMETER( NumCoresPerTorusRow, uint32_t, "Number of cores per torus row", "torus_row_size", 4 )

  FLEXUS_PARAMETER( SizeOfInstrCluster, uint32_t, "Number of cores in instruction interleaving cluster", "instr_cluster_size", 4 )
  FLEXUS_PARAMETER( SizeOfPrivCluster, uint32_t, "Number of cores in private data interleaving cluster", "priv_cluster_size", 4 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromNetwork)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToCore)

  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromCore)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNetwork)

  /* CMU-ONLY-BLOCK-BEGIN */
  //nikos: R-NUCA purge
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, PurgeAddrOut)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, PurgeAckIn)
  /* CMU-ONLY-BLOCK-END */

  DRIVE(CMPNetworkControlDrive)

);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMPNetworkControl
