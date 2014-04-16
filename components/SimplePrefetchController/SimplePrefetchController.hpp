#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT SimplePrefetchController
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PrefetchTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( MaxPrefetches, uint32_t, "Maximum number of parallel prefetches", "prefetches", 16 )
  PARAMETER( QueueSizes, uint32_t, "Size of request & response queues", "queue_size", 8 )
  PARAMETER( L1Size, int, "Cache size in bytes", "size", 65536 )
  PARAMETER( L1Associativity, int, "Set associativity", "assoc", 2 )
  PARAMETER( L1BlockSize, int, "Block size", "bsize", 64 )

);

COMPONENT_INTERFACE(
  PORT(PushOutput, MemoryTransport, Monitor_FrontSideOut )
  PORT(PushInput, MemoryTransport, Monitor_FrontSideIn_Request)
  PORT(PushInput, MemoryTransport, Monitor_FrontSideIn_Prefetch)
  PORT(PushInput, MemoryTransport, Monitor_FrontSideIn_Snoop)
  PORT(PushOutput, MemoryTransport, Monitor_BackSideOut_Request)
  PORT(PushOutput, MemoryTransport, Monitor_BackSideOut_Prefetch)
  PORT(PushOutput, MemoryTransport, Monitor_BackSideOut_Snoop)
  PORT(PushInput, MemoryTransport, Monitor_BackSideIn)

  PORT(PushInput, PrefetchTransport, MasterIn)

  PORT(PushInput, MemoryTransport, Prefetch_FromCore_Request)
  PORT(PushOutput, MemoryTransport, Prefetch_ToL1_Request)
  PORT(PushOutput, MemoryTransport, Prefetch_ToL1_Prefetch)
  PORT(PushInput, MemoryTransport, Prefetch_FromL1_Reply)
  PORT(PushOutput, MemoryTransport, Prefetch_ToCore_Reply)

  DRIVE(CoreToL1Drive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SimplePrefetchController
