#ifndef FLEXUS_QEMU_MAI_API_HPP_INCLUDED
#define FLEXUS_QEMU_MAI_API_HPP_INCLUDED

#include <bitset>
#include <core/flexus.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <cstdint>

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

    PhysicalMemoryAddress translate_va2pa(VirtualMemoryAddress addr, bool unprivileged)
    {
        return PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, addr, unprivileged));
    }

    bits read_va(VirtualMemoryAddress anAddress, size_t size, bool unprivileged)
    {
        VirtualMemoryAddress finalAddress(((uint64_t)(anAddress) + size - 1) & ~0xFFF);

        if ((finalAddress & 0x1000) != (anAddress & 0x1000)) {
            // Break the access into two memory accesses on each page, then concaticate their result.
            bits value1, value2;
            size_t partial = finalAddress - anAddress; // Partial is the size of the first access.
            value1         = read_pa(
              PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, API::logical_address_t(anAddress), unprivileged)),
              partial);
            value2 = read_pa(
              PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index,
              API::logical_address_t(finalAddress), unprivileged)), size - partial);
            // * 8 convert bytes to bits. value2 si appended to value2
            value2 = (value2 << (partial * 8)) | value1;
            return value2;
        }
        return read_pa(
          PhysicalMemoryAddress(API::qemu_api.translate_va2pa(core_index, API::logical_address_t(anAddress), unprivileged)),
          size);
    }

    uint32_t fetch_inst(VirtualMemoryAddress addr) { return static_cast<uint32_t>(read_va(addr, 4, false)); }

    bits read_pa(PhysicalMemoryAddress anAddress, size_t aSize) const
    {
        uint8_t *buf = new uint8_t[aSize];

        API::qemu_api.get_mem(buf, API::physical_address_t(anAddress), aSize);

        bits tmp = 0;
        for (size_t i = 0; i < aSize; i++) {
            const size_t s = i * 8;
            bits val       = (((bits)buf[i] << s) & ((bits)0xff << s));
            tmp |= val;
        }

        delete[] buf;
        return tmp;
    }

    uint64_t read_sysreg(uint8_t opc0, uint8_t opc1, uint8_t opc2, uint8_t crn, uint8_t crm)
    {
        return API::qemu_api.read_sys_register(core_index, opc0, opc1, opc2, crn, crm, false);
    }

    uint64_t id() const { return core_index; }

    std::string disassemble(VirtualMemoryAddress const& address) const
    {
        char* qemu_disas_str = API::qemu_api.disassembly(core_index, (uint64_t)address, 4);

        std::string buffer(qemu_disas_str);
        free(qemu_disas_str);

        return buffer;
    }

    bool is_busy() const { return API::qemu_api.is_busy(core_index); }
    // TODO ─── NOT implemented ────────────────────────────────────────────────

    void dump_state(SharedTypes::CPU_State& dump)
    {
        dump.pc = read_register(API::PC);

        for (std::size_t i{ 0 }; i < 32; i++) {
            dump.regs[i] = read_register(API::GENERAL, i);
        }
    }

    // uint64_t readSCTLR(uint64_t index) { return 0; }

    // uint64_t readPC() const { return 0; }

    // void breakSimulation() {}

    // void readException(API::exception_t* exp) const {}
    //  explicit Processor(): base(0) {}

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
