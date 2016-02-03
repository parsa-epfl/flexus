#if !BOOST_PP_IS_ITERATING

#ifndef FLEXUS_NIC_NUMPORTS
#error "FLEXUS_NIC_NUMPORTS must be defined"
#endif

#include <components/Nic/NicX.hpp>

#include <vector>
#include <list>

#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/cat.hpp>

#include <core/stats.hpp>

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/ProtocolMessage.hpp> /* CMU-ONLY */
#include <components/Common/Slices/TransactionTracker.hpp>

#define FLEXUS_BEGIN_COMPONENT NicX
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DeclareCategories Nic
#define DBG_SetDefaultOps AddCat(Nic)
#include DBG_Control()

namespace nNic {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

class FLEXUS_COMPONENT( NicX ) {
  FLEXUS_COMPONENT_IMPL( NicX );

  // The received message queues (one for each virtual channel)
  std::vector< std::list<NetworkTransport>  >   recvQueue;

  // The send message queues (one for each port)
  std::vector< std::list<NetworkTransport>  >   sendQueue;

  // The number of messages in the receive queues
  int32_t currRecvCount;
  int32_t currSendCount;

  // statistics
  std::vector<boost::intrusive_ptr<Stat::StatMax> > theRQueueSizes;
  Stat::StatCounter theRecvCount;
  Stat::StatCounter theSendCount;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(NicX)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , currRecvCount(0)
    , currSendCount(0)
    , theRecvCount("MsgsReceived", this)
    , theSendCount("MsgsSent", this)
  {}

  bool isQuiesced() const {
    return currRecvCount == 0 && currSendCount == 0;
  }

  // Initialization
  void initialize() {
    recvQueue.resize( cfg.VChannels );
    sendQueue.resize( FLEXUS_NIC_NUMPORTS );
    currRecvCount = 0;
    currSendCount = 0;

    for (int32_t i = 0; i < cfg.VChannels; i++) {
      theRQueueSizes.push_back(new Stat::StatMax("max rcv queue [vc:" + std::to_string(i) + "]", this));
    }
  }

  // Ports
  bool available( interface::FromNetwork const &, index_t aVC) {
    return recvAvailable(aVC);
  }
  void push( interface::FromNetwork const &, index_t anIndex, NetworkTransport & transport) {
    DBG_(Iface, ( << "Recv from network: src=" << transport[NetworkMessageTag]->src
                  << " dest=" << transport[NetworkMessageTag]->dest
                  << " vc=" << transport[NetworkMessageTag]->vc
                  << " src_port=" << transport[NetworkMessageTag]->src_port
                  << " dest_port=" << transport[NetworkMessageTag]->dst_port
                )
         Comp(*this)
        );
    DBG_(Iface, Condition((transport[ProtocolMessageTag]))
         ( << "  Packet contains: " << *transport[ProtocolMessageTag])
         Comp(*this)
        );

    DBG_Assert(anIndex == (index_t)transport[NetworkMessageTag]->vc, Comp(*this) ( << "Mismatched port/vc assignment"));

    recv(transport);
    ++theRecvCount;
  }

  //Generate the code for the ports from and to the node
  //The ports will be called FromNode0, FromNode1, ToNode0, etc.

#define BOOST_PP_ITERATION_LIMITS (0, FLEXUS_NIC_NUMPORTS - 1)
#define BOOST_PP_FILENAME_1 "components/Nic/NicXImpl.hpp"
#include BOOST_PP_ITERATE()

  //Drive Interfaces
  void drive( interface::NicDrive const &) {
    // if there are received messages pending and the node can
    // accept a message, give it one
    if (currSendCount > 0) {
      msgFromNode();
    }
    if (currRecvCount > 0) {
      msgToNode();
    }
  }

private:

  bool recvAvailable(index_t aVC) {
    // check if any of the receive queues are at capacity - if so,
    // return false; otherwise true.
    DBG_Assert(aVC >= 0 && aVC < (index_t)cfg.VChannels, Comp(*this) ( << "VC out of range: " << aVC));
    return (recvQueue[aVC].size() < cfg.RecvCapacity);
    //return true;  // infinite buffers
  }

  void recv(NetworkTransport & transport) {
    intrusive_ptr<NetworkMessage> msg = transport[NetworkMessageTag];
    DBG_Assert( (msg->vc >= 0) && (msg->vc < cfg.VChannels), Comp(*this) );
    recvQueue[msg->vc].push_back(transport);
    currRecvCount++;
    *(theRQueueSizes[msg->vc]) << currRecvCount;
  }

