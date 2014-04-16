#include <core/simulator_layout.hpp>

#include <components/Common/Transports/DirectoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT DirMux2
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_NO_PARAMETERS ;

COMPONENT_INTERFACE(
  PORT(  PushOutput, DirectoryTransport, TopOut1 )
  PORT(  PushOutput, DirectoryTransport, TopOut2 )
  PORT(  PushOutput, DirectoryTransport, BottomOut )
  PORT(  PushInput,  DirectoryTransport, TopIn1 )
  PORT(  PushInput,  DirectoryTransport, TopIn2 )
  PORT(  PushInput,  DirectoryTransport, BottomIn )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DirMux2
