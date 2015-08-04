#include <components/TraceTrackerQEMU/TraceTrackerComponent.hpp>

#define DBG_DeclareCategories TraceTrack
#define DBG_SetDefaultOps AddCat(TraceTrack)
#include DBG_Control()

#include <core/stats.hpp>
#include <core/performance/profile.hpp>

#define FLEXUS_BEGIN_COMPONENT TraceTrackerComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nTraceTrackerComponent {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(TraceTrackerComponent) {
  FLEXUS_COMPONENT_IMPL( TraceTrackerComponent );

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(TraceTrackerComponent)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return true;
  }

  void initialize() {
    theTraceTracker.initialize();
    /* CMU-ONLY-BLOCK-BEGIN */
    if (cfg.Sharing) {
      theTraceTracker.initSharing(cfg.NumNodes, cfg.BlockSize, cfg.SharingLevel);
    }
    /* CMU-ONLY-BLOCK-END */
  }

  void finalize() {
    theTraceTracker.finalize();
  }

};

} // end namespace nTraceTrackerComponent

FLEXUS_COMPONENT_INSTANTIATOR( TraceTrackerComponent,  nTraceTrackerComponent);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TraceTrackerComponent

#define DBG_Reset
#include DBG_Control()
