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

#ifndef FLEXUS_CACHE_CONTROLLER_HPP_INCLUDED
#define FLEXUS_CACHE_CONTROLLER_HPP_INCLUDED

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;
#include <boost/none.hpp>

#include <components/CommonQEMU/MessageQueues.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#include <list>

#define DBG_DeclareCategories Cache
#define DBG_SetDefaultOps AddCat(Cache)
#include DBG_Control()

namespace nCache {

using namespace nMessageQueues;
typedef Flexus::SharedTypes::MemoryTransport Transport;
typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

// the states that a MAF entry may be in
enum MafStates {
  kWaitResponse
  //This indicates that a request of some sort has been sent on behalf
  //of this MAF entry, and a response will eventually arrive, fulfilling
  //this miss.  There can be at most one MAF entry in kWaitResponse per
  //block address
  , kWaitAddress
  //This indicates that a MAF entry has not yet been examined because its
  //address conflicts with an entry in kWaitResponse state.  There should
  //always be one entry in kWaitResponse if there are any kWaitAddress,
  //except when the cache is in the process of waking kWaitAddress entries.
  , kWaking
  //This MAF entry is in the process of waking up
  , kWaitProbe
  //This indicates that the request in this MAF entry is waiting for a probe
  // to return.  Currently, this is only used for Ifetch misses.
  , kProbeHit
  //This indicates that the request in this MAF entry is waiting for a probe
  // to return.  Currently, this is only used for Ifetch misses.
  , kProbeMiss
  //This indicates that the request in this MAF entry is waiting for a probe
  // to return.  Currently, this is only used for Ifetch misses.
  , kCompleted
  //This indicates that the request has completed and created all of its
  // response messages, however the messages have not been sent yet and
  // further transactions on the block are not cleared to proceed.
  , kWaitSnoop
  // This indicates that a MAF entry is stalled waiting for a snoop to complete
  // snoops implicitly have higher priority than new requests
  , kWaitEvict
  // This indicates that a MAF entry is stalled waiting for a 2-phase evict to complete
  , kWaitRegion
  // This indicates that a MAF entry is stalled waiting for some action
  // that affects multiple blocks
};

inline std::string mafState2String(const MafStates & state) {
  switch (state) {
    case kWaitResponse:
      return "kWaitResponse";
    case kWaitAddress:
      return "kWaitAddress";
    case kWaking:
      return "kWaking";
    case kWaitProbe:
      return "kWaitProbe";
    case kProbeHit:
      return "kProbeHit";
    case kProbeMiss:
      return "kProbeMiss";
    case kCompleted:
      return "kCompleted";
    case kWaitSnoop:
      return "kWaitSnoop";
    case kWaitEvict:
      return "kWaitEvict";
    case kWaitRegion:
      return "kWaitRegion";
    default:
      return "UNKNOWN MAF STATE!!!!";
  }
}

enum eEntryType {
  kRead
  , kWrite
  , kAtomic
  , kFetch
  , kReadPrefetch
  , kWritePrefetch

  , kTotal
  , kLastType
};

struct MafEntry {
  MemoryAddress theBlockAddress;  //The block address of the entry, for index by_addr
  MafStates state;                 //State of the MAF entry
  mutable Transport transport;    //the transport object associated with this miss
  mutable eEntryType type;
  mutable int32_t outstanding_msgs;
  mutable bool data_received;

  MafEntry(MemoryAddress aBlockAddress, Transport & aTransport, MafStates aState, eEntryType aType)
    : theBlockAddress(aBlockAddress)
    , state(aState)
    , transport(aTransport)
    , type(aType)
    , outstanding_msgs(0)
    , data_received(false)
  {}
};

class MissAddressFile {
  //MAF entries are indexed by their block address and state
  struct by_state {};

  typedef multi_index_container
  < MafEntry
  , indexed_by
  < ordered_non_unique
  < composite_key
  < MafEntry
  , member< MafEntry, MemoryAddress, &MafEntry::theBlockAddress >
  , member< MafEntry, MafStates, &MafEntry::state>
  >
  >
  >
  >
  maf_t;
  maf_t theMshrs;

  uint32_t theSize;
  uint32_t theMaxTargetsPerRequest;
  uint32_t theWaitResponseEntries;

