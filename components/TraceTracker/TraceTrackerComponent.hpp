#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT TraceTrackerComponent
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include "components/Common/TraceTracker.hpp"

COMPONENT_PARAMETERS(
  PARAMETER(Enable, bool, "Enable Trace Tracker", "enable", false )
  PARAMETER(NumNodes, int, "Number of nodes", "num-nodes", 16)
  PARAMETER(BlockSize, long, "Cache block size", "bsize", 64 )
  PARAMETER(Sharing, bool, "Enable Sharing Tracker", "sharing" , false ) /* CMU-ONLY */
  PARAMETER(SharingLevel, Flexus::SharedTypes::tFillLevel, "SharingLevel", "sh-level", Flexus::SharedTypes::eL2 ) /* CMU-ONLY */
);

COMPONENT_EMPTY_INTERFACE ;

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TraceTrackerComponent
