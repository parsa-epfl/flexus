
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( Delay, int, "Access time", "time", 1 )
  PARAMETER( MaxRequests, int, "Maximum requests queued in loopback", "max_requests", 64 )
  PARAMETER( UseFetchReply, bool, "Send FetchReply in response to FetchReq (instead of MissReply)", "UseFetchReply", false )
);

COMPONENT_INTERFACE(
  PORT(  PushOutput, MemoryTransport, LoopbackOut )
  PORT(  PushInput,  MemoryTransport, LoopbackIn )
  DRIVE( LoopbackDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MemoryLoopback
// clang-format on
