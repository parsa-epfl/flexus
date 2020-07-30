#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT PortCombiner
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_NO_PARAMETERS ;

COMPONENT_INTERFACE(
  PORT( PushOutput, MemoryTransport, FetchMissOut )
  PORT( PushInput, MemoryTransport, SnoopIn )
  PORT( PushInput, MemoryTransport, ReplyIn )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PortCombiner
