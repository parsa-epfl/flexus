
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/SMS/smsTypes.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT SMS
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( EnableSMS, bool, "Enable SMS", "enableSMS", false)
  PARAMETER( NumAccTableEntries, uint32_t, "Number of entries in the Accumulation Table", "numAccTableEntries", 64)
  PARAMETER( NumFilterTableEntries, uint32_t, "Number of entries in the Filter Table", "numFilterTableEntries", 64)
  PARAMETER( NumPHTSets, uint32_t, "Number of sets in the Pattern History Table", "numPHTSets", 1024)
  PARAMETER( PHTAssociativity, uint32_t, "Associativity of the Pattern History Table", "phtAssociativity", 16)
  PARAMETER( BlockSize, uint32_t, "Block size in bytes", "blockSize", 64)
  PARAMETER( NumBlks, uint32_t, "Number of blocks in a SMS region", "numBlks", 32)
  PARAMETER( SMSRot, bool, "Use rotation in the SMS", "smsRot", false)
  PARAMETER( SMSSepRdWr, bool, "Use separate read and write patterns", "smsSepRdWr", false)
  PARAMETER( SMSUseSatCnts, bool, "Use saturation counters in the SMS", "smsUseSatCnts", false)
  PARAMETER( PerfectPHT, bool, "Use perfect PHT", "perfectPHT", false)
);

COMPONENT_INTERFACE(
  PORT( PushInput, MemoryTransport, PredictIn )     // Triggers are sent here
  PORT( PushInput, boost::intrusive_ptr<SMSTrainInfo>, TrainIn )       // Training is done through here
  PORT( PushInput, MemoryTransport, EvictInvalIn )  // Evicts/Invalidates are sent here
  PORT( PushOutput, MemoryTransport, PredictOut )   // Prefetches arising from triggers are sent out from here
  DRIVE( SMSDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SMS
// clang-format on