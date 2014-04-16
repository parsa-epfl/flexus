#ifndef FLEXUS_FASTMEMORYLOOPBACK_MEMSTATS_HPP_INCLUDED
#define FLEXUS_FASTMEMORYLOOPBACK_MEMSTATS_HPP_INCLUDED

#include <core/stats.hpp>

#define debug(AAA) DBG_(Tmp, ( <<"TRACING  "<< AAA << std::dec) );

namespace nFastMemoryLoopback {

namespace Stat = Flexus::Stat;

struct MemStats {
  Stat::StatCounter theReadRequests_stat;
  Stat::StatCounter theWriteRequests_stat;
  Stat::StatCounter theEvictDirtys_stat;
  Stat::StatCounter theEvictCleans_stat;
  Stat::StatCounter theEvictWritables_stat;
  Stat::StatCounter theNonAllocatingStoreReq_stat;
  Stat::StatCounter theUpgradeRequest_stat;
  Stat::StatCounter theWriteDMA_stat;
  Stat::StatCounter theReadDMA_stat;
  
 MemStats(std::string const & theName)
    : theReadRequests_stat(theName + "-Reads")
    , theWriteRequests_stat(theName + "-Writes")
    , theEvictDirtys_stat(theName + "-Evict:Dirty")
    , theEvictCleans_stat(theName + "-Evict:Clean")
    , theEvictWritables_stat(theName + "-Evict:Writable")
    , theNonAllocatingStoreReq_stat(theName + "-NonAllocStore")
    , theUpgradeRequest_stat(theName + "-Upgrade")
    , theWriteDMA_stat(theName + "-DMA:Write")
    , theReadDMA_stat(theName + "-DMA:Read")
  {
  }
  void update() {
  }
};

}  // namespace nFastMemoryLoopback
#endif /* FLEXUS_FASTMEMORYLOOPBACK_MEMSTATS_HPP_INCLUDED */
