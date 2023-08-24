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
#ifndef QEMU_API_H
#define QEMU_API_H
#include <inttypes.h>
#include <stdbool.h>

#define OS_INSTR 0
#define USER_INSTR 1
#define BOTH_INSTR 2

typedef uint64_t cycles_t;
typedef uint64_t physical_address_t;
typedef uint64_t logical_address_t;
typedef void* conf_class_t;
typedef int exception_type_t;


/*---------------------------------------------------------------
 *-------------------------ENUMERATIONS----------------------------
 *---------------------------------------------------------------*/

#ifndef MemoryAccessType
// See cpu.h to match MMUAccessType
typedef enum MemoryAccessType {
    DATA_LOAD  = 0,
    DATA_STORE = 1,
    INST_FETCH = 2
} MemoryAccessType;
#define MemoryAccessType
#endif

typedef enum {
     QMP_FLEXUS_PRINTCYCLECOUNT = 0,
     QMP_FLEXUS_SETSTOPCYCLE,
     QMP_FLEXUS_SETSTATINTERVAL,
     QMP_FLEXUS_SETREGIONINTERVAL,
     QMP_FLEXUS_SETBREAKCPU,
     QMP_FLEXUS_SETBREAKINSN,
     QMP_FLEXUS_SETPROFILEINTERVAL,
     QMP_FLEXUS_SETTIMESTAMPINTERVAL,
     QMP_FLEXUS_PRINTPROFILE,
     QMP_FLEXUS_RESETPROFILE,
     QMP_FLEXUS_WRITEPROFILE,
     QMP_FLEXUS_PRINTCONFIGURATION,
     QMP_FLEXUS_WRITECONFIGURATION,
     QMP_FLEXUS_PARSECONFIGURATION,
     QMP_FLEXUS_SETCONFIGURATION,
     QMP_FLEXUS_PRINTMEASUREMENT,
     QMP_FLEXUS_LISTMEASUREMENTS,
     QMP_FLEXUS_WRITEMEASUREMENT,
     QMP_FLEXUS_ENTERFASTMODE,
     QMP_FLEXUS_LEAVEFASTMODE,
     QMP_FLEXUS_QUIESCE,
     QMP_FLEXUS_DOLOAD,
     QMP_FLEXUS_DOSAVE,
     QMP_FLEXUS_BACKUPSTATS,
     QMP_FLEXUS_SAVESTATS,
     QMP_FLEXUS_RELOADDEBUGCFG,
     QMP_FLEXUS_ADDDEBUGCFG,
     QMP_FLEXUS_SETDEBUG,
     QMP_FLEXUS_ENABLECATEGORY,
     QMP_FLEXUS_DISABLECATEGORY,
     QMP_FLEXUS_LISTCATEGORIES,
     QMP_FLEXUS_ENABLECOMPONENT,
     QMP_FLEXUS_DISABLECOMPONENT,
     QMP_FLEXUS_LISTCOMPONENTS,
     QMP_FLEXUS_PRINTDEBUGCONFIGURATION,
     QMP_FLEXUS_WRITEDEBUGCONFIGURATION,
     QMP_FLEXUS_TERMINATESIMULATION,
     QMP_FLEXUS_LOG,
     QMP_FLEXUS_PRINTMMU,
} qmp_flexus_cmd_t;

typedef enum {
	QEMU_Class_Kind_Vanilla,
	QEMU_Class_Kind_Pseudo,
	QEMU_Class_Kind_Session
} class_data_t;


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

#define EL0 0
#define EL1 1
#define EL2 2
#define EL3 3

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
  QEMU_IndirectReg_Branch = 4,
  QEMU_IndirectCall_Branch = 5,
  QEMU_Return_Branch = 6,
  QEMU_Last_Branch_Type = 7,
  // (ISB) Instruction Synchronization Barrier flushes the pipeline in the processor, so that all
  // instructions following the ISB are fetched from cache or memory
  // (SB) Speculation Barrier is a barrier that controls speculation
  QEMU_Barrier_Branch = 8,
  QEMU_BRANCH_TYPE_COUNT
} branch_type_t;

