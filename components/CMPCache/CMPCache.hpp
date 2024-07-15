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

#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT CMPCache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "cores", 1 )
  PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
  PARAMETER( Banks, int, "number of directory banks in each group", "banks", 1 )
  PARAMETER( BankInterleaving, int, "interleaving between directory banks (64 bytes)", "bank_interleaving", 64 )
  PARAMETER( Groups, int, "number of directory bank groups", "groups", 1 )
  PARAMETER( GroupInterleaving, int, "interleaving between directory bank groups (1024 bytes)", "group_interleaving", 1024 )

  PARAMETER( DirLatency, int, "Total latency of directory lookup", "dir_lat", 1 )
  PARAMETER( DirIssueLatency, int, "Minimum delay between issues to the directory", "dir_issue_lat", 0 )

  PARAMETER( TagLatency, int, "Total latency of tag array lookup", "tag_lat", 1 )
  PARAMETER( TagIssueLatency, int, "Minimum delay between issues to the tag array", "tag_issue_lat", 0 )

  PARAMETER( DataLatency, int, "Total latency of data array lookup", "data_lat", 1 )
  PARAMETER( DataIssueLatency, int, "Minimum delay between issues to the data array", "data_issue_lat", 0 )

  PARAMETER( QueueSize, int, "Size of input and output queues", "queue_size", 8 )
  PARAMETER( MAFSize, int, "Number of MAF entries", "maf_size", 32 )

  PARAMETER( DirEvictBufferSize, int, "Number of Evict Buffer entries for the directory", "dir_eb_size", 40 )
  PARAMETER( CacheEvictBufferSize, int, "Number of Evict Buffer entries for the cache", "cache_eb_size", 40 )

  PARAMETER( Policy, std::string, "Coherence policy for higher caches (NonInclusiveMESI)", "policy", "NonInclusiveMESI" )

  PARAMETER( ControllerType, std::string, "Type of controller (Default or Detailed)", "controller", "Default" )

  PARAMETER( DirectoryType, std::string, "Type of directory (infinite, std, region, etc.)", "dir_type", "infinite" )
  PARAMETER( DirectoryConfig, std::string, "Configuration of directory array (sets=1024:assoc=16)", "dir_config", "sets=1024:assoc=16" )

  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )

  PARAMETER( EvictClean, bool, "Cause the cache to evict clean blocks", "allow_evict_clean", false )
  PARAMETER( ArrayConfiguration, std::string, "Configuration of cache array (STD:sets=1024:assoc=16:repl=LRU", "array_config", "STD:sets=1024:assoc=16:repl=LRU" )

);

COMPONENT_INTERFACE(
  PORT(PushOutput, MemoryTransport, Request_Out)
  PORT(PushOutput, MemoryTransport, Snoop_Out)
  PORT(PushOutput, MemoryTransport, Reply_Out)
  PORT(PushInput, MemoryTransport, Request_In)
  PORT(PushInput, MemoryTransport, Snoop_In)
  PORT(PushInput, MemoryTransport, Reply_In)
  DRIVE(CMPCacheDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMPCache
// clang-format on