  uint32_t theEntries[kLastType];
  Stat::StatInstanceCounter<int64_t> * theCyclesWith[kLastType];
  Stat::StatAverage * theAverage[kLastType];
  uint64_t theLastAccounting;
  Stat::StatMax theMaxMAFTargets;
  Stat::StatMax theMaxMAFMisses;
  std::string theName;
  int32_t theReserve;

public:
  typedef maf_t::iterator maf_iter;

  MissAddressFile(std::string const & aStatName, uint32_t aSize, uint32_t aMaxTargetsPerRequest)
    : theSize(aSize)
    , theMaxTargetsPerRequest(aMaxTargetsPerRequest)
    , theWaitResponseEntries(0)
    , theLastAccounting(0)
    , theMaxMAFTargets( aStatName + "-MaxMAFTargets")
    , theMaxMAFMisses( aStatName + "-MaxMAFMisses")
    , theName(aStatName)
    , theReserve(0) {
    DBG_Assert(theMaxTargetsPerRequest == 0); //Not supported

    for (int32_t i = 0; i < kLastType; ++i) {
      theEntries[i] = 0;
    }

    theCyclesWith[kRead] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Read");
    theAverage[kRead] = new Stat::StatAverage(aStatName + "-MAFAvg:Read");
    theCyclesWith[kWrite] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Write");
    theAverage[kWrite] = new Stat::StatAverage(aStatName + "-MAFAvg:Write");
    theCyclesWith[kAtomic] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Atomic");
    theAverage[kAtomic] = new Stat::StatAverage(aStatName + "-MAFAvg:Atomic");
    theCyclesWith[kFetch] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Fetch");
    theAverage[kFetch] = new Stat::StatAverage(aStatName + "-MAFAvg:Fetch");
    theCyclesWith[kReadPrefetch] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:ReadPrefetch");
    theAverage[kReadPrefetch] = new Stat::StatAverage(aStatName + "-MAFAvg:ReadPrefetch");
    theCyclesWith[kWritePrefetch] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:WritePrefetch");
    theAverage[kWritePrefetch] = new Stat::StatAverage(aStatName + "-MAFAvg:WritePrefetch");
    theCyclesWith[kTotal] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Total");
    theAverage[kTotal] = new Stat::StatAverage(aStatName + "-MAFAvg:Total");
  }

  eEntryType getType(MemoryMessage::MemoryMessageType aMessageType) {
    switch (aMessageType) {
      case MemoryMessage::AtomicPreloadReq:  //twenisch - is this correct?
      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::StreamFetch:
        return kRead;
      case MemoryMessage::StoreReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::NonAllocatingStoreReq:
        return kWrite;
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        return kAtomic;
      case MemoryMessage::FetchReq:
        return kFetch;
      case MemoryMessage::PrefetchReadNoAllocReq:
      case MemoryMessage::PrefetchReadAllocReq:
        return kReadPrefetch;
      case MemoryMessage::StorePrefetchReq:
        return kWritePrefetch;
      default:
        DBG_Assert(false, ( << "Illegal message type entered into MAF: " << aMessageType) );
        return kRead;
    }
  }

  void account(eEntryType anEntry, int32_t aDelta) {
    //accumulation_type::Accumulate counts since the last accounting
    int64_t time = Flexus::Core::theFlexus->cycleCount() - theLastAccounting;
    if (time > 0) {
      for (int32_t i = 0; i < kLastType; ++i) {
        *(theCyclesWith[i]) << std::make_pair( static_cast<int64_t>(theEntries[i]), time );
        *(theAverage[i]) << std::make_pair( static_cast<int64_t>(theEntries[i]), time );
      }
    }

    if (aDelta != 0) {
      // Modify counts
      theEntries[anEntry] += aDelta;
      theEntries[kTotal] += aDelta;
    }

    //Assert that counts add up to MSHR contends
    int32_t sum = 0;
    for (int32_t i = 0; i < kTotal; ++i) {
      sum += theEntries[i];
    }
    DBG_Assert( sum == static_cast<int>(theEntries[kTotal]));
    DBG_Assert( sum == static_cast<int>(theMshrs.size()));
  }

