#include <components/PrefetchListener/PrefetchListener.hpp>

#define FLEXUS_BEGIN_COMPONENT PrefetchListener
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories PrefetchListener, PrefetchTrace
#define DBG_SetDefaultOps AddCat(PrefetchListener)
#include DBG_Control()

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>

namespace nPrefetchListener {

using namespace boost::multi_index;

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
using namespace Flexus::SharedTypes;

using boost::intrusive_ptr;

#ifdef FLEXUS_TRACK_PBHITS
Stat::StatInstanceCounter<int64_t> thePBHits_User("sys-PBHits:User");
Stat::StatInstanceCounter<int64_t> thePBHits_System("sys-PBHits:System");
#endif //FLEXUS_TRACK_PBHITS

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

struct PLStats {

  Stat::StatCounter statCompletedPrefetches;
  Stat::StatCounter statRejectedPrefetches;
  Stat::StatCounter statDiscardedLines;
  Stat::StatCounter statReplacedLines;
  Stat::StatCounter statInvalidatedLines;
  Stat::StatCounter statHits;
  Stat::StatCounter statHits_User;
  Stat::StatCounter statHits_System;
  Stat::StatCounter statHotHits;
  Stat::StatCounter statPrefetchAddressLists;
  Stat::StatCounter statIgnoredAddressLists;
  Stat::StatCounter statIgnoredAddressLists_Contained;
  Stat::StatCounter statIgnoredAddressLists_Extends;
  Stat::StatCounter statIntersectingAddressLists;
  Stat::StatCounter statInitiatedAddresses;
  Stat::StatCounter statRedundantAddresses;
  Stat::StatCounter statWrappedStreams;
  Stat::StatCounter statNoQueueClaimable;

  Stat::StatCounter statWatchPresent;
  Stat::StatCounter statWatchRequested;
  Stat::StatCounter statWatchRedundant;
  Stat::StatCounter statWatchRemoved;
  Stat::StatCounter statWatchReplaced;

  Stat::StatWeightedLog2Histogram statHitsByLength;
  Stat::StatLog2Histogram statLengths;

  Stat::StatLog2Histogram statPrefetchLatency;
  Stat::StatCounter statPrefetchLatencySum;
  Stat::StatLog2Histogram statLifetimeToHit;
  Stat::StatLog2Histogram statLifetimeToDiscard;
  Stat::StatLog2Histogram statHitInterarrivals;
  Stat::StatCounter statHitInterarrivalSum;

  Stat::StatCounter statOrphan_CompletedPrefetches;
  Stat::StatCounter statOrphan_RejectedPrefetches;
  Stat::StatCounter statOrphan_DiscardedLines;
  Stat::StatCounter statOrphan_ReplacedLines;
  Stat::StatCounter statOrphan_InvalidatedLines;
  Stat::StatCounter statOrphan_Hits;

  Stat::StatCounter statKilledFromRedundancy;

#ifdef FLEXUS_TRACK_HIT_INSTANCES
  Stat::StatInstanceCounter<int64_t> statHitInstances_User;
  Stat::StatInstanceCounter<int64_t> statHitInstances_System;
#endif //FLEXUS_TRACK_HIT_INSTANCES

  PLStats(std::string const & statName)
    : statCompletedPrefetches          ( statName + "-Prefetches:Completed")
    , statRejectedPrefetches           ( statName + "-Prefetches:Rejected")
    , statDiscardedLines               ( statName + "-Discards")
    , statReplacedLines                ( statName + "-Discards:Replaced")
    , statInvalidatedLines             ( statName + "-Discards:Invalidated")
    , statHits                         ( statName + "-Hits")
    , statHits_User                    ( statName + "-Hits:User")
    , statHits_System                  ( statName + "-Hits:System")
    , statHotHits                      ( statName + "-HotHits")
    , statPrefetchAddressLists         ( statName + "-PrefetchAddressLists")
    , statIgnoredAddressLists          ( statName + "-IgnoredAddressLists")
    , statIgnoredAddressLists_Contained( statName + "-IgnoredAddressLists:Contained")
    , statIgnoredAddressLists_Extends  ( statName + "-IgnoredAddressLists:Extends")
    , statIntersectingAddressLists     ( statName + "-IntersectingAddressLists")
    , statInitiatedAddresses           ( statName + "-InitatedAddresses")
    , statRedundantAddresses           ( statName + "-RedundantAddresses")
    , statWrappedStreams               ( statName + "-WrappedStreams")
    , statNoQueueClaimable             ( statName + "-NoQueueClaimable")
    , statWatchPresent                 ( statName + "-Watch:Present")
    , statWatchRequested               ( statName + "-Watch:Requested")
    , statWatchRedundant               ( statName + "-Watch:Redundant")
    , statWatchRemoved                 ( statName + "-Watch:Removed")
    , statWatchReplaced                ( statName + "-Watch:Replaced")
    , statHitsByLength                 ( statName + "-HitsByLength")
    , statLengths                      ( statName + "-Lengths")
    , statPrefetchLatency              ( statName + "-PrefetchLatency")
    , statPrefetchLatencySum           ( statName + "-PrefetchLatencySum")
    , statLifetimeToHit                ( statName + "-Lifetime:Hit")
    , statLifetimeToDiscard            ( statName + "-Lifetime:Discard")
    , statHitInterarrivals             ( statName + "-HitInterarrivals")
    , statHitInterarrivalSum           ( statName + "-HitInterarrivalSum")
    , statOrphan_CompletedPrefetches   ( statName + "-Prefetches:Completed[orphan]")
    , statOrphan_RejectedPrefetches    ( statName + "-Prefetches:Rejected[orphan]")
    , statOrphan_DiscardedLines        ( statName + "-Discards[orphan]")
    , statOrphan_ReplacedLines         ( statName + "-Discards:Replaced[orphan]")
    , statOrphan_InvalidatedLines      ( statName + "-Discards:Invalidated[orphan]")
    , statOrphan_Hits                  ( statName + "-Hits[orphan]")
    , statKilledFromRedundancy         ( statName + "-KilledFromRedundancy")
  { }
};

struct by_order {};
struct by_address {};
struct by_queue {};
struct by_replacement {};
struct by_source {};
struct by_watch {};

struct PrefetchEntry {
  MemoryAddress theAddress;
  int32_t theQueue;
  int64_t theInsertTime;

