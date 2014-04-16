#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT Consort
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( Enable, bool, "Enable Reflector", "enable", false )
  PARAMETER( OrderSize, uint32_t, "Size of order circular buffer", "size", 250000 )
  PARAMETER( AddressList, uint32_t, "Size of address list for each request", "list", 16)
  PARAMETER( CoherenceBlockSize, uint32_t, "Size of coherence unit", "block_size", 64)
  PARAMETER( OrderReadLatency, uint32_t, "Latency to fetch a block from the CMOB", "cmob_lat", 0)
  PARAMETER( OrderCacheSize, uint32_t, "Size of the order cache", "cmob_cache", 32)
  PARAMETER( VChannels, int, "Virtual channels", "vc", 3)
  PARAMETER( Trace, bool, "Create trace", "trace", false)
  PARAMETER( AllMisses, bool, "Record all off-chip misses", "all_misses", false)
);

COMPONENT_INTERFACE(
  PORT(PushOutput, NetworkTransport, ToNic )
  DYNAMIC_PORT_ARRAY(PushInput, NetworkTransport, FromNic )
  PORT(PushInput, PredictorTransport, FromCPU )
  DRIVE( ConsortDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Consort