  void allocEntry(MemoryAddress aBlockAddress, Transport & aTransport, MafStates aState) {
    DBG_(Trace, ( << theName << " - Adding MAF entry for block " << std::hex << aBlockAddress << " in state " << mafState2String(aState) ));
    DBG_Assert(! full() );
    eEntryType type = getType(aTransport[MemoryMessageTag]->type());
    theMshrs.insert( MafEntry( aBlockAddress, aTransport, aState, type ) );
    if (aState == kWaitResponse) {
      ++theWaitResponseEntries;
      theMaxMAFMisses << theWaitResponseEntries;
    }
    theMaxMAFTargets << theMshrs.size();
    account(type, 1);
  }

  maf_iter allocPlaceholderEntry ( MemoryAddress   aBlockAddress,
                                   Transport   &   aTransport ) {
    DBG_Assert ( ! full() );
    theMshrs.insert ( MafEntry ( aBlockAddress, aTransport, kWaitResponse, kRead ) );
    ++theWaitResponseEntries;
    theMaxMAFMisses << theWaitResponseEntries;
    theMaxMAFTargets << theMshrs.size();
    account(kRead, 1);

    maf_iter iter  = theMshrs.find( std::make_tuple( aBlockAddress, kWaitResponse ) );
    modifyState ( iter, kCompleted );

    return iter;
  }

  bool empty() const {
    return theMshrs.empty();
  }

  void reserve() {
    ++theReserve;
    DBG_Assert( theReserve + theMshrs.size() <= theSize);
  }

  void unreserve() {
    --theReserve;
    DBG_Assert( theReserve >= 0);
  }

  bool full() const {
    if (theMaxTargetsPerRequest == 0) {
      return theMshrs.size() + theReserve >= theSize;
    } else {
      return theWaitResponseEntries >= theSize || theMshrs.size() > theSize * theMaxTargetsPerRequest;
    }
  }

  maf_iter end() const {
    return theMshrs.end();
  }

  void remove(maf_iter iter) {
    eEntryType type = iter->type;
    if (iter->state == kWaitResponse) {
      --theWaitResponseEntries;
    }
    theMshrs.erase(iter);
    account(type, -1);
  }

  void modifyState( maf_iter iter, MafStates aState) {
    if (iter->state == kWaitResponse) {
      --theWaitResponseEntries;
    }
    //theMshrs.modify(iter, [aState](auto& x){ return x.state = aState; });
    theMshrs.modify(iter, ll::bind( &MafEntry::state, ll::_1 ) = aState);
    if (aState == kWaitResponse) {
      ++theWaitResponseEntries;
      theMaxMAFMisses << theWaitResponseEntries;
    }
  }

  bool contains ( const MemoryAddress  & aBlockAddress ) const {
    DBG_(Trace, ( << "Searching MAF for block " << std::hex << aBlockAddress ));
    return theMshrs.count( std::make_tuple( aBlockAddress ) ) > 0;
  }

  bool contains ( const MemoryAddress  & aBlockAddress , MafStates aState) const {
    return theMshrs.count( std::make_tuple( aBlockAddress, aState ) ) > 0;
  }

  std::pair
  < boost::intrusive_ptr<MemoryMessage>
  , boost::intrusive_ptr<TransactionTracker>
  >
  getWaitingMAFEntry( const MemoryAddress  & aBlockAddress) {
    maf_t::iterator iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitResponse ) );
    if ( iter == theMshrs.end() ) {
      DBG_( Iface, ( << "Expected to find MAF entry for " << aBlockAddress << " but found none.") );
      return std::make_pair( boost::intrusive_ptr<MemoryMessage>(0), boost::intrusive_ptr<TransactionTracker>(0) );
    }

