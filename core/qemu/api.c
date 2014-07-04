#ifdef __cplusplus
extern "C" {
#endif
#include "core/qemu/api.h"

int QEMU_clear_exception(void) 
{
	return 42;
}

uint64_t QEMU_read_register(conf_object_t *cpu, int reg_index) 
{
	REQUIRES(cpu->type == QEMUCPUState);
	return 42;
}

uint32_t QEMU_read_phys_memory(conf_object_t *cpu, 
		physical_address_t pa, int bytes) 
{
	REQUIRES(0 <= bytes && bytes <= 8);
	return 42;
}

conf_object_t *QEMU_get_cpu_by_index(int index)
{
	conf_object_t *cpu = (conf_object_t*)0xdeadbeef;
	ENSURES(cpu->type == QEMUCPUState);
	return cpu;
}

int QEMU_set_tick_frequency(conf_object_t *cpu, double tick_freq) 
{
	return 42;
}

uint64_t QEMU_get_program_counter(conf_object_t *cpu) 
{
	REQUIRES(cpu->type == QEMUCPUState);
	return 42;
}

physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va) 
{
	REQUIRES(cpu->type == QEMUCPUState);
	return 42;
}

int QEMU_mem_op_is_data(generic_transaction_t *mop)
{
	return 42;
}

int QEMU_mem_op_is_write(generic_transaction_t *mop) 
{
	return 42;
}

int QEMU_mem_op_is_read(generic_transaction_t *mop)
{
	return 42;
}

// note: see QEMU_callback_table in api.h
// should return a unique identifier to the callback struct
int QEMU_insert_callback(QEMU_callback_event_t event, QEMU_callback fn, void *arg) 
{
	QEMU_all_callbacks.next_callback_id++;
	return 42;
}

void QEMU_delete_callback(QEMU_callback_event_t, uint64_t callback_id) 
{
	return;
}

void QEMU_simulation_break(void) 
{
	return;
}

#ifdef __cplusplus
}
#endif
