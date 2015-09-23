#include <fstream>

#include <components/NetShim/MemoryNetwork.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>

#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/stats.hpp>
#include <core/flexus.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryNetwork
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "netcontainer.hpp"

#define DBG_DefineCategories MemoryNetwork
#define DBG_SetDefaultOps AddCat(MemoryNetwork)
#include DBG_Control()

namespace nNetwork {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;
using namespace nNetShim;

class FLEXUS_COMPONENT(MemoryNetwork) {
  FLEXUS_COMPONENT_IMPL(MemoryNetwork);

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MemoryNetwork)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ),
      theTotalMessages ( "Network Messages Received", this ),
      theTotalMessages_Data ( "Network Messages Received:Data", this ),
      theTotalMessages_NoData ( "Network Messages Received:NoData", this ),
      theTotalHops     ( "Network Message Hops", this ),
      theTotalFlitHops     ( "Network:Flit-Hops:Total", this ),
      theDataFlitHops     ( "Network:Flit-Hops:Data", this ),
      theOverheadFlitHops     ( "Network:Flit-Hops:Overhead", this ),
      theTotalFlits     ( "Network:Flits:Total", this ),
      theNetworkLatencies ("Network:Latencies", this),
      theMaxInfiniteBuffer ( "Maximum network input buffer depth", this ) {
    //nc = new NetContainer();
  }

//added by mehdi
ofstream LatencyOut;
long long latency;
long PacketCount;
//mehdi

  bool isQuiesced() const {
    return transports.empty();

  }

  // Initialization
  void initialize() {
    int
    i;
	//mehdi
	LatencyOut.open("NetshimStatOut");
	latency=0;
	PacketCount=0;
	//end of medhi

    for ( i = 0; i < cfg.VChannels; i++ ) {
      theNetworkLatencyHistograms.push_back ( new Stat::StatLog2Histogram  ( "NetworkLatency   VC[" + boost::lexical_cast<std::string>(i) + "]", this ) );
      theBufferTimes.push_back              ( new Stat::StatLog2Histogram ( "BufferTime       VC[" + boost::lexical_cast<std::string>(i) + "]", this ) );
      theAtHeadTimes.push_back              ( new Stat::StatLog2Histogram ( "AtBufferHeadTime VC[" + boost::lexical_cast<std::string>(i) + "]", this ) );
      theAcceptWaitTimes.push_back          ( new Stat::StatLog2Histogram ( "AcceptWaitTime   VC[" + boost::lexical_cast<std::string>(i) + "]", this ) );
    }

    nc = new NetContainer();
    if ( nc->buildNetwork ( cfg.NetworkTopologyFile.c_str() ) ) {
      throw Flexus::Core::FlexusException ( "Error building the network" );
    }
  }

  void finalize() {}

  // Ports
  bool available(interface::FromNode const &, index_t anIndex) {
    int32_t node = anIndex / cfg.VChannels;
    int32_t vc = anIndex % cfg.VChannels;
    vc = MAX_PROT_VC - vc - 1;
    DBG_(VVerb, Comp(*this) ( << "Check network availability for node: " << node << " vc: " << vc << " -> " << nc->isNodeOutputAvailable(node, vc)));
    return nc->isNodeOutputAvailable(node, vc);
  }
  void push(interface::FromNode const &, index_t anIndex, MemoryTransport & transport) {
    DBG_Assert( (transport[NetworkMessageTag]->src == static_cast<int>(anIndex / cfg.VChannels)) ,
                ( << "tp->src " << transport[NetworkMessageTag]->src
                  << "%src " << anIndex / cfg.VChannels
                  << "anIndex " << anIndex) ); //static_cast to suppress warning about signed/unsigned comparison
    DBG_Assert( ( (transport[NetworkMessageTag]->vc == static_cast<int>(anIndex % cfg.VChannels)) ) ); //static_cast to suppress warning about signed/unsigned comparison
    newPacket(transport);
  }

  //Drive Interfaces
  void drive( interface::NetworkDrive const &) {
    nNetShim::currTime = Flexus::Core::theFlexus->cycleCount();

        if(nNetShim::currTime==149999)
	{
                double avg_latency=double(latency)/double(PacketCount);
		LatencyOut<<"latency of netshim : "<<avg_latency;
		LatencyOut.flush();
	}

    // We need to use function objects for calls back into simics from the NetShim.
    // In particular, checking if a node will accept a message (.available()) and
    // final delivery of a message must be function objects for now.
    if (! theAvail) {
        theAvail = [this](auto x, auto y){ return this->isNodeAvailable(x,y); };
          // ll::bind( &MemoryNetworkComponent::isNodeAvailable,
          //           this, ll::_1,ll::_2);

        theDeliver = [this](auto x){ return this->deliverMessage(x); };
          // ll::bind ( &MemoryNetworkComponent::deliverMessage,
          //            this, ll::_1 );

      nc->setCallbacks ( theAvail, theDeliver );
    }

    // Kick the network simulation for one cycle
    if ( nc->drive() ) {
      throw Flexus::Core::FlexusException ( "MemoryNetwork error in drive()" );
    }

    // This could be a waste of time.  Use it as a monitor to see if any
    // of the infinite network input queues get too deep.  If this isn't common,
    // it may be worthwhile to comment this out.
    theMaxInfiniteBuffer << nc->getMaximumInfiniteBufferDepth();

  }

