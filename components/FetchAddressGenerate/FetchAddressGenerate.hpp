
#include <components/uFetch/uFetchTypes.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( MaxFetchAddress, uint32_t, "Max fetch addresses generated per cycle", "faddrs", 10 )
  PARAMETER( MaxBPred, uint32_t, "Max branches predicted per cycle", "bpreds", 2 )
  PARAMETER( Threads, uint32_t, "Number of threads under control of this FAG", "threads", 1 )
  PARAMETER( BTBSets, uint32_t, "Number of sets in the BTB", "btbsets", 512 )
  PARAMETER( BTBWays, uint32_t, "Number of ways in the BTB", "btbways", 4 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<BPredRedictRequest>, RedirectIn )
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<BPredState>, BranchTrainIn )

  DYNAMIC_PORT_ARRAY( PushOutput, boost::intrusive_ptr<FetchCommand>, FetchAddrOut )
  DYNAMIC_PORT_ARRAY( PullInput, int, AvailableFAQ )
  DYNAMIC_PORT_ARRAY( PullOutput, bool, Stalled)
  PORT( PullInput, bool, uArchHalted)

  DRIVE( FAGDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate
// clang-format on