  PrefetchEntry( MemoryAddress anAddress, int32_t aQueue)
    : theAddress(anAddress)
    , theQueue(aQueue)
    , theInsertTime( theFlexus->cycleCount() )
  { }
};

typedef multi_index_container
< PrefetchEntry
, indexed_by
< ordered_unique
< tag< by_address >
, member< PrefetchEntry, MemoryAddress, &PrefetchEntry::theAddress >
>
, ordered_non_unique
< tag< by_queue >
, member< PrefetchEntry, int, &PrefetchEntry::theQueue>
>
>
>
PrefetchBuffer;

struct PrefetchQueueEntry {
  MemoryAddress theAddress;
  uint64_t theInsertTime;
  PrefetchQueueEntry(MemoryAddress anAddress, uint64_t anInsertTime)
    : theAddress(anAddress)
    , theInsertTime(anInsertTime)
  {}
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & theAddress;
  }
  PrefetchQueueEntry()
    : theInsertTime(0)
  {} //For serialization
};
typedef std::list< PrefetchQueueEntry > pb_queue_t;

struct PrefetchQueue {
  int32_t theId; //Matches offset in thePrefetchQueues
  uint64_t theSequenceNumber; //Tracks the total number of queues allocated

  pb_queue_t theQueue[2]; //The two address queues
  bool theQueueValid[2]; //Valid bits for each address queue
  int32_t theHotQueue; //Indication of which address queue is "hot"

  int64_t theTag[2]; //Source tag
  int32_t theSource[2]; //Source node
  int64_t theFirstLocation[2]; //Start location
  int64_t theNextLocation[2]; //Next location
  uint32_t theHalfListThreshold[2]; //Threshold for requesting more address bits
  bool theMoreAddressesRequested[2]; //Flag indicating if an address request is outstanding
  bool theNoMoreAddresses[2]; //Flag indicating that no more addresses may be received

  int32_t theCredits;
  int32_t theBufferedLines; //Number of lines this queue has in the PB
  uint64_t theMostRecentHit; //Timestamp of most recent hit

  bool theWatching[2];
  bool theDegenerate;

  int64_t theHits;  //Number of hits produced by this queue pair
  int64_t theCompletedPrefetches; //Prefetches completed by this pair
  int64_t theAddressLists; //Address lists received by this pair
  int64_t theRedundants;  //Number of blocks from this queue that have been
  //rejected as redundant.  A queue is killed off if this ever reaches 8.

public:

  //Friendly print function
  friend std::ostream & operator << (std::ostream & anOstream, PrefetchQueue const & aQueue) {
    anOstream <<  "#" << aQueue.theSequenceNumber << " " << aQueue.theHits << "/" << aQueue.theCompletedPrefetches << "+" << aQueue.theBufferedLines << " {";
    if (aQueue.theHotQueue > 0) {
      anOstream << "HOT" << aQueue.theHotQueue << " [" << boost::padded_string_cast < 2, '0' > (aQueue.theSource[aQueue.theHotQueue]) << "] @" << aQueue.theNextLocation[aQueue.theHotQueue] << " q: " << aQueue.theQueue[aQueue.theHotQueue].size() << "]";
    } else {
      if (aQueue.theQueueValid[0]) {
        anOstream << "[" << boost::padded_string_cast < 2, '0' > (aQueue.theSource[0]) << "] @" << aQueue.theNextLocation[0] << " q: " << aQueue.theQueue[0].size() << "]";
      } else {
        anOstream << "[invalid queue]";
      }
      anOstream << ", ";
      if (aQueue.theQueueValid[1] && aQueue.theHotQueue != 1) {
        anOstream << "[" << boost::padded_string_cast < 2, '0' > (aQueue.theSource[1]) << "] @" << aQueue.theNextLocation[1] << " q: " << aQueue.theQueue[1].size() << "]";
      } else {
        anOstream << "[invalid queue]";
      }
    }
    anOstream << "}";
    return anOstream;
  }

  PrefetchQueue(int32_t anId)
    : theId(anId)
    , theSequenceNumber(0) {
    reset();
  }

public:
  int32_t id() const {
    return theId;
  }
  bool isValid() const {
    return theQueueValid[0] || theQueueValid[1];
  }
  bool isValid(int32_t aSlot) const {
    return theQueueValid[aSlot];
  }
  int32_t bufferedLines() const {
    return theBufferedLines;
  }
  int32_t credits() const {
    if (isValid()) {
      return theCredits;
    } else {
      return 0;
    }
  }
  int32_t value() const {
    if (isValid()) {
      int32_t score = theCredits + theBufferedLines;
      if (isWatching(0)) {
        score++;
      }
      if (isWatching(1)) {
        score++;
      }
      if (isDegenerate()) {
        score++;
      }
      if (isJoining()) {
        score += 2;
      }
      return score;
    }
    return 0;
  }
  uint64_t mostRecentHit() const {
    return theMostRecentHit;
  }

  unsigned source(int32_t aSlot) const {
    return theSource[aSlot];
  }
  int64_t tag(int32_t aSlot) const {
    return theTag[aSlot];
  }
  int64_t location(int32_t aSlot) const {
    return theNextLocation[aSlot];
  }
  int64_t startLocation(int32_t aSlot) const {
    return theFirstLocation[aSlot];
  }
  MemoryAddress head(int32_t aSlot) const {
    DBG_Assert( theQueueValid[aSlot] && !theQueue[aSlot].empty());
    return theQueue[aSlot].front().theAddress;
  }
  void popHead(int32_t aSlot) {
    DBG_Assert( theQueueValid[aSlot] && !theQueue[aSlot].empty());
    theQueue[aSlot].pop_front();
  }
  void setWatching(int32_t aSlot, bool aFlag) {
    theWatching[aSlot] = aFlag;
  }
  bool isWatching(int32_t aSlot) const {
    if (theQueueValid[aSlot]) {
      return theWatching[aSlot];
    } else {
      return false ;
    }
  }

