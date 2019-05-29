namespace Flexus{
namespace Qemu{
namespace API{
#include "api.h"

QEMU_GET_PHYS_MEMORY_PROC QEMU_get_phys_memory= nullptr;
QEMU_GET_ETHERNET_PROC QEMU_get_ethernet= nullptr;
QEMU_CLEAR_EXCEPTION_PROC QEMU_clear_exception= nullptr;
QEMU_GET_PENDING_INTERRUPT_PROC QEMU_get_pending_interrupt = nullptr;

QEMU_READ_REGISTER_PROC QEMU_read_register= nullptr;
QEMU_READ_PSTATE_PROC QEMU_read_pstate = nullptr;
QEMU_READ_FPCR_PROC QEMU_read_fpcr = nullptr;
QEMU_READ_FPSR_PROC QEMU_read_fpsr = nullptr;
QEMU_READ_EXCEPTION_PROC QEMU_read_exception = nullptr;
QEMU_READ_SCTLR_PROC QEMU_read_sctlr = nullptr;
QEMU_READ_HCR_EL2_PROC QEMU_read_hcr_el2 = nullptr;
QEMU_CPU_HAS_WORK_PROC QEMU_cpu_has_work = nullptr;
QEMU_READ_SP_EL_PROC QEMU_read_sp_el = nullptr;

QEMU_READ_PHYS_MEMORY_PROC QEMU_read_phys_memory= nullptr;
QEMU_GET_PHYS_MEM_PROC QEMU_get_phys_mem= nullptr;
QEMU_GET_CPU_BY_INDEX_PROC QEMU_get_cpu_by_index= nullptr;
QEMU_GET_CPU_INDEX_PROC QEMU_get_cpu_index= nullptr;
QEMU_STEP_COUNT_PROC QEMU_step_count= nullptr;
QEMU_GET_NUM_CPUS_PROC QEMU_get_num_cpus= nullptr;
QEMU_GET_NUM_SOCKETS_PROC QEMU_get_num_sockets= nullptr;
QEMU_GET_NUM_CORES_PROC QEMU_get_num_cores= nullptr;
QEMU_GET_NUM_THREADS_PER_CORE_PROC QEMU_get_num_threads_per_core= nullptr;
QEMU_GET_ALL_CPUS_PROC QEMU_get_all_cpus= nullptr;
QEMU_SET_TICK_FREQUENCY_PROC QEMU_set_tick_frequency= nullptr;
QEMU_GET_TICK_FREQUENCY_PROC QEMU_get_tick_frequency= nullptr;
QEMU_GET_PROGRAM_COUNTER_PROC QEMU_get_program_counter= nullptr;
QEMU_LOGICAL_TO_PHYSICAL_PROC QEMU_logical_to_physical= nullptr;
QEMU_BREAK_SIMULATION_PROC QEMU_break_simulation= nullptr;
QEMU_IS_STOPPED_PROC QEMU_is_stopped = nullptr;
QEMU_SET_SIMULATION_TIME_PROC QEMU_setSimulationTime= nullptr;
QEMU_GET_SIMULATION_TIME_PROC QEMU_getSimulationTime= nullptr;
QEMU_MEM_OP_IS_DATA_PROC QEMU_mem_op_is_data= nullptr;
QEMU_MEM_OP_IS_WRITE_PROC QEMU_mem_op_is_write= nullptr;
QEMU_MEM_OP_IS_READ_PROC QEMU_mem_op_is_read= nullptr;
QEMU_INSTRUCTION_HANDLE_INTERRUPT_PROC QEMU_instruction_handle_interrupt = nullptr;
QEMU_GET_PENDING_EXCEPTION_PROC QEMU_get_pending_exception = nullptr;
QEMU_GET_OBJECT_BY_NAME_PROC QEMU_get_object_by_name = nullptr;
QEMU_CPU_EXEC_PROC QEMU_cpu_execute = nullptr;
QEMU_IS_IN_SIMULATION_PROC QEMU_is_in_simulation = nullptr;
QEMU_TOGGLE_SIMULATION_PROC QEMU_toggle_simulation = nullptr;
QEMU_FLUSH_TB_CACHE_PROC QEMU_flush_tb_cache = nullptr;
QEMU_GET_INSTRUCTION_COUNT_PROC QEMU_get_instruction_count = nullptr;
QEMU_INSERT_CALLBACK_PROC QEMU_insert_callback= nullptr;
QEMU_DELETE_CALLBACK_PROC QEMU_delete_callback= nullptr;
QEMU_DISASSEMBLE_PROC QEMU_disassemble = nullptr;
QEMU_DUMP_STATE_PROC QEMU_dump_state = nullptr;
// Get MMU register state on initialization
//QEMU_GET_MMU_STATE_PROC QEMU_get_mmu_state= nullptr;

void QFLEX_API_set_Interface_Hooks( const QFLEX_API_Interface_Hooks_t* hooks ) {
  QEMU_get_phys_memory = hooks->QEMU_get_phys_memory;
  QEMU_get_ethernet= hooks->QEMU_get_ethernet;
  QEMU_clear_exception= hooks->QEMU_clear_exception;
  QEMU_read_register= hooks->QEMU_read_register;
  QEMU_cpu_has_work = hooks->QEMU_cpu_has_work;

  QEMU_read_fpcr = hooks->QEMU_read_fpcr;
  QEMU_read_fpsr = hooks->QEMU_read_fpsr;
  QEMU_read_pstate = hooks->QEMU_read_pstate;
  QEMU_read_hcr_el2 = hooks->QEMU_read_hcr_el2;
  QEMU_get_pending_interrupt = hooks->QEMU_get_pending_interrupt;
  QEMU_read_exception = hooks->QEMU_read_exception;
  QEMU_read_sp_el = hooks->QEMU_read_sp_el;

  QEMU_read_phys_memory= hooks->QEMU_read_phys_memory;
  QEMU_get_phys_mem= hooks->QEMU_get_phys_mem;
  QEMU_get_cpu_by_index= hooks->QEMU_get_cpu_by_index;
  QEMU_get_cpu_index= hooks->QEMU_get_cpu_index;
  QEMU_step_count= hooks->QEMU_step_count;
  QEMU_get_num_cpus= hooks->QEMU_get_num_cpus;
  QEMU_get_num_sockets= hooks->QEMU_get_num_sockets;
  QEMU_get_num_cores= hooks->QEMU_get_num_cores;
  QEMU_get_num_threads_per_core= hooks->QEMU_get_num_threads_per_core;
  QEMU_get_all_cpus= hooks->QEMU_get_all_cpus;
  QEMU_set_tick_frequency= hooks->QEMU_set_tick_frequency;
  QEMU_get_tick_frequency= hooks->QEMU_get_tick_frequency;
  QEMU_get_program_counter= hooks->QEMU_get_program_counter;
  QEMU_logical_to_physical= hooks->QEMU_logical_to_physical;
  QEMU_break_simulation= hooks->QEMU_break_simulation;
  QEMU_getSimulationTime= hooks->QEMU_getSimulationTime;
  QEMU_setSimulationTime= hooks->QEMU_setSimulationTime;
  QEMU_mem_op_is_data= hooks->QEMU_mem_op_is_data;
  QEMU_mem_op_is_write= hooks->QEMU_mem_op_is_write;
  QEMU_mem_op_is_read= hooks->QEMU_mem_op_is_read;
  QEMU_instruction_handle_interrupt = hooks->QEMU_instruction_handle_interrupt;
  QEMU_get_pending_exception = hooks->QEMU_get_pending_exception;
  QEMU_get_object_by_name = hooks->QEMU_get_object_by_name;
  QEMU_is_in_simulation = hooks->QEMU_is_in_simulation;
  QEMU_toggle_simulation = hooks->QEMU_toggle_simulation;
  QEMU_flush_tb_cache = hooks->QEMU_flush_tb_cache;
  QEMU_get_instruction_count = hooks->QEMU_get_instruction_count;
  QEMU_insert_callback= hooks->QEMU_insert_callback;
  QEMU_delete_callback= hooks->QEMU_delete_callback;
  QEMU_cpu_execute = hooks->QEMU_cpu_execute;///NOOSHIN
  QEMU_disassemble = hooks->QEMU_disassemble;
  QEMU_dump_state = hooks->QEMU_dump_state;
  QEMU_read_sctlr =  hooks->QEMU_read_sctlr;

  // Msutherl: MMU hooks
//  QEMU_get_mmu_state= hooks->QEMU_get_mmu_state;
}

} // namespace API
} // namespace Qemu
} // namespace Flexus
