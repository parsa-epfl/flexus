#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/RegionScoutMessage.hpp>

#define FLEXUS_BEGIN_COMPONENT FastCache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( MTWidth, int, "Number of threads sharing this cache", "mt_width", 1 )
  PARAMETER( Size, int, "Cache size in bytes", "size", 65536 )
  PARAMETER( Associativity, int, "Set associativity", "assoc", 2 )
  PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
  PARAMETER( CleanEvictions, bool, "Issue clean evictions", "clean_evict", false )
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )
  PARAMETER( NotifyReads, bool, "Notify on reads (does not notify on fast-hit)", "notify_reads", false )
  PARAMETER( NotifyWrites, bool, "Notify on writes", "notify_writes", false )
  PARAMETER( TraceTracker, bool, "Turn trace tracker on/off", "trace_tracker_on", false )

  PARAMETER( RegionSize, int, "Region size in bytes", "rsize", 1024 )
  PARAMETER( RTAssoc, int, "RegionTracker Associativity", "rt_assoc", 16 )
  PARAMETER( RTSize, int, "RegionTracker size (number of regions tracked)", "rt_size", 8192 )
  PARAMETER( RTReplPolicy, std::string, "RegionTracker replacement policy (SetLRU | RegionLRU)", "rt_repl", "SetLRU" )
  PARAMETER( ERBSize, int, "Evicted Region Buffer size", "erb_size", 8 )

  PARAMETER( StdArray, bool, "Use Standard Tag Array instead of RegionTracker", "std_array", false )

  PARAMETER( BlockScout, bool, "Use precise block sharing info", "block_scout", false )

  PARAMETER( SkewBlockSet, bool, "skew block set indices based on rt way", "skew_block_set", false )

  PARAMETER( Protocol, std::string, "Name of the coherence protocol (InclusiveMOESI)", "protocol", "InclusiveMOESI" )

  PARAMETER( UsingTraces, bool, "References are coming from traces (allow certain inconsistancies", "using_traces", true )

  PARAMETER( TextFlexpoints, bool, "Store flexpoints as text files (compatible with old FastCache component)", "text_flexpoints", false )
  PARAMETER( GZipFlexpoints, bool, "Compress flexpoints with gzip", "gzip_flexpoints", true )

  PARAMETER( DowngradeLRU, bool, "Move block to LRU position when a Downgrade is recieved for a block in Modified or Exclusive state", "downgrade_lru", false )
  PARAMETER( SnoopLRU, bool, "Move block to LRU position when a Snoop (ReturnReq) is recieved for a block in Modified or Exclusive state", "snoop_lru", false )

);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestIn )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FetchRequestIn )
  PORT( PushInput, MemoryMessage, SnoopIn )
  PORT( PushOutput, MemoryMessage, RequestOut )

  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutI )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutD )

  PORT( PushOutput, MemoryMessage, Reads )
  PORT( PushOutput, MemoryMessage, Writes )

  PORT( PushInput, RegionScoutMessage, RegionProbe )
  PORT( PushOutput, RegionScoutMessage, RegionNotify )

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastCache
