
#include "Tracer.hpp"

#include "TracerDispatcher.hpp"

using namespace Flexus::Qemu;
using namespace Flexus::Qemu::API;
using namespace nDecoupledFeeder;

Tracer* Tracer::instance = nullptr;
/**
 * INFO: Something a bit stupid happen here
 *
 *  Bryan Perdrizat:
 *      At first, I wanted to push the TracerStat object onto an std::vector
 *      but, because of the nature of the stupid mechanism (@see FLEXUS_trace_mem)
 *      this somehow trigger the disalocation of the vector therefore of the
 *      TracerStat objects. Then, I've then replicated the previous behaviour -> using pointer.
 *
 **/
Tracer::Tracer(std::size_t nb_cores, std::function<void(std::size_t, MemoryMessage&)> callback)
  : nb_cores(nb_cores)
  , callback(callback)
{
    vCPUTracer = new TracerStat*[nb_cores];
    for (std::size_t index{ 0 }; index < nb_cores; index++) {
        vCPUTracer[index] = new TracerStat(index);
    }
}

Tracer::~Tracer()
{
    delete[] vCPUTracer;
    delete instance;
}

Tracer*
Tracer::getInstance(std::size_t nb_cores, std::function<void(std::size_t, MemoryMessage&)> callback)
{
    if (!instance) instance = new Tracer(nb_cores, callback);
    return instance;
}

void
Tracer::trace(uint64_t idx, memory_transaction_t& tr)
{
    MemoryMessage msg(MemoryMessage::NumMemoryMessageTypes);
    msg.coreIdx() = idx;

    TracerDispatcher::dispatch(*vCPUTracer[idx], tr, msg);

    // if (tr.io) return;

    callback(idx, msg);
}

TracerStat*
Tracer::core(std::size_t index)
{
    DBG_Assert(index < nb_cores);
    return vCPUTracer[index];
}

void
Tracer::update_collector()
{
    for (std::size_t idx{ 0 }; idx < nb_cores; idx++) {
        vCPUTracer[idx]->update_collector();
    }
}
/**
 * QEMU Entrypoint
 * Instruction from the TCG, (that QEMU generate) will be pushed
 * here by the tracing plugin.
 *
 *  Bryan Perdrizat
 *      Because of the cluckiness of the mechanism, i add to resort
 *      to a mad singleton-ish type of structure. We need to keep
 *      a static var holding the instance for this function to access
 *      the tracing system. A bit stupid if you ask me.
 **/
void
Flexus::Qemu::API::FLEXUS_trace_mem(uint64_t idx, memory_transaction_t* tr)
{
    DBG_Assert(Tracer::instance != nullptr);
    Tracer::instance->trace(idx, *tr);
}