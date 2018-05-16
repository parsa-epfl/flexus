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


/*! \file CacheController.hpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 *     twenisch    03 Sep 04 - Split implementation out to compile separately
 *     zebchuk     27 Aug 08 - New Coherence model and changes to support it
 */


#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/flexus.hpp>
#include <core/performance/profile.hpp>
#include <core/metaprogram.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;
#include <boost/none.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/TraceTracker.hpp>

using namespace Flexus;

#include <components/CommonQEMU/MessageQueues.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>

#include "CacheController.hpp"
#include "BaseCacheControllerImpl.hpp"

#define DBG_DeclareCategories Cache
//#define DBG_SetDefaultOps AddCat(Cache)
#define DBG_SetDefaultOps AddCat(Cache) Set( (CompName) << theName ) Set( (CompIdx) << theNodeId )
#include DBG_Control()

namespace nCache {

std::ostream & operator << ( std::ostream & s, const enum ProcessType eProcess ) {
  const char * process_types[] = {
    "Request Process",
    "Prefetch Process",
    "Snoop Process",
    "Back Request Process",
    "Back Reply Process",
    "MAFWakeup Process",
    "IProbe Process",
    "Eviction Process",
    "NoMoreWork Process",
    "IdleWork Process",
    "WakeSnoop Process"
  };
  return s << process_types[eProcess];
}

std::ostream & operator << ( std::ostream & s, ProcessEntry   &  process ) {
  s << process.type();
  if (process.transport()[MemoryMessageTag]) {
    s << ": OrigMessage = " << *(process.transport()[MemoryMessageTag]);
  }
  s << " Reservations: ";
  if (process.getReservations() & kResFrontSideOut) {
    s << "FrontSideOut |";
  }
  if (process.getReservations() & kResBackSideOut_Request) {
    s << "BackSideOut_Request |";
  }
  if (process.getReservations() & kResBackSideOut_Snoop) {
    s << "BackSideOut_Snoop |";
  }
  if (process.getReservations() & kResBackSideOut_Reply) {
    s << "BackSideOut_Reply |";
  }
  if (process.getReservations() & kResBackSideOut_Prefetch) {
    s << "BackSideOut_Prefetch |";
  }
  if (process.getReservations() & kResEvictBuffer) {
    s << "EvictBuffer |";
  }
  if (process.getReservations() & kResSnoopBuffer) {
    s << "SnoopBuffer |";
  }
  if (process.getReservations() & kResScheduledEvict) {
    s << "ScheduledEvict |";
  }
  if (process.getReservations() & kResMaf) {
    s << "MAF |";
  }
  if (process.tagOutstanding()) {
    s << " TagOutstanding";
  }
  if (process.requiresData()) {
    s << " requiresData=" << process.requiresData();
  }
  if (process.removeMafEntry()) {
    s << " removeMafEntry";
  }
  if (process.hasFrontMessage()) {
    s << " hasFrontMessage";
  }
  if (process.hasBackMessage()) {
    s << " hasBackMessage";
  }
  return s;
}

// Now that there are an array of FrontSideOut ports
// we need to look at each one to determine whether
// the empty()/full() conditions are met globally.
const bool CacheController::isFrontSideOutEmpty_D ( void ) const {
  for ( int32_t i = 0; i < theCores; i++ )
    if ( !FrontSideOut_D[i].empty() )
      return false;
  return true;
}

const bool CacheController::isFrontSideOutEmpty_I ( void ) const {
  for ( int32_t i = 0; i < theCores; i++ )
    if ( !FrontSideOut_I[i].empty() )
      return false;
  return true;
}

const bool CacheController::isFrontSideOutFull ( void ) const {
  for ( int32_t i = 0; i < theCores; i++ )
    if ( FrontSideOut_I[i].full(theFrontSideOutReserve) || FrontSideOut_D[i].full(theFrontSideOutReserve) )
      return true;
  return false;
}

const bool CacheController::isQueueSetEmpty ( const std::vector< MessageQueue<MemoryTransport> > & queues ) const {
  for ( int32_t i = 0; i < theCores; i++ ) {
    if ( !queues[i].empty() )
      return false;
  }

  return true;
}

const bool CacheController::isPipelineSetEmpty ( const std::vector<Pipeline> & pipelines ) const {
  for ( int32_t i = 0; i < theBanks; i++ ) {
    if ( !pipelines[i].empty() )
      return false;
  }
  return true;
}

const bool CacheController::isQueueSetFull ( const MessageQueue<MemoryTransport> * const & queues ) const {

  for ( int32_t i = 0; i < theCores; i++ ) {
    if ( !queues[i].full() )
      return false;
  }

  return true;
}

const bool CacheController::isWakeMAFListEmpty() const {
  for ( int32_t i = 0; i < theBanks; i++ ) {
    if ( !theWakeMAFList[i].empty() )
      return false;
  }
  return true;
}

const bool CacheController::isIProbeListEmpty() const {
  for ( int32_t i = 0; i < theBanks; i++ ) {
    if ( !theIProbeList[i].empty() )
      return false;
  }
  return true;

}

const uint32_t CacheController::getBank ( const ProcessEntry_p entry ) const {
  return getBank ( addressOf ( entry ) );
}

const uint32_t CacheController::getBank ( const MemoryTransport trans ) const {
  return getBank ( trans[MemoryMessageTag]->address() );
}

const uint32_t CacheController::getBank ( const boost::intrusive_ptr<MemoryMessage> msg ) const {
  return getBank ( msg->address() );
}

const uint32_t CacheController::getBank ( const MemoryAddress address ) const {
  // Hard-coded for 64-byte lines
  return ( ( ((uint64_t)address) >> 6) % theBanks );
}

uint32_t CacheController::totalPipelineSize ( std::vector<Pipeline> & pipe ) const {
  uint32_t
  size = 0;

  for ( uint32_t i = 0; i < pipe.size(); i++ ) {
    size += pipe[i].size();
  }

  return size;
}

bool CacheController::isQuiesced() const {
  return    theMaf.empty()
            &&      isPipelineSetEmpty ( theMAFPipeline )
            &&      isPipelineSetEmpty ( theTagPipeline )
            &&      isPipelineSetEmpty ( theDataPipeline )
            &&      isWakeMAFListEmpty()
            &&      isIProbeListEmpty()
            &&      isQueueSetEmpty ( FrontSideIn_Snoop )
            &&      isQueueSetEmpty ( FrontSideIn_Request )
            &&      isQueueSetEmpty ( FrontSideIn_Prefetch )
            &&      BackSideIn_Reply[0].empty()
            &&      BackSideIn_Request[0].empty()
            &&      isFrontSideOutEmpty_I()
            &&      isFrontSideOutEmpty_D()
            &&      BackSideOut_Reply.empty()
            &&      BackSideOut_Snoop.empty()
            &&      BackSideOut_Request.empty()
            &&      BackSideOut_Prefetch.empty()
            &&      theCacheControllerImpl->isQuiesced()
            ;
}

void CacheController::saveState(std::string const & aDirName) {
  theCacheControllerImpl->saveState( aDirName );
}

void CacheController::loadState(std::string const & aDirName, bool aTextFlexpoint, bool aGZippedFlexpoint) {
  theCacheControllerImpl->loadState( aDirName, aTextFlexpoint, aGZippedFlexpoint);
}

CacheController::CacheController
( std::string const & aName
  , int32_t aCores
  , std::string const & anArrayConfiguration
  , int32_t aBlockSize
  , uint32_t aBanks
  , uint32_t aPorts
  , uint32_t aTagLatency
  , uint32_t aTagIssueLatency
  , uint32_t aDataLatency
  , uint32_t aDataIssueLatency
  , int32_t nodeId
  , tFillLevel aCacheLevel
  , uint32_t aQueueSize
  , uint32_t aPreQueueSize
  , uint32_t aMAFSize
  , uint32_t aMAFTargetsPerRequest
  , uint32_t anEBSize
  , uint32_t aSnoopBufSize
  , bool aProbeOnIfetchMiss
  , bool aDoCleanEvictions
  , bool aWritableEvictsHaveData
  , const std::string & aCacheType
  , uint32_t aTraceAddress
  , bool anAllowOffChipStreamFetch
  , bool anEvictOnSnoop
  , bool anUseReplyChannel
) : theName(aName)
  , theCacheInitInfo ( aName,
                       aCores,
                       anArrayConfiguration,
                       aBlockSize,
                       nodeId,
                       aCacheLevel,
                       anEBSize,
                       aSnoopBufSize,
                       aProbeOnIfetchMiss,
                       aDoCleanEvictions,
                       aWritableEvictsHaveData,
                       anAllowOffChipStreamFetch
                     )
  , theCacheControllerImpl(
    BaseCacheControllerImpl::construct ( this,
                                         &theCacheInitInfo,
                                         aCacheType ))
  , theCores ( aCores )
  , theBanks ( aBanks )
  , thePorts ( aPorts )
  , theNodeId ( nodeId )
  , theBlockSize ( aBlockSize )
  , theEvictOnSnoop(anEvictOnSnoop)
  , theUseReplyChannel(anUseReplyChannel)
  , theMaf(aName, aMAFSize, aMAFTargetsPerRequest)
  , theScheduledEvicts(0)
  , theFrontSideOutReserve(0)
  , theMafUtilization( aName + "-MafUtilization")
  , theTagUtilization( aName + "-TagUtilization")
  , theDataUtilization( aName + "-DataUtilization")
  , BackSideOut_Reply(aQueueSize + 1)
  , BackSideOut_Snoop(aQueueSize + 1)
  , BackSideOut_Request(aQueueSize)
  , BackSideOut_Prefetch(aQueueSize) {
  BackSideIn_Reply.push_back ( MessageQueue<MemoryTransport> (aQueueSize) );
  BackSideIn_Request.push_back ( MessageQueue<MemoryTransport> (aQueueSize) );

  for ( int32_t i = 0; i < theCores; i++ ) {
    FrontSideOut_I.push_back ( MessageQueue<MemoryTransport>(aQueueSize) );
    FrontSideOut_D.push_back ( MessageQueue<MemoryTransport>(aQueueSize) );

    // The input queues per processor must be a minimum size for
    // compatibility with cores that must insert more than one message
    // per cycle.
    // Nikos: In the CMT we only use FrontSideIn_*[0].
    FrontSideIn_Snoop.push_back    ( MessageQueue<MemoryTransport>(aPreQueueSize) );
    FrontSideIn_Request.push_back  ( MessageQueue<MemoryTransport>(aPreQueueSize) );
    FrontSideIn_Prefetch.push_back ( MessageQueue<MemoryTransport>(aPreQueueSize) );
  }

  boost::intrusive_ptr<Stat::StatLog2Histogram>
  mafHist          = new Stat::StatLog2Histogram ( aName + "-MafServer"    +  "-InterArrivalTimes" ),
  tagHist          = new Stat::StatLog2Histogram ( aName + "-TagServer"    +  "-InterArrivalTimes" ),
  dataHist         = new Stat::StatLog2Histogram ( aName + "-DataServer"   +  "-InterArrivalTimes" );

  // Allocate per-bank resources
  for ( int32_t i = 0; i < theBanks; i++ ) {
    theMAFPipeline.push_back ( Pipeline ( aName + "-MafServer",
                                          aPorts, 1, 0, mafHist ) );

    theTagPipeline.push_back ( Pipeline ( aName + "-TagServer",
                                          aPorts, aTagIssueLatency, aTagLatency, tagHist ) );

    theDataPipeline.push_back ( Pipeline ( aName + "-DataServer",
                                           aPorts, aDataIssueLatency, aDataLatency, dataHist ) );

    theWakeMAFList.push_back ( std::list< std::pair < MemoryAddress, boost::intrusive_ptr<TransactionTracker> > >() );
    theIProbeList.push_back ( std::list< MissAddressFile::maf_iter >() );

    BankFrontSideIn_Snoop.push_back    ( MessageQueue<MemoryTransport>(aQueueSize) );
    BankFrontSideIn_Request.push_back  ( MessageQueue<MemoryTransport>(aQueueSize) );
    BankFrontSideIn_Prefetch.push_back ( MessageQueue<MemoryTransport>(aQueueSize) );
    BankBackSideIn_Request.push_back   ( MessageQueue<MemoryTransport>(aQueueSize) );
    BankBackSideIn_Reply.push_back     ( MessageQueue<MemoryTransport>(aQueueSize) );
  }

  theTraceAddress  = (int64_t)aTraceAddress & ~((int64_t) theBlockSize - 1);
  theTraceTimeout  = 0;

  lastSnoopQueue   = 0;
  lastRequestQueue = 0;
  lastPrefetchQueue = 0;
  theLastScheduledBank =
    theLastTagPipeline =
      theLastDataPipeline = 0;
}

//These methods are used by the Impl to manipulate the MAF

// Warning: this may return completed MAF entries with reply messages.  You may want to
// use the function below which excludes completed messages.
std::list<boost::intrusive_ptr<MemoryMessage> > CacheController::getAllMessages( const MemoryAddress  & aBlockAddress) {
  return theMaf.getAllMessages( aBlockAddress );
}

std::list<boost::intrusive_ptr<MemoryMessage> > CacheController::getAllUncompletedMessages( const MemoryAddress  & aBlockAddress) {
  return theMaf.getAllUncompletedMessages( aBlockAddress );
}

std::pair
< boost::intrusive_ptr<MemoryMessage>
, boost::intrusive_ptr<TransactionTracker>
> CacheController::getWaitingMAFEntry( const MemoryAddress  & aBlockAddress) {
  return theMaf.getWaitingMAFEntry( aBlockAddress );
}

MissAddressFile::maf_iter CacheController::getWaitingMAFEntryIter( const MemoryAddress  & aBlockAddress) {
  MissAddressFile::maf_iter iter = theMaf.getWaitingMAFEntryIter( aBlockAddress );
  // Make sure we're looking for a MAF that actually exists
  // We give the controller access to the actual entry, but not to the whole MAF, so they can't do this check
  DBG_Assert( iter != theMaf.end() );
  return iter;
}

bool CacheController::hasMAF(const MemoryAddress & aBlockAddress) const {
  return theMaf.contains( aBlockAddress );
}

void CacheController::reserveFrontSideOut(ProcessEntry_p aProcess) {
  aProcess->reserve(kResFrontSideOut);
  theFrontSideOutReserve++;
}

void CacheController::reserveBackSideOut_Request(ProcessEntry_p aProcess) {
  aProcess->reserve(kResBackSideOut_Request);
  BackSideOut_Request.reserve();
}

void CacheController::reserveBackSideOut_Evict(ProcessEntry_p aProcess) {
  if (theEvictOnSnoop) {
    aProcess->reserve ( kResBackSideOut_Snoop );
    BackSideOut_Snoop.reserve();
  } else {
    aProcess->reserve ( kResBackSideOut_Request );
    BackSideOut_Request.reserve();
  }
}

void CacheController::reserveBackSideOut_Snoop(ProcessEntry_p aProcess) {
  if (theUseReplyChannel) {
    aProcess->reserve ( kResBackSideOut_Reply );
    BackSideOut_Reply.reserve();
  } else {
    aProcess->reserve ( kResBackSideOut_Snoop );
    BackSideOut_Snoop.reserve();
  }
}

void CacheController::reserveBackSideOut_Prefetch(ProcessEntry_p aProcess) {
  aProcess->reserve ( kResBackSideOut_Prefetch );
  BackSideOut_Prefetch.reserve();
}

void CacheController::reserveEvictBuffer(ProcessEntry_p aProcess) {
  aProcess->reserve ( kResEvictBuffer );
  theCacheControllerImpl->reserveEvictBuffer();
}

void CacheController::reserveSnoopBuffer(ProcessEntry_p aProcess) {
  aProcess->reserve ( kResSnoopBuffer );
  theCacheControllerImpl->reserveSnoopBuffer();
}

void CacheController::reserveScheduledEvict(ProcessEntry_p aProcess) {
  aProcess->reserve ( kResScheduledEvict );
  theScheduledEvicts++;
}

void CacheController::reserveMAF(ProcessEntry_p aProcess) {
  aProcess->reserve ( kResMaf );
  theMaf.reserve();
}

// Unreserves queue and structured entries for a process
// in a centralized bitmask "stew"
void CacheController::unreserveFrontSideOut ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResFrontSideOut );
  theFrontSideOutReserve--;
}

