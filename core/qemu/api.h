#ifndef QEMU_API_H
#define QEMU_API_H
#include <inttypes.h>
#include <stdlib.h>

///
/// Data structures declarations
///

typedef uint64_t cycles_t;
typedef uint64_t physical_address_t;
typedef uint64_t logical_address_t;
typedef void* conf_class_t;
typedef int exception_type_t;

// things for api.c:

struct conf_object {
	void *object; // pointer to the struct in question
	enum { // what kind of QEMU struct does it represent
		QEMU_CPUState, // add new types as necessary
		QEMU_AddressSpace,
		QEMU_NetworkDevice
	} type;
};
typedef struct conf_object conf_object_t;
typedef conf_object_t processor_t;

typedef enum {
	QEMU_Class_Kind_Vanilla,
	QEMU_Class_Kind_Pseudo,
	QEMU_Class_Kind_Session
} class_data_t;

typedef enum {
	QEMU_Set_Ok
} set_error_t;

typedef enum {
	QEMU_Trans_Load,
	QEMU_Trans_Store,
	QEMU_Trans_Instr_Fetch,
	QEMU_Trans_Prefetch,
	QEMU_Trans_Cache
} mem_op_type_t;

typedef enum {
	QEMU_PE_No_Exception = 1025,
	QEMU_PE_Code_Break,
	QEMU_PE_Silent_Break,
	QEMU_PE_Inquiry_Outside_Memory,
	QEMU_PE_Inquiry_Unhandled,
	QEMU_PE_IO_Not_Taken,
	QEMU_PE_IO_Error,
	QEMU_PE_Interrupt_Break,
	QEMU_PE_Interrupt_Break_Take_Now,
	QEMU_PE_Exception_Break,
	QEMU_PE_Hap_Exception_Break,
	QEMU_PE_Stall_Cpu,
	QEMU_PE_Locked_Memory,
	QEMU_PE_Return_Break,
	QEMU_PE_Instruction_Finished,
	QEMU_PE_Default_Semantics,
	QEMU_PE_Ignore_Semantics,
	QEMU_PE_Speculation_Failed,
	QEMU_PE_Invalid_Address,
	QEMU_PE_MAI_Return,
	QEMU_PE_Last
} pseudo_exceptions_t;

typedef enum {
	QEMU_Initiator_Illegal         = 0x0,    /* catch uninitialized */
	QEMU_Initiator_CPU             = 0x1000,
	QEMU_Initiator_CPU_V9          = 0x1100,
	QEMU_Initiator_CPU_UII         = 0x1101,
	QEMU_Initiator_CPU_UIII        = 0x1102,
	QEMU_Initiator_CPU_UIV         = 0x1103,
	QEMU_Initiator_CPU_UT1         = 0x1104, /* 1105, 1106 internal */
	QEMU_Initiator_CPU_X86         = 0x1200,
	QEMU_Initiator_CPU_PPC         = 0x1300,
	QEMU_Initiator_CPU_Alpha       = 0x1400,
	QEMU_Initiator_CPU_IA64        = 0x1500,
	QEMU_Initiator_CPU_MIPS        = 0x1600,
	QEMU_Initiator_CPU_MIPS_RM7000 = 0x1601,
	QEMU_Initiator_CPU_MIPS_E9000  = 0x1602,
	QEMU_Initiator_CPU_ARM         = 0x1700,
	QEMU_Initiator_Device          = 0x2000,
	QEMU_Initiator_PCI_Device      = 0x2010,
	QEMU_Initiator_Cache           = 0x3000, /* The transaction is a cache
											   transaction as defined by
											   g-cache */
	QEMU_Initiator_Other           = 0x4000  /* initiator == NULL */
} ini_type_t;

typedef enum {
  QEMU_Non_Branch = 0,
  QEMU_Conditional_Branch = 1,
  QEMU_Unconditional_Branch = 2,
  QEMU_Call_Branch = 3,
  QEMU_Return_Branch = 4,
  QEMU_Last_Branch_Type = 5,
  QEMU_BRANCH_TYPE_COUNT
} branch_type_t;

