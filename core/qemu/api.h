/*
    ___  ___ __  __ _   _    ____    ___ _
   / _ \| __|  \/  | | | |  / /\ \  | __| |_____ ___  _ ___
  | (_) | _|| |\/| | |_| | < <  > > | _|| / -_) \ / || (_-<
   \__\_\___|_|  |_|\___/   \_\/_/  |_| |_\___/_\_\\_,_/__/
             _
   __ _ _ __(_)
  / _` | '_ \ |
  \__,_| .__/_|
       |_|
*/

/**
 * If Flexus is getting compiled we expect preprocessor variable
 * FLEXUS to exist in the scope.
 *
 * so:
 *    #ifdef FLEXUS
 *        flexus specific code
 *
 *    #ifdef QEMU
 *        QEMU specific code
 */

#ifndef QEMU_2_FLEXUS_API_H
#define QEMU_2_FLEXUS_API_H

/**
 * For clarity, I prefer using true condition
 * insted of inverting
 */
#ifndef FLEXUS
#define QEMU
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void* conf_class_t;
typedef uint32_t exception_type_t;
typedef uint64_t cycles_t;
typedef uint64_t physical_address_t;
typedef uint64_t logical_address_t;

/*---------------------------------------------------------------
 *-------------------------ENUMERATIONS----------------------------
 *---------------------------------------------------------------*/

typedef enum
{
    QMP_FLEXUS_SETSTATINTERVAL,
    QMP_FLEXUS_WRITEMEASUREMENT,
    QMP_FLEXUS_DOLOAD,
    QMP_FLEXUS_DOSAVE,
    QMP_FLEXUS_SAVESTATS,
    QMP_FLEXUS_TERMINATESIMULATION,
} qmp_flexus_cmd_t;

typedef enum
{
    QEMU_Class_Kind_Vanilla,
    QEMU_Class_Kind_Pseudo,
    QEMU_Class_Kind_Session
} class_data_t;

typedef enum
{
    // ─── Bryan ───────────────────────────────────────────────────────────

    GENERAL = 0, // Regs for A64 mode.
    FLOATING_POINT,

    PC,
    PSTATE,       // PSTATE isn't an architectural register for ARMv8
    TTBR0,        // MMU translation table base 0
    TTBR1,        // MMU translation table base 1
    TPIDR,        // Thread ID
    ID_AA64MMFR0, // AArch64 Memory Model Feature Register 0
    SCTLR,        // System Control Register
    TCR,
    ISA,
    DAIF,
} register_type_t;

typedef enum
{
    QEMU_Set_Ok
} set_error_t;

typedef enum
{
    QEMU_Trans_Load,
    QEMU_Trans_Store,
    QEMU_Trans_Instr_Fetch,
    QEMU_Trans_Prefetch,
    QEMU_Trans_Cache
} mem_op_type_t;

typedef enum
{
    QEMU_PE_No_Exception = 1025,
} pseudo_exceptions_t;

typedef enum
{
    QEMU_Non_Branch           = 0,
    QEMU_Conditional_Branch   = 1,
    QEMU_Unconditional_Branch = 2,
    QEMU_Call_Branch          = 3,
    QEMU_IndirectReg_Branch   = 4,
    QEMU_IndirectCall_Branch  = 5,
    QEMU_Return_Branch        = 6,
    QEMU_Last_Branch_Type     = 7,
    QEMU_Barrier_Branch       = 8,
    QEMU_BRANCH_TYPE_COUNT
} branch_type_t;

typedef enum
{
    QEMU_Instruction_Cache = 1,
    QEMU_Data_Cache        = 2
} cache_type_t;

typedef enum
{
    QEMU_Invalidate_Cache = 0,
    QEMU_Clean_Cache,
    QEMU_Flush_Cache,
} cache_maintenance_op_t;

typedef enum
{
    QEMU_DI_Instruction,
    QEMU_DI_Data
} data_or_instr_t;

/*---------------------------------------------------------------
 *-------------------------STRUCTURES----------------------------
 *---------------------------------------------------------------*/

typedef struct conf_object
{
    char* name;
    void* object; // pointer to the struct in question
} conf_object_t;

typedef struct exception_t
{
    uint32_t syndrome;
} exception_t;

typedef struct
{
    logical_address_t pc;
    logical_address_t logical_address;
    logical_address_t target_address;
    physical_address_t physical_address;
    uint32_t opcode;
    size_t size;
    mem_op_type_t type;
    exception_type_t exception;
    branch_type_t branch_type;
    uint8_t annul : 1; // annul the delay slot or not
    uint8_t atomic : 1;
    uint8_t inquiry : 1;
    uint8_t may_stall : 1;
    uint8_t speculative : 1;
    uint8_t ignore : 1;
    uint8_t inverse_endian : 1;
} generic_transaction_t;

typedef struct
{
    uint32_t set;
    uint32_t way;
} set_and_way_data_t;

typedef struct address_range
{
    physical_address_t start_paddr;
    physical_address_t end_paddr;
} address_range_t;

typedef struct
{
    generic_transaction_t s;
    cache_type_t cache;              // cache to operate on
    cache_maintenance_op_t cache_op; // operation to perform on cache

    uint8_t io : 1;
    uint8_t line : 1;                // 1 for line, 0 for whole cache
    uint8_t data_is_set_and_way : 1; // wether or not the operation provides set&way or address (range)

    union
    {
        set_and_way_data_t set_and_way;
        address_range_t addr_range; // same start and end addresses for not range operations
    };

} memory_transaction_t;

struct cycles_opts
{
    uint64_t until_stop;
    uint64_t stats_interval;
    uint64_t log_delay;
};

/*---------------------------------------------------------------
 *-------------------------TYPEDEFS----------------------------
 *---------------------------------------------------------------*/

// typedef conf_object_t    *(*QEMU_GET_ALL_CPUS_t)   (void);
// typedef conf_object_t    *(*QEMU_GET_CPU_BY_IDX_t) (uint64_t idx);
// typedef int               (*QEMU_GET_CPU_IDX_t)    (conf_object_t *cpu);
// typedef uint64_t          (*QEMU_GET_CSR_t)        (conf_object_t *cpu, int idx);
// typedef uint64_t          (*QEMU_GET_CYCLES_LEFT_t)(void);
// typedef uint64_t          (*QEMU_GET_FPR_t)        (conf_object_t *cpu, int idx);
// typedef uint64_t          (*QEMU_GET_GPR_t)        (conf_object_t *cpu, int idx);
// typedef conf_object_t    *(*QEMU_GET_OBJ_BY_NAME_t)(const char *name);
// typedef int               (*QEMU_GET_PL_t)         (conf_object_t *cpu);
// typedef char             *(*QEMU_GET_SNAP_t)       (conf_object_t* cpu);
// typedef int               (*QEMU_MEM_OP_IS_DATA_t) (generic_transaction_t *mop);
// typedef int               (*QEMU_MEM_OP_IS_WRITE_t)(generic_transaction_t *mop);

typedef physical_address_t (*QEMU_GET_PA_t)(size_t core_index, logical_address_t va, bool unprivileged);
typedef uint64_t (*QEMU_READ_REG_t)(size_t core_index, register_type_t reg, size_t reg_info);
typedef uint64_t (*QEMU_READ_SYSREG_t)(size_t core_index,
                                       uint8_t op0,
                                       uint8_t op1,
                                       uint8_t op2,
                                       uint8_t crn,
                                       uint8_t crm,
                                       bool ignore_permission_check);
typedef size_t (*QEMU_GET_NUM_CORES_t)(void);
typedef logical_address_t (*QEMU_GET_PC_t)(size_t core_index);
typedef bool (*QEMU_GET_IRQ_t)(size_t core_index);
typedef uint64_t (*QEMU_CPU_EXEC_t)(size_t core_index, bool count);
typedef void (*QEMU_TICK_t)(void);
typedef void (*QEMU_GET_MEM_t)(uint8_t* buffer, physical_address_t pa, size_t nb_bytes);
typedef void (*QEMU_STOP_t)(char const* const msg);
typedef char* (*QEMU_DISASS_t)(size_t core_index, uint64_t addr, size_t size);
typedef bool (*QEMU_CPU_BUSY_t)(size_t core_index);
// ─────────────────────────────────────────────────────────────────────────────

typedef void (*FLEXUS_START_t)(uint64_t);
typedef void (*FLEXUS_STOP_t)(void);
typedef void (*FLEXUS_QMP_t)(qmp_flexus_cmd_t, const char*);
typedef void (*FLEXUS_TRACE_MEM_t)(uint64_t, memory_transaction_t*);

typedef struct FLEXUS_API_t
{
    FLEXUS_START_t start;
    FLEXUS_STOP_t stop;
    FLEXUS_QMP_t qmp;
    FLEXUS_TRACE_MEM_t trace_mem;
} FLEXUS_API_t;

typedef struct QEMU_API_t
{
    QEMU_GET_NUM_CORES_t get_num_cores;
    QEMU_READ_REG_t read_register;
    QEMU_READ_SYSREG_t read_sys_register;
    QEMU_GET_PA_t translate_va2pa;
    QEMU_GET_PC_t get_pc;
    QEMU_GET_IRQ_t has_irq;
    QEMU_CPU_EXEC_t cpu_exec;
    QEMU_GET_MEM_t get_mem;
    QEMU_STOP_t stop;
    QEMU_TICK_t tick;
    QEMU_DISASS_t disassembly;
    QEMU_CPU_BUSY_t is_busy;
} QEMU_API_t;

extern QEMU_API_t qemu_api;
extern FLEXUS_API_t flexus_api;

#ifdef FLEXUS
void FLEXUS_start(uint64_t);
void
FLEXUS_stop(void);
void
FLEXUS_qmp(qmp_flexus_cmd_t, const char*);
void
FLEXUS_trace_mem(uint64_t, memory_transaction_t*);

void
QEMU_get_api(QEMU_API_t* api);
void
FLEXUS_get_api(FLEXUS_API_t* api);
#endif

#endif