  bool isWatching(int32_t aSlot, MemoryAddress anAddress) const {
    if ( theQueueValid[aSlot] && theWatching[aSlot] ) {
      if ( theQueue[aSlot].size() == 0) {
        return false;
      }
      return theQueue[aSlot].front().theAddress == anAddress;
    }
    return false;
  }
  void hot(int32_t aSlot) {
    DBG_Assert(theQueueValid[aSlot]);
    theWatching[0] = false;
    theWatching[1] = false;
    theHotQueue = aSlot;
  }

  void changeCredit( int32_t aCreditChange ) {
    theCredits += aCreditChange;
  }

  bool wantsMoreAddresses(int32_t aSlot, uint32_t aThreshold) const {
    if (    theQueueValid[aSlot]
            && (theHotQueue == -1 || theHotQueue == aSlot)
            && !theNoMoreAddresses[aSlot]
            && !theMoreAddressesRequested[aSlot]
            && ( theQueue[aSlot].size() < aThreshold && theQueue[aSlot].size() < theHalfListThreshold[aSlot])
       ) {
      return true;
    }
    return false;
  }

  void reset() {
    for (int32_t i = 0; i < 2; ++i) {
      theQueue[i].clear();
      theQueueValid[i] = false;
      theTag[i] = -2;
      theSource[i] = -1;
      theFirstLocation[i] = -1;
      theNextLocation[i] = -1;
      theHalfListThreshold[i] = 0;
      theMoreAddressesRequested[i] = false;
      theNoMoreAddresses[i] = false;
      theWatching[i] = false;
    }
    theDegenerate = false;
    theHotQueue = -1;
    theBufferedLines = 0;
    theMostRecentHit = theFlexus->cycleCount();
    theCredits = 0;
    theHits = 0;
    theAddressLists = 0;
    theCompletedPrefetches = 0;
    theRedundants = 0;
  }

  void finalize( PLStats & theStats ) {
    theStats.statHitsByLength << std::make_pair(static_cast<int64_t>(theHits), static_cast<int64_t>(theHits));
    theStats.statLengths << theHits;

    if (theRedundants >= 8) {
      ++theStats.statKilledFromRedundancy;
    }
  }

  void completePrefetch( PLStats & theStats ) {
    ++theCompletedPrefetches;

    ++theStats.statCompletedPrefetches;
  }

  bool rejectPrefetch( PLStats & theStats ) {
    --theBufferedLines;

    ++theStats.statRejectedPrefetches;

    ++theRedundants;
    if (theRedundants >= 8) {
      return true;
    }
    return false;

  }

  void removeLine( PLStats & theStats, bool isReplacement ) {
    --theBufferedLines;

    ++theStats.statDiscardedLines;

    if (isReplacement) {
      ++theStats.statReplacedLines;
    } else {
      ++theStats.statInvalidatedLines;
    }
  }

  bool redundantAddress( PLStats & theStats) {
    ++theStats.statRedundantAddresses;
    ++theRedundants;
    if (theRedundants >= 8) {
      return true;
    }
    return false;
  }

  void recordHit( PLStats & theStats, bool isSystem) {
    --theBufferedLines;

    if (theHits > 0) {
      theStats.statHitInterarrivals << theFlexus->cycleCount() - theMostRecentHit;
      theStats.statHitInterarrivalSum += theFlexus->cycleCount() - theMostRecentHit;
    }
    ++theHits;

    theMostRecentHit = theFlexus->cycleCount();

    ++theStats.statHits;
    if (isSystem) {
      ++theStats.statHits_System;
    } else {
      ++theStats.statHits_User;
    }

    if (theHotQueue != -1) {
      ++theStats.statHotHits;

    }
  }

  void initiatePrefetch( PLStats & theStats) {
    ++theBufferedLines;

    ++theStats.statInitiatedAddresses;
  }

  //returns true if the prefetch command should request a new queue
  bool extend(PLStats & theStats, PrefetchCommand & cmd) {
    int32_t slot = -1;
    if (theQueueValid[0] && cmd.tag() == theTag[0]) {
      slot = 0;
    } else if (theQueueValid[1] && cmd.tag() == theTag[1]) {
      slot = 1;
    }

    if (slot == -1) {
      return true; //The address list is not for this queue.  Have it allocate a new one
    }

    if (theNoMoreAddresses[slot]) {
      ++theStats.statIgnoredAddressLists;
      return false; //Drop the address list on the floor.
    }

    ++theStats.statPrefetchAddressLists;
    ++theAddressLists;

    std::list<MemoryAddress>::iterator iter = cmd.addressList().begin();
    std::list<MemoryAddress>::iterator end = cmd.addressList().end();
    while (iter != end) {
      theQueue[slot].push_back( PrefetchQueueEntry( *iter, theFlexus->cycleCount() ) );
      ++iter;
    }
    theMoreAddressesRequested[slot] = false;
    theHalfListThreshold[slot] = theQueue[slot].size() - 1;
    theNextLocation[slot] = cmd.location();
    if (theFirstLocation[slot] == -1) {
      //On the first forwarded list, record the starting location of the queue
      theFirstLocation[slot] = cmd.location() - cmd.addressList().size();
    }
    return false; //extend complete
  }

  bool isJoining() const {
    return ( theQueueValid[0] != theQueueValid[1] ) && (theTag[0] > 0 || theTag[1] > 0 ) && !theDegenerate;
  }

  bool join(PLStats & theStats, PrefetchCommand & cmd) {
    if (!isValid()) {
      return false;
    }
    if (theQueueValid[0] && theQueueValid[1]) {
      return false;
    }
    if (cmd.tag() == -1) {
      return false;
    }
    int32_t requested_slot = (cmd.tag() >> 4) & 1;
    if (theQueueValid[requested_slot]) {
      return false;
    }
    int32_t existing_slot = 0;
    if (requested_slot == 0) {
      existing_slot = 1;
    }
    if (!theQueueValid[existing_slot]) {
      return false;
    }

    if ( ( cmd.tag() & ~16) != ( tag(existing_slot) & ~16) ) {
      return false;
    }

    //This list does join this queue!
    theQueueValid[requested_slot] = true;
    theTag[requested_slot] = cmd.tag();
    theSource[requested_slot] = cmd.source();

    extend( theStats, cmd);

    return true;
  }

  void allocate(PLStats & theStats, PrefetchCommand & cmd, uint64_t aQueueSequence) {
    theSequenceNumber = aQueueSequence;
    int32_t requested_slot = 0;
    if (cmd.tag() != -1) {
      requested_slot = (cmd.tag() >> 4) & 1;
    } else {
      theDegenerate = true;
    }

    theQueueValid[requested_slot] = true;
    theTag[requested_slot] = cmd.tag();
    theSource[requested_slot] = cmd.source();

    extend(theStats, cmd);
  }

  bool isDegenerate() const {
    return theDegenerate && theQueueValid[0] && !theQueue[0].empty() && !theWatching[0];
  }

  bool isDivergent() const {
    if (theHotQueue == -1 && !theWatching[0] && !theWatching[1] ) {
      if (theQueueValid[0] && theQueueValid[1]) {
        if (!theQueue[0].empty() && !theQueue[1].empty() ) {
          return theQueue[0].front().theAddress != theQueue[1].front().theAddress;
        }
      }
    }
    return false;
  }

  bool canPrefetch() const {
    if (credits() > 0) {
      if (theHotQueue == -1) {
        if (theQueueValid[0] && theQueueValid[1]) {
          if (!theQueue[0].empty() && !theQueue[1].empty() ) {
            return theQueue[0].front().theAddress == theQueue[1].front().theAddress;
          }
        }
      } else {
        DBG_Assert(theQueueValid[theHotQueue]);
        return !theQueue[theHotQueue].empty();
      }
    }
    return false;
  }

  MemoryAddress nextAddress( PLStats & theStats ) {
    DBG_Assert( canPrefetch() );

    if (theHotQueue == -1) {
      MemoryAddress ret_val = theQueue[0].front().theAddress;
      theQueue[0].pop_front();
      DBG_Assert( theQueue[1].front().theAddress == ret_val );
      theQueue[1].pop_front();
      return ret_val;
    } else {
      MemoryAddress ret_val = theQueue[theHotQueue].front().theAddress;
      theQueue[theHotQueue].pop_front();
      return ret_val;
    }
  }

  void addressListRequested(int32_t aSlot) {
    theMoreAddressesRequested[aSlot] = true;
  }
};

static const int32_t kPrefetchListener = 1;
static const int32_t kConsort = 2;
static const int32_t kMRCReflector = 3;

class FLEXUS_COMPONENT(PrefetchListener) {
  FLEXUS_COMPONENT_IMPL(PrefetchListener);

  std::list<PrefetchTransport> theBufferMessagesIn;
  std::list<PrefetchTransport> theBufferMessagesOut;
  std::list<NetworkTransport> theNetworkMessagesIn;
  std::list<NetworkTransport> theNetworkMessagesOut;

  PrefetchBuffer theBufferContents;
  PrefetchBuffer thePendingPrefetches;

  std::vector<PrefetchQueue> thePrefetchQueues;
  uint64_t theQueueSequence;

  int32_t theNextPrefetchQueue;
  int32_t theTotalCredit;

  PLStats theStats;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(PrefetchListener)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theStats(statName())
  { }

  bool isQuiesced() const {
    return  theBufferMessagesIn.empty()
            &&    theBufferMessagesOut.empty()
            &&    theNetworkMessagesIn.empty()
            &&    theNetworkMessagesOut.empty()
            &&    thePendingPrefetches.empty()
            ;
  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) {  }

  // Initialization
  void initialize() {
    //Create thePrefetchQueues

    //Set up LRU stack for thePrefetchQueues
    for (uint32_t i = 0; i < cfg.NumQueues; ++i) {
      thePrefetchQueues.push_back( PrefetchQueue(i) );
    }

    theQueueSequence = 0;
    theNextPrefetchQueue = 0;
    theTotalCredit = 0;
    theCheckQueues = true;
  }

  // Ports
  FLEXUS_PORT_ALWAYS_AVAILABLE(FromPrefetchBuffer);
  void push( interface::FromPrefetchBuffer const &, PrefetchTransport & aMessage) {
    DBG_(Trace, Comp(*this) ( << "Received FromPrefetchBuffer: " << *(aMessage[PrefetchMessageTag]) ) Addr(aMessage[PrefetchMessageTag]->address()) );
    enqueue(aMessage);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromNic);
  void push( interface::FromNic const &, index_t aVC, NetworkTransport & aMessage) {
    DBG_(Trace, Comp(*this) ( << "Received FromNic: "  << *(aMessage[PrefetchCommandTag]) ) );
    enqueue(aMessage);
  }

  // Drive Interfaces
  void drive(interface::PrefetchListenerDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "PrefetchListenerDrive" ) ) ;
    process();
  }

