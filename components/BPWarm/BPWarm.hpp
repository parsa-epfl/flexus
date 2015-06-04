#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT BPWarm
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/InstructionTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "cores", 1 )
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

