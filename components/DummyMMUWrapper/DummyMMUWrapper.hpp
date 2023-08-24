#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT DummyMMUWrapper
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( InitState, int, "Initial DummyMMUWrapper state", "init", 0 )
);

COMPONENT_INTERFACE(
  PORT( PushOutput, TranslationPtr, iTranslationReplyDummy )

  DRIVE( DummyMMUWrapperDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DummyMMUWrapper
// clang-format on
