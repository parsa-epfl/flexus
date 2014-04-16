#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT NUCATracer
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( TraceOutPath, std::string, "trace output path", "trace_out_path", "/afs/scotch/project/nuca_cache" )
  PARAMETER( OnSwitch, bool, "tracer on switch", "on_switch", false)
);

COMPONENT_INTERFACE(
  PORT( PushInput, MemoryMessage, Reads)
  PORT( PushInput, MemoryMessage, Writes)
  PORT( PushInput, MemoryMessage, Fetches)
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, L2Reads )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, L2Writes )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, L2Fetches )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, L1CleanEvicts )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, L1DirtyEvicts )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, L1IEvicts )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NUCATracer