public:

  // Interface to the NetContainer
  boost::function<bool(const int, const int)> theAvail;
  boost::function<bool(const MessageState *)> theDeliver;

  // Can another message be removed from the network?
  // Encapsulated in a function object "theAvail" to call from outside code
  bool isNodeAvailable ( const int32_t node, const int32_t vc ) const {
    int32_t real_net_vc = MAX_PROT_VC - vc - 1;
    index_t pdest = (node) * cfg.VChannels + real_net_vc;
    DBG_(Iface,  ( << "netmessage: available? "
                   << "node: " << node
                   << " vc: " << real_net_vc
                   << " pdest: " << pdest
                 ));
    return FLEXUS_CHANNEL_ARRAY ( ToNode, pdest ).available();
  }

  // Set a particular message as delivered, finish any statistics related to the message
  // and deliver to the destination node.
  // Encapsulated in a function object "theDeliver" to call from outside code
  bool deliverMessage ( const MessageState * msg ) {
    index_t pdest = (transports[msg->serial][NetworkMessageTag]->dest) * cfg.VChannels +
                    transports[msg->serial][NetworkMessageTag]->vc;

    int32_t vc = transports[msg->serial][NetworkMessageTag]->vc;

    DBG_(Trace, ( << "Network Delivering msg From " << msg->srcNode << " to " << msg->destNode << ", on vc " << msg->networkVC << ", serial: " << msg->serial << " Message =  " << *(transports[msg->serial][MemoryMessageTag]) ));

    FLEXUS_CHANNEL_ARRAY( ToNode, pdest) << transports[msg->serial];

    ++theTotalMessages;
    if (transports[msg->serial][NetworkMessageTag]->size > 8) {
      ++theTotalMessages_Data;
    } else {
      ++theTotalMessages_NoData;
    }

    // transmitLatency now equals flits, go back to NetworkMessageTag to detmine actual size
    theTotalFlits += msg->transmitLatency;
    theTotalFlitHops += (msg->hopCount * msg->transmitLatency);
    if (transports[msg->serial][NetworkMessageTag]->size > 8) {
      theDataFlitHops += msg->hopCount * msg->transmitLatency;
      // No consistent way to identify how many flits used for non-data portion of data message
      // so just put entire message in the data bin.
    } else {
      theOverheadFlitHops += msg->hopCount * msg->transmitLatency;
    }

    theTotalHops += msg->hopCount;
    *theNetworkLatencyHistograms[vc]
        << Flexus::Core::theFlexus->cycleCount() - msg->startTS;

    latency+=Flexus::Core::theFlexus->cycleCount() - msg->startTS;
    PacketCount++;

    theNetworkLatencies << std::make_pair(((int64_t) Flexus::Core::theFlexus->cycleCount() - msg->startTS), 1); // mammad

    /*
        *theBufferTimes[vc]     << std::make_pair ( msg->bufferTime, 1 );

        *theAtHeadTimes[vc]     << std::make_pair ( msg->atHeadTime, 1 );

        *theAcceptWaitTimes[vc] << std::make_pair ( msg->acceptTime, 1 );
    */

    transports.erase ( msg->serial );

    return false;
  }

private:

  // Add a new transport to the network from a node
  void newPacket(MemoryTransport & transport) {

    MessageState * msg;

    DBG_Assert(transport[NetworkMessageTag]);

    //Ensure all NetworkMessage fields have been initialized
    DBG_Assert(transport[NetworkMessageTag]->src != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->dest != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->vc != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->size != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->src_port != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));
    DBG_Assert(transport[NetworkMessageTag]->dst_port != -1, ( << "No src for " << *(transport[MemoryMessageTag]) ));

    // Allocate and initialize the internal NetShim simulator message
    // state and send it into the interconnect.
    msg = allocMessageState();

    msg->srcNode         = transport[NetworkMessageTag]->src;
    msg->destNode        = transport[NetworkMessageTag]->dest;
    msg->priority        = MAX_PROT_VC - transport[NetworkMessageTag]->vc - 1; // Note, this field really needs to be added to the NetworkMessage
    msg->networkVC       = 0;
#if 0
    // Size is a boolean (!control/data), which is translated into a
    // real latency inside the network simulator
    if (transport[MemoryMessageTag]) {
      DBG_Assert(transport[NetworkMessageTag]->size == 0 || transport[NetworkMessageTag]->size == 64,
                 ( << "Bad size field: src=" << transport[NetworkMessageTag]->src << " dest=" << transport[NetworkMessageTag]->dest
                   << " vc=" << transport[NetworkMessageTag]->vc
                   << " size=" << transport[NetworkMessageTag]->size
                   << " src_port=" << transport[NetworkMessageTag]->src_port
                   << " dst_port=" << transport[NetworkMessageTag]->dst_port
                   << " content=" << *transport[MemoryMessageTag]
                 ));
    } else {
      DBG_Assert(transport[NetworkMessageTag]->size == 0 || transport[NetworkMessageTag]->size == 64,
                 ( << "Bad size field: src=" << transport[NetworkMessageTag]->src << " dest=" << transport[NetworkMessageTag]->dest
                   << " vc=" << transport[NetworkMessageTag]->vc
                   << " size=" << transport[NetworkMessageTag]->size
                   << " src_port=" << transport[NetworkMessageTag]->src_port
                   << " dst_port=" << transport[NetworkMessageTag]->dst_port
                 ));
    }
#endif
    msg->transmitLatency = transport[NetworkMessageTag]->size;
    msg->flexusInFastMode = Flexus::Core::theFlexus->isFastMode();
    msg->hopCount        = -1;  // Note, the local switch also gets counted, so we start at -1
    msg->startTS         = Flexus::Core::theFlexus->cycleCount();
    msg->myList          = nullptr;

    if (transport[TransactionTrackerTag]) {
      std::string cause( boost::padded_string_cast < 3, '0' > (transport[NetworkMessageTag]->src) + " -> " + boost::padded_string_cast < 3, '0' > (transport[NetworkMessageTag]->dest) );

      transport[TransactionTrackerTag]->setDelayCause("Network", cause);
    }

    // We index the actual transport object through a map of serial numbers (assigned when
    // the MessageState object is allocated) to transports
    transports.insert ( make_pair ( msg->serial, transport ) );

    DBG_(Iface, ( << "New packet: "
                  << " serial=" << msg->serial
                  << " src=" << transport[NetworkMessageTag]->src
                  << " dest=" << transport[NetworkMessageTag]->dest
                  << " vc=" << transport[NetworkMessageTag]->vc
                  << " src_port=" << transport[NetworkMessageTag]->src_port
                  << " dest_port=" << transport[NetworkMessageTag]->dst_port
                )
         Comp(*this)
        );

    DBG_(Trace, ( << "Network Received msg From " << msg->srcNode << " to " << msg->destNode << ", on vc " << msg->networkVC << ", serial: " << msg->serial << " Message =  " << *(transport[MemoryMessageTag]) ));

    if ( nc->insertMessage ( msg ) ) {
      throw Flexus::Core::FlexusException ( "MemoryNetwork: error inserting message to network" );
    }

  }

  std::pair<int, MemoryTransport> SerialTrans;
  std::map<const int, MemoryTransport> transports;

  NetContainer * nc;

  Stat::StatCounter theTotalMessages;
  Stat::StatCounter theTotalMessages_Data;
  Stat::StatCounter theTotalMessages_NoData;
  Stat::StatCounter theTotalHops;
  Stat::StatCounter theTotalFlitHops;
  Stat::StatCounter theDataFlitHops;
  Stat::StatCounter theOverheadFlitHops;
  Stat::StatCounter theTotalFlits;
  Stat::StatInstanceCounter<int64_t> theNetworkLatencies;

  std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram > > theNetworkLatencyHistograms;
  std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram > > theBufferTimes;
  std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram > > theAtHeadTimes;
  std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram > > theAcceptWaitTimes;

  Stat::StatMax     theMaxInfiniteBuffer;

};

} //End Namespace nNetwork

FLEXUS_COMPONENT_INSTANTIATOR( MemoryNetwork, nNetwork );
FLEXUS_PORT_ARRAY_WIDTH( MemoryNetwork, ToNode ) {
  return cfg.VChannels * cfg.NumNodes;
}
FLEXUS_PORT_ARRAY_WIDTH( MemoryNetwork, FromNode ) {
  return cfg.VChannels * cfg.NumNodes;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MemoryNetwork

#define DBG_Reset
#include DBG_Control()
