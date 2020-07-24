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

#include <core/simulator_layout.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/uFetch/uFetchTypes.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT uArchARM
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( ROBSize, uint32_t, "Reorder buffer size", "rob", 256 )
  PARAMETER( SBSize, uint32_t, "Store buffer size", "sb", 256 )
  PARAMETER( NAWBypassSB, bool, "Allow Non-Allocating-Writes to bypass store-buffer", "naw_bypass_sb", false )
  PARAMETER( NAWWaitAtSync, bool, "Force MEMBAR #Sync to wait for non-allocating writes to finish", "naw_wait_at_sync", false )
  PARAMETER( RetireWidth, uint32_t, "Retirement width", "retire", 8 )
  PARAMETER( MemoryPorts, uint32_t, "Memory Ports", "memports", 4 )
  PARAMETER( SnoopPorts, uint32_t, "Snoop Ports", "snoopports", 1 )
  PARAMETER( StorePrefetches, uint32_t, "Simultaneous store prefeteches", "storeprefetch", 16 )
  PARAMETER( PrefetchEarly, bool, "Issue store prefetch requests when address resolves", "prefetch_early", false )
  PARAMETER( ConsistencyModel, uint32_t, "Consistency Model", "consistency", 0 /* SC */ )
  PARAMETER( CoherenceUnit, uint32_t, "Coherence Unit", "coherence", 64 )
  PARAMETER( BreakOnResynchronize, bool, "Break on resynchronizer", "break_on_resynch", false )
  PARAMETER( SpinControl, bool, "Enable spin control", "spin_control", true )
  PARAMETER( SpeculativeOrder, bool, "Speculate on Memory Order", "spec_order", false )
  PARAMETER( SpeculateOnAtomicValue, bool, "Speculate on the Value of Atomics", "spec_atomic_val", false )
  PARAMETER( SpeculateOnAtomicValuePerfect, bool, "Use perfect atomic value prediction", "spec_atomic_val_perfect", false )
  PARAMETER( SpeculativeCheckpoints, int, "Number of checkpoints allowed.  0 for infinite", "spec_ckpts", 0)
  PARAMETER( CheckpointThreshold, int, "Number of instructions between checkpoints.  0 disables periodic checkpoints", "ckpt_threshold", 0)
  PARAMETER( EarlySGP, bool, "Notify SGP Early", "early_sgp", false )   /* CMU-ONLY */
  PARAMETER( TrackParallelAccesses, bool, "Track which memory accesses can proceed in parallel", "track_parallel", false ) /* CMU-ONLY */
  PARAMETER( InOrderMemory, bool, "Only allow ROB/SB head to issue to memory", "in_order_memory", false )
  PARAMETER( InOrderExecute, bool, "Ensure that instructions execute in order", "in_order_execute", false )
  PARAMETER( OnChipLatency, uint32_t, "On-Chip Side-Effect latency", "on-chip-se", 0)
  PARAMETER( OffChipLatency, uint32_t, "Off-Chip Side-Effect latency", "off-chip-se", 0)
  PARAMETER( Multithread, bool, "Enable multi-threaded execution", "multithread", false )

  PARAMETER( NumIntAlu, uint32_t, "Number of integer ALUs", "numIntAlu", 1)
  PARAMETER( IntAluOpLatency, uint32_t, "End-to-end latency of an integer ALU operation", "intAluOpLatency", 1)
  PARAMETER( IntAluOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent integer ALU operations", "intAluOpPipelineResetTime", 1)

  PARAMETER( NumIntMult, uint32_t, "Number of integer MUL/DIV units", "numIntMult", 1)
  PARAMETER( IntMultOpLatency, uint32_t, "End-to-end latency of an integer MUL operation", "intMultOpLatency", 1)
  PARAMETER( IntMultOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent integer MUL operations", "intMultOpPipelineResetTime", 1)
  PARAMETER( IntDivOpLatency, uint32_t, "End-to-end latency of an integer DIV operation", "intDivOpLatency", 1)
  PARAMETER( IntDivOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent integer DIV operations", "intDivOpPipelineResetTime", 1)

  PARAMETER( NumFpAlu, uint32_t, "Number of FP ALUs", "numFpAlu", 1)
  PARAMETER( FpAddOpLatency, uint32_t, "End-to-end latency of an FP ADD/SUB operation", "fpAddOpLatency", 1)
  PARAMETER( FpAddOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP ADD/SUB operations", "fpAddOpPipelineResetTime", 1)
  PARAMETER( FpCmpOpLatency, uint32_t, "End-to-end latency of an FP compare operation", "fpCmpOpLatency", 1)
  PARAMETER( FpCmpOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP compare operations", "fpCmpOpPipelineResetTime", 1)
  PARAMETER( FpCvtOpLatency, uint32_t, "End-to-end latency of an FP convert operation", "fpCvtOpLatency", 1)
  PARAMETER( FpCvtOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP convert operations", "fpCvtOpPipelineResetTime", 1)

  PARAMETER( NumFpMult, uint32_t, "Number of FP MUL/DIV units", "numFpMult", 1)
  PARAMETER( FpMultOpLatency, uint32_t, "End-to-end latency of an FP MUL operation", "fpMultOpLatency", 1)
  PARAMETER( FpMultOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP MUL operations", "fpMultOpPipelineResetTime", 1)
  PARAMETER( FpDivOpLatency, uint32_t, "End-to-end latency of an FP DIV operation", "fpDivOpLatency", 1)
  PARAMETER( FpDivOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP DIV operations", "fpDivOpPipelineResetTime", 1)
  PARAMETER( FpSqrtOpLatency, uint32_t, "End-to-end latency of an FP SQRT operation", "fpSqrtOpLatency", 1)
  PARAMETER( FpSqrtOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP SQRT operations", "fpSqrtOpPipelineResetTime", 1)
);

typedef std::pair<int, bool> dispatch_status;
typedef VirtualMemoryAddress vaddr_pair;

COMPONENT_INTERFACE(
  PORT( PushInput, boost::intrusive_ptr< AbstractInstruction >, DispatchIn)
  PORT( PullOutput, dispatch_status, AvailableDispatchOut)
  PORT( PullOutput, bool, Stalled)
  PORT( PullOutput, bool, CoreHalted)
  PORT( PullOutput, int, ICount)
  PORT( PushOutput, eSquashCause, SquashOut )
  PORT( PushOutput, vaddr_pair, RedirectOut )
  PORT( PushOutput, CPUState, ChangeCPUState )
  PORT( PushOutput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackOut )
  PORT( PushOutput, MemoryTransport, MemoryOut_Request )
  PORT( PushOutput, MemoryTransport, MemoryOut_Snoop )
  PORT( PushInput, MemoryTransport, MemoryIn )
  PORT( PushInput, PhysicalMemoryAddress, WritePermissionLost )
  PORT( PushOutput, bool, StoreForwardingHitSeen) // Signal a store forwarding hit in the LSQ to the PowerTracker

  PORT( PushOutput, int, ResyncOut )

  PORT( PushOutput, TranslationPtr, dTranslationOut )
  PORT( PushInput, TranslationPtr,  dTranslationIn )
  PORT( PushInput, TranslationPtr,  MemoryRequestIn )


  DRIVE( uArchDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT uArchARM
// clang-format on
