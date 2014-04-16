#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT StreamController
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/PrefetchTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( StreamQueues, uint32_t, "Number of stream queues", "queues", 0 ) //0 is unbounded
  PARAMETER( FetchTarget, uint32_t, "Number of blocks per stream to fetch", "target", 4 )
  PARAMETER( MessageQueueSizes, uint32_t, "Size of message queues", "msg_queues", 8 )

);

COMPONENT_INTERFACE(
  PORT(PushOutput, PrefetchTransport, SVBOut)
  PORT(PushInput, PrefetchTransport, SVBIn)
  PORT(PushInput, PrefetchTransport, PredictorIn)
  DRIVE(StreamDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT StreamController
