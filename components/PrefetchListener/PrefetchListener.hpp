#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT PrefetchListener
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/PrefetchTransport.hpp>
#include <components/Common/Transports/NetworkTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( ConcurrentPrefetches, int, "# of addresses that can be prefetched in parallel", "concurrent", 16) //Set to zero to disable prefetching
  PARAMETER( NumQueues, uint32_t, "# of streams that can be followed in parallel", "queues", 4) //Set to zero to disable prefetching
  PARAMETER( TargetBlocks, int, "Target Blocks in prefetch buffer", "target", 8)
  PARAMETER( MoreAddressThreshold, int, "Threshold to request more addresses", "more_addresses", 8)
  PARAMETER( VChannels, int, "Virtual channels", "vc", 3)
);

COMPONENT_INTERFACE(

  PORT(PushOutput, PrefetchTransport, ToPrefetchBuffer )
  PORT(PushInput, PrefetchTransport, FromPrefetchBuffer )
  PORT(PushOutput, NetworkTransport, ToNic )
  DYNAMIC_PORT_ARRAY(PushInput, NetworkTransport, FromNic )

  DRIVE( PrefetchListenerDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PrefetchListener