struct generic_transaction {
        void *cpu_state;// (CPUState*) state of the CPU source of the transaction
	conf_object_t *ini_ptr; // note: for efficiency, arrange struct from
	char *real_address;     // largest datatype to smallest to avoid
	uint64_t bytes;         // unnecessary padding.
        logical_address_t pc; // QEMU pc not updated regularly, need to send pc
	logical_address_t logical_address;
	physical_address_t physical_address;
	size_t size;
	mem_op_type_t type;
	ini_type_t ini_type;
	exception_type_t exception;
        branch_type_t branch_type;
        unsigned int annul : 1;// annul the delay slot or not
	unsigned int atomic:1;
	unsigned int inquiry:1;
	unsigned int may_stall:1;
	unsigned int speculative:1;
	unsigned int ignore:1;
};
typedef struct generic_transaction generic_transaction_t;

typedef enum {
	QEMU_Val_Invalid  = 0,
	QEMU_Val_String   = 1,
	QEMU_Val_Integer  = 2,
	QEMU_Val_Floating = 3,
	QEMU_Val_Nil      = 6,
	QEMU_Val_Object   = 7,
	QEMU_Val_Boolean  = 9
} attr_kind_t;

struct attr_value {
	attr_kind_t kind;
	union {
		const char *string;
		int64_t integer;
		int64_t boolean;
		double floating;
		conf_object_t *object;
	} u;
};
typedef struct attr_value attr_value_t;

// part of MAI
typedef enum instruction_error {
	QEMU_IE_OK = 0,
	QEMU_IE_Unresolved_Dependencies,
	QEMU_IE_Speculative,
	QEMU_IE_Stalling,
	QEMU_IE_Not_Inserted,      /* trying to execute or squash an 
							 instruction that is inserted. */ 
	QEMU_IE_Exception,         /* (SPARC-V9 only) */
	QEMU_IE_Fault = QEMU_IE_Exception, 
	QEMU_IE_Trap,              /* (X86 only) Returned if a trap is 
							 encountered */
	QEMU_IE_Interrupt,         /* (X86 only) Returned if an interrupt is 
							 waiting and interrupts are enabled */

	QEMU_IE_Sync_Instruction,  /* Returned if sync instruction is 
							 not allowd to execute */
	QEMU_IE_No_Exception,      /* Returned by QEMU_instruction_
							 handle_exception */
	QEMU_IE_Illegal_Interrupt_Point,
	QEMU_IE_Illegal_Exception_Point,
	QEMU_IE_Illegal_Address,
	QEMU_IE_Illegal_Phase,
	QEMU_IE_Interrupts_Disabled,
	QEMU_IE_Illegal_Id,
	QEMU_IE_Instruction_Tree_Full,
	QEMU_IE_Null_Pointer,
	QEMU_IE_Illegal_Reg,
	QEMU_IE_Invalid,
	QEMU_IE_Out_of_Order_Commit, 
	QEMU_IE_Retired_Instruction, /* try to squash a retiring instruction */
	QEMU_IE_Not_Committed,       /* Returned by QEMU_instruction_end */
	QEMU_IE_Code_Breakpoint,
	QEMU_IE_Mem_Breakpoint,
	QEMU_IE_Step_Breakpoint,
	QEMU_IE_Hap_Breakpoint
} instruction_error_t;

typedef enum {
	QEMU_CPU_Mode_User,
	QEMU_CPU_Mode_Supervisor,
	QEMU_CPU_Mode_Hypervisor
} processor_mode_t;

typedef enum {
  QEMU_Instruction_Cache = 1,
  QEMU_Data_Cache = 2,
  QEMU_Prefetch_Buffer = 4
} cache_type_t;

