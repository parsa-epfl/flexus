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
    Stat::StatCounter theKernelSkipInsns;
    Stat::StatCounter theKernelSkipCalls;
    Stat::StatCounter theKernelSkipYieldExits;

public:
    FLEXUS_COMPONENT_CONSTRUCTOR(PhantomCPU)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS),
        theCPU(),
        theCPUIndex(0),
        theCommitCount(std::string("Phantom-") + std::to_string(flexusIndex()) + std::string("-CommitCount")),
        theKernelSkipInsns(std::string("Phantom-") + std::to_string(flexusIndex()) + std::string("-KernelSkip:Insns")),
        theKernelSkipCalls(std::string("Phantom-") + std::to_string(flexusIndex()) + std::string("-KernelSkip:Calls")),
        theKernelSkipYieldExits(std::string("Phantom-") + std::to_string(flexusIndex()) + std::string("-KernelSkip:YieldLimitExits"))
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
    // Fast-forward privileged code. Returns true if the core is back at EL0.
    // Note: like the regular phantom path, ASID changes are not reported.
    bool skipKernel() {
        uint64_t executed = 0;
        uint64_t rcode = theCPU.run_until_el0(cfg.KernelYieldLimit, cfg.KernelInsnLimit, &executed);
        ++theKernelSkipCalls;
        theKernelSkipInsns += executed;
        if (rcode == 0x10004) { ++theKernelSkipYieldExits; } // EXCP_YIELD: spinning, yield this cycle.
        return rcode == 0;
    }

    void doCycle() {
        if (!cfg.UserOnly) {
            // 1. advance the CPU by the estimated IPC.
            for(std::size_t i = 0; i < cfg.EstimatedIPC; i++) {
                uint64_t return_value = theCPU.advance();
                if(return_value != 0x10003){
                    ++theCommitCount;
                }
            }
            return;
        }

        // user_only: only EL0 instructions count towards the estimated IPC,
        // privileged code is fast-forwarded in zero cycles.
        for(std::size_t i = 0; i < cfg.EstimatedIPC; i++) {
            if (theCPU.current_el() != 0 && !skipKernel()) {
                // Halted, spinning, or not runnable: done for this cycle.
                return;
            }

            DBG_Assert(theCPU.current_el() == 0,
                       (<< "Phantom-" << flexusIndex() << " user_only: stepping while QEMU is not at EL0"));
            DBG_Assert((static_cast<uint64_t>(theCPU.get_pc()) >> 55) == 0,
                       (<< "Phantom-" << flexusIndex() << " user_only: stepping a kernel-space instruction at "
                        << theCPU.get_pc()));

            uint64_t return_value = theCPU.advance();
            if (return_value == 0x10003) {
                // Halted (cannot happen at EL0 in Linux, which traps WFI).
                return;
            }
            ++theCommitCount;
        }
    }
};

} // End namespace nPhantomCPU

FLEXUS_COMPONENT_INSTANTIATOR(PhantomCPU, nphatromCPU);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PhantomCPU

#define DBG_Reset
#include DBG_Control()