void CacheController::unreserveBackSideOut_Request ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResBackSideOut_Request );
  BackSideOut_Request.unreserve();
}

void CacheController::unreserveBackSideOut_Evict ( ProcessEntry_p aProcess ) {
  if (theEvictOnSnoop) {
    aProcess->unreserve ( kResBackSideOut_Snoop );
    BackSideOut_Snoop.unreserve();
  } else {
    aProcess->unreserve ( kResBackSideOut_Request );
    BackSideOut_Request.unreserve();
  }
}

void CacheController::unreserveBackSideOut_Snoop ( ProcessEntry_p aProcess ) {
  if (theUseReplyChannel) {
    aProcess->unreserve ( kResBackSideOut_Reply );
    BackSideOut_Reply.unreserve();
  } else {
    aProcess->unreserve ( kResBackSideOut_Snoop );
    BackSideOut_Snoop.unreserve();
  }
}

void CacheController::unreserveBackSideOut_Prefetch ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResBackSideOut_Prefetch );
  BackSideOut_Prefetch.unreserve();
}

void CacheController::unreserveEvictBuffer ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResEvictBuffer );
  theCacheControllerImpl->unreserveEvictBuffer();
}

void CacheController::clearEvictBufferReservation ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResEvictBuffer );
}

void CacheController::unreserveSnoopBuffer ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResSnoopBuffer );
  theCacheControllerImpl->unreserveSnoopBuffer();
}

void CacheController::unreserveScheduledEvict ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResScheduledEvict );
  theScheduledEvicts--;
}

void CacheController::unreserveMAF ( ProcessEntry_p aProcess ) {
  aProcess->unreserve ( kResMaf );
  theMaf.unreserve();
}

