#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( HousekeepingPeriod, int64_t, "Simics cycles between housekeeping events", "housekeeping_period", 1000 )
  PARAMETER( CMPWidth, std::size_t, "Number of cores per CMP chip (0 = sys width)", "CMPwidth", 0)

  //FIXME Trying to see if it works without WhiteBox
  //PARAMETER( WhiteBoxDebug, bool, "WhiteBox debugging on/off", "whitebox_debug", false )
  //PARAMETER( WhiteBoxPeriod, int, "WhiteBox period", "whitebox_debug_period", 10000 )
  PARAMETER( SendNonAllocatingStores, bool, "Send NonAllocatingStores on/off", "send_non_allocating_stores", false )
);

typedef std::pair< uint64_t, uint32_t> ulong_pair;
typedef std::pair< uint64_t, std::pair< uint32_t, uint32_t> > pc_type_annul_triplet;

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToL1D )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToL1I )
  DYNAMIC_PORT_ARRAY( PushOutput, pc_type_annul_triplet, ToBPred )
  DYNAMIC_PORT_ARRAY( PushOutput, TranslationPtr, ToMMU )
  PORT( PushOutput, MemoryMessage, ToNIC )  // TODO: Should technically be connected to the Root Complex. But we don't have a Root Complex yet.
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder
// clang-format on