typedef enum {
  QEMU_Invalidate_Cache = 0,
  QEMU_Clean_Cache = 1,
  QEMU_Flush_Cache = 2,
  QEMU_Prefetch_Cache = 3
} cache_maintenance_op_t;

typedef struct set_and_way_data{
  uint32_t set;
  uint32_t way;
} set_and_way_data_t;

/* little structure to hold an address range */
/* it is inclusive: if start == end, the range contains start */
typedef struct address_range{
  physical_address_t start_paddr;
  physical_address_t end_paddr;
} address_range_t;

typedef struct memory_transaction_sparc_specific {
	unsigned int cache_virtual:1;
	unsigned int cache_physical:1;
	unsigned int priv:1;
	uint8_t	     address_space;
	uint8_t        prefetch_fcn;
} memory_transaction_sparc_specific_t;

typedef struct memory_transaction_i386_specific {
  processor_mode_t mode;
} memory_transaction_i386_specific_t;

typedef struct memory_transaction_arm_specific {
  // wether or not the transaction comes from the user
  unsigned int user:1;
} memory_transaction_arm_specific_t;

typedef struct memory_transaction {
  generic_transaction_t s;
  unsigned int io:1;
  cache_type_t cache;// cache to operate on
  cache_maintenance_op_t cache_op;// operation to perform on cache
  int line:1; // 1 for line, 0 for whole cache
  int data_is_set_and_way : 1;// wether or not the operation provides set&way or address (range)
  union{
    set_and_way_data_t set_and_way;
    address_range_t addr_range;// same start and end addresses for not range operations
  };

  union{
    memory_transaction_sparc_specific_t sparc_specific;
    memory_transaction_i386_specific_t i386_specific;
    memory_transaction_arm_specific_t arm_specific;
  };
} memory_transaction_t;

typedef enum {
	QEMU_DI_Instruction,
	QEMU_DI_Data
} data_or_instr_t;

///
/// State interaction API
///
/// Get/set machine state from the Flexus.
///

typedef void (*CPU_READ_REGISTER_PROC)(void* env_ptr, int reg_index, unsigned *reg_size, void *data_out);
typedef physical_address_t (*MMU_LOGICAL_TO_PHYSICAL_PROC)(void *cs, logical_address_t va);
typedef uint64_t (*CPU_GET_PROGRAM_COUNTER_PROC)(void* cs);
typedef void* (*CPU_GET_ADDRESS_SPACE_PROC)(void* cs);
typedef int (*CPU_PROC_NUM_PROC)(void* cs);
typedef void (*CPU_POP_INDEXES_PROC)(int* indexes);
typedef conf_object_t* (*QEMU_GET_PHYS_MEMORY_PROC)(conf_object_t* cpu);
typedef conf_object_t* (*QEMU_GET_ETHERNET_PROC)(void);
typedef int (*QEMU_CLEAR_EXCEPTION_PROC)(void);
typedef void (*QEMU_READ_REGISTER_PROC)(conf_object_t *cpu, int reg_index, unsigned* reg_size, void* data_out);
typedef uint64_t (*QEMU_READ_PHYS_MEMORY_PROC)(conf_object_t *cpu, physical_address_t pa, int bytes);
typedef conf_object_t *(*QEMU_GET_PHYS_MEM_PROC)(conf_object_t *cpu);
typedef conf_object_t* (*QEMU_GET_CPU_BY_INDEX_PROC)(int index);
typedef int (*QEMU_GET_PROCESSOR_NUMBER_PROC)(conf_object_t *cpu);
typedef uint64_t (*QEMU_STEP_COUNT_PROC)(conf_object_t *cpu);
typedef int (*QEMU_GET_NUM_CPUS_PROC)(void);
typedef int (*QEMU_GET_NUM_SOCKETS_PROC)(void);
typedef int (*QEMU_GET_NUM_CORES_PROC)(void);
typedef int (*QEMU_GET_NUM_THREADS_PER_CORE_PROC)(void);
typedef int (*QEMU_CPU_GET_SOCKET_ID_PROC)(conf_object_t *cpu);
typedef int (*QEMU_CPU_GET_CORE_ID_PROC)(conf_object_t *cpu);
typedef int (*QEMU_CPU_GET_THREAD_ID_PROC)(conf_object_t *cpu);
typedef conf_object_t* (*QEMU_GET_ALL_PROCESSORS_PROC)(int *numCPUS);
typedef int (*QEMU_SET_TICK_FREQUENCY_PROC)(conf_object_t *cpu, double tick_freq);
typedef double (*QEMU_GET_TICK_FREQUENCY_PROC)(conf_object_t *cpu);
typedef uint64_t (*QEMU_GET_PROGRAM_COUNTER_PROC)(conf_object_t *cpu);
typedef physical_address_t (*QEMU_LOGICAL_TO_PHYSICAL_PROC)(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va);
typedef void (*QEMU_BREAK_SIMULATION_PROC)(const char* msg);
typedef void (*QEMU_FLUSH_ALL_CACHES_PROC)(void);
typedef int (*QEMU_MEM_OP_IS_DATA_PROC)(generic_transaction_t *mop);
typedef int (*QEMU_MEM_OP_IS_WRITE_PROC)(generic_transaction_t *mop);
typedef int (*QEMU_MEM_OP_IS_READ_PROC)(generic_transaction_t *mop);

//ALEX - begin 
////For Timing (mai) 
typedef instruction_error_t (*QEMU_INSTRUCTION_HANDLE_INTERRUPT_PROC)(conf_object_t *cpu, pseudo_exceptions_t pendingInterrupt);
typedef int (*QEMU_GET_PENDING_EXCEPTION_PROC)(void);
typedef int (*QEMU_ADVANCE_PROC)(void);
////ALEX - end 

#ifndef QEMUFLEX_PROTOTYPES
extern CPU_READ_REGISTER_PROC cpu_read_register;
extern MMU_LOGICAL_TO_PHYSICAL_PROC mmu_logical_to_physical;
extern CPU_GET_PROGRAM_COUNTER_PROC cpu_get_program_counter;
extern CPU_GET_ADDRESS_SPACE_PROC cpu_get_address_space;
extern CPU_PROC_NUM_PROC cpu_proc_num;
extern CPU_POP_INDEXES_PROC cpu_pop_indexes;
extern QEMU_GET_PHYS_MEMORY_PROC QEMU_get_phys_memory;
extern QEMU_GET_ETHERNET_PROC QEMU_get_ethernet;
extern QEMU_CLEAR_EXCEPTION_PROC QEMU_clear_exception;
extern QEMU_READ_REGISTER_PROC QEMU_read_register;
extern QEMU_READ_PHYS_MEMORY_PROC QEMU_read_phys_memory;
extern QEMU_GET_PHYS_MEM_PROC QEMU_get_phys_mem;
extern QEMU_GET_CPU_BY_INDEX_PROC QEMU_get_cpu_by_index;
extern QEMU_GET_PROCESSOR_NUMBER_PROC QEMU_get_processor_number;
extern QEMU_STEP_COUNT_PROC QEMU_step_count;
extern QEMU_GET_NUM_CPUS_PROC QEMU_get_num_cpus;

// return the number of sockets on he motherboard
extern QEMU_GET_NUM_SOCKETS_PROC QEMU_get_num_sockets;

// returns the number of cores per CPU socket
extern QEMU_GET_NUM_CORES_PROC QEMU_get_num_cores;

// return the number of native threads per core
extern QEMU_GET_NUM_THREADS_PER_CORE_PROC QEMU_get_num_threads_per_core;

// return the id of the socket of the processor
extern QEMU_CPU_GET_SOCKET_ID_PROC QEMU_cpu_get_socket_id;

// return the core id of the processor
extern QEMU_CPU_GET_CORE_ID_PROC QEMU_cpu_get_core_id;