    return std::make_pair( iter->transport[MemoryMessageTag], iter->transport[TransactionTrackerTag]);
  }

  maf_iter getWaitingMAFEntryIter ( const MemoryAddress & aBlockAddress ) {
    return theMshrs.find( std::make_tuple( aBlockAddress, kWaitResponse ) );
  }

  maf_iter getProbingMAFEntry( const MemoryAddress & aBlockAddress) {
    return theMshrs.find( std::make_tuple( aBlockAddress, kWaitProbe ) );
  }

  Transport getWaitingMAFEntryTransport ( const MemoryAddress & aBlockAddress ) {
    maf_t::iterator iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitResponse ) );
    DBG_Assert( iter != theMshrs.end() );
    return iter->transport;
  }

  Transport removeWaitingMafEntry( const MemoryAddress  & aBlockAddress ) {
    Transport ret_val;
    maf_t::iterator iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitResponse ) );
    DBG_Assert( iter != theMshrs.end() );
    --theWaitResponseEntries;
    ret_val = iter->transport;
    eEntryType type = iter->type;
    theMshrs.erase( iter );
    account(type, -1);
    return ret_val;
  }

  maf_t::iterator getBlockedMafEntry( const MemoryAddress  & aBlockAddress ) {
    // Need to prioritize waiting MAFs WaitRegion -> WaitSnoop -> Wait Address

    maf_t::iterator iter;
    iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitRegion ) );
    if (iter == theMshrs.end() ) {
      iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitSnoop ) );
      if (iter != theMshrs.end() ) {
        modifyState(iter, kWaking);
      } else {
        iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitEvict ) );
        if (iter != theMshrs.end() ) {
          modifyState(iter, kWaking);
        } else {
          iter = theMshrs.find( std::make_tuple( aBlockAddress, kWaitAddress ) );
        }
      }
    }
    return iter;
  }

  std::list<boost::intrusive_ptr<MemoryMessage> > getAllMessages( const MemoryAddress  & aBlockAddress) {
    std::list< boost::intrusive_ptr<MemoryMessage> > ret_val;
    maf_t::iterator iter, end;
    std::tie(iter, end) = theMshrs.equal_range( std::make_tuple( aBlockAddress ) );
    while (iter != end) {
      ret_val.push_back( iter->transport[MemoryMessageTag] );
      ++iter;
    }
    return ret_val;
  }

  std::list<boost::intrusive_ptr<MemoryMessage> > getAllUncompletedMessages( const MemoryAddress  & aBlockAddress) {
    std::list< boost::intrusive_ptr<MemoryMessage> > ret_val;
    maf_t::iterator iter, end;
    std::tie(iter, end) = theMshrs.equal_range( std::make_tuple( aBlockAddress ) );
    while (iter != end) {
      if ( iter->state != kCompleted ) {
        ret_val.push_back( iter->transport[MemoryMessageTag] );
      }
      ++iter;
    }
    return ret_val;
  }

  void dump ( void ) {
    maf_t::iterator iter = theMshrs.begin();

    DBG_ ( Trace, Set( (CompName) << theName ) ( << theName << " MAF content dump (" << theMshrs.size() << "/" << theSize ) );

    while ( iter != theMshrs.end() ) {
      DBG_ ( Trace, Set( (CompName) << theName ) ( << "entry[" << iter->state << ":" << iter->type << "]: "
             <<  *iter->transport[MemoryMessageTag] ) );
      iter++;
    }

    DBG_ ( Trace, Set( (CompName) << theName ) ( << theName << " MAF content finished" ) );
  }

};  // end class MissAddressFile

enum ProcessType {
  eProcRequest = 0
  , eProcPrefetch = 1
  , eProcSnoop = 2
  , eProcBackRequest = 3
  , eProcBackReply = 4
  , eProcMAFWakeup = 5
  , eProcIProbe = 6
  , eProcEviction = 7
  , eProcNoMoreWork = 8
  , eProcIdleWork = 9
  , eProcWakeSnoop = 10
};

// Resources needed by a ProcessEntry.  These can be combined a bitmask
// for group reservation/unreservation.
enum ProcessResourceType {
  kResFrontSideOut         = 0x01,
  kResBackSideOut_Request  = 0x02,
  kResBackSideOut_Snoop    = 0x04,
  kResBackSideOut_Prefetch = 0x08,
  kResEvictBuffer          = 0x10,
  kResScheduledEvict       = 0x20,
  kResMaf                  = 0x40,
  kResSnoopBuffer          = 0x80,
  kResBackSideOut_Reply    = 0x100,
};

static int32_t theProcessSerial = 0;

class ProcessEntry;
class CacheController;
}; // namespace

#include "BaseCacheControllerImpl.hpp"

namespace nCache {

// the process queue contains entries of this type
class ProcessEntry : public boost::counted_base {
  Transport theOrig;              // The transport that caused this process to start
  //std::list<Transport> theOutputTransports; // Transports that are generated by this process (may include a morphed starting transport)