void CacheController::unreserveBSO ( ProcessEntry_p aProcess ) {
  switch ( aProcess->type() ) {
    case eProcPrefetch:
      unreserveBackSideOut_Prefetch ( aProcess );
      break;
    case eProcSnoop:
    case eProcBackReply:
    case eProcBackRequest:
      unreserveBackSideOut_Snoop ( aProcess );
      break;
    case eProcEviction:
      unreserveBackSideOut_Evict ( aProcess );
      break;
    case eProcRequest:
    case eProcIProbe:
    case eProcMAFWakeup:
      unreserveBackSideOut_Request ( aProcess );
      break;
    default:
      DBG_Assert ( false,
                   ( << " Cannot unreserve resources for processes of type: "
                     << aProcess->type() ) );
  }
}

// Enqueue requests in the appropriate tag pipelines
void CacheController::enqueueTagPipeline ( Action         action,
    ProcessEntry_p aProcess ) {
  DBG_Assert ( action.theRequiresTag > 0 );

  if ( action.theRequiresTag ) {
    theTagPipeline[getBank(aProcess)].enqueue ( aProcess,
        action.theRequiresTag );
  }

}

//Process all pending stuff in the cache.  This is called once each cycle.
//It iterates over all the message and process queues, moving things along
//if they are ready to go and there is no back pressure.  The order of
//operations in this function is important - it is designed to make sure
//a request can be received and processed in a single cycle for L1 cache
//hits
void CacheController::processMessages() {
  FLEXUS_PROFILE();
  DBG_(VVerb, ( << " Process messages" ) );

  {
    // Give a dummy variable to the backside in, which has one queue
    int32_t i = 0;
    doNewRequests ( BackSideIn_Reply, BankBackSideIn_Reply, 1, i );
    doNewRequests ( BackSideIn_Request, BankBackSideIn_Request, 1, i );
  }

  doNewRequests ( FrontSideIn_Snoop,    BankFrontSideIn_Snoop,    theCores, lastSnoopQueue    );
  doNewRequests ( FrontSideIn_Request,  BankFrontSideIn_Request,  theCores, lastRequestQueue  );
  doNewRequests ( FrontSideIn_Prefetch, BankFrontSideIn_Prefetch, theCores, lastPrefetchQueue );

  theMafUtilization << std::make_pair( totalPipelineSize ( theMAFPipeline ), 1 );
  theTagUtilization << std::make_pair( totalPipelineSize ( theTagPipeline ), 1 );
  theDataUtilization << std::make_pair( totalPipelineSize ( theDataPipeline ), 1 );

  // End address tracing?
  if ( theTraceTimeout > 0 &&
       theTraceTimeout < theFlexus->cycleCount() ) {
    theTraceTimeout = 0;
    theFlexus->setDebug ( "dev" );
  }

  //Insert new processes into maf pipeline.
  scheduleNewProcesses();

  //Drain maf pipeline
  for ( int32_t i = 0; i < theBanks; i++ ) {
    while ( theMAFPipeline[i].ready() && theTagPipeline[i].serverAvail()) {
      ProcessEntry_p process = theMAFPipeline[i].dequeue();
      switch ( process->type() ) {
        case eProcRequest:
        case eProcPrefetch:
          runRequestProcess( process );
          break;
        case eProcSnoop:
          runSnoopProcess( process );
          break;
        case eProcBackRequest:
        case eProcBackReply:
          runBackProcess( process );
          break;
        case eProcMAFWakeup:
          runWakeMafProcess( process );
          break;
        case eProcIProbe:
          runIProbeProcess( process );
          break;
        case eProcEviction:
          runEvictProcess( process );
          break;
        case eProcIdleWork:
          runIdleWorkProcess( process );
          break;
        case eProcWakeSnoop:
          runWakeSnoopProcess( process );
          break;
        default:
          DBG_Assert(false);
      }
    }
  }

  advancePipelines();

}

// Schedule the Tag and Data pipelines for a non-CMP cache
void CacheController::advancePipelines ( void ) {
  FLEXUS_PROFILE();
  int32_t i, bankCount;

  i = theLastTagPipeline;
  theLastTagPipeline = ( theLastTagPipeline + 1 ) % theBanks;
  for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
    //Drain tag pipeline
    while ( theTagPipeline[i].ready() ) {
      ProcessEntry_p process = theTagPipeline[i].peek();
      if (process->requiresData()) {
        if (theDataPipeline[i].serverAvail()) {
          if (process->transmitAfterTag()) {
            doTransmitProcess( process );
          }
          theTagPipeline[i].dequeue();
          theDataPipeline[i].enqueue( process );
        } else {
          theTagPipeline[i].stall();
          break;
        }
      } else {
        theTagPipeline[i].dequeue();
        doTransmitProcess( process );
      }
    }
    i = ( i + 1 ) % theBanks;
  }

  //Drain data pipeline
  i = theLastDataPipeline;
  theLastDataPipeline = ( theLastDataPipeline + 1 ) % theBanks;
  for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
    while ( theDataPipeline[i].ready() ) {
      ProcessEntry_p process = theDataPipeline[i].dequeue();
      doTransmitProcess( process );
    }
    i = ( i + 1 ) % theBanks;
  }

}

//Insert new processes into maf pipeline.  We can return once we are out
//of ready MAF servers
void CacheController::scheduleNewProcesses() {
  FLEXUS_PROFILE();

  int32_t i, bankCount;

  for ( i = theLastScheduledBank, bankCount = 0;
        bankCount < theBanks;
        i = ( i + 1 ) % theBanks, bankCount++ ) {
    bool scheduled = false;
    bool evict_waiting_on_idle_work = false;
    bool active_msg_waiting_on_evict = false;

    //First,  MAF entries that woke up are allocated into the MAF pipeline
    //A woken MAF process must reserve the same things as a request process
    //except that it does not need a MAF entry.
    while (      theMAFPipeline[i].serverAvail()
                 && ! theWakeMAFList[i].empty()
                 && ! BackSideOut_Request.full()
                 && ! isFrontSideOutFull()
                 && ! theCacheControllerImpl->fullEvictBuffer()
                 && theCacheControllerImpl->canStartRequest(theWakeMAFList[i].front().first)
          ) {
      std::pair < MemoryAddress, boost::intrusive_ptr<TransactionTracker> > wake_entry = theWakeMAFList[i].front();
      theWakeMAFList[i].pop_front();
      MissAddressFile::maf_iter iter = theMaf.getBlockedMafEntry(wake_entry.first);
      if (iter == theMaf.end()) {
        DBG_(Trace, ( << "Woke MAF entry for addr: " << std::hex << wake_entry.first << " but failed to find blocked maf entry." ));
        continue;
      }

      // Should this really be here?  Livelock is possible!
      //if (theMaf.contains( wake_entry.first, kWaitAddress ) ) {
      //More MAF entries to wake
      //theWakeMAFList.push_back(wake_entry);
      //}

      DBG_(Trace, ( << " schedule WakeMAF " << *iter->transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( iter, wake_entry.second, eProcMAFWakeup );
      reserveFrontSideOut( aProcess );
      reserveBackSideOut_Request( aProcess );
      reserveEvictBuffer( aProcess );

      theMAFPipeline[i].enqueue ( aProcess );
      theCacheControllerImpl->addPendingRequest(wake_entry.first);
      scheduled = true;
    }
    if (!scheduled && !theWakeMAFList[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule WakeMAF: BSO_Req full " << std::boolalpha << BackSideOut_Request.full() << ", FSO Full " << isFrontSideOutFull() << ", EB full " << theCacheControllerImpl->fullEvictBuffer() ));

      // If there's a full Evict Buffer, signal that we're waiting on it so Idle work gets scheduled even if we're quiescing.
      if (theCacheControllerImpl->fullEvictBuffer()) {
        active_msg_waiting_on_evict = true;
      }
    }

    //Next, wake up IProbe MAF entries
    //A woken IProbe process must reserve a FrontSideOut and a BackSideOut_Request.
    while (       theMAFPipeline[i].serverAvail()
                  && ! theIProbeList[i].empty()
                  && ! BackSideOut_Request.full()
                  && ! isFrontSideOutFull()
          ) {
      MissAddressFile::maf_iter iter = theIProbeList[i].front();
      theIProbeList[i].pop_front();

      DBG_(Iface, ( << " schedule IProbe " << *iter->transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( iter, iter->transport[TransactionTrackerTag], eProcIProbe );
      reserveFrontSideOut( aProcess );
      reserveBackSideOut_Request( aProcess );
      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && !theIProbeList[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule IProbe: BSO_Req full " << std::boolalpha << BackSideOut_Request.full() << ", FSO Full " << isFrontSideOutFull() ));
    }

    // Look for BackSideSnoop Processes that were woken in the Snoop Buffer
    while ( theMAFPipeline[i].serverAvail()
            && theCacheControllerImpl->hasWakingSnoops()
            && ! isFrontSideOutFull()
            && ! (theUseReplyChannel ? BackSideOut_Reply.full() : BackSideOut_Snoop.full())
          ) {
      MemoryTransport transport = theCacheControllerImpl->getWakingSnoopTransport();

      ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcWakeSnoop );
      reserveFrontSideOut( aProcess );
      reserveBackSideOut_Snoop( aProcess );
      reserveSnoopBuffer( aProcess );

      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && theCacheControllerImpl->hasWakingSnoops()) {
      DBG_(Trace, ( << "Failed to schedule WakeSnoop: BSO_Snoop full " << std::boolalpha << BackSideOut_Snoop.full() << ", FSO Full " << isFrontSideOutFull() ));
    }

    //Next, allocate eviction processes if the evict buffer could become
    //fully reserved (i.e., < # ports entries free).  Evict buffer entries
    //reserve a BackSideOut_Snoop buffer, and increment theScheduledEvicts.

    // FIXME: allocates resources for any bank to process the eviction, instead of the
    // specific bank pipe which owns the victim. On average, this still produces
    // fair resource consumption.
    while ( theMAFPipeline[i].serverAvail()
            && theCacheControllerImpl->evictableBlockExists(theScheduledEvicts)
            && (theCacheControllerImpl->freeEvictBuffer() + theScheduledEvicts <= thePorts)
            && (theEvictOnSnoop ? !BackSideOut_Snoop.full() : !BackSideOut_Request.full())
          ) {
      DBG_(Trace, ( << " schedule Evict" ) );

      theCacheControllerImpl->dumpEvictBuffer();

      ProcessEntry_p aProcess = new ProcessEntry ( eProcEviction );
      reserveBackSideOut_Evict( aProcess );
      //reserveBackSideOut_Snoop( aProcess );
      reserveScheduledEvict( aProcess );
      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && theCacheControllerImpl->evictableBlockExists(theScheduledEvicts) && (theCacheControllerImpl->freeEvictBuffer() + theScheduledEvicts <= thePorts)) {
      DBG_(Trace, ( << "Failed to schedule Forced Evict: BSO_Snoop full " << std::boolalpha << BackSideOut_Snoop.full() ));
    } else if (theCacheControllerImpl->fullEvictBuffer()) {
      DBG_(Trace, ( << "Failed to schedule Forced Evict: BSO_Snoop full " << std::boolalpha << BackSideOut_Snoop.full() << ", scheduled evicts " << theScheduledEvicts << ", freeEvictBuffer() = " << theCacheControllerImpl->freeEvictBuffer() << ", ports = " << thePorts << " evictableBlockExists? " << theCacheControllerImpl->evictableBlockExists(theScheduledEvicts) ));
      theCacheControllerImpl->dumpEvictBuffer();
      if (!theCacheControllerImpl->evictableBlockExists(theScheduledEvicts)) {
        DBG_(Trace, ( << "evict_waiting_on_idle_work = true" ));
        evict_waiting_on_idle_work = true;
      }
    } else if ( theFlexus->quiescing() && !theCacheControllerImpl->evictableBlockExists(theScheduledEvicts) && (theCacheControllerImpl->freeEvictBuffer() + theScheduledEvicts <= thePorts)) {
      // Try to schedule more idle work to avoid waiting any longer while quiescing
      evict_waiting_on_idle_work = true;
    }

    //Next, allocate back processes.  Back Reply processes reserve a FrontSideOut
    //buffer, and a BackSideOut_Snoop buffer
    // We don't allocate an evict buffer entry, but instead relying on having one from
    // an earlier request
    while (      theMAFPipeline[i].serverAvail()
                 && ! BankBackSideIn_Reply[i].empty()
                 && ! isFrontSideOutFull()
                 && ! (theUseReplyChannel ? BackSideOut_Reply.full() : BackSideOut_Snoop.full())
          ) {
      MemoryTransport transport(BankBackSideIn_Reply[i].dequeue());
      DBG_(Iface, ( << " schedule Back " << *transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcBackReply );
      reserveFrontSideOut( aProcess );
      reserveBackSideOut_Snoop( aProcess );
      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && !BankBackSideIn_Reply[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule Back Reply: BSO_Snoop full " << std::boolalpha << BackSideOut_Snoop.full() << ", FSO Full " << isFrontSideOutFull() ));
    }

    //Next, allocate back processes.  Back processes reserve a FrontSideOut
    //buffer, and a BackSideOut_Snoop buffer
    // We don't allocate an evict buffer entry, but instead relying on having one from
    // an earlier request
    while (      theMAFPipeline[i].serverAvail()
                 && ! BankBackSideIn_Request[i].empty()
                 && ! isFrontSideOutFull()
                 && ! (theUseReplyChannel ? BackSideOut_Reply.full() : BackSideOut_Snoop.full())
                 && ! theCacheControllerImpl->fullSnoopBuffer()
          ) {
      MemoryTransport transport(BankBackSideIn_Request[i].dequeue());
      DBG_(Iface, ( << " schedule Back " << *transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcBackRequest );
      reserveFrontSideOut( aProcess );
      reserveSnoopBuffer( aProcess );
      reserveBackSideOut_Snoop( aProcess );
      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && !BankBackSideIn_Request[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule BackSide Request (Snoop): BSO_Snoop full " << std::boolalpha << BackSideOut_Snoop.full() << ", FSO Full " << isFrontSideOutFull() << ", SB full " << theCacheControllerImpl->fullSnoopBuffer() ));
    }

    //Next, allocate request processes if they have a newer timestamp than
    //the newest waiting snoop process.  Request processes reserve a MAF
    //entry, a BackSideOut_Request buffer, and a FrontSideOut buffer.
    // We also reserve an evict buffer that might be used later when we
    // receive a backside in reply
    while (      theMAFPipeline[i].serverAvail()
                 && ! BankFrontSideIn_Request[i].empty()
                 && ! BackSideOut_Request.full()
                 && ! isFrontSideOutFull()
                 && ! theMaf.full()
                 && ! theCacheControllerImpl->fullEvictBuffer()
                 && theCacheControllerImpl->canStartRequest(BankFrontSideIn_Request[i].peek()[MemoryMessageTag]->address())
                 && ( BankFrontSideIn_Snoop[i].empty() ||
                      BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Request[i].headTimestamp() )
          ) {
      MemoryTransport transport(BankFrontSideIn_Request[i].dequeue());
      DBG_(Iface, ( << " schedule Request " << *transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcRequest );
      reserveBackSideOut_Request( aProcess );
      reserveFrontSideOut( aProcess );
      reserveEvictBuffer( aProcess );
      reserveMAF( aProcess );
      theMAFPipeline[i].enqueue( aProcess );
      theCacheControllerImpl->addPendingRequest(transport[MemoryMessageTag]->address());
      scheduled = true;
    }
    if (!scheduled && !BankFrontSideIn_Request[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule Request: BSO_Req full " << std::boolalpha << BackSideOut_Request.full() << ", FSO Full " << isFrontSideOutFull() << ", MAF full " << theMaf.full() << ", EB full " << theCacheControllerImpl->fullEvictBuffer() << " older snoop avail: " << !(BankFrontSideIn_Snoop[i].empty() || BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Request[i].headTimestamp()) << " canStartRequest(" << std::hex << BankFrontSideIn_Request[i].peek()[MemoryMessageTag]->address() << ") = " << theCacheControllerImpl->canStartRequest(BankFrontSideIn_Request[i].peek()[MemoryMessageTag]->address()) ));
      if (theCacheControllerImpl->fullEvictBuffer()) {
        theCacheControllerImpl->dumpEvictBuffer();
        active_msg_waiting_on_evict = true;
      }
    }

    //Next, allocate prefetch processes if there are no waiting request
    //processes and the prefetches have a newer timestamp than
    //the newest waiting snoop process.  Prefetch processes reserve a MAF
    //entry, a BackSideOut_Prefetch buffer, and a FrontSideOut buffer.
    while (      theMAFPipeline[i].serverAvail()
                 && ! BankFrontSideIn_Prefetch[i].empty()
                 && ! BackSideOut_Prefetch.full()
                 && ! isFrontSideOutFull()
                 && ! theMaf.full()
                 && ! theCacheControllerImpl->fullEvictBuffer()
                 && ( BankFrontSideIn_Snoop[i].empty() ||
                      BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Prefetch[i].headTimestamp() )
          ) {
      MemoryTransport transport(BankFrontSideIn_Prefetch[i].dequeue());
      DBG_(Iface, ( << " schedule Prefetch " << *transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcPrefetch );
      reserveBackSideOut_Prefetch( aProcess );
      reserveFrontSideOut( aProcess );
      reserveEvictBuffer( aProcess );
      reserveMAF( aProcess );
      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && !BankFrontSideIn_Prefetch[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule Prefetch: BSO_Prefetch full " << std::boolalpha << BackSideOut_Prefetch.full() << ", FSO Full " << isFrontSideOutFull() << ", MAF full " << theMaf.full() << ", EB full " << theCacheControllerImpl->fullEvictBuffer() << " older snoop avail: " << !(BankFrontSideIn_Snoop[i].empty() || BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Prefetch[i].headTimestamp()) ));
    }

    //Next, allocate any snoop processes.
    // Some snoop processes might need an evict buffer, some CAN'T use an evict buffer
    // (to avoid deadlock)
    // So we make this determination conditional
    // Similarly, some need FrontSideOut, some don't, and those that don't shouldn't wait for it
    // We assume the conditions are simple enough to be practically implemented
    while (       theMAFPipeline[i].serverAvail()
                  && ! BankFrontSideIn_Snoop[i].empty()
                  && ! (theUseReplyChannel ? BackSideOut_Reply.full() : BackSideOut_Snoop.full())
                  && theCacheControllerImpl->snoopResourcesAvailable(BankFrontSideIn_Snoop[i].peek()[MemoryMessageTag])
          ) {
      MemoryTransport transport(BankFrontSideIn_Snoop[i].dequeue());
      DBG_(Iface, ( << " schedule Snoop " << *transport[MemoryMessageTag] ) );

      ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcSnoop );
      reserveBackSideOut_Snoop( aProcess );
      theCacheControllerImpl->reserveSnoopResources(transport[MemoryMessageTag], aProcess);

      theMAFPipeline[i].enqueue( aProcess );
      scheduled = true;
    }
    if (!scheduled && !BankFrontSideIn_Snoop[i].empty()) {
      DBG_(Trace, ( << "Failed to schedule FS Snoop: BSO_Snoop full " << std::boolalpha << BackSideOut_Snoop.full() << ", Snoop Resources Avail: " << theCacheControllerImpl->snoopResourcesAvailable(BankFrontSideIn_Snoop[i].peek()[MemoryMessageTag]) ));
    }

    // Check for some idle work
    // This includes sending invalidates for blocks we're trying to evict
    // Or sending eager evicts for blocks RegionTracker is trying to evict
    while ( theMAFPipeline[i].serverAvail()
            && theCacheControllerImpl->idleWorkAvailable()
          ) {
      // Create a new process
      ProcessEntry_p aProcess = new ProcessEntry( eProcIdleWork );

      // Get a message for the idle work
      // This function will also make any resource reservations as well
      aProcess->transport().set(MemoryMessageTag, theCacheControllerImpl->getIdleWorkMessage(aProcess));

      theMAFPipeline[i].enqueue( aProcess );
      DBG_(Trace, ( << "Scheduled Idle Work: " << *(aProcess->transport()[MemoryMessageTag]) ));
    }
  }

  // Make the last lowest-priority bank the highest for the next time around
  theLastScheduledBank = ( theLastScheduledBank + theBanks - 1 ) % theBanks;
}

//For convenience.  Record the wakeup address, and do wakeup.
void CacheController::enqueueWakeMaf(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aWakeTransaction) {
  DBG_(Iface, ( << " enqueueWakeMaf " << (uint64_t)anAddress ) );
  theWakeMAFList[getBank(anAddress)].push_back( std::make_pair(anAddress, aWakeTransaction) );
}

void CacheController::enqueueWakeRegionMaf(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aWakeTransaction) {
  DBG_(Trace, ( << " enqueueWakeRegionMaf " << anAddress ) );
  // Get all maf entries in state WaitRegion
  std::list<MemoryAddress> blocks = theCacheControllerImpl->getRegionBlockList(anAddress);
  for (; !blocks.empty();) {
    MemoryAddress addr = blocks.front();
    blocks.pop_front();
    if (theMaf.contains(addr, kWaitRegion)) {
      theWakeMAFList[getBank(addr)].push_back(std::make_pair(addr, aWakeTransaction));
      DBG_(Trace, ( << "enqueueWakeRegionMaf(" << std::hex << anAddress << ") Waking Maf Waiting with addr: " << addr ));
    } else {
      DBG_(Trace, ( << "enqueueWakeRegionMaf(" << std::hex << anAddress << ") No Maf Waiting with addr: " << addr ));
    }
  }
}

void CacheController::doNewRequests(std::vector< MessageQueue<MemoryTransport> > & aMessageQueue,
                                    std::vector< MessageQueue<MemoryTransport> > & aBankQueue,
                                    const int32_t numMsgQueues,
                                    int32_t & lastQueue ) {
  FLEXUS_PROFILE();
  int32_t i, queueCount;
  bool sentMessages = true;

  while ( sentMessages ) {

    sentMessages = false;

    for ( i = lastQueue, queueCount = 0;
          queueCount < numMsgQueues;
          queueCount++, i = (i + 1) % numMsgQueues ) {

      if ( !aMessageQueue[i].empty() ) {

        uint32_t bank = getBank ( aMessageQueue[i].peek() );

        if (  ( theTraceAddress > 0 ) && ( aMessageQueue[i].peek()[MemoryMessageTag]->address() & ~((int64_t) theBlockSize - 1)) == theTraceAddress ) {
          if ( theTraceTimeout <= 0 ) {
            theTraceTimeout = theFlexus->cycleCount() + 1000;
            theFlexus->setDebug ( "iface" );
          }
        }

        if ( !aBankQueue[bank].full() ) {
          MemoryTransport trans ( aMessageQueue[i].dequeue() );

          DBG_ ( Iface, ( << " scheduling request to bank " << bank << ": " << *trans[MemoryMessageTag] << " " << theCacheInitInfo.theCacheLevel ) );
          aBankQueue[bank].enqueue ( trans );
          sentMessages = true;
        } else {
          DBG_ ( Iface, ( << " bank[" << bank << "] conflict for[" << i << "]: "
                          << *aMessageQueue[i].peek()[MemoryMessageTag] ) );
        }
      }
    }
  }

  lastQueue = ( lastQueue + numMsgQueues - 1 ) % numMsgQueues;
}

void CacheController::runIdleWorkProcess(ProcessEntry_p aProcess ) {
  // We should probably get a TransactionTracker at this point
  TransactionTracker_p tracker = new TransactionTracker;
  tracker->setAddress(aProcess->transport()[MemoryMessageTag]->address());
  tracker->setInitiator(theNodeId);
  tracker->setSource(theName + " IdleWork");
  tracker->setDelayCause(theName, "IdleWork");

  DBG_(Trace, ( << " runIdleWorkProcess serial " << aProcess->serial() << ": " << *aProcess->transport()[MemoryMessageTag] ));
  aProcess->transport().set(TransactionTrackerTag, tracker);

  Action action = theCacheControllerImpl->handleIdleWork( aProcess->transport());
  aProcess->consumeAction(action);

  theCacheControllerImpl->removeIdleWorkReservations(aProcess, action);

  switch (action.theAction) {
    case kNoAction:
      if (aProcess->requiresTag()) {
        enqueueTagPipeline(action, aProcess);
      }
      break;

    case kSend:
      if (action.theFrontMessage) {
        aProcess->enqueueFrontTransport(action);
      }
      if (action.theBackMessage) {
        aProcess->enqueueBackTransport(action);
      }
      if (aProcess->requiresTag()) {
        enqueueTagPipeline ( action, aProcess );
      } else {
        doTransmitProcess(aProcess);
      }
      break;

      // For now assume we don't need these actions.
      // If we do, then try to add support for the
      // case at hand that is as generic as possible
    default:
      DBG_Assert( false, ( << "Unsupported Action type for IdleWorkProcess"));
      break;
  };
}

void CacheController::runWakeSnoopProcess(ProcessEntry_p aProcess ) {
  DBG_(Trace, ( << " runWakeSnoopProcess " << aProcess->serial() << ": " << *aProcess->transport()[MemoryMessageTag] ) );
  TransactionTracker_p tracker = aProcess->transport()[TransactionTrackerTag];

  DBG_Assert(tracker != nullptr);

  Action action = theCacheControllerImpl->handleWakeSnoop( aProcess->transport() );
  aProcess->consumeAction(action);

  unreserveSnoopBuffer(aProcess);

  switch (action.theAction) {
    case kNoAction:
      if (aProcess->requiresTag()) {
        enqueueTagPipeline(action, aProcess);
      }
      unreserveFrontSideOut( aProcess );
      unreserveBackSideOut_Snoop( aProcess );

      break;

    case kSend:
      if (action.theFrontMessage) {
        aProcess->enqueueFrontTransport(action);
      } else {
        unreserveFrontSideOut( aProcess );
      }
      if (action.theBackMessage) {
        aProcess->enqueueBackTransport(action);
      } else {
        unreserveBackSideOut_Snoop( aProcess );
      }
      if (aProcess->requiresTag()) {
        enqueueTagPipeline ( action, aProcess );
      } else {
        doTransmitProcess(aProcess);
      }
      break;

      // For now assume we don't need these actions.
      // If we do, then try to add support for the
      // case at hand that is as generic as possible
    default:
      DBG_Assert( false, ( << "Unsupported Action type for IdleWorkProcess"));
      break;
  };
}

void CacheController::runEvictProcess(ProcessEntry_p aProcess ) {
  FLEXUS_PROFILE();
  DBG_(Trace, ( << " runEvictProcess " << aProcess->serial() << ": " ) );

  Action action = theCacheControllerImpl->doEviction();

  unreserveBackSideOut_Evict( aProcess );
  unreserveScheduledEvict( aProcess );

  // Our Evict might have been stolen by another request that came in before we got here
  // In this case, don't do anything
  if (action.theAction == kNoAction) {
    return;
  }

  DBG_Assert ( action.theBackMessage );

  // JCS - Apparently evictions are free?!?  No pipelines used?!
  // JZ - we already used the MAF pipeline, if we assume the EB is accessed at the same time, we're good
  sendBack_Evict( action.theBackTransport );
}

void CacheController::runRequestProcess(ProcessEntry_p aProcess) {
  FLEXUS_PROFILE();
  DBG_Assert(aProcess->type() == eProcRequest || aProcess->type() == eProcPrefetch);
  DBG_(Trace, ( << " runRequestProcess " << aProcess->serial() << ": " << *aProcess->transport()[MemoryMessageTag] ) );

  bool has_maf_entry = theMaf.contains( addressOf( aProcess) );
  //We call handleRequestMessage even if there is a maf entry outstanding,
  //because PrefetchReadRequests can be handled even if there is an address
  //conflict (they produce PrefetchReadRedudant responses)

  Action action ( kNoAction, aProcess->transport()[TransactionTrackerTag] );

  theCacheControllerImpl->removePendingRequest(aProcess->transport()[MemoryMessageTag]->address());
  action = theCacheControllerImpl->handleRequestTransport( aProcess->transport(), has_maf_entry );

  aProcess->consumeAction ( action );

  DBG_ ( Trace, ( << "  Action for " << *aProcess->transport()[MemoryMessageTag] << " is: " << action.theAction ) );

  switch (action.theAction) {
    case kSend:
      DBG_Assert( aProcess->type() == eProcRequest || aProcess->type() == eProcPrefetch );
      //This is used for PrefetchReadRedundant and other cases which should
      //not wake MAF entries

      unreserveMAF( aProcess );
      unreserveEvictBuffer( aProcess );

      DBG_Assert(action.theFrontMessage == false);
      DBG_Assert(action.theBackMessage);

      unreserveFrontSideOut( aProcess );
      aProcess->enqueueBackTransport(action);

      enqueueTagPipeline ( action, aProcess );
      break;

    case kInsertMAF_WaitAddress:
      DBG_Assert( aProcess->type() == eProcRequest || aProcess->type() == eProcPrefetch );

      //Blocked due to address conflict.
      unreserveMAF( aProcess );
      unreserveFrontSideOut( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer( aProcess );

      theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitAddress);
      theMaf.dump();
      break;

    case kInsertMAF_WaitSnoop:
      DBG_Assert( aProcess->type() == eProcRequest || aProcess->type() == eProcPrefetch );

      //Blocked due to oustanding Snoop.
      unreserveMAF( aProcess );
      unreserveFrontSideOut( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer( aProcess );

      theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitSnoop);
      theMaf.dump();
      break;

    case kInsertMAF_WaitEvict:
      DBG_Assert( aProcess->type() == eProcRequest || aProcess->type() == eProcPrefetch );

      //Blocked due to oustanding Snoop.
      unreserveMAF( aProcess );
      unreserveFrontSideOut( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer( aProcess );

      theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitEvict);
      theMaf.dump();
      break;

    case kInsertMAF_WaitRegion:
      DBG_Assert( aProcess->type() == eProcRequest || aProcess->type() == eProcPrefetch );

      //Blocked due to oustanding Snoop.
      unreserveMAF( aProcess );
      unreserveFrontSideOut( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer( aProcess );

      theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitRegion);
      theMaf.dump();
      break;

    case kInsertMAF_WaitResponse: {

      unreserveMAF( aProcess );

      DBG_Assert(action.theFrontMessage == false);
      DBG_Assert(action.theBackMessage);

      unreserveFrontSideOut( aProcess );
      aProcess->enqueueBackTransport( action );

      theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitResponse);
      enqueueTagPipeline ( action, aProcess );
      break;
    }
    case kInsertMAF_WaitProbe: {

      //Cache needs to probe hierarchy above to determine what to do
      unreserveMAF( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer( aProcess );
      //FrontSideOut still reserved

      theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitProbe);
      DBG_Assert(action.theFrontMessage);
      aProcess->enqueueFrontTransport( action );

      enqueueTagPipeline ( action, aProcess );
      break;
    }
    case kReplyAndRemoveMAF: {
      //Request was satisfied.  It never got a MAF entry, so no need to
      //remove.

      DBG_Assert(action.theFrontMessage);
      aProcess->enqueueFrontTransport( action );

      unreserveMAF( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer ( aProcess );
      //FrontSideOut still reserved

      enqueueTagPipeline ( action, aProcess );
      break;
    }
    default:
      //Not possible for requests
      DBG_Assert( false, ( << "Unexpected result from runRequestProcess: " << action << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );
  }
}

//If there is a MAF wakeup in progress, continue it.
void CacheController::runWakeMafProcess(ProcessEntry_p aProcess) {
  FLEXUS_PROFILE();
  DBG_Assert(aProcess->type() == eProcMAFWakeup );
  DBG_(Trace, ( << " runWakeMafProcess " << aProcess->serial() << ": " << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

  theCacheControllerImpl->removePendingRequest(aProcess->mafEntry()->transport[MemoryMessageTag]->address());
  Action action = theCacheControllerImpl->wakeMaf( aProcess->mafEntry()->transport, aProcess->wakeTrans());

  aProcess->consumeAction ( action );

  switch (action.theAction) {
    case kInsertMAF_WaitResponse: {

      DBG_Assert(action.theFrontMessage == false);
      DBG_Assert(action.theBackMessage);

      unreserveFrontSideOut( aProcess );
      aProcess->enqueueBackTransport( action );

      theMaf.modifyState(aProcess->mafEntry(), kWaitResponse);
      enqueueTagPipeline ( action, aProcess );
      break;
    }
    case kInsertMAF_WaitProbe: {
      //Cache needs to probe hierarchy above to determine what to do
      unreserveBSO ( aProcess );          //FrontSideOut still reserved
      unreserveEvictBuffer( aProcess );

      theMaf.modifyState(aProcess->mafEntry(), kWaitProbe);
      DBG_Assert(action.theFrontMessage);
      aProcess->enqueueFrontTransport( action );
      enqueueTagPipeline ( action, aProcess );
      break;
    }
    case kReplyAndRemoveMAF: {
      //Request was satisfied. We send the response, and, if there are
      //other MAF entries, wake them.

      // We're waking a MAF entry, so unconditionally remove the MAF entry when we're done
      // A normal search will find the entry waiting for an address at this point...
      aProcess->removeMafEntry() = true;
      theMaf.modifyState ( aProcess->mafEntry(), kCompleted );

      DBG_Assert(action.theFrontMessage);
      aProcess->enqueueFrontTransport( action );

      DBG_ ( Trace, ( << " Selecting maf entry for removal (serial: "
                      << aProcess->serial() << "): "
                      << *(aProcess->mafEntry()->transport[MemoryMessageTag]) ) );

      unreserveBSO ( aProcess );            //FrontSideOut still reserved
      unreserveEvictBuffer ( aProcess );
      enqueueTagPipeline ( action, aProcess );
      break;
    }
    case kInsertMAF_WaitAddress:
      // Queuing at the MAF allows requests to be scheduled for wakeup before
      // we know that the address is busy. Free resources
      unreserveFrontSideOut( aProcess );
      unreserveBSO ( aProcess );
      unreserveEvictBuffer( aProcess ); // don't forget requests assume an EvictBuffer is required
      theMaf.modifyState(aProcess->mafEntry(), kWaitAddress);
      theMaf.dump();
      break;

    case kInsertMAF_WaitSnoop:
      // Queuing at the MAF allows requests to be scheduled for wakeup before
      // we know that the address is busy. Free resources
      unreserveFrontSideOut( aProcess );
      unreserveBSO ( aProcess );
      unreserveEvictBuffer( aProcess ); // don't forget requests assume an EvictBuffer is required
      theMaf.modifyState(aProcess->mafEntry(), kWaitSnoop);
      break;

    case kInsertMAF_WaitEvict:
      // Queuing at the MAF allows requests to be scheduled for wakeup before
      // we know that the address is busy. Free resources
      unreserveFrontSideOut( aProcess );
      unreserveBSO ( aProcess );
      unreserveEvictBuffer( aProcess ); // don't forget requests assume an EvictBuffer is required
      theMaf.modifyState(aProcess->mafEntry(), kWaitEvict);
      break;

    case kInsertMAF_WaitRegion:
      //Blocked due to oustanding Snoop.
      unreserveFrontSideOut( aProcess );
      unreserveBSO( aProcess );
      unreserveEvictBuffer( aProcess );
      theMaf.modifyState(aProcess->mafEntry(), kWaitRegion);
      break;

    case kSend:

      DBG_Assert(action.theFrontMessage == false);
      DBG_Assert(action.theBackMessage);

      unreserveEvictBuffer( aProcess );
      unreserveFrontSideOut( aProcess );

      aProcess->enqueueBackTransport(action);
      aProcess->removeMafEntry() = true;

      theMaf.modifyState ( aProcess->mafEntry(), kCompleted );

      enqueueTagPipeline ( action, aProcess );
      break;

    default:
      //Not possible for requests
      DBG_Assert( false, ( << "Unexpected result from runWakeMafProcess: " << action << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );
  }
}

void CacheController::runSnoopProcess(ProcessEntry_p aProcess ) {
  FLEXUS_PROFILE();
  DBG_(Trace, ( << " runSnoopProcess " << aProcess->serial() << ": " << *aProcess->transport()[MemoryMessageTag] ) );

  //Need to deal with I-cache probes
  MissAddressFile::maf_iter temp = theMaf.getProbingMAFEntry( addressOf(aProcess) );
  if (  temp != theMaf.end()
        && aProcess->transport()[MemoryMessageTag]->isProbeType()
     ) {

    //Mark the MAF entry as to whether the IProbe was a hit or miss,
    //and stick it on the queue to be handled
    switch (aProcess->transport()[MemoryMessageTag]->type()) {
      case MemoryMessage::ProbedNotPresent:
        theMaf.modifyState( temp, kProbeMiss );
        break;
      case MemoryMessage::ProbedClean:
      case MemoryMessage::ProbedWritable:
      case MemoryMessage::ProbedDirty:
        theMaf.modifyState( temp, kProbeHit );
        break;
      default:
        DBG_Assert(false);
    }

    unreserveBackSideOut_Snoop ( aProcess );
    theCacheControllerImpl->unreserveSnoopResources(aProcess->transport()[MemoryMessageTag], aProcess);
    theIProbeList[getBank(aProcess)].push_back( temp );

  } else {
    Action action = theCacheControllerImpl->handleSnoopMessage( aProcess->transport() );
    aProcess->consumeAction ( action );

    //The only legal actions for runSnoopProcesses are Send and NoAction
    //Note that a snoop operation will never wake up MAF entries, since it
    //can never remove an entry from the MAF.
    switch (action.theAction) {
      case kNoAction:
        //Message was handled in this cache, ie PrefetchInsert
        unreserveBackSideOut_Snoop ( aProcess );
        aProcess->type() = eProcNoMoreWork;
        aProcess->wakeAfterSnoop() = action.theWakeSnoops;
        aProcess->wakeAfterEvict() = action.theWakeEvicts;
        enqueueTagPipeline ( action, aProcess );
        break;

      case kSend: {
        //Forward on to next cache level
        //BackSideOut_Snoop still reserved

        //Even if a snoop message requires data, we can pass it on
        //to the next hierarchy level without waiting for the data access
        // JZ -> Not anymore we can't
        //aProcess->transmitAfterTag() = true;

        aProcess->enqueueBackTransport( action );
        aProcess->wakeAfterSnoop() = action.theWakeSnoops;
        aProcess->wakeAfterEvict() = action.theWakeEvicts;

        enqueueTagPipeline ( action, aProcess );
        break;
      }
      default:
        //Not allowed
        DBG_Assert( false, ( << "Unexpected result from runSnoopProcess: " << action << " for msg: " << *aProcess->transport()[MemoryMessageTag]) );
        break;
    }
  }
}

void CacheController::runIProbeProcess(ProcessEntry_p aProcess ) {
  FLEXUS_PROFILE();
  DBG_(Trace, ( << " runIProbeProcess " << aProcess->serial() << ": " << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

  Action action = theCacheControllerImpl->handleIprobe( aProcess->mafEntry()->state == kProbeHit, aProcess->mafEntry()->transport );

  aProcess->consumeAction ( action );

  switch (action.theAction) {

    case kInsertMAF_WaitResponse: {
      unreserveFrontSideOut( aProcess );            //BackSideOut_* still reserved
      theMaf.modifyState(aProcess->mafEntry(), kWaitSnoop);
      break;
    }

    case kInsertMAF_WaitSnoop: {
      // An intervening snoop takes priority, wait for it to finish then replay the fetch
      unreserveFrontSideOut( aProcess );            //BackSideOut_* still reserved
      unreserveBackSideOut_Request( aProcess );
      theMaf.modifyState(aProcess->mafEntry(), kWaitSnoop);
      break;
    }

    case kReplyAndRemoveMAF: {
      aProcess->removeMafEntry() = true;
      theMaf.modifyState(aProcess->mafEntry(), kCompleted);

      DBG_Assert(action.theFrontMessage);
      aProcess->enqueueFrontTransport( action );

      unreserveBackSideOut_Request( aProcess );

      enqueueTagPipeline ( action, aProcess );
      break;
    }
    default:
      DBG_Assert( false, ( << "Unexpected result from runIProbeProcess: " << action << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );
  }

}

void CacheController::runBackProcess(ProcessEntry_p aProcess ) {
  FLEXUS_PROFILE();
  DBG_(Trace, ( << " runBackProcess " << aProcess->serial() << ": " << *aProcess->transport()[MemoryMessageTag] ) );

  Action action = theCacheControllerImpl->handleBackMessage( aProcess->transport() );
  aProcess->consumeAction ( action );

  DBG_(Trace, ( << " Process " << aProcess->serial() << ": action = " << action.theAction << ", " << (action.theFrontMessage == false ? "NoFront" : "Front") << (action.theBackMessage == false ? " NoBack" : " Back") ));

  if (aProcess->type() == eProcBackRequest) {
    unreserveSnoopBuffer( aProcess );
  }

  switch ( action.theAction) {
    case kSend:
      // Send might be to either front or back
      if (action.theFrontMessage) {
        aProcess->enqueueFrontTransport( action );

        if (action.theBackMessage) {
          aProcess->enqueueBackTransport( action );
        } else {
          unreserveBackSideOut_Snoop( aProcess );
        }

      } else {
        DBG_Assert(action.theBackMessage, ( << " Process " << aProcess->serial() << ": action = " << action.theAction << ", " << *aProcess->transport()[MemoryMessageTag]));
        DBG_Assert(action.theFrontMessage == false);
        aProcess->enqueueBackTransport( action );
        unreserveFrontSideOut( aProcess );
      }

      aProcess->wakeRegion() = action.theWakeRegion;
      enqueueTagPipeline ( action, aProcess );
      break;

    case kReplyAndRemoveResponseMAF: {
      aProcess->removeMafEntry() = true;
      aProcess->mafEntry() = theMaf.getWaitingMAFEntryIter ( addressOf ( aProcess ) );
      DBG_Assert ( aProcess->mafEntry() != theMaf.end() );
      theMaf.modifyState ( aProcess->mafEntry(), kCompleted );

      // Initialize other transports
      aProcess->frontTransport() = aProcess->mafEntry()->transport;

      // Update transports with the correct MemoryMessage
      DBG_Assert(action.theFrontMessage);
      aProcess->enqueueFrontTransport ( action );
      // Upgrade Replies don't need Acks
      if (action.theBackMessage) {
        aProcess->enqueueBackTransport ( action );
      } else {
        unreserveBackSideOut_Snoop( aProcess );
      }

      aProcess->transport() = aProcess->mafEntry()->transport;

      aProcess->wakeRegion() = action.theWakeRegion;
      DBG_(Trace, ( << "runBackProcess(" << *aProcess->transport()[MemoryMessageTag] << "): aProcess->wakeRegion() = " << std::boolalpha << aProcess->wakeRegion() << ", action.theWakeRegion = " << action.theWakeRegion ));

      // We send to both front and back
      // so leave both reserved

      // We need to un-reserve the evict buffer that was reserved during the original request
      theCacheControllerImpl->unreserveEvictBuffer();

      enqueueTagPipeline ( action, aProcess );
    }
    break;

    case kReplyAndRetryMAF: {
      aProcess->mafEntry() = theMaf.getWaitingMAFEntryIter ( addressOf ( aProcess ) );
      DBG_Assert ( aProcess->mafEntry() != theMaf.end() );
      theMaf.modifyState(aProcess->mafEntry(), kWaitAddress);

      // Make sure we wake up the MAF and retry it
      aProcess->wakeMAF() = true;

      unreserveFrontSideOut( aProcess );
      aProcess->enqueueBackTransport ( action );

      aProcess->transport() = aProcess->mafEntry()->transport;

      aProcess->wakeRegion() = action.theWakeRegion;
      DBG_(Trace, ( << "runBackProcess(" << *aProcess->transport()[MemoryMessageTag] << "): aProcess->wakeRegion() = " << std::boolalpha << aProcess->wakeRegion() << ", action.theWakeRegion = " << action.theWakeRegion ));

      // We send to both front and back
      // so leave both reserved

      // We need to un-reserve the evict buffer that was reserved during the original request
      theCacheControllerImpl->unreserveEvictBuffer();

      enqueueTagPipeline ( action, aProcess );
    }
    break;

    case kRetryRequest:
      aProcess->mafEntry() = theMaf.getWaitingMAFEntryIter ( addressOf ( aProcess ) );
      DBG_Assert ( aProcess->mafEntry() != theMaf.end() );
      unreserveBackSideOut_Snoop( aProcess );
      unreserveFrontSideOut( aProcess );

      // We don't need any pipelines, just need to wake the MAF entry

      // Make it look like it's waiting for the address
      theMaf.modifyState(aProcess->mafEntry(), kWaitAddress);

      // We'll reserve a new evict entry when we wake the maf
      // We need to un-reserve the evict buffer that was reserved during the original request
      theCacheControllerImpl->unreserveEvictBuffer();

      // Put it on the wake list
      enqueueWakeMaf ( addressOf ( aProcess ), trackerOf ( aProcess ) );
      break;

    case kNoAction:
      // Don't send anything (use send for those cases)

      unreserveBackSideOut_Snoop( aProcess );
      unreserveFrontSideOut( aProcess );

      aProcess->wakeAfterEvict() = action.theWakeEvicts;
      aProcess->type() = eProcNoMoreWork;
      DBG_ ( Trace, ( << " no more work required for: " << *aProcess->transport()[MemoryMessageTag] ) );

      if (aProcess->wakeAfterEvict()) {
        doTransmitProcess(aProcess);
      } else {
        enqueueTagPipeline( action, aProcess );
      }
      break;

    default:
      DBG_Assert( false, ( << "Unexpected result from runBackProcess: " << action << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );

  }
}

//Get the block address of a process
MemoryAddress const CacheController::addressOf( ProcessEntry_p aProcess ) const {
  return theCacheControllerImpl->getBlockAddress( (aProcess->transport()[MemoryMessageTag] )->address() );
}

boost::intrusive_ptr<TransactionTracker> CacheController::trackerOf( ProcessEntry_p aProcess ) const {
  return aProcess->transport()[TransactionTrackerTag];
}

// Send all enqueued transports to their destinations
void CacheController::doTransmitProcess( ProcessEntry_p aProcess ) {

  DBG_ ( Trace, ( << " starting transmit for process serial: " << aProcess->serial() << " addr: " << std::hex << (uint64_t)addressOf( aProcess ) << ", wakeAfterSnoop: " << std::boolalpha << aProcess->wakeAfterSnoop() << ", removeMafEntry: " << aProcess->removeMafEntry() ));

  if ( aProcess->hasBackMessage() ) {
    Transport & trans = aProcess->backTransport();

    switch ( aProcess->type() ) {

      case eProcPrefetch:

        unreserveBackSideOut_Prefetch( aProcess );
        sendBack_Prefetch ( trans );
        break;

      case eProcSnoop:
      case eProcWakeSnoop:
      case eProcBackRequest:
      case eProcBackReply:
        //case eProcEviction:

        unreserveBackSideOut_Snoop( aProcess );
        sendBack_Snoop ( trans );
        break;

      case eProcEviction:
      case eProcRequest:
      case eProcIProbe:
      case eProcMAFWakeup:

        unreserveBackSideOut_Request( aProcess );
        sendBack_Request ( trans );
        break;

      default:
        DBG_Assert ( false,
                     ( << " For transmit to back, invalid process type: "
                       << aProcess->type() ) );
    }
  }

  if ( aProcess->hasFrontMessage() ) {
    unreserveFrontSideOut( aProcess );
    DBG_Assert( aProcess->frontTransport()[MemoryMessageTag] != nullptr, ( << "Process serial: " << aProcess->serial() << " addr: " << std::hex << (uint64_t)addressOf( aProcess ) << " missing Front Transport MemoryMessage." ));
    sendFront ( aProcess->frontTransport(), aProcess->sendToD(), aProcess->sendToI() );
  }

  if (aProcess->type() == eProcIdleWork) {
    theCacheControllerImpl->completeIdleWork(aProcess->transport()[MemoryMessageTag]);
  }

  if ((aProcess->type() == eProcRequest || aProcess->type() == eProcMAFWakeup) && aProcess->hasReserved(kResEvictBuffer)) {
    // We might have an evict buffer reserved waiting for the reply that will eventually come back
    // Clear the bit but don't give up the reservation
    clearEvictBufferReservation(aProcess);
  }

  DBG_Assert(aProcess->getReservations() == 0, ( << "Process has no more work but still holds reservations! " << (*aProcess) ));

  aProcess->type() = eProcNoMoreWork;

  // Finally, deallocate and unlock the MAF, if necessary and
  // wakeup any processes waiting on the address

  DBG_(Trace, ( << " process serial: " << aProcess->serial() << " wakeRegion " << std::boolalpha << aProcess->wakeRegion() << " addr: " << std::hex << (uint64_t)addressOf( aProcess ) ));

  if ( aProcess->wakeRegion() ) {
    // wake-up all MAF's for blocks in the same region as this maf in the wake region state
    enqueueWakeRegionMaf( addressOf( aProcess), trackerOf( aProcess ) );
  }

  if ( aProcess->removeMafEntry() ) {
    aProcess->removeMafEntry() = false;

    // add to deal with the fact that some blocks are never unset from unprotected
    // this lead to the bug that it wasn't possible to allocate a block.

    //theCacheControllerImpl->setProtectedPublic(addressOf(aProcess),false);


    DBG_Assert ( aProcess->mafEntry() != theMaf.end() );
    DBG_Assert ( aProcess->mafEntry()->state == kCompleted );
    DBG_ ( Trace, ( << " removing MAF entry: " << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

    if ( theMaf.contains ( addressOf ( aProcess ), kWaitAddress ) ) {
      enqueueWakeMaf ( addressOf ( aProcess ), trackerOf ( aProcess ) );
    } else {
      DBG_(Trace, ( << " process serial: " << aProcess->serial() << " addr: " << std::hex << (uint64_t)addressOf(aProcess) << ", NO MAF entries waiting on Address." ));
    }

    theMaf.remove ( aProcess->mafEntry() );

  } else if ( aProcess->wakeMAF() ) {
    if ( theMaf.contains ( addressOf ( aProcess ), kWaitAddress ) ) {
      enqueueWakeMaf ( addressOf ( aProcess ), trackerOf ( aProcess ) );
    }
  } else if ( aProcess->wakeAfterSnoop() ) {

    theCacheControllerImpl->wakeWaitingSnoops( addressOf( aProcess ) );

    if ( theMaf.contains ( addressOf ( aProcess ), kWaitSnoop ) ) {
      enqueueWakeMaf ( addressOf ( aProcess ), trackerOf ( aProcess ) );
    } else if ( aProcess->wakeAfterEvict() ) {

      if ( theMaf.contains ( addressOf ( aProcess ), kWaitEvict ) ) {
        enqueueWakeMaf ( addressOf ( aProcess ), trackerOf ( aProcess ) );
      }
    } else {
      DBG_(Trace, ( << " process serial: " << aProcess->serial() << " addr: " << std::hex << (uint64_t)addressOf(aProcess) << ", NO MAF entries waiting on Snoop." ));
    }
  } else if ( aProcess->wakeAfterEvict() ) {

    if ( theMaf.contains ( addressOf ( aProcess ), kWaitEvict ) ) {
      enqueueWakeMaf ( addressOf ( aProcess ), trackerOf ( aProcess ) );
    }
  }
}

// Message -> Front
void CacheController::sendFront(MemoryTransport  & transport, bool to_D, bool to_I) {
  DBG_(Trace, ( << " sendFront (D-" << to_D << ", I-" << to_I << ") : " << *transport[MemoryMessageTag] ) );

  if (transport[TransactionTrackerTag]) {
    transport[TransactionTrackerTag]->setDelayCause(theName, "Front Tx");
  }

  DBG_Assert( transport[MemoryMessageTag] != nullptr );

  DBG_Assert ( transport[MemoryMessageTag]->coreIdx() >= 0 &&
               transport[MemoryMessageTag]->coreIdx() < theCores );

  Transport i_transport(transport);
  i_transport.set(MemoryMessageTag, MemoryMessage_p(new MemoryMessage(*transport[MemoryMessageTag])));
  i_transport[MemoryMessageTag]->dstream() = false;

  if (to_D) {
    FrontSideOut_D[transport[MemoryMessageTag]->coreIdx()].enqueue(transport);
  }
  if (to_I) {
    FrontSideOut_I[transport[MemoryMessageTag]->coreIdx()].enqueue(i_transport);
  }
}

// Message -> Back(Request)
void CacheController::sendBack_Request(MemoryTransport & transport) {
  DBG_(Trace, ( << " sendBack_Request " << *transport[MemoryMessageTag] ) );
  if (transport[TransactionTrackerTag]) {
    transport[TransactionTrackerTag]->setDelayCause(theName, "Back Tx");
  }
  BackSideOut_Request.enqueue(transport);
}

// Message -> Back(Prefetch)
void CacheController::sendBack_Prefetch(MemoryTransport & transport) {
  DBG_(Trace, ( << " sendBack_Prefetch" << *transport[MemoryMessageTag] ) );
  if (transport[TransactionTrackerTag]) {
    transport[TransactionTrackerTag]->setDelayCause(theName, "Back Tx");
  }
  BackSideOut_Prefetch.enqueue(transport);
}

// Message -> Back(Snoop)
void CacheController::sendBack_Snoop(MemoryTransport & transport) {
  DBG_(Trace, ( << " sendBack_Snoop " << *transport[MemoryMessageTag] ) );
  if (transport[TransactionTrackerTag]) {
    transport[TransactionTrackerTag]->setDelayCause(theName, "Back Tx");
  }
  if (theUseReplyChannel) {
    DBG_(Trace, ( << " Using REPLY channel sendBack_Snoop" << *transport[MemoryMessageTag] ) );
    BackSideOut_Reply.enqueue(transport);
  } else {
    BackSideOut_Snoop.enqueue(transport);
  }
}

// Message -> Back(Snoop)
void CacheController::sendBack_Evict(MemoryTransport & transport) {
  DBG_(Trace, ( << " sendBack_Evict" << *transport[MemoryMessageTag] ) );
  if (transport[TransactionTrackerTag]) {
    transport[TransactionTrackerTag]->setDelayCause(theName, "Back Tx");
  }
  if (theEvictOnSnoop) {
    BackSideOut_Snoop.enqueue(transport);
  } else {
    BackSideOut_Request.enqueue(transport);
  }
}

} // namespace nCache
