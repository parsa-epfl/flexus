#ifndef FLEXUS_CORE_KERNEL_SCHEDULER_HPP
#define FLEXUS_CORE_KERNEL_SCHEDULER_HPP

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace Flexus::Core {

enum class KernelServiceResult { Inactive, Ready, Halted, Yielded, NotRunnable };

// Services execute functional instructions only: no timed drives or clock ticks.
class KernelScheduler
{
  public:
    using Service = std::function<KernelServiceResult()>;
    static constexpr std::size_t MaxPasses = 64;

    void add(Service service) { services.push_back(std::move(service)); }

    // False means a persistent spinner exhausted the zero-time budget. The
    // caller must allow a timed cycle so user code and devices can progress.
    bool service()
    {
        for (std::size_t pass = 0; pass < MaxPasses; ++pass) {
            bool retry = false;
            for (auto& run : services) {
                auto result = run();
                retry |= result == KernelServiceResult::Yielded;
            }
            if (!retry) { return true; }
        }
        return false;
    }

  private:
    std::vector<Service> services;
};

} // namespace Flexus::Core

#endif
