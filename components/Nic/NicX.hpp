#ifndef FLEXUS_NIC_NUMPORTS
#error "FLEXUS_NIC_NUMPORTS must be defined"
#endif

#include <core/simulator_layout.hpp>
#include <boost/preprocessor/repeat.hpp>

#define NicX BOOST_PP_CAT(Nic, FLEXUS_NIC_NUMPORTS)

#define FLEXUS_BEGIN_COMPONENT NicX
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>

#define NodeOutputPort(z,N,_) DYNAMIC_PORT_ARRAY( PushOutput, NetworkTransport, BOOST_PP_CAT(ToNode,N) )
#define NodeInputPort(z,N,_) PORT( PushInput, NetworkTransport, BOOST_PP_CAT(FromNode,N) )

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RecvCapacity, uint32_t, "Recv Queue Capacity", "recv-capacity", 1)
  FLEXUS_PARAMETER( SendCapacity, uint32_t, "Send Queue Capacity", "send-capacity", 1)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, NetworkTransport, ToNetwork )
  DYNAMIC_PORT_ARRAY( PushInput, NetworkTransport, FromNetwork )
  BOOST_PP_REPEAT(FLEXUS_NIC_NUMPORTS, NodeOutputPort, _ )
  BOOST_PP_REPEAT(FLEXUS_NIC_NUMPORTS, NodeInputPort, _ )
  DRIVE( NicDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NicX
