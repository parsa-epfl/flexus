#include "core/component.hpp"
#include "core/flexus.hpp"
#include "core/types.hpp"
#include <components/PhantomCPUFull/PhantomCPUFull.hpp>

#define FLEXUS_BEGIN_COMPONENT PhantomCPUFull
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "core/qemu/configuration_api.hpp"

#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/MTManager/MTManager.hpp>
#include <core/debug/debug.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>

namespace Stat = Flexus::Stat;

#define DBG_DefineCategories PhantomCPUFull
#define DBG_SetDefaultOps    AddCat(PhantomCPUFull)
#include DBG_Control()

namespace nPhantomCPUFull {

class FLEXUS_COMPONENT(PhantomCPUFull)
{
    FLEXUS_COMPONENT_IMPL(PhantomCPUFull);

private:
    Flexus::Qemu::Processor theCPU;
    uint64_t theCPUIndex;

    Stat::StatCounter theCommitCount;

public:
    FLEXUS_COMPONENT_CONSTRUCTOR(PhantomCPUFull)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS),
        theCPU(),
        theCPUIndex(0),
        theCommitCount(std::string("Phantom-") + std::to_string(flexusIndex()) + std::string("-CommitCount"))
    {
    }

    bool isQuiesced() const { return true; }

    void initialize() {
        // All cores are phantom in this target — there are no real uArch cores below, so the
        // phantom core maps directly onto QEMU core flexusIndex() (offset 0). This is the only
        // difference from PhantomCPU, whose offset is systemWidth() (the real-core count).
        uint64_t cpu_index = flexusIndex();

        theCPU = Flexus::Qemu::Processor::getProcessor(cpu_index);
        theCPUIndex = cpu_index;
    }

    void finalize() {}

public:
    void drive(interface::PhantomDrive const&) { doCycle(); }

private:
    void doCycle() {
        // 1. advance the CPU by the estimated IPC.
        for(std::size_t i = 0; i < cfg.EstimatedIPC; i++) {
            uint64_t return_value = theCPU.advance();
            if(return_value != 0x10003){
                ++theCommitCount;
            }
        }
        // 2. Reset this core's watchdog. Normally the uArch does this on every advance (microArch.cpp);
        // this target has no uArch, so without it Flexus's per-core watchdog counts up unchecked and
        // aborts with "No progress by CPU" even though the phantom core is advancing at the estimated IPC.
        Flexus::Core::theFlexus->reset_core_watchdog((uint32_t)theCPUIndex);
    }
};

} // End namespace nPhantomCPUFull

FLEXUS_COMPONENT_INSTANTIATOR(PhantomCPUFull, nPhantomCPUFull);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PhantomCPUFull

#define DBG_Reset
#include DBG_Control()
