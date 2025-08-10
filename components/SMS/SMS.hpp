
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/SMS/smsTypes.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT SMS
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( NumAccTableEntries, uint32_t, "Number of entries in the Accumulation Table", "numAccTableEntries", 64)
  PARAMETER( NumFilterTableEntries, uint32_t, "Number of entries in the Filter Table", "numFilterTableEntries", 64)
  PARAMETER( NumPHTSets, uint32_t, "Number of sets in the Pattern History Table", "numPHTSets", 1024)
  PARAMETER( PHTAssociativity, uint32_t, "Associativity of the Pattern History Table", "phtAssociativity", 16)
  PARAMETER( NumBlks, uint32_t, "Number of blocks in a SMS region", "numBlks", 32)
  PARAMETER( SMSRot, bool, "Use rotation in the SMS", "smsRot", false)
  PARAMETER( SMSSepRdWr, bool, "Use separate read and write patterns", "smsSepRdWr", false)
  PARAMETER( SMSUseSatCnts, bool, "Use saturation counters in the SMS", "smsUseSatCnts", false)
  PARAMETER( PerfectPHT, bool, "Use perfect PHT", "perfectPHT", false)
);

COMPONENT_INTERFACE(
  PORT( PushInput, MemoryTransport, RequestIn )
  PORT( PushInput, MemoryTransport, SnoopIn )
  PORT( PushOutput, MemoryTransport, Prefetch_Request )

  DRIVE( SMSDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SMS
// clang-format on