  // A process can send only one message in each direction
  Transport theFrontTransport;
  Transport theBackTransport;
  MissAddressFile::maf_iter theMafEntry;
  ProcessType theType;
  boost::intrusive_ptr<TransactionTracker> theWakeTransaction;

  int32_t  theRequiresData;            // Number of times to access the data array
  bool theTransmitAfterTag;        // May send messages after the tag check (before data array)
  bool theTagOutstanding;          // Must access the local tags
  int32_t  theReservations;            // Reservations held by this process (for sanity check)
  bool theRemoveMafEntry;          // Should the MAF entry be removed?
  int32_t  theSerial;

  bool theSendToD;
  bool theSendToI;
  bool theSendToBack;
  bool theWakeSnoops;
  bool theWakeEvicts;
  bool theWakeRegion;
  bool theWakeMaf;

public:
  ProcessEntry(ProcessType aType)
    : theType(aType)
    , theRequiresData(0)
    , theTransmitAfterTag(false)
    , theTagOutstanding(false)
    , theReservations ( 0 )
    , theRemoveMafEntry ( false )
    , theSerial ( theProcessSerial++ )
    , theSendToD(false)
    , theSendToI(false)
    , theSendToBack(false)
    , theWakeSnoops(false)
    , theWakeEvicts(false)
    , theWakeRegion(false)
    , theWakeMaf(false)
  {}

  ProcessEntry(const Transport & request, ProcessType aType)
    : theOrig(request)
    , theFrontTransport(request)
    , theBackTransport(request)
    , theType(aType)
    , theRequiresData(0)
    , theTransmitAfterTag(false)
    , theTagOutstanding(false)
    , theReservations(0)
    , theRemoveMafEntry ( false )
    , theSerial ( theProcessSerial++ )
    , theSendToD(false)
    , theSendToI(false)
    , theSendToBack(false)
    , theWakeSnoops(false)
    , theWakeEvicts(false)
    , theWakeRegion(false)
    , theWakeMaf(false)
  {}

  ProcessEntry(MissAddressFile::maf_iter iter, boost::intrusive_ptr<TransactionTracker> wake, ProcessType aType)
    : theOrig(iter->transport)
    , theFrontTransport(iter->transport)
    , theBackTransport(iter->transport)
    , theMafEntry(iter)
    , theType(aType)
    , theWakeTransaction(wake)
    , theRequiresData(0)
    , theTransmitAfterTag(false)
    , theTagOutstanding(false)
    , theReservations(0)
    , theRemoveMafEntry ( false )
    , theSerial ( theProcessSerial++ )
    , theSendToD(false)
    , theSendToI(false)
    , theSendToBack(false)
    , theWakeSnoops(false)
    , theWakeEvicts(false)
    , theWakeRegion(false)
    , theWakeMaf(false)
  {}

  ~ProcessEntry() {
    // Check whether we leaked any resources or not
    // Unfortunately, debugging macros don't work in this scope (for now)
    if ( theReservations != 0 ) {
      DBG_(Crit, ( << "Process ending with resources reserved: " << std::hex << theReservations << ": " << *theOrig[MemoryMessageTag]  ));
    }
  }

  Transport & transport() {
    return theOrig;
  }

  void consumeTransport ( Transport & trans ) {
    // For now, just make sure we keep the DestinationTag's
    // If other Tag's become important in the future, then this should be updated
    theOrig.set(DestinationTag, trans[DestinationTag]);
    theFrontTransport.set(DestinationTag, trans[DestinationTag]);
    theBackTransport.set(DestinationTag, trans[DestinationTag]);
  }

  Transport & frontTransport() {
    return theFrontTransport;
  }

  Transport & backTransport() {
    return theBackTransport;
  }

  bool & wakeAfterSnoop() {
    return theWakeSnoops;
  }
  bool & wakeAfterEvict() {
    return theWakeEvicts;
  }
  bool & wakeRegion() {
    return theWakeRegion;
  }
  bool & wakeMAF() {
    return theWakeMaf;
  }

