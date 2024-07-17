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

    Processor(std::size_t core_index)
      : core_index(core_index)
    {
    }

  public:
    static Processor getProcessor(uint64_t core_index = 0) { return Processor(core_index); }

    Processor()
      : core_index(0)
    {
    }

    uint64_t read_register(API::register_type_t reg, std::size_t index = 0xFF)
    {
        return API::qemu_api.read_register(core_index, reg, index);
    }

    // Timing implemented
    //
    VirtualMemoryAddress get_pc() const { return VirtualMemoryAddress(API::qemu_api.get_pc(core_index)); }

    uint64_t has_irq() const { return API::qemu_api.has_irq(core_index); }

    uint64_t advance(bool count_time = true) { return API::qemu_api.cpu_exec(core_index, count_time); }

    PhysicalMemoryAddress translate_va2pa(VirtualMemoryAddress addr)
    {
        return PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, addr));
    }

    bits read_va(VirtualMemoryAddress anAddress, size_t size)
    {
        VirtualMemoryAddress finalAddress(((uint64_t)(anAddress) + size - 1) & ~0xFFF);
        if ((finalAddress & 0x1000) != (anAddress & 0x1000)) {
            DBG_Assert(false, (<< "Execution ended up somewhere Bryan did NOT expected it to go"));
        //    bits value1, value2;
        //    size_t partial = finalAddress - anAddress;
        //    value1         = read_pa(
        //      PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, API::logical_address_t(anAddress))),
        //      partial);
        //    value2 = read_pa(
        //      PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, API::logical_address_t(finalAddress))),
        //      size - partial);
        //    value2 = (value2 << (partial << 3)) | value1;
        //    return value2;
        }
        return read_pa(
          PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, API::logical_address_t(anAddress))),
          size);
    }

    uint32_t fetch_inst(VirtualMemoryAddress addr) { return static_cast<uint32_t>(read_va(addr, 4)); }

    bits read_pa(PhysicalMemoryAddress anAddress, size_t aSize) const
    {
        uint8_t buf[aSize] = { 0 };

        API::qemu_api.get_mem(buf, API::physical_address_t(anAddress), aSize);

        bits tmp = 0;
        for (size_t i = 0; i < aSize; i++) {
            const size_t s = i * 8;
            bits val       = (((bits)buf[i] << s) & ((bits)0xff << s));
            tmp |= val;
        }
        return tmp;
    }
    // TODO ─── NOT implemented ────────────────────────────────────────────────

    std::string dump_state() { return "0xDEADBEEF"; }

    uint64_t readSCTLR(uint64_t index) { return 0; }

    uint64_t readPC() const { return 0; }

    uint64_t id() const { return core_index; }

    std::string disassemble(VirtualMemoryAddress const& anAddress) const { return "TODO()!"; }

    bool is_busy() const { return true; }

    uint64_t read_sysreg_from_qemu(uint32_t no) { return 0; }

    void breakSimulation() {}

    void readException(API::exception_t* exp) const {}
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
};

// #undef PROCESSOR_IMPL

} // end Namespace Qemu
} // end namespace Flexus

#endif // FLEXUS_QEMU_MAI_API_HPP_INCLUDED