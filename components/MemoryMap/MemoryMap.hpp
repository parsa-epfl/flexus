
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MemoryMap
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( PageSize, uint32_t, "Page size in bytes (used by statistics only)", "pagesize", 8192 )
  PARAMETER( NumNodes, unsigned, "Number of Nodes", "nodes", 16)
  PARAMETER( RoundRobin, bool, "Use static round-robin page allocation", "round_robin", false)
  PARAMETER( CreatePageMap, bool, "Write page map as pages are created", "write_page_map", false)
  PARAMETER( ReadPageMap, bool, "Load Page Map on start", "page_map", false)
);

COMPONENT_EMPTY_INTERFACE ;

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MemoryMap
// clang-format on