typedef enum {
    FETCH_IO_MEM_OP = 0,
    FETCH_MEM_OP,
    NON_FETCH_IO_MEM_OP,
    NON_IO_MEM_OP,
    CPUMEMTRANS,
    ALL_CALLBACKS,
    ALL_GENERIC_CALLBACKS,
    NON_EXISTING_EVENT,
    USER_INSTR_CNT,
    OS_INSTR_CNT,
    ALL_INSTR_CNT,
    USER_FETCH_CNT,
    OS_FETCH_CNT,
    ALL_FETCH_CNT,
    LD_USER_CNT,
    LD_OS_CNT,
    LD_ALL_CNT,
    ST_USER_CNT,
    ST_OS_CNT,
    ST_ALL_CNT,
    CACHEOPS_USER_CNT,
    CACHEOPS_OS_CNT,
    CACHEOPS_ALL_CNT,
    NUM_TRANS_USER,
    NUM_TRANS_OS,
    NUM_TRANS_ALL,
    NUM_DMA_ALL,
    QEMU_CALLBACK_CNT,
    ALL_DEBUG_TYPE,
} debug_types;


typedef enum {
    QEMU_Val_Invalid  = 0,
    QEMU_Val_String   = 1,
    QEMU_Val_Integer  = 2,
    QEMU_Val_Floating = 3,
    QEMU_Val_Nil      = 6,
    QEMU_Val_Object   = 7,
    QEMU_Val_Boolean  = 9
} attr_kind_t;


typedef enum instruction_error {
    QEMU_IE_OK = 0,
    QEMU_IE_Unresolved_Dependencies,
    QEMU_IE_Speculative,
    QEMU_IE_Stalling,
    QEMU_IE_Not_Inserted,      /* trying to execute or squash an instruction that is inserted. */
    QEMU_IE_Exception,         /* (SPARC-V9 only) */
    QEMU_IE_Fault = QEMU_IE_Exception,
    QEMU_IE_Trap,              /* (X86 only) Returned if a trap is encountered */
    QEMU_IE_Interrupt,         /* (X86 only) Returned if an interrupt is waiting and interrupts are enabled */
    QEMU_IE_Sync_Instruction,  /* Returned if sync instruction is not allowd to execute */
    QEMU_IE_No_Exception,      /* Returned by QEMU_instruction_handle_exception */
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

typedef enum {
    QEMU_DI_Instruction,
    QEMU_DI_Data
} data_or_instr_t;

/*---------------------------------------------------------------
 *-------------------------STRUCTURES----------------------------
 *---------------------------------------------------------------*/

typedef struct conf_object {
    char *name;
	void *object; // pointer to the struct in question
	enum { // what kind of QEMU struct does it represent
		QEMU_CPUState, // add new types as necessary
		QEMU_NetworkDevice,
        QEMU_MMUObject,
        QEMU_DisasContext,
	} type;
}conf_object_t;
typedef conf_object_t processor_t;
typedef conf_object_t mmu_api_obj_t;

typedef struct exception_t{
    uint32_t syndrome; /* AArch64 format syndrome register */
    uint32_t fsr; /* AArch32 format fault status register info */
    uint64_t vaddress; /* virtual addr associated with exception, if any */
    uint32_t target_el; /* EL the exception should be targeted for */
    /* If we implement EL2 we will also need to store information
     * about the intermediate physical address for stage 2 faults.
     */
} exception_t;

typedef struct generic_transaction {
  logical_address_t pc; // QEMU pc not updated regularly, need to send pc
  logical_address_t logical_address;
  physical_address_t physical_address;
  unsigned size;
  mem_op_type_t type;
  ini_type_t ini_type;
  exception_type_t exception;
  branch_type_t branch_type;
  unsigned int annul : 1; // annul the delay slot or not
  unsigned int atomic : 1;
  unsigned int inquiry : 1;
  unsigned int may_stall : 1;
  unsigned int speculative : 1;
  unsigned int ignore : 1;
  unsigned int inverse_endian : 1;
} generic_transaction_t;

typedef struct set_and_way_data{
  uint32_t set;
  uint32_t way;
} set_and_way_data_t;

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

typedef struct attr_value {
    attr_kind_t kind;
    union {
        const char *string;
        int64_t integer;
        int64_t boolean;
        double floating;
        conf_object_t *object;
    } u;
}attr_value_t;

#ifdef FLEXUS_TARGET_ARM
typedef enum {
        ARM_Access_Normal,
        ARM_Access_Normal_FP,
        ARM_Access_Double_FP, /* ldd/std */
        ARM_Access_Short_FP,
        ARM_Access_FSR,
        ARM_Access_Atomic,
        ARM_Access_Atomic_Load,
        ARM_Access_Prefetch,
        ARM_Access_Partial_Store,
        ARM_Access_Ldd_Std_1,
        ARM_Access_Ldd_Std_2,
        ARM_Access_Block,
        ARM_Access_Internal1
} arm_access_type_t;

typedef struct armInterface {
    //This is the interface for a Sparc CPU in QEMU. The interface should provide the following functions:
    //uint64_t read_fp_register_x(conf_object_t *cpu, int reg)
    //void write_fp_register_x(conf_object_t *cpu, int reg, uint64 value);
    //uint64_t read_global_register(conf_object_t *cpu, int globals, int reg);
    //uint64_t read_window_register(conf_object_t *cpu, int window, int reg);
    //exception_type_t access_asi_handler(conf_object_t *cpu, v9_memory_transaction_t *mem_op);
} armInterface_t;

// Interface from libqflex back to the ARM MMU
typedef struct {
} mmu_interface_t;

typedef struct arm_memory_transaction {
        generic_transaction_t s;
        unsigned              cache_virtual:1;
        unsigned              cache_physical:1;
        unsigned              side_effect:1;
        unsigned              priv:1;
        unsigned              red:1;
        unsigned              hpriv:1;
        unsigned              henb:1;
        /* Because of a bug in the Sun Studio12 C compiler, bit fields must not
 *            be followed by members with alignment smaller than 32 bit.
 *                       See bug 9151. */
        uint32_t address_space;
        uint8_t                 prefetch_fcn;
        arm_access_type_t   access_type;

        /* if non-zero, the id needed to calculate the program counter */
        intptr_t turbo_miss_id;
} arm_memory_transaction_t;
#endif

/*---------------------------------------------------------------
 *-------------------------TYPEDEFS----------------------------
 *---------------------------------------------------------------*/


// registers
    //write
typedef void                (*QEMU_WRITE_REGISTER_PROC)         (conf_object_t *cpu, arm_register_t reg_type, int reg_index, uint64_t value);
    //read
typedef uint64_t            (*QEMU_READ_REGISTER_PROC)          (conf_object_t *cpu, arm_register_t reg_type, int reg_index, int el);
typedef uint64_t            (*QEMU_READ_UNHASHED_SYSREG_PROC)   (conf_object_t *cpu, uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm);

typedef uint32_t            (*QEMU_READ_PSTATE_PROC)             (conf_object_t *cpu);
typedef uint32_t            (*QEMU_READ_FPCR_PROC)               (conf_object_t *cpu);
typedef uint32_t            (*QEMU_READ_FPSR_PROC)               (conf_object_t *cpu);
typedef uint32_t            (*QEMU_READ_FPSR_PROC)               (conf_object_t *cpu);
typedef uint64_t            (*QEMU_READ_SP_EL_PROC)              (uint8_t Id, conf_object_t *cpu);

typedef uint64_t            (*QEMU_READ_SCTLR_PROC)              (uint8_t id, conf_object_t *cpu);
typedef bool                (*QEMU_GET_PENDING_INTERRUPT_PROC)   (conf_object_t *cpu);

//memory
typedef void*               (*QEMU_CPU_GET_ADDRESS_SPACE_PROC)  (void* cs);
typedef conf_object_t*      (*QEMU_GET_PHYS_MEMORY_PROC)        (conf_object_t* cpu);
typedef void                (*QEMU_READ_PHYS_MEMORY_PROC)       (uint8_t* buf, physical_address_t pa, int bytes);
typedef conf_object_t *     (*QEMU_GET_PHYS_MEM_PROC)           (conf_object_t *cpu);
typedef physical_address_t  (*QEMU_LOGICAL_TO_PHYSICAL_PROC)    (conf_object_t *cpu, data_or_instr_t fetch, logical_address_t va);

//cache
typedef int                 (*QEMU_MEM_OP_IS_DATA_PROC)         (generic_transaction_t *mop);
typedef int                 (*QEMU_MEM_OP_IS_WRITE_PROC)        (generic_transaction_t *mop);
typedef int                 (*QEMU_MEM_OP_IS_READ_PROC)         (generic_transaction_t *mop);

//cpu
typedef conf_object_t*      (*QEMU_GET_CPU_BY_INDEX_PROC)       (int index);
typedef int                 (*QEMU_GET_CPU_INDEX_PROC)          (conf_object_t *cpu);
typedef int                 (*QEMU_GET_NUM_CPUS_PROC)           (void);
typedef int                 (*QEMU_GET_NUM_SOCKETS_PROC)        (void);
typedef int                 (*QEMU_GET_NUM_CORES_PROC)          (void);
typedef int                 (*QEMU_GET_NUM_THREADS_PER_CORE_PROC)(void);
typedef int                 (*QEMU_CPU_GET_SOCKET_ID_PROC)      (conf_object_t *cpu);
typedef conf_object_t*      (*QEMU_GET_ALL_CPUS_PROC)           (void);
typedef int                 (*QEMU_CPU_EXEC_PROC)               (conf_object_t *cpu, bool count_time);

//qemu settings
typedef void                (*QEMU_CPU_SET_QUANTUM)             (const int *val);
typedef bool                (*QEMU_CPU_HAS_WORK_PROC)           ( conf_object_t* obj);


//arch
typedef uint64_t            (*QEMU_GET_PROGRAM_COUNTER_PROC)    (conf_object_t *cpu);
typedef uint64_t            (*CPU_GET_PROGRAM_COUNTER_PROC)     (void* cs);
typedef void                (*QEMU_FLUSH_TB_CACHE_PROC)         (void);
typedef int                 (*QEMU_SET_TICK_FREQUENCY_PROC)     (conf_object_t *cpu, double tick_freq);
typedef double              (*QEMU_GET_TICK_FREQUENCY_PROC)     (conf_object_t *cpu);

//stat
typedef void                (*QEMU_INCREMENT_DEBUG_STAT_PROC)   (int val);

//simulation
typedef bool                (*QEMU_BREAK_SIMULATION_PROC)       (const char* msg);
typedef void                (*QEMU_SET_SIMULATION_TIME_PROC)    (uint64_t time);
typedef int64_t             (*QEMU_GET_SIMULATION_TIME_PROC)    (void);
typedef int                 (*QEMU_ADVANCE_PROC)                (void);
typedef int                 (*QEMU_IS_IN_SIMULATION_PROC)       (void);
typedef void                (*QEMU_TOGGLE_SIMULATION_PROC)      (int enable);
typedef void                (*QEMU_DISASSEMBLE_PROC)            (conf_object_t* cpu, uint64_t addr, char **buf_ptr);
typedef void                (*QEMU_DUMP_STATE_PROC)             (conf_object_t* cpu, char **buf_ptr);
typedef conf_object_t*      (*QEMU_GET_ETHERNET_PROC)           (void);

//exp/irq
typedef int                 (*QEMU_CLEAR_EXCEPTION_PROC)        (void);
typedef instruction_error_t (*QEMU_INSTRUCTION_HANDLE_INTERRUPT_PROC)(conf_object_t *cpu, pseudo_exceptions_t pendingInterrupt);
typedef int                 (*QEMU_GET_PENDING_EXCEPTION_PROC)  (void);
typedef uint8_t             (*QEMU_GET_CURRENT_EL_PROC_PROC)    (conf_object_t* cpu);

//MMU
typedef conf_object_t* (*QEMU_GET_MMU_STATE_PROC)(int cpu_index);

//api
typedef conf_object_t*      (*QEMU_GET_OBJECT_BY_NAME_PROC)     (const char *name);
typedef uint64_t            (*QEMU_GET_INSTRUCTION_COUNT_PROC)  (int cpu_number, int isUser);
typedef uint64_t            (*QEMU_GET_INSTRUCTION_COUNT_PROC2) (int cpu_number, int isUser);
typedef uint64_t            (*QEMU_STEP_COUNT_PROC)             (conf_object_t *cpu);

// Callbacks for trace mode, QEMU -> QFlex
typedef void (*SIMULATOR_START_TIMING)(void);
typedef void (*SIMULATOR_SIM_QUIT)(void);
typedef void (*SIMULATOR_QMP)(qmp_flexus_cmd_t, const char *);
typedef void (*SIMULATOR_TRACE_MEM)(int, memory_transaction_t*);
typedef void (*SIMULATOR_TRACE_MEM_DMA)(memory_transaction_t*);
typedef void (*SIMULATOR_PERIODIC_EVENT)(void);
typedef void (*SIMULATOR_MAGIC_INST)(int, long long);
typedef void (*SIMULATOR_ETHERNET_FRAME)(int32_t, int32_t, long long);
typedef void (*SIMULATOR_XTERM_BREAK_STRING)(char *);
typedef struct QEMU_TO_QFLEX_CALLBACKS_t {
  SIMULATOR_START_TIMING start_timing;
  SIMULATOR_SIM_QUIT sim_quit;
  SIMULATOR_QMP qmp;
	SIMULATOR_TRACE_MEM trace_mem;
	SIMULATOR_TRACE_MEM_DMA trace_mem_dma;
	SIMULATOR_PERIODIC_EVENT periodic;
	SIMULATOR_MAGIC_INST magic_inst;
  SIMULATOR_ETHERNET_FRAME ethernet_frame;
  SIMULATOR_XTERM_BREAK_STRING xterm_break_string;
} QEMU_TO_QFLEX_CALLBACKS_t;

typedef struct QFLEX_TO_QEMU_API_t
{

    QEMU_CLEAR_EXCEPTION_PROC QEMU_clear_exception;
    QEMU_GET_PENDING_INTERRUPT_PROC QEMU_has_pending_irq;

    QEMU_WRITE_REGISTER_PROC QEMU_write_register;
    QEMU_READ_REGISTER_PROC QEMU_read_register;
    QEMU_READ_UNHASHED_SYSREG_PROC QEMU_read_unhashed_sysreg;
    QEMU_READ_PSTATE_PROC QEMU_read_pstate;
    QEMU_READ_SCTLR_PROC QEMU_read_sctlr;
    QEMU_CPU_HAS_WORK_PROC QEMU_cpu_has_work;
    QEMU_READ_SP_EL_PROC QEMU_read_sp_el;

    QEMU_READ_PHYS_MEMORY_PROC QEMU_read_phys_memory;
    QEMU_GET_CPU_BY_INDEX_PROC QEMU_get_cpu_by_index;
    QEMU_GET_CPU_INDEX_PROC QEMU_get_cpu_index;
    QEMU_STEP_COUNT_PROC QEMU_step_count;
    QEMU_GET_NUM_SOCKETS_PROC QEMU_get_num_sockets;
    QEMU_GET_NUM_CORES_PROC QEMU_get_num_cores;
    QEMU_GET_NUM_THREADS_PER_CORE_PROC QEMU_get_num_threads_per_core;
    QEMU_GET_ALL_CPUS_PROC QEMU_get_all_cpus;

    QEMU_SET_TICK_FREQUENCY_PROC QEMU_set_tick_frequency;
    QEMU_GET_TICK_FREQUENCY_PROC QEMU_get_tick_frequency;
    QEMU_GET_PROGRAM_COUNTER_PROC QEMU_get_program_counter;
    QEMU_LOGICAL_TO_PHYSICAL_PROC QEMU_logical_to_physical;
    QEMU_BREAK_SIMULATION_PROC QEMU_quit_simulation;
    QEMU_GET_SIMULATION_TIME_PROC QEMU_getCyclesLeft;

    QEMU_MEM_OP_IS_DATA_PROC QEMU_mem_op_is_data;
    QEMU_MEM_OP_IS_WRITE_PROC QEMU_mem_op_is_write;
    QEMU_MEM_OP_IS_READ_PROC QEMU_mem_op_is_read;

    QEMU_INSTRUCTION_HANDLE_INTERRUPT_PROC QEMU_instruction_handle_interrupt;
    QEMU_GET_OBJECT_BY_NAME_PROC QEMU_get_object_by_name;
    QEMU_CPU_EXEC_PROC QEMU_cpu_execute;
    QEMU_IS_IN_SIMULATION_PROC QEMU_is_in_simulation;
    QEMU_TOGGLE_SIMULATION_PROC QEMU_toggle_simulation;
    QEMU_FLUSH_TB_CACHE_PROC QEMU_flush_tb_cache;
    QEMU_GET_INSTRUCTION_COUNT_PROC QEMU_get_instruction_count;
    QEMU_DISASSEMBLE_PROC QEMU_disassemble;
    QEMU_DUMP_STATE_PROC QEMU_dump_state;

    QEMU_CPU_SET_QUANTUM QEMU_cpu_set_quantum;
    QEMU_INCREMENT_DEBUG_STAT_PROC QEMU_increment_debug_stat;
//    CPU_GET_PROGRAM_COUNTER_PROC cpu_get_program_counter;
//    QEMU_GET_MMU_STATE_PROC QEMU_get_mmu_state;
//    QEMU_CPU_GET_SOCKET_ID_PROC QEMU_cpu_get_socket_id;
} QFLEX_TO_QEMU_API_t;

#ifdef CONFIG_QFLEX
// Internal to qemu functions
void qflex_api_init(bool timing_mode, uint64_t sim_cycles);

// Exposed to QFlex simulator functions

void QEMU_disassemble                                       (conf_object_t* cpu, uint64_t addr, char **buf_ptr);
int QEMU_clear_exception                                    (void);
void QEMU_write_register                                    (conf_object_t *cpu, arm_register_t reg_type, int reg_index, uint64_t value);
uint64_t QEMU_read_register                                 (conf_object_t *cpu, arm_register_t reg_type, int reg_index, int el);
uint64_t QEMU_read_unhashed_sysreg                          (conf_object_t *cpu, uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crm, uint8_t crn);
uint64_t QEMU_read_sctlr                                    (uint8_t id, conf_object_t *cpu);
bool QEMU_has_pending_irq                                   (conf_object_t *cpu);
 
uint32_t QEMU_read_pstate                                   (conf_object_t *cpu);
uint64_t QEMU_read_sp_el                                    (uint8_t id, conf_object_t *cpu);

void QEMU_read_phys_memory                                  (uint8_t* buf, physical_address_t pa, int bytes);
conf_object_t *QEMU_get_cpu_by_index                        (int index);
int QEMU_get_cpu_index                                      (conf_object_t *cpu);
uint64_t QEMU_step_count                                    (conf_object_t *cpu);
int QEMU_get_num_sockets                                    (void);
int QEMU_get_num_cores                                      (void);
int QEMU_get_num_threads_per_core                           (void);
conf_object_t *QEMU_get_all_cpus                            (void);
void QEMU_cpu_set_quantum                                   (const int * val);
int QEMU_set_tick_frequency                                 (conf_object_t *cpu, double tick_freq);
double QEMU_get_tick_frequency                              (conf_object_t *cpu);
uint64_t QEMU_get_program_counter                           (conf_object_t *cpu);
void QEMU_increment_debug_stat                              (int val);
physical_address_t QEMU_logical_to_physical                 (conf_object_t *cpu, data_or_instr_t fetch, logical_address_t va);
bool QEMU_quit_simulation                                   (const char *msg);
int64_t QEMU_getCyclesLeft                                  (void);
void QEMU_setSimCyclesLength                                (uint64_t);
int QEMU_mem_op_is_data                                     (generic_transaction_t *mop);
int QEMU_mem_op_is_write                                    (generic_transaction_t *mop);
int QEMU_mem_op_is_read                                     (generic_transaction_t *mop);
instruction_error_t QEMU_instruction_handle_interrupt       (conf_object_t *cpu, pseudo_exceptions_t pendingInterrupt);
conf_object_t *QEMU_get_object_by_name                      (const char *name);
int QEMU_cpu_execute                                        (conf_object_t *cpu, bool count_time);
int QEMU_is_in_simulation                                   (void);
void QEMU_toggle_simulation                                 (int enable);
void  QEMU_cpu_executeation                                 (int enable);
void QEMU_flush_tb_cache                                    (void);
uint64_t QEMU_get_instruction_count                         (int cpu_number, int isUser);
uint64_t QEMU_get_total_instruction_count                   (void);
void QEMU_increment_instruction_count                       (int cpu_number, int isUser);
void QEMU_dump_state                                        (conf_object_t* cpu, char **buf_ptr);
bool QEMU_cpu_has_work                                      ( conf_object_t* obj);

void qflex_api_shutdown                                          (void);

extern QEMU_TO_QFLEX_CALLBACKS_t qflex_sim_callbacks;

#else
/*---------------------------------------------------------------
 *-------------------------FLEXUS----------------------------
 *---------------------------------------------------------------*/
extern QFLEX_TO_QEMU_API_t qemu_callbacks;

typedef enum {
  MagicIterationTracker,
  MagicTransactionTracker,
  MagicBreakpointTracker,
  MagicRegressionTracker,
  MagicSimPrintHandler,
  MagicConsoleStringTracker,
  MagicInstsTotalHooks,
} MagicInst_t;

// QEMU to QFlex callbacks for Trace mode
typedef struct callback_t {
  void *obj;
  void *fn;
} callback_t;

typedef struct qflex_sim_callbacks_t {
  callback_t start_timing;
  callback_t sim_quit;
  callback_t qmp;
  callback_t *trace_mem;
  callback_t trace_mem_dma;
  callback_t periodic;
  callback_t ethernet_frame;
  callback_t xterm_break_string;
  callback_t magic_inst[MagicInstsTotalHooks];
} qflex_sim_callbacks_t;

extern qflex_sim_callbacks_t qflex_sim_callbacks;

typedef void (*QFLEX_SIM_CALLBACK_START_TIMING)(void);
typedef void (*QFLEX_SIM_CALLBACK_SIM_QUIT)(void);
typedef void (*QFLEX_SIM_CALLBACK_QMP)(qmp_flexus_cmd_t, const char *);
typedef void (*QFLEX_SIM_CALLBACK_PERIODIC)(void *);
typedef void (*QFLEX_SIM_CALLBACK_TRACE_MEM)(void *, memory_transaction_t *);
typedef void (*QFLEX_SIM_CALLBACK_TRACE_MEM_DMA)(void *, memory_transaction_t *);
typedef void (*QFLEX_SIM_CALLBACK_MAGIC_INST)(void *, int, long long);
typedef void (*QFLEX_SIM_CALLBACK_ETHERNET_FRAME)(void *, int32_t, int32_t, long long);
typedef void (*QFLEX_SIM_CALLBACK_XTERM_BREAK_STRING)(void *, char *);

#endif



void QFLEX_API_get_Interface_Hooks(QFLEX_TO_QEMU_API_t* hooks);
void QFLEX_API_set_Interface_Hooks(const QFLEX_TO_QEMU_API_t* hooks);


void QEMU_API_get_Interface_Hooks(QEMU_TO_QFLEX_CALLBACKS_t *hooks);
void QEMU_API_set_Interface_Hooks(const QEMU_TO_QFLEX_CALLBACKS_t *hooks);

#endif
