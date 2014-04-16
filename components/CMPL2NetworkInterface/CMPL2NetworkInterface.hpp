#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPL2NetworkInterface
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NumL2Tiles, uint32_t, "Number of L2 tiles in the CMP network", "numL2Tiles", 0)
  FLEXUS_PARAMETER( NumMemControllers, uint32_t, "Number of memory controllers in the CMP", "numMemControllers", 4)
  FLEXUS_PARAMETER( L2InterleavingGranularity, uint32_t, "Granularity in bytes at which the L2 tiles are interleaved", "l2InterleavingGranularity", 64)
  FLEXUS_PARAMETER( Placement, std::string, "Placement policy [shared, private, R-NUCA]", "placement_policy", "shared" )

  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RecvCapacity, uint32_t, "Recv Queue Capacity", "recv-capacity", 1)
  FLEXUS_PARAMETER( SendCapacity, uint32_t, "Send Queue Capacity", "send-capacity", 1)
  FLEXUS_PARAMETER( RequestVc, int, "Virtual channel used for requests", "requestVc", 0)
  FLEXUS_PARAMETER( SnoopVc, int, "Virtual channel used for snoops", "snoopVc", 1)
  FLEXUS_PARAMETER( ReplyVc, int, "Virtual channel used for replies", "replyVc", 1)
);

COMPONENT_INTERFACE(
  // Out into the network
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, IReplyFromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, DReplyFromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, IRequestFromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, DRequestFromL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNetwork)

  // from/to backside
  PORT(PushInput,  MemoryTransport, RequestFromL2BackSide)
  PORT(PushInput,  MemoryTransport, SnoopFromL2BackSide)
  PORT(PushOutput, MemoryTransport, RequestToMem)
  PORT(PushOutput, MemoryTransport, ReplyToL2)
  PORT(PushOutput, MemoryTransport, SnoopToL2)
  PORT(PushInput,  MemoryTransport, ReplyFromMem)

  // In from the network
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromNetwork)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ISnoopToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, IRequestToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DSnoopToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DRequestToL2)

  DRIVE(CMPL2NetworkInterfaceDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMPL2NetworkInterface
