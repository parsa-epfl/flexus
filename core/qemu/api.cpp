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
#include <core/debug/debug.hpp>
#include <core/flexus.hpp>

namespace Flexus {
namespace Qemu {
namespace API {
#include "qflex-api.h"

static void qflex_sim_callbacks_start_timing(void) {
  DBG_Assert(qflex_sim_callbacks.start_timing.fn != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_START_TIMING) qflex_sim_callbacks.start_timing.fn)();
}

static void qflex_sim_callbacks_sim_quit(void) {
  DBG_Assert(qflex_sim_callbacks.sim_quit.fn != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_SIM_QUIT) qflex_sim_callbacks.sim_quit.fn)();
}

static void qflex_sim_callbacks_qmp(qmp_flexus_cmd_t cmd, const char *args) {
  DBG_Assert(qflex_sim_callbacks.qmp.fn != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_QMP) qflex_sim_callbacks.qmp.fn)(cmd, args);
}

static void qflex_sim_callbacks_periodic(void) {
  DBG_Assert(qflex_sim_callbacks.periodic.fn != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_PERIODIC) qflex_sim_callbacks.periodic.fn)(qflex_sim_callbacks.periodic.obj);
}

void qflex_sim_callbacks_trace_mem(int cpu_index, memory_transaction_t *mem_trans) {
  DBG_Assert(qflex_sim_callbacks.trace_mem[cpu_index].fn != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_TRACE_MEM) qflex_sim_callbacks.trace_mem[cpu_index].fn)(qflex_sim_callbacks.trace_mem[cpu_index].obj, mem_trans);
}

void qflex_sim_callbacks_trace_mem_dma(memory_transaction_t *mem_trans) {
  DBG_Assert(qflex_sim_callbacks.trace_mem_dma.obj != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_TRACE_MEM_DMA) qflex_sim_callbacks.trace_mem_dma.fn)(qflex_sim_callbacks.trace_mem_dma.obj, mem_trans);
}

void qflex_sim_callbacks_magic_inst(int cpu_index, long long aBreakpoint) {
  for (int type = 0; type < MagicInstsTotalHooks; type++) {
    if(qflex_sim_callbacks.magic_inst[type].obj != NULL){
      ((QFLEX_SIM_CALLBACK_MAGIC_INST) qflex_sim_callbacks.magic_inst[type].fn)(qflex_sim_callbacks.magic_inst[type].obj, cpu_index, aBreakpoint);
    } else {
      DBG_(Crit, (<< "Callback was never intialized but still was called"));
    }
  }
}

void qflex_sim_callbacks_ethernet_frame(int32_t aNetworkID, int32_t aFrameType, long long aTimestamp) {
  DBG_Assert(qflex_sim_callbacks.ethernet_frame.obj != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_ETHERNET_FRAME) qflex_sim_callbacks.ethernet_frame.fn)(qflex_sim_callbacks.ethernet_frame.obj, aNetworkID, aFrameType, aTimestamp);
}

void qflex_sim_callbacks_xterm_break_string(char *aString) {
  DBG_Assert(qflex_sim_callbacks.xterm_break_string.obj != NULL, (<< "Callback was never intialized but still was called"));
  ((QFLEX_SIM_CALLBACK_XTERM_BREAK_STRING) qflex_sim_callbacks.xterm_break_string.fn)(qflex_sim_callbacks.xterm_break_string.obj, aString);
}



// Get MMU register state on initialization
// QEMU_GET_MMU_STATE_PROC QEMU_get_mmu_state= nullptr;
QFLEX_TO_QEMU_API_t qemu_callbacks =
{
  .QEMU_clear_exception = nullptr,
  .QEMU_has_pending_irq = nullptr,

  .QEMU_write_register = nullptr,
  .QEMU_read_register = nullptr,
  .QEMU_read_unhashed_sysreg = nullptr,
  .QEMU_read_pstate = nullptr,
  .QEMU_read_sctlr = nullptr,
  .QEMU_cpu_has_work = nullptr,
  .QEMU_read_sp_el = nullptr,

  .QEMU_read_phys_memory = nullptr,
  .QEMU_get_cpu_by_index = nullptr,
  .QEMU_get_cpu_index = nullptr,
  .QEMU_step_count = nullptr,
  .QEMU_get_num_sockets = nullptr,
  .QEMU_get_num_cores = nullptr,
  .QEMU_get_num_threads_per_core = nullptr,
  .QEMU_get_all_cpus = nullptr,

  .QEMU_set_tick_frequency = nullptr,
  .QEMU_get_tick_frequency = nullptr,
  .QEMU_get_program_counter = nullptr,
  .QEMU_logical_to_physical = nullptr,
  .QEMU_quit_simulation = nullptr,
  .QEMU_getCyclesLeft = nullptr,

  .QEMU_mem_op_is_data = nullptr,
  .QEMU_mem_op_is_write = nullptr,
  .QEMU_mem_op_is_read = nullptr,

  .QEMU_instruction_handle_interrupt = nullptr,
  .QEMU_get_object_by_name = nullptr,
  .QEMU_cpu_execute = nullptr,
  .QEMU_is_in_simulation = nullptr,
  .QEMU_toggle_simulation = nullptr,
  .QEMU_flush_tb_cache = nullptr,
  .QEMU_get_instruction_count = nullptr,
  .QEMU_disassemble = nullptr,
  .QEMU_dump_state = nullptr,

  .QEMU_cpu_set_quantum = nullptr,
  .QEMU_increment_debug_stat = nullptr,
};

