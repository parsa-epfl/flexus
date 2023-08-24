#ifndef DUMMYQEMU_H
#define DUMMYQEMU_H

#include <unordered_map>
#include <core/qemu/qflex-api.h>
#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/mai_api.hpp>

  typedef struct {
    uint64_t reg[31];
    // Add other members here if needed
    uint64_t reg_sctlr[4];
    uint64_t reg_tcr[4];
    uint64_t reg_ttbr0[4];
    uint64_t reg_ttbr1[4];
    uint64_t reg_isa_reg[4];
    uint64_t ID_AA64MMFR0_EL1;
  } DummyRegs;

  typedef struct {
    int cpu_index;
    int cpu_execute;
    DummyRegs registers;
    // Add other members here if needed
  } TestBenchDummyCPUState;

class DummyQemu {
  public: 

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

  // Declare the class variables
    TestBenchDummyCPUState *cpu_states;
    static Flexus::Qemu::API::conf_object_t* qemu_cpus;
    int ncpus;
    static DummyRegs dummyRegs;
    static std::vector<uint64_t> programCounterVector;
    std::vector<int> permissibleMemoryRequestPayloadValues = {
        0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
        0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10,
        0x1000 // Adding the value 0x1000 (hexadecimal)
    };

  // Declare the function prototypes
  void initialize(int ncpus);
  // Flexus::Qemu::API::physical_address_t DummyQEMU_logical_to_physical(Flexus::Qemu::API::conf_object_t *cpu, Flexus::Qemu::API::data_or_instr_t fetch, Flexus::Qemu::API::logical_address_t va);
  // uint64_t DummyQEMU_read_register(Flexus::Qemu::API::conf_object_t *cpu, Flexus::Qemu::API::arm_register_t reg_type, int reg_index, int el); 
  // uint64_t DummyQEMU_read_sctlr(uint8_t id, Flexus::Qemu::API::conf_object_t *cpu);
  // int DummyQEMU_get_cpu_index(Flexus::Qemu::API::conf_object_t *cpu); 
  // Flexus::Qemu::API::conf_object_t *DummyQEMU_get_cpu_by_index(int index); 
  // void DummyQEMU_read_phys_memory(uint8_t *buf, Flexus::Qemu::API::physical_address_t pa, int bytes);
  // uint64_t DummyQEMU_get_program_counter(Flexus::Qemu::API::conf_object_t * cpu);

  DummyRegs getDummyRegs() const;
  TestBenchDummyCPUState getTestBenchDummyCPUState() const;
  Flexus::Qemu::API::conf_object_t getQemuCPUs() const;

  DummyQemu(int ncpus){
    for (uint64_t pc = 0x1000; pc < 0x1200; pc += 4) {
        programCounterVector.push_back(pc);
        std::cout<<"PC Value inserted:"<<programCounterVector.back()<<"\n";
  }
  }

  static Flexus::Qemu::API::conf_object_t* DummyQEMU_get_cpu_by_index(int index) {
      return &qemu_cpus[index];
  }
  static int DummyQEMU_get_cpu_index(Flexus::Qemu::API::conf_object_t *cpu) {
    return ((TestBenchDummyCPUState *) cpu->object)->cpu_index;
  }

  static void DummyQEMU_read_phys_memory(uint8_t *buf, Flexus::Qemu::API::physical_address_t pa, int bytes) {
    // assert(0 <= bytes && bytes <= 16);
    // for (int i = 0; i < bytes; ++i) {
    //     buf[i] = static_cast<uint8_t>((pa + i) & 0xFF);
    // }

    assert(0 <= bytes && bytes <= 16);
    buf[0] = 0x23;
    buf[1]= 0x10;
    for (int i = 2; i < bytes-1; ++i) {
        // Set the buffer values to return 0x3
        buf[i] = 0x0;
    }
  }
  static uint64_t DummyQEMU_get_program_counter(Flexus::Qemu::API::conf_object_t * cpu) {
    // CPUState *cs = qemu_get_cpu(((CPUState *)cpu->object)->cpu_index);
    // return QFLEX_GET_ARCH(pc)(cs);
     uint64_t pc = 0x1000; // Default program counter value

        if (!programCounterVector.empty()) {
            // Get the latest program counter value from the vector and remove it
            pc = programCounterVector.back();
            programCounterVector.pop_back();
        }
        std::cout<<"PC Value popped:"<<pc<<"\n";
        return pc;
  }
  static uint64_t DummyQEMU_read_sctlr(uint8_t id, Flexus::Qemu::API::conf_object_t *cpu) {
      return dummyRegs.reg_sctlr[id];
 }

  static Flexus::Qemu::API::physical_address_t DummyQEMU_logical_to_physical(Flexus::Qemu::API::conf_object_t *cpu, Flexus::Qemu::API::data_or_instr_t fetch, Flexus::Qemu::API::logical_address_t va) 
  {
      // int access_type = (fetch == Flexus::Qemu::API::QEMU_DI_Instruction ? Flexus::Qemu::API::INST_FETCH : Flexus::Qemu::API::DATA_LOAD);
      // CPUState *cs = qemu_get_cpu(((CPUState *)cpu->object)->cpu_index);
      // TestBenchDummyCPUState *cs = (TestBenchDummyCPUState *)cpu->object;
      // Assuming 4KB page size
      // Dummy page table, using an unordered_map for simplicity
      std::unordered_map<Flexus::Qemu::API::logical_address_t, Flexus::Qemu::API::physical_address_t> page_table;
      // Populate the page table with some mappings
      // (These are just dummy mappings for illustration)
      for (int i = 0; i < 17; i++) {

      page_table[0x80000000+(i*4096)] = 0x00001000+(i*4096); // Map logical address 0x80000000 to physical address 0x00001000
      }

      // Check if the logical address exists in the page table
      if (page_table.find(va) != page_table.end()) {
          // Perform address translation
          Flexus::Qemu::API::physical_address_t pa = page_table[va] ;
          return pa;
      } else {
          // If the logical address is not found in the page table, return an error value (e.g., -1)
          return static_cast<Flexus::Qemu::API::physical_address_t>(-1);
      }
  }

  static uint64_t DummyQEMU_read_register(Flexus::Qemu::API::conf_object_t *cpu, Flexus::Qemu::API::arm_register_t reg_type, int reg_index, int el) {

    switch (reg_type) {
    case kGENERAL:
      assert(reg_index <= 31 && reg_index >= 0);
      return dummyRegs.reg[reg_index];;
    case kFLOATING_POINT:
      assert(reg_index <= 31 && reg_index >= 0);
      return true;
    case kMMU_TCR: 
      return dummyRegs.reg_tcr[el];
    case kMMU_SCTLR:
      return dummyRegs.reg_sctlr[el];
    case kMMU_TTBR0:
      return  dummyRegs.reg_ttbr0[el];
    case kMMU_TTBR1:
      return  dummyRegs.reg_ttbr1[el];
    case kMMU_ID_AA64MMFR0:
      return dummyRegs.ID_AA64MMFR0_EL1;
    default:
      fprintf(stderr,
              "ERROR case triggered in readReg. reg_idx: %d, reg_type: %d\n, el: %d",
              reg_index, reg_type, el);
      assert(false);
      break;
    }
  }
};
#endif