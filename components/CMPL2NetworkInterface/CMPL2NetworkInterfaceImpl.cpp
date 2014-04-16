#include <components/CMPL2NetworkInterface/CMPL2NetworkInterface.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/CachePlacementPolicyDefn.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#define FLEXUS_BEGIN_COMPONENT CMPL2NetworkInterface
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories CMPL2NetworkInterface
#define DBG_SetDefaultOps AddCat(CMPL2NetworkInterface)
#include DBG_Control()

namespace nCMPL2NetworkInterface {

using namespace Flexus;
using namespace Core;

class FLEXUS_COMPONENT(CMPL2NetworkInterface) {
  FLEXUS_COMPONENT_IMPL(CMPL2NetworkInterface);

private:
  std::vector< std::list<MemoryTransport> > recvQueue;         // One per VC
  std::vector< std::list<MemoryTransport> > recvMemBusQueue;   // One per VC
  std::vector< std::list<MemoryTransport> > sendFrontBusQueue; // One per port+VC, only one port
  std::vector< std::list<MemoryTransport> > sendBackBusQueue;  // One per port+VC, only one port

  // The number of messages in the queues
  uint32_t currRecvCount;
  uint32_t currRecvMemBusCount;
  uint32_t currSendFrontBusCount;
  uint32_t currSendBackBusCount;

  uint32_t theInterleavingBlockShiftBits; // interleaving granularity
  uint32_t theNumL2Slices;                // # L2 Slices
  uint32_t theNumMemControllers;          // # memory controllers

  tPlacement theL2Design; // L2 design

  ////////////////////// Helper functions
  uint32_t log_base2( const uint32_t num ) const {
    uint32_t x = num;
    uint32_t ii = 0;
    while (x > 1) {
      ii++;
      x >>= 1;
    }
    return ii;
  }

  uint32_t getDstL2ID ( const uint64_t anAddr ) const {
    return ((anAddr >> theInterleavingBlockShiftBits ) % theNumL2Slices ); //fixme: respect floorplan
  }

  int32_t getDstMC( const uint32_t aDstL2ID ) const {
    return (aDstL2ID  / (theNumL2Slices / theNumMemControllers)); //fixme : respect floorplan
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CMPL2NetworkInterface)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
  }

  bool isQuiesced() const {
    return (!currRecvCount && !currRecvMemBusCount && !currSendFrontBusCount && !currSendBackBusCount);
  }

