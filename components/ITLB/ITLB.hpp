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

// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info

#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

#define FLEXUS_BEGIN_COMPONENT ITLB
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Cores, int, "Number of cores", "cores", 1 )
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )
  PARAMETER( ArrayConfiguration, std::string, "Configuration of cache array (STD:sets=1024:assoc=16:repl=LRU", "array_config", "STD:sets=1024:assoc=16:repl=LRU" )
  PARAMETER( TextFlexpoints, bool, "Store flexpoints as text files (compatible with old FastCache component)", "text_flexpoints", false )
  PARAMETER( GZipFlexpoints, bool, "Compress flexpoints with gzip", "gzip_flexpoints", true )
  /*
  PARAMETER( BlockSize, int, "Block size", "bsize", 8 ) // 64b = 8B entries
  PARAMETER( QueueSizes, uint32_t, "Size of input and output queues", "queue_size", 8 )
  PARAMETER( PreQueueSizes, uint32_t, "Size of input arbitration queues", "pre_queue_size", 4 )
  PARAMETER( MAFSize, uint32_t, "Number of MAF entries", "maf", 32 )
  PARAMETER( MAFTargetsPerRequest, uint32_t, "Number of MAF targets per request", "maf_targets", 8)
  PARAMETER( EvictBufferSize, uint32_t, "Number of Evict Buffer entries", "eb", 40 )
  PARAMETER( SnoopBufferSize, uint32_t, "Number of Snoop Buffer entries", "snoops", 8 )
  PARAMETER( ProbeFetchMiss, bool, "Probe hierarchy on Ifetch miss", "probe_fetchmiss", false )
  PARAMETER( EvictClean, bool, "Cause the cache to evict clean blocks", "allow_evict_clean", false )
  PARAMETER( EvictWritableHasData, bool, "Send data with EvictWritable messages", "evict_writable_has_data", false )
  PARAMETER( EvictOnSnoop, bool, "Send evictions on Snoop channel", "evict_on_snoop", false )
  PARAMETER( FastEvictClean, bool, "Send clean evicts without reserving data bus", "fast_evict_clean", false )
  PARAMETER( TraceAddress, uint32_t, "Address to initiate tracing", "trace_address", 0 )
  PARAMETER( CacheType, std::string, "Type of cache (InclusiveMOESI)", "cache_type", "InclusiveMOESI" )
  PARAMETER( UseReplyChannel, bool, "Separate Reply and Snoop channels on BackSide", "use_reply_channel", false )
  PARAMETER( BusTime_NoData, uint32_t, "Bus transfer time - no data", "bustime_nodata", 1 )
  PARAMETER( BusTime_Data, uint32_t, "Bus transfer time - data", "bustime_data", 2 )
  */
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_I )
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Snoop)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Request)
  PORT(PushOutput, MemoryTransport, BackSideOut_Reply)
  PORT(PushOutput, MemoryTransport, BackSideOut_Snoop)
  PORT(PushOutput, MemoryTransport, BackSideOut_Request)
  PORT(PushInput, MemoryTransport, BackSideIn_Request)
  PORT(PushInput, MemoryTransport, BackSideIn_Reply)
  DRIVE(CacheDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT ITLB
