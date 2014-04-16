#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT FastVictimCache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Enable, bool, "Enable victim buffer", "enable", true )
  PARAMETER( NumEntries, uint32_t, "Number of victim cache entries", "entries", 4 )
  PARAMETER( CacheLineSize, uint32_t, "Cache line size in bytes", "cache_line_size", 64 )
);

COMPONENT_INTERFACE(
  PORT( PushOutput, MemoryMessage, RequestOut )
  PORT( PushOutput, MemoryMessage, SnoopOut )

  PORT( PushInput,  MemoryMessage, RequestIn )
  PORT( PushInput,  MemoryMessage, SnoopIn )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastVictimCache
