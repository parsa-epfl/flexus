#ifndef FLEXUS_TRACER_HPP
#define FLEXUS_TRACER_HPP

#include "TracerStat.hpp"
#include "components/CommonQEMU/Slices/MemoryMessage.hpp"

#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/configuration_api.hpp>
#include <cstddef>
#include <vector>

using namespace Flexus::Qemu::API;
using Flexus::SharedTypes::MemoryMessage;

namespace nDecoupledFeeder {

class Tracer
{

  private:
    std::size_t nb_cores;
    TracerStat** vCPUTracer;
    std::function<void(std::size_t, MemoryMessage&)> callback;

  public:
    static Tracer* instance; // Static pointer to the singleton instance

  private:
    Tracer(std::size_t, std::function<void(std::size_t, MemoryMessage&)>);

  public:
    /**
     * Singleton-ish function to return the instance if wasn't yet created before
     **/
    static Tracer* getInstance(std::size_t nb_core, std::function<void(std::size_t, MemoryMessage&)> callback);
    // Destgructor
    ~Tracer();
    /**
     * Return Tracer associated with a given core
     *
     *  Bryan Perdrizat
     *      This could have been an operator[] overloading
     *      but also this is time to stop with this madness if not necessary
     **/
    TracerStat* core(std::size_t index);
    /**
     * Create a MemoryMessage and dispatch the incoming transaction
     * to populate the MemoryMessage
     **/
    void trace(uint64_t idx, memory_transaction_t& tr);
    /**
     * For all tracer, update their internal statistics
     **/
    void update_collector(void);
};

};
#endif