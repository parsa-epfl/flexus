//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <components/SpatialPrefetcher/SpatialPrefetcher.hpp>

#include <memory>
#include <zlib.h>

#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

#include <components/CommonQEMU/TraceTracker.hpp>

#include <core/performance/profile.hpp>
#include <core/stats.hpp>

#define FLEXUS_BEGIN_COMPONENT SpatialPrefetcher
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nSpatialPrefetcher {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

using boost::intrusive_ptr;

typedef uint32_t MemoryAddress;

class FLEXUS_COMPONENT(SpatialPrefetcher) {
  FLEXUS_COMPONENT_IMPL(SpatialPrefetcher);

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(SpatialPrefetcher) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

public:
  void initialize() {
    if (cfg.UsageEnable || cfg.RepetEnable || cfg.PrefetchEnable) {
      theTraceTracker.initSGP(
          flexusIndex(), cfg.CacheLevel, cfg.UsageEnable, cfg.RepetEnable, cfg.BufFetchEnable,
          cfg.TimeRepetEnable, cfg.PrefetchEnable, cfg.ActiveEnable, cfg.OrderEnable,
          cfg.StreamEnable, cfg.BlockSize, cfg.SgpBlocks, cfg.RepetType, cfg.RepetFills,
          cfg.SparseOpt, cfg.PhtSize, cfg.PhtAssoc, cfg.PcBits, cfg.CptType, cfg.CptSize,
          cfg.CptAssoc, cfg.CptSparse, cfg.FetchDist, cfg.SgpBlocks /*window*/, cfg.StreamDense,
          true /*sendStreams*/, cfg.BufSize, cfg.StreamDescs, cfg.DelayedCommits, cfg.CptFilter);
    }
    if (!cfg.PrefetchEnable) {
      theTraceTracker.initOffChipTracking(flexusIndex());
    }
  }

  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const &aDirName) {
    if (cfg.RepetEnable) {
      theTraceTracker.saveSGP(flexusIndex(), cfg.CacheLevel, aDirName);
    }
  }

  void loadState(std::string const &aDirName) {
    if (cfg.RepetEnable) {
      theTraceTracker.loadSGP(flexusIndex(), cfg.CacheLevel, aDirName);
    } else {
      DBG_(Dev, (<< "Warning: SGP state not loaded because Repet not enabled"));
    }
  }

public:
public:
  // The PrefetchDrive drive checks if it should issue a prefetch this cycle.
  void drive(interface::PrefetchDrive const &) {
    // Implementation is in the tryPrefetch() member below
    if (cfg.PrefetchEnable == true) {
      while (theTraceTracker.prefetchReady(flexusIndex(), cfg.CacheLevel)) {
        if (FLEXUS_CHANNEL(PrefetchOut_1).available() &&
            FLEXUS_CHANNEL(PrefetchOut_2).available()) {
          PrefetchTransport transport;
          transport.set(PrefetchCommandTag,
                        theTraceTracker.getPrefetchCommand(flexusIndex(), cfg.CacheLevel));
          DBG_(Trace, Comp(*this)(<< "issuing prefetch: " << *transport[PrefetchCommandTag]));
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

FLEXUS_COMPONENT_INSTANTIATOR(SpatialPrefetcher, nSpatialPrefetcher);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SpatialPrefetcher

#define DBG_Reset
#include DBG_Control()
