#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  //FIXME Not sure what this does-- 
  //PARAMETER( QemuQuantum, int64_t, "CPU quantum size in simics", "simics_quantum", 100 )
  PARAMETER( SystemTickFrequency, double, "CPU System tick frequency. 0.0 leaves frequency unchanged", "stick", 0.0 )
  PARAMETER( HousekeepingPeriod, int64_t, "Simics cycles between housekeeping events", "housekeeping_period", 1000 )
  PARAMETER( TrackIFetch, bool, "Track and report instruction fetches", "ifetch", false )
  PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPwidth", 1 )
  PARAMETER( DecoupleInstrDataSpaces, bool, "Decouple instruction from data address spaces", "decouple_addr_spaces", false ) /* CMU-ONLY */

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
  //DYNAMIC_PORT_ARRAY( PushOutput, ulong_pair, ToBPred )
  DYNAMIC_PORT_ARRAY( PushOutput, pc_type_annul_triplet, ToBPred )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToNAW )

  PORT( PushOutput, MemoryMessage, ToDMA )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder
