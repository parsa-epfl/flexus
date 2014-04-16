#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT TraceVMMapper
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(

  PARAMETER( VMIndexShift, int, "How many bits to shift VM index when creating new address (32).", "VMIndexShift", 32)
  PARAMETER( VMXORShift, int, "XOR the VM index shifted this many bits with the address, (-1 = no xor).", "VMXORShift", -1)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, RequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, FetchRequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, SnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, SnoopIn_D)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, SnoopIn_I)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, NonAllocateWrite_In)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryMessage, DMA_In)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryMessage, RequestOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryMessage, FetchRequestOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryMessage, SnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryMessage, SnoopOut_D)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryMessage, SnoopOut_I)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryMessage, NonAllocateWrite_Out)
  PORT(PushOutput, MemoryMessage, DMA_Out)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TraceVMMapper
