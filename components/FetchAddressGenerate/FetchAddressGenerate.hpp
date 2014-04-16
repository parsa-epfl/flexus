#include <core/simulator_layout.hpp>

#include <components/uFetch/uFetchTypes.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

typedef std::pair<Flexus::SharedTypes::VirtualMemoryAddress, Flexus::SharedTypes::VirtualMemoryAddress> vaddr_pair;

COMPONENT_PARAMETERS(
  PARAMETER( MaxFetchAddress, uint32_t, "Max fetch addresses generated per cycle", "faddrs", 10 )
  PARAMETER( MaxBPred, uint32_t, "Max branches predicted per cycle", "bpreds", 2 )
  PARAMETER( Threads, uint32_t, "Number of threads under control of this FAG", "threads", 1 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, vaddr_pair, RedirectIn )
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackIn )
  DYNAMIC_PORT_ARRAY( PushOutput, boost::intrusive_ptr<FetchCommand>, FetchAddrOut )
  DYNAMIC_PORT_ARRAY( PullInput, int, AvailableFAQ )
  DYNAMIC_PORT_ARRAY( PullOutput, bool, Stalled)
  DRIVE( FAGDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

