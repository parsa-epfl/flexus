#include <components/PrefetchBuffer/PrefetchBuffer.hpp>

#define FLEXUS_BEGIN_COMPONENT PrefetchBuffer
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/none.hpp>

#include <components/Common/MessageQueues.hpp>
#include <components/PrefetchBuffer/PBController.hpp>

#include <core/stats.hpp>

#define DBG_DefineCategories PrefetchBuffer
#define DBG_SetDefaultOps AddCat(PrefetchBuffer)
#include DBG_Control()

namespace nPrefetchBuffer {

using namespace boost::multi_index;

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
namespace SharedTypes = Flexus::SharedTypes;

using namespace nMessageQueues;

using boost::intrusive_ptr;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

enum eEntryType {
  kRead
  , kWrite
  , kAtomic
  , kFetch
  , kReadPrefetch
  , kWritePrefetch
  , kWatchEntry

  , kTotal
  , kLastType
};

struct MAFEntry {
  MemoryAddress theAddress;
  bool theBlockedOnAddress;
  mutable MemoryTransport theMemoryTransport;
  mutable PrefetchTransport thePrefetchTransport;
  mutable bool theIsMemory;
  mutable bool theProbed;
  mutable bool theCancelled;
  mutable bool theWatchProbe;
  mutable int64_t theTimestamp;
  mutable eEntryType theType;
public:
  MAFEntry(MemoryTransport aTransport, bool isBlocked, eEntryType aType)
    : theAddress(aTransport[MemoryMessageTag]->address())
    , theBlockedOnAddress(isBlocked)
    , theMemoryTransport(aTransport)
    , theIsMemory(true)
    , theProbed(false)
    , theCancelled(false)
    , theWatchProbe(false)
    , theTimestamp( theFlexus->cycleCount() )
    , theType(aType)
  {}
  MAFEntry(PrefetchTransport aTransport, bool isBlocked, eEntryType aType, bool isWatch = false)
    : theAddress(aTransport[PrefetchMessageTag]->address())
    , theBlockedOnAddress(isBlocked)
    , thePrefetchTransport(aTransport)
    , theIsMemory(false)
    , theProbed(false)
    , theCancelled(false)
    , theWatchProbe(isWatch)
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
  bool isBlockedOnAddress() const {
    return theBlockedOnAddress;
  }
  bool isMemory() const {
    return theIsMemory;
  }
  bool isPrefetch() const {
    return ! theIsMemory;
  }
  bool & probed() const {
    return theProbed;
  }
  bool & watch() const {
    return theWatchProbe;
  }
  bool & cancelled() const {
    return theCancelled;
  }
  int64_t & timestamp() const {
    return theTimestamp;
  }
  eEntryType type() const {
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
    theCyclesWith[kWatchEntry] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Watch");
    theAverage[kWatchEntry] = new Stat::StatAverage(aStatName + "-MAFAvg:Watch");
    theCyclesWith[kTotal] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Total");
    theAverage[kTotal] = new Stat::StatAverage(aStatName + "-MAFAvg:Total");

  }

  eEntryType getType(MemoryMessage::MemoryMessageType aMessageType) {
    switch (aMessageType) {
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
        return kReadPrefetch;
      case MemoryMessage::StorePrefetchReq:
        return kWritePrefetch;
      default:
        DBG_Assert(false, ( << "Illegal message type entered into MAF: " << aMessageType) );
        return kRead;
    }
  }

