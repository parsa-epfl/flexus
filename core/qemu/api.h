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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef void*     conf_class_t;
typedef uint32_t  exception_type_t;
typedef uint64_t  cycles_t;
typedef uint64_t  physical_address_t;
typedef uint64_t  logical_address_t;


/*---------------------------------------------------------------
 *-------------------------ENUMERATIONS----------------------------
 *---------------------------------------------------------------*/

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

  // ─── Bryan ───────────────────────────────────────────────────────────

  GENERAL,            // Regs for A64 mode.
  FLOATING_POINT,

  // PC,              // Program counter
  // PSTATE,          // PSTATE isn't an architectural register for ARMv8
  // SYSREG,          // maybe
  TTBR0,              // MMU translation table base 0
  TTBR1,              // MMU translation table base 1
  ID_AA64MMFR0,       // AArch64 Memory Model Feature Register 0
  SCTLR,              // System Control Register
  // SP,              // AArch64 banked stack pointers
  TCR,
  ISA,
} register_type_t;

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
} pseudo_exceptions_t;

typedef enum {
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

typedef enum {
  QEMU_Instruction_Cache = 1,
  QEMU_Data_Cache        = 2
} cache_type_t;

typedef enum {
  QEMU_Invalidate_Cache = 0,
  QEMU_Clean_Cache,
  QEMU_Flush_Cache,
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
} conf_object_t;

typedef struct exception_t{
  uint32_t syndrome;
} exception_t;

typedef struct {
  logical_address_t   pc;
  logical_address_t   logical_address;
  logical_address_t   target_address;
  physical_address_t  physical_address;
  uint32_t            opcode;
  size_t              size;
  mem_op_type_t       type;
  exception_type_t    exception;
  branch_type_t       branch_type;
  uint8_t             annul         : 1;  // annul the delay slot or not
  uint8_t             atomic        : 1;
  uint8_t             inquiry       : 1;
  uint8_t             may_stall     : 1;
  uint8_t             speculative   : 1;
  uint8_t             ignore        : 1;
  uint8_t             inverse_endian: 1;
} generic_transaction_t;

typedef struct {
  uint32_t set;
  uint32_t way;
} set_and_way_data_t;

typedef struct address_range {
  physical_address_t start_paddr;
  physical_address_t end_paddr;
} address_range_t;


typedef struct {
  generic_transaction_t  s;
  cache_type_t           cache;     // cache to operate on
  cache_maintenance_op_t cache_op;  // operation to perform on cache

  uint8_t io                 : 1;
  uint8_t line               : 1;  // 1 for line, 0 for whole cache
  uint8_t data_is_set_and_way: 1;  // wether or not the operation provides set&way or address (range)

  union{
    set_and_way_data_t set_and_way;
    address_range_t    addr_range;               // same start and end addresses for not range operations
  };

} memory_transaction_t;

/*---------------------------------------------------------------
 *-------------------------TYPEDEFS----------------------------
 *---------------------------------------------------------------*/

typedef bool              (*QEMU_CPU_BUSY_t)       (conf_object_t* cpu);
typedef int               (*QEMU_CPU_EXEC_t)       (conf_object_t *cpu, bool count);
typedef char             *(*QEMU_DISASS_t)         (conf_object_t* cpu, uint64_t addr);
typedef conf_object_t    *(*QEMU_GET_ALL_CPUS_t)   (void);
typedef conf_object_t    *(*QEMU_GET_CPU_BY_IDX_t) (uint64_t idx);
typedef int               (*QEMU_GET_CPU_IDX_t)    (conf_object_t *cpu);
typedef uint64_t          (*QEMU_GET_CSR_t)        (conf_object_t *cpu, int idx);
typedef uint64_t          (*QEMU_GET_CYCLES_LEFT_t)(void);
typedef uint64_t          (*QEMU_GET_FPR_t)        (conf_object_t *cpu, int idx);
typedef uint64_t          (*QEMU_GET_GPR_t)        (conf_object_t *cpu, int idx);
typedef bool              (*QEMU_GET_IRQ_t)        (conf_object_t *cpu);
typedef void              (*QEMU_GET_MEM_t)        (uint8_t* buf, physical_address_t pa, int bytes);
typedef conf_object_t    *(*QEMU_GET_OBJ_BY_NAME_t)(const char *name);
typedef uint64_t          (*QEMU_GET_PC_t)         (conf_object_t *cpu);
typedef int               (*QEMU_GET_PL_t)         (conf_object_t *cpu);
typedef char             *(*QEMU_GET_SNAP_t)       (conf_object_t* cpu);
typedef int               (*QEMU_MEM_OP_IS_DATA_t) (generic_transaction_t *mop);
typedef int               (*QEMU_MEM_OP_IS_WRITE_t)(generic_transaction_t *mop);
typedef void              (*QEMU_STOP_t)           (const char *msg);

// ─── Bryan Qemu-8.2 ──────────────────────────────────────────────────────────
typedef physical_address_t(*QEMU_GET_PA_t)          (size_t core_index, data_or_instr_t fetch, logical_address_t va);
typedef uint64_t          (*QEMU_READ_REG_t)        (size_t core_index, register_type_t reg , size_t reg_info);
typedef size_t            (*QEMU_GET_NUM_CORES_t)   (void);

// ─────────────────────────────────────────────────────────────────────────────



typedef void              (*FLEXUS_START_t)        (void);
typedef void              (*FLEXUS_STOP_t)         (void);
typedef void              (*FLEXUS_QMP_t)          (qmp_flexus_cmd_t, const char *);
typedef void              (*FLEXUS_TRACE_MEM_t)    (uint64_t, memory_transaction_t *);

typedef struct FLEXUS_API_t {
  FLEXUS_START_t          start;
  FLEXUS_STOP_t           stop;
  FLEXUS_QMP_t            qmp;
  FLEXUS_TRACE_MEM_t      trace_mem;
} FLEXUS_API_t;

typedef struct QEMU_API_t
{
  QEMU_CPU_BUSY_t        cpu_busy;
  QEMU_CPU_EXEC_t        cpu_exec;
  QEMU_DISASS_t          disass;
  QEMU_GET_ALL_CPUS_t    get_all_cpus;
  QEMU_GET_CPU_BY_IDX_t  get_cpu_by_idx;
  QEMU_GET_CPU_IDX_t     get_cpu_idx;
  QEMU_GET_CSR_t         get_csr;
  QEMU_GET_CYCLES_LEFT_t get_cycles_left;
  QEMU_GET_FPR_t         get_fpr;
  QEMU_GET_GPR_t         get_gpr;
  QEMU_GET_IRQ_t         get_irq;
  QEMU_GET_MEM_t         get_mem;
  QEMU_GET_OBJ_BY_NAME_t get_obj_by_name;
  QEMU_GET_PC_t          get_pc;
  QEMU_GET_PL_t          get_pl;
  QEMU_GET_SNAP_t        get_snap;
  QEMU_MEM_OP_IS_DATA_t  mem_op_is_data;
  QEMU_MEM_OP_IS_WRITE_t mem_op_is_write;
  QEMU_STOP_t            stop;
  // ─── Bryan ───────────────────────────────────────────────────────────
  QEMU_GET_NUM_CORES_t   get_num_cores;
  QEMU_READ_REG_t        read_register;
  QEMU_GET_PA_t          translate_va2pa;
  // ─────────────────────────────────────────────────────────────────────


} QEMU_API_t;

extern QEMU_API_t   qemu_api;
extern FLEXUS_API_t flexus_api;

#ifdef FLEXUS
  void FLEXUS_start    (void);
  void FLEXUS_stop     (void);
  void FLEXUS_qmp      (qmp_flexus_cmd_t, const char*);
  void FLEXUS_trace_mem(uint64_t, memory_transaction_t*);

  void   QEMU_get_api(  QEMU_API_t *api);
  void FLEXUS_get_api(FLEXUS_API_t *api);
#endif


#endif