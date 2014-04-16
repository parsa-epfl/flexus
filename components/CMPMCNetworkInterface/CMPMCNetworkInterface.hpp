#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPMCNetworkInterface
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NumL2Tiles, int, "Number of L2 Tiles", "num_L2_tiles", 16 )
  FLEXUS_PARAMETER( NetRecvCapacity, uint32_t, "Net Recv Queue Capacity", "net-recv-capacity", 1)
  FLEXUS_PARAMETER( NetSendCapacity, uint32_t, "Net Send Queue Capacity", "net-send-capacity", 1)
  FLEXUS_PARAMETER( MCQueueCapacity, uint32_t, "MC Send/Recv Queue Capacity", "mc-queue-capacity", 1)
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RequestVc, int, "Virtual channel used for requests", "requestVc", 0)
  FLEXUS_PARAMETER( SnoopVc, int, "Virtual channel used for snoops", "snoopVc", 0)
  FLEXUS_PARAMETER( ReplyVc, int, "Virtual channel used for replies", "replyVc", 0)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, RequestFromL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ReplyToL2)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, RequestToMem)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, ReplyFromMem)

  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, SnoopFromL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, SnoopToL2)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNetwork)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromNetwork)

  DRIVE(CMPMCNetworkInterfaceDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMPMCNetworkInterface
