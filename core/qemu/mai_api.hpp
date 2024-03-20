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
#include <boost/utility.hpp>
#include <core/exception.hpp>
#include <core/flexus.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <iomanip>
#include <sstream>
#include <string>

using namespace Flexus::Core;
namespace Flexus {
namespace Qemu {

// void onInterrupt(void *aPtr, void *anObj, long long aVector);

using Flexus::SharedTypes::PhysicalMemoryAddress;
using Flexus::SharedTypes::VirtualMemoryAddress;

class ARMProcessor {

protected:
//   API::conf_object_t *theProcessor;
    uint64_t cpu_index;

public:
    ARMProcessor(uint64_t cpu_index): cpu_index(cpu_index)
    {}


//   ARMProcessor(API::conf_object_t *aProcessor)
//       : theProcessor(aProcessor),
//         theProcessorNumber(
//             aProcessor
//                 ? API::qemu_api.get_cpu_idx(aProcessor)
//                 : 0),
//         theQEMUProcessorNumber(aProcessor ? API::qemu_api.get_cpu_idx(aProcessor) : 0) {
//   }

// public:
//   virtual ~ARMProcessor() {
//   }

//   operator API::conf_object_t *() const {
//     return theProcessor;
//   }

//   std::string disassemble(VirtualMemoryAddress const &anAddress) const {
//     API::logical_address_t addr(anAddress);
//     char *buf = API::qemu_api.disass(*this, addr);
//     std::string s(buf);
//     free(buf); // qemu called "malloc" for this area
//     return s;
//   }

//   std::string dump_state() const {
//     char *buf = API::qemu_api.get_snap(*this);
//     std::string s(buf);
//     free(buf);
//     return s;
//   }

//   std::string describeException(int anException) const {
//     return "unknown_exception";
//   }


//     return VirtualMemoryAddress(API::qemu_api.get_pc(*this));
//   }

    uint64_t readXRegister(size_t anIndex) const
    {
        return 0;
    }
//     return API::qemu_api.get_gpr(*this, anIndex);
//   }

    uint64_t readVRegister(int anIndex) const
    {
        return 0;
    }
//     return API::qemu_api.get_fpr(*this, anIndex);
//   }

    void readException(API::exception_t *exp) const
    {}
//     assert(false);
//     return;
//   }

    uint64_t getPendingInterrupt() const
    {
        return 0;
    }
//     return API::qemu_api.get_irq(*this);
//   }

//   bool hasWork() const {
//     return API::qemu_api.cpu_busy(*this);
//   }

//   uint64_t readPC() const {
//     return API::qemu_api.get_pc(*this);
//   }


//     uint8_t *buf = new uint8_t[aSize];
//     for (size_t i = 0; i < aSize; i++)
//       buf[i] = 0;
//     API::qemu_api.get_mem(buf, API::physical_address_t(anAddress), aSize);

//     bits tmp;
//     for (size_t i = 0; i <readException aSize; i++) {
//       const size_t s = i * 8;
//       bits val = (((bits)buf[i] << s) & ((bits)0xff << s));
//       tmp |= val;
//     }
//     delete[] buf;
//     return tmp;
//   }

    bits readVirtualAddress(VirtualMemoryAddress anAddress, size_t size)
    {
        return bits(0);
    }
//     VirtualMemoryAddress finalAddress(((uint64_t)(anAddress) + size - 1) & ~0xFFF);
//     if ((finalAddress & 0x1000) != (anAddress & 0x1000)) {
//       bits value1, value2;
//       size_t partial = finalAddress - anAddress;
//       value1 = readPhysicalAddress(
//           PhysicalMemoryAddress(API::qemu_api.get_pa(*this, API::QEMU_DI_Instruction,
//                                                             API::logical_address_t(anAddress))),
//           partial);
//       value2 = readPhysicalAddress(
//           PhysicalMemoryAddress(API::qemu_api.get_pa(*this, API::QEMU_DI_Instruction,
//                                                             API::logical_address_t(finalAddress))),
//           size - partial);
//       value2 = (value2 << (partial << 3)) | value1;
//       return value2;
//     }
//     return readPhysicalAddress(
//         PhysicalMemoryAddress(API::qemu_api.get_pa(*this, API::QEMU_DI_Instruction,
//                                                           API::logical_address_t(anAddress))),
//         size);
//   }

//   PhysicalMemoryAddress translateVirtualAddress(VirtualMemoryAddress anAddress) {
//     return PhysicalMemoryAddress(API::qemu_api.get_pa(*this, API::QEMU_DI_Instruction,
//                                                              API::logical_address_t(anAddress)));
//   }

//   uint32_t fetchInstruction(VirtualMemoryAddress anAddress) {
//     return (uint32_t)readVirtualAddress(anAddress, 4);
//   }


//     return theProcessorNumber;
//   }
//   int QEMUId() const {
//     return theQEMUProcessorNumber;
//   }
//   bool mai_mode() const {
//     return true;
//   }

//   uint64_t read_sysreg_from_qemu(uint32_t no) {
//     return API::qemu_api.get_csr(*this, no);
//   }


    // void readAArch() {}
    // void readDCZID_EL0() {}
    // void readFPCR() {}
    // void readFPSR() {}
    // void readHCREL2() {}
    // void readPSTATE() {}
    // // void readSCTLR() {}
    // void readSP_el() {}
    // void breakSimulation() {}
    // int advance(bool count_time = true) {return 0;}

};

// class ARMProcessor : public ARMProcessor {
// private:
//   int thePendingInterrupt;
//   bool theInterruptsConnected;
//   void initialize();
//   void handleInterrupt(long long aVector);

// public:
//   explicit ProcessorImpl(API::conf_object_t *aProcessor)
//       : ARMProcessor(aProcessor), thePendingInterrupt(API::QEMU_PE_No_Exception),
//         theInterruptsConnected(false) {
//   }

// public:
//   uint8_t getQEMUExceptionLevel() const {
//     return API::qemu_api.get_pl(*this);
//   }


//     API::qemu_api.stop("");
//   }

//     int exception = 0;
//     exception = Qemu::API::qemu_api.cpu_exec(theProcessor, count_time);
//     return exception;
//   }
// };

// #define PROCESSOR_IMPL ProcessorImpl

class Processor /*: public BuiltInObject<PROCESSOR_IMPL>*/
{


private:
    uint64_t core_index;

    // typedef BuiltInObject<PROCESSOR_IMPL> base;

    Processor(u_int64_t core_index): core_index(core_index)
    {}


public:

    bits readPhysicalAddress(PhysicalMemoryAddress anAddress, size_t aSize) const { return bits(0); }

    PhysicalMemoryAddress translateVirtualAddress(VirtualMemoryAddress addr) { return PhysicalMemoryAddress(); }

    uint32_t fetchInstruction(VirtualMemoryAddress addr) {return 0;}
    uint64_t readSCTLR(uint64_t index) { return 0;}

    VirtualMemoryAddress getPC() const { return VirtualMemoryAddress(); }

    size_t id() const { return core_index; }

    std::string disassemble(VirtualMemoryAddress const &anAddress) const { return "TODO()!"; }

    bool is_busy() const { return true; }

    uint64_t read_sysreg_from_qemu(uint32_t no) { return 0; }

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

    static Processor getProcessor(uint64_t core_index)
    {
        return Processor(core_index);
    }

};

// #undef PROCESSOR_IMPL

} // end Namespace Qemu
} // end namespace Flexus

#endif  // FLEXUS_QEMU_MAI_API_HPP_INCLUDED
