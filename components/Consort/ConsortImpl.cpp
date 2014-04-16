#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/Consort/Consort.hpp>

#include <list>
#include <fstream>

#include "Tracer.hpp"

#define FLEXUS_BEGIN_COMPONENT Consort
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <boost/serialization/vector.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <core/stats.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>

#include <components/Common/MemoryMap.hpp>
#include <components/Common/Slices/PrefetchCommand.hpp>
#include <components/Common/Slices/MRCMessage.hpp>
#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/ProtocolMessage.hpp>

#define DBG_DefineCategories Consort
#define DBG_SetDefaultOps AddCat(Consort) Component(*this)
#include DBG_Control()

namespace nConsort {

using namespace Flexus::SharedTypes;
using namespace Flexus::Core;
using Flexus::SharedTypes::PhysicalMemoryAddress;

namespace Stat = Flexus::Stat;
using namespace boost::multi_index;

struct ConsortStats {
  std::string theName;

  Stat::StatCounter statRequests;
  Stat::StatCounter statRequests_MoreAddresses;
  Stat::StatCounter statRequests_StaleAddress;
  Stat::StatCounter statRequests_Tail;
  Stat::StatCounter statRequests_NoList;

  Stat::StatCounter statAppends;
  Stat::StatCounter statAppends_System_Coherence;
  Stat::StatCounter statAppends_System_NonCoherence;
  Stat::StatCounter statAppends_System_PBHit;
  Stat::StatCounter statAppends_System_SpecAtomic;
  Stat::StatCounter statAppends_User_Coherence;
  Stat::StatCounter statAppends_User_NonCoherence;
  Stat::StatCounter statAppends_User_PBHit;
  Stat::StatCounter statAppends_User_SpecAtomic;

  Stat::StatCounter statDuplicates;
  Stat::StatCounter statDuplicates_System_Coherence;
  Stat::StatCounter statDuplicates_System_NonCoherence;
  Stat::StatCounter statDuplicates_System_PBHit;
  Stat::StatCounter statDuplicates_System_SpecAtomic;
  Stat::StatCounter statDuplicates_User_Coherence;
  Stat::StatCounter statDuplicates_User_NonCoherence;
  Stat::StatCounter statDuplicates_User_PBHit;
  Stat::StatCounter statDuplicates_User_SpecAtomic;

  Stat::StatCounter statCMOBCacheHits;
  Stat::StatCounter statCMOBCacheMisses;

  Stat::StatCounter statForwardedAddresses;
  Stat::StatCounter statForwardedLists;

  ConsortStats( std::string const & aName )
    : theName(aName)
    , statRequests                       ( theName + "-Requests"               )
    , statRequests_MoreAddresses         ( theName + "-Requests:MoreAddresses" )
    , statRequests_StaleAddress          ( theName + "-Requests:StaleAddress"  )
    , statRequests_Tail                  ( theName + "-Requests:Tail"          )
    , statRequests_NoList                ( theName + "-Requests:NoList"        )

    , statAppends                        ( theName + "-Appends"                )
    , statAppends_System_Coherence       ( theName + "-Appends:System:Coherence")
    , statAppends_System_NonCoherence    ( theName + "-Appends:System:NonCoherence")
    , statAppends_System_PBHit           ( theName + "-Appends:System:PBHit"   )
    , statAppends_System_SpecAtomic      ( theName + "-Appends:System:SpecAtomic"   )
    , statAppends_User_Coherence         ( theName + "-Appends:User:Coherence" )
    , statAppends_User_NonCoherence      ( theName + "-Appends:User:NonCoherence" )
    , statAppends_User_PBHit             ( theName + "-Appends:User:PBHit"     )
    , statAppends_User_SpecAtomic        ( theName + "-Appends:User:SpecAtomic"     )

    , statDuplicates                     ( theName + "-Duplicates"               )
    , statDuplicates_System_Coherence    ( theName + "-Duplicates:System:Coherence")
    , statDuplicates_System_NonCoherence ( theName + "-Duplicates:System:NonCoherence")
    , statDuplicates_System_PBHit        ( theName + "-Duplicates:System:PBHit"  )
    , statDuplicates_System_SpecAtomic   ( theName + "-Duplicates:System:SpecAtomic"  )
    , statDuplicates_User_Coherence      ( theName + "-Duplicates:User:Coherence")
    , statDuplicates_User_NonCoherence   ( theName + "-Duplicates:User:NonCoherence")
    , statDuplicates_User_PBHit          ( theName + "-Duplicates:User:PBHit"    )
    , statDuplicates_User_SpecAtomic     ( theName + "-Duplicates:User:SpecAtomic"  )

    , statCMOBCacheHits                  ( theName + "-CMOBCache:Hits")
    , statCMOBCacheMisses                ( theName + "-CMOBCache:Misses")

    , statForwardedAddresses             ( theName + "-Forwards"               )
    , statForwardedLists                 ( theName + "-ForwardedLists"         )
  { }
};

static const int32_t kPrefetchListener = 1;
static const int32_t kConsort = 2;
static const int32_t kMRCReflector = 3;

struct DelayedTransport {
  DelayedTransport()
    : theSendCycle(0) {
    theCMOBInsert[0] = -1;
    theCMOBInsert[1] = -1;
  }
  DelayedTransport(uint64_t aCycle, NetworkTransport aTransport)
    : theSendCycle(aCycle)
    , theTransport(aTransport) {
    theCMOBInsert[0] = -1;
    theCMOBInsert[1] = -1;
  }
  DelayedTransport(uint64_t aCycle, NetworkTransport aTransport, int64_t insert0, int64_t insert1)
    : theSendCycle(aCycle)
    , theTransport(aTransport) {
    theCMOBInsert[0] = insert0;
    theCMOBInsert[1] = insert1;
  }
  uint64_t theSendCycle;
  NetworkTransport theTransport;
  int64_t theCMOBInsert[2];
};

struct dt_less {
  bool operator()(const DelayedTransport & x, const DelayedTransport & y) const {
    return x.theSendCycle > y.theSendCycle;
  }
};

struct by_addr {};
typedef multi_index_container
< PhysicalMemoryAddress
, indexed_by
< sequenced<>
, ordered_non_unique
< tag<by_addr>
, identity
< PhysicalMemoryAddress
>
>
>
>
recent_t;

typedef multi_index_container
< long
, indexed_by
< sequenced<>
, ordered_unique
< tag<by_addr>
, identity
< long
>
>
>
>
cmob_cache_t;

class FLEXUS_COMPONENT(Consort) {
  FLEXUS_COMPONENT_IMPL(Consort);

private:

  ConsortStats theStats;

  std::list<NetworkTransport> theNetworkMessagesIn;
  std::list<PredictorTransport> thePredictorMessagesIn;

  std::priority_queue<DelayedTransport, std::vector<DelayedTransport>, dt_less> theNetworkMessagesOut;

  std::vector< PhysicalMemoryAddress > theOrder;
  int64_t theOrderTail;

  boost::intrusive_ptr<MemoryMap> theMemoryMap;

  recent_t theRecentAppends;
  cmob_cache_t theCMOBCache;

  bool cmobCacheLookup( int64_t list_num ) {
    if (cfg.OrderReadLatency == 0 || theFlexus->isFastMode()) {
      //Perfect CMOB cache
      return true;
    }
    //See if the entry is being written
    if (list_num == static_cast<long>(theOrderTail / cfg.AddressList) ) {
      return true; //Hit on the write buffer
    }
    cmob_cache_t::index<by_addr>::type::iterator iter = theCMOBCache.get<by_addr>().find( list_num );
    if (iter != theCMOBCache.get<by_addr>().end()) {
      return true; //Hit on the write buffer
    }
    return false;
  }
  void cmobCacheInsert( int64_t entry ) {
    if (cfg.OrderReadLatency == 0 || theFlexus->isFastMode() || entry < 0 || entry > static_cast<long>(cfg.OrderSize / cfg.AddressList)) {
      return;
    }
    theCMOBCache.push_back( entry );
    if (theCMOBCache.size() > cfg.OrderCacheSize) {
      theCMOBCache.pop_front();
    }
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(Consort)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theStats(statName() )
    , theOrderTail(0)
  {}

  bool isQuiesced() const {
    return    theNetworkMessagesIn.empty()
              &&      theNetworkMessagesOut.empty()
              &&      thePredictorMessagesIn.empty()
              ;
  }

  void saveState(std::string const & aDirName) const {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    oa << theOrderTail;
    oa << theOrder;
    // close archive
    ofs.close();
  }

  void loadState(std::string const & aDirName) {
    if (cfg.Enable) {
      std::string fname( aDirName);
      fname += "/" + statName();
      std::ifstream ifs(fname.c_str(), std::ios::binary);
      boost::archive::binary_iarchive ia(ifs);

      ia >> theOrderTail;
      ia >> theOrder;
      // close archive
      ifs.close();
    }
    int32_t i = theOrderTail - 8;
    if (i < 0) {
      i = 0;
    }
    for ( ; i < theOrderTail; ++i) {
      if (theOrder[i] != 0) {
        theRecentAppends.push_back(PhysicalMemoryAddress(theOrder[i]));
      }
    }
    DBG_Assert( theRecentAppends.size() <= 8 );

    uint64_t zero = 0;
    for (uint32_t i = 0; i < theOrder.size(); ++i) {
      if (theOrder[i] == 0) {
        ++zero;
      }
    }
    DBG_(Dev, ( << "OrderSize: " << theOrder.size() << " EmptyEntries: " << zero ) );
  }

  // Initialization
  void initialize() {
    if (cfg.Enable) {
      //Only allocate all this memory if we are actually going to use it
      theOrder.resize(cfg.OrderSize, PhysicalMemoryAddress(0));
    }
    theMemoryMap = MemoryMap::getMemoryMap(flexusIndex());
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromNic);
  void push( interface::FromNic const &, index_t aVC, NetworkTransport & aMessage) {
    DBG_(Iface, Condition(aMessage[MRCMessageTag]) ( << "Received FromNic: "  << *(aMessage[MRCMessageTag]) ) );
    DBG_(Iface, Condition(aMessage[PrefetchCommandTag]) ( << "Received FromNic: "  << *(aMessage[PrefetchCommandTag]) ) );
    DBG_Assert( aVC == 0 );
    enqueue(aMessage);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromCPU);
  void push( interface::FromCPU const &, PredictorTransport & aMessage) {
    DBG_(Iface, ( << "Received FromCPU: "  << *(aMessage[PredictorMessageTag]) ) );
    enqueue(aMessage);
  }

  void drive( interface::ConsortDrive const &) {
    DBG_(VVerb, ( << "ConsortDrive" ) ) ;
    process();
  }

private:

  void notifyAppend( PhysicalMemoryAddress anAddress, int64_t aLocation) {
    boost::intrusive_ptr<MRCMessage> mrc_msg (new MRCMessage(MRCMessage::kAppend, flexusIndex(), anAddress, aLocation));

    // create the network routing piece
    boost::intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
    net_msg->src = flexusIndex();
    net_msg->dest = theMemoryMap->node(anAddress);
    net_msg->src_port = kConsort;
    net_msg->dst_port = kMRCReflector;
    net_msg->vc = 0;
    net_msg->size = 0; //Control message

    // now make the transport and queue it
    NetworkTransport transport;
    transport.set(MRCMessageTag, mrc_msg);
    transport.set(NetworkMessageTag, net_msg);
    theNetworkMessagesOut.push( DelayedTransport( 0ULL, transport) );

  }

  void addEvent ( PredictorMessage const & msg, TransactionTracker const & tracker) {
    bool coherence = ( tracker.fillType() && *tracker.fillType() == eCoherence );
    bool pb_hit = ( tracker.fillLevel() && *tracker.fillLevel() == ePrefetchBuffer );
    bool spec_atomic = tracker.speculativeAtomicLoad() && *tracker.speculativeAtomicLoad();
    bool off_chip = ( tracker.fillLevel() && ( *tracker.fillLevel() == eLocalMem || *tracker.fillLevel() == eRemoteMem ) );

    if ( coherence || pb_hit || ( off_chip && cfg.AllMisses ) || (spec_atomic && off_chip) ) {
      bool system = ( tracker.OS() && *tracker.OS() );

      PhysicalMemoryAddress aligned_addr = PhysicalMemoryAddress( msg.address() & ~( cfg.CoherenceBlockSize - 1) );
      if (theRecentAppends.get<by_addr>().count(aligned_addr) > 0) {
        ++theStats.statDuplicates;
        if (spec_atomic) {
          if (system) {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [S] [M] [A]" ) );
            ++theStats.statDuplicates_System_SpecAtomic;
          } else {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [U] [M] [A]") );
            ++theStats.statDuplicates_User_SpecAtomic;
          }
        } else if (coherence) {
          if (system) {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [S] [M] [C]" ) );
            ++theStats.statDuplicates_System_Coherence;
          } else {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [U] [M] [C]") );
            ++theStats.statDuplicates_User_Coherence;
          }
        } else if (pb_hit) {
          //PB Hit
          if (system) {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [S] [H] ") );
            ++theStats.statDuplicates_System_PBHit;
          } else {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [U] [H] ") );
            ++theStats.statDuplicates_User_PBHit;
          }
        } else {
          DBG_Assert(off_chip);
          if (system) {
            DBG_( Trace, ( << "Duplicate " << aligned_addr << " [S] [M] [N]" ) );
            ++theStats.statDuplicates_System_NonCoherence;
          } else {
            DBG_( Trace, ( <<  "Duplicate " << aligned_addr << " [U] [M] [N]") );
            ++theStats.statDuplicates_User_NonCoherence;
          }
        }
        return; //Don't add the address to the order twice if it was recently added
      }

      ++theStats.statAppends;
      if (spec_atomic) {
        if (system) {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [S] [M] [A]" ) );
          ++theStats.statDuplicates_System_SpecAtomic;
        } else {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [U] [M] [A]") );
          ++theStats.statDuplicates_User_SpecAtomic;
        }
      } else if (coherence) {
        if (system) {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [S] [M] [C]") );
          ++theStats.statAppends_System_Coherence;
        } else {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [U] [M] [C]") );
          ++theStats.statAppends_User_Coherence;
        }
      } else if (pb_hit) {
        //PB Hit
        if (system) {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [S] [H]") );
          ++theStats.statAppends_System_PBHit;
        } else {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [U] [H]") );
          ++theStats.statAppends_User_PBHit;
        }
      } else {
        DBG_Assert(off_chip);
        //Non-coherence off-chip
        if (system) {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [S] [M] [N]") );
          ++theStats.statAppends_System_NonCoherence;
        } else {
          DBG_( Trace, ( << "APPEND " << aligned_addr << " [U] [M] [N]") );
          ++theStats.statAppends_User_NonCoherence;
        }
      }

      //if (coherence) {
      //  nPrefetchListener::theSVB_JMY.coherenceMiss( aligned_addr, system, flexusIndex() );
      //}

      theOrder[theOrderTail] = aligned_addr;
      while ( theRecentAppends.size() >= 8 ) {
        theRecentAppends.pop_front();
      }
      theRecentAppends.push_back(aligned_addr);

      notifyAppend(aligned_addr, theOrderTail);

      theOrderTail = increment(theOrderTail);
      if (theOrderTail == 0) {
        DBG_( Dev, ( << "Order wrapped around to zero upon append" ) );
      }
    }
  }

  int64_t decrement(int64_t aLocation) {
    --aLocation;
    if (aLocation < 0) {
      aLocation += theOrder.size();
    }
    if (aLocation > static_cast<long>(theOrder.size())) {
      aLocation %= theOrder.size();
    }
    return aLocation;
  }

  int64_t increment(int64_t  aLocation) {
    ++aLocation;
    if (aLocation > static_cast<long>(theOrder.size())) {
      aLocation %= theOrder.size();
    }
    return aLocation;
  }

  boost::intrusive_ptr<PrefetchCommand> constructList( int64_t aLocation, uint32_t aListSize, uint32_t aDestNode) {
    boost::intrusive_ptr<PrefetchCommand> ret_val( new PrefetchCommand( PrefetchCommand::ePrefetchAddressList ) );
    ret_val->source() = flexusIndex();

    for ( uint32_t i = 0; i < aListSize; ++i ) {
      if ( theOrder[aLocation] != 0 ) {
        ret_val->addressList().push_back(theOrder[aLocation]);
        ++theStats.statForwardedAddresses;
      } else {
        break; //Hit the bottom of the order
      }
      aLocation = increment(aLocation);
    }
    ret_val->location() = aLocation;
    return ret_val;
  }

  void processMRCMessage( MRCMessage const & msg ) {
    DBG_Assert( msg.type() == MRCMessage::kRequestStream); //This component only gets stream requests

    ++theStats.statRequests;
    DBG_( Iface, ( << "NewStreamRequest " << msg) );

    //Ensure that location is valid
    if (msg.address() != theOrder[msg.location()]) {
      ++theStats.statRequests_StaleAddress;
      DBG_( Iface, ( << "StaleAddress " << msg ) );
      return;
    }
    if (increment(msg.location()) == theOrderTail) {
      ++theStats.statRequests_Tail;
      DBG_( Iface, ( << "TailRequest " << msg ) );
      return;
    }

    int64_t list_loc = msg.location() / cfg.AddressList;

    //We have a valid prefetch request!  Lets send it a chunk
    boost::intrusive_ptr<PrefetchCommand> cmd = constructList( increment(msg.location()), cfg.AddressList, msg.node() );
    cmd->tag() = msg.tag();
    cmd->queue() = -1; //Tell the PrefetchListener it must allocate a new queue

    if (cmd->addressList().empty()) {
      ++theStats.statRequests_NoList;
      //Don't count stillborn streams in the length histograms
      DBG_( Iface, ( << "No addresses " << *cmd) );
      return;
    }

    DBG_( Iface, ( << "Forward " << *cmd << " in-reply-to " << msg) );

    ++theStats.statForwardedLists;

    //Determine how int64_t it took to locate this CMOB list
    uint64_t cmob_lookup_latency = 0;
    int64_t insert0 = -1;
    int64_t insert1 = -1;
    if (cmobCacheLookup( list_loc ) ) {
      ++theStats.statCMOBCacheHits;
      if (list_loc + 1 < static_cast<long>(theOrderTail / cfg.AddressList)) {
        insert0 = list_loc + 1;
      }
    } else {
      ++theStats.statCMOBCacheMisses;
      cmob_lookup_latency = cfg.OrderReadLatency;
      if (list_loc < static_cast<long>(theOrderTail / cfg.AddressList)) {
        insert0 = list_loc;
      }
      if (list_loc + 1 < static_cast<long>(theOrderTail / cfg.AddressList)) {
        insert1 = list_loc + 1;
      }
    }

    // create the network routing piece
    boost::intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
    net_msg->src = flexusIndex();
    net_msg->dest = msg.node();
    net_msg->src_port = kConsort;
    net_msg->dst_port = kPrefetchListener;
    net_msg->vc = 0;
    net_msg->size = 0; //Control message

    // now make the transport and queue it
    NetworkTransport transport;
    transport.set(PrefetchCommandTag, cmd);
    transport.set(NetworkMessageTag, net_msg);
    theNetworkMessagesOut.push(DelayedTransport( theFlexus->cycleCount() + cmob_lookup_latency, transport, insert0, insert1) );
  }

  void processPrefetchCommand( PrefetchCommand const & msg, uint32_t aNode ) {
    DBG_Assert( msg.type() == PrefetchCommand::ePrefetchRequestMoreAddresses);

    ++theStats.statRequests;
    ++theStats.statRequests_MoreAddresses;

    DBG_Assert( msg.location() != -1 );
    DBG_Assert( msg.source() == flexusIndex() );
    DBG_( Iface, ( << "REQUEST-MORE by " << aNode << " " << msg ) );

    int64_t list_loc = msg.location() / cfg.AddressList;

    boost::intrusive_ptr<PrefetchCommand> cmd = constructList( msg.location(), cfg.AddressList, aNode);
    cmd->queue() = msg.queue(); //Send the new address list to the same queue it came from

    if (cmd->addressList().empty()) {
      DBG_( Iface, ( << "No more addresses " << *cmd) );
      ++theStats.statRequests_NoList;
      return;
    }
    ++theStats.statForwardedLists;

    //Determine how int64_t it took to locate this CMOB list
    uint64_t cmob_lookup_latency = 0;
    int64_t insert0 = -1;
    int64_t insert1 = -1;
    if (cmobCacheLookup( list_loc ) ) {
      ++theStats.statCMOBCacheHits;
      if (list_loc + 1 < static_cast<long>(theOrderTail / cfg.AddressList)) {
        insert0 = list_loc + 1;
      }
    } else {
      ++theStats.statCMOBCacheMisses;
      cmob_lookup_latency = cfg.OrderReadLatency;
      if (list_loc < static_cast<long>(theOrderTail / cfg.AddressList)) {
        insert0 = list_loc;
      }
      if (list_loc + 1 < static_cast<long>(theOrderTail / cfg.AddressList)) {
        insert1 = list_loc + 1;
      }
    }

    DBG_( Iface, ( << "Forward " << *cmd << " in-reply-to " << msg ) );

    // create the network routing piece
    boost::intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
    net_msg->src = flexusIndex();
    net_msg->dest = aNode;
    net_msg->src_port = kConsort;
    net_msg->dst_port = kPrefetchListener;
    net_msg->vc = 0;
    net_msg->size = 0; //Control message

    // now make the transport and queue it
    NetworkTransport transport;
    transport.set(PrefetchCommandTag, cmd);
    transport.set(NetworkMessageTag, net_msg);
    theNetworkMessagesOut.push( DelayedTransport( theFlexus->cycleCount() + cmob_lookup_latency, transport, insert0, insert1)  );
  }

  void enqueue(NetworkTransport & aMessage) {
    if (cfg.Enable) {
      theNetworkMessagesIn.push_back(aMessage);
    }
  }
  void enqueue(PredictorTransport & aMessage) {
    if (cfg.Enable) {
      thePredictorMessagesIn.push_back(aMessage);
    }
  }

  void process() {
    if (cfg.Enable) {
      //Process incoming messages from the Protocol engine - reflects stream requests to Consorts
      processPredictorMessages();

      //Process incoming messages from the network - add notifications from Consorts
      processNetworkMessages();

      //Send outgoing messages to the network
      sendMessages();
    }
  }

  void sendMessages() {
    //Send to Network
    while ( FLEXUS_CHANNEL(ToNic).available() && !theNetworkMessagesOut.empty() && theNetworkMessagesOut.top().theSendCycle <= theFlexus->cycleCount() ) {
      if (theNetworkMessagesOut.top().theTransport[MRCMessageTag]) {
        DBG_(Iface, ( << "Sending ToNetwork: "  << *(theNetworkMessagesOut.top().theTransport[MRCMessageTag]) ));
      } else if (theNetworkMessagesOut.top().theTransport[PrefetchCommandTag]) {
        DBG_(Iface, ( << "Sending ToNetwork: "  << *(theNetworkMessagesOut.top().theTransport[PrefetchCommandTag]) ));
      }
      FLEXUS_CHANNEL(ToNic) << const_cast<NetworkTransport &>( theNetworkMessagesOut.top().theTransport );
      cmobCacheInsert( theNetworkMessagesOut.top().theCMOBInsert[0] );
      cmobCacheInsert( theNetworkMessagesOut.top().theCMOBInsert[1] );
      theNetworkMessagesOut.pop();
    }
  }

  void processPredictorMessages() {
    while (! thePredictorMessagesIn.empty() ) {
      boost::intrusive_ptr<PredictorMessage> msg = thePredictorMessagesIn.front()[PredictorMessageTag];
      DBG_Assert(msg);
      boost::intrusive_ptr<TransactionTracker> tracker = thePredictorMessagesIn.front()[TransactionTrackerTag];
      DBG_Assert( tracker );
      DBG_(Iface, Comp(*this) ( << "Processing predictor message: "  << *msg) );

      //Trace support
      if (msg->type() == PredictorMessage::eReadNonPredicted && tracker->fillType() && ( !tracker->blockedInMAF() || ! *tracker->blockedInMAF() ) ) {
        traceRead(msg->node(), msg->address(), *tracker->fillType() == eCoherence );
      } else if (msg->type() == PredictorMessage::eWrite && tracker->fillLevel() && ( *tracker->fillLevel() == eLocalMem || *tracker->fillLevel() == eRemoteMem ) ) {
        traceWrite(msg->node(), msg->address());
      }

      if ( msg->type() == PredictorMessage::eReadNonPredicted ) {
        addEvent( *msg, *tracker );
      }

      thePredictorMessagesIn.pop_front();
    }
  }

  void processNetworkMessages() {
    while (! theNetworkMessagesIn.empty() ) {
      boost::intrusive_ptr<NetworkMessage> net_msg = theNetworkMessagesIn.front()[NetworkMessageTag];
      boost::intrusive_ptr<MRCMessage> mrc_msg = theNetworkMessagesIn.front()[MRCMessageTag];
      boost::intrusive_ptr<PrefetchCommand> prefetch_cmd = theNetworkMessagesIn.front()[PrefetchCommandTag];
      DBG_Assert(net_msg);
      if (mrc_msg) {
        processMRCMessage(*mrc_msg);
      }
      if (prefetch_cmd) {
        processPrefetchCommand(*prefetch_cmd, net_msg->src);
      }
      theNetworkMessagesIn.pop_front();
    }
  }

  void traceRead( uint32_t aNode, PhysicalMemoryAddress anAddress, bool is_coherence) {
    /*
        if (is_coherence) {
          DBG_(Trace, ( << (boost::padded_string_cast<2,'0'>(aNode)) << "-consort C " << anAddress ) );
          if (cfg.Trace) {
            theTraceManager->consumption(aNode, anAddress);
          }
        } else {
          DBG_(Trace, ( << (boost::padded_string_cast<2,'0'>(aNode)) << "-consort R " << anAddress ) );
        }
    */
  }

  void traceWrite( uint32_t aNode, PhysicalMemoryAddress  anAddress) {
    /*
        DBG_(Trace, ( << (boost::padded_string_cast<2,'0'>(aNode)) << "-consort U " << anAddress ) );
        if (cfg.Trace) {
          theTraceManager->upgrade(aNode, anAddress);
        }
    */
  }

};

} //End Namespace nConsort

FLEXUS_COMPONENT_INSTANTIATOR( Consort, nConsort );
FLEXUS_PORT_ARRAY_WIDTH( Consort, FromNic) {
  return cfg.VChannels;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Consort

#define DBG_Reset
#include DBG_Control()

