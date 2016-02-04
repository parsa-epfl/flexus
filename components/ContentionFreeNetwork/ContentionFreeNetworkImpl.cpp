#include <set>
#include <vector>
#include <queue>

#include <components/ContentionFreeNetwork/ContentionFreeNetwork.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>

#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/stats.hpp>
#include <core/flexus.hpp>

#define FLEXUS_BEGIN_COMPONENT ContentionFreeNetwork
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories ContentionFreeNetwork
#define DBG_SetDefaultOps AddCat(ContentionFreeNetwork)
#include DBG_Control()

namespace nCFNetwork {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;

class FLEXUS_COMPONENT(ContentionFreeNetwork) {
  FLEXUS_COMPONENT_IMPL(ContentionFreeNetwork);

  struct QueueEntry {
    int64_t arrival_time;
    int64_t delivery_time;
    MemoryTransport transport;

    QueueEntry(int64_t atime, int64_t dtime, MemoryTransport & t) : arrival_time(atime), delivery_time(dtime), transport(t) {}

    bool operator<(const QueueEntry & b) const {
      if (delivery_time == b.delivery_time) {
        return arrival_time > b.arrival_time;
      } else {
        return delivery_time > b.delivery_time;
      }
    }
  };

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(ContentionFreeNetwork)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ),
      theTotalMessages ( "Network Messages Received", this ),
      theTotalMessages_Data ( "Network Messages Received:Data", this ),
      theTotalMessages_NoData ( "Network Messages Received:NoData", this ),
      theTotalHops     ( "Network Message Hops", this ),
      theTotalFlitHops     ( "Network:Flit-Hops:Total", this ),
      theDataFlitHops     ( "Network:Flit-Hops:Data", this ),
      theOverheadFlitHops     ( "Network:Flit-Hops:Overhead", this ),
      theTotalFlits     ( "Network:Flits:Total", this ),
      theDelayHistogram     ( "Network:ContentionDelay", this ) {
  }

  bool isQuiesced() const {
    for (int32_t i = 0; i < (theNumNodes * cfg.VChannels); i++) {
      if (!theDeliveryQueue[i].empty()) {
        return false;
      }
    }
    return true;
  }

// Initialization
  void initialize() {
    int32_t i;
    for ( i = 0; i < cfg.VChannels; i++ ) {
      theNetworkLatencyHistograms.push_back ( new Stat::StatLog2Histogram  ( "NetworkLatency   VC[" + std::to_string(i) + "]", this ) );
    }

    theNumNodes = cfg.NumNodes;
    theNumCores = cfg.NumCores;

    theGridWidth = (int)std::sqrt((double)theNumCores);

    theDeliveryQueue.resize(theNumNodes * cfg.VChannels);
  }

// Ports
  bool available(interface::FromNode const &, index_t anIndex) {
    return true;
  }

