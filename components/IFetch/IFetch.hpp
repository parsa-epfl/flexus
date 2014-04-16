#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/InstructionTransport.hpp>
#include <components/Common/Slices/ArchitecturalInstruction.hpp>

#define FLEXUS_BEGIN_COMPONENT IFetch
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( StallInstructions, bool, "Stall instructions", "stalli", true )
  PARAMETER( ICacheLineSize, uint32_t, "Instruction Cache Line's Size in bytes", "icachelinesize", 64)
);

COMPONENT_INTERFACE(
  //The FetchIn port is used to read InstructionTransports from the feeder object.  Note that
  //only the ArchitecturalInstruction slice of the InstructionTransport will be valid after
  //the instruction is read from the Feeder.
  PORT( PullInput, InstructionTransport, FetchIn)
  //IFetch sends the instructions it fetches to the FetchOut port.
  PORT( PushOutput, InstructionTransport, FetchOut)
  //IFetch sends instruction commands to the Instruction Memory via this port.
  PORT( PushOutput, MemoryTransport, IMemOut )
  // Sends snoop acknowledgements out to the cache
  PORT( PushOutput, MemoryTransport, IMemSnoopOut )
  //IFetch recieves Instruction Memory responses via this port.
  PORT( PushInput, MemoryTransport, IMemIn )
  //The FeederDrive drive interface sends a commands to the Feeder and then fetches instructions,
  //then queries the instruction memory for the associated PC.
  DRIVE(FeederDrive)
  //The PipelineDrive forwards instructions along the pipeline, after a
  //response is received from the instruction memory.
  DRIVE(PipelineDrive)

);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT IFetch

