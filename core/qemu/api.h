#include <inttypes.h>
#include <stdlib.h>

#ifdef DEBUG // temporary, just to make requirements of dummies clear
#include <assert.h>
#define ASSERT(COND) assert(COND)
#define REQUIRES(COND) assert(COND)
#define ENSURES(COND) assert(COND)
#else
#define ASSERT(COND) ((void)0)
#define REQUIRES(COND) ((void)0)
#define ENSURES(COND) ((void)0)
#endif

typedef uint64_t cycles_t;
typedef uint64_t physical_address_t;
typedef uint64_t logical_address_t;

struct conf_object {
	void *object; // pointer to the struct in question
	enum { // what kind of QEMU struct does it represent
		QEMUCPUState, // add new types as necessary
		QEMUAddressSpace
	} type;
};
typedef struct conf_object conf_object_t;

typedef enum {
	QEMU_Trans_Load,
	QEMU_Trans_Store,
	QEMU_Trans_Instr_Fetch,
	QEMU_Trans_Prefetch,
	QEMU_Trans_Cache
} mem_op_type_t;

struct generic_transaction {
	logical_address_t logical_address;
	physical_address_t physical_address;
	uint64_t bytes;
	size_t size;
	mem_op_type_t type;
	conf_object_t *ini_ptr;
	unsigned int atomic:1;
};
typedef struct generic_transaction generic_transaction_t;

typedef enum {
	QEMU_CPU_Mode_User,
	QEMU_CPU_Mode_Supervisor,
	QEMU_CPU_Mode_Hypervisor
} processor_mode_t;

struct memory_transaction {
	generic_transaction_t s;
#if FLEXUS_TARGET == FLEXUS_TARGET_v9
	unsigned int cache_virtual:1;
	unsigned int cache_physical:1;
	unsigned int priv:1;
	uint8_t	     address_space;
#elif FLEXUS_TARGET == FLEXUS_TARGET_x86
	processor_mode_t mode;
#endif
};
typedef struct memory_transaction memory_transaction_t;

typedef enum {
	QEMU_DI_Instruction,
	QEMU_DI_Data
} data_or_instr_t;


// read an arbitrary register.
uint64_t QEMU_read_register(conf_object_t *cpu, int reg_index);
// read an arbitrary physical memory address.
uint32_t QEMU_read_phys_memory(conf_object_t *cpu, 
								physical_address_t pa, int bytes);
// return a conf_object_t of the cpu in question.
conf_object_t *QEMU_get_cpu_by_index(int index);
// set the frequency of a given cpu.
int QEMU_set_tick_frequency(conf_object_t *cpu, double tick_freq);
// get the program counter of a given cpu.
uint64_t QEMU_get_program_counter(conf_object_t *cpu);
// convert a logical address to a physical address.
physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va);
void QEMU_simulation_break(void);
// determine the memory operation type by the transaction struct.
int QEMU_mem_op_is_data(generic_transaction_t *mop);
int QEMU_mem_op_is_write(generic_transaction_t *mop);
int QEMU_mem_op_is_read(generic_transaction_t *mop);

// insert callback to given event. 
typedef void (*QEMU_callback_t)(void *);

typedef enum {
	QEMU_config_ready,
	QEMU_mem_op,
	QEMU_dma_op,
	QEMU_exec_instr,
	QEMU_callback_event_count // MUST BE LAST.
} QEMU_callback_event_t;

struct QEMU_callback_container {
	uint64_t id;
	QEMU_callback_t callback;
	void *callback_arg;
	struct QEMU_callback_container *next;
};
typedef struct QEMU_callback_container QEMU_callback_container_t;

struct QEMU_callback_table {
	uint64_t next_callback_id;
	QEMU_callback_container_t *callbacks[QEMU_callback_event_count];
};

// global callback hash table
struct QEMU_callback_table QEMU_all_callbacks = {0, {NULL}};

int QEMU_insert_callback(QEMU_callback_event_t, QEMU_callback_t, void *arg);
