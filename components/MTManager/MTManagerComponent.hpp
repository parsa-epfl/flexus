
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MTManagerComponent
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include "components/MTManager/MTManager.hpp"

static const int32_t kFE_StrictRoundRobin = 0;
static const int32_t kFE_ICount = 1;

static const int32_t kBE_StrictRoundRobin = 0;
static const int32_t kBE_SmartRoundRobin = 1;

COMPONENT_PARAMETERS(
  PARAMETER(Cores, uint32_t, "Number of cores", "cores", 1 )
  PARAMETER(Threads, uint32_t, "Number of threads per core", "threads", 1 )
  PARAMETER(FrontEndPolicy, int, "Scheduling policy for front end", "front_policy", kFE_StrictRoundRobin )
  PARAMETER(BackEndPolicy, int, "Scheduling policy for back end", "back_policy", kBE_StrictRoundRobin )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PullInput, bool, EXStalled )
  DYNAMIC_PORT_ARRAY( PullInput, bool, FAGStalled )
  DYNAMIC_PORT_ARRAY( PullInput, bool, FStalled )
  DYNAMIC_PORT_ARRAY( PullInput, bool, DStalled )
  DYNAMIC_PORT_ARRAY( PullInput, int, FAQ_ICount)
  DYNAMIC_PORT_ARRAY( PullInput, int, FIQ_ICount)
  DYNAMIC_PORT_ARRAY( PullInput, int, ROB_ICount)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MTManagerComponent
// clang-format on
