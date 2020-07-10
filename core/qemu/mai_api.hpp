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

void onInterrupt(void *aPtr, void *anObj, long long aVector);

struct InstructionRaisesException : public QemuException {
  InstructionRaisesException() : QemuException("InstructionRaisesException") {
  }
};
struct UnresolvedDependenciesException : public QemuException {
  UnresolvedDependenciesException() : QemuException("UnresolvedDependenciesException") {
  }
};
struct SpeculativeException : public QemuException {
  SpeculativeException() : QemuException("SpeculativeException ") {
  }
};
struct StallingException : public QemuException {
  StallingException() : QemuException("StallingException ") {
  }
};
struct SyncException : public QemuException {
  SyncException() : QemuException("SyncException") {
  }
};

struct MemoryException : public QemuException {
  MemoryException() : QemuException("MemoryException") {
  }
};

// class ProcessorMapper {
// private:
//  static ProcessorMapper * theMapper;

//  int theNumVMs;
//  std::vector<std::pair<int, int> > theProcMap;
//  std::vector<int> theClientMap;
//  std::vector<std::pair<int, bool> > theReverseMap;

//  ProcessorMapper();

// public:
//  static int mapFlexusIndex2ProcNum(int index);
//  static int mapClientNum2ProcNum(int index);
//  static int mapProcNum2FlexusIndex(int index);
//  static int mapFlexusIndex2VM(int index);

//  static int numVMs();
//  static int numClients();
//  static int numProcessors();
//};

using Flexus::SharedTypes::PhysicalMemoryAddress;
using Flexus::SharedTypes::VirtualMemoryAddress;

class BaseProcessorImpl {
protected:
  API::conf_object_t *theProcessor;
  int theProcessorNumber;
  int theQEMUProcessorNumber;

  BaseProcessorImpl(API::conf_object_t *aProcessor)
      : theProcessor(aProcessor),
        theProcessorNumber(
            aProcessor
                ? /*ProcessorMapper::mapProcNum2FlexusIndex(*/ API::QEMU_get_cpu_index(aProcessor)
                : 0),
        theQEMUProcessorNumber(aProcessor ? API::QEMU_get_cpu_index(aProcessor) : 0) {
  }

public:
  virtual ~BaseProcessorImpl() {
  }

  operator API::conf_object_t *() const {
    return theProcessor;
  }

  std::string disassemble(VirtualMemoryAddress const &anAddress) const {
    API::logical_address_t addr(anAddress);
    char *buffer = API::QEMU_disassemble(*this, addr);
    char *buffer_dup = (char *)buffer;
    while (buffer_dup[0] != '\n') {
      buffer_dup++;
    }
    buffer_dup[0] = '\0';
    std::string s(buffer);
    free(buffer); // qemu called "malloc" for this area
    return s;
  }

  std::string dump_state() const {
    char *buf = new char[716];
    API::QEMU_dump_state(*this, &buf);
    std::string s(buf);
    delete buf;
    return s;
  }

  std::string describeException(int anException) const {
    return "unknown_exception";
  }

  VirtualMemoryAddress getPC() const {
    return VirtualMemoryAddress(API::QEMU_get_program_counter(*this));
  }

  uint64_t readXRegister(int anIndex) const {
    return API::QEMU_read_register(*this, API::kGENERAL, anIndex);
  }

  uint64_t readVRegister(int anIndex) const {
    return API::QEMU_read_register(*this, API::kFLOATING_POINT, anIndex);
  }

  uint32_t readPSTATE() const {
    return API::QEMU_read_pstate(*this);
  }

  uint64_t readSP_el(uint8_t anId) const {
    return API::QEMU_read_sp_el(anId, *this);
  }

  uint32_t readDCZID_EL0() const {
    return API::QEMU_read_DCZID_EL0(*this);
  }

  uint32_t readAARCH64() const {
    return API::QEMU_read_AARCH64(*this);
  }

  void readException(API::exception_t *exp) const {
    return API::QEMU_read_exception(*this, exp);
  }

  uint64_t getPendingInterrupt() const {
    return API::QEMU_get_pending_interrupt(*this);
  }

  uint64_t readSCTLR(uint8_t id) const {
    return API::QEMU_read_sctlr(id, *this);
  }

  uint64_t readTPIDR(uint8_t id) const {
    return API::QEMU_read_tpidr(id, *this);
  }

  uint64_t readHCREL2() const {
    return API::QEMU_read_hcr_el2(*this);
  }

  uint32_t readFPCR() const {
    return API::QEMU_read_fpcr(*this);
  }

