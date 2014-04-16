#include <list>

#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT CMOB
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Slices/CMOBMessage.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( CMOBSize, uint32_t, "CMOB Size.  Must be divisible by CMOB Line size (12)", "cmobsize", 131072 * 12  )
  PARAMETER( CMOBName, std::string, "CMOB saved file name", "cmob_name", "TMS_CMOB_p1_" )
  PARAMETER( UseMemory, bool, "Model memory traffic/latency for CMOB", "use_memory", true )
  PARAMETER( CMOBLineSize, int, "CMOB Line Size", "line_size", 12 )
);

typedef boost::intrusive_ptr<CMOBMessage> cmob_message_t;
typedef std::pair<int, long> prefix_read_t;
typedef std::list< PhysicalMemoryAddress > prefix_read_out_t;

COMPONENT_INTERFACE(
  PORT(PushInput, cmob_message_t, TMSif_Request )
  PORT(PushOutput,  cmob_message_t, TMSif_Reply   )

  PORT(PullOutput, long, TMSif_NextAppendIndex )
  PORT(PullOutput, CMOBMessage, TMSif_Initialize )

  PORT(PushOutput, MemoryTransport, ToMemory )
  PORT(PushInput,  MemoryTransport, FromMemory )

  PORT(PushInput,  prefix_read_t, PrefixRead )
  PORT(PushOutput,  prefix_read_out_t, PrefixReadOut )

  DRIVE( CMOBDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMOB
