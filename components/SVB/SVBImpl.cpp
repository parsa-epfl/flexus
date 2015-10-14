#include <components/SVB/SVB.hpp>

#define FLEXUS_BEGIN_COMPONENT SVB
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

#include <components/Common/MessageQueues.hpp>

#include <core/stats.hpp>

#define DBG_DefineCategories SVB
#define DBG_SetDefaultOps AddCat(SVB)
#include DBG_Control()

namespace nSVB {

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
namespace SharedTypes = Flexus::SharedTypes;

using namespace nMessageQueues;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

struct by_order {};
struct by_address {};

struct SVBEntry {
  MemoryAddress theAddress;
  mutable MemoryMessage::MemoryMessageType theState; //ProbedClean, ProbedWritable, ProbedDirty
  mutable int64_t theStreamId;
  SVBEntry( MemoryAddress anAddress, MemoryMessage::MemoryMessageType aState, int64_t aStreamId)
    : theAddress(anAddress)
    , theState(aState)
    , theStreamId(aStreamId) {
  }
};

typedef multi_index_container
< SVBEntry
, indexed_by
< sequenced < tag<by_order> >
, ordered_unique
< tag<by_address>
, member< SVBEntry, MemoryAddress, &SVBEntry::theAddress >
>
>
>
svb_array_t;

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

struct MAFEntry {
  MemoryAddress theAddress;
  bool theBlockedOnAddress;
  mutable MemoryTransport theMemoryTransport;
  mutable PrefetchTransport thePrefetchTransport;
  mutable bool theIsMemory;
  mutable int64_t theTimestamp;
  mutable eEntryType theType;
public:
  MAFEntry(MemoryTransport aTransport, bool isBlocked, eEntryType aType)
    : theAddress(aTransport[MemoryMessageTag]->address())
    , theBlockedOnAddress(isBlocked)
    , theMemoryTransport(aTransport)
    , theIsMemory(true)
    , theTimestamp( theFlexus->cycleCount() )
    , theType(aType)
  {}
  MAFEntry(PrefetchTransport aTransport, bool isBlocked, eEntryType aType)
    : theAddress(aTransport[PrefetchMessageTag]->address())
    , theBlockedOnAddress(isBlocked)
    , thePrefetchTransport(aTransport)
    , theIsMemory(false)
    , theTimestamp( theFlexus->cycleCount() )
    , theType(aType)
  {}
  MemoryAddress getAddress() const {
    return theAddress;
  }
  MemoryTransport & memoryTransport() const {
    return theMemoryTransport;
  }
  PrefetchTransport & prefetchTransport() const {
    return thePrefetchTransport;
  }
  bool isMemory() const {
    return theIsMemory;
  }
  bool isBlocked() const {
    return theBlockedOnAddress;
  }
  int64_t & timestamp() const {
    return theTimestamp;
  }
  eEntryType & type() const {
    return theType;
  }
};

class MissAddressFile {
  typedef multi_index_container
  < MAFEntry
  , indexed_by
  < ordered_non_unique
  < composite_key
  < MAFEntry
  , member< MAFEntry, MemoryAddress, &MAFEntry::theAddress >
  , member< MAFEntry, bool, &MAFEntry::theBlockedOnAddress>
  >
  >
  >
  >
  maf_t;

  maf_t theMshrs;

  uint32_t theEntries[kLastType];
  Stat::StatInstanceCounter<int64_t> * theCyclesWith[kLastType];
  Stat::StatAverage * theAverage[kLastType];
  uint64_t theLastAccounting;

public:
  MissAddressFile(std::string const & aStatName)
    : theLastAccounting(0) {
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
      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        return kRead;
      case MemoryMessage::StoreReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
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

  eEntryType getType(PrefetchMessage::PrefetchMessageType aMessageType) {
    return kReadPrefetch;
  }

  typedef maf_t::iterator maf_iter;

  void account(eEntryType anEntry, int32_t aDelta) {
    //Accumulate counts since the last accounting
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

  uint32_t size() const {
    return theMshrs.size();
  }

  void insert( MemoryTransport aTransport, bool isBlocked ) {
    eEntryType type = getType(aTransport[MemoryMessageTag]->type());
    theMshrs.insert( MAFEntry( aTransport, isBlocked, type) );
    account(type, 1);
  }

  void insert( PrefetchTransport aTransport, bool isBlocked  ) {
    eEntryType type = getType(aTransport[PrefetchMessageTag]->type());
    theMshrs.insert( MAFEntry( aTransport, isBlocked, type) );
    account(type, 1);
  }

  void remove(maf_iter iter) {
    eEntryType type = iter->type();
    theMshrs.erase(iter);
    account(type, -1);
  }

  boost::optional<maf_iter> getActive( MemoryAddress const & anAddress) {
    maf_iter iter = theMshrs.find( std::make_tuple( anAddress, false ) );
    if (iter == theMshrs.end()) {
      return boost::none;
    } else {
      return iter;
    }
  }

  std::pair< maf_iter, maf_iter > getAllEntries( MemoryAddress const & aBlockAddress) {
    return theMshrs.equal_range( std::make_tuple( aBlockAddress ) );
  }

};  // end class MissAddressFile

typedef std::set< MemoryAddress > snoop_buffer_t; //Indicates that there is dirty data pending invalidation/downgrade

class FLEXUS_COMPONENT(SVB)  {
  FLEXUS_COMPONENT_IMPL(SVB);

  DelayFifo< MemoryTransport > qFrontSideInRequest;
  DelayFifo< MemoryTransport > qFrontSideInSnoop;
  DelayFifo< MemoryTransport > qBackSideIn;

  MessageQueue< MemoryTransport > qFrontSideOut;
  MessageQueue< MemoryTransport > qBackSideOutRequest;
  MessageQueue< MemoryTransport > qBackSideOutSnoop;

  MessageQueue< PrefetchTransport > qMasterIn;
  MessageQueue< PrefetchTransport > qMasterOut;

  MissAddressFile theMAF;
  boost::optional< MemoryAddress > theWakeMAFAddress;
  svb_array_t theSVBArray;
  snoop_buffer_t theSnoopBuffer;

  Stat::StatCounter statPrefetchesIssued;
  Stat::StatCounter statPrefetches_Completed;
  Stat::StatCounter statPrefetches_RedundantAbove;
  Stat::StatCounter statPrefetches_RedundantMAF;
  Stat::StatCounter statPrefetches_RedundantSVB;

  Stat::StatCounter statConflictingTransports;
  Stat::StatCounter statAccesses;
  Stat::StatCounter statAccesses_Read;
  Stat::StatCounter statHits;
  Stat::StatCounter statHits_Read;
  Stat::StatCounter statHits_Write;
  Stat::StatCounter statMisses;
  Stat::StatCounter statMisses_Read;
  Stat::StatCounter statMisses_Permission;
  Stat::StatCounter statMisses_Write;

private:
  uint32_t delay ( MemoryTransport const & aTransport ) const {
    return (aTransport[MemoryMessageTag]->isRequest() ? cfg.ProcessingDelay : 0 );
  }

  //Helper functions
  void enqueue( DelayFifo< MemoryTransport> & aQueue, MemoryTransport const & aTransport) {
    DBG_Assert( ! isFull( aQueue) );
    aQueue.enqueue( aTransport,  delay(aTransport) );
  }

  void enqueue( MessageQueue< MemoryTransport> & aQueue, MemoryTransport const & aTransport) {
    DBG_Assert( ! isFull( aQueue) );
    aQueue.enqueue( aTransport );
  }

  void enqueue( MessageQueue< PrefetchTransport> & aQueue, PrefetchTransport const & aTransport) {
    DBG_Assert( ! isFull( aQueue) );
    aQueue.enqueue( aTransport );
  }

  template <class Queue>
  bool isFull( Queue & aQueue) {
    return aQueue.full();
  }

  bool MAFFull() {
    return theMAF.size() >= cfg.MAFSize;
  }

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(SVB)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theMAF( statName() )
    , statPrefetchesIssued( statName() + "-IssuedPrefetches" )
    , statPrefetches_Completed( statName() + "-Prefetches:Completed" )
    , statPrefetches_RedundantAbove( statName() + "-Prefetches:Redundant:Above" )
    , statPrefetches_RedundantMAF( statName() + "-Prefetches:Redundant:MAF" )
    , statPrefetches_RedundantSVB( statName() + "-Prefetches:Redundant:SVB" )

    , statConflictingTransports  ( statName() + "-ConflictingTransports" )
    , statAccesses               ( statName() + "-Accesses" )
    , statAccesses_Read          ( statName() + "-Accesses:Read" )
    , statHits                   ( statName() + "-Hits" )
    , statHits_Read              ( statName() + "-Hits:Read" )
    , statHits_Write             ( statName() + "-Hits:Write" )
    , statMisses                 ( statName() + "-Misses" )
    , statMisses_Read            ( statName() + "-Misses:Read" )
    , statMisses_Permission      ( statName() + "-Misses:Permission" )
    , statMisses_Write           ( statName() + "-Misses:Write" )
  {}

  bool isQuiesced() const {
    return    qFrontSideInRequest.empty()
              &&      qFrontSideInSnoop.empty()
              &&      qBackSideIn.empty()
              &&      qFrontSideOut.empty()
              &&      qBackSideOutRequest.empty()
              &&      qBackSideOutSnoop.empty()
              &&      qMasterIn.empty()
              &&      qMasterOut.empty()
              &&      theMAF.size() == 0
              ;
  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) { }

  // Initialization
  void initialize() {
    qFrontSideInRequest.setSize( cfg.QueueSizes );
    qFrontSideInSnoop.setSize( cfg.QueueSizes);
    qBackSideIn.setSize( cfg.QueueSizes );
    qFrontSideOut.setSize( cfg.QueueSizes);
    qBackSideOutRequest.setSize( cfg.QueueSizes);
    qBackSideOutSnoop.setSize( cfg.QueueSizes);
    qMasterIn.setSize( cfg.QueueSizes );
    qMasterOut.setSize( cfg.QueueSizes );
  }

  // Ports
  bool available(interface::FrontSideIn_Request const &) {
    return ! isFull( qFrontSideInRequest );
  }
  void push(interface::FrontSideIn_Request const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FrontSideIn(Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    enqueue( qFrontSideInRequest, aMessage );
  }

  bool available(interface::FrontSideIn_Snoop const &) {
    return ! isFull( qFrontSideInSnoop );
  }
  void push(interface::FrontSideIn_Snoop const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FrontSideIn(Snoop): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    enqueue( qFrontSideInSnoop, aMessage );
  }

  bool available(interface::BackSideIn const &) {
    return ! isFull( qBackSideIn );
  }
  void push(interface::BackSideIn const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port BackSideIn: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    enqueue( qBackSideIn, aMessage );
  }

  bool available(interface::MasterIn const &) {
    return ! isFull( qMasterIn );
  }
  void push(interface::MasterIn const &,  PrefetchTransport & aMessage) {
    DBG_Assert( aMessage[PrefetchMessageTag] );
    DBG_(Iface, Comp(*this) ( << "Received on Port MasterIn: " << *(aMessage[PrefetchMessageTag]) ) );
    enqueue( qMasterIn, aMessage );
  }

  // Drive Interfaces
  void drive(interface::SVBDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "SVBDrive" ) ) ;
    process();
    transmit();
  }

  void process() {
    doWakeMAF();
    doBackSide();
    doFrontSideSnoops();
    doFrontSideRequests();
    doMasterRequests();
  }

  void transmit() {
    //Transmit will send as many messages as possible in each direction.

    while (!qBackSideOutRequest.empty() && FLEXUS_CHANNEL(BackSideOut_Request).available() ) {
      MemoryTransport transport = qBackSideOutRequest.dequeue();
      DBG_Assert(transport[MemoryMessageTag]);
      DBG_(Iface, Comp(*this) ( << "Sent on Port BackSideOut(Request): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(BackSideOut_Request) << transport;
    }

    while (!qBackSideOutSnoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
      MemoryTransport transport = qBackSideOutSnoop.dequeue();
      DBG_Assert(transport[MemoryMessageTag]);
      DBG_(Iface, Comp(*this) ( << "Sent on Port BackSideOut(Snoop): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
    }

    while (!qFrontSideOut.empty() && FLEXUS_CHANNEL(FrontSideOut).available() ) {
      MemoryTransport transport = qFrontSideOut.dequeue();
      DBG_Assert(transport[MemoryMessageTag]);
      DBG_(Iface, Comp(*this) ( << "Sent on Port FrontSideOut: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(FrontSideOut) << transport;
    }

    while (!qMasterOut.empty() && FLEXUS_CHANNEL(MasterOut).available() ) {
      PrefetchTransport transport = qMasterOut.dequeue();
      DBG_Assert(transport[PrefetchMessageTag]);
      DBG_(Iface, Comp(*this) ( << "Sent on Port MasterOut: " << *(transport[PrefetchMessageTag]) ) Addr(transport[PrefetchMessageTag]->address()) );
      FLEXUS_CHANNEL(MasterOut) << transport;
    }
  }

  //Queues messages to be sent
  void sendBackRequest( MemoryTransport & aTransport ) {
    DBG_Assert( ! qBackSideOutRequest.full() );
    qBackSideOutRequest.enqueue( aTransport );
  }

  void sendBackSnoop( MemoryTransport & aTransport ) {
    DBG_Assert( ! qBackSideOutSnoop.full() );
    qBackSideOutSnoop.enqueue( aTransport );
  }

  void sendFront( MemoryTransport & aTransport ) {
    DBG_Assert( ! qFrontSideOut.full() );
    qFrontSideOut.enqueue( aTransport  );
  }

  void sendMaster( PrefetchTransport & aTransport ) {
    DBG_Assert( ! qMasterOut.full() );
    qMasterOut.enqueue( aTransport  );
  }

private:

  void doWakeMAF(MemoryAddress const & anAddress) {
    DBG_Assert (! theWakeMAFAddress );
    theWakeMAFAddress = anAddress;
    doWakeMAF();
  }

  void doWakeMAF() {
    if (theWakeMAFAddress) {
      DBG_Assert( ! theMAF.getActive( *theWakeMAFAddress ));
      MissAddressFile::maf_iter iter, end;
      std::tie(iter, end) = theMAF.getAllEntries(*theWakeMAFAddress);
      //Can require 1 master, 1 front, 1 request, 1 snoop
      while ( iter != end && !qFrontSideOut.full() && !qBackSideOutRequest.full() && !qBackSideOutSnoop.full() && ! qMasterOut.full()) {
        //The only things that can be blocked on address in the MAF is a
        //memory request message
        DBG_Assert( iter->isMemory() );
        MemoryTransport trans( iter->memoryTransport() );
        processRequestMessage(trans);
        MissAddressFile::maf_iter temp = iter;
        ++iter;
        theMAF.remove(temp);
      }
      if (iter == end) {
        theWakeMAFAddress = boost::none;
      }
    }
  }

  //Can require 2 master, 1 front, 1 snoop
  void doBackSide() {
    while ( qBackSideIn.ready() && !qFrontSideOut.full() && !qBackSideOutSnoop.full()  && qMasterOut.hasSpace(2) ) {
      MemoryTransport trans( qBackSideIn.dequeue() );
      processBackMessage(trans);
    }
  }

  //Can require 1 snoop
  void doFrontSideSnoops() {
    while ( qFrontSideInSnoop.ready() && !qBackSideOutSnoop.full() && !qMasterOut.full() ) {
      MemoryTransport trans( qFrontSideInSnoop.dequeue() );
      processSnoopMessage(trans);
    }
  }

  //Can require 1 master, 1 front, 1 request, 1 snoop, 1 maf
  void doFrontSideRequests() {
    while ( qFrontSideInRequest.ready() && !MAFFull() && !qFrontSideOut.full() && !qBackSideOutRequest.full() && !qBackSideOutSnoop.full() && ! qMasterOut.full()) {
      MemoryTransport trans( qFrontSideInRequest.dequeue() );
      processRequestMessage(trans);
    }
  }

  //Can require 1 master, 1 request, 1 maf
  void doMasterRequests() {
    while ( !qMasterIn.empty() && !MAFFull() &&  !qBackSideOutRequest.full() &&  !qMasterOut.full() ) {
      PrefetchTransport trans( qMasterIn.dequeue() );
      processMasterMessage(trans);
    }
  }

  //Helper functions
  void drop( MemoryAddress anAddress, bool notifySnoop = true, bool notifyMaster = true) {
    svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(anAddress);
    if (iter != theSVBArray.get<by_address>().end() ) {
      DBG_(Trace, Comp(*this) ( << "Dropping: " << anAddress) );
      MemoryMessage::MemoryMessageType state = iter->theState;
      int64_t stream_id = iter->theStreamId;
      theSVBArray.get<by_address>().erase(iter);
      if (notifySnoop) {
        sendEvict( anAddress, state );
      }
      if (notifyMaster) {
        sendRemoved( anAddress, stream_id );
      }
    }
  }

  void insert( MemoryAddress anAddress,  MemoryMessage::MemoryMessageType state , int64_t aStreamId) {
    theSVBArray.push_back( SVBEntry( anAddress, state, aStreamId) );
    DBG_(Trace, Comp(*this) ( << "Inserting: " << anAddress << " from " << aStreamId << " state " << state) );
    DBG_Assert(aStreamId != 0);
    if (theSVBArray.size() > cfg.SVBSize ) {
      drop( theSVBArray.front().theAddress, true, true );
    }
  }

  void sendEvict( MemoryAddress anAddress, MemoryMessage::MemoryMessageType state ) {
    boost::intrusive_ptr<MemoryMessage> msg(new MemoryMessage(MemoryMessage::SVBClean, anAddress ));
    if (state == MemoryMessage::ProbedDirty) {
      msg->type() = MemoryMessage::EvictDirty;
      msg->reqSize() = 64;
    }
    DBG_(Trace, Comp(*this) ( << "Sending " << *msg) );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anAddress);
    tracker->setInitiator(flexusIndex());
    MemoryTransport trans;
    trans.set( MemoryMessageTag, msg);
    trans.set( TransactionTrackerTag, tracker);
    sendBackSnoop(trans);
  }

  void sendStreamFetch( MemoryAddress anAddress) {
    boost::intrusive_ptr<MemoryMessage> msg(new MemoryMessage(MemoryMessage::StreamFetch, anAddress ));
    DBG_(Trace, Comp(*this) ( << "Sending " << *msg) );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anAddress);
    tracker->setInitiator(flexusIndex());
    tracker->setOriginatorLevel(ePrefetchBuffer);
    MemoryTransport trans;
    trans.set( MemoryMessageTag, msg);
    trans.set( TransactionTrackerTag, tracker);
    sendBackRequest(trans);
  }

  void sendRedundant(MemoryAddress anAddress, int64_t aStreamId ) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::PrefetchRedundant, anAddress, aStreamId );
    DBG_(Trace, Comp(*this) ( << "Sending " << *msg) );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    PrefetchTransport trans;
    trans.set( PrefetchMessageTag, msg);
    trans.set( TransactionTrackerTag, tracker);
    sendMaster(trans);
  }

  void sendHit(MemoryAddress anAddress, int64_t aStreamId, boost::intrusive_ptr<TransactionTracker> tracker) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::LineHit, anAddress, aStreamId );
    DBG_(Trace, Comp(*this) ( << "Sending " << *msg) );
    PrefetchTransport trans;
    trans.set( PrefetchMessageTag, msg);
    trans.set( TransactionTrackerTag, tracker);
    sendMaster(trans);
  }

  void sendRemoved(MemoryAddress anAddress, int64_t aStreamId) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::LineRemoved, anAddress, aStreamId );
    DBG_(Trace, Comp(*this) ( << "Sending " << *msg) );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    PrefetchTransport trans;
    trans.set( PrefetchMessageTag, msg);
    trans.set( TransactionTrackerTag, tracker);
    sendMaster(trans);
  }

