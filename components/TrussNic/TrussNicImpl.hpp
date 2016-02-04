#define FLEXUS_BEGIN_COMPONENT TrussNic
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories TrussNic
#define DBG_SetDefaultOps AddCat(TrussNic)
#include DBG_Control()

namespace nTrussNic {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

static const int32_t VC4 = 3;

template <class Configuration>
class TrussNicComponent : public FlexusComponentBase< TrussNicComponent, Configuration> {
  FLEXUS_COMPONENT_IMPL(nTrussNic::TrussNicComponent, Configuration);

public:
  TrussNicComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theRecvCount("MsgsReceived", this)
    , theSendCount("MsgsSent", this)
    , theDirtyForwardedReads("DirtyForwardedReads", this)
    , theDirtyForwardedWrites("DirtyForwardedWrites", this)
    , theDirtyRecallReads("DirtyRecallReads", this)
    , theDirtyRecallWrites("DirtyRecallWrites", this)
    , theWritebackReqs("WritebackReqs", this)

  {}

  // Initialization
  void initialize() {
    theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(0);
    recvQueue = new std::vector<NetworkTransport> [cfg.VChannels.value];
    currRecvCount = 0;

    for (int32_t i = 0; i < cfg.VChannels.value; i++) {
      theRQueueSizes.push_back(new Stat::StatMax("max rcv queue [vc:" + std::to_string(i) + "]", this));
    }
  }

  // Ports
  struct ToNetwork : public PushOutputPort<NetworkTransport> { };

  struct FromNode : public PushInputPort<NetworkTransport>, AvailabilityComputedOnRequest {
    typedef FLEXUS_IO_LIST(1, Availability<ToNetwork>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNetwork>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aTrussNic, NetworkTransport transport) {

      DBG_(Iface, ( << "Send to network: src=" << transport[NetworkMessageTag]->src
                    << " dest=" << transport[NetworkMessageTag]->dest
                    << " vc=" << transport[NetworkMessageTag]->vc
                  ) );
      //                   Comp(aTrussNic) );
      DBG_(Iface, Condition((transport[ProtocolMessageTag]))
           ( << "  Packet contains: " << *transport[ProtocolMessageTag])
           Comp(aTrussNic)
          );

      bool bDirtyDataFlag = transport[ProtocolMessageTag] &&
                            (transport[ProtocolMessageTag]->type() == nProtocolEngine::ReadFwd ||
                             transport[ProtocolMessageTag]->type() == nProtocolEngine::WriteFwd ||
                             transport[ProtocolMessageTag]->type() == nProtocolEngine::ForwardedReadAck ||
                             transport[ProtocolMessageTag]->type() == nProtocolEngine::ForwardedWriteAck ||
                             transport[ProtocolMessageTag]->type() == nProtocolEngine::RecallReadAck ||
                             transport[ProtocolMessageTag]->type() == nProtocolEngine::RecallWriteAck ||
                             transport[ProtocolMessageTag]->type() == nProtocolEngine::WritebackReq);

      DBG_(Iface, Condition(transport[ProtocolMessageTag] &&
                            transport[ProtocolMessageTag]->type() == nProtocolEngine::WritebackReq)
           ( << "Writeback Request!" ) );

      // update statistics
      if (transport[ProtocolMessageTag]) {
        switch (transport[ProtocolMessageTag]->type()) {
          case nProtocolEngine::ForwardedReadAck:
            ++aTrussNic.theDirtyForwardedReads;
            break;
          case nProtocolEngine::ForwardedWriteAck:
            ++aTrussNic.theDirtyForwardedWrites;
            break;
          case nProtocolEngine::RecallReadAck:
            ++aTrussNic.theDirtyRecallReads;
            break;
          case nProtocolEngine::RecallWriteAck:
            ++aTrussNic.theDirtyRecallWrites;
            break;
          case nProtocolEngine::WritebackReq:
            ++aTrussNic.theWritebackReqs;
            break;
          default:
            break;
        }
      }

      if ((aTrussNic.theTrussManager->isSlaveNode(aTrussNic.flexusIndex()) && bDirtyDataFlag) ||
          (aTrussNic.theTrussManager->isMasterNode(aTrussNic.flexusIndex()) && !bDirtyDataFlag)) {

        // super hack-a-riffic.  If this is a WritebackReq, we have to fake the original sender/requester as
        // the master, not the slave actually doing the sending.  We could fix this by sending the original WritebackReq
        // to the slave and doing the 'verification', then forwarding the MASTER's request on.  TODO.
        if (aTrussNic.theTrussManager->isSlaveNode(aTrussNic.flexusIndex()) &&
            transport[ProtocolMessageTag] && transport[ProtocolMessageTag]->type() == nProtocolEngine::WritebackReq) {
          transport[ProtocolMessageTag]->setRequester(aTrussNic.theTrussManager->getMasterIndex(aTrussNic.flexusIndex()));
          transport[ProtocolMessageTag]->setSource(aTrussNic.theTrussManager->getMasterIndex(aTrussNic.flexusIndex()));
        }

        FLEXUS_CHANNEL(aTrussNic, ToNetwork) << transport;
        ++aTrussNic.theSendCount;
      } else {
        DBG_(Iface, ( << "Silencing nic"));
      }
    }

    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aTrussNic) {
      return FLEXUS_CHANNEL( aTrussNic, ToNetwork).available();
    }
  };

