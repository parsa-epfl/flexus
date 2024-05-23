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
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/RegionScoutMessage.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT FastCMPCache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPWidth", 16 )

  PARAMETER( Size, int, "Cache size in bytes", "size", 65536 )
  PARAMETER( Associativity, int, "Set associativity", "assoc", 2 )
  PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
  PARAMETER( CleanEvictions, bool, "Issue clean evictions", "clean_evict", false )
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )
  PARAMETER( TraceTracker, bool, "Turn trace tracker on/off", "trace_tracker_on", false )

  PARAMETER( ReplPolicy, std::string, "Cache replacement policy (LRU | SetLRU | RegionLRU)", "repl", "LRU" )

  PARAMETER( RegionSize, int, "Region size in bytes", "rsize", 1024 )
  PARAMETER( RTAssoc, int, "RegionTracker Associativity", "rt_assoc", 16 )
  PARAMETER( RTSize, int, "RegionTracker size (number of regions tracked)", "rt_size", 8192 )
  PARAMETER( ERBSize, int, "Evicted Region Buffer size", "erb_size", 8 )

  PARAMETER( StdArray, bool, "Use Standard Tag Array (true) or RegionTracker (false) default is true", "std_array", true )

  PARAMETER( DirectoryType, std::string, "Directory Type", "directory_type", "Infinite")
  PARAMETER( Protocol, std::string, "Protocol Type", "protocol", "SingleCMP")
  PARAMETER( AlwaysMulticast, bool, "Perform multicast instead of serial snooping", "always_multicast", false)
  PARAMETER( SeparateID, bool, "Track Instruction and Data caches separately", "seperate_id", false)

  PARAMETER( CoherenceUnit, uint64_t, "Coherence Unit", "coherence_unit", 64)

);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestIn )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FetchRequestIn )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutD )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutI )

  DYNAMIC_PORT_ARRAY( PushInput, RegionScoutMessage, RegionNotify )
  DYNAMIC_PORT_ARRAY( PushOutput, RegionScoutMessage, RegionNotifyOut )
  DYNAMIC_PORT_ARRAY( PushOutput, RegionScoutMessage, RegionProbe )

  // From Directory OUT to Memory Controller
  PORT( PushInput, MemoryMessage, SnoopIn ) // From MemController IN to topology
  PORT( PushOutput, MemoryMessage, RequestOut ) // From topology OUT to memcontroller

  DRIVE( UpdateStatsDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastCMPCache
// clang-format on
