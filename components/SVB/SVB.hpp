#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT SVB
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PrefetchTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( MAFSize, uint32_t, "Size of miss address file", "maf", 32 )
  PARAMETER( QueueSizes, uint32_t, "Size of request & response queues", "queue_size", 4 )
  PARAMETER( SVBSize, uint32_t, "Size of SVB in cache blocks", "size", 64 )
  PARAMETER( ProcessingDelay, uint32_t, "Processing Delay of SVB accesses", "delay", 0 )

);

COMPONENT_INTERFACE(
  PORT(PushOutput, MemoryTransport, FrontSideOut )
  PORT(PushInput, MemoryTransport, FrontSideIn_Request)
  PORT(PushInput, MemoryTransport, FrontSideIn_Snoop)
  PORT(PushOutput, MemoryTransport, BackSideOut_Request)
  PORT(PushOutput, MemoryTransport, BackSideOut_Snoop)
  PORT(PushInput, MemoryTransport, BackSideIn)
  PORT(PushInput, PrefetchTransport, MasterIn)
  PORT(PushOutput, PrefetchTransport, MasterOut)
  DRIVE(SVBDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SVB
