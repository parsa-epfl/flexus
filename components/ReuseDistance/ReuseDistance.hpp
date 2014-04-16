#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/ReuseDistanceSlice.hpp>

#define FLEXUS_BEGIN_COMPONENT ReuseDistance
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( ReuseWindowSizeInCycles, uint64_t, "Reuse distance window size in cycles", "reuse_win_size", 100000000 ) // 100M cycles
  PARAMETER( BlockSize, int, "Block size in bytes", "bsize", 64 )
  PARAMETER( IgnoreBlocksWithMinDist, int, "Ignore blocks with int64_t reuse distance (0=off)", "ignore_blk_dist", 0 )  // 0 L2 refs (off)
);

COMPONENT_INTERFACE(
  PORT( PushInput, ReuseDistanceSlice, RequestIn )

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT ReuseDistance

