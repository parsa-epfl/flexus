#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT MRCReflector
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( Enable, bool, "Enable Reflector", "enable", false )
  PARAMETER( TwoEnable, bool, "Enable 2MRC", "two_enable", true )
  PARAMETER( VChannels, int, "Virtual channels", "vc", 3)
  PARAMETER( AllMisses, bool, "Reflect all off-chip misses", "all_misses", false)
);

COMPONENT_INTERFACE(
  PORT(PushOutput, NetworkTransport, ToNic )
  DYNAMIC_PORT_ARRAY(PushInput, NetworkTransport, FromNic )
  PORT(PushInput, PredictorTransport, FromEngine )
  PORT(PushInput, PredictorTransport, FromLocal )
  DRIVE( ReflectorDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MRCReflector
