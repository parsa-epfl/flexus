#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT FastBigBus
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( BlockSize, int, "Size of coherence unit", "bsize", 64 )
  PARAMETER( TrackDMA, bool, "Track DMA blocks", "dma", true )
  PARAMETER( TrackWrites, bool, "Track/Notify on upgrades", "upgrades", false )
  PARAMETER( TrackProductions, bool, "Track/Notify on productions", "productions", false )
  PARAMETER( TrackReads, bool, "Track/Notify on off-chip reads", "offchipreads", false )
  PARAMETER( InvalAll, bool, "Invalidate all nodes on exclusive", "invalall", false )
  PARAMETER( TrackEvictions, bool, "Track/Notify on evictions", "evictions", false )
  PARAMETER( TrackFlushes, bool, "Track/Notify on flushes", "flushes", false )
  PARAMETER( TrackInvalidations, bool, "Track/Notify on invalidations", "invalidations", false )
  PARAMETER( PageSize, uint32_t, "Page size in bytes", "pagesize", 8192)
  PARAMETER( RoundRobin, bool, "Use static round-robin page allocation", "round_robin", true)
  PARAMETER( SavePageMap, bool, "Save page_map.out in checkpoints", "save_page_map", true)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToSnoops)
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FromCaches)

  PORT( PushOutput, MemoryMessage, Writes )
  PORT( PushOutput, MemoryMessage, Reads )
  PORT( PushOutput, MemoryMessage, Fetches )
  PORT( PushOutput, MemoryMessage, Evictions )
  PORT( PushOutput, MemoryMessage, Flushes )
  PORT( PushOutput, MemoryMessage, Invalidations )

  PORT( PushInput, MemoryMessage, DMA )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, NonAllocateWrite )

);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastBigBus
