#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT TraceTrackerComponent
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include "components/CommonQEMU/TraceTracker.hpp"

COMPONENT_PARAMETERS(
  PARAMETER(Enable, bool, "Enable Trace Tracker", "enable", false )
  PARAMETER(NumNodes, int, "Number of nodes", "num-nodes", 16)
  PARAMETER(BlockSize, long, "Cache block size", "bsize", 64 )
);

COMPONENT_EMPTY_INTERFACE ;

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TraceTrackerComponent
// clang-format on
