#include <components/CMPCoreNetworkInterface/CMPCoreNetworkInterface.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <vector>
#include <list>

#define FLEXUS_BEGIN_COMPONENT CMPCoreNetworkInterface
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories CMPCoreNetworkInterface
#define DBG_SetDefaultOps AddCat(CMPCoreNetworkInterface)
#include DBG_Control()

namespace nCMPCoreNetworkInterface {

using namespace Flexus;
using namespace Core;

class FLEXUS_COMPONENT(CMPCoreNetworkInterface) {
  FLEXUS_COMPONENT_IMPL(CMPCoreNetworkInterface);

private:
  std::vector< std::list< MemoryTransport > > recvQueue; // One per VC
  std::list< MemoryTransport > sendQueue; // One per port, only one port

  // The number of messages in the queues
  uint32_t currRecvCount;
  uint32_t currSendCount;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CMPCoreNetworkInterface)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) { }

  bool isQuiesced() const {
    return (!currRecvCount && !currSendCount);
  }

  // Initialization
  void initialize() {
    recvQueue.resize(cfg.VChannels);
    currRecvCount = currSendCount = 0;
  }

//NEED TO CHECK THIS!!!
  void finalize() override {

  }

  // Ports
  bool available(interface::DRequestFromCore const &) {
    return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::DRequestFromCore const &, MemoryTransport & transport) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );

    transport[NetworkMessageTag]->vc = cfg.RequestVc;
    transport[MemoryMessageTag]->coreNumber() = flexusIndex();
    transport[MemoryMessageTag]->dstream() = true;

    sendQueue.push_back( transport );
    ++currSendCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted data request " << *transport[MemoryMessageTag]
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " from cpu[" << flexusIndex() << "]"));
  }

  bool available(interface::DSnoopFromCore const &) {
    return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::DSnoopFromCore const &, MemoryTransport & transport) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );

    transport[NetworkMessageTag]->vc = cfg.SnoopVc;
    if (!transport[MemoryMessageTag]->isPurge()) { /* CMU-ONLY */
      transport[MemoryMessageTag]->coreNumber() = flexusIndex();
      transport[MemoryMessageTag]->dstream() = true;
    }

    sendQueue.push_back( transport );
    ++currSendCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted data snoop " << *transport[MemoryMessageTag]
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " from cpu[" << flexusIndex() << "]"));
  }

  bool available(interface::IRequestFromCore const &) {
    return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::IRequestFromCore const &, MemoryTransport & transport) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );

    transport[NetworkMessageTag]->vc = cfg.RequestVc;
    transport[MemoryMessageTag]->coreNumber() = flexusIndex();
    transport[MemoryMessageTag]->dstream() = false;

    sendQueue.push_back( transport );
    ++currSendCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted instruction request " << *transport[MemoryMessageTag]
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " from cpu[" << flexusIndex() << "]"));
  }

  bool available(interface::ISnoopFromCore const &) {
    return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::ISnoopFromCore const &, MemoryTransport & transport) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );

    transport[NetworkMessageTag]->vc = cfg.SnoopVc;
    if (!transport[MemoryMessageTag]->isPurge()) { /* CMU-ONLY */
      transport[MemoryMessageTag]->coreNumber() = flexusIndex();
      transport[MemoryMessageTag]->dstream() = false;
    }

    sendQueue.push_back( transport );
    ++currSendCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted instruction snoop " << *transport[MemoryMessageTag]
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " from cpu[" << flexusIndex() << "]"));
  }

  bool available(interface::FromNetwork const &, index_t aVC) {
    return (recvQueue[aVC].size() < cfg.RecvCapacity);
  }
  void push(interface::FromNetwork const &, index_t aVC, MemoryTransport & transport) {
    DBG_Assert(aVC == (index_t)transport[NetworkMessageTag]->vc, Comp(*this) ( << "Mismatched port/vc assignment"));

    recvQueue[aVC].push_back( transport );
    ++currRecvCount;
    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted data reply " << *transport[MemoryMessageTag]
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " on vc[" << aVC << "]"
           << " for cpu[" << flexusIndex() << "]"));
  }

  //Drive Interfaces
  void drive( interface::CMPCoreNetworkInterfaceDrive const &) {
    // Handle any pending messages in the queues
    if (currSendCount > 0) {
      msgFromNode();
    }
    if (currRecvCount > 0) {
      msgToNode();
    }
  }

private:
  void msgFromNode() {
    // try to give a message to the network
    if (!sendQueue.empty()) {
      if (FLEXUS_CHANNEL_ARRAY(ToNetwork, sendQueue.front()[NetworkMessageTag]->vc).available()) {
        MemoryTransport transport = sendQueue.front();
        sendQueue.pop_front();

        --currSendCount;
        FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
      }
    }
  }

  void msgToNode() {
    int32_t i;

    // try to give a message to the node, beginning with the highest priority virtual channelz
    for (i = cfg.VChannels - 1; i >= 0; --i) {
      if (!recvQueue[i].empty()) {
        MemoryTransport transport = recvQueue[i].front();

        // Request to L1 from network (snoops and ReturnReq)
        if (transport[MemoryMessageTag]->isExtReqType()) {
          // Route to L1D
          if (transport[MemoryMessageTag]->isDstream()) {
            if (FLEXUS_CHANNEL(DRequestToCore).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending data request " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " into cpu[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL(DRequestToCore) << transport;
            }
          }
          // Route to L1I
          else {
            if (FLEXUS_CHANNEL(IRequestToCore).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending instruction request " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " into cpu[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL(IRequestToCore) << transport;
            }
          }
        }

        // reply to L1 from network
        else {
          // Route to L1D
          if (transport[MemoryMessageTag]->isDstream()) {
            if (FLEXUS_CHANNEL(DReplyToCore).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending data reply " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " into cpu[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL(DReplyToCore) << transport;
            }
          }
          // Route to L1I
          else {
            if (FLEXUS_CHANNEL(IReplyToCore).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending instruction reply " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " into cpu[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL(IReplyToCore) << transport;
            }
          }
        }
      }
    }
  }
};

} //End Namespace nCMPCoreNetworkInterface

FLEXUS_COMPONENT_INSTANTIATOR( CMPCoreNetworkInterface, nCMPCoreNetworkInterface );

FLEXUS_PORT_ARRAY_WIDTH( CMPCoreNetworkInterface, ToNetwork ) {
  return cfg.VChannels;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPCoreNetworkInterface, FromNetwork ) {
  return cfg.VChannels;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPCoreNetworkInterface

#define DBG_Reset
#include DBG_Control()
