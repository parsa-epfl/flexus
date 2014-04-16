#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT CacheTraceMemory
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( NumProcs, int, "Number of Processors", "procs", 16)
  PARAMETER( BlockSize, int, "Size of coherence unit", "bsize", 64 )
  PARAMETER( TraceEnabled, bool, "Enable tracing", "trace", false )
  PARAMETER( TrackUniqueAddress, bool, "Enable Tracking of unique addresses", "track-unique", false )
  PARAMETER( CurvesEnabled, bool, "Add a measurement to track stats over time", "curve", false )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, ToNode)
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNode_Snoop)
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNode_Req)
  DRIVE(MemoryDrive)

);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CacheTraceMemory
