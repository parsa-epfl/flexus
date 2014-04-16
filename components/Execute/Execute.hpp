#include <core/simulator_layout.hpp>

#include <components/Common/Transports/InstructionTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */

#define FLEXUS_BEGIN_COMPONENT Execute
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( SBSize, uint32_t, "Store buffer size", "sb", 0 )
  PARAMETER( ROBSize, uint32_t, "Reorder buffer size", "rob", 16 )
  PARAMETER( LSQSize, uint32_t, "Load-Store queue size", "lsq", 8 )
  PARAMETER( MemoryWidth, uint32_t, "Mem width", "memory", 4 )
  PARAMETER( SequentialConsistency, bool, "Sequential Consistency", "sc", false )
  PARAMETER( StorePrefetching, bool, "StorePrefetching", "stpf", false )
  PARAMETER( MaxSpinLoads, int, "Max number of loads in spin loops", "MaxSpinLoads", 3 )
);

COMPONENT_INTERFACE(
  PORT(  PushInput, InstructionTransport, ExecuteIn )
  PORT(  PushOutput, MemoryTransport, ExecuteMemRequest )
  PORT(  PushOutput, MemoryTransport, ExecuteMemSnoop )
  PORT(  PushInput, MemoryTransport, ExecuteMemReply )
  PORT(  PushOutput, PredictorTransport, ToConsort )  /* CMU-ONLY */
  DRIVE( ExecuteDrive )
  DRIVE( CommitDrive )

);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Execute