  void reserve ( const int32_t res ) {
    if ( ( theReservations & res ) != 0 ) {
      DBG_(Crit, ( << "Process serial: " << serial() << " WARNING: cache resource leak of type: 0x" << std::hex << (theReservations & res) ));
    }
    theReservations = theReservations | res;
  }

  void unreserve ( const int32_t res ) {
    if ( (theReservations & res) != res ) {
      DBG_(Crit, ( << "Process serial: " << serial() << " MISSING RESOURCE ON UNRESERVE: " << std::hex << res << " have: " << theReservations << " - > " << *theOrig[MemoryMessageTag]) );
    }
    theReservations = theReservations & (~res);
  }

  bool hasReserved ( const int32_t res ) {
    return ( (theReservations & res) == res );
  }

  int32_t getReservations ( void ) {
    return theReservations;
  }

  void enqueueFrontTransport( Action & action ) {
    //theFrontTransport.set(MemoryMessageTag, action.theFrontMessage);
    //theFrontTransport.set(TransactionTrackerTag, action.theOutputTracker);
    theFrontTransport = action.theFrontTransport;
    theSendToD = action.theFrontToDCache;
    theSendToI = action.theFrontToICache;
  }

  void enqueueBackTransport( Action & action ) {
    //theBackTransport.set(MemoryMessageTag, action.theBackMessage);
    //theBackTransport.set(TransactionTrackerTag, action.theOutputTracker);
    theBackTransport = action.theBackTransport;
    theSendToBack = true;
  }

  bool hasBackMessage() {
    return theSendToBack;
  }
  bool hasFrontMessage() {
    return (theSendToD | theSendToI);
  }
  bool sendToD() {
    return theSendToD;
  }
  bool sendToI() {
    return theSendToI;
  }

  const ProcessType & const_type() const {
    return theType;
  }
  ProcessType & type() {
    return theType;
  }
  boost::intrusive_ptr<TransactionTracker> & wakeTrans() {
    return theWakeTransaction;
  }
  bool requiresTag() {
    return (theTagOutstanding != 0);
  }
  int32_t & requiresData() {
    return theRequiresData;
  }
  bool & transmitAfterTag() {
    return theTransmitAfterTag;
  }
  MissAddressFile::maf_iter & mafEntry() {
    return theMafEntry;
  }
  bool & removeMafEntry() {
    return theRemoveMafEntry;
  }
  bool & tagOutstanding() {
    return theTagOutstanding;
  }

  void consumeAction ( Action & action ) {
    theRequiresData = action.theRequiresData;
    theTagOutstanding = action.theRequiresTag;
  }

  int32_t serial ( void ) {
    return theSerial;
  }

};  // end struct ProcessEntry

std::ostream & operator << ( std::ostream & s, const enum ProcessType eProcess );
std::ostream & operator << ( std::ostream & s, ProcessEntry   &  process );

struct SnoopTransportEntry {
  int32_t serial;
  mutable Transport transport;

  SnoopTransportEntry(intrusive_ptr<ProcessEntry> process)
    : serial(process->transport()[MemoryMessageTag]->serial())
    , transport(process->transport())
  { }
};

typedef multi_index_container
< SnoopTransportEntry
, indexed_by
< hashed_unique
< member<SnoopTransportEntry, int, &SnoopTransportEntry::serial > >
>
> snoop_transport_hash_t;

//Since the MessageQueues and the MissAddressFile contain
//transports, they must live in CacheController, which is the only thing
//that knows what the MemoryTransport contains.

class CacheController {
  std::string theName;

  CacheInitInfo theCacheInitInfo;
  std::unique_ptr<BaseCacheControllerImpl> theCacheControllerImpl;

  int32_t theCores;
  int32_t theBanks;
  uint32_t thePorts;
  int32_t theNodeId;

  int32_t theBlockSize;

  bool theEvictOnSnoop;
  bool theUseReplyChannel;

  // types and declarations for the miss address file (MAF)
  typedef boost::intrusive_ptr<MafEntry> MafEntry_p;
  MissAddressFile theMaf;

  //snoop_transport_hash_t theSnoopTransports;

  // types and declarations for the process queue
  typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;
  typedef PipelineFifo<ProcessEntry_p> Pipeline;
  std::vector<Pipeline> theMAFPipeline;
  std::vector<Pipeline> theTagPipeline;
  std::vector<Pipeline> theDataPipeline;

