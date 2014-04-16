#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT DRAMController
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/DRAMController/DRAMSim2/MultiChannelMemorySystem.h>

COMPONENT_PARAMETERS(
  PARAMETER( MemorySystemFile, std::string, "Memory System configuration file", "memory-system-file", "") 
  PARAMETER( DeviceFile, std::string, "DRAM device configuration file", "device-file", "") 
  PARAMETER( Frequency, int, "Clock Frequency of the CPU", "frequency", 2000 )
  PARAMETER( InterconnectDelay, int, "Circuitry delay for off-chip communication in ns", "InterconnectDelay", 15 )
  PARAMETER( Interleaving, int, "Address interleaving among memory controllers (in bytes)", "interleaving", 4096 )
  PARAMETER( Size, int, "Size of the memory  in MB", "size", 2048 )
  PARAMETER( DynamicSize, bool, "Adjast size to the workload", "dyn_size", true )
  PARAMETER( MaxRequests, int, "Maximum requests queued in loopback", "max_requests", 128 )
  PARAMETER( MaxReplies, int, "Maximum replies queued in loopback", "max_replies", 8 )
  PARAMETER( UseFetchReply, bool, "Send FetchReply in response to FetchReq (instead of MissReply)", "UseFetchReply", false )
);

COMPONENT_INTERFACE(
  PORT(  PushOutput, MemoryTransport, LoopbackOut )
  PORT(  PushInput,  MemoryTransport, LoopbackIn )
  DRIVE( DRAMDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DRAMController
