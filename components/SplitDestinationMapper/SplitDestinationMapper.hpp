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
