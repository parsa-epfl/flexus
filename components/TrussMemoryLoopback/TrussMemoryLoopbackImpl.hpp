#define FLEXUS_BEGIN_COMPONENT TrussMemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <vector>

#define DBG_DefineCategories TrussMemoryLoopback
#define DBG_SetDefaultOps AddCat(TrussMemoryLoopback)
#include DBG_Control()

namespace nTrussMemoryLoopback {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

#ifndef FLEXUS_MemoryMap_TYPE_PROVIDED
//Dummy MemoryMap that does nothing
struct MemoryMap : public boost::counted_base {
  enum AccessType { Read, Write };
  static MemoryMap * getMemoryMap(index_t) {
    return new MemoryMap();
  }
  template <class T> void recordAccess(T const &, AccessType) {}
};

#else
//Use the MemoryMap provided by the MemoryMap component
using Flexus::SharedTypes::MemoryMap;

#endif

using boost::intrusive_ptr;

template <class Cfg>
class TrussMemoryLoopbackComponent : public FlexusComponentBase<TrussMemoryLoopbackComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(nTrussMemoryLoopback::TrussMemoryLoopbackComponent, Cfg);

  boost::intrusive_ptr<MemoryMap> theMemoryMap;
  boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager;

public:
  TrussMemoryLoopbackComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  // Initialization
  void initialize() {
    if (cfg.Delay.value < 1) {
      std::cout << "Error: memory access time must be greater than 0." << std::endl;
      throw FlexusException();
    }
    theMemoryMap = MemoryMap::getMemoryMap(flexusIndex());
    theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(0);
  }

  // Ports
  struct LoopbackOut : public PushOutputPort< MemoryTransport > { };

  struct LoopbackIn : public PushInputPort<MemoryTransport>, AvailabilityDeterminedByInputs {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aLoopback, MemoryTransport aMessageTransport) {

      // TRUSS: send the transport to the nic as well, if this is a master node
      if (aLoopback.theTrussManager->isMasterNode(aLoopback.flexusIndex())) {

        DBG_(Iface, Comp(aLoopback) ( << "request received: " << *aMessageTransport[MemoryMessageTag]) Addr(aMessageTransport[MemoryMessageTag]->address()) );
        intrusive_ptr<MemoryMessage> reply;
        switch (aMessageTransport[MemoryMessageTag]->type()) {
          case MemoryMessage::LoadReq:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::LoadReply, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;
          case MemoryMessage::StoreReq:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::StoreReply, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;
          case MemoryMessage::CmpxReq:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::CmpxReply, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;

          case MemoryMessage::ReadReq:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::MissReplyWritable, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::MissReplyWritable, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;
          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::UpgradeReply, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;
          case MemoryMessage::FlushReq:
          case MemoryMessage::Flush:
          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            // no reply required
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->complete();
            }
            return;
          case MemoryMessage::PrefetchReadNoAllocReq:
            aLoopback.theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::PrefetchWritableReply, aMessageTransport[MemoryMessageTag]->address());
            if (aMessageTransport[TransactionTrackerTag]) {
              aMessageTransport[TransactionTrackerTag]->setFillLevel(nTransactionTracker::eLocalMem);
              aMessageTransport[TransactionTrackerTag]->setResponder(aLoopback.flexusIndex());
              aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
            }
            break;
          case MemoryMessage::PrefetchInsert:
            // should never happen
            DBG_Assert(false, Component(aLoopback) ( << "TrussMemoryLoopback received PrefetchInsert request") );
            break;
          case MemoryMessage::PrefetchInsertWritable:
            // should never happen
            DBG_Assert(false, Component(aLoopback) ( << "TrussMemoryLoopback received PrefetchInsertWritable request") );
            break;
          default:
            DBG_Assert(false, Component(aLoopback) ( << "Don't know how to handle message.  No reply sent.") );
            return;
        }
        DBG_(Iface, Comp(aLoopback) ( << "Queing reply: " << *reply) Addr(aMessageTransport[MemoryMessageTag]->address()) );
        aMessageTransport.set(MemoryMessageTag, reply);

