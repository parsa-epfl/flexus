#include <components/CMPMCNetworkInterface/CMPMCNetworkInterface.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPMCNetworkInterface
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories CMPMCNetworkInterface
#define DBG_SetDefaultOps AddCat(CMPMCNetworkInterface)
#include DBG_Control()

namespace nCMPMCNetworkInterface {

using namespace Flexus;
using namespace Core;

class FLEXUS_COMPONENT(CMPMCNetworkInterface) {
  FLEXUS_COMPONENT_IMPL(CMPMCNetworkInterface);

private:
  std::vector< std::list<MemoryTransport> > NetRecvQueue; // One per VC
  std::list<MemoryTransport> NetSendQueue;    // One per port, only one port

  std::list<MemoryTransport> MCRecvQueue; // One per memory controller, only one memory controller // FIXME

  // The number of messages in the queues
  uint32_t currNetRecvCount;
  uint32_t currNetSendCount;
  uint32_t currMCRecvCount;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CMPMCNetworkInterface)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) { }

  bool isQuiesced() const {
    return (!currNetRecvCount && !currNetSendCount && !currMCRecvCount);
  }

  // Initialization
  void initialize() {
    NetRecvQueue.resize(cfg.VChannels);
    currNetRecvCount = currNetSendCount = currMCRecvCount = 0;
  }

  // Ports
  // L2 misses
  bool available(interface::RequestFromL2 const &
                 , index_t anL2Number
                ) {
    return (NetSendQueue.size() < cfg.NetSendCapacity);
  }
  void push(interface::RequestFromL2 const &
            , index_t anL2Number
            , MemoryTransport & transport
           ) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[ NetworkMessageTag ]->vc = cfg.RequestVc;
    transport[ MemoryMessageTag ]->l2Number() = anL2Number;

    NetSendQueue.push_back( transport );
    ++currNetSendCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << " accepted request from L2[" << anL2Number << "]"
           << " msg: " << (*transport[MemoryMessageTag])
         ));
  }

  // L2 snoops
  bool available(interface::SnoopFromL2 const &
                 , index_t anL2Number
                ) {
    return (NetSendQueue.size() < cfg.NetSendCapacity);
  }
  void push(interface::SnoopFromL2 const &
            , index_t anL2Number
            , MemoryTransport & transport
           ) {
    DBG_Assert( false );
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[ NetworkMessageTag ]->vc = cfg.SnoopVc;
    transport[ MemoryMessageTag ]->l2Number() = anL2Number;

    NetSendQueue.push_back( transport );
    ++currNetSendCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << " accepted snoop from L2[" << anL2Number << "]"
           << " msg: " << (*transport[MemoryMessageTag])
         ));
  }

  // reply from memory
  bool available(interface::ReplyFromMem const &
                 , index_t anL2Number
                ) {
    return (MCRecvQueue.size() < cfg.MCQueueCapacity);
  }
  void push( interface::ReplyFromMem const &
             , index_t anL2Number
             , MemoryTransport & transport
           ) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[ NetworkMessageTag ]->vc = cfg.ReplyVc;
    transport[ MemoryMessageTag ]->l2Number() = anL2Number;

    MCRecvQueue.push_back( transport );
    ++currMCRecvCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << " accepted reply from memory msg: " << (*transport[MemoryMessageTag])
         ));
  }

  // requests/snoops from network
  bool available( interface::FromNetwork const &
                  , index_t anIndex
                ) {
    const uint32_t aVC  = anIndex % cfg.VChannels;
    return (NetRecvQueue[aVC].size() < cfg.NetRecvCapacity);
  }
  void push( interface::FromNetwork const &
             , index_t anIndex
             , MemoryTransport & transport
           ) {
    const int32_t aVC  = anIndex % cfg.VChannels;
    DBG_Assert(aVC == transport[ NetworkMessageTag ]->vc, Comp(*this) ( << "Mismatched port/vc assignment"));

    NetRecvQueue[aVC].push_back( transport );
    ++currNetRecvCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << " accepted msg from network msg: " << (*transport[MemoryMessageTag])
         ));
  }

  //Drive Interfaces
  void drive( interface::CMPMCNetworkInterfaceDrive const &) {
    // Handle any pending messages in the queues
    if (currMCRecvCount > 0) {
      replyFromMem();
    }
    if (currNetSendCount > 0) {
      msgFromNode();
    }
    if (currNetRecvCount > 0) {
      msgFromNet();
    }
  }

