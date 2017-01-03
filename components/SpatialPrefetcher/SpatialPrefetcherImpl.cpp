#include <components/SpatialPrefetcher/SpatialPrefetcher.hpp>

#include <memory>
#include <zlib.h>

#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

#include <components/CommonQEMU/TraceTracker.hpp>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>

#define FLEXUS_BEGIN_COMPONENT SpatialPrefetcher
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nSpatialPrefetcher {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

using boost::intrusive_ptr;

typedef uint32_t MemoryAddress;

class FLEXUS_COMPONENT(SpatialPrefetcher) {
  FLEXUS_COMPONENT_IMPL( SpatialPrefetcher );

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(SpatialPrefetcher)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

public:
  void initialize() {
    if (cfg.UsageEnable || cfg.RepetEnable || cfg.PrefetchEnable) {
      theTraceTracker.initSGP( flexusIndex(), cfg.CacheLevel,
                               cfg.UsageEnable, cfg.RepetEnable, cfg.BufFetchEnable,
                               cfg.TimeRepetEnable, cfg.PrefetchEnable, cfg.ActiveEnable,
                               cfg.OrderEnable, cfg.StreamEnable,
                               cfg.BlockSize, cfg.SgpBlocks, cfg.RepetType, cfg.RepetFills,
                               cfg.SparseOpt, cfg.PhtSize, cfg.PhtAssoc, cfg.PcBits,
                               cfg.CptType, cfg.CptSize, cfg.CptAssoc, cfg.CptSparse,
                               cfg.FetchDist, cfg.SgpBlocks /*window*/, cfg.StreamDense, true /*sendStreams*/,
                               cfg.BufSize, cfg.StreamDescs, cfg.DelayedCommits, cfg.CptFilter);
    }
    if (!cfg.PrefetchEnable) {
      theTraceTracker.initOffChipTracking(flexusIndex());
    }
  }

  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const & aDirName) {
    if (cfg.RepetEnable) {
      theTraceTracker.saveSGP(flexusIndex(), cfg.CacheLevel, aDirName);
    }
  }

  void loadState(std::string const & aDirName) {
    if (cfg.RepetEnable) {
      theTraceTracker.loadSGP(flexusIndex(), cfg.CacheLevel, aDirName);
    } else {
      DBG_(Dev, ( << "Warning: SGP state not loaded because Repet not enabled" ) );
    }
  }

public:

public:
  //The PrefetchDrive drive checks if it should issue a prefetch this cycle.
  void drive(interface::PrefetchDrive const &) {
    //Implementation is in the tryPrefetch() member below
    if (cfg.PrefetchEnable == true) {
      while (theTraceTracker.prefetchReady(flexusIndex(), cfg.CacheLevel)) {
        if (FLEXUS_CHANNEL(PrefetchOut_1).available() && FLEXUS_CHANNEL(PrefetchOut_2).available()) {
          PrefetchTransport transport;
          transport.set( PrefetchCommandTag, theTraceTracker.getPrefetchCommand(flexusIndex(), cfg.CacheLevel));
          DBG_(Trace, Comp(*this)  ( << "issuing prefetch: " << *transport[PrefetchCommandTag]) );
          FLEXUS_CHANNEL(PrefetchOut_1) << transport;
          FLEXUS_CHANNEL(PrefetchOut_2) << transport;
        } else {
          // port not available - don't loop forever
          break;
        }
      }
    }
  }

};

} // end namespace nSpatialPrefetcher

FLEXUS_COMPONENT_INSTANTIATOR( SpatialPrefetcher,  nSpatialPrefetcher);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SpatialPrefetcher

#define DBG_Reset
#include DBG_Control()
