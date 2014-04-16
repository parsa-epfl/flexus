#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT ProtocolEngine
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/DirectoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */

COMPONENT_PARAMETERS(
  PARAMETER( Remote, int, "Use Remote Engine", "useRE", 1 )
  PARAMETER( SpuriousSharerOpt, int, "Use SpuriousSharer Optimization", "useSpuriousSharerOpt", 1 )
  PARAMETER( TSRFSize, uint32_t, "TSRF Size", "tsrf", 16 )
  PARAMETER( VChannels, int, "Virtual channels", "vc", 3)
  //PARAMETER(RecvCapacity, uint32_t, "Receive Capacity", "receive-capacity", 8)
  PARAMETER(CPI, int, "Cycles per instr", "cpi", 4)
);

COMPONENT_INTERFACE(
  PORT(PushOutput, NetworkTransport, ToNic )
  DYNAMIC_PORT_ARRAY(PushInput, NetworkTransport, FromNic )
  PORT(PushOutput, DirectoryTransport, ToDirectory )
  PORT(PushInput, DirectoryTransport, FromDirectory )
  PORT(PushOutput, MemoryTransport, ToCpu )
  PORT(PushInput, MemoryTransport, FromCpu )
  PORT(PushOutput, PredictorTransport, ToPredictor ) /* CMU-ONLY */
  DRIVE(ProtocolEngineDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT ProtocolEngine