private:
  void replyFromMem() {
    // try to send a reply from the memory
    if (!MCRecvQueue.empty()) {
      const uint32_t anL2 = MCRecvQueue.front()[ MemoryMessageTag ]->getL2Number();
      const uint32_t aVC  = MCRecvQueue.front()[ NetworkMessageTag ]->vc;
      const uint32_t anIndex = anL2 * cfg.VChannels + aVC;

      if (FLEXUS_CHANNEL_ARRAY(ToNetwork, anIndex).available()) {
        MemoryTransport transport = MCRecvQueue.front();
        MCRecvQueue.pop_front();

        --currMCRecvCount;

        DBG_( Iface,
              Addr(transport[MemoryMessageTag]->address())
              ( << " sending reply to network towards L2[" << anL2 << "]"
                << " vc: " << aVC
                << " via idx: " << anIndex
                << " msg: " << (*transport[MemoryMessageTag])
              ));
        FLEXUS_CHANNEL_ARRAY(ToNetwork, anIndex) << transport;
      }
    }
  }

  void msgFromNode() {
    // try to give a message to the network
    if (!NetSendQueue.empty()) {
      const uint32_t anL2 = NetSendQueue.front()[ MemoryMessageTag ]->getL2Number();
      const uint32_t aVC  = NetSendQueue.front()[ NetworkMessageTag ]->vc;
      const uint32_t anIndex = anL2 * cfg.VChannels + aVC;

      if (FLEXUS_CHANNEL_ARRAY( ToNetwork, anIndex ).available()) {
        MemoryTransport transport = NetSendQueue.front();
        NetSendQueue.pop_front();

        --currNetSendCount;

        DBG_( Iface,
              Addr(transport[MemoryMessageTag]->address())
              ( << " sending request to network from L2[" << anL2 << "]"
                << " vc: " << aVC
                << " via idx: " << anIndex
                << " msg: " << (*transport[MemoryMessageTag])
              ));
        FLEXUS_CHANNEL_ARRAY( ToNetwork, anIndex ) << transport;
      }
    }
  }

  void msgFromNet() {
    int32_t i;
    int32_t l2Number;

    // try to give a message to the node, beginning with the highest priority virtual channel
    for (i = cfg.VChannels - 1; i >= 0; --i) {
      if (!NetRecvQueue[i].empty()) {
        l2Number = NetRecvQueue[i].front()[MemoryMessageTag]->getL2Number();

        DBG_Assert(!NetRecvQueue[i].front()[MemoryMessageTag]->isPrefetchType(), ( << "CMP interconnect does not support prefetch messages!"));

        // ReplyToL2
        if (!NetRecvQueue[i].front()[MemoryMessageTag]->isRequest()) {
          if (FLEXUS_CHANNEL_ARRAY(ReplyToL2, l2Number).available()) {
            MemoryTransport transport = NetRecvQueue[i].front();
            NetRecvQueue[i].pop_front();

            --currNetRecvCount;
            DBG_( Iface,
                  Addr(transport[MemoryMessageTag]->address())
                  ( << " sending reply to L2[" << l2Number << "]"
                    << " msg: " << (*transport[MemoryMessageTag])
                  ));
            FLEXUS_CHANNEL_ARRAY(ReplyToL2, l2Number) << transport;
          }
        }

        // SnoopToL2
        else if (NetRecvQueue[i].front()[MemoryMessageTag]->isExtReqType()) {
          if (FLEXUS_CHANNEL_ARRAY(SnoopToL2, l2Number).available()) {
            MemoryTransport transport = NetRecvQueue[i].front();
            NetRecvQueue[i].pop_front();

            --currNetRecvCount;
            DBG_( Iface,
                  Addr(transport[MemoryMessageTag]->address())
                  ( << " sending snoop to L2[" << l2Number << "]"
                    << " msg: " << (*transport[MemoryMessageTag])
                  ));
            FLEXUS_CHANNEL_ARRAY(SnoopToL2, l2Number) << transport;
          }
        }

        // Request to mem
        else {
          if (FLEXUS_CHANNEL_ARRAY(RequestToMem, l2Number).available()) {
            MemoryTransport transport = NetRecvQueue[i].front();
            NetRecvQueue[i].pop_front();

            --currNetRecvCount;
            DBG_( Iface,
                  Addr(transport[MemoryMessageTag]->address())
                  ( << " sending request to memory from L2[" << l2Number << "]"
                    << " msg: " << (*transport[MemoryMessageTag])
                  ));
            FLEXUS_CHANNEL_ARRAY(RequestToMem, l2Number) << transport;
          }
        }

      }
    }
  }
};

} //End Namespace nCMPMCNetworkInterface

FLEXUS_COMPONENT_INSTANTIATOR( CMPMCNetworkInterface, nCMPMCNetworkInterface );

FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, RequestFromL2 ) {
  return cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, ReplyToL2 )     {
  return cfg.NumL2Tiles;
}

FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, RequestToMem )  {
  return cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, ReplyFromMem )  {
  return cfg.NumL2Tiles;
}

FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, SnoopFromL2 )   {
  return cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, SnoopToL2 )     {
  return cfg.NumL2Tiles;
}

FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, ToNetwork )     {
  return cfg.VChannels * cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPMCNetworkInterface, FromNetwork )   {
  return cfg.VChannels * cfg.NumL2Tiles;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPMCNetworkInterface

#define DBG_Reset
#include DBG_Control()