// return the hread id of the processor
extern QEMU_CPU_GET_THREAD_ID_PROC QEMU_cpu_get_thread_id;

// return an array of all processors
// (numSockets * numCores * numthreads CPUs)
extern QEMU_GET_ALL_PROCESSORS_PROC QEMU_get_all_processors;

// set the frequency of a given cpu.
extern QEMU_SET_TICK_FREQUENCY_PROC QEMU_set_tick_frequency;

// get freq of given cpu
extern QEMU_GET_TICK_FREQUENCY_PROC QEMU_get_tick_frequency;

// get the program counter of a given cpu.
extern QEMU_GET_PROGRAM_COUNTER_PROC QEMU_get_program_counter;

// convert a logical address to a physical address.
extern QEMU_LOGICAL_TO_PHYSICAL_PROC QEMU_logical_to_physical;

extern QEMU_BREAK_SIMULATION_PROC QEMU_break_simulation;

// dummy function at the moment. should flush the translation cache.
extern QEMU_FLUSH_ALL_CACHES_PROC QEMU_flush_all_caches;

// determine the memory operation type by the transaction struct.
//[???]I assume return true if it is data, false otherwise
extern QEMU_MEM_OP_IS_DATA_PROC QEMU_mem_op_is_data;

//[???]I assume return true if it is write, false otherwise
extern QEMU_MEM_OP_IS_WRITE_PROC QEMU_mem_op_is_write;

//[???]I assume return true if it is read, false otherwise
extern QEMU_MEM_OP_IS_READ_PROC QEMU_mem_op_is_read;

extern QEMU_INSTRUCTION_HANDLE_INTERRUPT_PROC QEMU_instruction_handle_interrupt;
extern QEMU_GET_PENDING_EXCEPTION_PROC QEMU_get_pending_exception;
extern QEMU_ADVANCE_PROC QEMU_advance;
#else /* QEMUFLEX_PROTOTYPES */
// query the content/size of a register
// if reg_size != NULL, write the size of the register (in bytes) in reg_size
// if data_out != NULL, write the content of the register in data_out
void cpu_read_register( void *env_ptr, int reg_index, unsigned *reg_size, void *data_out );

physical_address_t mmu_logical_to_physical(void *cs, logical_address_t va);
uint64_t cpu_get_program_counter(void *cs);
void* cpu_get_address_space(void *cs);
int cpu_proc_num(void *cs);
void cpu_pop_indexes(int *indexes);
conf_object_t *QEMU_get_phys_memory(conf_object_t *cpu);
conf_object_t *QEMU_get_ethernet(void);


//[???]based on name it clears exceptions
int QEMU_clear_exception(void);

// read an arbitrary register.
// query the content/size of a register
// if reg_size != NULL, write the size of the register (in bytes) in reg_size
// if data_out != NULL, write the content of the register in data_out
void QEMU_read_register(conf_object_t *cpu,
			int reg_index,
			unsigned *reg_size,
			void *data_out);

// read an arbitrary physical memory address.
uint64_t QEMU_read_phys_memory(conf_object_t *cpu, 
								physical_address_t pa, int bytes);
// get the physical memory for a given cpu.
conf_object_t *QEMU_get_phys_mem(conf_object_t *cpu);
// return a conf_object_t of the cpu in question.
conf_object_t *QEMU_get_cpu_by_index(int index);

// return an int specifying the processor number of the cpu.
int QEMU_get_processor_number(conf_object_t *cpu);

// how many instructions have been executed since the start of QEMU for a CPU
uint64_t QEMU_step_count(conf_object_t *cpu);

// return an array of all processorsthe totaltthe total number of processors
// (numSockets * numCores * numthreads CPUs)
int QEMU_get_num_cpus(void);

// return the number of sockets on he motherboard
int QEMU_get_num_sockets(void);

// returns the number of cores per CPU socket
int QEMU_get_num_cores(void);

