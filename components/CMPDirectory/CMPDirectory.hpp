#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPDirectory
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "cores", 1 )
  PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
  PARAMETER( Banks, int, "number of directory banks", "banks", 1 )
  PARAMETER( Interleaving, int, "interleaving between directory banks (64 bytes)", "interleaving", 64 )

  PARAMETER( DirLatency, int, "Total latency of directory lookup", "dir_lat", 1 )
  PARAMETER( DirIssueLatency, int, "Minimum delay between issues to the directory", "dir_issue_lat", 0 )

  PARAMETER( QueueSize, int, "Size of input and output queues", "queue_size", 8 )
  PARAMETER( MAFSize, int, "Number of MAF entries", "maf_size", 32 )
  PARAMETER( EvictBufferSize, int, "Number of Evict Buffer entries", "eb_size", 40 )

  PARAMETER( DirectoryPolicy, std::string, "Type of directory (InclusiveMOESI)", "dir_policy", "InclusiveMOESI" )
  PARAMETER( DirectoryType, std::string, "Type of directory (infinite, std, region, etc.)", "dir_type", "infinite" )
  PARAMETER( DirectoryConfig, std::string, "Configuration of directory array (sets=1024:assoc=16)", "dir_config", "sets=1024:assoc=16" )
);

COMPONENT_INTERFACE(
  PORT(PushOutput, MemoryTransport, Request_Out)
  PORT(PushOutput, MemoryTransport, Snoop_Out)
  PORT(PushOutput, MemoryTransport, Reply_Out)
  PORT(PushInput, MemoryTransport, Request_In)
  PORT(PushInput, MemoryTransport, Snoop_In)
  PORT(PushInput, MemoryTransport, Reply_In)
  DRIVE(DirectoryDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMPDirectory
