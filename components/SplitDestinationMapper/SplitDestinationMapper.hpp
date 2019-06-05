//#define WEIGHTED_ROUTING		//WEAO
#define SPLIT_RMC_MESSAGES_DEBUG Verb


#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT SplitDestinationMapper
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "Cores", 64)

  PARAMETER( MemControllers, int, "Number of memory controllers", "MemControllers", 8)

  PARAMETER( Directories, int, "Number of directories", "Directories", 64)
  PARAMETER( Banks, int, "Number of banks", "Banks", 64)

  PARAMETER( DirInterleaving, int, "Interleaving between directories (in bytes)", "DirInterleaving", 64)
  PARAMETER( MemInterleaving, int, "Interleaving between memory controllers (in bytes)", "MemInterleaving", 4096)

  PARAMETER( DirXORShift, int, "XOR high order bits after shifting this many bits when calculating directory index", "DirXORShift", -1)
  PARAMETER( MemXORShift, int, "XOR high order bits after shifting this many bits when calculating memory index", "MemXORShift", -1)

  PARAMETER( DirLocation, std::string, "Directory location (Distributed|AtMemory)", "DirLocation", "Distributed")
  PARAMETER( MemLocation, std::string, "Memory controller locations (ex: '8,15,24,31,32,39,48,55')", "MemLocation", "8,15,24,31,32,39,48,55")

  PARAMETER( MemReplyToDir, bool, "Send memory replies to the directory (instead of original requester)", "MemReplyToDir", false)
  PARAMETER( MemAcksNeedData, bool, "When memory replies directly to requester, require data with final ack", "MemAcksNeedData", true)
  PARAMETER( TwoPhaseWB, bool, "2 Phase Write-Back sends NAcks to requester, not directory", "TwoPhaseWB", false)
  PARAMETER( LocalDir, bool, "Treat directory as always being local to the requester", "LocalDir", false)
  PARAMETER( EdgeInterfaces, int, "The number of interfaces (MCs, RMCs) per edge of the chip", "edge_interfaces", 4 )
  
  PARAMETER(MachineCount, uint32_t, "Number of machines", "MachineCount", 1)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, DirReplyIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, DirSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, DirRequestIn)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DirReplyOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DirSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DirRequestOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, ICacheRequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, ICacheSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, ICacheReplyIn)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ICacheSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ICacheReplyOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, CacheRequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, CacheSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, CacheReplyIn)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, CacheSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, CacheReplyOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, MemoryIn)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, MemoryOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FromNIC0)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNIC0)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FromNIC1)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNIC1)
  
  //ALEX
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FromNIC2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNIC2)  
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, RMCCacheRequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, RMCCacheSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, RMCCacheReplyIn)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, RMCCacheSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, RMCCacheReplyOut)
  
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SplitDestinationMapper