private:

  void enqueue(PrefetchTransport & aMessage) {
    theBufferMessagesIn.push_back(aMessage);
  }

  void enqueue(NetworkTransport & aMessage) {
    theNetworkMessagesIn.push_back(aMessage);
  }

  bool theCheckQueues;

  void process() {
    FLEXUS_PROFILE();
    //First, process messages arriving from the PrefetchBuffer indicating
    //actions it took last cycle
    processPrefetchBufferReplies();

    //Start new prefetches if we are allowed more concurrent prefetches
    issuePrefetches();

    //See if there is any other cleanup to do
    if (theCheckQueues) {
      checkQueues();
    }

    //Next, accept new messages from the Network that indicate that new prefetches
    //should be enqueued
    processNetworkCommands();

    //Send outgoing messages to the prefetch buffer and network
    sendMessages();

  }

  void processPrefetchBufferReplies() {
    FLEXUS_PROFILE();
    while ( ! theBufferMessagesIn.empty()) {
      boost::intrusive_ptr<PrefetchMessage> msg = theBufferMessagesIn.front()[PrefetchMessageTag];
      DBG_Assert(msg);
      DBG_(VVerb, Comp(*this) ( << "Processing PrefetchBuffer reply: "  << *msg) Addr(msg->address() ) );
      switch (msg->type()) {
        case PrefetchMessage::PrefetchComplete:
          //Move the prefetch from theInProcessPrefetches to the theBufferContents
          completePrefetch(msg->address());
          break;
        case PrefetchMessage::PrefetchRedundant:
          //Discard the prefetch from theInProcessPrefetches
          cancelPrefetch(msg->address());
          break;

        case PrefetchMessage::LineHit:
          //Remove the address from theBufferContents and send a hit notification
          recordHit(msg->address(), theBufferMessagesIn.front()[TransactionTrackerTag]);
          break;

        case PrefetchMessage::LineReplaced:
          //Remove the address from theBufferContents
          removeLine(msg->address(), true);
          break;

        case PrefetchMessage::LineRemoved:
          //Remove the address from theBufferContents
          removeLine(msg->address(), false);
          break;

        case PrefetchMessage::WatchPresent:
        case PrefetchMessage::WatchRequested:
          watchRequested(msg->address());
          break;

        case PrefetchMessage::WatchRedundant:
        case PrefetchMessage::WatchRemoved:
        case PrefetchMessage::WatchReplaced:
          watchKill(msg->address(), msg->type());
          break;

        default:
          DBG_Assert(false, ( << "PrefetchListener received illegal PrefetchMessage: " << *msg ) );
      }
      theBufferMessagesIn.pop_front();
    }
  }

  void completePrefetch(MemoryAddress const & anAddress) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Completing prefetch to : "  << & std::hex << anAddress << & std::dec) Addr(anAddress) );
    PrefetchBuffer::iterator iter = thePendingPrefetches.find(anAddress);
    DBG_Assert( iter != thePendingPrefetches.end(), Comp(*this) ( << "Completed a prefetch for @" << anAddress << " but no prefetch was InProcess"));

    int32_t queue = iter->theQueue;
    if (queue != -1) {
      DBG_Assert( static_cast<unsigned>(queue) < thePrefetchQueues.size() );
      PrefetchQueue & q = thePrefetchQueues[queue];
      DBG_(Trace, ( << statName() << " Prefetched " << anAddress << " " << q ) );
      //Inform the queue about this completion
      q.completePrefetch(theStats);
    } else {
      //This prefetch has been orphaned.  Count it in the orphan stats.
      DBG_(Trace, ( << statName() << " Prefetched " << anAddress << " (orphaned) " ) );
      ++theStats.statCompletedPrefetches;
      ++theStats.statOrphan_CompletedPrefetches;
    }
    if (iter->theInsertTime != 0) {
      theStats.statPrefetchLatency << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
      theStats.statPrefetchLatencySum += Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
    }

    //Move the block to theBuffer from thePending
    theBufferContents.insert( PrefetchEntry( iter->theAddress, iter->theQueue ) );
    thePendingPrefetches.erase(iter);
  }

  void cancelPrefetch(MemoryAddress const & anAddress) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Cancelling prefetch to : "  << & std::hex << anAddress << & std::dec) Addr(anAddress) );

    PrefetchBuffer::iterator iter = thePendingPrefetches.find(anAddress);
    DBG_Assert( iter != thePendingPrefetches.end(), Comp(*this) ( << "Attempted to cancel a prefetch for @" << anAddress << " but no prefetch was Pending"));

    int32_t queue = iter->theQueue;
    if (queue != -1 ) {
      DBG_Assert( static_cast<unsigned>(queue) < thePrefetchQueues.size() );
      PrefetchQueue & q = thePrefetchQueues[queue];

      DBG_(Trace, ( << statName() << " Rejected " << anAddress << " " << q ) );
      //Inform the queue about this completion
      if ( q.rejectPrefetch(theStats) ) {
        //Too many rejected prefetches.  Kill this queue
        finalizeQueue(q);
      }
    } else {
      DBG_(Trace, ( << statName() << " Rejected " << anAddress << " (orphaned)" ) );
      ++theStats.statRejectedPrefetches;
      ++theStats.statOrphan_RejectedPrefetches;
    }

    thePendingPrefetches.erase(iter);
  }

  void removeLine(MemoryAddress const & anAddress, bool isReplacement) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Removing line from theBufferContents: "  << anAddress ) Addr(anAddress) );

    PrefetchBuffer::iterator iter = theBufferContents.find(anAddress);
    DBG_Assert( iter != theBufferContents.end(), Comp(*this) ( << "Attempted to drop a prefetch line for @" << & std::hex << anAddress << & std::dec << " but address was not in theBufferContents"));

    int32_t queue = iter->theQueue;
    if (queue != -1) {
      DBG_Assert( static_cast<unsigned>(queue) < thePrefetchQueues.size() );
      PrefetchQueue & q = thePrefetchQueues[queue];

      DBG_(Trace, ( << statName() << " Remove " << anAddress << " from " << q << " after " << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime << " Replacement: " << isReplacement  ) );
      q.removeLine(theStats, isReplacement);
    } else {
      DBG_(Trace, ( << statName() << " Remove " << anAddress << " (orphaned) after " << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime << " Replacement: " << isReplacement  ) );
      ++theStats.statDiscardedLines;
      ++theStats.statOrphan_DiscardedLines;

      if (isReplacement) {
        ++theStats.statReplacedLines;
        ++theStats.statOrphan_ReplacedLines;
      } else {
        ++theStats.statInvalidatedLines;
        ++theStats.statOrphan_InvalidatedLines;
      }
    }

    if (iter->theInsertTime != 0) {
      theStats.statLifetimeToDiscard << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
    }
    theBufferContents.erase(iter);
  }

  void recordHit(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aTracker) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Processing hit on address: "  << anAddress ) Addr(anAddress) );

    PrefetchBuffer::iterator iter = theBufferContents.find(anAddress);
    DBG_Assert( iter != theBufferContents.end(), Comp(*this) ( << "Attempted to process hit for @" << anAddress << " but address was not in theBufferContents"));

#ifdef FLEXUS_TRACK_PBHITS
    if (aTracker->OS() && *aTracker->OS()) {
      thePBHits_System << std::make_pair (anAddress & (~ 63LL), 1LL);
    } else {
      thePBHits_User << std::make_pair (anAddress & (~ 63LL), 1LL);
    }
