//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef FLEXUS_QEMU_MAI_API_HPP_INCLUDED
#define FLEXUS_QEMU_MAI_API_HPP_INCLUDED

#include <bitset>
#include <core/flexus.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/target.hpp>
#include <core/types.hpp>


using namespace Flexus::Core;
namespace Flexus {
namespace Qemu {

// void onInterrupt(void *aPtr, void *anObj, long long aVector);

using Flexus::SharedTypes::PhysicalMemoryAddress;
using Flexus::SharedTypes::VirtualMemoryAddress;

class Processor
{


private:
    std::size_t core_index;

    Processor(std::size_t core_index): core_index(core_index)
    {}


public:

    Processor(): core_index(0)
    {}

    uint64_t
    read_register(API::register_type_t reg, std::size_t index = 0xFF)
    {
        return API::qemu_api.read_register(core_index, reg, index);
    }

    // TODO ─── NOT implemented ────────────────────────────────────────────────


    std::string
    dump_state()
    {
        return "0xDEADBEEF";
    }


    bits readVirtualAddress(VirtualMemoryAddress anAddress, std::size_t size)
    {
        return bits(0);
    }

    bits readPhysicalAddress(PhysicalMemoryAddress anAddress, std::size_t aSize) const { return bits(0); }

    PhysicalMemoryAddress translateVirtualAddress(VirtualMemoryAddress addr) { return PhysicalMemoryAddress(); }

    uint32_t fetchInstruction(VirtualMemoryAddress addr) {return 0;}
    uint64_t readSCTLR(uint64_t index) { return 0;}

    uint64_t readPC() const { return 0;}
    VirtualMemoryAddress getPC() const { return VirtualMemoryAddress(); }

    uint64_t id() const { return core_index; }

    std::string disassemble(VirtualMemoryAddress const &anAddress) const { return "TODO()!"; }

    bool is_busy() const { return true; }

    uint64_t read_sysreg_from_qemu(uint32_t no) { return 0; }

     uint64_t readXRegister(size_t anIndex) const
    {
        return 0;
    }

    uint64_t readVRegister(int anIndex) const
    {
        return 0;
    }

    int advance(bool count_time = true) {return 0;}

    uint64_t getPendingInterrupt() const
    {
        return 0;
    }

    void breakSimulation() {}

    void readException(API::exception_t *exp) const {}
// explicit Processor(): base(0) {}

    // explicit Processor(API::conf_object_t* cpu_object) : base(PROCESSOR_IMPL(cpu_object)) {}

// operator API::conf_object_t *() {
//   return theImpl;
// }
// static Processor getProcessor(int aProcessorNumber) {
//   return Processor(API::qemu_api.get_cpu_by_idx(
//       /*ProcessorMapper::mapFlexusIndex2ProcNum(*/ aProcessorNumber));
// }
// static Processor getProcessor(std::string const &aProcessorName) {
//   return Processor(API::qemu_api.get_obj_by_name(aProcessorName.c_str()));
// }
// static Processor current() {
//   assert(false);
//   return getProcessor(0);
// }

    static Processor getProcessor(uint64_t core_index = 0)
    {
        return Processor(core_index);
    }

};

// #undef PROCESSOR_IMPL

} // end Namespace Qemu
} // end namespace Flexus

#endif  // FLEXUS_QEMU_MAI_API_HPP_INCLUDED