  eEntryType getType(PrefetchMessage::PrefetchMessageType aMessageType) {
    switch (aMessageType) {
      case PrefetchMessage::PrefetchReq:
        return kReadPrefetch;
      case PrefetchMessage::WatchReq:
        return kWatchEntry;
      default:
        DBG_Assert(false, ( << "Illegal message type entered into MAF: " << aMessageType) );
        return kRead;
    }
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

  eProbeMAFResult  probeMAF( MemoryAddress const & anAddress) const {
    maf_t::iterator iter = theMshrs.find( std::make_tuple( anAddress, false ) );
    if (iter == theMshrs.end()) {
      return kNoMatch;
    } else {
      if (! iter->isPrefetch()) {
        return kNoMatch;
      }
      if (iter->probed()) {
        return kNoMatch;
      }
      iter->probed() = true;
      if (iter->watch()) {
        return kWatch;
      } else {
        if (iter->cancelled()) {
          return kCancelled;
        } else {
          return kNotCancelled;
        }
      }
    }
  }

  bool cancel( MemoryAddress const & anAddress) const {
    maf_t::iterator iter = theMshrs.find( std::make_tuple( anAddress, false ) );
    if (iter == theMshrs.end()) {
      return false;
    } else {
      if (iter->cancelled()) {
        return false;
      } else {
        iter->cancelled() = true;
        return true;
      }
    }
  }

  void insert( MemoryTransport aTransport, bool isBlocked = true ) {
    eEntryType type = getType(aTransport[MemoryMessageTag]->type());
    theMshrs.insert( MAFEntry( aTransport, isBlocked, type) );
    account(type, 1);
  }

  void insert( PrefetchTransport aTransport ) {
    eEntryType type = getType(aTransport[PrefetchMessageTag]->type());
    theMshrs.insert( MAFEntry( aTransport, false, type) );
    account(type, 1);
  }

  void insertWatch( PrefetchTransport aTransport ) {
    eEntryType type = getType(aTransport[PrefetchMessageTag]->type());
    theMshrs.insert( MAFEntry( aTransport, false, type, true) );
    account(type, 1);
  }

  boost::optional<maf_iter> hasActive( MemoryAddress const & anAddress) {
    maf_iter iter = theMshrs.find( std::make_tuple( anAddress, false ) );
    if (iter == theMshrs.end()) {
      return boost::none;
    } else {
      return iter;
    }
  }

  bool noEntries( MemoryAddress const & anAddress) {
    maf_iter iter = theMshrs.find( std::make_tuple( anAddress ) );
    if (iter == theMshrs.end()) {
      return true;
    } else {
      DBG_( Dev, ( << *iter->memoryTransport()[MemoryMessageTag] ) );
      return false;
    }
  }

  void removeActive( MemoryAddress const & anAddress) {
    maf_t::iterator iter = theMshrs.find( std::make_tuple( anAddress, false ) );
    if (iter != theMshrs.end()) {
      eEntryType type = iter->type();
      theMshrs.erase(iter);
      account(type, -1);
    }
  }

  void removeAll( MemoryAddress const & anAddress) {
    maf_t::iterator first, temp, last;
    std::tie(first, last) = getAllEntries( anAddress );
    while (first != last) {
      temp = first;
      ++first;
      eEntryType type = temp->type();
      theMshrs.erase(temp);
      account(type, -1);
    }
  }

  std::pair< maf_iter, maf_iter > getAllEntries( MemoryAddress const & aBlockAddress) {
    return theMshrs.equal_range( std::make_tuple( aBlockAddress ) );
  }

  void remove(maf_iter iter) {
    eEntryType type = iter->type();
    theMshrs.erase(iter);
    account(type, -1);
  }

  void blockOnPrefetch( maf_iter iter ) {
    theMshrs.modify(iter, [](auto& x){ return x.theBlockedOnAddress = false; });//ll::bind( &MAFEntry::theBlockedOnAddress, ll::_1 ) = false);
  }

  void make_active( maf_iter iter ) {
    theMshrs.modify(iter, [](auto& x){ return x.theBlockedOnAddress = false; });//ll::bind( &MAFEntry::theBlockedOnAddress, ll::_1 ) = false);
  }

};  // end class MissAddressFile

class FLEXUS_COMPONENT(PrefetchBuffer), public PB {
  FLEXUS_COMPONENT_IMPL(PrefetchBuffer);

  boost::scoped_ptr<PBController> theController;

  boost::optional< MemoryAddress > theWakeMAFAddress;

  DelayFifo< MemoryTransport > qFrontSideInRequest;
  DelayFifo< MemoryTransport > qFrontSideInPrefetch;
  DelayFifo< MemoryTransport > qFrontSideInSnoop;
  DelayFifo< MemoryTransport > qBackSideIn;

  MessageQueue< MemoryTransport > qFrontSideOut;
  MessageQueue< MemoryTransport > qBackSideOutRequest;
  MessageQueue< MemoryTransport > qBackSideOutPrefetch;
  MessageQueue< MemoryTransport > qBackSideOutSnoop;

  MessageQueue< PrefetchTransport > qMasterIn;
  MessageQueue< PrefetchTransport > qMasterOut;

  MissAddressFile theMAF;

  Stat::StatCounter statPartialPrefetches;
  Stat::StatCounter statPartialPrefetchCycles;
  Stat::StatLog2Histogram statPartialPrefetchHistogram;

  Stat::StatCounter statLatePrefetches;
  Stat::StatCounter statLatePrefetchCycles;
  Stat::StatLog2Histogram statLatePrefetchHistogram;

  Stat::StatCounter statDelayedMisses;
  Stat::StatCounter statDelayedMissCycles;
  Stat::StatLog2Histogram statDelayedMissHistogram;

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

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(PrefetchBuffer)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theMAF                        ( statName()                               )
    , statPartialPrefetches         ( statName() + "-PartialPrefetches"        )
    , statPartialPrefetchCycles     ( statName() + "-PartialPrefetchCycles"    )
    , statPartialPrefetchHistogram  ( statName() + "-PartialPrefetchHistogram" )
    , statLatePrefetches            ( statName() + "-LatePrefetches"           )
    , statLatePrefetchCycles        ( statName() + "-LatePrefetchCycles"       )
    , statLatePrefetchHistogram     ( statName() + "-LatePrefetchHistogram"    )
    , statDelayedMisses             ( statName() + "-DelayedMisses"            )
    , statDelayedMissCycles         ( statName() + "-DelayedMissCycles"        )
    , statDelayedMissHistogram      ( statName() + "-DelayedMissHistogram"     )
  {}

  bool isQuiesced() const {
    return    qFrontSideInRequest.empty()
              &&      qFrontSideInPrefetch.empty()
              &&      qFrontSideInSnoop.empty()
              &&      qBackSideIn.empty()
              &&      qFrontSideOut.empty()
              &&      qBackSideOutRequest.empty()
              &&      qBackSideOutPrefetch.empty()
              &&      qBackSideOutSnoop.empty()
              &&      qMasterIn.empty()
              &&      qMasterOut.empty()
              &&      theMAF.size() == 0
              &&      !theWakeMAFAddress
              ;
  }

  void saveState(std::string const & aDirName) const {
    theController->saveState(aDirName);
  }

  void loadState(std::string const & aDirName) {
    theController->loadState(aDirName);
  }

  // Initialization
  void initialize() {
    theController.reset(PBController::construct(this, statName(), flexusIndex(), cfg.NumEntries, cfg.NumWatches, cfg.EvictClean, cfg.UseStreamFetch));
    qFrontSideInRequest.setSize( cfg.QueueSizes );
    qFrontSideInSnoop.setSize( cfg.QueueSizes);
    qBackSideIn.setSize( cfg.QueueSizes );
    qFrontSideOut.setSize( cfg.QueueSizes);
    qBackSideOutRequest.setSize( cfg.QueueSizes);
    qBackSideOutPrefetch.setSize( cfg.QueueSizes);
    qBackSideOutSnoop.setSize( cfg.QueueSizes);
    qMasterIn.setSize( cfg.QueueSizes * 2 );
    qMasterOut.setSize( cfg.QueueSizes * 2 );
  }