  struct ToMemory : public PushOutputPort<MemoryTransport> { };

  struct FromMemory : public PushInputPort<MemoryTransport>, AvailabilityComputedOnRequest {
    typedef FLEXUS_IO_LIST(1, Availability<ToNetwork>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNetwork>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aTrussNic, MemoryTransport transport) {
      // TRUSS: build a network transport from the memory transport
      DBG_Assert(transport[MemoryMessageTag], ( << "Expected transport to contain a MemoryMessageTag"));

      intrusive_ptr<NetworkMessage> repl_msg(new NetworkMessage());

      repl_msg->src  = aTrussNic.flexusIndex();
      repl_msg->dest = aTrussNic.theTrussManager->getSlaveNode(aTrussNic.flexusIndex(), 0);
      repl_msg->vc   = VC4;

      // create truss message -- contains timestamp and memory message
      intrusive_ptr<TrussMessage> tmsg = new TrussMessage(Flexus::Core::theFlexus->cycleCount(),
          transport[MemoryMessageTag]);

      // create a new transport
      NetworkTransport repl_transport;
      repl_transport.set(NetworkMessageTag, repl_msg);
      repl_transport.set(TrussMessageTag, tmsg);

      DBG_(Iface, ( << "Send (memory) to slave: " << repl_msg->dest << " VC4, time: " << tmsg->theTimestamp));

      // send it
      FLEXUS_CHANNEL(aTrussNic, ToNetwork) << repl_transport;
      ++aTrussNic.theSendCount;
    }
  };

  struct FromNetwork : public PushInputPort<NetworkTransport>, AvailabilityComputedOnRequest {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aTrussNic, NetworkTransport transport) {
      DBG_(Iface, ( << "Recv from network: src=" << transport[NetworkMessageTag]->src
                    << " dest=" << transport[NetworkMessageTag]->dest
                    << " vc=" << transport[NetworkMessageTag]->vc
                  )
           Comp(aTrussNic)
          );
      DBG_(Iface, Condition((transport[ProtocolMessageTag]))
           ( << "  Packet contains: " << *transport[ProtocolMessageTag])
           Comp(aTrussNic)
          );
      DBG_(Iface, Condition((transport[TrussMessageTag] && transport[TrussMessageTag]->theProtocolMessage))
           ( << "  Packet contains TRUSS message, timestamp: " << transport[TrussMessageTag]->theTimestamp
             << " and protocol message: " << *transport[TrussMessageTag]->theProtocolMessage) Comp(aTrussNic));
      DBG_(Iface, Condition((transport[TrussMessageTag] && transport[TrussMessageTag]->theMemoryMessage))
           ( << "  Packet contains TRUSS message, timestamp: " << transport[TrussMessageTag]->theTimestamp
             << " and memory message: " << *transport[TrussMessageTag]->theMemoryMessage) Comp(aTrussNic));

      aTrussNic.recv(transport);
      ++aTrussNic.theRecvCount;
    }

    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aTrussNic) {
      return aTrussNic.recvAvailable();
    }
  };

  struct ToNode : public PushOutputPort<NetworkTransport> { };

  //Drive Interfaces
  struct TrussNicDrive {
    FLEXUS_DRIVE( TrussNicDrive ) ;
    typedef FLEXUS_IO_LIST(1, Availability<ToNode>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNode>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aTrussNic) {
      // if there are received messages pending and the node can
      // accept a message, give it one
      if (aTrussNic.currRecvCount > 0) {
        if (FLEXUS_CHANNEL(aTrussNic, ToNode).available()) {
          aTrussNic.msgToNode<FLEXUS_PASS_WIRING>();
        }
      }
    }
  };

  //Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, TrussNicDrive) DriveInterfaces;

