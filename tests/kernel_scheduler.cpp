// Run: c++ -std=c++17 -I flexus flexus/tests/kernel_scheduler.cpp -o /tmp/kernel_scheduler_test && /tmp/kernel_scheduler_test
#include <core/kernel_scheduler.hpp>
#include <cassert>
#include <iostream>

using Flexus::Core::KernelScheduler;
using Result = Flexus::Core::KernelServiceResult;

int main()
{
    // An ordinary syscall completes without a timed drive or tick.
    {
        KernelScheduler scheduler;
        int calls = 0;
        scheduler.add([&]() { ++calls; return Result::Ready; });
        assert(scheduler.service());
        assert(calls == 1);
    }

    // The waiter is visited before its lock holder. Both complete within the
    // same service phase, without needing an intervening timed cycle.
    {
        KernelScheduler scheduler;
        bool locked = true;
        bool waiterDone = false;
        int waiterCalls = 0;
        scheduler.add([&]() {
            ++waiterCalls;
            if (locked) { return Result::Yielded; }
            waiterDone = true;
            return Result::Ready;
        });
        scheduler.add([&]() { locked = false; return Result::Ready; });
        assert(scheduler.service());
        assert(waiterDone && waiterCalls == 2);
    }

    // A long kernel segment split into instruction quanta remains zero-time.
    {
        KernelScheduler scheduler;
        int slices = 0;
        scheduler.add([&]() { return ++slices < 5 ? Result::Yielded : Result::Ready; });
        assert(scheduler.service());
        assert(slices == 5);
    }

    // WFI is not polled repeatedly in the zero-time phase; a timed drive must
    // poll for the interrupt. Inactive and not-runnable cores also do not spin.
    for (auto state : {Result::Halted, Result::Inactive, Result::NotRunnable}) {
        KernelScheduler scheduler;
        int calls = 0;
        scheduler.add([&]() { ++calls; return state; });
        assert(scheduler.service());
        assert(calls == 1);
    }

    // A lock requiring future user execution cannot freeze the clock. Once a
    // timed cycle releases it, the next zero-time phase completes normally.
    {
        KernelScheduler scheduler;
        bool locked = true;
        std::size_t calls = 0;
        scheduler.add([&]() { ++calls; return locked ? Result::Yielded : Result::Ready; });
        assert(!scheduler.service());
        assert(calls == KernelScheduler::MaxPasses);
        locked = false;
        assert(scheduler.service());
        assert(calls == KernelScheduler::MaxPasses + 1);
    }
    std::cout << "kernel scheduler tests passed\n";
}
