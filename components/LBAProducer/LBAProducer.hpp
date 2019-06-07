#include <core/simulator_layout.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/RMCEntry.hpp>
#include "Generator.hpp"

#define LBA_DEBUG Iface

#define FLEXUS_BEGIN_COMPONENT LBAProducer
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( EnableServiceTimeGen, uint32_t, "Toggle on or off - only when on the parameters below will be used", "EnableServiceTimeGen", 0 )
  PARAMETER( ServiceTimeDistribution, std::string, "The distribution of emulated service times", "ServiceTimeDistribution", "uniform" )
  PARAMETER( DistrArg0, double, "1st argument for selected service time distribution", "DistrArg0", 0)
  PARAMETER( DistrArg1, double, "2nd argument for selected service time distribution", "DistrArg1", 0)
  PARAMETER( DistrArg2, double, "3rd argument for selected service time distribution", "DistrArg2", 0)
  PARAMETER( BaseServiceLatency, uint32_t, "The base service latency", "BaseServiceLatency", 100)
  PARAMETER( UseServTimeFile, bool, "Whether or not to use service times from a file", "UseServTimeFile", false)
  PARAMETER( ServTimeFName, std::string, "The file containing service times in binary format.", "ServTimeFName", "")
  PARAMETER( NumElementsInServTimeFile , uint64_t , "Number of service times contained in the file.", "NumElementsInServTimeFile", 0)
);


COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr< AbstractInstruction >, MagicInstructionIn )
  PORT( PushInput, RMCEntry, FunctionIn )

  DYNAMIC_PORT_ARRAY( PushOutput, uint32_t, CoreControlFlow )

  DRIVE( Cycle )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT LBAProducer
