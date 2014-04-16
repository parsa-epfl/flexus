#ifndef FLEXUS_FASTCMPCACHE_DIRECTORYSTATS_HPP_INCLUDED
#define FLEXUS_FASTCMPCACHE_DIRECTORYSTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nFastCMPCache {

namespace Stat = Flexus::Stat;

struct DirectoryStats {
  Stat::StatCounter theReadsOnChip;
  Stat::StatCounter theFetchesOnChip;
  Stat::StatCounter theWritesOnChip;
  Stat::StatCounter theUpgradesOnChip;
  Stat::StatCounter theReadsOffChip;
  Stat::StatCounter theFetchesOffChip;
  Stat::StatCounter theWritesOffChip;

  Stat::StatInstanceCounter<int64_t> theReadsOffChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theReadsOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theReadsOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theReadsOnChip_ASharers;
  Stat::StatInstanceCounter<int64_t> theWritesOffChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theWritesOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theWritesOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theWritesOnChip_ASharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOffChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOnChip_ASharers;
  Stat::StatInstanceCounter<int64_t> theUpgradesOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theUpgradesOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theUpgradesOnChip_ASharers;

  Stat::StatCounter theReadsOnChipLatency;
  Stat::StatCounter theWritesOnChipLatency;
  Stat::StatCounter theFetchesOnChipLatency;
  Stat::StatCounter theUpgradesOnChipLatency;
  Stat::StatCounter theOtherOnChipLatency;
  Stat::StatCounter theReadsOffChipLatency;
  Stat::StatCounter theWritesOffChipLatency;
  Stat::StatCounter theFetchesOffChipLatency;
  Stat::StatCounter theOtherOffChipLatency;

  DirectoryStats(std::string const & theName)
    : theReadsOnChip(theName + "-Reads:OnChip")
    , theFetchesOnChip(theName + "-Fetches:OnChip")
    , theWritesOnChip(theName + "-Writes:OnChip")
    , theUpgradesOnChip(theName + "-Upgrades:OnChip")
    , theReadsOffChip(theName + "-Reads:OffChip")
    , theFetchesOffChip(theName + "-Fetches:OffChip")
    , theWritesOffChip(theName + "-Writes:OffChip")

    , theReadsOffChip_PSharers(theName + "-Reads:Off:PotentialSharers")
    , theReadsOnChip_PSharers(theName + "-Reads:On:PotentialSharers")
    , theReadsOnChip_FSharers(theName + "-Reads:On:WrongSharers")
    , theReadsOnChip_ASharers(theName + "-Reads:On:AdditionalSharers")
    , theWritesOffChip_PSharers(theName + "-Writes:Off:PotentialSharers")
    , theWritesOnChip_PSharers(theName + "-Writes:On:PotentialSharers")
    , theWritesOnChip_FSharers(theName + "-Writes:On:WrongSharers")
    , theWritesOnChip_ASharers(theName + "-Writes:On:AdditionalSharers")
    , theFetchesOffChip_PSharers(theName + "-Fetches:Off:PotentialSharers")
    , theFetchesOnChip_PSharers(theName + "-Fetches:On:PotentialSharers")
    , theFetchesOnChip_FSharers(theName + "-Fetches:On:WrongSharers")
    , theFetchesOnChip_ASharers(theName + "-Fetches:On:AdditionalSharers")
    , theUpgradesOnChip_PSharers(theName + "-Upgrades:On:PotentialSharers")
    , theUpgradesOnChip_FSharers(theName + "-Upgrades:On:WrongSharers")
    , theUpgradesOnChip_ASharers(theName + "-Upgrades:On:AdditionalSharers")

    , theReadsOnChipLatency(theName + "Reads:On:Latency")
    , theWritesOnChipLatency(theName + "Writes:On:Latency")
    , theFetchesOnChipLatency(theName + "Fetches:On:Latency")
    , theUpgradesOnChipLatency(theName + "Upgrades:On:Latency")
    , theOtherOnChipLatency(theName + "Other:On:Latency")
    , theReadsOffChipLatency(theName + "Reads:Off:Latency")
    , theWritesOffChipLatency(theName + "Writes:Off:Latency")
    , theFetchesOffChipLatency(theName + "Fetches:Off:Latency")
    , theOtherOffChipLatency(theName + "Other:Off:Latency") {
  }

  void update() {
  }
};

}  // namespace nFastCMPCache

#endif /* FLEXUS_FASTCMPCACHE_DIRECTORYSTATS_HPP_INCLUDED */

