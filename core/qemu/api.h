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

#include <stdint.h>
#include <stdbool.h>

#define BOTH_INSTR 2

typedef uint64_t cycles_t;
typedef uint64_t physical_address_t;
typedef uint64_t logical_address_t;
typedef void*    conf_class_t;
typedef int      exception_type_t;


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

typedef struct generic_transaction {
  logical_address_t  pc;
  logical_address_t  logical_address;
  logical_address_t  target_address;
  physical_address_t physical_address;
  uint32_t           opcode;
  unsigned           size;
  mem_op_type_t      type;
  exception_type_t   exception;
  branch_type_t      branch_type;
  unsigned int       annul:          1; // annul the delay slot or not
  unsigned int       atomic:         1;
  unsigned int       inquiry:        1;
  unsigned int       may_stall:      1;
  unsigned int       speculative:    1;
  unsigned int       ignore:         1;
  unsigned int       inverse_endian: 1;
} generic_transaction_t;

typedef struct set_and_way_data {
  uint32_t set;
  uint32_t way;
} set_and_way_data_t;

typedef struct address_range{
  physical_address_t start_paddr;
  physical_address_t end_paddr;
} address_range_t;


typedef struct memory_transaction {
  generic_transaction_t  s;
  unsigned int           io:                  1;
  cache_type_t           cache;                  // cache to operate on
  cache_maintenance_op_t cache_op;               // operation to perform on cache
  int                    line:                1; // 1 for line, 0 for whole cache
  int                    data_is_set_and_way: 1; // wether or not the operation provides set&way or address (range)

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
typedef int               (*QEMU_GET_NUM_CORES_t)  (void);
typedef conf_object_t    *(*QEMU_GET_OBJ_BY_NAME_t)(const char *name);
// typedef physical_address_t(*QEMU_GET_PA_t)         (conf_object_t *cpu, data_or_instr_t fetch, logical_address_t va);
typedef uint64_t          (*QEMU_GET_PC_t)         (conf_object_t *cpu);
typedef int               (*QEMU_GET_PL_t)         (conf_object_t *cpu);
typedef char             *(*QEMU_GET_SNAP_t)       (conf_object_t* cpu);
typedef int               (*QEMU_MEM_OP_IS_DATA_t) (generic_transaction_t *mop);
typedef int               (*QEMU_MEM_OP_IS_WRITE_t)(generic_transaction_t *mop);
typedef void              (*QEMU_STOP_t)           (const char *msg);

// ─── Bryan Qemu-8.2 ──────────────────────────────────────────────────────────
typedef physical_address_t(*QEMU_GET_PA_t)          (uint64_t core_index, data_or_instr_t fetch, logical_address_t va);
typedef uint64_t          (*QEMU_READ_REG_t)        (uint64_t core_index, register_type_t reg , uint64_t reg_index);

// ─────────────────────────────────────────────────────────────────────────────



typedef void              (*FLEXUS_START_t)        (void);
typedef void              (*FLEXUS_STOP_t)         (void);
typedef void              (*FLEXUS_QMP_t)          (qmp_flexus_cmd_t, const char *);
typedef void              (*FLEXUS_TRACE_MEM_t)    (int, memory_transaction_t *);

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
  QEMU_GET_NUM_CORES_t   get_num_cores;
  QEMU_GET_OBJ_BY_NAME_t get_obj_by_name;
  QEMU_GET_PC_t          get_pc;
  QEMU_GET_PL_t          get_pl;
  QEMU_GET_SNAP_t        get_snap;
  QEMU_MEM_OP_IS_DATA_t  mem_op_is_data;
  QEMU_MEM_OP_IS_WRITE_t mem_op_is_write;
  QEMU_STOP_t            stop;
  // ─── Bryan ───────────────────────────────────────────────────────────
  QEMU_READ_REG_t        read_register;
  QEMU_GET_PA_t          translate_va2pa;
  // ─────────────────────────────────────────────────────────────────────


} QEMU_API_t;

extern QEMU_API_t qemu_api;
extern FLEXUS_API_t flexus_api;

void FLEXUS_start    (void);
void FLEXUS_stop     (void);
void FLEXUS_qmp      (qmp_flexus_cmd_t, const char *);
void FLEXUS_trace_mem(int, memory_transaction_t *);

// #ifndef FLEXUS
// bool               QEMU_cpu_busy       (conf_object_t *cpu);
// int                QEMU_cpu_exec       (conf_object_t *cpu, bool count);
// char              *QEMU_disass         (conf_object_t* cpu, uint64_t pc);
// conf_object_t     *QEMU_get_all_cpus   (void);
// conf_object_t     *QEMU_get_cpu_by_idx (uint64_t idx);
// int                QEMU_get_cpu_idx    (conf_object_t *cpu);
// uint64_t           QEMU_get_csr        (conf_object_t *cpu, int idx);
// uint64_t           QEMU_get_cycles_left(void);
// uint64_t           QEMU_get_fpr        (conf_object_t *cpu, int idx);
// uint64_t           QEMU_get_gpr        (conf_object_t *cpu, int idx);
// bool               QEMU_get_irq        (conf_object_t *cpu);
// void               QEMU_get_mem        (uint8_t* buf, physical_address_t pa, int bytes);
// int                QEMU_get_num_cores  (void);
// conf_object_t     *QEMU_get_obj_by_name(const char *name);
// physical_address_t QEMU_get_pa         (conf_object_t *cpu, data_or_instr_t fetch, logical_address_t va);
// uint64_t           QEMU_get_pc         (conf_object_t *cpu);
// int                QEMU_get_pl         (conf_object_t *cpu);
// char              *QEMU_get_snap       (conf_object_t* cpu);
// int                QEMU_mem_op_is_data (generic_transaction_t *mop);
// int                QEMU_mem_op_is_write(generic_transaction_t *mop);
// void               QEMU_stop           (const char *msg);
// uint64_t           QEMU_read         (uint64_t core_index, register_type_t reg , uint64_t reg_index);


// #else

// #endif

void   QEMU_get_api(  QEMU_API_t *api);
void FLEXUS_get_api(FLEXUS_API_t *api);

void qflex_api_init(bool timing_mode, uint64_t sim_cycles);


#endif