  void msgFromNode() {
    // try to give a message to the node, beginning with the
    // highest priority virtual channel
    for (int32_t ii = 0; ii <  FLEXUS_NIC_NUMPORTS; ++ii) {
      if (!sendQueue[ii].empty()) {
        if (static_cast<uint32_t>(sendQueue[ii].front()[NetworkMessageTag]->dest) == flexusIndex()) {
          if (recvAvailable(sendQueue[ii].front()[NetworkMessageTag]->vc)) {
            recv(sendQueue[ii].front());
            sendQueue[ii].pop_front();
            -- currSendCount;
          }
        } else {
          if (FLEXUS_CHANNEL_ARRAY( ToNetwork, sendQueue[ii].front()[NetworkMessageTag]->vc).available()) {
            NetworkTransport transport = sendQueue[ii].front();
            sendQueue[ii].pop_front();
            -- currSendCount;
            DBG_(Iface, ( << "Send to network: src=" << transport[NetworkMessageTag]->src
                          << " dest=" << transport[NetworkMessageTag]->dest
                          << " vc=" << transport[NetworkMessageTag]->vc
                          << " size=" << transport[NetworkMessageTag]->size
                          << " src_port=" << transport[NetworkMessageTag]->src_port
                          << " dst_port=" << transport[NetworkMessageTag]->dst_port
                        )
                 Comp(*this) );
            DBG_(Iface, Condition((transport[ProtocolMessageTag]))
                 ( << "  Packet contains: " << *transport[ProtocolMessageTag])
                 Comp(*this)
                );

            FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
            ++theSendCount;
          }
        }
      }
    }
  }

  void msgToNode() {
    // try to give a message to the node, beginning with the
    // highest priority virtual channel
    for (int32_t ii = cfg.VChannels - 1; ii >= 0; ii--) {
      if (!recvQueue[ii].empty()) {
        switch ( recvQueue[ii].front()[NetworkMessageTag]->dst_port ) {
            //Generate a switch case for each Nic port
#define BOOST_PP_LOCAL_MACRO(N)                                                           \
            case N:                                                                                 \
              if (FLEXUS_CHANNEL_ARRAY(BOOST_PP_CAT(ToNode,N), ii).available()) {          \
                FLEXUS_CHANNEL_ARRAY(BOOST_PP_CAT(ToNode,N), ii) << recvQueue[ii].front(); \
                recvQueue[ii].pop_front();                                                          \
                currRecvCount--;                                                                    \
              }                                                                                     \
              break;                                                                                /**/
#define BOOST_PP_LOCAL_LIMITS (0, FLEXUS_NIC_NUMPORTS -1)
#include BOOST_PP_LOCAL_ITERATE()
          default:
            DBG_Assert( false, ( << name() << " Network message requesting delivery to non-existant destination port: " << recvQueue[ii].front()[NetworkMessageTag]->dst_port ) );
        }
      }
    }
  }

};

} //End Namespace nNic

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NicX

#define ToNode_ArrayWidth(z,N,_) FLEXUS_PORT_ARRAY_WIDTH( NicX, BOOST_PP_CAT(ToNode,N) ) { return cfg.VChannels; }

FLEXUS_COMPONENT_INSTANTIATOR( NicX, nNic);
FLEXUS_PORT_ARRAY_WIDTH( NicX, ToNetwork ) {
  return cfg.VChannels;
}
FLEXUS_PORT_ARRAY_WIDTH( NicX, FromNetwork ) {
  return cfg.VChannels;
}
BOOST_PP_REPEAT(FLEXUS_NIC_NUMPORTS, ToNode_ArrayWidth, _ )

#define DBG_Reset
#include DBG_Control()

#else //BOOST_PP_IS_ITERATING

#define N BOOST_PP_ITERATION()
#define FromNodeN BOOST_PP_CAT(FromNode,N)

bool available( interface::FromNodeN const &) {
  return (sendQueue[N].size() < cfg.SendCapacity);
}
void push( interface::FromNodeN const &, NetworkTransport & transport) {
  //Ensure src port is set correctly.
  DBG_Assert(transport[NetworkMessageTag]->src_port == N );

  sendQueue[N].push_back(transport);
  currSendCount++;
}

#undef N
#undef FromNodeN

#endif //BOOST_PP_IS_ITERATING
