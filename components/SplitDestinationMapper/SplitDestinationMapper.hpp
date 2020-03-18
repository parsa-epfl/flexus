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

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT SplitDestinationMapper
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "Cores", 64)

  PARAMETER( MemControllers, int, "Number of memory controllers", "MemControllers", 8)

  PARAMETER( Directories, int, "Number of directories", "Directories", 64)
  PARAMETER( Banks, int, "Number of banks", "Banks", 64)

  PARAMETER( DirInterleaving, int, "Interleaving between directories (in bytes)", "DirInterleaving", 64)
  PARAMETER( MemInterleaving, int, "Interleaving between memory controllers (in bytes)", "MemInterleaving", 4096)

  PARAMETER( DirXORShift, int, "XOR high order bits after shifting this many bits when calculating directory index", "DirXORShift", -1)
  PARAMETER( MemXORShift, int, "XOR high order bits after shifting this many bits when calculating memory index", "MemXORShift", -1)

  PARAMETER( DirLocation, std::string, "Directory location (Distributed|AtMemory)", "DirLocation", "Distributed")
  PARAMETER( MemLocation, std::string, "Memory controller locations (ex: '8,15,24,31,32,39,48,55')", "MemLocation", "8,15,24,31,32,39,48,55")

  PARAMETER( MemReplyToDir, bool, "Send memory replies to the directory (instead of original requester)", "MemReplyToDir", false)
  PARAMETER( MemAcksNeedData, bool, "When memory replies directly to requester, require data with final ack", "MemAcksNeedData", true)
  PARAMETER( TwoPhaseWB, bool, "2 Phase Write-Back sends NAcks to requester, not directory", "TwoPhaseWB", false)
  PARAMETER( LocalDir, bool, "Treate directory as always being local to the requester", "LocalDir", false)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, DirReplyIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, DirSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, DirRequestIn)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DirReplyOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DirSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DirRequestOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, ICacheRequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, ICacheSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, ICacheReplyIn)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ICacheSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ICacheReplyOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, CacheRequestIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, CacheSnoopIn)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, CacheReplyIn)

  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, CacheSnoopOut)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, CacheReplyOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, MemoryIn)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, MemoryOut)

  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FromNIC0)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNIC0)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FromNIC1)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNIC1)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SplitDestinationMapper
// clang-format on
