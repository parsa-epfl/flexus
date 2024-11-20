#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/RegionScoutMessage.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT FastCMPCache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPWidth", 16 )

  PARAMETER( Size, int, "Cache size in bytes", "size", 65536 )
  PARAMETER( Associativity, int, "Set associativity", "assoc", 2 )
  PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
  PARAMETER( CleanEvictions, bool, "Issue clean evictions", "clean_evict", false )
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )
  PARAMETER( TraceTracker, bool, "Turn trace tracker on/off", "trace_tracker_on", false )

  PARAMETER( ReplPolicy, std::string, "Cache replacement policy (LRU | SetLRU | RegionLRU)", "repl", "LRU" )

  PARAMETER( RegionSize, int, "Region size in bytes", "rsize", 1024 )
  PARAMETER( RTAssoc, int, "RegionTracker Associativity", "rt_assoc", 16 )
  PARAMETER( RTSize, int, "RegionTracker size (number of regions tracked)", "rt_size", 8192 )
  PARAMETER( ERBSize, int, "Evicted Region Buffer size", "erb_size", 8 )

  PARAMETER( StdArray, bool, "Use Standard Tag Array (true) or RegionTracker (false) default is true", "std_array", true )

  PARAMETER( DirectoryType, std::string, "Directory Type", "directory_type", "Infinite")
  PARAMETER( Protocol, std::string, "Protocol Type", "protocol", "SingleCMP")
  PARAMETER( AlwaysMulticast, bool, "Perform multicast instead of serial snooping", "always_multicast", false)
  PARAMETER( SeparateID, bool, "Track Instruction and Data caches separately", "seperate_id", false)

  PARAMETER( CoherenceUnit, uint64_t, "Coherence Unit", "coherence_unit", 64)

);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestIn )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FetchRequestIn )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutD )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutI )

  DYNAMIC_PORT_ARRAY( PushInput, RegionScoutMessage, RegionNotify )
  DYNAMIC_PORT_ARRAY( PushOutput, RegionScoutMessage, RegionNotifyOut )
  DYNAMIC_PORT_ARRAY( PushOutput, RegionScoutMessage, RegionProbe )

  PORT( PushInput, MemoryMessage, SMMURequestIn )

  // From Directory OUT to Memory Controller
  PORT( PushInput, MemoryMessage, SnoopIn ) // From MemController IN to topology
  PORT( PushOutput, MemoryMessage, RequestOut ) // From topology OUT to memcontroller

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastCMPCache
// clang-format on
