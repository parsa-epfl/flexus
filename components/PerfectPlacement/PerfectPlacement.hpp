#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/PerfectPlacementSlice.hpp>

#define FLEXUS_BEGIN_COMPONENT PerfectPlacement
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Size, int, "Cache size in bytes", "size", 16777216 )
  PARAMETER( Associativity, int, "Set associativity", "assoc", 8 )
  PARAMETER( BlockSize, int, "Block size in bytes", "bsize", 64 )

  PARAMETER( PerfReplOnly, bool, "Perfect replacement only", "perf_repl_only", true )
);

COMPONENT_INTERFACE(
  PORT( PushInput, PerfectPlacementSlice, RequestIn )

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PerfectPlacement
