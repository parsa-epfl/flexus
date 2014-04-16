#ifndef __CACHE_STATS_HPP__
#define __CACHE_STATS_HPP__

namespace nCMPCache {

struct CacheStats {

  Flexus::Stat::StatCounter WriteHit;
  Flexus::Stat::StatCounter WriteMissInvalidatesOnly;
  Flexus::Stat::StatCounter WriteMissInvalidatesAndData;
  Flexus::Stat::StatCounter WriteMissMemory;
  Flexus::Stat::StatCounter ReadHit;
  Flexus::Stat::StatCounter ReadMissMemory;
  Flexus::Stat::StatCounter ReadMissPeer;
  Flexus::Stat::StatCounter FetchHit;
  Flexus::Stat::StatCounter FetchMissPeer;
  Flexus::Stat::StatCounter FetchMissMemory;
  Flexus::Stat::StatCounter UpgradeHit;
  Flexus::Stat::StatCounter UpgradeMiss;
  Flexus::Stat::StatCounter NASHit;
  Flexus::Stat::StatCounter NASMissInvalidatesAndData;
  Flexus::Stat::StatCounter NASMissMemory;
  Flexus::Stat::StatCounter	EvictClean;
  Flexus::Stat::StatCounter	EvictWritableWrite;
  Flexus::Stat::StatCounter	EvictWritableBypass;
  Flexus::Stat::StatCounter	EvictDirtyWrite;
  Flexus::Stat::StatCounter	EvictDirtyBypass;

  CacheStats(const std::string & aName) : WriteHit(aName + "-WriteHit")
    , WriteMissInvalidatesOnly(aName + "-WriteMissInvalidatesOnly")
    , WriteMissInvalidatesAndData(aName + "-WriteMissInvalidatesAndData")
    , WriteMissMemory(aName + "-WriteMissMemory")
    , ReadHit(aName + "-ReadHit")
    , ReadMissMemory(aName + "-ReadMissMemory")
    , ReadMissPeer(aName + "-ReadMissPeer")
    , FetchHit(aName + "-FetchHit")
    , FetchMissPeer(aName + "-FetchMissPeer")
    , FetchMissMemory(aName + "-FetchMissMemory")
    , UpgradeHit(aName + "-UpgradeHit")
    , UpgradeMiss(aName + "-UpgradeMiss")
    , NASHit(aName + "-NASHit")
    , NASMissInvalidatesAndData(aName + "-NASMissInvalidatesAndData")
    , NASMissMemory(aName + "-NASMissMemory")
    , EvictClean(aName + "-EvictClean")
    , EvictWritableWrite(aName + "-EvictWritableWrite")
    , EvictWritableBypass(aName + "-EvictWritableBypass")
    , EvictDirtyWrite(aName + "-EvictDirtyWrite")
    , EvictDirtyBypass(aName + "-EvictDirtyBypass")
  {}

}; // struct CacheStats

}; // namespace nCMPCache

#endif // ! __CACHE_STATS_HPP__
