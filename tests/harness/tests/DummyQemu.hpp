#ifndef DUMMYQEMU_H
#define DUMMYQEMU_H

#include <unordered_map>
#include <vector>
#include <iostream>
#include <cassert>
#include <memory>
#include <core/qemu/api_wrappers.hpp>

typedef struct {
  uint64_t pc;
  uint64_t reg[32];
  uint64_t reg_sctlr[4];
  uint64_t reg_tcr[4];
  uint64_t reg_ttbr0[4];
  uint64_t reg_ttbr1[4];
  uint64_t reg_isa_reg[4];
  uint64_t ID_AA64MMFR0_EL1;
} DummyRegs;

// Class Variables
typedef enum {
  kGENERAL = 0,
  kFLOATING_POINT,
  kFPSR,
  kFPCR,
  kPSTATE,
  kEXP_IDX,
  kMMU_TCR,
  kMMU_SCTLR,
  kMMU_TTBR0,
  kMMU_TTBR1,
  kMMU_ID_AA64MMFR0,
} arm_register_t;

typedef struct {
  int cpu_index;
  int cpu_execute;
  DummyRegs registers;
  // Add other members here if needed
} TestBenchDummyCPUState;

class DummyQemu {

private:
  std::unique_ptr<TestBenchDummyCPUState[]> cpu_states;

public:
  // Declare the function prototypes

  DummyQemu(int ncpus);

  DummyRegs getDummyRegs(int index) const;
  TestBenchDummyCPUState& getTestBenchDummyCPUState(int index) const;
  int DummyQEMU_cpu_execute(Flexus::Qemu::API::conf_object_t *cpu, int cycles);
  Flexus::Qemu::API::conf_object_t getQemuCPUs() const;
  static Flexus::Qemu::API::conf_object_t *DummyQEMU_get_cpu_by_index(int index);
  static int DummyQEMU_get_cpu_index(Flexus::Qemu::API::conf_object_t *cpu);

  static void DummyQEMU_read_phys_memory(uint8_t *buf, Flexus::Qemu::API::physical_address_t pa,
                                  int bytes);

  static uint64_t DummyQEMU_get_program_counter(Flexus::Qemu::API::conf_object_t *cpu);
  static uint64_t DummyQEMU_read_sctlr(uint8_t id, Flexus::Qemu::API::conf_object_t *cpu);
  static Flexus::Qemu::API::physical_address_t DummyQEMU_logical_to_physical(Flexus::Qemu::API::conf_object_t *cpu,
                                Flexus::Qemu::API::data_or_instr_t fetch,
                                Flexus::Qemu::API::logical_address_t va);
  static uint64_t DummyQEMU_read_register(Flexus::Qemu::API::conf_object_t *cpu,
                                   Flexus::Qemu::API::arm_register_t reg_type, int reg_index,
                                   int el);
};


#endif