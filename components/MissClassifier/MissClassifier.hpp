#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>

#define FLEXUS_BEGIN_COMPONENT MissClassifier
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( OnOffSwitch, bool, "Turn Miss classifier on/off", "switch_on", false )
  PARAMETER( BlockSize, int, "Block size in bytes", "bsize", 64 )
);

COMPONENT_INTERFACE(
  PORT( PushInput, MemoryMessage, RequestIn )

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MissClassifier
