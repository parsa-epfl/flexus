#ifndef FLEXUS_BOOKSHIM_BOOKSIMSTATS_HPP_INCLUDED
#define FLEXUS_BOOKSHIM_BOOKSIMSTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nNetwork {

namespace Stat = Flexus::Stat;

struct BooksimStats {
  std::string theInitialName;
  Stat::StatCounter HopStats_SumSample;
  Stat::StatCounter HopStats_NoSample;
  Stat::StatCounter LatencyStats_SumSample;
  Stat::StatCounter LatencyStats_NoSample;
  Stat::StatCounter **PairLatency_SumSample;
  Stat::StatCounter **PairLatency_NoSample;
  Stat::StatCounter **AcceptedPackets_SumSample;
  Stat::StatCounter **AcceptedPackets_NoSample;

//  Stat::StatInstanceCounter<int64_t> Sharers_Histogram_Accesses;

  BooksimStats(std::string const & theName)
    : HopStats_SumSample(theName + "-Booksim:HopStats:SumSample")
    , HopStats_NoSample(theName + "-Booksim:HopStats:NoSample")
    , LatencyStats_SumSample(theName + "-Booksim:LatencyStats:SumSample")
    , LatencyStats_NoSample(theName + "-Booksim:LatencyStats:NoSample")
  {
    theInitialName = theName;
  }

  void update() {
  }
};

}  // namespace nNetwork

#endif /* FLEXUS_BOOKSHIM_BOOKSIMSTATS_HPP_INCLUDED */

