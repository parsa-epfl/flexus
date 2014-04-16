#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT InorderSimicsFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/InstructionTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( TraceFile, std::string, "Trace filename", "trace-file", "")
  PARAMETER( UseTrace, bool, "Use trace file", "use-trace", false)
  PARAMETER( StallCap, long, "Maximum cycles Flexus can stall any simics operation (0 to disable the cap)", "stall-cap", 5000)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PullOutput, InstructionTransport, InstructionOutputPort )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT InorderSimicsFeeder
