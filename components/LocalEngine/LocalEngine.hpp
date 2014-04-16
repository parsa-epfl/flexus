#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT LocalEngine
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/Common/Transports/DirectoryTransport.hpp>

COMPONENT_NO_PARAMETERS ;

COMPONENT_INTERFACE(
  PORT( PushOutput, MemoryTransport, ToCache )
  PORT( PushOutput, MemoryTransport, ToMemory )
  PORT( PushOutput, MemoryTransport, ToProtEngines )
  PORT( PushOutput, DirectoryTransport, ToDirectory )
  PORT( PushOutput, PhysicalMemoryAddress, WritePermissionLost )
  PORT( PushOutput, PredictorTransport, ToPredictor ) /* CMU-ONLY */
  PORT( PushInput, MemoryTransport, FromCache_Request )
  PORT( PushInput, MemoryTransport, FromCache_Prefetch )
  PORT( PushInput, MemoryTransport, FromCache_Snoop )
  PORT( PushInput, MemoryTransport, FromMemory )
  PORT( PushInput, MemoryTransport, FromProtEngines )
  PORT( PushInput, DirectoryTransport, FromDirectory )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT LocalEngine