  int32_t theScheduledEvicts;
  int32_t theFrontSideOutReserve;

  Stat::StatInstanceCounter<int64_t> theMafUtilization;
  Stat::StatInstanceCounter<int64_t> theTagUtilization;
  Stat::StatInstanceCounter<int64_t> theDataUtilization;

  std::vector< std::list< std::pair < MemoryAddress, boost::intrusive_ptr<TransactionTracker> > > > theWakeMAFList;
  std::vector< std::list< MissAddressFile::maf_iter > > theIProbeList;

public:
  //These are directly manipulated by CacheImpl

  // The FrontSideIn_* queues are small queues that take messages
  // and then arbitrate across queus for each bank controller
  std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Snoop;
  std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Request;
  std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Prefetch;

  std::vector< MessageQueue<MemoryTransport> > BackSideIn_Request;
  std::vector< MessageQueue<MemoryTransport> > BackSideIn_Reply;
  std::vector< MessageQueue<MemoryTransport> > FrontSideOut_I;
  std::vector< MessageQueue<MemoryTransport> > FrontSideOut_D;
  MessageQueue<MemoryTransport> BackSideOut_Reply;
  MessageQueue<MemoryTransport> BackSideOut_Snoop;
  MessageQueue<MemoryTransport> BackSideOut_Request;
  MessageQueue<MemoryTransport> BackSideOut_Prefetch;

  std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Snoop;
  std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Request;
  std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Prefetch;
  std::vector< MessageQueue<MemoryTransport> > BankBackSideIn_Request;
  std::vector< MessageQueue<MemoryTransport> > BankBackSideIn_Reply;

  // Round-robin counters to keep fairness across the
  // n input queues
  int32_t lastSnoopQueue;
  int32_t lastRequestQueue;
  int32_t lastPrefetchQueue;

  int32_t theLastTagPipeline, theLastDataPipeline, theLastScheduledBank;

  int64_t theTraceAddress;
  uint64_t theTraceTimeout;

  // Now that there are an array of FrontSideOut ports
  // we need to look at each one to determine whether
  // the empty()/full() conditions are met globally.
  const bool isFrontSideOutEmpty_D ( void ) const;
  const bool isFrontSideOutEmpty_I ( void ) const;

  const bool isFrontSideOutFull ( void ) const;

  const bool isQueueSetEmpty ( const std::vector< MessageQueue<MemoryTransport> > & queues ) const;

  const bool isPipelineSetEmpty ( const std::vector<Pipeline> & pipelines ) const;

  const bool isQueueSetFull ( const MessageQueue<MemoryTransport> * const & queues ) const;

  const bool isWakeMAFListEmpty() const;

  const bool isIProbeListEmpty() const;

  const uint32_t getBank ( const ProcessEntry_p entry ) const;
  const uint32_t getBank ( const MemoryTransport trans ) const;
  const uint32_t getBank ( const boost::intrusive_ptr<MemoryMessage> msg ) const;
  const uint32_t getBank ( const MemoryAddress address ) const;

  uint32_t totalPipelineSize ( std::vector<Pipeline> & pipe ) const;

  bool isQuiesced() const;

  void saveState(std::string const & aDirName);

  void loadState(std::string const & aDirName, bool aTextFlexpoint, bool aGZippedFlexpoint);

  CacheController
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
    , bool aUseReplyChannel
  );

  //These methods are used by the Impl to manipulate the MAF

  // Warning: this may return completed MAF entries with reply messages.  You may want to
  // use the function below which excludes completed messages.
  std::list<boost::intrusive_ptr<MemoryMessage> > getAllMessages( const MemoryAddress  & aBlockAddress);

  std::list<boost::intrusive_ptr<MemoryMessage> > getAllUncompletedMessages( const MemoryAddress  & aBlockAddress);

  std::pair<MemoryMessage_p, TransactionTracker_p> getWaitingMAFEntry( const MemoryAddress  & aBlockAddress);
  MissAddressFile::maf_iter getWaitingMAFEntryIter( const MemoryAddress  & aBlockAddress);

  bool hasMAF(const MemoryAddress & ablockAddress) const;

  void reserveFrontSideOut(ProcessEntry_p aProcess);