  void push(interface::FromNode const &, index_t anIndex, MemoryTransport & transport) {
    DBG_Assert( (transport[NetworkMessageTag]->src == static_cast<int>(anIndex / cfg.VChannels)) ,
                ( << "tp->src " << transport[NetworkMessageTag]->src
                  << "%src " << anIndex / cfg.VChannels
                  << "anIndex " << anIndex) ); //static_cast to suppress warning about signed/unsigned comparison
    DBG_Assert( (transport[NetworkMessageTag]->vc == static_cast<int>(anIndex % cfg.VChannels)) ); //static_cast to suppress warning about signed/unsigned comparison

    newPacket(transport);
  }

//Drive Interfaces
  void drive( interface::NetworkDrive const &) {
    int64_t currTime = Flexus::Core::theFlexus->cycleCount();

    // First, add newly active queues to the active set
    while (!theActivationHeap.empty() && theActivationHeap.top().first <= currTime) {
      DBG_Assert(theActivationHeap.top().first == currTime, ( << "Found activation record for the past!!! Should have already been active." ));
      index_t pdest = theActivationHeap.top().second;

      theActiveSet.insert(pdest);
      theActivationHeap.pop();
    }

    std::set<index_t>::iterator iter = theActiveSet.begin();
    std::set<index_t>::iterator end = theActiveSet.end();;

    std::set<index_t> remove_set;

    for (; iter != end; iter++) {
      index_t pdest = *iter;
      if (!FLEXUS_CHANNEL_ARRAY( ToNode, pdest).available()) {
        continue;
      }
      DBG_Assert(theDeliveryQueue[pdest].top().delivery_time <= currTime);
      deliverMessage(theDeliveryQueue[pdest].top());
      theDeliveryQueue[pdest].pop();
      if (theDeliveryQueue[pdest].empty()) {
        remove_set.insert(pdest);
      } else if (theDeliveryQueue[pdest].top().delivery_time > (currTime + 1)) {
        theActivationHeap.push(std::make_pair(theDeliveryQueue[pdest].top().delivery_time, pdest));
        remove_set.insert(pdest);
      }
    }
    iter = remove_set.begin();
    end = remove_set.end();
    for (; iter != end; iter++) {
      theActiveSet.erase(*iter);
    }
  }

public:

// Set a particular message as delivered, finish any statistics related to the message
// and deliver to the destination node.
// Encapsulated in a function object "theDeliver" to call from outside code
  bool deliverMessage ( const QueueEntry & entry) {
    MemoryTransport transport(entry.transport);
    int64_t arrival_time = entry.arrival_time;
    int64_t scheduled_delivery_time = entry.delivery_time;
    int64_t curr_time = Flexus::Core::theFlexus->cycleCount();

    index_t pdest = (transport[NetworkMessageTag]->dest) * cfg.VChannels + transport[NetworkMessageTag]->vc;

    DBG_(Iface, ( << "Delivering Message to port " << pdest << ": " << (*transport[MemoryMessageTag]) ));

    FLEXUS_CHANNEL_ARRAY( ToNode, pdest) << transport;

    int32_t vc = transport[NetworkMessageTag]->vc;
    *theNetworkLatencyHistograms[vc] << Flexus::Core::theFlexus->cycleCount() - arrival_time;

    int64_t delay = curr_time - scheduled_delivery_time;
    theDelayHistogram << delay;

    return false;
  }

private:

  void queueMessage(MemoryTransport & transport, index_t pdest, int64_t delivery_time) {
    int64_t arrival_time = Flexus::Core::theFlexus->cycleCount();

    DBG_Assert(( pdest < theDeliveryQueue.size()), ( << "Queueing new memory transport for delivery in queue " << pdest << " num queues = " << theDeliveryQueue.size() ));
    // Put in the destination queue with arrival and delivery time
    theDeliveryQueue[pdest].push(QueueEntry(arrival_time, delivery_time, transport));

    // Check if destination queue is active
    // if not, add an entry to the activation queue
    if (theActiveSet.count(pdest) == 0) {
      theActivationHeap.push(std::make_pair(delivery_time, pdest));
    }
  }

  inline int32_t absolute(int32_t x) {
    return (( x < 0) ? -x : x);
  }

  inline int32_t get_x(int32_t a) {
    return (a % theGridWidth);
  }

  inline int32_t get_y(int32_t a) {
    return (a / theGridWidth);
  }

  int32_t calc_distance(int32_t a, int32_t b) {
    return (absolute(get_x(a) - get_x(b)) + absolute(get_y(a) - get_y(b)));
  }

// Add a new transport to the network from a node
  void newPacket(MemoryTransport & transport) {

    DBG_Assert(transport[NetworkMessageTag]);

    //Ensure all NetworkMessage fields have been initialized
    DBG_Assert(transport[NetworkMessageTag]->src != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->dest != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->vc != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->size != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->src_port != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->dst_port != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));

    // Calculate the switch indices for the source and destination nodes
    int32_t source = transport[NetworkMessageTag]->src % theNumCores;
    int32_t dest = transport[NetworkMessageTag]->dest % theNumCores;

    // Calculate the Manhatten distance between the nodes
    int32_t hop_count = calc_distance(source, dest);

    // we have one router per hop, plus another one at the end to eject the node
    int32_t latency = (hop_count + 1) * cfg.RouterDelay;

    // additional transmission latency
    int32_t flit_count = (transport[NetworkMessageTag]->size + cfg.ChannelBandwidth - 1) / cfg.ChannelBandwidth;
    latency += flit_count;

    int64_t delivery_time = Flexus::Core::theFlexus->cycleCount() + latency;

    // Calculate the destination
    index_t pdest = (transport[NetworkMessageTag]->dest) * cfg.VChannels + transport[NetworkMessageTag]->vc;

    DBG_(Iface, ( << "Queueing message from " << transport[NetworkMessageTag]->src << " (" << source << ") to " << transport[NetworkMessageTag]->dest << " (" << dest << ") at cycle " << Flexus::Core::theFlexus->cycleCount() << " for delivery at " << delivery_time << " to port " << pdest << ", hops=" << hop_count << ", flits=" << flit_count << ": " << *transport[MemoryMessageTag] ));

    queueMessage(transport, pdest, delivery_time);

    if (transport[TransactionTrackerTag]) {
      std::string cause( boost::padded_string_cast < 2, '0' > (transport[NetworkMessageTag]->src) + " -> " +
                         boost::padded_string_cast < 2, '0' > (transport[NetworkMessageTag]->dest) );
      transport[TransactionTrackerTag]->setDelayCause("Network", cause);
    }

    ++theTotalMessages;
    // Header size = 8
    if (transport[NetworkMessageTag]->size > 8) {
      ++theTotalMessages_Data;
    } else {
      ++theTotalMessages_NoData;
    }

    theTotalFlits += flit_count;
    theTotalFlitHops += (hop_count * flit_count);

    theTotalHops += hop_count;
  }

  struct ActivationHeapCompare {
    bool operator()(const std::pair<int64_t, index_t> &a, const std::pair<int64_t, index_t> &b) const {
      // return a < b
      // Heap returns greatest element
      // so a < b, means that b comes BEFORE a
      if (a.first == b.first) {
        return a.second > b.second;
      } else {
        return a.first > b.first;
      }
    }
  };

  std::set<index_t> theActiveSet;
  std::priority_queue<std::pair<int64_t, index_t>, std::vector<std::pair<int64_t, index_t> >, ActivationHeapCompare> theActivationHeap;
  std::vector<std::priority_queue<QueueEntry> > theDeliveryQueue;

  Stat::StatCounter theTotalMessages;
  Stat::StatCounter theTotalMessages_Data;
  Stat::StatCounter theTotalMessages_NoData;
  Stat::StatCounter theTotalHops;
  Stat::StatCounter theTotalFlitHops;
  Stat::StatCounter theDataFlitHops;
  Stat::StatCounter theOverheadFlitHops;
  Stat::StatCounter theTotalFlits;

  Stat::StatLog2Histogram theDelayHistogram;

  std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram > > theNetworkLatencyHistograms;

  int32_t theNumNodes;
  int32_t theNumCores;
  int32_t theGridWidth;

};

} //End Namespace nNetwork

FLEXUS_COMPONENT_INSTANTIATOR( ContentionFreeNetwork, nCFNetwork );
FLEXUS_PORT_ARRAY_WIDTH( ContentionFreeNetwork, ToNode ) {
  return cfg.VChannels * cfg.NumNodes;
}
FLEXUS_PORT_ARRAY_WIDTH( ContentionFreeNetwork, FromNode ) {
  return cfg.VChannels * cfg.NumNodes;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT ContentionFreeNetwork

#define DBG_Reset
#include DBG_Control()
