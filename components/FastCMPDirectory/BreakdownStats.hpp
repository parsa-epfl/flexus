#ifndef FLEXUS_FASTCMPDIRECTORY_BREAKDOWNSTATS_HPP_INCLUDED
#define FLEXUS_FASTCMPDIRECTORY_BREAKDOWNSTATS_HPP_INCLUDED

#include <components/Common/Slices/MemoryMessage.hpp>
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

  enum SharingDimension {
    NonShared = 0,
    SharedRO,
    SharedRW,
    NumSharingDimensions
  };

  enum ChipDimension {
    OnChip = 0,
    OffChip,
    NumChipDimensions
  };

  enum HitDimension {
    Miss = 0,
    HitMatch,
    HitMore,
    HitFewer,
    NumHitDimensions
  };

  Stat::StatCounter * theBlockStats[NumRequestDimensions][NumSharingDimensions][NumChipDimensions][NumHitDimensions];
  Stat::StatCounter * theRegionStats[NumRequestDimensions][NumSharingDimensions][NumChipDimensions][NumHitDimensions];

  BreakdownStats(std::string const & theName) {
    static const char * request_dimension_strings[NumRequestDimensions] = { "Read", "Fetch", "Upgrade", "Write", "Evict" };
    static const char * sharing_dimension_strings[NumSharingDimensions] = { "NonShared", "Shared:RdOnly", "Shared:RdWr" };
    static const char * chip_dimension_strings[NumChipDimensions] = { "OnChip", "OffChip" };
    static const char * hit_dimension_strings[NumHitDimensions] = { "Miss", "Hit:Match", "Hit:More", "Hit:Fewer" };

    for (RequestDimension r = ReadRequest; r < NumRequestDimensions; r = (RequestDimension)((int)r + 1) ) {
      for (SharingDimension s = NonShared; s < NumSharingDimensions; s = (SharingDimension)((int)s + 1) ) {
        for (ChipDimension c = OnChip; c < NumChipDimensions; c = (ChipDimension)((int)c + 1) ) {
          for (HitDimension h = Miss; h < NumHitDimensions; h = (HitDimension)((int)h + 1) ) {
            theBlockStats[(int)r][(int)s][(int)c][(int)h] = new Stat::StatCounter(theName + "-Block:" + request_dimension_strings[r]
                + ":" + chip_dimension_strings[c]
                + ":" + sharing_dimension_strings[s]
                + ":" + hit_dimension_strings[h]);
            theRegionStats[(int)r][(int)s][(int)c][(int)h] = new Stat::StatCounter(theName + "-Region:" + request_dimension_strings[r]
                + ":" + chip_dimension_strings[c]
                + ":" + sharing_dimension_strings[s]
                + ":" + hit_dimension_strings[h]);
          }
        }
      }
    }
  }

  void incrementStat(MemoryMessage::MemoryMessageType req_type, bool off_chip, SharingDimension s, HitDimension bh, HitDimension rh) {
    RequestDimension r = NumRequestDimensions;
    switch (req_type) {
      case MemoryMessage::ReadReq:
        r = ReadRequest;
        break;
      case MemoryMessage::FetchReq:
        r = FetchRequest;
        break;
      case MemoryMessage::WriteReq:
        r = WriteRequest;
        break;
      case MemoryMessage::UpgradeReq:
        r = UpgradeRequest;
        break;
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictDirty:
        r = EvictRequest;
        break;
      default:
        DBG_Assert(false, ( << "Unknown Request Type" << req_type ));
        break;
    }

    ChipDimension c = (off_chip ? OffChip : OnChip );

    (*(theBlockStats[r][s][c][bh]))++;
    (*(theRegionStats[r][s][c][rh]))++;
  }

  void update() {
  }
};

}  // namespace nFastCMPDirectory

#endif /* FLEXUS_FASTCMPDIRECTORY_BREAKDOWNSTATS_HPP_INCLUDED */