// return the number of native threads per core
int QEMU_get_num_threads_per_core(void);

// return the id of the socket of the processor
int QEMU_cpu_get_socket_id(conf_object_t *cpu);

// return the core id of the processor
int QEMU_cpu_get_core_id(conf_object_t *cpu);

// return the hread id of the processor
int QEMU_cpu_get_thread_id(conf_object_t *cpu);

// return an array of all processors
// (numSockets * numCores * numthreads CPUs)
conf_object_t *QEMU_get_all_processors(int *numCPUs);

// set the frequency of a given cpu.
int QEMU_set_tick_frequency(conf_object_t *cpu, double tick_freq);
// get freq of given cpu
double QEMU_get_tick_frequency(conf_object_t *cpu);
// get the program counter of a given cpu.
uint64_t QEMU_get_program_counter(conf_object_t *cpu);
// convert a logical address to a physical address.
physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va);
void QEMU_break_simulation(const char *msg);
// dummy function at the moment. should flush the translation cache.
void QEMU_flush_all_caches(void);
// determine the memory operation type by the transaction struct.
//[???]I assume return true if it is data, false otherwise
int QEMU_mem_op_is_data(generic_transaction_t *mop);
//[???]I assume return true if it is write, false otherwise
int QEMU_mem_op_is_write(generic_transaction_t *mop);
//[???]I assume return true if it is read, false otherwise
int QEMU_mem_op_is_read(generic_transaction_t *mop);

//ALEX - begin 
////For Timing (mai) 
instruction_error_t QEMU_instruction_handle_interrupt(conf_object_t *cpu, pseudo_exceptions_t pendingInterrupt); 
int QEMU_get_pending_exception(); 
int QEMU_advance(); 
////ALEX - end 

#endif /* QEMUFLEX_PROTYPES */

///
/// Callback API
///
/// The callback API is used to send event notifications
/// to the Flexus when using trace mode.
///

// callback function types.
// 
// naming convention:
// ------------------
// i - int
// I - int64_t
// e - exception_type_t
// o - class data (void*)
// s - string
// m - generic_transaction_t*
// c - conf_object_t*
// v - void*
typedef void (*cb_func_void)(void);
typedef void (*cb_func_noc_t)(void *, conf_object_t *);
typedef void (*cb_func_noc_t2)(void*, void *, conf_object_t *);
typedef void (*cb_func_nocI_t)(void *, conf_object_t *, int64_t);
typedef void (*cb_func_nocI_t2)(void*, void *, conf_object_t *, int64_t);
typedef void (*cb_func_nocIs_t)(void *, conf_object_t *, int64_t, char *);
typedef void (*cb_func_nocIs_t2)(void *, void *, conf_object_t *, int64_t, char *);
typedef void (*cb_func_noiiI_t)(void *, int, int, int64_t);
typedef void (*cb_func_noiiI_t2)(void *, void *, int, int, int64_t);

typedef void (*cb_func_ncm_t)(
		  conf_object_t *
		, memory_transaction_t *
		);
typedef void (*cb_func_ncm_t2)(
          void *
		, conf_object_t *
		, memory_transaction_t *
		);
typedef void (*cb_func_nocs_t)(void *, conf_object_t *, char *);
typedef void (*cb_func_nocs_t2)(void *, void *, conf_object_t *, char *);


typedef struct {
	void *class_data;
	conf_object_t *obj;
} QEMU_noc;

typedef struct {
	void *class_data;
	conf_object_t *obj;
	int64_t bigint;
} QEMU_nocI;

typedef struct {
	void *class_data;
	conf_object_t *obj;
	char *string;
	int64_t bigint;
} QEMU_nocIs;

typedef struct {
	void *class_data;
	int integer0;
	int integer1;
	int64_t bigint;
} QEMU_noiiI;

typedef struct {
	conf_object_t *space;
	memory_transaction_t *trans;
} QEMU_ncm;

