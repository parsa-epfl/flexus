#include <components/StridePrefetcher/StridePrefetcher.hpp>

#define FLEXUS_BEGIN_COMPONENT StridePrefetcher
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/none.hpp>

#include <components/Common/MessageQueues.hpp>

#include <core/stats.hpp>

#define DBG_DefineCategories StridePrefetcher
#define DBG_SetDefaultOps AddCat(StridePrefetcher)
#include DBG_Control()

namespace nStridePrefetcher {

using namespace boost::multi_index;

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
namespace SharedTypes = Flexus::SharedTypes;

using namespace nMessageQueues;

using boost::intrusive_ptr;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

enum eAction {
  kNoAction
  , kSendToFront
  , kSendToFrontAndClearMAF
  , kSendToBackSnoop
  , kSendToBackRequest
  , kBlockOnAddress
  , kBlockOnPrefetch
  , kBlockOnWatch
  , kRemoveAndWakeMAF
};

enum eEntryType {
  kRead
  , kWrite
  , kAtomic
  , kFetch
  , kStridePrefetch

  , kTotal
  , kLastType
};

struct MAFEntry {
  MemoryAddress theAddress;
  bool theBlockedOnAddress;
  mutable MemoryTransport theMemoryTransport;
  mutable int64_t theTimestamp;
  mutable eEntryType theType;
  mutable int32_t theNode;
  mutable int32_t theStride;
public:
  MAFEntry(MemoryTransport aTransport, bool isBlocked, eEntryType aType)
    : theAddress(aTransport[MemoryMessageTag]->address())
    , theBlockedOnAddress(isBlocked)
    , theMemoryTransport(aTransport)
    , theTimestamp( theFlexus->cycleCount() )
    , theType(aType)
    , theNode(0)
    , theStride(0)
  {}
  MAFEntry(MemoryTransport aTransport, bool isBlocked, eEntryType aType, int32_t aNode, int32_t aStride)
    : theAddress(aTransport[MemoryMessageTag]->address())
    , theBlockedOnAddress(isBlocked)
    , theMemoryTransport(aTransport)
    , theTimestamp( theFlexus->cycleCount() )
    , theType(aType)
    , theNode(aNode)
    , theStride(aStride)
  {}
  MemoryAddress getAddress() const {
    return theAddress;
  }
  int32_t getNode() const {
    return theNode;
  }
  int32_t getStride() const {
    return theStride;
  }
  MemoryTransport & memoryTransport() const {
    return theMemoryTransport;
  }
  bool isBlockedOnAddress() const {
    return theBlockedOnAddress;
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
    theCyclesWith[kStridePrefetch] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:StridePrefetch");
    theAverage[kStridePrefetch] = new Stat::StatAverage(aStatName + "-MAFAvg:StridePrefetch");
    theCyclesWith[kTotal] = new Stat::StatInstanceCounter<int64_t>(aStatName + "-MAFEntries:Total");
    theAverage[kTotal] = new Stat::StatAverage(aStatName + "-MAFAvg:Total");

  }

  eEntryType getType(MemoryMessage::MemoryMessageType aMessageType) {
    switch (aMessageType) {
      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::StreamFetch:
      case MemoryMessage::PrefetchReadNoAllocReq:
        return kRead;
      case MemoryMessage::StoreReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::NonAllocatingStoreReq:
      case MemoryMessage::StorePrefetchReq:
        return kWrite;
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        return kAtomic;
      case MemoryMessage::FetchReq:
        return kFetch;
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

  void insert( MemoryTransport aTransport, bool isBlocked = true ) {
    eEntryType type = getType(aTransport[MemoryMessageTag]->type());
    theMshrs.insert( MAFEntry( aTransport, isBlocked, type) );
    account(type, 1);
  }

  void insert( MemoryTransport aTransport, int32_t aNode, int32_t aStride) {
    theMshrs.insert( MAFEntry( aTransport, false, kStridePrefetch, aNode, aStride) );
    account(kStridePrefetch, 1);
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

  void make_active( maf_iter iter ) {
    theMshrs.modify(iter, [](auto& x){ return x.theBlockedOnAddress = false; });//ll::bind( &MAFEntry::theBlockedOnAddress, ll::_1 ) = false);
  }

};  // end class MissAddressFile

class FLEXUS_COMPONENT(StridePrefetcher) {
  FLEXUS_COMPONENT_IMPL(StridePrefetcher);

  DelayFifo< MemoryTransport > qFrontSideInRequest;
  DelayFifo< MemoryTransport > qFrontSideInPrefetch;
  DelayFifo< MemoryTransport > qFrontSideInSnoop;
  DelayFifo< MemoryTransport > qBackSideIn;

  MessageQueue< MemoryTransport > qFrontSideOut;
  MessageQueue< MemoryTransport > qBackSideOutRequest;
  MessageQueue< MemoryTransport > qBackSideOutPrefetch;
  MessageQueue< MemoryTransport > qBackSideOutSnoop;

  MissAddressFile theMAF;
  boost::optional< MemoryAddress > theWakeMAFAddress;
  int32_t theOutstandingPrefetches;

  Stat::StatCounter statPartialPrefetches;
  Stat::StatCounter statPartialPrefetchCycles;
  Stat::StatLog2Histogram statPartialPrefetchHistogram;

  Stat::StatCounter statLatePrefetches;
  Stat::StatCounter statLatePrefetchCycles;
  Stat::StatLog2Histogram statLatePrefetchHistogram;

  Stat::StatCounter statDelayedMisses;
  Stat::StatCounter statDelayedMissCycles;
  Stat::StatLog2Histogram statDelayedMissHistogram;

  Stat::StatCounter statRequestedPrefetches;
  Stat::StatCounter statCompletedPrefetches;
  Stat::StatCounter statCompletedPrefetches_Unknown;
  Stat::StatCounter statCompletedPrefetches_Cold;
  Stat::StatCounter statCompletedPrefetches_Coherence;
  Stat::StatCounter statCompletedPrefetches_Replacement;
  Stat::StatCounter statRedundantPrefetches;
  Stat::StatCounter statRedundantPrefetches_PresentInBuffer;
  Stat::StatCounter statRedundantPrefetches_PrefetchInProgress;

  Stat::StatCounter statAccesses;
  Stat::StatCounter statHits;
  Stat::StatCounter statHits_Partial;

  Stat::StatCounter statInvalidatedBlocks;
  Stat::StatCounter statReplacedBlocks;

  Stat::StatCounter statConflictingTransports;

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

  template <class Queue>
  bool isFull( Queue & aQueue) {
    return aQueue.full();
  }

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(StridePrefetcher)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theMAF                        ( statName()                               )
    , theOutstandingPrefetches      (0)
    , statPartialPrefetches         ( statName() + "-PartialPrefetches"        )
    , statPartialPrefetchCycles     ( statName() + "-PartialPrefetchCycles"    )
    , statPartialPrefetchHistogram  ( statName() + "-PartialPrefetchHistogram" )
    , statLatePrefetches            ( statName() + "-LatePrefetches"           )
    , statLatePrefetchCycles        ( statName() + "-LatePrefetchCycles"       )
    , statLatePrefetchHistogram     ( statName() + "-LatePrefetchHistogram"    )
    , statDelayedMisses             ( statName() + "-DelayedMisses"            )
    , statDelayedMissCycles         ( statName() + "-DelayedMissCycles"        )
    , statDelayedMissHistogram      ( statName() + "-DelayedMissHistogram"     )

    , statRequestedPrefetches                       (statName() + "-Prefetches")
    , statCompletedPrefetches                       (statName() + "-Prefetches:Completed")
    , statCompletedPrefetches_Unknown               (statName() + "-Prefetches:Completed:Unknown")
    , statCompletedPrefetches_Cold                  (statName() + "-Prefetches:Completed:Cold")
    , statCompletedPrefetches_Coherence             (statName() + "-Prefetches:Completed:Coherence")
    , statCompletedPrefetches_Replacement           (statName() + "-Prefetches:Completed:Replacement")
    , statRedundantPrefetches                       (statName() + "-Prefetches:Redundant")
    , statRedundantPrefetches_PresentInBuffer       (statName() + "-Prefetches:Redundant:PresentInBuffer")
    , statRedundantPrefetches_PrefetchInProgress    (statName() + "-Prefetches:Redundant:AlreadyInProgress")
    , statAccesses                                  (statName() + "-Accesses")
    , statHits                                      (statName() + "-Hits")
    , statHits_Partial                              (statName() + "-Hits:Partial")
    , statInvalidatedBlocks                         (statName() + "-InvalidatedBlocks")
    , statReplacedBlocks                            (statName() + "-ReplacedBlocks")
    , statConflictingTransports                     (statName() + "-ConflictingTransports")

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
              &&      theMAF.size() == 0
              &&      !theWakeMAFAddress
              ;
  }

  void saveState(std::string const & aDirName) const {
  }

  void loadState(std::string const & aDirName) {
  }

  // Initialization
  void initialize() {
    qFrontSideInRequest.setSize( cfg.QueueSizes );
    qFrontSideInSnoop.setSize( cfg.QueueSizes);
    qBackSideIn.setSize( cfg.QueueSizes );
    qFrontSideOut.setSize( cfg.QueueSizes);
    qBackSideOutRequest.setSize( cfg.QueueSizes);
    qBackSideOutPrefetch.setSize( cfg.QueueSizes);
    qBackSideOutSnoop.setSize( cfg.QueueSizes);

    theLastAddress.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2 );
    theNextAddress.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2 );
    theObservedStride.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2 );
    theCurrentStride.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2 );

  }

  // Ports
  bool available(interface::FrontSideIn_Request const &) {
    return ! isFull( qFrontSideInRequest );
  }
  void push(interface::FrontSideIn_Request const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FrontSideIn(Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    enqueue( qFrontSideInRequest, aMessage );
  }

  bool available(interface::FrontSideIn_Prefetch const &) {
    return ! isFull( qFrontSideInPrefetch );
  }
  void push(interface::FrontSideIn_Prefetch const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FrontSideIn(Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
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

  // Drive Interfaces
  void drive(interface::StrideDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "StrideDrive" ) ) ;
    process();
    transmit();
  }

public:
  //Implementation of the PB interface
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

private:

  void process() {
    doWakeMAF();
    doBackSide();
    doFrontSideSnoops();
    doFrontSideRequests();
    doFrontSidePrefetches();
    if (cfg.NumEntries > 0 ) {
      doPrefetches();
    }

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
      while ( iter != end && !qFrontSideOut.full() && !qBackSideOutRequest.full() ) {
        bool remove_maf = true;
        //The only things that can be blocked on address in the MAF is a
        //memory request message
        MemoryTransport trans( iter->memoryTransport() );
        eAction act = processRequestMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag], false, iter->isBlockedOnAddress());
        switch (act) {
          case kSendToFront:
            DBG_Assert( ! qFrontSideOut.full() );
            qFrontSideOut.enqueue(trans);
            //We added delay to the hit time for this request - our prefetch was late
            if (iter->isBlockedOnAddress()) {
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
            if (iter->isBlockedOnAddress()) {
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
    while ( qBackSideIn.ready() && !qFrontSideOut.full() && !qBackSideOutSnoop.full() && !theWakeMAFAddress) {
      MemoryTransport trans( qBackSideIn.dequeue() );
      eAction act = processBackMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag]);
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
    while ( qFrontSideInSnoop.ready() && !qBackSideOutSnoop.full() && !qBackSideOutRequest.full() && !qBackSideOutPrefetch.full() && !theWakeMAFAddress ) {
      MemoryTransport trans( qFrontSideInSnoop.dequeue() );
      eAction act = processSnoopMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag]);
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
    while ( qFrontSideInRequest.ready() && qFrontSideInSnoop.empty() && !qFrontSideOut.full() && !qBackSideOutRequest.full() && !qBackSideOutSnoop.full() && !MAFFull()) {
      MemoryTransport trans( qFrontSideInRequest.dequeue() );
      if (cfg.NumEntries > 0 ) {
        observeAccess( trans[MemoryMessageTag]->coreIdx(), trans[MemoryMessageTag]->address() );
      }
      bool has_maf_entry = theMAF.hasActive(trans[MemoryMessageTag]->address());
      eAction act = processRequestMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag], has_maf_entry, false);
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
    while ( qFrontSideInPrefetch.ready() && qFrontSideInSnoop.empty() && !qFrontSideOut.full() && !qBackSideOutPrefetch.full() && !qBackSideOutSnoop.full() && !MAFFull()) {
      MemoryTransport trans( qFrontSideInPrefetch.dequeue() );
      bool has_maf_entry = theMAF.hasActive(trans[MemoryMessageTag]->address());
      eAction act = processRequestMessage(trans[MemoryMessageTag], trans[TransactionTrackerTag], has_maf_entry, false);
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

  }

  struct StrideTableEntry {
    MemoryAddress theAddress;
    mutable bool theWritable;
    mutable int32_t theNode;
    mutable int32_t theStride;
    StrideTableEntry(MemoryAddress const & anAddress,  bool aWritable, int32_t aNode, int32_t aStride)
      : theAddress(anAddress)
      , theWritable(aWritable)
      , theNode(aNode)
      , theStride(aStride)
    {}
  };

  struct by_order {};
  struct by_address {};

  typedef multi_index_container
  < StrideTableEntry
  , indexed_by
  < sequenced < tag<by_order> >
  , ordered_unique
  < tag<by_address>
  , member< StrideTableEntry, MemoryAddress, &StrideTableEntry::theAddress >
  >
  >
  >
  stride_table_t;

  stride_table_t theBuffer;

  bool drop(MemoryAddress anAddress) {
    DBG_(VVerb, Comp(*this) ( << "Dropping address from prefetchBuffer: " << & std::hex << anAddress << & std::dec ) Addr(anAddress) );
    stride_table_t::index<by_address>::type::iterator iter = theBuffer.get<by_address>().find(anAddress);
    if (iter != theBuffer.get<by_address>().end() ) {
      theBuffer.get<by_address>().erase(iter);
      return true;
    } else {
      return false;
    }
  }

  void makeReadReply(boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker, bool aWritable) {
    switch (msg->type()) {
      case MemoryMessage::StreamFetch:
        msg->type() = MemoryMessage::StreamFetchWritableReply;
        break;
      case MemoryMessage::ReadReq:
        if (aWritable) {
          msg->type() = MemoryMessage::MissReplyWritable;
        } else {
          msg->type() = MemoryMessage::MissReply;
        }
        break;
      case MemoryMessage::FetchReq:
        msg->type() = MemoryMessage::FetchReply;
        break;
      case MemoryMessage::PrefetchReadNoAllocReq:
      case MemoryMessage::PrefetchReadAllocReq:
        if (aWritable) {
          msg->type() = MemoryMessage::PrefetchWritableReply;
        } else {
          msg->type() = MemoryMessage::PrefetchReadReply;
        }
        break;
      default:
        DBG_Assert(false);
    }
    if (tracker) {
      tracker->setFillLevel(ePrefetchBuffer);
    }
  }

  void markNonWritable(MemoryAddress anAddress) {
    DBG_(VVerb, Comp(*this)( << "Marking block non-writable: " << & std::hex << anAddress << & std::dec )  Addr(anAddress) );
    stride_table_t::index<by_address>::type::iterator iter = theBuffer.get<by_address>().find(anAddress);
    if (iter != theBuffer.get<by_address>().end() ) {
      iter->theWritable = false;
    }
  }

  void add(MemoryAddress anAddress, bool aWritable, int32_t aNode, int32_t aStride) {
    DBG_(VVerb,  Comp(*this) ( << " Adding block to prefetch buff : " << & std::hex << anAddress << & std::dec << ( aWritable ? " (writeable) " : " " ) ) Addr(anAddress) );

    if (cfg.NumEntries == 0) {
      return;
    }

    while ( theBuffer.size() >= static_cast<size_t>(cfg.NumEntries)) {
      ++statReplacedBlocks;
      theBuffer.pop_back();
    }

    theBuffer.push_front( StrideTableEntry( anAddress, aWritable, aNode, aStride) );
  }

  void makeWriteReply(boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker) {
    switch (msg->type()) {
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        msg->type() = MemoryMessage::MissReplyWritable;
        break;
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        msg->type() = MemoryMessage::UpgradeReply;
        break;
      case MemoryMessage::NonAllocatingStoreReq:
        msg->type() = MemoryMessage::NonAllocatingStoreReply;
        break;
      case MemoryMessage::StorePrefetchReq:
        msg->type() = MemoryMessage::StorePrefetchReply;
        break;
      default:
        DBG_Assert(false);
    }
    if (tracker) {
      tracker->setFillLevel(ePrefetchBuffer);
    }
  }

  std::pair<bool, bool> isHit(MemoryAddress anAddress, bool waited) {
    ++statAccesses;
    stride_table_t::index<by_address>::type::iterator iter = theBuffer.get<by_address>().find(anAddress);
    if (iter != theBuffer.get<by_address>().end() ) {
      int32_t node = iter->theNode;
      int32_t stride = iter->theStride;
      bool writable = iter->theWritable;
      drop(anAddress);
      if (waited) {
        ++statHits_Partial;
      }
      ++statHits;
      goodStride( node, stride, waited );
      return std::make_pair(true, writable);
    }
    return std::make_pair(false, false);
  }

  eAction processRequestMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker, bool hasMAFConflict, bool waited ) {
    DBG_(Iface, Comp(*this) ( << "Processing MemoryMessage(Request): " << *msg ) Addr(msg->address()) );
    DBG_Assert( msg );
    DBG_Assert( (msg->address() & 63) == 0, Comp(*this) (  << " received unaligned request: " << *msg) );

    switch (msg->type() ) {
      case MemoryMessage::StreamFetch:
      case MemoryMessage::FetchReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::PrefetchReadNoAllocReq:
        if ( hasMAFConflict ) {
          ++statConflictingTransports;
          return kBlockOnAddress;
        } else {
          bool is_hit;
          bool writable;
          std::tie(is_hit, writable) = isHit( msg->address(), waited );
          if ( is_hit ) {
            DBG_(Dev, ( << "Stride-HIT " << *msg) );
            makeReadReply( msg, tracker, writable );
            return kSendToFront;
          } else {
            return kSendToBackRequest;
          }
        }
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::NonAllocatingStoreReq:
        if ( hasMAFConflict ) {
          ++statConflictingTransports;
          return kBlockOnAddress;
        } else {
          bool is_hit;
          bool writable;
          std::tie(is_hit, writable) = isHit( msg->address(), waited );
          if ( is_hit && writable ) {
            DBG_(Dev, ( << "Stride-HIT " << *msg) );
            makeWriteReply( msg, tracker);
            return kSendToFront;
          } else {
            return kSendToBackRequest;
          }
        }

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << msg->type()));
        break;
    }
    return kNoAction;

  }

  eAction processSnoopMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) {
    DBG_(Iface, Comp(*this) ( << "Processing MemoryMessage(Snoop): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0 );

    switch (msg->type() ) {
      case MemoryMessage::FlushReq:
      case MemoryMessage::SVBClean:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::DowngradeAck:
      case MemoryMessage::Flush:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::DownUpdateAck:
      case MemoryMessage::ReturnReply:
        return kSendToBackSnoop;

      case MemoryMessage::PrefetchInsert:
      case MemoryMessage::ProbedClean:
      case MemoryMessage::ProbedWritable:
      case MemoryMessage::ProbedDirty:
      case MemoryMessage::ProbedNotPresent:
      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg ));
        break;
    }
    return kNoAction;
  }

  eAction processBackMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) {
    DBG_(Iface, Comp(*this) ( << " Processing MemoryMessage(Back): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0 );

    switch (msg->type() ) {
      case MemoryMessage::Invalidate:
        if ( drop(msg->address()) ) {
          ++statInvalidatedBlocks;
        }
        return kSendToFront;

      case MemoryMessage::ReturnReq:
        return kSendToFront;

      case MemoryMessage::Downgrade:
        markNonWritable(msg->address());
        return kSendToFront;

      case MemoryMessage::Probe:
        return kSendToFront;

      case MemoryMessage::FetchReply:
      case MemoryMessage::MissReply:
      case MemoryMessage::MissReplyDirty:
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::UpgradeReply:
      case MemoryMessage::NonAllocatingStoreReply:
      case MemoryMessage::StreamFetchWritableReply:
      case MemoryMessage::StreamFetchRejected:
        return kSendToFrontAndClearMAF;

      case MemoryMessage::PrefetchReadReply:
      case MemoryMessage::PrefetchWritableReply: {
        boost::optional<MissAddressFile::maf_iter> maf_entry = theMAF.hasActive( msg->address() );
        DBG_Assert( maf_entry );
        if ( (*maf_entry)->type() == kStridePrefetch ) {
          ++statCompletedPrefetches;
          --theOutstandingPrefetches;
          DBG_(Iface, ( << "Stride prefetch complete " << *msg) );
          add( msg->address(), msg->type() == MemoryMessage::PrefetchWritableReply, (*maf_entry)->getNode(), (*maf_entry)->getStride() );
          return kRemoveAndWakeMAF;

        } else {
          return kSendToFrontAndClearMAF;
        }
      }

      case MemoryMessage::PrefetchReadRedundant: {
        boost::optional<MissAddressFile::maf_iter> maf_entry = theMAF.hasActive( msg->address() );
        DBG_Assert( maf_entry );
        if ( (*maf_entry)->type() == kStridePrefetch ) {
          ++statRedundantPrefetches;
          --theOutstandingPrefetches;
          return kRemoveAndWakeMAF;
        } else {
          return kSendToFrontAndClearMAF;
        }
      }

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg ));
        return kNoAction;

    }
  }

  std::deque< StrideTableEntry > thePrefetchQueue;
  std::vector< MemoryAddress > theLastAddress;
  std::vector< MemoryAddress > theNextAddress;
  std::vector< int32_t > theObservedStride;
  std::vector< int32_t > theCurrentStride;

  void enqueuePrefetch( MemoryAddress anAddress, int32_t aNode, int32_t aStride) {
    while (thePrefetchQueue.size() >= cfg.MaxPrefetchQueue) {
      thePrefetchQueue.pop_front();
    }
    thePrefetchQueue.push_back( StrideTableEntry( anAddress, false, aNode, aStride ) );
  }

  void goodStride( int32_t aNode, int32_t aStride, bool accelerate) {
    if (theCurrentStride[aNode] == aStride) {
      enqueuePrefetch( theNextAddress[aNode], aNode, aStride );
      theNextAddress[aNode] += aStride;
      if (accelerate) {
        enqueuePrefetch( theNextAddress[aNode], aNode, aStride );
        theNextAddress[aNode] += aStride;
      }
    }
  }

  void observeAccess( int32_t aNode, MemoryAddress anAddress) {
    int32_t stride = static_cast<int64_t>(anAddress) - theLastAddress[aNode];
    if (stride == theObservedStride[aNode] && stride != 0) {
      theCurrentStride[aNode] = stride;
      theNextAddress[aNode] = MemoryAddress(anAddress + stride);
      goodStride( aNode, stride, true);
      goodStride( aNode, stride, true);
    }
    theObservedStride[aNode] = stride;
    theLastAddress[aNode] = anAddress;
  }

  void doPrefetches() {
    while ( !thePrefetchQueue.empty() && !qBackSideOutPrefetch.full() && !MAFFull() && theOutstandingPrefetches < cfg.MaxOutstandingPrefetches) {
      StrideTableEntry entry( thePrefetchQueue.front() );
      thePrefetchQueue.pop_front();
      bool has_maf_entry = theMAF.hasActive(entry.theAddress);
      if (has_maf_entry) {
        //Redundant
        ++statRedundantPrefetches;
      } else if (theBuffer.get<by_address>().count(entry.theAddress) < 0 ) {
        //Redundant
        ++statRedundantPrefetches;
      } else {
        //Issue prefetch
        boost::intrusive_ptr<MemoryMessage> msg;
        msg = new MemoryMessage(MemoryMessage::PrefetchReadAllocReq, entry.theAddress );
        boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
        tracker->setAddress(entry.theAddress);
        tracker->setInitiator(entry.theNode);
        tracker->setOriginatorLevel(ePrefetchBuffer);
        ++theOutstandingPrefetches;
        DBG_(Iface, ( << "Issue prefetch " << *msg ) );

        DBG_Assert( ! qBackSideOutPrefetch.full() );
        MemoryTransport trans;
        trans.set(MemoryMessageTag, msg);
        trans.set(TransactionTrackerTag, tracker);
        theMAF.insert( trans, entry.theNode, entry.theStride );
        qBackSideOutPrefetch.enqueue( trans );
      }
    }
  }

};

} //End Namespace nStridePrefetcher

FLEXUS_COMPONENT_INSTANTIATOR( StridePrefetcher, nStridePrefetcher );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT StridePrefetcher

#define DBG_Reset
#include DBG_Control()

