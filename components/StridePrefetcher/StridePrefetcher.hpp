#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT StridePrefetcher
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( NumEntries, uint32_t, "# of entries in the stream buffer", "entries", 0 )
  PARAMETER( QueueSizes, uint32_t, "Size of memory message queues", "queues", 8 )
  PARAMETER( MAFSize, uint32_t, "Size of miss address file", "maf", 32 )
  PARAMETER( ProcessingDelay, uint32_t, "Delay to check buffer", "delay", 0 )
  PARAMETER( MaxPrefetchQueue, uint32_t, "Size of prefetch queue", "prefetch_queue", 32 )
  PARAMETER( MaxOutstandingPrefetches, int, "Number of outstanding prefetches", "outstanding", 16 )

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
  DRIVE(StrideDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT StridePrefetcher
