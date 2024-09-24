
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT Cache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "cores", 1 )
  // PARAMETER( Size, int, "Cache size in bytes", "size", 16384 )
  // PARAMETER( Associativity, int, "Set associativity", "assoc", 2 )
  PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
  PARAMETER( Ports, uint32_t, "Number of ports on data and tag arrays", "ports", 1 )
  PARAMETER( Banks, uint32_t, "number of banks on the data and tag arrays", "banks", 1 )
  PARAMETER( TagLatency, uint32_t, "Total latency of tag pipeline", "tag_lat", 1 )
  PARAMETER( TagIssueLatency, uint32_t, "Minimum delay between issues to tag pipeline", "dup_tag_issue_lat", 0 )
  PARAMETER( DataLatency, uint32_t, "Total latency of data pipeline", "data_lat", 1 )
  PARAMETER( DataIssueLatency, uint32_t, "Minimum delay between issues to data pipeline", "data_issue_lat", 0 )
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )
  PARAMETER( QueueSizes, uint32_t, "Size of input and output queues", "queue_size", 8 )
  PARAMETER( PreQueueSizes, uint32_t, "Size of input arbitration queues", "pre_queue_size", 4 )
  PARAMETER( MAFSize, uint32_t, "Number of MAF entries", "maf", 32 )
  PARAMETER( MAFTargetsPerRequest, uint32_t, "Number of MAF targets per request", "maf_targets", 8)
  PARAMETER( EvictBufferSize, uint32_t, "Number of Evict Buffer entries", "eb", 40 )
  PARAMETER( SnoopBufferSize, uint32_t, "Number of Snoop Buffer entries", "snoops", 8 )
  PARAMETER( ProbeFetchMiss, bool, "Probe hierarchy on Ifetch miss", "probe_fetchmiss", false )
  PARAMETER( BusTime_NoData, uint32_t, "Bus transfer time - no data", "bustime_nodata", 1 )
  PARAMETER( BusTime_Data, uint32_t, "Bus transfer time - data", "bustime_data", 2 )
  PARAMETER( EvictClean, bool, "Cause the cache to evict clean blocks", "allow_evict_clean", true )
  PARAMETER( EvictWritableHasData, bool, "Send data with EvictWritable messages", "evict_writable_has_data", false )
  PARAMETER( EvictOnSnoop, bool, "Send evictions on Snoop channel", "evict_on_snoop", false )
  PARAMETER( FastEvictClean, bool, "Send clean evicts without reserving data bus", "fast_evict_clean", false )
  PARAMETER( NoBus, bool, "No bus model (i.e., infinite BW, no latency)", "no_bus", false )
  PARAMETER( TraceAddress, uint32_t, "Address to initiate tracing", "trace_address", 0 )
  PARAMETER( CacheType, std::string, "Type of cache (InclusiveMOESI)", "cache_type", "InclusiveMOESI" )
  PARAMETER( ArrayConfiguration, std::string, "Configuration of cache array (STD:sets=1024:assoc=16:repl=LRU", "array_config", "STD:sets=1024:assoc=16:repl=LRU" )
  PARAMETER( UseReplyChannel, bool, "Separate Reply and Snoop channels on BackSide", "use_reply_channel", false )
  PARAMETER( TextFlexpoints, bool, "Store flexpoints as text files (compatible with old FastCache component)", "text_flexpoints", false )
  PARAMETER( GZipFlexpoints, bool, "Compress flexpoints with gzip", "gzip_flexpoints", true )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_D )
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_I )
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Snoop)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Request)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Prefetch)
  PORT(PushOutput, MemoryTransport, BackSideOut_Reply)
  PORT(PushOutput, MemoryTransport, BackSideOut_Snoop)
  PORT(PushOutput, MemoryTransport, BackSideOut_Request)
  PORT(PushOutput, MemoryTransport, BackSideOut_Prefetch)
  PORT(PushInput, MemoryTransport, BackSideIn_Request)
  PORT(PushInput, MemoryTransport, BackSideIn_Reply)
  DRIVE(CacheDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Cache
// clang-format on
