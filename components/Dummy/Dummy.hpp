
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT Dummy
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( InitState, int, "Initial state", "init", 0 )
);

COMPONENT_INTERFACE(
  PORT( PushOutput, int, getState )
  PORT( PushInput,  int, setState )
  PORT( PullInput, int, pullStateIn )
  PORT( PullOutput, int, pullStateRet )
  
  DYNAMIC_PORT_ARRAY( PushInput, int, setStateDyn )
  DYNAMIC_PORT_ARRAY( PushOutput, int, getStateDyn )
  DYNAMIC_PORT_ARRAY( PullInput, int, pullStateInDyn )
  DYNAMIC_PORT_ARRAY( PullOutput, int, pullStateRetDyn )
  DRIVE( DummyDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Dummy
// clang-format on
