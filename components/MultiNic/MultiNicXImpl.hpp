
#if !BOOST_PP_IS_ITERATING

#ifndef FLEXUS_MULTI_NIC_NUMPORTS
#error "FLEXUS_MULTI_NIC_NUMPORTS must be defined"
#endif

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp> /* CMU-ONLY */
#include <components/CommonQEMU/Slices/NetworkMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/MultiNic/MultiNicX.hpp>
#include <core/stats.hpp>
#include <list>
#include <vector>

#define FLEXUS_BEGIN_COMPONENT MultiNicX
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DeclareCategories MultiNic
#define DBG_SetDefaultOps     AddCat(MultiNic)
#include DBG_Control()

namespace nMultiNic {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

class FLEXUS_COMPONENT(MultiNicX)
{
    FLEXUS_COMPONENT_IMPL(MultiNicX);

    // The received message queues (one for each virtual channel)
    std::vector<std::list<MemoryTransport>> recvQueue;

    // The send message queues (one for each port)
    std::vector<std::list<MemoryTransport>> sendQueue;

    // The number of messages in the receive queues
    int32_t currRecvCount;
    int32_t currSendCount;

    // statistics
    std::vector<boost::intrusive_ptr<Stat::StatMax>> theRQueueSizes;
    Stat::StatCounter theRecvCount;
    Stat::StatCounter theSendCount;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(MultiNicX)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
      , currRecvCount(0)
      , currSendCount(0)
      , theRecvCount("MsgsReceived", this)
      , theSendCount("MsgsSent", this)
    {
    }

    bool isQuiesced() const { return currRecvCount == 0 && currSendCount == 0; }

    // Initialization
    void initialize()
    {
        recvQueue.resize(cfg.VChannels);
        sendQueue.resize(FLEXUS_MULTI_NIC_NUMPORTS * cfg.VChannels);
        currRecvCount = 0;
        currSendCount = 0;

        for (int32_t i = 0; i < cfg.VChannels; i++) {
            theRQueueSizes.push_back(new Stat::StatMax("max rcv queue [vc:" + std::to_string(i) + "]", this));
        }
    }

    void finalize() {}

    // Ports
    bool available(interface::FromNetwork const&, index_t aVC) { return recvAvailable(aVC); }
    void push(interface::FromNetwork const&, index_t anIndex, MemoryTransport& transport)
    {
        DBG_(Iface,
             (<< "Recv from network: src=" << transport[NetworkMessageTag]->src
              << " dest=" << transport[NetworkMessageTag]->dest << " vc=" << transport[NetworkMessageTag]->vc
              << " src_port=" << transport[NetworkMessageTag]->src_port
              << " dest_port=" << transport[NetworkMessageTag]->dst_port) Comp(*this));
        DBG_(Iface,
             Condition((transport[MemoryMessageTag]))(<< "  Packet contains: " << *transport[MemoryMessageTag])
               Comp(*this));

        DBG_Assert(anIndex == (index_t)transport[NetworkMessageTag]->vc,
                   Comp(*this)(<< "Mismatched port/vc assignment"));

        recv(transport);
        ++theRecvCount;
    }

    // Generate the code for the ports from and to the node
    // The ports will be called FromNode0, FromNode1, ToNode0, etc.

#define BOOST_PP_ITERATION_LIMITS (0, FLEXUS_MULTI_NIC_NUMPORTS - 1)
#define BOOST_PP_FILENAME_1       "components/MultiNic/MultiNicXImpl.hpp"
#include BOOST_PP_ITERATE()

    // Drive Interfaces
    void drive(interface::MultiNicDrive const&)
    {
        // if there are received messages pending and the node can
        // accept a message, give it one
        if (currSendCount > 0) { msgFromNode(); }
        if (currRecvCount > 0) { msgToNode(); }
    }

  private:
    bool recvAvailable(index_t aVC)
    {
        // check if any of the receive queues are at capacity - if so,
        // return false; otherwise true.
        DBG_Assert(aVC >= 0 && aVC < (index_t)cfg.VChannels, Comp(*this)(<< "VC out of range: " << aVC));
        return (recvQueue[aVC].size() < cfg.RecvCapacity);
        // return true;  // infinite buffers
    }

    void recv(MemoryTransport& transport)
    {
        intrusive_ptr<NetworkMessage> msg = transport[NetworkMessageTag];
        DBG_Assert((msg->vc >= 0) && (msg->vc < cfg.VChannels), Comp(*this));
        recvQueue[msg->vc].push_back(transport);
        currRecvCount++;
        *(theRQueueSizes[msg->vc]) << currRecvCount;
    }

    void msgFromNode()
    {
        // try to get a message from the node
        for (int32_t ii = 0; ii < (FLEXUS_MULTI_NIC_NUMPORTS * cfg.VChannels); ++ii) {
            if (!sendQueue[ii].empty()) {
                if (static_cast<uint32_t>(sendQueue[ii].front()[NetworkMessageTag]->dest) == flexusIndex()) {
                    if (recvAvailable(sendQueue[ii].front()[NetworkMessageTag]->vc)) {
                        recv(sendQueue[ii].front());
                        sendQueue[ii].pop_front();
                        --currSendCount;
                    }
                } else {
                    if (FLEXUS_CHANNEL_ARRAY(ToNetwork, sendQueue[ii].front()[NetworkMessageTag]->vc).available()) {
                        MemoryTransport transport = sendQueue[ii].front();
                        sendQueue[ii].pop_front();
                        --currSendCount;
                        DBG_(Iface,
                             (<< "Send to network: src=" << transport[NetworkMessageTag]->src << " dest="
                              << transport[NetworkMessageTag]->dest << " vc=" << transport[NetworkMessageTag]->vc
                              << " size=" << transport[NetworkMessageTag]->size
                              << " src_port=" << transport[NetworkMessageTag]->src_port
                              << " dst_port=" << transport[NetworkMessageTag]->dst_port) Comp(*this));
                        DBG_(Iface,
                             Condition((transport[MemoryMessageTag]))(<< "  Packet contains: "
                                                                      << *transport[MemoryMessageTag]) Comp(*this));

                        FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
                        ++theSendCount;
                    }
                }
            }
        }
    }

    void msgToNode()
    {
        // try to give a message to the node, beginning with the
        // highest priority virtual channel
        for (int32_t ii = cfg.VChannels - 1; ii >= 0; ii--) {
            if (!recvQueue[ii].empty()) {
                switch (recvQueue[ii].front()[NetworkMessageTag]->dst_port) {
                    // Generate a switch case for each MultiNic port
#define BOOST_PP_LOCAL_MACRO(N)                                                                                        \
    case N:                                                                                                            \
        if (FLEXUS_CHANNEL_ARRAY(BOOST_PP_CAT(ToNode, N), ii).available()) {                                           \
            FLEXUS_CHANNEL_ARRAY(BOOST_PP_CAT(ToNode, N), ii) << recvQueue[ii].front();                                \
            recvQueue[ii].pop_front();                                                                                 \
            currRecvCount--;                                                                                           \
        }                                                                                                              \
        break; /**/
#define BOOST_PP_LOCAL_LIMITS (0, FLEXUS_MULTI_NIC_NUMPORTS - 1)
#include BOOST_PP_LOCAL_ITERATE()
                    default:
                        DBG_Assert(false,
                                   (<< name()
                                    << " Network message requesting delivery to non-existant "
                                       "destination port: "
                                    << recvQueue[ii].front()[NetworkMessageTag]->dst_port));
                }
            }
        }
    }
};

} // End Namespace nMultiNic

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MultiNicX

#define ToNode_ArrayWidth(z, N, _)                                                                                     \
    FLEXUS_PORT_ARRAY_WIDTH(MultiNicX, BOOST_PP_CAT(ToNode, N))                                                        \
    {                                                                                                                  \
        return cfg.VChannels;                                                                                          \
    }
#define FromNode_ArrayWidth(z, N, _)                                                                                   \
    FLEXUS_PORT_ARRAY_WIDTH(MultiNicX, BOOST_PP_CAT(FromNode, N))                                                      \
    {                                                                                                                  \
        return cfg.VChannels;                                                                                          \
    }

FLEXUS_COMPONENT_INSTANTIATOR(MultiNicX, nMultiNic);
FLEXUS_PORT_ARRAY_WIDTH(MultiNicX, ToNetwork)
{
    return cfg.VChannels;
}
FLEXUS_PORT_ARRAY_WIDTH(MultiNicX, FromNetwork)
{
    return cfg.VChannels;
}
BOOST_PP_REPEAT(FLEXUS_MULTI_NIC_NUMPORTS, ToNode_ArrayWidth, _)
BOOST_PP_REPEAT(FLEXUS_MULTI_NIC_NUMPORTS, FromNode_ArrayWidth, _)

#define DBG_Reset
#include DBG_Control()

#else // BOOST_PP_IS_ITERATING

#define N         BOOST_PP_ITERATION()
#define FromNodeN BOOST_PP_CAT(FromNode, N)

bool
available(interface::FromNodeN const&, index_t aVC)
{
    return (sendQueue[(N * cfg.VChannels) + aVC].size() < cfg.SendCapacity);
}
void
push(interface::FromNodeN const&, index_t aVC, MemoryTransport& transport)
{
    // Ensure src port is set correctly.
    DBG_Assert(transport[NetworkMessageTag]->src_port == N);
    DBG_Assert(transport[NetworkMessageTag]->vc == (int)aVC, (<< "wrong VC: " << *(transport[NetworkMessageTag])));

    sendQueue[(N * cfg.VChannels) + aVC].push_back(transport);
    currSendCount++;
}

#undef N
#undef FromNodeN

#endif // BOOST_PP_IS_ITERATING