  // Ports
  bool available(interface::FrontSideIn_Request const &) {
    return ! isFull( qFrontSideInRequest );
  }
  void push(interface::FrontSideIn_Request const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FrontSideIn(Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    MemoryAddress addr = aMessage[MemoryMessageTag]->address();
    FLEXUS_CHANNEL(RecentRequests) << addr;
    enqueue( qFrontSideInRequest, aMessage );
  }

  bool available(interface::FrontSideIn_Prefetch const &) {
    return ! isFull( qFrontSideInPrefetch );
  }
  void push(interface::FrontSideIn_Prefetch const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FrontSideIn(Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    MemoryAddress addr = aMessage[MemoryMessageTag]->address();
    FLEXUS_CHANNEL(RecentRequests) << addr;
    enqueue( qFrontSideInPrefetch, aMessage );
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
    DBG_(Iface, Comp(*this) ( << "Received on Port MasterIn: " << *(aMessage[PrefetchMessageTag]) ) Addr(aMessage[PrefetchMessageTag]->address()) );
    if (! cfg.NumEntries == 0) {
      enqueue( qMasterIn, aMessage );
    }
  }

  // Drive Interfaces
  void drive(interface::PrefetchDrive const &) {
    DBG_(VVerb, ( << "PrefetchBufferDrive" ) ) ;
    process();
    transmit();
  }

public:
  //Implementation of the PB interface
  void sendMaster( boost::intrusive_ptr<PrefetchMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) {
    DBG_Assert( ! qMasterOut.full() );
    PrefetchTransport trans;
    trans.set(PrefetchMessageTag, msg);
    trans.set(TransactionTrackerTag, tracker);
    qMasterOut.enqueue( trans );
  }

  void sendBackRequest( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) {
    DBG_Assert( ! qBackSideOutRequest.full() );
    MemoryTransport trans;
    trans.set(MemoryMessageTag, msg);
    trans.set(TransactionTrackerTag, tracker);
    qBackSideOutRequest.enqueue( trans );
  }

  void sendBackPrefetch( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) {
    DBG_Assert( ! qBackSideOutPrefetch.full() );
    MemoryTransport trans;
    trans.set(MemoryMessageTag, msg);
    trans.set(TransactionTrackerTag, tracker);
    qBackSideOutPrefetch.enqueue( trans );
  }

  void sendBackSnoop( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) {
    DBG_Assert( ! qBackSideOutSnoop.full() );
    MemoryTransport trans;
    trans.set(MemoryMessageTag, msg);
    trans.set(TransactionTrackerTag, tracker);
    qBackSideOutSnoop.enqueue( trans );
  }

  void sendFront( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) {
    DBG_Assert( ! qFrontSideOut.full() );
    MemoryTransport trans;
    trans.set(MemoryMessageTag, msg);
    trans.set(TransactionTrackerTag, tracker);
    qFrontSideOut.enqueue( trans );
  }

  bool cancel( MemoryAddress const & anAddress ) {
    return theMAF.cancel( anAddress );
  }

  eProbeMAFResult probeMAF( MemoryAddress const & anAddress ) {
    return theMAF.probeMAF(anAddress);
  }

private:

  void process() {
    doWakeMAF();
    doBackSide();
    doFrontSideSnoops();
    doFrontSideRequests();
    doFrontSidePrefetches();
    doMasterRequests();
  }

  void doWakeMAF(MemoryAddress const & anAddress) {
    DBG_Assert (! theWakeMAFAddress );
    theWakeMAFAddress = anAddress;
    doWakeMAF();
  }

  void doWakeMAF() {
    if (theWakeMAFAddress) {
      DBG_Assert( ! theMAF.hasActive( *theWakeMAFAddress ));
      MissAddressFile::maf_iter iter, end;
      std::tie(iter, end) = theMAF.getAllEntries(*theWakeMAFAddress);
      while ( iter != end && !qFrontSideOut.full() && !qMasterOut.full() && !qBackSideOutRequest.full() ) {
        bool remove_maf = true;
        //The only things that can be blocked on address in the MAF is a
        //memory request message
        DBG_Assert( iter->isMemory() );
        MemoryTransport trans( iter->memoryTransport() );
        eAction act = theController->processRequestMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag], false, iter->isMemory() && iter->isBlockedOnAddress());
        switch (act) {
          case kSendToFront:
            DBG_Assert( ! qFrontSideOut.full() );
            qFrontSideOut.enqueue(trans);
            //We added delay to the hit time for this request - our prefetch was late
            if (iter->isMemory() && iter->isBlockedOnAddress()) {
              ++statPartialPrefetches;
              statPartialPrefetchCycles += theFlexus->cycleCount() - iter->timestamp();
              statPartialPrefetchHistogram << theFlexus->cycleCount() - iter->timestamp();
            }
            break;
          case kSendToBackRequest:
            DBG_Assert( ! qBackSideOutRequest.full() );
            remove_maf = false;
            qBackSideOutRequest.enqueue(trans);
            //We added delay to the miss time for this request
            if (iter->isMemory() && iter->isBlockedOnAddress()) {
              ++statDelayedMisses;
              statDelayedMissCycles += theFlexus->cycleCount() - iter->timestamp();
              statDelayedMissHistogram << theFlexus->cycleCount() - iter->timestamp();
            }
            break;
          case kBlockOnAddress: //Block on address is not possible, since has_maf is false
          default:
            DBG_Assert( false, ( << " Invalid action from woken request: " << act) );
        }
        MissAddressFile::maf_iter temp = iter;
        ++iter;
        if (remove_maf) {
          theMAF.remove(temp);
        } else {
          theMAF.make_active(temp);
          theWakeMAFAddress = boost::none;
          return;
        }
      }
      if (iter == end) {
        theWakeMAFAddress = boost::none;
      }
    }
  }

