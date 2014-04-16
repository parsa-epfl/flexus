#ifndef FLEXUS_FASTCMPDIRECTORY_BREAKDOWNSTATS_HPP_INCLUDED
#define FLEXUS_FASTCMPDIRECTORY_BREAKDOWNSTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nFastCMPDirectory {

namespace Stat = Flexus::Stat;

struct BreakdownStats {

  enum RequestDimension {
    ReadRequest = 0,
    FetchRequest,
    UpgradeRequest,
    WriteRequest,
    EvictRequest,
    NumRequestDimensions
  };

  static const char * request_dimension_strings[] = { "Read", "Fetch", "Upgrade", "Write", "Evict" };

  enum SharingDimension {
    NonShared = 0,
    SharedRO,
    SharedRW,
    NumSharingDimensions
  };

  static const char * sharing_dimension_strings[] = { "NonShared", "Shared:RdOnly", "Shared:RdWr" };

  enum ChipDimension {
    OnChip = 0,
    OffChip,
    NumChipDimensions
  };

  static const char * chip_dimension_strings[] = { "OnChip", "OffChip" };

  enum HitDimension {
    Miss = 0,
    HitMatch,
    HitMore,
    HitFewer,
    NumHitDimensions
  };

  static const char * hit_dimension_strings[] = { "Miss", "Hit:Match", "Hit:More", "Hit:Fewer" };

  Stat::StatCounter * theBlockStats[NumRequestDimensions][NumSharingDimensions][NumChipDimensions][NumHitDimensions];
  Stat::StatCounter * theRegionStats[NumRequestDimensions][NumSharingDimensions][NumChipDimensions][NumHitDimensions];

  BreakdownStats(std::string const & theName) {
    for (RequestDimension r = 0; r < NumRequestDimensions; r++) {
      for (SharingDimension s = 0; s < NumSharingDimensions; s++) {
        for (ChipDimension c = 0; c < NumChipDimensions; c++) {
          for (HitDimension h = 0; h < NumHitDimensions; h++) {
            theBlockStats[r][s][c][h] = new Stat::StatCounter(theName + "-Block:" + request_dimension_strings[r]
                + ":" + chip_dimension_strings[c]
                + ":" + sharing_dimension_strings[s]
                + ":" + hit_dimension_strings[h]);
            theRegionStats[r][s][c][h] = new Stat::StatCounter(theName + "-Region:" + request_dimension_strings[r]
                + ":" + chip_dimension_strings[c]
                + ":" + sharing_dimension_strings[s]
                + ":" + hit_dimension_strings[h]);
          }
        }
      }
    }
  }

  void incrementStat(MemoryMessage::MemoryMessageType req_type, bool off_chip, SharingDimension s, HitDimension bh, HitDimension rh) {
    RequestDimension r;
    switch (req_type) {
MemoryMessage::ReadReq:
        r = ReadRequest;
        break;
MemoryMessage::FetchReq:
        r = FetchRequest;
        break;
MemoryMessage::WriteReq:
        r = WriteRequest;
        break;
MemoryMessage::UpgradeReq:
        r = UpgradeRequest;
        break;
MemoryMessage::EvictClean:
MemoryMessage::EvictWritable:
MemoryMessage::EvictDirty:
        r = EvictRequest;
        break;
      default:
        DBG_Assert(false, ( << "Unknown Request Type" << req_type ));
        break;
    }

    ChipDimension c = (off_chip ? OffChip : OnChip );

    (*(theBlockStats[r][s][c][bh]))++;
    (*(theRegionStats[r][s][c][hh]))++;
  }

  void update() {
  }
};

}  // namespace nFastCMPDirectory

#endif /* FLEXUS_FASTCMPDIRECTORY_BREAKDOWNSTATS_HPP_INCLUDED */

