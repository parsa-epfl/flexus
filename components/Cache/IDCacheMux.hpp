#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT IDCacheMux
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_NO_PARAMETERS ;

COMPONENT_INTERFACE(
  PORT( PushOutput, MemoryTransport, TopOutI )
  PORT( PushOutput, MemoryTransport, TopOutD )
  PORT( PushOutput, MemoryTransport, BottomOut )
  PORT( PushOutput, MemoryTransport, BottomOut_Snoop )
  PORT( PushInput, MemoryTransport, TopInI )
  PORT( PushInput, MemoryTransport, TopInD )
  PORT( PushInput, MemoryTransport, BottomIn )
  PORT( PushInput, MemoryTransport, TopIn_SnoopD )
  PORT( PushInput, MemoryTransport, TopIn_SnoopI )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT IDCacheMux