#endif //FLEXUS_TRACK_PBHITS

    int32_t queue = iter->theQueue;
    if (queue != -1 ) {
      DBG_Assert( static_cast<unsigned>(queue) < thePrefetchQueues.size() );
      PrefetchQueue & q = thePrefetchQueues[queue];

      DBG_(Trace, ( << statName() << " HIT " << anAddress << " on " << q << " after " << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime  << " cycles" ) );
      q.recordHit(theStats, (aTracker->OS() && *aTracker->OS()) );
      addCredit(q);
    } else {
      DBG_(Trace, ( << statName() << " HIT " << anAddress << " (orphaned) after " << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime  << " cycles" ) );
      ++theStats.statHits;
      if (aTracker->OS() && *aTracker->OS()) {
        ++theStats.statHits_System;
      } else {
        ++theStats.statHits_User;
      }
      ++theStats.statOrphan_Hits;
    }

    if (iter->theInsertTime != 0) {
      theStats.statLifetimeToHit << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
    }

    //Remove the Hit line from the buffer
    theBufferContents.erase(iter);
  }

  void processNetworkCommands() {
    FLEXUS_PROFILE();
    while ( ! theNetworkMessagesIn.empty() ) {
      if ( cfg.ConcurrentPrefetches > 0) {

        boost::intrusive_ptr<NetworkMessage> net_msg = theNetworkMessagesIn.front()[NetworkMessageTag];
        boost::intrusive_ptr<PrefetchCommand> cmd = theNetworkMessagesIn.front()[PrefetchCommandTag];
        DBG_Assert(net_msg);
        DBG_Assert(cmd);
        DBG_Assert(cmd->type() == PrefetchCommand::ePrefetchAddressList, ( << "PrefetchListener received illegal PrefetchCommand: " << *cmd) );
        DBG_(Trace, Comp(*this) ( << "Processing prefetch command: "  << *cmd << " from " << net_msg->src) );

        bool new_queue = true;
        if (cmd->queue() != -1 ) {
          DBG_Assert( static_cast<unsigned>( cmd->queue() ) < thePrefetchQueues.size() );
          PrefetchQueue & q = thePrefetchQueues[cmd->queue()];
          new_queue = q.extend(theStats, *cmd);
        }
        if (new_queue) {
          //See if this is the second half of an already-allocated queue
          new_queue = join(*cmd );
        }

        if (new_queue) {
          //Although this address list is ostensibly supposed to create a new
          //queue, see if it intersects any existing queue
          clipAndAllocate( *cmd );
        }

      }
      theNetworkMessagesIn.pop_front();
    }
  }

  void finalizeQueue( PrefetchQueue & q) {
    FLEXUS_PROFILE();
    DBG_(Trace, ( << statName() << " finalizing " << q ) );

    if (q.credits() > 0) {
      removeCredit(q, q.credits());
    }

    q.finalize(theStats);
    q.reset();

    //Orphan any prefetch-buffer entries associated with this queue
    PrefetchBuffer::index<by_queue>::type::iterator iter, temp, end;
    std::tie(iter, end) = theBufferContents.get<by_queue>().equal_range(q.id());
    while (iter != end) {
      temp = iter;
      ++iter;
      theBufferContents.get<by_queue>().modify( temp, ll::bind( &PrefetchEntry::theQueue, ll::_1) = -1);
    }

    std::tie(iter, end) = thePendingPrefetches.get<by_queue>().equal_range(q.id());
    while (iter != end) {
      temp = iter;
      ++iter;
      thePendingPrefetches.get<by_queue>().modify( temp, ll::bind( &PrefetchEntry::theQueue, ll::_1) = -1);
    }
  }

  void removeCredit(PrefetchQueue & q, int32_t credit = 1) {
    q.changeCredit( -1 * credit);
    theTotalCredit += ( -1 * credit);
  }

  void addCredit(PrefetchQueue & q) {
    q.changeCredit( cfg.TargetBlocks );
    theTotalCredit += ( cfg.TargetBlocks );
  }

  void prefetch( MemoryAddress anAddress, int32_t aQueue) {
    FLEXUS_PROFILE();
    //Queue a new prefetch command
    boost::intrusive_ptr<PrefetchMessage> prefetch (new PrefetchMessage(PrefetchMessage::PrefetchReq, anAddress));
    PrefetchTransport transport;
    transport.set(PrefetchMessageTag, prefetch);
    theBufferMessagesOut.push_back(transport);

    //Record that the prefetch is pending
    thePendingPrefetches.insert( PrefetchEntry( anAddress, aQueue ) );

    //theSVB_JMY.prefetch( anAddress, flexusIndex() );
  }

  void requestAddressList(PrefetchQueue & q, int32_t aSlot ) {
    FLEXUS_PROFILE();
    q.addressListRequested(aSlot);

    boost::intrusive_ptr<PrefetchCommand> prefetch_cmd(new PrefetchCommand(PrefetchCommand::ePrefetchRequestMoreAddresses));
    prefetch_cmd->queue() = q.id();
    prefetch_cmd->source() = q.source(aSlot);
    prefetch_cmd->location() = q.location(aSlot);
    prefetch_cmd->tag() = q.tag(aSlot);

    // create the network routing piece
    boost::intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
    net_msg->src = flexusIndex();
    net_msg->dest = q.source(aSlot);
    net_msg->src_port = kPrefetchListener;
    net_msg->dst_port = kConsort;
    net_msg->vc = 0;
    net_msg->size = 0; //Control message

    // now make the transport and queue it
    NetworkTransport transport;
    transport.set(PrefetchCommandTag, prefetch_cmd);
    transport.set(NetworkMessageTag, net_msg);
    theNetworkMessagesOut.push_back(transport);
  }

  void checkQueues() {
    FLEXUS_PROFILE();
    //Now check all queues to see if they need to request more addresses
    for (uint32_t i = 0; i < thePrefetchQueues.size(); ++i) {
      PrefetchQueue & q = thePrefetchQueues[i];

      if (q.isValid() ) {
        //See if this queue wants more addresses
        for (int32_t slot = 0; slot < 2; ++slot) {
          if (q.wantsMoreAddresses( slot, cfg.MoreAddressThreshold )) {
            requestAddressList(q, slot);
          }
        }

        startWatch(q);
      }
    }
    theCheckQueues = false; //Don't check again till something changes
  }

  void issuePrefetches() {
    FLEXUS_PROFILE();
    if (theTotalCredit == 0 || (thePendingPrefetches.size() >= static_cast<size_t>(cfg.ConcurrentPrefetches)) ) {
      return; //Nothing to do
    }

    int32_t queue = theNextPrefetchQueue;
    ++theNextPrefetchQueue;
    theNextPrefetchQueue %= thePrefetchQueues.size();
    while (theTotalCredit > 0 && (thePendingPrefetches.size() < static_cast<size_t>(cfg.ConcurrentPrefetches))) {
      PrefetchQueue & q = thePrefetchQueues[queue];
      if (q.credits() > 0) {
        //Attempt one prefetch from queue
        removeCredit(q);

        if (q.bufferedLines() < cfg.TargetBlocks && q.canPrefetch()) {
          theCheckQueues = true;
          MemoryAddress addr = q.nextAddress(theStats);

          //Check if the address is redundant
          PrefetchBuffer::iterator buf_contents = theBufferContents.find(addr);
          PrefetchBuffer::iterator pending_contents = thePendingPrefetches.find(addr);
          if ( buf_contents  != theBufferContents.end() ||  pending_contents != thePendingPrefetches.end() ) {
            //Address is redundant
            DBG_(Trace, ( << statName() << " Redundant " << addr << " " << q ) );
            if ( q.redundantAddress(theStats) ) {
              //This queue has produced too many redundant addresses.  We kill
              //it off
              finalizeQueue(q);
            }

          }  else {
            q.initiatePrefetch(theStats);
            DBG_(Trace, ( << statName() << " Issued " << addr << " " << q ) );

            prefetch(addr, queue);
          }
        }
      }

      ++queue;
      queue %= thePrefetchQueues.size();
    }
  }

  void sendMessages() {
    FLEXUS_PROFILE();

    //Send to PrefetchBuffer
    while ( FLEXUS_CHANNEL(ToPrefetchBuffer).available() && !theBufferMessagesOut.empty()) {
      DBG_(Trace, Comp(*this) ( << "Sending ToPrefetchBuffer: "  << *(theBufferMessagesOut.front()[PrefetchMessageTag]) ));
      FLEXUS_CHANNEL(ToPrefetchBuffer) << theBufferMessagesOut.front();
      theBufferMessagesOut.pop_front();
    }

    //Send to Network
    while ( FLEXUS_CHANNEL(ToNic).available() && !theNetworkMessagesOut.empty()) {
      DBG_(Trace, Comp(*this) ( << "Sending ToNetwork: "  << *(theNetworkMessagesOut.front()[PrefetchCommandTag]) ));
      FLEXUS_CHANNEL(ToNic) << theNetworkMessagesOut.front();
      theNetworkMessagesOut.pop_front();
    }
  }

  bool join( PrefetchCommand & cmd ) {
    FLEXUS_PROFILE();
    if (cmd.tag() == -1) {
      return true;
    }

    for (uint32_t i = 0; i < thePrefetchQueues.size(); ++i) {
      if (thePrefetchQueues[i].join(theStats, cmd)) {
        addCredit(thePrefetchQueues[i]);
        return false;
      }
    }
    return true;
  }

  void clipAndAllocate( PrefetchCommand & cmd ) {
    FLEXUS_PROFILE();
    //Search through all the prefetch queues to see if this cmd should simply be
    //ignored, or should merge with / kill one of them

    if (cmd.addressList().size() == 0) {
      return;  //Don't allocate an empty list
    }

    int64_t new_end = cmd.location();
    int64_t new_start = cmd.location() - cmd.addressList().size();

    //Walk over all the queues, clipping this new command against each queue.
    for (uint32_t i = 0; i < thePrefetchQueues.size(); ++i) {
      PrefetchQueue & q = thePrefetchQueues[i];
      if (! q.isValid()) {
        continue; //No need to clip against invalid queues;
      }

      for (int32_t slot = 0; slot < 2; ++slot) {
        if (!q.isValid(slot) ) {
          continue;
        }
        if (q.source(slot) != cmd.source()) {
          continue; //Source mismatch, no need to clip
        }

        int64_t existing_start = q.startLocation(slot);
        int64_t existing_end = q.location(slot);

        if (existing_end < existing_start) {
          //The exceedingly rare case that the stream we compare against is wrapped
          ++theStats.statWrappedStreams;
          //Don't do the rest of these comparisons, as they assume that streams
          //aren't wrapped
          continue;
        }
        if (new_start >= existing_start && new_start < existing_end) {
          //Intersection.  Don't allocate
          ++theStats.statIgnoredAddressLists;
          ++theStats.statIntersectingAddressLists;
          return;
        }
        if (new_end > existing_start && new_end <= existing_end) {
          //Intersection.  Don't allocate
          ++theStats.statIgnoredAddressLists;
          ++theStats.statIntersectingAddressLists;
          return;
        }
        if ( new_start <= existing_start && new_end >= existing_end) {
          //Containment.  Don't allocatte
          ++theStats.statIgnoredAddressLists;
          ++theStats.statIntersectingAddressLists;
          return;
        }
        if ( new_start >= existing_start && new_end <= existing_end) {
          //Containment.  Don't allocatte
          ++theStats.statIgnoredAddressLists;
          ++theStats.statIntersectingAddressLists;
          return;
        }
      }
    }

    allocate(cmd);
  }

  int32_t selectVictim() {
    FLEXUS_PROFILE();
    int32_t victim = 0;
    for (uint32_t i = 1; i < thePrefetchQueues.size(); ++i) {
      if (thePrefetchQueues[victim].value() == 0) {
        break;  //No need to search further if we found a valueless queue
      }
      if (thePrefetchQueues[i].value() < thePrefetchQueues[victim].value()) {
        victim = i;
      } else if (thePrefetchQueues[i].value() == thePrefetchQueues[victim].value()) {
        if (thePrefetchQueues[i].mostRecentHit() < thePrefetchQueues[victim].mostRecentHit()) {
          victim = i;
        }
      }
    }
    DBG_Assert( static_cast<unsigned>(victim) < thePrefetchQueues.size() );
    return victim;
  }

  void allocate(PrefetchCommand & cmd ) {
    FLEXUS_PROFILE();
    int32_t queue = selectVictim();
    PrefetchQueue & q = thePrefetchQueues[queue];

    if (q.isValid()) {
      finalizeQueue(q);
    }

    ++theQueueSequence;

    theCheckQueues = true;
    q.allocate(theStats, cmd, theQueueSequence);
  }

  void watchRequested( MemoryAddress anAddress ) {
    FLEXUS_PROFILE();
    //We hit on a watch.  Let's walk through the queues and make any of them
    //that match hot
    for (uint32_t i = 0; i < thePrefetchQueues.size(); ++i) {
      for (int32_t slot = 0; slot < 2; ++slot) {
        if (thePrefetchQueues[i].isWatching(slot, anAddress)) {
          thePrefetchQueues[i].popHead(slot);
          theCheckQueues = true;
          thePrefetchQueues[i].hot(slot);
          addCredit(thePrefetchQueues[i]);
          DBG_(Trace, ( << statName() << " Watch-Present-or-Requested " << anAddress << " Hot-Queue " << thePrefetchQueues[i] ) );
          break;
        }
      }
    }
  }

  void watchKill( MemoryAddress anAddress, PrefetchMessage::PrefetchMessageType aReason ) {
    FLEXUS_PROFILE();
    for (uint32_t i = 0; i < thePrefetchQueues.size(); ++i) {
      for (int32_t slot = 0; slot < 2; ++slot) {
        if (thePrefetchQueues[i].isWatching(slot, anAddress)) {

          switch ( aReason) {
            case PrefetchMessage::WatchRedundant:
              DBG_(Trace, ( << statName() << " Watch-Redundant " << anAddress << " " << thePrefetchQueues[i] ) );
              ++theStats.statWatchRedundant;
              break;
            case PrefetchMessage::WatchRemoved:
              DBG_(Trace, ( << statName() << " Watch-Removed" << anAddress << " " << thePrefetchQueues[i] ) );
              ++theStats.statWatchRemoved;
              break;
            case PrefetchMessage::WatchReplaced:
              DBG_(Trace, ( << statName() << " Watch-Replaced" << anAddress << " " << thePrefetchQueues[i] ) );
              ++theStats.statWatchReplaced;
              break;
            default:
              DBG_Assert(false);
              break;
          };

          //Pop the non-requested block, and set watching to false.  We will
          //be divergent again and start watching the next block.
          thePrefetchQueues[i].popHead(slot);
          thePrefetchQueues[i].setWatching(slot, false);
          theCheckQueues = true;

          if ( thePrefetchQueues[i].redundantAddress(theStats) ) {
            //This queue has produced too many redundant addresses.  We kill
            //it off
            finalizeQueue(thePrefetchQueues[i]);
          }

        }
      }
    }
  }

  void startWatch( PrefetchQueue & q ) {
    FLEXUS_PROFILE();
    DBG_Assert( q.isValid() );

    while ( q.isDivergent() ) {
      //The two queues are divergent. We want to start watching the heads.

      // See if either of their heads is already
      //in the prefetch buffer
      bool redundant = false;
      for (int32_t i = 0; i < 2; ++i) {
        if (!q.isWatching(i)) {
          PrefetchBuffer::iterator buf_contents = theBufferContents.find(q.head(i));
          PrefetchBuffer::iterator pending_contents = thePendingPrefetches.find(q.head(i));
          //Check if the address is redundant
          if ( buf_contents  != theBufferContents.end() ||  pending_contents != thePendingPrefetches.end() ) {
            DBG_(Trace, ( << statName() << " Watch-Redundant " << q.head(i) << " " << q ) );
            q.popHead(i);
            redundant = true;
            if ( q.redundantAddress(theStats) ) {
              //This queue has produced too many redundant addresses.  We kill
              //it off
              finalizeQueue(q);
              return; //Don't continue processing as queue is now dead
            }
          }
        }
      }
      if (redundant) {
        //We advanced a block.  Go back to the top of the loop and see if
        //we are still divergent.
        continue;
      }

      for (int32_t i = 0; i < 2; ++i) {
        if (!q.isWatching(i)) {
          q.setWatching(i, true);
          DBG_(Trace, ( << statName() << " Watching " << q.head(i) << " " << q ) );

          //Queue a new prefetch command
          boost::intrusive_ptr<PrefetchMessage> watch (new PrefetchMessage(PrefetchMessage::WatchReq, q.head(i)));
          PrefetchTransport transport;
          transport.set(PrefetchMessageTag, watch);
          theBufferMessagesOut.push_back(transport);
        }
      }
    }

    while ( q.isDegenerate() ) {
      PrefetchBuffer::iterator buf_contents = theBufferContents.find(q.head(0));
      PrefetchBuffer::iterator pending_contents = thePendingPrefetches.find(q.head(0));
      //Check if the address is redundant
      if ( buf_contents  != theBufferContents.end() ||  pending_contents != thePendingPrefetches.end() ) {
        DBG_(Trace, ( << statName() << " Watch-Redundant " << q.head(0) << " " << q ) );
        q.popHead(0);

        if ( q.redundantAddress(theStats) ) {
          //This queue has produced too many redundant addresses.  We kill
          //it off
          finalizeQueue(q);
          return; //Don't continue processing as queue is now dead
        }
        continue;
      }

      q.setWatching(0, true);
      DBG_(Trace, ( << statName() << " Watching " << q.head(0) << " " << q ) );

      //Queue a new prefetch command
      boost::intrusive_ptr<PrefetchMessage> watch (new PrefetchMessage(PrefetchMessage::WatchReq, q.head(0)));
      PrefetchTransport transport;
      transport.set(PrefetchMessageTag, watch);
      theBufferMessagesOut.push_back(transport);

    }
  }

};

} //End Namespace nPrefetchListener

FLEXUS_COMPONENT_INSTANTIATOR( PrefetchListener, nPrefetchListener );
FLEXUS_PORT_ARRAY_WIDTH( PrefetchListener, FromNic) {
  return cfg.VChannels;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PrefetchListener

#define DBG_Reset
#include DBG_Control()