  void doBackSide() {
    while ( qBackSideIn.ready() && !qFrontSideOut.full() && !qBackSideOutSnoop.full() && qMasterOut.hasSpace(2) && !theWakeMAFAddress) {
      MemoryTransport trans( qBackSideIn.dequeue() );
      eAction act = theController->processBackMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag]);
      switch (act) {
        case kSendToFront:
          DBG_Assert( ! qFrontSideOut.full() );
          qFrontSideOut.enqueue(trans);
          break;
        case kSendToFrontAndClearMAF:
          DBG_Assert( ! qFrontSideOut.full() );
          theMAF.removeActive(trans[MemoryMessageTag]->address());
          qFrontSideOut.enqueue(trans);
          if (! theMAF.noEntries(trans[MemoryMessageTag]->address())) {
            doWakeMAF(trans[MemoryMessageTag]->address());
          }
          break;
        case kRemoveAndWakeMAF:
          theMAF.removeActive(trans[MemoryMessageTag]->address());
          doWakeMAF(trans[MemoryMessageTag]->address());
          break;
        case kNoAction:
          break;
        default:
          DBG_Assert( false, ( << " Invalid action from back-side: " << act) );
      }
    }
  }

  void doFrontSideSnoops() {
    while ( qFrontSideInSnoop.ready() && !qBackSideOutSnoop.full() && !qMasterOut.full() && !qBackSideOutRequest.full() && !qBackSideOutPrefetch.full() && !theWakeMAFAddress ) {
      MemoryTransport trans( qFrontSideInSnoop.dequeue() );
      eAction act = theController->processSnoopMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag]);
      switch (act) {
        case kSendToBackSnoop:
          DBG_Assert( ! qBackSideOutSnoop.full() );
          qBackSideOutSnoop.enqueue(trans);
          break;
        case kRemoveAndWakeMAF:
          theMAF.removeActive(trans[MemoryMessageTag]->address());
          doWakeMAF(trans[MemoryMessageTag]->address());
          break;
        case kNoAction:
          break;
        default:
          DBG_Assert( false, ( << " Invalid action from back-side: " << act) );
      }
    }
  }

  bool MAFFull() {
    return theMAF.size() >= cfg.MAFSize;
  }

  void doFrontSideRequests() {
    while ( qFrontSideInRequest.ready() && qFrontSideInSnoop.empty() && !qFrontSideOut.full() && !qMasterOut.full() && !qBackSideOutRequest.full() && !qBackSideOutSnoop.full() && !MAFFull()) {
      MemoryTransport trans( qFrontSideInRequest.dequeue() );
      bool has_maf_entry = theMAF.hasActive(trans[MemoryMessageTag]->address());
      eAction act = theController->processRequestMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag], has_maf_entry, false);
      switch (act) {

        case kSendToFront:
          DBG_Assert( ! qFrontSideOut.full() );
          qFrontSideOut.enqueue(trans);
          break;
        case kSendToBackRequest:
          DBG_Assert( ! qBackSideOutRequest.full() );
          DBG_Assert( ! MAFFull());
          theMAF.insert( trans, false );
          qBackSideOutRequest.enqueue(trans);
          break;
        case kBlockOnAddress:
          DBG_Assert( ! MAFFull());
          theMAF.insert( trans );
          break;
        default:
          DBG_Assert( false, ( << " Invalid action from front-side request: " << act) );
      }
    }
  }

  void doFrontSidePrefetches() {
    while ( qFrontSideInPrefetch.ready() && qFrontSideInSnoop.empty() && !qFrontSideOut.full() && !qMasterOut.full() && !qBackSideOutPrefetch.full() && !qBackSideOutSnoop.full() && !MAFFull()) {
      MemoryTransport trans( qFrontSideInPrefetch.dequeue() );
      bool has_maf_entry = theMAF.hasActive(trans[MemoryMessageTag]->address());
      eAction act = theController->processRequestMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag], has_maf_entry, false);
      switch (act) {

        case kSendToFront:
          DBG_Assert( ! qFrontSideOut.full() );
          qFrontSideOut.enqueue(trans);
          break;
        case kSendToBackRequest:
          DBG_Assert( ! qBackSideOutPrefetch.full() );
          DBG_Assert( ! MAFFull());
          theMAF.insert( trans, false );
          qBackSideOutPrefetch.enqueue(trans);
          break;
        case kBlockOnAddress:
          DBG_Assert( ! MAFFull());
          theMAF.insert( trans );
          break;
        default:
          DBG_Assert( false, ( << " Invalid action from front-side request: " << act) );
      }
    }
  }

  void doMasterRequests() {
    while ( !qMasterIn.empty() && !qFrontSideOut.full() && !qMasterOut.full() && !MAFFull()) {
      PrefetchTransport trans( qMasterIn.dequeue() );
      DBG_Assert(trans[PrefetchMessageTag]);
      boost::optional<MissAddressFile::maf_iter> maf_entry = theMAF.hasActive(trans[PrefetchMessageTag]->address());
      eAction act = theController->processPrefetch(trans[PrefetchMessageTag], trans[TransactionTrackerTag], maf_entry);
      switch (act) {
        case kNoAction:
          if ( maf_entry && trans[PrefetchMessageTag]->type() == PrefetchMessage::PrefetchReq) {
            //Prefetch command was late, there is already a miss outstanding.
            //See if the miss is a prefetch or a real miss.
            if ((*maf_entry)->isMemory()) {
              //We received a prefetch command and the address already has a
              //real miss outstanding.  The prefetch was late.
              ++statLatePrefetches;
              statLatePrefetchCycles += theFlexus->cycleCount() - (*maf_entry)->timestamp();
              statLatePrefetchHistogram << theFlexus->cycleCount() - (*maf_entry)->timestamp();
            }
          }
          break;
        case kBlockOnPrefetch:
          DBG_Assert( ! MAFFull());
          theMAF.insert( trans );
          break;
        case kBlockOnWatch:
          DBG_Assert( ! MAFFull());
          theMAF.insertWatch( trans );
          break;
        default:
          DBG_Assert( false, ( << " Invalid action from front-side request: " << act) );
      }
    }
  }

  void transmit() {
    //Transmit will send as many messages as possible in each direction.

    while (!qBackSideOutRequest.empty() && FLEXUS_CHANNEL(BackSideOut_Request).available() ) {
      MemoryTransport transport = qBackSideOutRequest.dequeue();
      DBG_(Iface, Comp(*this) ( << "Sent on Port BackSideOut(Request): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(BackSideOut_Request) << transport;
    }

    while (!qBackSideOutPrefetch.empty() && FLEXUS_CHANNEL(BackSideOut_Prefetch).available() ) {
      MemoryTransport transport = qBackSideOutPrefetch.dequeue();
      DBG_(Iface, Comp(*this) ( << "Sent on Port BackSideOut(Prefetch): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(BackSideOut_Prefetch) << transport;
    }

    while (!qBackSideOutSnoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
      MemoryTransport transport = qBackSideOutSnoop.dequeue();
      DBG_(Iface, Comp(*this) ( << "Sent on Port BackSideOut(Snoop): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
    }

    while (!qFrontSideOut.empty() && FLEXUS_CHANNEL(FrontSideOut).available() ) {
      MemoryTransport transport = qFrontSideOut.dequeue();
      DBG_(Iface, Comp(*this) ( << "Sent on Port FrontSideOut: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(FrontSideOut) << transport;
    }

    while (!qMasterOut.empty() && FLEXUS_CHANNEL(MasterOut).available() ) {
      PrefetchTransport transport = qMasterOut.dequeue();
      DBG_(Iface, Comp(*this) ( << "Sent on Port MasterOut: " << *(transport[PrefetchMessageTag]) ) Addr(transport[PrefetchMessageTag]->address()) );
      FLEXUS_CHANNEL(MasterOut) << transport;
    }

  }

};

} //End Namespace nPrefetchBuffer

FLEXUS_COMPONENT_INSTANTIATOR( PrefetchBuffer, nPrefetchBuffer );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PrefetchBuffer

#define DBG_Reset
#include DBG_Control()

