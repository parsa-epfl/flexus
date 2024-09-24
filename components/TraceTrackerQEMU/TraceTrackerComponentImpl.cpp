#include <components/TraceTrackerQEMU/TraceTrackerComponent.hpp>

#define DBG_DeclareCategories TraceTrack
#define DBG_SetDefaultOps     AddCat(TraceTrack)
#include DBG_Control()

#include <core/performance/profile.hpp>
#include <core/stats.hpp>

#define FLEXUS_BEGIN_COMPONENT TraceTrackerComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nTraceTrackerComponent {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(TraceTrackerComponent)
{
    FLEXUS_COMPONENT_IMPL(TraceTrackerComponent);

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(TraceTrackerComponent)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const { return true; }

    void initialize() { theTraceTracker.initialize(); }

    void finalize() { theTraceTracker.finalize(); }
};

} // end namespace nTraceTrackerComponent

FLEXUS_COMPONENT_INSTANTIATOR(TraceTrackerComponent, nTraceTrackerComponent);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TraceTrackerComponent

#define DBG_Reset
#include DBG_Control()