  // Initialization
  void initialize() {
    theInterleavingBlockShiftBits = log_base2( cfg.L2InterleavingGranularity ); // interleaving granularity
    theNumL2Slices = (cfg.NumL2Tiles ? cfg.NumL2Tiles : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
    theNumMemControllers = cfg.NumMemControllers;

    recvQueue.resize(cfg.VChannels);
    recvMemBusQueue.resize(cfg.VChannels);
    sendFrontBusQueue.resize(cfg.VChannels);
    sendBackBusQueue.resize(cfg.VChannels);
    currRecvCount = currRecvMemBusCount = currSendFrontBusCount = currSendBackBusCount = 0;

    // L2 design
    /* CMU-ONLY-BLOCK-BEGIN */
    if (cfg.Placement == "R-NUCA") {
      theL2Design = kRNUCACache;
    } else
      /* CMU-ONLY-BLOCK-END */
      if (cfg.Placement == "private") {
        theL2Design = kPrivateCache;
      } else if (cfg.Placement == "shared") {
        theL2Design = kSharedCache;
      } else {
        DBG_Assert(false, ( << "Unknown L2 design") );
      }

  }

  //////////// Ports
  // Reply
  bool available(interface::DReplyFromL2 const &, index_t coreNumber) {
    return sendFrontBusQueue[cfg.ReplyVc].size() < cfg.SendCapacity;
  }
  void push(interface::DReplyFromL2 const &, index_t coreNumber, MemoryTransport & transport) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[NetworkMessageTag]->vc = cfg.ReplyVc;

    transport[MemoryMessageTag]->coreNumber() = coreNumber;
    transport[MemoryMessageTag]->dstream() = true;

    // if this is a message from the private directory to another tile, set the MC field
    const uint64_t anAddr = transport[ MemoryMessageTag ]->address();
    const uint32_t aDstL2ID = getDstL2ID(anAddr);
    if (theL2Design == kPrivateCache && aDstL2ID == flexusIndex() && aDstL2ID != coreNumber) {
      const uint32_t aDstMC = getDstMC(aDstL2ID);
      transport[ MemoryMessageTag ]->dstMC() = aDstMC;
    }

    sendFrontBusQueue[cfg.ReplyVc].push_back( transport );
    ++currSendFrontBusCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted data reply #" << transport[MemoryMessageTag]->serial()
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " from L2[" << flexusIndex() << "]"
           << " and bound for cpu[" << coreNumber << "]"));
  }

  bool available(interface::IReplyFromL2 const &, index_t coreNumber) {
    return sendFrontBusQueue[cfg.ReplyVc].size() < cfg.SendCapacity;
  }
  void push(interface::IReplyFromL2 const &, index_t coreNumber, MemoryTransport & transport) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[NetworkMessageTag]->vc = cfg.ReplyVc;

    transport[MemoryMessageTag]->coreNumber() = coreNumber;
    transport[MemoryMessageTag]->dstream() = false;

    // if this is a message from the private directory to another tile, set the MC field
    const uint64_t anAddr = transport[ MemoryMessageTag ]->address();
    const uint32_t aDstL2ID = getDstL2ID(anAddr);
    if (theL2Design == kPrivateCache && aDstL2ID == flexusIndex() && aDstL2ID != coreNumber) {
      const uint32_t aDstMC = getDstMC(aDstL2ID);
      transport[ MemoryMessageTag ]->dstMC() = aDstMC;
    }

    sendFrontBusQueue[cfg.ReplyVc].push_back( transport );
    ++currSendFrontBusCount;

    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         ( << statName()
           << " accepted instruction reply #" << transport[MemoryMessageTag]->serial()
           << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
           << " from L2[" << flexusIndex() << "]"
           << " and bound for cpu[" << coreNumber << "]"));
  }

  // Network
  bool available(interface::FromNetwork const &, index_t aVC) {
    return (recvQueue[aVC].size() < cfg.RecvCapacity);
  }
  void push(interface::FromNetwork const &, index_t aVC, MemoryTransport & transport) {
    DBG_Assert(aVC == (index_t)transport[NetworkMessageTag]->vc, Comp(*this) ( << "Mismatched port/vc assignment"));

    recvQueue[aVC].push_back( transport );
    ++currRecvCount;
  }

  // Request
  bool available(interface::DRequestFromL2 const &
                 , index_t coreNumber
                ) {
    return sendFrontBusQueue[cfg.RequestVc].size() < cfg.SendCapacity;
  }
  void push(interface::DRequestFromL2 const &
            , index_t coreNumber
            , MemoryTransport & transport
           ) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[NetworkMessageTag]->vc = cfg.RequestVc;

    if (!transport[MemoryMessageTag]->isPurge()) { /* CMU-ONLY */
      transport[MemoryMessageTag]->coreNumber() = coreNumber;
      transport[MemoryMessageTag]->l2Number() = flexusIndex();
    }

    sendFrontBusQueue[cfg.RequestVc].push_back( transport );
    ++currSendFrontBusCount;

    DBG_(Iface,
         Addr(transport[ MemoryMessageTag ]->address())
         ( << statName()
           << " accepted data request " << *transport[MemoryMessageTag]
           << " from L2[" << flexusIndex() << "]"
           << " bound for cpu[" << coreNumber << "]"
         ));
  }

  bool available(interface::IRequestFromL2 const &
                 , index_t coreNumber
                ) {
    return sendFrontBusQueue[cfg.RequestVc].size() < cfg.SendCapacity;
  }
  void push(interface::IRequestFromL2 const &
            , index_t coreNumber
            , MemoryTransport & transport
           ) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[NetworkMessageTag]->vc = cfg.RequestVc;

    if (!transport[MemoryMessageTag]->isPurge()) { /* CMU-ONLY */
      transport[MemoryMessageTag]->coreNumber() = coreNumber;
      transport[MemoryMessageTag]->l2Number() = flexusIndex();
    }

    sendFrontBusQueue[cfg.RequestVc].push_back( transport );
    ++currSendFrontBusCount;

    DBG_(Iface,
         Addr(transport[ MemoryMessageTag ]->address())
         ( << statName()
           << " accepted instruction request " << *transport[MemoryMessageTag]
           << " from L2[" << flexusIndex() << "]"
           << " bound for cpu[" << coreNumber << "]"
         ));
  }

  // from backside
  bool available(interface::RequestFromL2BackSide const & ) {
    return sendBackBusQueue[cfg.RequestVc].size() < cfg.SendCapacity;
  }
  void push(interface::RequestFromL2BackSide const &
            , MemoryTransport & transport
           ) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[NetworkMessageTag]->vc = cfg.RequestVc;
    transport[MemoryMessageTag]->l2Number() = flexusIndex();

    sendBackBusQueue[cfg.RequestVc].push_back( transport );
    ++currSendBackBusCount;

    DBG_(Iface,
         Addr(transport[ MemoryMessageTag ]->address())
         ( << statName()
           << " accepted request from backside " << *transport[MemoryMessageTag]
           << " from L2[" << transport[MemoryMessageTag]->l2Number() << "]"
         ));
  }

  bool available(interface::SnoopFromL2BackSide const & ) {
    return sendBackBusQueue[cfg.SnoopVc].size() < cfg.SendCapacity;
  }
  void push(interface::SnoopFromL2BackSide const &
            , MemoryTransport & transport
           ) {
    boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
    transport.set ( NetworkMessageTag, netMsg );
    transport[NetworkMessageTag]->vc = cfg.SnoopVc;

    // remember which L2 sent the snoop
    transport[MemoryMessageTag]->l2Number() = flexusIndex();

    // If this is an evict, it originated from this L2. Thus, coreNumber() is garbage.
    // Fix it, as the next cache down the hierarchy may use the coreNumber() as an index in the input channel array.
    if (transport[MemoryMessageTag]->isEvict()) {
      transport[MemoryMessageTag]->coreNumber() = flexusIndex();
    }

    sendBackBusQueue[cfg.SnoopVc].push_back( transport );
    ++currSendBackBusCount;

    DBG_(Iface,
         Addr(transport[ MemoryMessageTag ]->address())
         ( << statName()
           << " accepted snoop from backside " << *transport[MemoryMessageTag]
           << " from L2[" << transport[MemoryMessageTag]->l2Number() << "]"
         ));
  }

  // to backside
  bool available(interface::ReplyFromMem const & ) {
    return recvMemBusQueue[cfg.ReplyVc].size() < cfg.RecvCapacity;
  }
  void push(interface::ReplyFromMem const &
            , MemoryTransport & transport
           ) {

    recvMemBusQueue[cfg.ReplyVc].push_back( transport );
    ++currRecvMemBusCount;

    DBG_(Iface,
         Addr(transport[ MemoryMessageTag ]->address())
         ( << statName()
           << " accepted mem reply for L2[" << transport[MemoryMessageTag]->l2Number() << "]"
           << " via iface[" << flexusIndex() << "]"
           << " " << *transport[MemoryMessageTag]
         ));
  }

  //Drive Interfaces
  void drive( interface::CMPL2NetworkInterfaceDrive const &) {
    // Handle any pending messages in the queues
    if (currSendFrontBusCount > 0) {
      msgFromNodeFrontBus();
    }
    if (currSendBackBusCount > 0) {
      msgFromNodeBackBus();
    }
    if (currRecvCount > 0) {
      msgToNode();
    }
    if (currRecvMemBusCount > 0) {
      msgFromMemBus();
    }
  }

