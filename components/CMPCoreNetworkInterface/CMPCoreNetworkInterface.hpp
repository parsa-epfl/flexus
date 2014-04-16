#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPCoreNetworkInterface
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( VChannels, int, "Number of virtual channels", "vChannels", 3 )
  FLEXUS_PARAMETER( RecvCapacity, uint32_t, "Recv Queue Capacity", "recvCapacity", 1)
  FLEXUS_PARAMETER( SendCapacity, uint32_t, "Send Queue Capacity", "sendCapacity", 1)
  FLEXUS_PARAMETER( RequestVc, int, "Virtual channel used for requests", "requestVc", 0)
  FLEXUS_PARAMETER( SnoopVc, int, "Virtual channel used for snoops", "snoopVc", 1)
  FLEXUS_PARAMETER( ReplyVc, int, "Virtual channel used for replies", "replyVc", 1)
);

COMPONENT_INTERFACE(
  // Out into the network
  PORT(PushInput,  MemoryTransport, ISnoopFromCore)
  PORT(PushInput,  MemoryTransport, IRequestFromCore)
  PORT(PushInput,  MemoryTransport, DSnoopFromCore)
  PORT(PushInput,  MemoryTransport, DRequestFromCore)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNetwork)

  // In from the network
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromNetwork)
  PORT(PushOutput, MemoryTransport, IReplyToCore)
  PORT(PushOutput, MemoryTransport, DReplyToCore)
  PORT(PushOutput, MemoryTransport, IRequestToCore)
  PORT(PushOutput, MemoryTransport, DRequestToCore)

  DRIVE(CMPCoreNetworkInterfaceDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMPCoreNetworkInterface
