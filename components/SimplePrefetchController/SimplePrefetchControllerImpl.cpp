#include <components/SimplePrefetchController/SimplePrefetchController.hpp>

#define FLEXUS_BEGIN_COMPONENT SimplePrefetchController
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <ext/hash_set>
#include <unordered_map>
#include <fstream>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
using namespace boost::multi_index;

#include <core/stats.hpp>
#include <components/SimplePrefetchController/PrefetchTracker.hpp>

#define DBG_DefineCategories SimplePrefetchController
#define DBG_SetDefaultOps AddCat(SimplePrefetchController)
#include DBG_Control()

namespace __gnu_cxx {
template<> struct hash< Flexus::SharedTypes::PhysicalMemoryAddress > {
  size_t operator()(Flexus::SharedTypes::PhysicalMemoryAddress __x) const {
    return __x;
  }
};
}

namespace nSimplePrefetchController {

uint32_t log_base2(uint32_t num) {
  uint32_t ii = 0;
  while (num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

struct IntHash {
  std::size_t operator()(uint32_t key) const {
    key = key >> 6;
    return key;
  }
};

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
namespace SharedTypes = Flexus::SharedTypes;

template <class Transport>
class Queue {
  std::list< Transport > theQueue;
  int32_t theSize;
  int32_t theCurrentUsage;

public:
  Queue( )
    : theSize(1)
    , theCurrentUsage(0)
  {}
  Queue( uint32_t aSize )
    : theSize(aSize)
    , theCurrentUsage(0)
  {}

  void setSize( uint32_t aSize ) {
    theSize = aSize;
  }

  void enqueue(Transport const & aMessage) {
    theQueue.push_back( aMessage );
    ++theCurrentUsage;
  }

  Transport dequeue() {
    DBG_Assert( ! theQueue.empty() );
    Transport  ret_val(theQueue.front());
    theQueue.pop_front();
    --theCurrentUsage;
    return ret_val;
  }

  bool empty() const {
    return theQueue.empty();
  }
  bool full() const {
    return theCurrentUsage >= theSize;
  }
  int32_t size() const {
    return theSize;
  }
  int32_t usage() const {
    return theCurrentUsage;
  }

};  // class Queue

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

class ActivePrefetchFile {
  typedef std::unordered_map<MemoryAddress, std::pair<uint64_t, MemoryMessage::MemoryMessageType> > maf_t;
  maf_t theMaf;

  Stat::StatInstanceCounter<int64_t> theCyclesWithN;
  Stat::StatAverage theAverage;
  uint64_t theLastAccounting;

public:
  ActivePrefetchFile(std::string const & aStatName)
    : theCyclesWithN( aStatName + "ActivePrefetches" )
    , theAverage( aStatName + "ActivePrefetchAvg" )
    , theLastAccounting(0)
  { }

  void account() {
    //Accumulate counts since the last accounting
    int64_t time = Flexus::Core::theFlexus->cycleCount() - theLastAccounting;
    if (time > 0) {
      theCyclesWithN << std::make_pair( static_cast<int64_t>(theMaf.size()), time );
      theAverage << std::make_pair( static_cast<int64_t>(theMaf.size()), time );
      theLastAccounting = Flexus::Core::theFlexus->cycleCount();
    }
  }

  uint32_t size() const {
    return theMaf.size();
  }

  bool contains( MemoryAddress anAddress ) {
    return theMaf.count( anAddress ) > 0;
  }

  void insert( MemoryAddress anAddress ) {
    account();
    theMaf.insert( std::make_pair(anAddress, std::make_pair(0, MemoryMessage::SVBClean)) );
  }

  void remove(MemoryAddress anAddress) {
    account();
    theMaf.erase( anAddress );
  }

  bool markLate(MemoryAddress anAddress, MemoryMessage::MemoryMessageType aType) {
    maf_t::iterator iter = theMaf.find( anAddress );
    if ( iter != theMaf.end() ) {
      if ( iter->second.first == 0 ) {
        iter->second.first = Flexus::Core::theFlexus->cycleCount();
        iter->second.second = aType;
        return true;
      }
    }
    return false;
  }

  std::pair<uint64_t, MemoryMessage::MemoryMessageType> getLate(MemoryAddress anAddress) {
    maf_t::iterator iter = theMaf.find( anAddress );
    if ( iter != theMaf.end() ) {
      return iter->second;
    }
    return std::make_pair(0, MemoryMessage::SVBClean);
  }

};  // end class MissAddressFile

struct PrefetchQueue {
  mutable std::list < MemoryAddress > theAddresses;
  int64_t theId;

  PrefetchQueue(std::list < MemoryAddress > const & aList, int64_t anId)
    : theAddresses (aList)
    , theId(anId)
  {}

  bool empty() const {
    return theAddresses.empty();
  }
  int64_t id() const {
    return theId;
  }

  MemoryAddress dequeue() const {
    DBG_Assert( ! theAddresses.empty() );
    MemoryAddress ret = theAddresses.front();
    theAddresses.pop_front();
    return ret;
  }

  int32_t swapList(std::list < MemoryAddress > const & aList) const {
    //int32_t squelch_count = theAddresses.size();
    //theAddresses = aList;
    //return squelch_count;
    theAddresses.insert(theAddresses.end(), aList.begin(), aList.end());
    return 0;
  }

};

struct by_tag {};
typedef multi_index_container
< PrefetchQueue
, indexed_by
< sequenced<>
, ordered_non_unique
< tag<by_tag>
, member< PrefetchQueue, int64_t, &PrefetchQueue::theId >
>
>
>
PrefetchQueues;

class FLEXUS_COMPONENT(SimplePrefetchController)  {
  FLEXUS_COMPONENT_IMPL(SimplePrefetchController);

  Queue< MemoryTransport > qPrefetch_ToL1_Request;

  Queue< PrefetchTransport > qMasterIn;

  ActivePrefetchFile theActivePrefetches;
  nPrefetchTracker::PrefetchTracker thePrefetchTracker;

  typedef __gnu_cxx::hash_set<PhysicalMemoryAddress> dup_tags_t;
  dup_tags_t theDuplicateTags;
  dup_tags_t thePrefetchedTags_L2;
  dup_tags_t thePrefetchedTags_OffChip;
  dup_tags_t thePrefetchedTags_Coherence;
  dup_tags_t thePrefetchedTags_PeerL1;

  PrefetchQueues thePrefetchQueues;

  Stat::StatCounter statPrefetches_Total;
  Stat::StatCounter statPrefetches_Issued;
  Stat::StatCounter statPrefetches_IssuedL2;
  Stat::StatCounter statPrefetches_DuplicateL1;
  Stat::StatCounter statPrefetches_RedundantL1;
  Stat::StatCounter statPrefetches_RedundantInProgress;
  Stat::StatCounter statPrefetches_CompletedL2;
  Stat::StatCounter statPrefetches_CompletedOffChip;
  Stat::StatCounter statPrefetches_CompletedCoherence;
  Stat::StatCounter statPrefetches_CompletedPeerL1;

  Stat::StatCounter statHits;
  Stat::StatCounter statHits_Read_FillL2;
  Stat::StatCounter statHits_Read_FillOffChip;
  Stat::StatCounter statHits_Read_FillCoherence;
  Stat::StatCounter statHits_Read_FillPeerL1;
  Stat::StatCounter statHits_Write_FillL2;
  Stat::StatCounter statHits_Write_FillOffChip;
  Stat::StatCounter statHits_Write_FillCoherence;
  Stat::StatCounter statHits_Write_FillPeerL1;

  Stat::StatCounter statLateHits_Read_FillL2;
  Stat::StatCounter statLateHits_Read_FillOffChip;
  Stat::StatCounter statLateHits_Read_FillCoherence;
  Stat::StatCounter statLateHits_Read_FillPeerL1;
  Stat::StatCounter statLateHits_Write_FillL2;
  Stat::StatCounter statLateHits_Write_FillOffChip;
  Stat::StatCounter statLateHits_Write_FillCoherence;
  Stat::StatCounter statLateHits_Write_FillPeerL1;

  Stat::StatCounter statOverPredictions;
  Stat::StatCounter statOverPredictions_FillL2;
  Stat::StatCounter statOverPredictions_FillOffChip;
  Stat::StatCounter statOverPredictions_FillCoherence;
  Stat::StatCounter statOverPredictions_FillPeerL1;

  Stat::StatLog2Histogram statHits_WaitTimeL2;
  Stat::StatLog2Histogram statHits_WaitTimeOffChip;
  Stat::StatLog2Histogram statHits_WaitTimeCoherence;
  Stat::StatLog2Histogram statHits_WaitTimePeerL1;

  Stat::StatCounter statPrefetchLists;
  Stat::StatCounter statSquelches;
  Stat::StatCounter statSquelchedAddresses;

  Stat::StatInstanceCounter<int64_t> statNumQueues;

private:
  //Helper functions
  void enqueue( Queue< MemoryTransport> & aQueue, MemoryTransport const & aTransport) {
    aQueue.enqueue( aTransport );
  }

  void enqueue( Queue< PrefetchTransport> & aQueue, PrefetchTransport const & aTransport) {
    aQueue.enqueue( aTransport );
  }

  template <class Queue>
  bool isFull( Queue & aQueue) {
    return aQueue.full();
  }

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(SimplePrefetchController)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theActivePrefetches( statName() + "-")
    , thePrefetchTracker(statName())
    , statPrefetches_Total( statName() + "-Prefetches:Total" )
    , statPrefetches_Issued( statName() + "-Prefetches:Issued" )
    , statPrefetches_IssuedL2( statName() + "-Prefetches:IssuedL2" )
    , statPrefetches_DuplicateL1( statName() + "-Prefetches:Duplicate:L1" )
    , statPrefetches_RedundantL1( statName() + "-Prefetches:Redundant:L1" )
    , statPrefetches_RedundantInProgress( statName() + "-Prefetches:Redundant:InProgress" )
    , statPrefetches_CompletedL2( statName() + "-Prefetches:Completed:L2" )
    , statPrefetches_CompletedOffChip( statName() + "-Prefetches:Completed:OffChip" )
    , statPrefetches_CompletedCoherence( statName() + "-Prefetches:Completed:Coherence" )
    , statPrefetches_CompletedPeerL1( statName() + "-Prefetches:Completed:PeerL1" )
    , statHits( statName() + "-Hits" )
    , statHits_Read_FillL2( statName() + "-Hits:Read:FillL2" )
    , statHits_Read_FillOffChip( statName() + "-Hits:Read:FillOffChip" )
    , statHits_Read_FillCoherence( statName() + "-Hits:Read:FillCoherence" )
    , statHits_Read_FillPeerL1( statName() + "-Hits:Read:FillPeerL1" )
    , statHits_Write_FillL2( statName() + "-Hits:Write:FillL2" )
    , statHits_Write_FillOffChip( statName() + "-Hits:Write:FillOffChip" )
    , statHits_Write_FillCoherence( statName() + "-Hits:Write:FillCoherence" )
    , statHits_Write_FillPeerL1( statName() + "-Hits:Write:FillPeerL1" )
    , statLateHits_Read_FillL2( statName() + "-LateHits:Read:FillL2" )
    , statLateHits_Read_FillOffChip( statName() + "-LateHits:Read:FillOffChip" )
    , statLateHits_Read_FillCoherence( statName() + "-LateHits:Read:FillCoherence" )
    , statLateHits_Read_FillPeerL1( statName() + "-LateHits:Read:FillPeerL1" )
    , statLateHits_Write_FillL2( statName() + "-LateHits:Write:FillL2" )
    , statLateHits_Write_FillOffChip( statName() + "-LateHits:Write:FillOffChip" )
    , statLateHits_Write_FillCoherence( statName() + "-LateHits:Write:FillCoherence" )
    , statLateHits_Write_FillPeerL1( statName() + "-LateHits:Write:FillPeerL1" )
    , statOverPredictions( statName() + "-OverPreds" )
    , statOverPredictions_FillL2( statName() + "-OverPreds:FillL2" )
    , statOverPredictions_FillOffChip( statName() + "-OverPreds:FillOffChip" )
    , statOverPredictions_FillCoherence( statName() + "-OverPreds:FillCoherence" )
    , statOverPredictions_FillPeerL1( statName() + "-OverPreds:FillPeerL1" )
    , statHits_WaitTimeL2( statName() + "-Hits:WaitTime:L2" )
    , statHits_WaitTimeOffChip( statName() + "-Hits:WaitTime:OffChip" )
    , statHits_WaitTimeCoherence( statName() + "-Hits:WaitTime:Coherence" )
    , statHits_WaitTimePeerL1( statName() + "-Hits:WaitTime:PeerL1" )
    , statPrefetchLists( statName() + "-PrefetchLists" )
    , statSquelches( statName() + "-Squelches" )
    , statSquelchedAddresses( statName() + "-SquelchedAddresses" )
    , statNumQueues( statName() + "-NumQueues" )
  {}

  bool isQuiesced() const {
    return    qMasterIn.empty()
              &&      qPrefetch_ToL1_Request.empty()
              &&      theActivePrefetches.size() == 0
              ;
  }

  void saveState(std::string const & aDirName) {

    //For testing.  This output can be compared against what is saved by the
    //L1 cache

    std::string out( aDirName + '/' + statName() );
    std::ofstream ofs( out.c_str() );

    uint32_t num_sets = cfg.L1Size / cfg.L1BlockSize / cfg.L1Associativity;
    uint32_t set_shift = log_base2( cfg.L1BlockSize ) ;

    int32_t shift = set_shift + log_base2(  num_sets ) ;

    dup_tags_t::iterator iter = theDuplicateTags.begin();
    dup_tags_t::iterator end = theDuplicateTags.end();

    while (iter != end) {
      ofs << std::dec << ( *iter >> shift ) << std::endl;
      ++iter;
    }
    ofs.close();

  }

  bool loadTags( std::istream & s ) {
    uint32_t num_sets = cfg.L1Size / cfg.L1BlockSize / cfg.L1Associativity;
    uint32_t set_shift = log_base2( cfg.L1BlockSize ) ;
    uint32_t tag_shift = log_base2( num_sets ) ;

    char paren;
    int32_t dummy;
    int32_t load_state;
    uint64_t load_tag;
    for ( uint32_t i = 0; i < num_sets; i++ ) {
      s >> paren; // {
      if ( paren != '{' ) {
        DBG_ (Crit, ( << "Expected '{' when loading checkpoint" ) );
        return false;
      }
      for ( int32_t j = 0; j < cfg.L1Associativity; j++ ) {
        s >> paren >> load_state >> load_tag >> paren;
        theDuplicateTags.insert( MemoryAddress( (load_tag << (tag_shift + set_shift)) | (i << set_shift ) ) );
      }
      s >> paren; // }
      if ( paren != '}' ) {
        DBG_ (Crit, ( << "Expected '}' when loading checkpoint" ) );
        return false;
      }

      // useless associativity information
      s >> paren; // <
      if ( paren != '<' ) {
        DBG_ (Crit, ( << "Expected '<' when loading checkpoint" ) );
        return false;
      }
      for ( int32_t j = 0; j < cfg.L1Associativity; j++ ) {
        s >> dummy;
      }
      s >> paren; // >
      if ( paren != '>' ) {
        DBG_ (Crit, ( << "Expected '>' when loading checkpoint" ) );
        return false;
      }
    }
    return true;
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName + '/' + boost::padded_string_cast < 2, '0' > (flexusIndex()) + "-L1d" );
    std::ifstream ifs(fname.c_str());
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      ifs >> std::skipws;

      if ( ! loadTags( ifs ) ) {
        DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                      ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
      ifs.close();
    }

  }

  // Initialization
  void initialize() {
    qMasterIn.setSize( cfg.QueueSizes );
    qPrefetch_ToL1_Request.setSize( cfg.QueueSizes );
  }

  bool available(interface::Prefetch_FromCore_Request const &) {
    return ! isFull( qPrefetch_ToL1_Request );
  }
  void push(interface::Prefetch_FromCore_Request const &, MemoryTransport & aMessage) {
    DBG_Assert( aMessage[MemoryMessageTag] );
    DBG_(Iface, Comp(*this) ( << "Received on Prefetch_FromCore_Request: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );

    PhysicalMemoryAddress addr( aMessage[MemoryMessageTag]->address() & ~63);

    theActivePrefetches.markLate(addr, aMessage[MemoryMessageTag]->type());

    switch (aMessage[MemoryMessageTag]->type()) {
      case MemoryMessage::LoadReq:
        if (thePrefetchedTags_L2.count(addr) > 0) {
          ++statHits;
          ++statHits_Read_FillL2;
          statHits_WaitTimeL2 << (0);
          thePrefetchedTags_L2.erase(addr);
        }
        if (thePrefetchedTags_OffChip.count( addr ) > 0) {
          ++statHits;
          ++statHits_Read_FillOffChip;
          statHits_WaitTimeOffChip << (0);
          thePrefetchedTags_OffChip.erase(addr);
        }
        if (thePrefetchedTags_Coherence.count( addr ) > 0) {
          ++statHits;
          ++statHits_Read_FillCoherence;
          statHits_WaitTimeCoherence << (0);
          thePrefetchedTags_Coherence.erase(addr);
        }
        if (thePrefetchedTags_PeerL1.count( addr ) > 0) {
          ++statHits;
          ++statHits_Read_FillPeerL1;
          statHits_WaitTimePeerL1 << (0);
          thePrefetchedTags_PeerL1.erase(addr);
        }
        break;
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        if (thePrefetchedTags_L2.count(addr) > 0) {
          ++statHits;
          ++statHits_Write_FillL2;
          statHits_WaitTimeL2 << (0);
          thePrefetchedTags_L2.erase(addr);
        }
        if (thePrefetchedTags_OffChip.count( addr ) > 0) {
          ++statHits;
          ++statHits_Write_FillOffChip;
          statHits_WaitTimeOffChip << (0);
          thePrefetchedTags_OffChip.erase(addr);
        }
        if (thePrefetchedTags_Coherence.count( addr ) > 0) {
          ++statHits;
          ++statHits_Write_FillCoherence;
          statHits_WaitTimeCoherence << (0);
          thePrefetchedTags_Coherence.erase(addr);
        }
        if (thePrefetchedTags_PeerL1.count( addr ) > 0) {
          ++statHits;
          ++statHits_Write_FillPeerL1;
          statHits_WaitTimePeerL1 << (0);
          thePrefetchedTags_PeerL1.erase(addr);
        }
        break;
      default:
        break;
    }

    enqueue( qPrefetch_ToL1_Request, aMessage );
  }

  void drive(interface::CoreToL1Drive const &) {
    DBG_(VVerb, Comp(*this) ( << "CoreToL1_Drive" ) ) ;

    //Fill the queue with prefetches if we have some.
    issuePrefetches();

    while (!qPrefetch_ToL1_Request.empty() && FLEXUS_CHANNEL(Prefetch_ToL1_Request).available() ) {
      MemoryTransport transport = qPrefetch_ToL1_Request.dequeue();
      DBG_Assert(transport[MemoryMessageTag]);
      DBG_(Iface, Comp(*this) ( << "Sent on Port Prefetch_ToL1_Request: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(Prefetch_ToL1_Request) << transport;
    }
  }

  bool available(interface::Prefetch_FromL1_Reply const &) {
    return FLEXUS_CHANNEL(Prefetch_ToCore_Reply).available();
  }
  void push(interface::Prefetch_FromL1_Reply const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port Prefetch_FromL1_Reply: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    bool drop = processFromL1_ReplyMessage( aMessage[MemoryMessageTag], aMessage[TransactionTrackerTag] );
    if (! drop) {
      FLEXUS_CHANNEL(Prefetch_ToCore_Reply) << aMessage;
    }
  }

  // Ports
  bool available(interface::Monitor_FrontSideIn_Request const &) {
    return FLEXUS_CHANNEL(Monitor_BackSideOut_Request).available();
  }
  void push(interface::Monitor_FrontSideIn_Request const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port Monitor_FrontSideIn(Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    //Don't do anything with request messages
    if (  aMessage[TransactionTrackerTag]->originatorLevel()
          && *aMessage[TransactionTrackerTag]->originatorLevel() == eL2Prefetcher
          && aMessage[TransactionTrackerTag]->initiator()
          && *aMessage[TransactionTrackerTag]->initiator() == flexusIndex()
       ) {
      ++statPrefetches_IssuedL2;
      thePrefetchTracker.startPrefetch(aMessage[TransactionTrackerTag]);
    }
    FLEXUS_CHANNEL(Monitor_BackSideOut_Request) << aMessage;
  }

  bool available(interface::Monitor_FrontSideIn_Prefetch const &) {
    return FLEXUS_CHANNEL(Monitor_BackSideOut_Prefetch).available();
  }
  void push(interface::Monitor_FrontSideIn_Prefetch const &,  MemoryTransport & aMessage) {
    DBG_Assert( aMessage[MemoryMessageTag] );
    DBG_(Iface, Comp(*this) ( << "Received on Port Monitor_FrontSideIn(Prefetch): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    //Don't do anything with request messages
    if (  aMessage[TransactionTrackerTag]->originatorLevel()
          && *aMessage[TransactionTrackerTag]->originatorLevel() == eL2Prefetcher
          && aMessage[TransactionTrackerTag]->initiator()
          && *aMessage[TransactionTrackerTag]->initiator() == flexusIndex()
       ) {
      ++statPrefetches_IssuedL2;
      thePrefetchTracker.startPrefetch(aMessage[TransactionTrackerTag]);
    }

    FLEXUS_CHANNEL(Monitor_BackSideOut_Prefetch) << aMessage;
  }

  bool available(interface::Monitor_FrontSideIn_Snoop const &) {
    return FLEXUS_CHANNEL(Monitor_BackSideOut_Snoop).available();
  }
  void push(interface::Monitor_FrontSideIn_Snoop const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port Monitor_FrontSideIn(Snoop): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    bool drop = processSnoopMessage( aMessage[MemoryMessageTag], aMessage[TransactionTrackerTag] );
    if (! drop) {
      FLEXUS_CHANNEL(Monitor_BackSideOut_Snoop) << aMessage;
    }
  }

  bool available(interface::Monitor_BackSideIn const &) {
    return FLEXUS_CHANNEL(Monitor_FrontSideOut).available();
  }
  void push(interface::Monitor_BackSideIn const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port Monitor_BackSideIn: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    bool drop = monitorBackMessage( aMessage[MemoryMessageTag], aMessage[TransactionTrackerTag] );
    if (! drop) {
      FLEXUS_CHANNEL(Monitor_FrontSideOut) << aMessage;
    }
  }

  bool available(interface::MasterIn const &) {
    return ! isFull( qMasterIn );
  }
  void push(interface::MasterIn const &,  PrefetchTransport & aMessage) {
    DBG_Assert( aMessage[PrefetchCommandTag] );
    DBG_(Iface, Comp(*this) ( << "Received on Port MasterIn: " << *(aMessage[PrefetchCommandTag]) ) );

    DBG_Assert(aMessage[PrefetchCommandTag]->type() == PrefetchCommand::ePrefetchAddressList);
    int64_t qtag = aMessage[PrefetchCommandTag]->tag();
    /*
    if ( qtag > 0) {
      PrefetchQueues::index<by_tag>::type::iterator iter = thePrefetchQueues.get<by_tag>().find(qtag);
      if (iter != thePrefetchQueues.get<by_tag>().end()) {
        int32_t squelch_count = iter->swapList( aMessage[PrefetchCommandTag]->addressList() );
        ++statSquelches;
        statSquelchedAddresses += squelch_count;
      } else {
        thePrefetchQueues.push_front( PrefetchQueue( aMessage[PrefetchCommandTag]->addressList(), qtag ) );
      }
    } else {
      thePrefetchQueues.push_front( PrefetchQueue( aMessage[PrefetchCommandTag]->addressList(),  0 ) );
    }
    */
    thePrefetchQueues.push_front( PrefetchQueue( aMessage[PrefetchCommandTag]->addressList(),  0 ) );
    ++statPrefetchLists;
  }

private:

  //Processing code
  bool monitorBackMessage( boost::intrusive_ptr<MemoryMessage> aMessage, boost::intrusive_ptr<TransactionTracker> aTracker) {
    DBG_Assert(aTracker);
    DBG_Assert(aMessage);

    MemoryMessage::MemoryMessageType type = aMessage->type();
    if (  type == MemoryMessage::MissReply
          || type == MemoryMessage::MissReplyWritable
          || type == MemoryMessage::MissReplyDirty
          || type == MemoryMessage::PrefetchReadReply
          || type == MemoryMessage::PrefetchWritableReply
          || type == MemoryMessage::PrefetchDirtyReply
       ) {
      theDuplicateTags.insert( aMessage->address() );
    }

    if (  type == MemoryMessage::Invalidate
       ) {
      clearBlock( aMessage->address() );
    }

    return false;
  }

  //Returns true if message should be silently dropped
  bool processSnoopMessage( boost::intrusive_ptr<MemoryMessage> aMessage, boost::intrusive_ptr<TransactionTracker> aTracker) {
    DBG_Assert(aMessage);
    MemoryMessage::MemoryMessageType type = aMessage->type();
    if (  type == MemoryMessage::EvictClean
          || type == MemoryMessage::EvictWritable
          || type == MemoryMessage::EvictDirty
       ) {
      clearBlock( aMessage->address() );
    }

    if (type == MemoryMessage::EvictClean ||
        type == MemoryMessage::EvictWritable ) {
      return true;  //drop message
    } else {
      return false; //pass message
    }
  }

  void clearBlock(PhysicalMemoryAddress & addr) {
    theDuplicateTags.erase(addr);
    if (thePrefetchedTags_L2.count(addr) > 0 ) {
      ++statOverPredictions;
      ++statOverPredictions_FillL2;
      thePrefetchedTags_L2.erase(addr);
    }
    if (thePrefetchedTags_OffChip.count(addr) > 0 ) {
      ++statOverPredictions;
      ++statOverPredictions_FillOffChip;
      thePrefetchedTags_OffChip.erase(addr);
    }
    if (thePrefetchedTags_Coherence.count(addr) > 0 ) {
      ++statOverPredictions;
      ++statOverPredictions_FillCoherence;
      thePrefetchedTags_Coherence.erase(addr);
    }
    if (thePrefetchedTags_PeerL1.count(addr) > 0 ) {
      ++statOverPredictions;
      ++statOverPredictions_FillPeerL1;
      thePrefetchedTags_PeerL1.erase(addr);
    }
  }

  //Returns true if message should be silently dropped
  bool processFromL1_ReplyMessage( boost::intrusive_ptr<MemoryMessage> aMessage, boost::intrusive_ptr<TransactionTracker> aTracker) {
    DBG_Assert(aMessage);
    DBG_Assert(aTracker);
    if (    aTracker->originatorLevel()
            && *aTracker->originatorLevel() == eL2Prefetcher
            && aTracker->initiator()
            && *aTracker->initiator() == flexusIndex()
            //It turns out we can get invalidations here when they are races
            //However, we don't need to do anything with them
            && aMessage->type() != MemoryMessage::Invalidate
            && aMessage->type() != MemoryMessage::Downgrade
       ) {

      //Normal prefetches

      DBG_Assert( theActivePrefetches.contains( aMessage->address() ), ( << *aMessage ) );

      bool done = false;
      uint64_t access_time;
      MemoryMessage::MemoryMessageType type;
      std::tie( access_time, type) = theActivePrefetches.getLate( aMessage->address() );
      if ( access_time != 0 ) {
        if ( aTracker->fillLevel() && (aMessage->type() == MemoryMessage::PrefetchReadReply || aMessage->type() == MemoryMessage::PrefetchWritableReply)) {
          switch (type) {
            case MemoryMessage::LoadReq:
              if (* aTracker->fillLevel() == eL2 ) {
                ++statHits;
                ++statLateHits_Read_FillL2;
                statHits_WaitTimeL2 << (Flexus::Core::theFlexus->cycleCount() - access_time);
              } else if (* aTracker->fillLevel() == ePeerL1Cache ) {
                ++statHits;
                ++statLateHits_Read_FillPeerL1;
                statHits_WaitTimePeerL1 << (Flexus::Core::theFlexus->cycleCount() - access_time);
              } else {
                ++statHits;
                if ( (aTracker->fillType()) && (*aTracker->fillType() == eCoherence) ) {
                  ++statLateHits_Read_FillCoherence;
                  statHits_WaitTimeCoherence << (Flexus::Core::theFlexus->cycleCount() - access_time);
                } else {
                  ++statLateHits_Read_FillOffChip;
                  statHits_WaitTimeOffChip << (Flexus::Core::theFlexus->cycleCount() - access_time);
                }
              }
              done = true;
              break;
            case MemoryMessage::StoreReq:
            case MemoryMessage::StorePrefetchReq:
            case MemoryMessage::RMWReq:
            case MemoryMessage::CmpxReq:
              if (* aTracker->fillLevel() == eL2 ) {
                ++statHits;
                ++statLateHits_Write_FillL2;
                statHits_WaitTimeL2 << (Flexus::Core::theFlexus->cycleCount() - access_time);
              } else if (* aTracker->fillLevel() == ePeerL1Cache ) {
                ++statHits;
                ++statLateHits_Write_FillPeerL1;
                statHits_WaitTimePeerL1 << (Flexus::Core::theFlexus->cycleCount() - access_time);
              } else {
                ++statHits;
                if ( (aTracker->fillType()) && (*aTracker->fillType() == eCoherence) ) {
                  ++statLateHits_Write_FillCoherence;
                  statHits_WaitTimeCoherence << (Flexus::Core::theFlexus->cycleCount() - access_time);
                } else {
                  ++statLateHits_Write_FillOffChip;
                  statHits_WaitTimeOffChip << (Flexus::Core::theFlexus->cycleCount() - access_time);
                }
              }
              done = true;
              break;
            default:
              break;
          }
        }
      }
      theActivePrefetches.remove( aMessage->address() );
      if (done) {
        // don't insert into the prefetch tags, because there has already been
        // a CPU request for this block - if we insert into the tags, it will
        // later be incorrectly counted as a  prefetch
        thePrefetchTracker.finishPrefetch(aTracker);
        return true;
      }

      switch (aMessage->type()) {
        case MemoryMessage::PrefetchReadReply:
        case MemoryMessage::PrefetchWritableReply:
          if ( aTracker->fillLevel() ) {
            if (* aTracker->fillLevel() == eL2 ) {
              ++statPrefetches_CompletedL2;
              DBG_(Trace, Comp(*this) ( << "Completed Prefetch (L2):" << aMessage->address() ) );
              thePrefetchedTags_L2.insert( aMessage->address() );
            } else if (* aTracker->fillLevel() == ePeerL1Cache ) {
              ++statPrefetches_CompletedPeerL1;
              DBG_(Trace, Comp(*this) ( << "Completed Prefetch (PeerL1):" << aMessage->address() ) );
              thePrefetchedTags_PeerL1.insert( aMessage->address() );
            } else {
              //assume off chip - categorize as coherence or not
              if ( (aTracker->fillType()) && (*aTracker->fillType() == eCoherence) ) {
                ++statPrefetches_CompletedCoherence;
                DBG_(Trace, Comp(*this) ( << "Completed Prefetch (Coherence):" << aMessage->address() ) );
                thePrefetchedTags_Coherence.insert( aMessage->address() );
              } else {
                ++statPrefetches_CompletedOffChip;
                DBG_(Trace, Comp(*this) ( << "Completed Prefetch (Off-chip):" << aMessage->address() ) );
                thePrefetchedTags_OffChip.insert( aMessage->address() );
              }
            }
          }
          thePrefetchTracker.finishPrefetch(aTracker);
          break;
        case MemoryMessage::PrefetchReadRedundant:
          // "redundant" can only be returned by the L1 cache, because the L1 turns
          // it into a normal read request
          ++statPrefetches_RedundantL1;
          DBG_(Trace, Comp(*this) ( << "Redundant Prefetch (L1):" << aMessage->address() ) );
          break;
        default:
          DBG_Assert(false, Comp(*this) ( << "Unexpected message:" << *aMessage ) );
      }
      return true;
    }

    return false; //Pass all other messages
  }

  void issuePrefetches() {
    bool accounted = false;
    while (
      ! thePrefetchQueues.empty()
      && FLEXUS_CHANNEL(Prefetch_ToL1_Prefetch).available()
      && theActivePrefetches.size() < cfg.MaxPrefetches
    ) {
      PrefetchQueues::iterator queue = thePrefetchQueues.begin();
      if (queue->empty()) {
        thePrefetchQueues.pop_front();
        continue;
      }
      if (!accounted) {
        statNumQueues << std::make_pair(thePrefetchQueues.size(), 1);
        accounted = true;
      }

      MemoryAddress  addr = queue->dequeue();
      DBG_Assert( (addr & 63 ) == 0 );
      ++statPrefetches_Total;

      if (queue->empty()) {
        thePrefetchQueues.pop_front();
      } else {
        thePrefetchQueues.relocate(thePrefetchQueues.end(), queue);
      }

      //Filter prefetch against MAF
      if (theActivePrefetches.contains( addr ) ) {
        ++statPrefetches_RedundantInProgress;
        DBG_(Trace, Comp(*this) ( << "Redundant Prefetch (InProgress):" << addr ) );
        continue;
      }

      //Filter prefetch against duplicate tags
      if (theDuplicateTags.count( addr ) > 0 ) {
        ++statPrefetches_DuplicateL1;
        DBG_(Trace, Comp(*this) ( << "Duplicate Prefetch (L1):" << addr ) );
        continue;
      }

      //Issue prefetch read allocate
      MemoryTransport transport;
      boost::intrusive_ptr<MemoryMessage> msg( new MemoryMessage( MemoryMessage::PrefetchReadAllocReq, addr ) );
      boost::intrusive_ptr<TransactionTracker> tracker( new TransactionTracker() );
      tracker->setAddress( addr );
      tracker->setInitiator( flexusIndex() );
      tracker->setOriginatorLevel( eL2Prefetcher );
      transport.set( MemoryMessageTag, msg );
      transport.set( TransactionTrackerTag, tracker );

      theActivePrefetches.insert(addr);
      DBG_(Trace, Comp(*this) ( << "Prefetching " << addr ) );
      DBG_(Iface, Comp(*this) ( << "Sent on Port Prefetch_ToL1_Prefetch: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(Prefetch_ToL1_Prefetch) << transport;
      ++statPrefetches_Issued;
    }

    if (!accounted) {
      statNumQueues << std::make_pair(thePrefetchQueues.size(), 1);
    }
  }

  uint64_t makeGroup(MemoryAddress addr) {
    return (addr & ~((1 << 13) - 1));
  }
  uint64_t makeOffset(MemoryAddress addr) {
    return ((addr & ((1 << 13) - 1)) >> 6);
  }

};

} //End Namespace nPrefetchBuffer

FLEXUS_COMPONENT_INSTANTIATOR( SimplePrefetchController, nSimplePrefetchController);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SimplePrefetchController

#define DBG_Reset
#include DBG_Control()

