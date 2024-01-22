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

#include <components/uFetch/uFetchTypes.hpp>

#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( FAQSize, uint32_t, "Fetch address queue size", "faq", 32 )
  PARAMETER( MaxFetchLines, uint32_t, "Max i-cache lines fetched per cycle", "flines", 2 )
  PARAMETER( MaxFetchInstructions, uint32_t, "Max instructions fetched per cycle", "finst", 10 )
  PARAMETER( ICacheLineSize, uint64_t, "Icache line size in bytes", "iline", 64 )
  PARAMETER( PerfectICache, bool, "Use a perfect ICache", "perfect", true )
  PARAMETER( PrefetchEnabled, bool, "Enable Next-line Prefetcher", "prefetch", true )
  PARAMETER( CleanEvict, bool, "Enable eviction messages", "clean_evict", false)
  PARAMETER( Size, int, "ICache size in bytes", "size", 65536 )
  PARAMETER( Associativity, int, "ICache associativity", "associativity", 4 )
  PARAMETER( MissQueueSize, uint32_t, "Maximum size of the fetch miss queue", "miss_queue_size", 4 )
  PARAMETER( Threads, uint32_t, "Number of threads under control of this uFetch", "threads", 1 )
  PARAMETER( SendAcks, bool, "Send acknowledgements when we received data", "send_acks", false )
  PARAMETER( UseReplyChannel, bool, "Send replies on Reply Channel and only Evicts on Snoop Channel", "use_reply_channel", false )
  PARAMETER( EvictOnSnoop, bool, "Send evicts on Snoop Channel (otherwise use Request Channel)", "evict_on_snoop", true )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<FetchCommand>, FetchAddressIn )
  DYNAMIC_PORT_ARRAY( PushInput, eSquashCause, SquashIn )
  PORT( PushInput, MemoryTransport, FetchMissIn )
  DYNAMIC_PORT_ARRAY( PullOutput, int, AvailableFAQOut )
  DYNAMIC_PORT_ARRAY( PullInput, int, AvailableFIQ )
  DYNAMIC_PORT_ARRAY( PullOutput, int, ICount)
  DYNAMIC_PORT_ARRAY( PullOutput, bool, Stalled)
  DYNAMIC_PORT_ARRAY( PushOutput, pFetchBundle, FetchBundleOut )

  PORT( PushOutput, MemoryTransport, FetchMissOut )
  PORT( PushOutput, MemoryTransport, FetchSnoopOut )
  PORT( PushOutput, MemoryTransport, FetchReplyOut )

  PORT( PushOutput, TranslationPtr , iTranslationOut )
  PORT( PushInput,  TranslationPtr , iTranslationIn )

  PORT( PushOutput, bool, InstructionFetchSeen ) // Notify PowerTracker when an instruction is fetched.
  PORT( PushOutput, bool, ClockTickSeen )        // Notify PowerTracker when the clock in this core ticks. This goes here just because uFetch is driven first and it's convenient.


  DYNAMIC_PORT_ARRAY( PushInput, int, ResyncIn )

  DRIVE( uFetchDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT uFetch
// clang-format on
