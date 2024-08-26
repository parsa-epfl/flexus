//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#include <components/CommonQEMU/Slices/DirectoryEntry.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/NetworkMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/NetShim/MemoryNetwork.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/flexus.hpp>
#include <core/stats.hpp>
#include <fstream>

#define FLEXUS_BEGIN_COMPONENT MemoryNetwork
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "netcontainer.hpp"

#define DBG_DefineCategories MemoryNetwork
#define DBG_SetDefaultOps    AddCat(MemoryNetwork)
#include DBG_Control()

namespace nNetwork {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;
using namespace nNetShim;

class FLEXUS_COMPONENT(MemoryNetwork)
{
    FLEXUS_COMPONENT_IMPL(MemoryNetwork);

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(MemoryNetwork)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
      , theTotalMessages("Network Messages Received", this)
      , theTotalMessages_Data("Network Messages Received:Data", this)
      , theTotalMessages_NoData("Network Messages Received:NoData", this)
      , theTotalHops("Network Message Hops", this)
      , theTotalFlitHops("Network:Flit-Hops:Total", this)
      , theDataFlitHops("Network:Flit-Hops:Data", this)
      , theOverheadFlitHops("Network:Flit-Hops:Overhead", this)
      , theTotalFlits("Network:Flits:Total", this)
      , theNetworkLatencies("Network:Latencies", this)
      , theMaxInfiniteBuffer("Maximum network input buffer depth", this)
    {
        // nc = new NetContainer();
    }

    // added by mehdi
    long long latency;
    long PacketCount;
    // mehdi

    bool isQuiesced() const { return transports.empty(); }

    // Initialization
    void initialize()
    {
        int i;
        // mehdi
        latency     = 0;
        PacketCount = 0;
        // end of medhi

        if (cfg.NumNodes == 0) cfg.NumNodes = Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 3;

        for (i = 0; i < cfg.VChannels; i++) {
            theNetworkLatencyHistograms.push_back(
              new Stat::StatLog2Histogram("NetworkLatency   VC[" + std::to_string(i) + "]", this));
            theBufferTimes.push_back(
              new Stat::StatLog2Histogram("BufferTime       VC[" + std::to_string(i) + "]", this));
            theAtHeadTimes.push_back(
              new Stat::StatLog2Histogram("AtBufferHeadTime VC[" + std::to_string(i) + "]", this));
            theAcceptWaitTimes.push_back(
              new Stat::StatLog2Histogram("AcceptWaitTime   VC[" + std::to_string(i) + "]", this));
        }

        nc = new NetContainer();
        if (nc->buildNetwork(cfg.NetworkTopologyFile.c_str())) {
            throw Flexus::Core::FlexusException("Error building the network");
        }
    }

    void finalize() {}

    // Ports
    bool available(interface::FromNode const&, index_t anIndex)
    {
        int32_t node = anIndex / cfg.VChannels;
        int32_t vc   = anIndex % cfg.VChannels;
        vc           = MAX_PROT_VC - vc - 1;
        DBG_(VVerb,
             Comp(*this)(<< "Check network availability for node: " << node << " vc: " << vc << " -> "
                         << nc->isNodeOutputAvailable(node, vc)));
        return nc->isNodeOutputAvailable(node, vc);
    }
    void push(interface::FromNode const&, index_t anIndex, MemoryTransport& transport)
    {
        DBG_Assert((transport[NetworkMessageTag]->src == static_cast<int>(anIndex / cfg.VChannels)),
                   (<< "tp->src " << transport[NetworkMessageTag]->src << "%src " << anIndex / cfg.VChannels
                    << "anIndex " << anIndex)); // static_cast to suppress warning about
                                                // signed/unsigned comparison
        DBG_Assert(((transport[NetworkMessageTag]->vc ==
                     static_cast<int>(anIndex % cfg.VChannels)))); // static_cast to suppress warning
                                                                   // about signed/unsigned comparison
        newPacket(transport);
    }