private:
  void msgFromMemBus() {
    for (int32_t i = cfg.VChannels - 1; i >= 0; --i) {

      if (!recvMemBusQueue[i].empty()) {
        MemoryTransport transport = recvMemBusQueue[i].front();

        // try to give reply to L2
        if (transport[MemoryMessageTag]->l2Number() == static_cast<int>(flexusIndex())) {
          if (FLEXUS_CHANNEL(ReplyToL2).available()) {
            recvMemBusQueue[i].pop_front();

            DBG_(Iface,
                 Addr(transport[ MemoryMessageTag ]->address())
                 ( << statName()
                   << " Sending mem reply to L2[" << flexusIndex() << "]"
                   << " " << *transport[MemoryMessageTag]
                 ));

            --currRecvMemBusCount;
            FLEXUS_CHANNEL(ReplyToL2) << transport;
          }
        }

        // try to give a message to the network
        else if (FLEXUS_CHANNEL_ARRAY(ToNetwork, cfg.ReplyVc).available() ) {
          boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
          transport.set ( NetworkMessageTag, netMsg );
          transport[ NetworkMessageTag ]->vc = cfg.ReplyVc;

          recvMemBusQueue[i].pop_front();

          DBG_(Iface,
               Addr(transport[ MemoryMessageTag ]->address())
               ( << statName()
                 << " Sending mem reply to network from L2[" << flexusIndex() << "]"
                 << " to L2[" << transport[MemoryMessageTag]->l2Number() << "]"
                 << " " << *transport[MemoryMessageTag]
               ));

          --currRecvMemBusCount;
          FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
        }
      }
    }
  }

  void msgFromNodeFrontBus() {
    for (int32_t i = cfg.VChannels - 1; i >= 0; --i) {
      // try to give a message to the network
      if (!sendFrontBusQueue[i].empty()) {
        if (FLEXUS_CHANNEL_ARRAY(ToNetwork, sendFrontBusQueue[i].front()[NetworkMessageTag]->vc).available() ) {
          MemoryTransport transport = sendFrontBusQueue[i].front();
          sendFrontBusQueue[i].pop_front();

          DBG_(Iface,
               Addr(transport[MemoryMessageTag]->address())
               ( << statName()
                 << " Sending to network " << *transport[MemoryMessageTag]
                 << " from L2[" << transport[MemoryMessageTag]->l2Number() << "]"
                 << " bound for cpu[" << transport[MemoryMessageTag]->coreNumber() << "]"
               ));

          --currSendFrontBusCount;
          FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
        }
      }
    }
  }

  void msgFromNodeBackBus() {
    for (int32_t i = cfg.VChannels - 1; i >= 0; --i) {
      // try to give a message to the main memory
      // either through our own MC
      // or via the network to a remote MC
      if (!sendBackBusQueue[i].empty()) {

        MemoryTransport transport = sendBackBusQueue[i].front();
        const uint64_t anAddr(transport[ MemoryMessageTag ]->address());
        const uint32_t aDstL2ID = getDstL2ID(anAddr);

        // send message to own MC
        if ( aDstL2ID == flexusIndex() ) {
          if (FLEXUS_CHANNEL(RequestToMem).available()) {
            sendBackBusQueue[i].pop_front();

            transport[ MemoryMessageTag ]->l2Number() = flexusIndex();

            DBG_(VVerb,
                 Addr(transport[ MemoryMessageTag ]->address())
                 ( << statName()
                   << " sending mem request from L2[" << flexusIndex() << "]"
                   << " " << *transport[MemoryMessageTag]
                 ));

            --currSendBackBusCount;
            FLEXUS_CHANNEL(RequestToMem) << transport;
          }
        }

        // send message to a remote MC via another L2
        else if (FLEXUS_CHANNEL_ARRAY(ToNetwork, sendBackBusQueue[i].front()[NetworkMessageTag]->vc).available() ) {
          sendBackBusQueue[i].pop_front();

          DBG_(VVerb,
               Addr(transport[ MemoryMessageTag ]->address())
               ( << statName()
                 << " Sending mem req to network from L2[" << flexusIndex() << "]"
                 << " to L2[" << aDstL2ID << "]"
                 << " " << *transport[MemoryMessageTag]
               ));

          --currSendBackBusCount;
          FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
        }

      }
    }
  }

  void msgToNode() {
    int32_t i;
    int32_t coreNumber;

    // try to give a message to the node, beginning with the highest priority virtual channel
    for (i = cfg.VChannels - 1; i >= 0; --i) {
      if (!recvQueue[i].empty()) {
        MemoryTransport transport = recvQueue[i].front();
        coreNumber = transport[MemoryMessageTag]->coreNumber();

        DBG_Assert(!transport[MemoryMessageTag]->isPrefetchType(), ( << "CMP interconnect does not support prefetch messages!"));

        // send message to own MC
        if ( transport[MemoryMessageTag]->dstMC() != -1
             && theL2Design != kPrivateCache
           ) {
          const uint64_t anAddr(transport[ MemoryMessageTag ]->address());
          const uint32_t aDstL2ID = getDstL2ID(anAddr);
          DBG_Assert( flexusIndex() == aDstL2ID,
                      ( << statName()
                        << " idx=" << flexusIndex()
                        << " aDstL2ID=" << aDstL2ID
                        << " " << *transport[MemoryMessageTag]
                      ));
          const int32_t aDstMC = getDstMC(aDstL2ID);
          DBG_Assert ( transport[MemoryMessageTag]->dstMC() == aDstMC,
                       ( << statName()
                         << " msg->dstMC()=" << transport[MemoryMessageTag]->dstMC()
                         << " aDstMC=" << aDstMC
                         << " aDstL2ID=" << aDstL2ID
                         << " theNumL2Slices=" << theNumL2Slices
                         << " theNumMemControllers=" << theNumMemControllers
                       ));

          if (FLEXUS_CHANNEL(RequestToMem).available()) {
            recvQueue[i].pop_front();

            // the message already has the L2 number of the true requestor L2
            // transport[ MemoryMessageTag ]->l2Number() = flexusIndex();

            DBG_(VVerb,
                 Addr(transport[ MemoryMessageTag ]->address())
                 ( << statName()
                   << " sending mem request from L2[" << flexusIndex() << "]"
                   << " " << *transport[MemoryMessageTag]
                 ));

            --currRecvCount;
            FLEXUS_CHANNEL(RequestToMem) << transport;
          }
        }

        // Snoop
        else if (transport[MemoryMessageTag]->usesSnoopChannel()) {

          // data snoop
          if (transport[MemoryMessageTag]->isDstream()) {
            //fixme
            DBG_(VVerb,
                 Addr(transport[MemoryMessageTag]->address())
                 ( << statName()
                   << " msg:" << *transport[MemoryMessageTag]
                 ));
            if (FLEXUS_CHANNEL_ARRAY(DSnoopToL2, coreNumber).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending data snoop #" << transport[MemoryMessageTag]->serial()
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " from cpu[" << coreNumber << "]"
                     << " into L2[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL_ARRAY(DSnoopToL2, coreNumber) << transport;
            }
          }
          // instruction snoop
          else {
            if (FLEXUS_CHANNEL_ARRAY(ISnoopToL2, coreNumber).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending instruction snoop #" << transport[MemoryMessageTag]->serial()
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " from cpu[" << coreNumber << "]"
                     << " into L2[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL_ARRAY(ISnoopToL2, coreNumber) << transport;
            }
          }
        }

        // L2-to-L2 request
        else if (transport[MemoryMessageTag]->isExtReqType()) {
          DBG_Assert (transport[MemoryMessageTag]->isRequest());

          // L2-to-L2 snoop reply
          if (!transport[MemoryMessageTag]->isRequest()) {
            if (FLEXUS_CHANNEL(ReplyToL2).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[ MemoryMessageTag ]->address())
                   ( << statName()
                     << " sending L2-to-L2 snoop reply " << *transport[MemoryMessageTag]
                     << " from L2[" << flexusIndex() << "]"
                   ));
              FLEXUS_CHANNEL(ReplyToL2) << transport;
            }
          } else {
            // L2-to-L2 snoop request
            if (FLEXUS_CHANNEL(SnoopToL2).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending request to backside #" << transport[MemoryMessageTag]->serial()
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " from cpu[" << coreNumber << "]"
                     << " into L2[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL(SnoopToL2) << transport;
            }
          }
        }

        // Request
        else if (transport[MemoryMessageTag]->isRequest()) {
          // data request
          if (transport[MemoryMessageTag]->isDstream()) {
            if (FLEXUS_CHANNEL_ARRAY(DRequestToL2, coreNumber).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending data request #" << transport[MemoryMessageTag]->serial()
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " from cpu[" << coreNumber << "]"
                     << " into L2[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL_ARRAY(DRequestToL2, coreNumber) << transport;
            }
          }
          // instruction request
          else {
            if (FLEXUS_CHANNEL_ARRAY(IRequestToL2, coreNumber).available()) {
              recvQueue[i].pop_front();

              --currRecvCount;
              DBG_(Iface,
                   Addr(transport[MemoryMessageTag]->address())
                   ( << statName()
                     << " sending instruction request #" << transport[MemoryMessageTag]->serial()
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
                     << " from cpu[" << coreNumber << "]"
                     << " into L2[" << flexusIndex() << "]"));
              FLEXUS_CHANNEL_ARRAY(IRequestToL2, coreNumber) << transport;
            }
          }
        }

        // Reply from a remote MC
        else {
          if (FLEXUS_CHANNEL(ReplyToL2).available()) {
            recvQueue[i].pop_front();

            DBG_(VVerb,
                 Addr(transport[ MemoryMessageTag ]->address())
                 ( << statName()
                   << " Sending mem reply to L2[" << flexusIndex() << "]"
                   << " " << *transport[MemoryMessageTag]
                 ));

            --currRecvCount;
            FLEXUS_CHANNEL(ReplyToL2) << transport;
          }
        }
      }
    }
  }
};

} //End Namespace nCMPL2NetworkInterface

FLEXUS_COMPONENT_INSTANTIATOR( CMPL2NetworkInterface, nCMPL2NetworkInterface );

FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, ToNetwork )      {
  return cfg.VChannels;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, FromNetwork )    {
  return cfg.VChannels;
}

FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, DReplyFromL2 )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, DRequestFromL2 ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, DSnoopToL2 )     {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, DRequestToL2 )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, IReplyFromL2 )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, IRequestFromL2 ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, ISnoopToL2 )     {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPL2NetworkInterface, IRequestToL2 )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPL2NetworkInterface

#define DBG_Reset
#include DBG_Control()