        // account for the one cycle delay inherent from Flexus when sending a
        // response back up the hierarchy: actually stall for one less cycle
        // than the configuration calls for
        aLoopback.outQueue.push_back(make_pair(aMessageTransport, aLoopback.cfg.Delay.value - 1));

        DBG_(Iface, Comp(aLoopback) ( << "Forwarding memory access to slave"));
        FLEXUS_CHANNEL( aLoopback, ToNic ) << aMessageTransport;
      }

    }

    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aLoopback) {
      return FLEXUS_CHANNEL( aLoopback, LoopbackOut).available();
    }
  };

  struct ToNic : public PushOutputPort<MemoryTransport> { };

  struct FromNic : public PushInputPort<MemoryTransport>, AvailabilityComputedOnRequest {
    typedef FLEXUS_IO_LIST(1, Availability<LoopbackOut>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<LoopbackOut>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aLoopback, MemoryTransport transport) {
      // This function is called at the slave.  When the master accesses local data from memory, we forward
      // the memory/local engine interactions to the slave, and they get received here.  The MemoryLoopback
      // at the slave feeds them to the slave local engine, and the slave acts just like the master
      // The master sends the forwarding message at the time the memory message is enqueued, so we enqueue it here
      // right away.  The TrussNic will have sent us this transport AFTER the Master/Slave delay is imposed.

      DBG_Assert(aLoopback.theTrussManager->isSlaveNode(aLoopback.flexusIndex()), ( << "Receiving something from Nic but not a slave!"));
      DBG_Assert(transport[MemoryMessageTag], ( << "transport must contain MemoryMessageTag"));
      DBG_(Iface, Comp(aLoopback) ( << "Queing (forwarded) reply: " << *transport[MemoryMessageTag]) Addr(transport[MemoryMessageTag]->address()) );
      aLoopback.outQueue.push_back(make_pair(transport, aLoopback.cfg.Delay.value - 1));
    }
  };

  //Drive Interfaces
  struct LoopbackDrive {
    FLEXUS_DRIVE( LoopbackDrive ) ;
    typedef FLEXUS_IO_LIST( 3, Availability<LoopbackOut>
                            , Value<LoopbackIn>
                            , Value<FromNic> ) Inputs;
    typedef FLEXUS_IO_LIST( 1, Value<LoopbackOut> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aLoopback) {
      //FLEXUS_MBR_DEBUG_I(aLoopback) << "LoopbackDrive" << end;
      DelayIter iter = aLoopback.outQueue.begin();
      while (iter != aLoopback.outQueue.end()) {
        if (iter->second == 0) {
          // the delay has expired - send the message
          if ( FLEXUS_CHANNEL(aLoopback, LoopbackOut).available() ) {
            DBG_(Iface, Comp(aLoopback) ( << "Sending reply: " << *(iter->first[MemoryMessageTag]) ) Addr(iter->first[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL( aLoopback, LoopbackOut) << iter->first;
            aLoopback.outQueue.erase(iter);
          } else {
            DBG_(VVerb, Comp(aLoopback) ( << "Output channel not available" ) );
          }
        } else {
          // decrement the delay counter
          (iter->second)--;
          iter++;
        }
      }
    }
  };

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, LoopbackDrive) DriveInterfaces;

private:
  typedef std::pair<MemoryTransport, int> DelayTrans;
  typedef std::vector<DelayTrans>::iterator DelayIter;
  std::vector<DelayTrans> outQueue;

};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(TrussMemoryLoopbackConfiguration,
                                        FLEXUS_PARAMETER( Delay, int, "Access time", "time", 1 )
                                       );

} //End Namespace nTrussMemoryLoopback

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TrussMemoryLoopback

#define DBG_Reset
#include DBG_Control()