typedef struct {
	void *class_data;
	conf_object_t *obj;
	char *string;
} QEMU_nocs;
typedef union {
	QEMU_noc	*noc;
	QEMU_nocIs	*nocIs;
	QEMU_nocI   *nocI;
	QEMU_noiiI	*noiiI;
	QEMU_nocs	*nocs;
	QEMU_ncm	*ncm;
} QEMU_callback_args_t;

typedef enum {
	QEMU_config_ready,
    QEMU_continuation,
    QEMU_simulation_stopped,
    QEMU_asynchronous_trap,
    QEMU_exception_return,
    QEMU_magic_instruction,
    QEMU_ethernet_frame,
    QEMU_ethernet_network_frame,
    QEMU_periodic_event,
    QEMU_xterm_break_string,
    QEMU_gfx_break_string,
    QEMU_cpu_mem_trans,
	QEMU_dma_mem_trans,
    QEMU_callback_event_count // MUST BE LAST.
} QEMU_callback_event_t;

struct QEMU_callback_container {
	uint64_t id;
	void *obj;
	void *callback;
	struct QEMU_callback_container *next;
};
typedef struct QEMU_callback_container QEMU_callback_container_t;

struct QEMU_callback_table {
	uint64_t next_callback_id;
	QEMU_callback_container_t *callbacks[QEMU_callback_event_count];
};
typedef struct QEMU_callback_table QEMU_callback_table_t;

#define QEMUFLEX_GENERIC_CALLBACK -1

typedef int (*QEMU_INSERT_CALLBACK_PROC)( int cpu_id, QEMU_callback_event_t event, void* obj, void* fun);
typedef void (*QEMU_DELETE_CALLBACK_PROC)( int cpu_id, QEMU_callback_event_t event, uint64_t callback_id);

#ifndef QEMUFLEX_PROTOTYPES
// insert a callback specific for the given cpu or -1 for a generic callback
extern QEMU_INSERT_CALLBACK_PROC QEMU_insert_callback;

// delete a callback specific for the given cpu or -1 for a generic callback
extern QEMU_DELETE_CALLBACK_PROC QEMU_delete_callback;
#else
// insert a callback specific for the given cpu or -1 for a generic callback
int QEMU_insert_callback( int cpu_id, QEMU_callback_event_t event, void* obj, void* fun);
// delete a callback specific for the given cpu or -1 for a generic callback
void QEMU_delete_callback( int cpu_id, QEMU_callback_event_t event, uint64_t callback_id);
#endif /* QEMUFLEX_PROTYPES */

///
/// QEMU QEMUFLEX internals
///

#ifdef QEMUFLEX_QEMU_INTERNAL
// Initialize the callback tables for every processor
// Must be called at QEMU startup, before initializing Flexus
void QEMU_setup_callback_tables(void);
// Free the allocated memory for the callback tables
void QEMU_free_callback_tables(void);
// execute a callback trigered by the given cpu id or -1 for a generic callback
void QEMU_execute_callbacks(
		  int cpu_id,
		  QEMU_callback_event_t event,
		  QEMU_callback_args_t *event_data
		);
#endif /* QEMUFLEX_QEMU_INTERNAL */

