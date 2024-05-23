
#ifndef FLEXUS_TRACER_COUNTER_HPP
#define FLEXUS_TRACER_COUNTER_HPP

#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/stats.hpp>
namespace Stat = Flexus::Stat;

/**
 * Class that hold all statitics per core
 * for the Tracing (KeenKraken) simulator
 *
 * So far is impletmented only
 *  - feeder
 *  - iTLB
 *  - dTLB
 **/
class TracerCounter
{

  private:
    std::string prefix;

  private:
    Stat::StatCounter feeder_memory_stat;
    Stat::StatCounter feeder_io_stat;
    Stat::StatCounter feeder_fetch_stat;
    Stat::StatCounter feeder_rmw_stat;
    Stat::StatCounter feeder_load_stat;
    Stat::StatCounter feeder_store_stat;
    Stat::StatCounter feeder_prefetch_stat;

    Stat::StatCounter itlb_request_stat;
    Stat::StatCounter itlb_hit_stat;
    Stat::StatCounter itlb_miss_stat;
    Stat::StatCounter itlb_access_stat;

    Stat::StatCounter dtlb_request_stat;
    Stat::StatCounter dtlb_hit_stat;
    Stat::StatCounter dtlb_miss_stat;
    Stat::StatCounter dtlb_access_stat;

  public:
    uint64_t feeder_io;
    uint64_t feeder_fetch;
    uint64_t feeder_rmw;
    uint64_t feeder_load;
    uint64_t feeder_store;
    uint64_t feeder_prefetch;

    uint64_t itlb_hit;
    uint64_t itlb_miss;
    uint64_t itlb_request;
    uint64_t itlb_access;
    uint64_t dtlb_hit;
    uint64_t dtlb_miss;
    uint64_t dtlb_request;
    uint64_t dtlb_access;

  public:
    TracerCounter(std::string const& prefix)
      : feeder_memory_stat(prefix + "MemOps")
      , feeder_io_stat(prefix + "IOOps")
      , feeder_fetch_stat(prefix + "Fetches")
      , feeder_rmw_stat(prefix + "RMWOps")
      , feeder_load_stat(prefix + "LoadOps")
      , feeder_store_stat(prefix + "StoreOps")
      , feeder_prefetch_stat(prefix + "PrefetchOps")
      ,

      itlb_request_stat(prefix + "iTLB-TotalReqs")
      , itlb_hit_stat(prefix + "iTLB-Hits")
      , itlb_miss_stat(prefix + "iTLB-Misses")
      , itlb_access_stat(prefix + "iTLB-MemAccesses")
      , dtlb_request_stat(prefix + "dTLB-TotalReqs")
      , dtlb_hit_stat(prefix + "dTLB-Hits")
      , dtlb_miss_stat(prefix + "dTLB-Misses")
      , dtlb_access_stat(prefix + "dTLB-MemAccesses")
      ,
      //--
      feeder_io(0)
      , feeder_fetch(0)
      , feeder_rmw(0)
      , feeder_load(0)
      , feeder_store(0)
      , feeder_prefetch(0)
      ,

      itlb_hit(0)
      , itlb_miss(0)
      , itlb_request(0)
      , itlb_access(0)
      , dtlb_hit(0)
      , dtlb_miss(0)
      , dtlb_request(0)
      , dtlb_access(0)
    {
    }

    void update_collector()
    {
        feeder_memory_stat += feeder_load + feeder_store + feeder_rmw + feeder_prefetch;
        feeder_io_stat += feeder_io;
        feeder_fetch_stat += feeder_fetch;
        feeder_rmw_stat += feeder_rmw;
        feeder_load_stat += feeder_load;
        feeder_store_stat += feeder_store;
        feeder_prefetch_stat += feeder_prefetch;

        itlb_request_stat += itlb_request;
        itlb_access_stat += itlb_access;
        itlb_hit_stat += itlb_hit;
        itlb_miss_stat += itlb_miss;
        dtlb_request_stat += dtlb_request;
        dtlb_access_stat += dtlb_access;
        dtlb_hit_stat += dtlb_hit;
        dtlb_miss_stat += dtlb_miss;
        //--
        feeder_io       = 0;
        feeder_fetch    = 0;
        feeder_rmw      = 0;
        feeder_load     = 0;
        feeder_store    = 0;
        feeder_prefetch = 0;

        itlb_hit     = 0;
        itlb_miss    = 0;
        itlb_request = 0;
        itlb_access  = 0;
        dtlb_hit     = 0;
        dtlb_miss    = 0;
        dtlb_request = 0;
        dtlb_access  = 0;
    }
};

#endif // FLEXUS_TRACER_COUNTER_HPP