  uint32_t readFPSR() const {
    return API::QEMU_read_fpsr(*this);
  }

  bool hasWork() const {
    return API::QEMU_cpu_has_work(*this);
  }

  uint64_t readPC() const {
    return API::QEMU_get_program_counter(*this);
  }

  bits readPhysicalAddress(PhysicalMemoryAddress anAddress, size_t aSize) const {
    uint8_t *buf = new uint8_t[aSize];
    for (size_t i = 0; i < aSize; i++)
      buf[i] = 0;
    API::QEMU_read_phys_memory(buf, API::physical_address_t(anAddress), aSize);

    bits tmp;
    for (size_t i = 0; i < aSize; i++) {
      const size_t s = i * 8;
      bits val = (((bits)buf[i] << s) & ((bits)0xff << s));
      tmp |= val;
    }
    delete[] buf;
    return tmp;
  }

  bits readVirtualAddress(VirtualMemoryAddress anAddress, size_t size) {
    VirtualMemoryAddress finalAddress((anAddress + size - 1) & ~0xFFF);
    if ((finalAddress & 0x1000) != (anAddress & 0x1000)) {
      bits value1, value2;
      size_t partial = finalAddress - anAddress;
      value1 = readPhysicalAddress(
          PhysicalMemoryAddress(API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction,
                                                              API::logical_address_t(anAddress))),
          partial);
      value2 = readPhysicalAddress(
          PhysicalMemoryAddress(API::QEMU_logical_to_physical(
              *this, API::QEMU_DI_Instruction, API::logical_address_t(finalAddress))),
          size - partial);
      value2 = (value2 << (partial << 3)) | value1;
      return value2;
    }
    return readPhysicalAddress(
        PhysicalMemoryAddress(API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction,
                                                            API::logical_address_t(anAddress))),
        size);
  }

  PhysicalMemoryAddress translateVirtualAddress(VirtualMemoryAddress anAddress) {
    return PhysicalMemoryAddress(API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction,
                                                               API::logical_address_t(anAddress)));
  }

  uint32_t fetchInstruction(VirtualMemoryAddress anAddress) {
    return (uint32_t)readVirtualAddress(anAddress, 4);
  }

  int id() const {
    return theProcessorNumber;
  }
  int QEMUId() const {
    return theQEMUProcessorNumber;
  }
  bool mai_mode() const {
    return true;
  }

  uint64_t read_sysreg_from_qemu(uint8_t opc0, uint8_t opc1, uint8_t opc2, uint8_t crn,
                                 uint8_t crm) {
    return API::QEMU_read_unhashed_sysreg(*this, opc0, opc1, opc2, crn, crm);
  }
};

class armProcessorImpl : public BaseProcessorImpl {
private:
  int thePendingInterrupt;
  bool theInterruptsConnected;
  void initialize();
  void handleInterrupt(long long aVector);

public:
  explicit armProcessorImpl(API::conf_object_t *aProcessor)
      : BaseProcessorImpl(aProcessor), thePendingInterrupt(API::QEMU_PE_No_Exception),
        theInterruptsConnected(false) {
  }

public:
  uint8_t getQEMUExceptionLevel() const {
    return API::QEMU_get_current_el(*this);
  }

  void breakSimulation() {
    API::QEMU_break_simulation("");
  }

  int advance() {
    int exception = 0;
    exception = Qemu::API::QEMU_cpu_execute(theProcessor);
    return exception;
  }
};

#define PROCESSOR_IMPL armProcessorImpl

class Processor : public BuiltInObject<PROCESSOR_IMPL> {
  typedef BuiltInObject<PROCESSOR_IMPL> base;

public:
  explicit Processor() : base(0) {
  }

  explicit Processor(API::conf_object_t *aProcessor) : base(PROCESSOR_IMPL(aProcessor)) {
  }

  operator API::conf_object_t *() {
    return *this;
  }
  static Processor getProcessor(int aProcessorNumber) {
    return Processor(API::QEMU_get_cpu_by_index(
        /*ProcessorMapper::mapFlexusIndex2ProcNum(*/ aProcessorNumber));
  }
  static Processor getProcessor(std::string const &aProcessorName) {
    return Processor(API::QEMU_get_object_by_name(aProcessorName.c_str()));
  }
  static Processor current() {
    assert(false);
    return getProcessor(0);
  }
};

#undef PROCESSOR_IMPL

} // end Namespace Qemu
} // end namespace Flexus

#endif // FLEXUS_QEMU_MAI_API_HPP_INCLUDED