///
/// Simulator interface function passing
///
typedef struct QFLEX_API_Interface_Hooks {
CPU_READ_REGISTER_PROC cpu_read_register;
MMU_LOGICAL_TO_PHYSICAL_PROC mmu_logical_to_physical;
CPU_GET_PROGRAM_COUNTER_PROC cpu_get_program_counter;
CPU_GET_ADDRESS_SPACE_PROC cpu_get_address_space;
CPU_PROC_NUM_PROC cpu_proc_num;
CPU_POP_INDEXES_PROC cpu_pop_indexes;
QEMU_GET_PHYS_MEMORY_PROC QEMU_get_phys_memory;
QEMU_GET_ETHERNET_PROC QEMU_get_ethernet;
QEMU_CLEAR_EXCEPTION_PROC QEMU_clear_exception;
QEMU_READ_REGISTER_PROC QEMU_read_register;
QEMU_READ_PHYS_MEMORY_PROC QEMU_read_phys_memory;
QEMU_GET_PHYS_MEM_PROC QEMU_get_phys_mem;
QEMU_GET_CPU_BY_INDEX_PROC QEMU_get_cpu_by_index;
QEMU_GET_PROCESSOR_NUMBER_PROC QEMU_get_processor_number;
QEMU_STEP_COUNT_PROC QEMU_step_count;
QEMU_GET_NUM_CPUS_PROC QEMU_get_num_cpus;

// return the number of sockets on he motherboard
QEMU_GET_NUM_SOCKETS_PROC QEMU_get_num_sockets;

// returns the number of cores per CPU socket
QEMU_GET_NUM_CORES_PROC QEMU_get_num_cores;

// return the number of native threads per core
QEMU_GET_NUM_THREADS_PER_CORE_PROC QEMU_get_num_threads_per_core;

// return the id of the socket of the processor
QEMU_CPU_GET_SOCKET_ID_PROC QEMU_cpu_get_socket_id;

// return the core id of the processor
QEMU_CPU_GET_CORE_ID_PROC QEMU_cpu_get_core_id;

// return the hread id of the processor
QEMU_CPU_GET_THREAD_ID_PROC QEMU_cpu_get_thread_id;

// return an array of all processors
// (numSockets * numCores * numthreads CPUs)
QEMU_GET_ALL_PROCESSORS_PROC QEMU_get_all_processors;

// set the frequency of a given cpu.
QEMU_SET_TICK_FREQUENCY_PROC QEMU_set_tick_frequency;

// get freq of given cpu
QEMU_GET_TICK_FREQUENCY_PROC QEMU_get_tick_frequency;

// get the program counter of a given cpu.
QEMU_GET_PROGRAM_COUNTER_PROC QEMU_get_program_counter;

// convert a logical address to a physical address.
QEMU_LOGICAL_TO_PHYSICAL_PROC QEMU_logical_to_physical;

QEMU_BREAK_SIMULATION_PROC QEMU_break_simulation;

// dummy function at the moment. should flush the translation cache.
QEMU_FLUSH_ALL_CACHES_PROC QEMU_flush_all_caches;

// determine the memory operation type by the transaction struct.
//[???]I assume return true if it is data, false otherwise
QEMU_MEM_OP_IS_DATA_PROC QEMU_mem_op_is_data;

//[???]I assume return true if it is write, false otherwise
QEMU_MEM_OP_IS_WRITE_PROC QEMU_mem_op_is_write;

//[???]I assume return true if it is read, false otherwise
QEMU_MEM_OP_IS_READ_PROC QEMU_mem_op_is_read;

QEMU_INSTRUCTION_HANDLE_INTERRUPT_PROC QEMU_instruction_handle_interrupt;
QEMU_GET_PENDING_EXCEPTION_PROC QEMU_get_pending_exception;
QEMU_ADVANCE_PROC QEMU_advance;

// insert a callback specific for the given cpu or -1 for a generic callback
QEMU_INSERT_CALLBACK_PROC QEMU_insert_callback;

// delete a callback specific for the given cpu or -1 for a generic callback
QEMU_DELETE_CALLBACK_PROC QEMU_delete_callback;
} QFLEX_API_Interface_Hooks_t;


#ifdef QEMUFLEX_QEMU_INTERNAL
void QFLEX_API_get_interface_hooks(QFLEX_API_Interface_Hooks_t* hooks);
#endif /* QEMUFLEX_QEMU_INTERNAL */


#ifdef QEMUFLEX_FLEXUS_INTERNAL
void QFLEX_API_set_interface_hooks( const QFLEX_API_Interface_Hooks_t* hooks );
#endif /* QEMUFLEX_FLEXUS_INTERNAL */

#endif
