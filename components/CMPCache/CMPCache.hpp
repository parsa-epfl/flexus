// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

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