    // Drive Interfaces
    void drive(interface::NetworkDrive const&)
    {
        nNetShim::currTime = Flexus::Core::theFlexus->cycleCount();

        if (nNetShim::currTime == 149999) { double avg_latency = double(latency) / double(PacketCount); }

        // We need to use function objects for calls back into simics from the
        // NetShim. In particular, checking if a node will accept a message
        // (.available()) and final delivery of a message must be function objects
        // for now.
        if (!theAvail) {
            theAvail = //[this](auto x, auto y){ return this->isNodeAvailable(x,y); };
              ll::bind(&MemoryNetworkComponent::isNodeAvailable, this, ll::_1, ll::_2);

            theDeliver = //[this](auto x){ return this->deliverMessage(x); };
              ll::bind(&MemoryNetworkComponent::deliverMessage, this, ll::_1);

            nc->setCallbacks(theAvail, theDeliver);
        }

        // Kick the network simulation for one cycle
        if (nc->drive()) { throw Flexus::Core::FlexusException("MemoryNetwork error in drive()"); }

        // This could be a waste of time.  Use it as a monitor to see if any
        // of the infinite network input queues get too deep.  If this isn't common,
        // it may be worthwhile to comment this out.
        theMaxInfiniteBuffer << nc->getMaximumInfiniteBufferDepth();
    }

  public:
    // Interface to the NetContainer
    std::function<bool(const int, const int)> theAvail;
    std::function<bool(const MessageState*)> theDeliver;

    // Can another message be removed from the network?
    // Encapsulated in a function object "theAvail" to call from outside code
    bool isNodeAvailable(const int32_t node, const int32_t vc) const
    {
        int32_t real_net_vc = MAX_PROT_VC - vc - 1;
        index_t pdest       = (node)*cfg.VChannels + real_net_vc;
        DBG_(Iface,
             (<< "netmessage: available? "
              << "node: " << node << " vc: " << real_net_vc << " pdest: " << pdest));
        return FLEXUS_CHANNEL_ARRAY(ToNode, pdest).available();
    }

    // Set a particular message as delivered, finish any statistics related to the
    // message and deliver to the destination node. Encapsulated in a function
    // object "theDeliver" to call from outside code
    bool deliverMessage(const MessageState* msg)
    {
        index_t pdest = (transports[msg->serial][NetworkMessageTag]->dest) * cfg.VChannels +
                        transports[msg->serial][NetworkMessageTag]->vc;

        int32_t vc = transports[msg->serial][NetworkMessageTag]->vc;

        DBG_(Trace,
             (<< "Network Delivering msg From " << msg->srcNode << " to " << msg->destNode << ", on vc "
              << msg->networkVC << ", serial: " << msg->serial
              << " Message =  " << *(transports[msg->serial][MemoryMessageTag])));

        FLEXUS_CHANNEL_ARRAY(ToNode, pdest) << transports[msg->serial];

        ++theTotalMessages;
        if (transports[msg->serial][NetworkMessageTag]->size > 8) {
            ++theTotalMessages_Data;
        } else {
            ++theTotalMessages_NoData;
        }

        // transmitLatency now equals flits, go back to NetworkMessageTag to detmine
        // actual size
        theTotalFlits += msg->transmitLatency;
        theTotalFlitHops += (msg->hopCount * msg->transmitLatency);
        if (transports[msg->serial][NetworkMessageTag]->size > 8) {
            theDataFlitHops += msg->hopCount * msg->transmitLatency;
            // No consistent way to identify how many flits used for non-data portion
            // of data message so just put entire message in the data bin.
        } else {
            theOverheadFlitHops += msg->hopCount * msg->transmitLatency;
        }

        theTotalHops += msg->hopCount;
        *theNetworkLatencyHistograms[vc] << Flexus::Core::theFlexus->cycleCount() - msg->startTS;

        latency += Flexus::Core::theFlexus->cycleCount() - msg->startTS;
        PacketCount++;

        theNetworkLatencies << std::make_pair(((int64_t)Flexus::Core::theFlexus->cycleCount() - msg->startTS),
                                              1); // mammad

        /*
            *theBufferTimes[vc]     << std::make_pair ( msg->bufferTime, 1 );

            *theAtHeadTimes[vc]     << std::make_pair ( msg->atHeadTime, 1 );

            *theAcceptWaitTimes[vc] << std::make_pair ( msg->acceptTime, 1 );
        */

        transports.erase(msg->serial);

        return false;
    }

  private:
    // Add a new transport to the network from a node
    void newPacket(MemoryTransport& transport)
    {

        MessageState* msg;

        DBG_Assert(transport[NetworkMessageTag]);

        // Ensure all NetworkMessage fields have been initialized
        DBG_Assert(transport[NetworkMessageTag]->src != -1, (<< "No src for " << *(transport[MemoryMessageTag])));
        DBG_Assert(transport[NetworkMessageTag]->dest != -1, (<< "No src for " << *(transport[MemoryMessageTag])));
        DBG_Assert(transport[NetworkMessageTag]->vc != -1, (<< "No src for " << *(transport[MemoryMessageTag])));
        DBG_Assert(transport[NetworkMessageTag]->size != -1, (<< "No src for " << *(transport[MemoryMessageTag])));
        DBG_Assert(transport[NetworkMessageTag]->src_port != -1, (<< "No src for " << *(transport[MemoryMessageTag])));
        DBG_Assert(transport[NetworkMessageTag]->dst_port != -1, (<< "No src for " << *(transport[MemoryMessageTag])));

        // Allocate and initialize the internal NetShim simulator message
        // state and send it into the interconnect.
        msg = allocMessageState();

        msg->srcNode  = transport[NetworkMessageTag]->src;
        msg->destNode = transport[NetworkMessageTag]->dest;
        msg->priority = MAX_PROT_VC - transport[NetworkMessageTag]->vc -
                        1; // Note, this field really needs to be added to the NetworkMessage
        msg->networkVC        = 0;
        msg->transmitLatency  = transport[NetworkMessageTag]->size;
        msg->flexusInFastMode = Flexus::Core::theFlexus->isFastMode();
        msg->hopCount         = -1; // Note, the local switch also gets counted, so we start at -1
        msg->startTS          = Flexus::Core::theFlexus->cycleCount();
        msg->myList           = nullptr;

        if (transport[TransactionTrackerTag]) {
            std::string cause(boost::padded_string_cast<3, '0'>(transport[NetworkMessageTag]->src) + " -> " +
                              boost::padded_string_cast<3, '0'>(transport[NetworkMessageTag]->dest));

            transport[TransactionTrackerTag]->setDelayCause("Network", cause);
        }

        // We index the actual transport object through a map of serial numbers
        // (assigned when the MessageState object is allocated) to transports
        transports.insert(make_pair(msg->serial, transport));

        DBG_(Iface,
             (<< "New packet: "
              << " serial=" << msg->serial << " src=" << transport[NetworkMessageTag]->src
              << " dest=" << transport[NetworkMessageTag]->dest << " vc=" << transport[NetworkMessageTag]->vc
              << " src_port=" << transport[NetworkMessageTag]->src_port
              << " dest_port=" << transport[NetworkMessageTag]->dst_port) Comp(*this));

        DBG_(Trace,
             (<< "Network Received msg From " << msg->srcNode << " to " << msg->destNode << ", on vc " << msg->networkVC
              << ", serial: " << msg->serial << " Message =  " << *(transport[MemoryMessageTag])));

        if (nc->insertMessage(msg)) {
            throw Flexus::Core::FlexusException("MemoryNetwork: error inserting message to network");
        }
    }

    std::pair<int, MemoryTransport> SerialTrans;
    std::map<const int, MemoryTransport> transports;

    NetContainer* nc;

    Stat::StatCounter theTotalMessages;
    Stat::StatCounter theTotalMessages_Data;
    Stat::StatCounter theTotalMessages_NoData;
    Stat::StatCounter theTotalHops;
    Stat::StatCounter theTotalFlitHops;
    Stat::StatCounter theDataFlitHops;
    Stat::StatCounter theOverheadFlitHops;
    Stat::StatCounter theTotalFlits;
    Stat::StatInstanceCounter<int64_t> theNetworkLatencies;

    std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram>> theNetworkLatencyHistograms;
    std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram>> theBufferTimes;
    std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram>> theAtHeadTimes;
    std::vector<boost::intrusive_ptr<Stat::StatLog2Histogram>> theAcceptWaitTimes;

    Stat::StatMax theMaxInfiniteBuffer;
};

} // End Namespace nNetwork

FLEXUS_COMPONENT_INSTANTIATOR(MemoryNetwork, nNetwork);
FLEXUS_PORT_ARRAY_WIDTH(MemoryNetwork, ToNode)
{
    return cfg.VChannels * cfg.NumNodes;
}
FLEXUS_PORT_ARRAY_WIDTH(MemoryNetwork, FromNode)
{
    return cfg.VChannels * cfg.NumNodes;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MemoryNetwork

#define DBG_Reset
#include DBG_Control()