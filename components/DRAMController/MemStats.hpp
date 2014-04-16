#ifndef __MEM_STATS_HPP__
#define __MEM_STATS_HPP__

#include <core/stats.hpp>

namespace DRAMSim {

struct MemStats {
public:
  Flexus::Stat::StatCounter MemWrites;
  Flexus::Stat::StatCounter MemReads;
  Flexus::Stat::StatCounter MemEvictions;
  Flexus::Stat::StatCounter MemNAS;
  Flexus::Stat::StatCounter MinLatency;
  Flexus::Stat::StatCounter MaxLatency;
  Flexus::Stat::StatInstanceCounter<int64_t> Latency_Histogram;
  int64_t min, max;

  MemStats(const std::string & aName) :
      MemWrites(aName + "-Writes")
    , MemReads(aName + "-Reads")
    , MemEvictions(aName + "-Evictions")
    , MemNAS(aName + "-NonAllocatingStores")
    , MinLatency(aName + "-MinLatency")
    , MaxLatency(aName + "-MaxLatency")
    , Latency_Histogram(aName + "-Latency:Histogram")
  {
   min=max=0L;
  }
 
 void updateMinLatency(int64_t delay){
   int64_t oldmin=min;
   if(min==0) min=delay;
   else if (delay<min) min=delay;
   MinLatency-=oldmin;
   MinLatency+=min;
  } 

 void updateMaxLatency(int64_t delay){
   int64_t oldmax=max;
   if(max==0) max=delay;
   else if (delay>max) max=delay;
   MaxLatency-=oldmax;
   MaxLatency+=max;
  }

}; // struct MemStats

}; // namespace DRAMSim

#endif // ! __MEM_STATS_HPP__
