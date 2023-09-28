// #include <unordered_map>
#include <memory>
#include "DummyQemu.hpp"

// class DummyQemu {
// private:
//   // Declare the class variables
//   TestBenchDummyCPUState *cpu_states;

//   static Flexus::Qemu::API::conf_object_t *qemu_cpus;

//   int ncpus;

//   static DummyRegs dummyRegs;

//   static std::vector<uint64_t> programCounterVector;

//   std::vector<int> permissibleMemoryRequestPayloadValues = {
//       0x1,   0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10,
//       0x1000 // Adding the value 0x1000 (hexadecimal)
//   };
//   Flexus::Qemu::API::conf_object_t *DummyQemu::qemu_cpus = nullptr;
//   DummyRegs DummyQemu::dummyRegs;
//   std::vector<uint64_t> DummyQemu::programCounterVector;

// public:

//   // Declare the class variables
//   std::vector<int> permissibleMemoryRequestPayloadValues = {
//       0x1,   0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10,
//       0x1000 // Adding the value 0x1000 (hexadecimal)
//   };
static std::unique_ptr<Flexus::Qemu::API::conf_object_t[]> qemu_cpus;

//   // Declare the function prototypes
DummyQemu::DummyQemu(int ncpus = 3) {

  qemu_cpus = std::unique_ptr<Flexus::Qemu::API::conf_object_t[]>(
      new Flexus::Qemu::API::conf_object_t[ncpus]);

  cpu_states = std::unique_ptr<TestBenchDummyCPUState[]>(new TestBenchDummyCPUState[ncpus]);

  auto qc = qemu_cpus.get();
  auto cs = cpu_states.get();

  // Create dummy objects
  for (int i = 0; i < ncpus; i++) {
    int needed = snprintf(NULL, 0, "cpu%d", i) + 1;
    cs[i].cpu_index = i;
    cs[i].registers.reg_tcr[0] = (16 << 16);
    cs[i].registers.reg_tcr[1] = 2148794384; // TODO (1 << 20) |
    cs[i].registers.reg_tcr[2] = (16 << 16);
    cs[i].registers.reg_tcr[3] = (12 << 16);
    cs[i].registers.reg_ttbr0[1] = 0x00001000;
    cs[i].registers.reg_ttbr1[1] = 0x00000000C0000000;
    cs[i].registers.ID_AA64MMFR0_EL1 = 0x33;
    cs[i].registers.pc = 0x1000;

    qc[i].name   = (char *)malloc(needed);
    qc[i].type   = Flexus::Qemu::API::conf_object_t::QEMU_DummyCPUState;
    qc[i].object = &cs[i];
  }
}

DummyRegs DummyQemu::getDummyRegs(int index) const {
  return cpu_states[index].registers;
}
TestBenchDummyCPUState& DummyQemu::getTestBenchDummyCPUState(int index) const {
  return cpu_states[index];
}
Flexus::Qemu::API::conf_object_t DummyQemu::getQemuCPUs() const {
  return *qemu_cpus.get();
}
Flexus::Qemu::API::conf_object_t *DummyQemu::DummyQEMU_get_cpu_by_index(int index) {
  return &qemu_cpus.get()[index];
}

int DummyQemu::DummyQEMU_get_cpu_index(Flexus::Qemu::API::conf_object_t *cpu) {
  return ((TestBenchDummyCPUState *)cpu->object)->cpu_index;
}

void DummyQemu::DummyQEMU_read_phys_memory(uint8_t *buf, Flexus::Qemu::API::physical_address_t pa,
                                           int bytes) {
  assert(0 <= bytes && bytes <= 16);
  buf[0] = 0x23;
  buf[1] = 0x10;
  for (int i = 2; i < bytes - 1; ++i) {
    // Set the buffer values to return 0x3
    buf[i] = 0x0;
  }
}

int DummyQemu::DummyQEMU_cpu_execute(Flexus::Qemu::API::conf_object_t *cpu, int cycles) {
  TestBenchDummyCPUState *cs = (TestBenchDummyCPUState *) cpu->object;
  cs->registers.pc = cs->registers.pc + 4 * cycles;
  return 0;
}

uint64_t DummyQemu::DummyQEMU_get_program_counter(Flexus::Qemu::API::conf_object_t *cpu) {
  TestBenchDummyCPUState *cs = (TestBenchDummyCPUState *)cpu->object;
  return cs->registers.pc;
}

uint64_t DummyQemu::DummyQEMU_read_sctlr(uint8_t id, Flexus::Qemu::API::conf_object_t *cpu) {
  TestBenchDummyCPUState *cs = (TestBenchDummyCPUState *) cpu->object;
  return cs->registers.reg_sctlr[id];
}

Flexus::Qemu::API::physical_address_t
DummyQemu::DummyQEMU_logical_to_physical(Flexus::Qemu::API::conf_object_t *cpu,
                                         Flexus::Qemu::API::data_or_instr_t fetch,
                                         Flexus::Qemu::API::logical_address_t va) {
  // int access_type = (fetch == Flexus::Qemu::API::QEMU_DI_Instruction ?
  // Flexus::Qemu::API::INST_FETCH : Flexus::Qemu::API::DATA_LOAD); CPUState *cs =
  // qemu_get_cpu(((CPUState *)cpu->object)->cpu_index); TestBenchDummyCPUState *cs =
  // (TestBenchDummyCPUState *)cpu->object; Assuming 4KB page size Dummy page table, using an
  // unordered_map for simplicity
  std::unordered_map<Flexus::Qemu::API::logical_address_t, Flexus::Qemu::API::physical_address_t>
      page_table;
  // Populate the page table with some mappings
  // (These are just dummy mappings for illustration)
  for (int i = 0; i < 17; i++) {

    page_table[0x80000000 + (i * 4096)] =
        0x00001000 + (i * 4096); // Map logical address 0x80000000 to physical address 0x00001000
  }

  // Check if the logical address exists in the page table
  if (page_table.find(va) != page_table.end()) {
    // Perform address translation
    Flexus::Qemu::API::physical_address_t pa = page_table[va];
    return pa;
  } else {
    // If the logical address is not found in the page table, return an error value (e.g., -1)
    return static_cast<Flexus::Qemu::API::physical_address_t>(-1);
  }
}

uint64_t DummyQemu::DummyQEMU_read_register(Flexus::Qemu::API::conf_object_t *cpu,
                                            Flexus::Qemu::API::arm_register_t reg_type,
                                            int reg_index, int el) {

  TestBenchDummyCPUState* cs = (TestBenchDummyCPUState*) cpu->object;

  switch (reg_type) {
  case kGENERAL:
    assert(reg_index <= 31 && reg_index >= 0);
    return cs->registers.reg[reg_index];
    ;
  case kFLOATING_POINT:
    assert(reg_index <= 31 && reg_index >= 0);
    return true;
  case kMMU_TCR:
    return cs->registers.reg_tcr[el];
  case kMMU_SCTLR:
    return cs->registers.reg_sctlr[el];
  case kMMU_TTBR0:
    return cs->registers.reg_ttbr0[el];
  case kMMU_TTBR1:
    return cs->registers.reg_ttbr1[el];
  case kMMU_ID_AA64MMFR0:
    return cs->registers.ID_AA64MMFR0_EL1;
  default:
    fprintf(stderr, "ERROR case triggered in readReg. reg_idx: %d, reg_type: %d\n, el: %d",
            reg_index, reg_type, el);
    assert(false);
    break;
  }
}
