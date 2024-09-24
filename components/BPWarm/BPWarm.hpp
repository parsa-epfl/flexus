#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT BPWarm
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/InstructionTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "cores", 1 )
  PARAMETER( BTBSets, uint32_t, "Number of sets in the BTB", "btbsets", 512 )
  PARAMETER( BTBWays, uint32_t, "Number of ways in the BTB", "btbways", 4 )
);

typedef std::pair< uint64_t, uint32_t> ulong_pair;
typedef std::pair< uint64_t, std::pair< uint32_t, uint32_t > > pc_type_annul_triplet;

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, InstructionTransport, InsnIn )
  DYNAMIC_PORT_ARRAY( PushOutput, InstructionTransport, InsnOut )
  DYNAMIC_PORT_ARRAY( PushInput, ulong_pair, ITraceIn )
  DYNAMIC_PORT_ARRAY( PushInput, pc_type_annul_triplet, ITraceInModern )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT BPWarm
// clang-format on
