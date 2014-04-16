#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT PrefetchBuffer
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PrefetchTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( NumEntries, uint32_t, "# of entries in the prefetch buffer", "entries", 0 )
  PARAMETER( NumWatches, uint32_t, "# of addresses that can be simultaneously watched", "watches", 16 )
  PARAMETER( QueueSizes, uint32_t, "Size of memory message queues", "queues", 8 )
  PARAMETER( MAFSize, uint32_t, "Size of miss address file", "maf", 32 )
  PARAMETER( ProcessingDelay, uint32_t, "# of cycles to process a message", "delay", 0 )
  PARAMETER( EvictClean, bool, "Cause the cache to evict clean blocks", "allow_evict_clean", false )
  PARAMETER( UseStreamFetch, bool, "Issue stream fetch requests", "stream_fetch", false )

);

COMPONENT_INTERFACE(
  PORT(PushOutput, MemoryTransport, FrontSideOut )
  PORT(PushInput, MemoryTransport, FrontSideIn_Snoop)
  PORT(PushInput, MemoryTransport, FrontSideIn_Request)
  PORT(PushInput, MemoryTransport, FrontSideIn_Prefetch)
  PORT(PushOutput, MemoryTransport, BackSideOut_Snoop)
  PORT(PushOutput, MemoryTransport, BackSideOut_Request)
  PORT(PushOutput, MemoryTransport, BackSideOut_Prefetch)
  PORT(PushInput, MemoryTransport, BackSideIn)
  PORT(PushOutput, PrefetchTransport, MasterOut)
  PORT(PushInput, PrefetchTransport, MasterIn)
  PORT(PushOutput, PhysicalMemoryAddress, RecentRequests)
  DRIVE(PrefetchDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PrefetchBuffer
