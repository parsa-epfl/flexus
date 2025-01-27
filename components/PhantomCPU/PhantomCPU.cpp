#include "core/component.hpp"
#include "core/flexus.hpp"
#include "core/types.hpp"
#include <components/PhantomCPU/PhantomCPU.hpp>

#define FLEXUS_BEGIN_COMPONENT PhantomCPU
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "core/qemu/configuration_api.hpp"

#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/MTManager/MTManager.hpp>
#include <core/debug/debug.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>

namespace Stat = Flexus::Stat;

#define DBG_DefineCategories PhantomCPU
#define DBG_SetDefaultOps    AddCat(PhantomCPU)
#include DBG_Control()

namespace nphatromCPU {

class FLEXUS_COMPONENT(PhantomCPU)
{
    FLEXUS_COMPONENT_IMPL(PhantomCPU);

private:
    Flexus::Qemu::Processor theCPU;
    uint64_t theCPUIndex;

    Stat::StatCounter theCommitCount;

public:
    FLEXUS_COMPONENT_CONSTRUCTOR(PhantomCPU)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS),
        theCPU(),
        theCPUIndex(0),
        theCommitCount(std::string("Phantom-") + std::to_string(flexusIndex()) + std::string("-CommitCount"))
    {
    }

    bool isQuiesced() const { return true; }

    void initialize() {
        uint64_t cpu_index = flexusIndex() + Flexus::Core::ComponentManager::getComponentManager().systemWidth();

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
            if(return_value == 0x10003){ // QEMU_EXCP_HALTED
                DBG_(Crit, (<< "CPU " << theCPUIndex << " has halted."));
            } else {
                ++theCommitCount;
            }
        }
    }
};

} // End namespace nPhantomCPU

FLEXUS_COMPONENT_INSTANTIATOR(PhantomCPU, nphatromCPU);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PhantomCPU

#define DBG_Reset
#include DBG_Control()
