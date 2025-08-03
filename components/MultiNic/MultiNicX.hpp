
#ifndef FLEXUS_MULTI_NIC_NUMPORTS
#error "FLEXUS_MULTI_NIC_NUMPORTS must be defined"
#endif

#include <boost/preprocessor/repeat.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define MultiNicX BOOST_PP_CAT(MultiNic, FLEXUS_MULTI_NIC_NUMPORTS)

#define FLEXUS_BEGIN_COMPONENT MultiNicX
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#define NodeOutputPort(z,N,_) DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, BOOST_PP_CAT(ToNode,N) )
#define NodeInputPort(z,N,_) DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, BOOST_PP_CAT(FromNode,N) )

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RecvCapacity, uint32_t, "Recv Queue Capacity", "recv-capacity", 1)
  FLEXUS_PARAMETER( SendCapacity, uint32_t, "Send Queue Capacity", "send-capacity", 1)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, ToNetwork )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNetwork )
  BOOST_PP_REPEAT(FLEXUS_MULTI_NIC_NUMPORTS, NodeOutputPort, _ )
  BOOST_PP_REPEAT(FLEXUS_MULTI_NIC_NUMPORTS, NodeInputPort, _ )
  DRIVE( MultiNicDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MultiNicX
// clang-format on