  void sendComplete(MemoryAddress anAddress, int64_t aStreamId) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::PrefetchComplete, anAddress, aStreamId );
    DBG_(Trace, Comp(*this) ( << "Sending " << *msg) );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    PrefetchTransport trans;
    trans.set( PrefetchMessageTag, msg);
    trans.set( TransactionTrackerTag, tracker);
    sendMaster(trans);
  }

  void sendReadReply( MemoryTransport & aTransport, MemoryMessage::MemoryMessageType state) {
    if (state == MemoryMessage::ProbedDirty) {
      aTransport[MemoryMessageTag]->type() = MemoryMessage::MissReplyDirty;
    } else if (state == MemoryMessage::ProbedWritable) {
      aTransport[MemoryMessageTag]->type() = MemoryMessage::MissReplyWritable;
    } else {
      aTransport[MemoryMessageTag]->type() = MemoryMessage::MissReply;
    }
    aTransport[MemoryMessageTag]->reqSize() = 64;
    aTransport[TransactionTrackerTag]->setFillLevel(ePrefetchBuffer);
    sendFront(aTransport);
  }

  void sendWriteReply( MemoryTransport & aTransport, MemoryMessage::MemoryMessageType state) {
    if (state == MemoryMessage::ProbedDirty) {
      aTransport[MemoryMessageTag]->type() = MemoryMessage::MissReplyDirty;
    } else {
      DBG_Assert(state == MemoryMessage::ProbedWritable);
      aTransport[MemoryMessageTag]->type() = MemoryMessage::MissReplyWritable;
    }
    aTransport[MemoryMessageTag]->reqSize() = 64;
    aTransport[TransactionTrackerTag]->setFillLevel(ePrefetchBuffer);
    sendFront(aTransport);
  }

  //Processing code
  void processBackMessage( MemoryTransport & aTransport) {
    DBG_Assert(aTransport[TransactionTrackerTag]);
    boost::intrusive_ptr<MemoryMessage> msg(aTransport[MemoryMessageTag]);
    DBG_Assert( msg );
    DBG_(Iface, Comp(*this) ( << "Processing MemoryMessage(Back): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0, (  << " received unaligned request: " << *msg) );

    boost::optional<MissAddressFile::maf_iter> active_transport = theMAF.getActive( msg->address() );

    switch (msg->type() ) {
        //We treat these the same - drop the line
      case MemoryMessage::Invalidate: {
        case MemoryMessage::Downgrade:
          svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(msg->address());
          if (iter != theSVBArray.get<by_address>().end() ) {
            if (iter->theState == MemoryMessage::ProbedDirty) {
              theSnoopBuffer.insert( msg->address() ); //Remember that we have dirty data
            }
            drop( msg->address(), false, true );  //Notify master only
          }
          sendFront(aTransport);
          break;
        }

      case MemoryMessage::Probe:
        sendFront(aTransport);
        break;

      case MemoryMessage::FetchReply:
      case MemoryMessage::MissReply:
      case MemoryMessage::MissReplyDirty:
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::UpgradeReply:
        DBG_Assert( active_transport );              //Better be an active MAF entry
        DBG_Assert( (*active_transport)->isMemory() );  //Better have originated at a higher cache level
        drop( msg->address(), true, true );  //Notify everyone

        theMAF.remove(*active_transport);

        sendFront(aTransport);
        doWakeMAF(msg->address());
        break;

      case MemoryMessage::PrefetchReadReply: {
        DBG_Assert ( aTransport[TransactionTrackerTag]->originatorLevel() && *(aTransport[TransactionTrackerTag]->originatorLevel()) == ePrefetchBuffer);
        DBG_Assert( active_transport );                   //Better be an active MAF entry
        DBG_Assert( ! (*active_transport)->isMemory() );  //Better be a prefetch
        int64_t stream_id = (*active_transport)->prefetchTransport()[PrefetchMessageTag]->streamID();
        insert( msg->address(), MemoryMessage::ProbedClean, stream_id );
        sendComplete( msg->address(), stream_id);
        ++statPrefetches_Completed;
        theMAF.remove(*active_transport);
        doWakeMAF(msg->address());
        break;
      }

      case MemoryMessage::PrefetchWritableReply: {
        DBG_Assert ( aTransport[TransactionTrackerTag]->originatorLevel() && *(aTransport[TransactionTrackerTag]->originatorLevel()) == ePrefetchBuffer);
        DBG_Assert( active_transport );                   //Better be an active MAF entry
        DBG_Assert( ! (*active_transport)->isMemory() );  //Better be a prefetch
        int64_t stream_id = (*active_transport)->prefetchTransport()[PrefetchMessageTag]->streamID();
        insert( msg->address(), MemoryMessage::ProbedWritable, stream_id );
        sendComplete( msg->address(), stream_id);
        ++statPrefetches_Completed;
        theMAF.remove(*active_transport);
        doWakeMAF(msg->address());
        break;
      }

      case MemoryMessage::PrefetchDirtyReply: {
        DBG_Assert ( aTransport[TransactionTrackerTag]->originatorLevel() && *(aTransport[TransactionTrackerTag]->originatorLevel()) == ePrefetchBuffer);
        DBG_Assert( active_transport );                   //Better be an active MAF entry
        DBG_Assert( ! (*active_transport)->isMemory() );  //Better be a prefetch
        int64_t stream_id = (*active_transport)->prefetchTransport()[PrefetchMessageTag]->streamID();
        insert( msg->address(), MemoryMessage::ProbedDirty, stream_id );
        sendComplete( msg->address(), stream_id);
        ++statPrefetches_Completed;
        theMAF.remove(*active_transport);
        doWakeMAF(msg->address());
        break;
      }

      case MemoryMessage::PrefetchReadRedundant: {
        DBG_Assert ( aTransport[TransactionTrackerTag]->originatorLevel() && *(aTransport[TransactionTrackerTag]->originatorLevel()) == ePrefetchBuffer);
        DBG_Assert( active_transport );                   //Better be an active MAF entry
        DBG_Assert( ! (*active_transport)->isMemory() );  //Better be a prefetch
        int64_t stream_id = (*active_transport)->prefetchTransport()[PrefetchMessageTag]->streamID();
        sendRedundant( msg->address(), stream_id);
        ++statPrefetches_RedundantAbove;                  //ReadRedundant indicates duplicate tags say L1 has the block
        theMAF.remove(*active_transport);
        doWakeMAF(msg->address());
        break;
      }

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg));
        break;
    }

  }

  void processRequestMessage( MemoryTransport & aTransport) {
    DBG_Assert(aTransport[TransactionTrackerTag]);
    boost::intrusive_ptr<MemoryMessage> msg(aTransport[MemoryMessageTag]);
    DBG_Assert( msg );
    DBG_(Iface, Comp(*this) ( << "Processing MemoryMessage(Request): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0, (  << " received unaligned request: " << *msg) );

    bool maf_conflict = theMAF.getActive(msg->address());

    switch (msg->type() ) {
      case MemoryMessage::FetchReq:
      case MemoryMessage::ReadReq:
        if ( maf_conflict ) {
          ++statConflictingTransports;

          theMAF.insert(aTransport, true);

        } else {
          ++statAccesses;
          ++statAccesses_Read;

          svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(msg->address());
          if (iter != theSVBArray.get<by_address>().end() ) {

            DBG_(Trace, Comp(*this) ( << "SVB Hit " << *msg) );
            ++statHits;
            ++statHits_Read;
            MemoryMessage::MemoryMessageType state = iter->theState;
            sendHit(msg->address(), iter->theStreamId,  aTransport[TransactionTrackerTag]);
            drop(msg->address(), false, false);  //No need to nofity

            sendReadReply( aTransport, state); //TODO
          } else {
            ++statMisses;
            ++statMisses_Read;
            theMAF.insert(aTransport, false);
            sendBackRequest( aTransport );
          }
        }
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        if ( maf_conflict ) {
          ++statConflictingTransports;

          theMAF.insert(aTransport, true);

        } else {

          svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(msg->address());
          if (iter != theSVBArray.get<by_address>().end() ) {
            MemoryMessage::MemoryMessageType state = iter->theState;
            if (state == MemoryMessage::ProbedWritable || state == MemoryMessage::ProbedDirty ) {
              DBG_(Trace, Comp(*this) ( << "SVB Hit " << *msg) );
              ++statHits;
              ++statHits_Write;
              sendHit(msg->address(), iter->theStreamId, aTransport[TransactionTrackerTag]);
              drop(msg->address(), false, false);  //No need to notify
              sendWriteReply( aTransport, state);
            } else {
              DBG_(Trace, Comp(*this) ( << "SVB Permission Miss " << *msg) );
              ++statMisses;
              ++statMisses_Permission;
              drop(msg->address(), true, true);
              theMAF.insert(aTransport, false);
              sendBackRequest( aTransport );
            }
          } else {
            ++statMisses;
            ++statMisses_Write;
            theMAF.insert(aTransport, false);
            sendBackRequest( aTransport );
          }
        }
        break;

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg));
        break;
    }
  }

  void processSnoopMessage( MemoryTransport & aTransport ) {
    DBG_Assert(aTransport[TransactionTrackerTag]);
    boost::intrusive_ptr<MemoryMessage> msg(aTransport[MemoryMessageTag]);
    DBG_Assert( msg );
    DBG_(Iface, Comp(*this) ( << "Processing MemoryMessage(Snoop): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0, (  << " received unaligned request: " << *msg) );

    switch (msg->type() ) {

        //Evictions will never allocate a block in the SVB.  However, they can
        //change the state of a block that is present.  Also, we will swallow
        //clean evict messages if the block is still in the SVB.
      case MemoryMessage::EvictClean: {
        svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(msg->address());
        if (iter == theSVBArray.get<by_address>().end() ) {
          sendBackSnoop(aTransport);
        }
        break;
      }
      case MemoryMessage::EvictWritable: {
        svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(msg->address());
        if (iter != theSVBArray.get<by_address>().end() ) {
          iter->theState = MemoryMessage::ProbedWritable;
        } else {
          sendBackSnoop(aTransport);
        }
        break;
      }
      case MemoryMessage::EvictDirty: {
        svb_array_t::index<by_address>::type::iterator iter = theSVBArray.get<by_address>().find(msg->address());
        if (iter != theSVBArray.get<by_address>().end() ) {
          iter->theState = MemoryMessage::ProbedDirty;
        } else {
          sendBackSnoop(aTransport);
        }
        break;
      }

      case MemoryMessage::ProbedDirty: {
        case MemoryMessage::ProbedClean:
        case MemoryMessage::ProbedWritable:
        case MemoryMessage::ProbedNotPresent:
          svb_array_t::index<by_address>::type::iterator svb_iter = theSVBArray.get<by_address>().find(msg->address());
          if (svb_iter != theSVBArray.get<by_address>().end() ) {
            msg->type() = MemoryMessage::maxProbe(msg->type(), svb_iter->theState);
          }
          if (theSnoopBuffer.count(msg->address())) {
            msg->type() = MemoryMessage::ProbedDirty;
          }
          sendBackSnoop(aTransport);  //We will not change a ProbedDirty into anything else
          break;
        }

      case MemoryMessage::DowngradeAck: {
        drop( msg->address(), true, true); //Notify both
        if (theSnoopBuffer.count(msg->address() )) {
          msg->type() = MemoryMessage::DownUpdateAck;
          theSnoopBuffer.erase(msg->address());
        }
        sendBackSnoop(aTransport);  //We will not change a ProbedDirty into anything else
        break;
      }
      case MemoryMessage::InvalidateAck: {
        drop( msg->address(), false, true); //Notify master only
        //Convert any blocked UpgradeReq's into WriteReq's
        MissAddressFile::maf_iter begin, end;
        std::tie(begin, end) = theMAF.getAllEntries( msg->address() );
        while (begin != end) {
          if (begin->isMemory() && begin->isBlocked() ) {
            if (begin->memoryTransport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq) {
              begin->memoryTransport()[MemoryMessageTag]->type() = MemoryMessage::WriteReq;
            } else if (begin->memoryTransport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeAllocate) {
              begin->memoryTransport()[MemoryMessageTag]->type() = MemoryMessage::WriteAllocate;
            }
          }
          ++begin;
        }
        if (theSnoopBuffer.count(msg->address() )) {
          msg->type() = MemoryMessage::InvUpdateAck;
          theSnoopBuffer.erase(msg->address());
        }
        sendBackSnoop(aTransport);  //We will not change a ProbedDirty into anything else
        break;
      }

      //Convert any blocked UpgradeReq's into WriteReq's
      case MemoryMessage::InvUpdateAck: {
        MissAddressFile::maf_iter begin, end;
        std::tie(begin, end) = theMAF.getAllEntries( msg->address() );
        while (begin != end) {
          if (begin->isMemory() && begin->isBlocked() ) {
            if (begin->memoryTransport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq) {
              begin->memoryTransport()[MemoryMessageTag]->type() = MemoryMessage::WriteReq;
            } else if (begin->memoryTransport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeAllocate) {
              begin->memoryTransport()[MemoryMessageTag]->type() = MemoryMessage::WriteAllocate;
            }
          }
          ++begin;
        }
        //pass through to next case
      }
      case MemoryMessage::DownUpdateAck:
        drop( msg->address(), false, true); //Notify master only
        theSnoopBuffer.erase( msg->address() );
        sendBackSnoop(aTransport);
        break;

      case MemoryMessage::Flush:    //Not supported
      case MemoryMessage::FlushReq:
      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg));
        break;
    }
  }

  void processMasterMessage( PrefetchTransport & trans) {
    boost::intrusive_ptr<PrefetchMessage> aMessage = trans[PrefetchMessageTag];
    DBG_Assert(aMessage);
    DBG_Assert(aMessage->type() == PrefetchMessage::PrefetchReq );

    //See if we have a Maf entry.  If we do, reject the stream fetch.
    bool maf_conflict = theMAF.getActive(aMessage->address());
    if ( maf_conflict ) {
      ++statPrefetches_RedundantMAF;
      sendRedundant( aMessage->address(), aMessage->streamID() );
      return;
    }

    //See if we already have the block in the SVB
    svb_array_t::index<by_address>::type::iterator svb_iter = theSVBArray.get<by_address>().find(aMessage->address());
    if (svb_iter != theSVBArray.get<by_address>().end() ) {
      ++statPrefetches_RedundantSVB;
      sendRedundant( aMessage->address(), aMessage->streamID() );
      return;
    }

    //Issue the prefetch
    ++statPrefetchesIssued;
    theMAF.insert(trans, false);
    sendStreamFetch( aMessage->address());
  }

};

} //End Namespace nSVB

FLEXUS_COMPONENT_INSTANTIATOR( SVB, nSVB);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SVB

#define DBG_Reset
#include DBG_Control()

