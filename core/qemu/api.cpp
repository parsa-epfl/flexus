#include <cassert>
#include <core/flexus.hpp>

namespace Flexus {
namespace Qemu {
namespace API {

#include "api.h"

QEMU_API_t qemu_api;

void
FLEXUS_get_api(FLEXUS_API_t* api)
{
    api->start     = FLEXUS_start;
    api->stop      = FLEXUS_stop;
    api->qmp       = FLEXUS_qmp;
    api->trace_mem = FLEXUS_trace_mem;
}

using namespace Flexus::Core;

void
FLEXUS_start(uint64_t cycle)
{
    theFlexus->setCycle(cycle);

    while (true)
        theFlexus->doCycle();
}

void
FLEXUS_stop()
{
    theFlexus->terminateSimulation();
}

void
FLEXUS_qmp(qmp_flexus_cmd_t aCMD, const char* anArgs)
{
    // qmp_api.hpp is bad
    flexus_qmp(aCMD, anArgs);
}

void __attribute__((weak))
FLEXUS_trace_mem(uint64_t idx, memory_transaction_t* tr)
{
    // not exposed in timing
}

} // namespace API
} // namespace Qemu
} // namespace Flexus