  void reserveBackSideOut_Request(ProcessEntry_p aProcess);

  void reserveBackSideOut_Snoop(ProcessEntry_p aProcess);
  void reserveBackSideOut_Evict(ProcessEntry_p aProcess);

  void reserveBackSideOut_Prefetch(ProcessEntry_p aProcess);

  void reserveEvictBuffer(ProcessEntry_p aProcess);

  void reserveSnoopBuffer(ProcessEntry_p aProcess);

  void reserveScheduledEvict(ProcessEntry_p aProcess);

  void reserveMAF(ProcessEntry_p aProcess);

  // Unreserves queue and structured entries for a process
  // in a centralized bitmask "stew"
  void unreserveFrontSideOut ( ProcessEntry_p aProcess );

  void unreserveBackSideOut_Request ( ProcessEntry_p aProcess );
  void unreserveBackSideOut_Snoop ( ProcessEntry_p aProcess );
  void unreserveBackSideOut_Evict ( ProcessEntry_p aProcess );
  void unreserveBackSideOut_Prefetch ( ProcessEntry_p aProcess );
  void unreserveEvictBuffer ( ProcessEntry_p aProcess );
  void clearEvictBufferReservation ( ProcessEntry_p aProcess );
  void unreserveSnoopBuffer ( ProcessEntry_p aProcess );
  void unreserveScheduledEvict ( ProcessEntry_p aProcess );
  void unreserveMAF ( ProcessEntry_p aProcess );
  void unreserveBSO ( ProcessEntry_p aProcess );

  //Process all pending stuff in the cache.  This is called once each cycle.
  //It iterates over all the message and process queues, moving things along
  //if they are ready to go and there is no back pressure.  The order of
  //operations in this function is important - it is designed to make sure
  //a request can be received and processed in a single cycle for L1 cache
  //hits
  void processMessages();

private:

  // Enqueue requests in the appropriate tag pipelines
  void enqueueTagPipeline ( Action         action,
                            ProcessEntry_p aProcess );

  // Schedule the Tag and Data pipelines for a non-CMP cache
  void advancePipelines ( void );

  //Insert new processes into maf pipeline.  We can return once we are out
  //of ready MAF servers
  void scheduleNewProcesses();

  //For convenience.  Record the wakeup address, and do wakeup.
  void enqueueWakeMaf(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aWakeTransaction);
  void enqueueWakeRegionMaf(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aWakeTransaction);

  void doNewRequests(std::vector< MessageQueue<MemoryTransport> > & aMessageQueue,
                     std::vector< MessageQueue<MemoryTransport> > & aBankQueue,
                     const int32_t numMsgQueues,
                     int32_t & lastQueue );

  void runEvictProcess(ProcessEntry_p aProcess );

  void runRequestProcess(ProcessEntry_p aProcess);

  //If there is a MAF wakeup in progress, continue it.
  void runWakeMafProcess(ProcessEntry_p aProcess);

  void runSnoopProcess(ProcessEntry_p aProcess );

  void runIProbeProcess(ProcessEntry_p aProcess );

  void runBackProcess(ProcessEntry_p aProcess );

  void runIdleWorkProcess(ProcessEntry_p aProcess );

  void runWakeSnoopProcess(ProcessEntry_p aProcess );

  //Get the block address of a process
  MemoryAddress const addressOf( ProcessEntry_p aProcess ) const;

  boost::intrusive_ptr<TransactionTracker> trackerOf( ProcessEntry_p aProcess ) const;

  // Send all enqueued transports to their destinations
  void doTransmitProcess( ProcessEntry_p aProcess );

  // Message -> Front
  void sendFront(MemoryTransport  & transport, bool to_D, bool to_I);

  // Message -> Back(Request)
  void sendBack_Request(MemoryTransport & transport);

  // Message -> Back(Prefetch)
  void sendBack_Prefetch(MemoryTransport & transport);

  // Message -> Back(Snoop)
  void sendBack_Snoop(MemoryTransport & transport);
  void sendBack_Evict(MemoryTransport & transport);

};  // end class CacheController

}  // end namespace nCache

#define DBG_Reset
#include DBG_Control()

#endif  // FLEXUS_CACHE_CONTROLLER_HPP_INCLUDED
