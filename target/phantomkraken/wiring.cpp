#include "core/component.hpp"
#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>

// This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
// Must NOT contain "SemiKraken": ComponentManager halves systemWidth only for SemiKraken, so this
// name keeps systemWidth == ncores, making every QEMU core a phantom core (PhantomCPUFull, offset 0).
std::string theSimulatorName = "PhantomKraken v1.0";
}

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/PhantomCPUFull/PhantomCPUFull.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION(PhantomCPUFull, "phantom-cpu", thePhantomCfg);

bool initializeParameters() {
  DBG_(Dev, (<< " initializing Parameters..."));

  thePhantomCfg.EstimatedIPC.initialize(5);

  theFlexus->setStatInterval(100000);

  return true; // true = Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()

// clang-format off
#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
// Every core is phantom: one PhantomCPUFull per QEMU core, no real uArch/cache components.
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( PhantomCPUFull, thePhantomCfg, thePhantomCPU, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

// No wiring: PhantomCPUFull has no request/reply channels, only its drive port.

#include FLEXUS_END_COMPONENT_WIRING_SECTION()

#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

// Two groups: core drives (driven per-core) then uncore drives (driven once). drive.hpp derefs both
// the first AND second inner vector, so the uncore group must exist — empty here (no uncore components).
mpl::vector <
DRIVE ( thePhantomCPU, PhantomDrive ) >
, mpl::vector < >

#include FLEXUS_END_DRIVE_ORDER_SECTION()
    // clang-format on
