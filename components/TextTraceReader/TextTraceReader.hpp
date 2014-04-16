#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#define FLEXUS_BEGIN_COMPONENT TextTraceReader
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPwidth", 16 )
  PARAMETER( MaxReferences, uint64_t, "Maximum number of references to read (0=infinite)", "MaxReferences", 0 )
  PARAMETER( TraceFile, std::string, "Name of input trace file", "trace", "l2_trace" )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, Request_Out )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, FetchRequest_Out )

  DRIVE( TextTraceReaderDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TextTraceReader