void QFLEX_API_set_Interface_Hooks(const QFLEX_TO_QEMU_API_t *hooks) {
  qemu_callbacks.QEMU_clear_exception                = hooks->QEMU_clear_exception;
  qemu_callbacks.QEMU_has_pending_irq                = hooks->QEMU_has_pending_irq;

  qemu_callbacks.QEMU_write_register                 = hooks->QEMU_write_register;
  qemu_callbacks.QEMU_read_register                  = hooks->QEMU_read_register;
  qemu_callbacks.QEMU_read_unhashed_sysreg           = hooks->QEMU_read_unhashed_sysreg;
  qemu_callbacks.QEMU_read_pstate                    = hooks->QEMU_read_pstate;
  qemu_callbacks.QEMU_read_sctlr                     = hooks->QEMU_read_sctlr;
  qemu_callbacks.QEMU_cpu_has_work                   = hooks->QEMU_cpu_has_work;
  qemu_callbacks.QEMU_read_sp_el                     = hooks->QEMU_read_sp_el;

  qemu_callbacks.QEMU_read_phys_memory               = hooks->QEMU_read_phys_memory;
  qemu_callbacks.QEMU_get_cpu_by_index               = hooks->QEMU_get_cpu_by_index;
  qemu_callbacks.QEMU_get_cpu_index                  = hooks->QEMU_get_cpu_index;
  qemu_callbacks.QEMU_step_count                     = hooks->QEMU_step_count;
  qemu_callbacks.QEMU_get_num_sockets                = hooks->QEMU_get_num_sockets;
  qemu_callbacks.QEMU_get_num_cores                  = hooks->QEMU_get_num_cores;
  qemu_callbacks.QEMU_get_num_threads_per_core       = hooks->QEMU_get_num_threads_per_core;
  qemu_callbacks.QEMU_get_all_cpus                   = hooks->QEMU_get_all_cpus;

  qemu_callbacks.QEMU_set_tick_frequency             = hooks->QEMU_set_tick_frequency;
  qemu_callbacks.QEMU_get_tick_frequency             = hooks->QEMU_get_tick_frequency;
  qemu_callbacks.QEMU_get_program_counter            = hooks->QEMU_get_program_counter;
  qemu_callbacks.QEMU_logical_to_physical            = hooks->QEMU_logical_to_physical;
  qemu_callbacks.QEMU_quit_simulation                = hooks->QEMU_quit_simulation;
  qemu_callbacks.QEMU_getCyclesLeft                  = hooks->QEMU_getCyclesLeft;

  qemu_callbacks.QEMU_mem_op_is_data                 = hooks->QEMU_mem_op_is_data;
  qemu_callbacks.QEMU_mem_op_is_write                = hooks->QEMU_mem_op_is_write;
  qemu_callbacks.QEMU_mem_op_is_read                 = hooks->QEMU_mem_op_is_read;

  qemu_callbacks.QEMU_instruction_handle_interrupt   = hooks->QEMU_instruction_handle_interrupt;
  qemu_callbacks.QEMU_get_object_by_name             = hooks->QEMU_get_object_by_name;
  qemu_callbacks.QEMU_cpu_execute                    = hooks->QEMU_cpu_execute;
  qemu_callbacks.QEMU_is_in_simulation               = hooks->QEMU_is_in_simulation;
  qemu_callbacks.QEMU_toggle_simulation              = hooks->QEMU_toggle_simulation;
  qemu_callbacks.QEMU_flush_tb_cache                 = hooks->QEMU_flush_tb_cache;
  qemu_callbacks.QEMU_get_instruction_count          = hooks->QEMU_get_instruction_count;
  qemu_callbacks.QEMU_disassemble                    = hooks->QEMU_disassemble;
  qemu_callbacks.QEMU_dump_state                     = hooks->QEMU_dump_state;

  qemu_callbacks.QEMU_cpu_set_quantum                = hooks->QEMU_cpu_set_quantum;
  qemu_callbacks.QEMU_increment_debug_stat           = hooks->QEMU_increment_debug_stat;
  // Msutherl: MMU hooks
  //  QEMU_get_mmu_state= hooks->QEMU_get_mmu_state;
}

void QEMU_API_get_Interface_Hooks (QEMU_TO_QFLEX_CALLBACKS_t* hooks) {
  hooks->start_timing = &qflex_sim_callbacks_start_timing;
  hooks->sim_quit = &qflex_sim_callbacks_sim_quit;
  hooks->qmp = &qflex_sim_callbacks_qmp;
  hooks->trace_mem = &qflex_sim_callbacks_trace_mem;
  hooks->trace_mem_dma = &qflex_sim_callbacks_trace_mem_dma;
  hooks->periodic = &qflex_sim_callbacks_periodic;
  hooks->magic_inst = &qflex_sim_callbacks_magic_inst;
  hooks->ethernet_frame = &qflex_sim_callbacks_ethernet_frame;
  hooks->xterm_break_string = &qflex_sim_callbacks_xterm_break_string;
}

qflex_sim_callbacks_t qflex_sim_callbacks =
{
  .start_timing = {NULL, NULL, },
  .sim_quit = {NULL, NULL, },
  .qmp = {NULL, NULL, },
  .trace_mem = NULL,
  .trace_mem_dma = {NULL, NULL, },
  .periodic = {NULL, NULL, },
  .ethernet_frame = {NULL, NULL, },
  .xterm_break_string = { NULL, NULL, },
  .magic_inst = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
  },
};

} // namespace API
} // namespace Qemu
} // namespace Flexus