private:

  // statistics
  std::vector<boost::intrusive_ptr<Stat::StatMax> > theRQueueSizes;
  Stat::StatCounter theRecvCount;
  Stat::StatCounter theSendCount;
  Stat::StatCounter theDirtyForwardedReads;
  Stat::StatCounter theDirtyForwardedWrites;
  Stat::StatCounter theDirtyRecallReads;
  Stat::StatCounter theDirtyRecallWrites;
  Stat::StatCounter theWritebackReqs;

  boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager;

  bool recvAvailable() {
    // check if any of the receive queues are at capacity - if so,
    // return false; otherwise true.
    return true;  // infinite buffers
  }

  void recv(NetworkTransport & transport) {
    intrusive_ptr<NetworkMessage> msg = transport[NetworkMessageTag];
    DBG_Assert( (msg->vc >= 0) && (msg->vc < cfg.VChannels.value), Comp(*this) );
    recvQueue[msg->vc].push_back(transport);
    currRecvCount++;
    *(theRQueueSizes[msg->vc]) << currRecvCount;
  }

  FLEXUS_WIRING_TEMPLATE
  void msgToNode() {
    // try to give a message to the node, beginning with the
    // highest priority virtual channel
    bool bSentFlag = false;
    std::vector<NetworkTransport>::iterator iter;
    for (iter = recvQueue[VC4].begin(); iter != recvQueue[VC4].end(); ) {
      DBG_(Iface, ( << "iter_start for node: " << this->flexusIndex()));
      DBG_Assert( (*iter)[TrussMessageTag], ( << "Message on VC4 must contain TrussMessageTag" ));
      if ((*iter)[TrussMessageTag]->theProtocolMessage) {
        DBG_Assert( (*iter)[TrussMessageTag]->theTimestamp + theTrussManager->getFixedDelay() >=
                    Flexus::Core::theFlexus->cycleCount(), ( << "Message not delivered in time.  [" << this->flexusIndex()
                        << "] -- timestamp: " << (*iter)[TrussMessageTag]->theTimestamp
                        << "carrier message: " <<
                        *(*iter)[TrussMessageTag]->theProtocolMessage));
      } else {
        DBG_Assert( (*iter)[TrussMessageTag]->theTimestamp + theTrussManager->getFixedDelay() >=
                    Flexus::Core::theFlexus->cycleCount(), ( << "Message not delivered in time.  [" << this->flexusIndex()
                        << "] -- timestamp: " << (*iter)[TrussMessageTag]->theTimestamp
                        << "carrier message: " <<
                        *(*iter)[TrussMessageTag]->theMemoryMessage));
      }

      if ((*iter)[TrussMessageTag]->theTimestamp + theTrussManager->getFixedDelay() ==
          Flexus::Core::theFlexus->cycleCount()) {

        // two types of messages here: protocol messages and memory messages
        if ((*iter)[TrussMessageTag]->theProtocolMessage) {
          // Make a new transport for the ORIGINAL message
          NetworkTransport transport;
          transport.set(ProtocolMessageTag, (*iter)[TrussMessageTag]->theProtocolMessage);
          DBG_(Iface, Condition((transport[ProtocolMessageTag]))
               ( << "Received from Master -- packet contains: " << *transport[ProtocolMessageTag])
               Comp(*this)
              );
          FLEXUS_CHANNEL(*this, ToNode) << transport;
        } else {
          // a memory message
          DBG_Assert((*iter)[TrussMessageTag]->theMemoryMessage, ( << "Expected a memory message"));
          DBG_(Iface, ( << "Received from Master -- packet contains: " << *(*iter)[TrussMessageTag]->theMemoryMessage));
          MemoryTransport transport;
          transport.set(MemoryMessageTag, (*iter)[TrussMessageTag]->theMemoryMessage);
          FLEXUS_CHANNEL(*this, ToMemory) << transport;
        }
        DBG_(Iface, ( << "iter_erase for node: " << this->flexusIndex()));
        recvQueue[VC4].erase(iter);
        currRecvCount--;
        bSentFlag = true;
      } else {
        iter++;
      }
      DBG_(Iface, ( << "iter_done for node: " << this->flexusIndex()));
    } // end for each entry in queue

    if (bSentFlag) return;

    int32_t ii;
    for (ii = cfg.VChannels.value - 2; ii >= 0; ii--) {
      if (!recvQueue[ii].empty()) {
        // For slave nodes, we're not expecting a message in a 'normal' VC
        DBG_Assert(theTrussManager->isMasterNode(this->flexusIndex()), ( << "Not expecting a slave node") );

        // TRUSS: Replicate message at slave
        // new message carrier
        intrusive_ptr<NetworkMessage> repl_msg(new NetworkMessage());

        // change src, destination and VC
        repl_msg->src  = this->flexusIndex();
        repl_msg->dest = theTrussManager->getSlaveNode(this->flexusIndex(), 0);
        repl_msg->vc   = VC4;

        // create truss message -- contains timestamp and original message
        intrusive_ptr<TrussMessage> tmsg = new TrussMessage(Flexus::Core::theFlexus->cycleCount(),
            recvQueue[ii].front()[ProtocolMessageTag]);

        // create a new transport
        NetworkTransport repl_transport;
        repl_transport.set(NetworkMessageTag, repl_msg);
        repl_transport.set(TrussMessageTag, tmsg);

        DBG_(Iface, ( << "Send to slave: " << repl_msg->dest << " VC4, time: " << tmsg->theTimestamp));

        // send it
        FLEXUS_CHANNEL(*this, ToNetwork) << repl_transport;
        ++theSendCount;

        // Now do normal receive stuff
        FLEXUS_CHANNEL(*this, ToNode) << recvQueue[ii].front();
        recvQueue[ii].erase(recvQueue[ii].begin());
        currRecvCount--;

        break;
      } // end if recvqueue not empty
    } // end for each VC
  }

  // The received message queues (one for each virtual channel)
  std::vector<NetworkTransport> *recvQueue;

  // The number of messages in the receive queues
  int32_t currRecvCount;
};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(TrussNicConfiguration,
                                        FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 4 )
                                       );

} //End Namespace nTrussNic

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TrussNic

#define DBG_Reset
#include DBG_Control()
