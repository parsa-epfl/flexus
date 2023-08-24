#include <unordered_map>
// #include <hw/core/cpu.h>
#include <core/qemu/api_wrappers.hpp>
// #include <core/qemu/mai_api.hpp>
// #include "qflex/qflex-arch.h"
#include "DummyQemu.h"

// Define the global variables
Flexus::Qemu::API::conf_object_t* DummyQemu::qemu_cpus = nullptr;
DummyRegs DummyQemu::dummyRegs; 
std::vector<uint64_t> DummyQemu::programCounterVector;

  DummyRegs DummyQemu::getDummyRegs() const{
    return dummyRegs;
  } 

  TestBenchDummyCPUState DummyQemu::getTestBenchDummyCPUState() const{
    return *cpu_states;
  } 

  Flexus::Qemu::API::conf_object_t DummyQemu::getQemuCPUs() const{
    return *qemu_cpus;
  }

 void DummyQemu::initialize(int ncpus = 3){
    qemu_cpus = NULL;
    qemu_cpus = (Flexus::Qemu::API::conf_object_t*)malloc(sizeof(Flexus::Qemu::API::conf_object_t) * ncpus);
    cpu_states = (TestBenchDummyCPUState*)malloc(sizeof(TestBenchDummyCPUState) * ncpus); // Create dummy objects

    for (int i = 0; i < ncpus; i++) {
        // qemu_cpus[i].type = static_cast<Flexus::Qemu::API::conf_object_t::e>(QEMU_DummyCPUState);
        qemu_cpus[i].type = Flexus::Qemu::API::conf_object_t::QEMU_DummyCPUState;
        int needed = snprintf(NULL, 0, "cpu%d", i) + 1;
        qemu_cpus[i].name = (char*)malloc(needed);
        cpu_states->cpu_index = i;
        qemu_cpus[i].object = &cpu_states[i];
    }

    dummyRegs.reg_tcr[0]=(16 << 16);
    // dummyRegs.reg_tcr[1]=2147483648;
    dummyRegs.reg_tcr[1]=2148794384;
    dummyRegs.reg_tcr[2]=(16 << 16);
    dummyRegs.reg_tcr[3]=(12 << 16);

    dummyRegs.reg_ttbr0[1]=0x00001000;
    dummyRegs.reg_ttbr1[1]=0x00000000C0000000;

    dummyRegs.ID_AA64MMFR0_EL1=0x33;
    cpu_states = NULL;
}


