#ifndef _SERVICE_FLEXUS_H_
#define _SERVICE_FLEXUS_H_

#include <core/debug/debug.hpp>
#include <core/flexus.hpp>

#include <components/Common/MemoryMap.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/Common/Transports/DirectoryTransport.hpp>
#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/DirectoryMessage.hpp>

#include "ProtSharedTypes.hpp"
#include "tSrvcProvider.hpp"
#include "util.hpp"
#include "protocol_engine.hpp"

namespace nProtocolEngine {

using namespace Flexus::SharedTypes;
using namespace Protocol;
using boost::intrusive_ptr;

class ServiceFlexus : public tSrvcProvider {

private:
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  const node_id_t theNodeId;
  const node_id_t theMaxNodes;
  const boost::intrusive_ptr<Flexus::SharedTypes::MemoryMap> theMemoryMap;

  int32_t theCPI;

  boost::shared_ptr<tProtocolEngineBase> theProtocolEngine;

public:

  ServiceFlexus(node_id_t                                              aNodeId,
                node_id_t                                              aMaxNodes,
                boost::intrusive_ptr<Flexus::SharedTypes::MemoryMap> aMemoryMap,
                int32_t                                                    aCPI)

    : theNodeId(aNodeId)
    , theMaxNodes(aMaxNodes)
    , theMemoryMap(aMemoryMap)
    , theCPI(aCPI)
  { }

  virtual ~ServiceFlexus() { }

  void setProtocolEngine(boost::shared_ptr<tProtocolEngineBase> aProtocolEngine) {
    DBG_Assert(aProtocolEngine);
    theProtocolEngine = aProtocolEngine;
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  ////////////////////////////////////////////////////////////////////////
  //
  // talk to the SORD manager (MRP)
  //
  void predNotifyFlush(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) {
    //Notify the predictor that a flush has been received
    intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eFlush, aNode, MemoryAddress(addr)));
    PredictorTransport transport;
    transport.set(PredictorMessageTag, msg);
    transport.set(TransactionTrackerTag, aTracker);
    toPredictor.push_back(transport);
  }

  void predNotifyWrite(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) {
    //Notify the predictor that a flush has been received
    intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eWrite, aNode, MemoryAddress(addr)));
    PredictorTransport transport;
    transport.set(PredictorMessageTag, msg);
    transport.set(TransactionTrackerTag, aTracker);
    toPredictor.push_back(transport);
  }

  void predNotifyReadPredicted(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) {
    //Notify the predictor that a flush has been received
    intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eReadPredicted, aNode, MemoryAddress(addr)));
    PredictorTransport transport;
    transport.set(PredictorMessageTag, msg);
    transport.set(TransactionTrackerTag, aTracker);
    toPredictor.push_back(transport);
  }

  void predNotifyReadNonPredicted(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) {
    //Notify the predictor that a flush has been received
    intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eReadNonPredicted, aNode, MemoryAddress(addr)));
    PredictorTransport transport;
    transport.set(PredictorMessageTag, msg);
    transport.set(TransactionTrackerTag, aTracker);
    toPredictor.push_back(transport);
  }
  /* CMU-ONLY-BLOCK-END */

  ////////////////////////////////////////////////////////////////////////
  //
  // node and address info
  //

  // number of nodes in the system
  node_id_t numNodes(void) {
    return theMaxNodes;
  }

  // This is the id of this particular node
  node_id_t myNodeId(void) {
    return theNodeId;
  }

  // the node id with the directory for that address
  node_id_t nodeForAddress(const tAddress address) {
    return theMemoryMap->node(MemoryAddress(address));
  }

  // return true if node has directory for that address
  // return false otherwise
  bool isAddressLocal(const tAddress address) {
    return (myNodeId() == nodeForAddress(address));
  }

  int32_t getCPI() {
    if (theFlexus->isFastMode()) {
      return 1;
    }
    return theCPI;
  }

  ////////////////////////////////////////////////////////////////////////
  //
  // directory and message info
  //

  // Translates a service provider memory operation to a directory command
  Flexus::SharedTypes::DirectoryCommand::DirectoryCommand translateSrvcToDir(tMemOpType op) {
    switch (op) {
      case READ:
        return DirectoryCommand::Get;
      case WRITE:
        return DirectoryCommand::Set;
      case MEM_LOCK:
        return DirectoryCommand::Lock;
      case MEM_UNLOCK:
        return DirectoryCommand::Unlock;
      case MEM_LOCK_SQUASH:
        return DirectoryCommand::Squash;
      default:
        DBG_Assert(false);
        return DirectoryCommand::Unlock; //suppress warning
    }
  }

  // translate a MemoryTransport into a LocalReq
  boost::intrusive_ptr<tPacket> translateMemoryTransport(MemoryTransport trans) {
    tVC message_vc = ( trans[MemoryMessageTag]->isRequest() ? nProtocolEngine::LocalVC0 : nProtocolEngine::LocalVC1) ;

    tMessageType message_type = translateMemoryMessage(trans[MemoryMessageTag]->type());
    bool prefetch = (message_type == LocalPrefetchRead);
    DBG_Assert(message_type != nProtocolEngine::ProtocolError, ( << theProtocolEngine->engineName() << " received message : " << *(trans[MemoryMessageTag]) << " on VC[" << message_vc << "] which does not map to any internal message type") );

    boost::intrusive_ptr<tPacket> req( new tPacket(trans[MemoryMessageTag]->address(), message_vc, message_type, trans[DirectoryEntryTag], prefetch, trans[TransactionTrackerTag] ));

    return req;
  }

  // outgoing queues for each destination
  // - these need to be public so the main Flexus component can inspect them
  std::vector<DirectoryTransport> toDir;
  std::vector<NetworkTransport> toNetwork;
  std::vector<MemoryTransport> toCpu;
  std::vector<PredictorTransport> toPredictor; /* CMU-ONLY */

  // incoming messages - these are queues for now to simplify matters
  std::list<DirectoryTransport> fromDir;

private:
  tMessageType translateMemoryMessage(MemoryMessage::MemoryMessageType aMessageType) {
    switch (aMessageType) {

      case MemoryMessage::ReadReq:
        return Protocol::LocalRead;

      case MemoryMessage::WriteReq:
        return Protocol::LocalWriteAccess;

      case MemoryMessage::Flush:
        return Protocol::LocalFlush;

      case MemoryMessage::UpgradeReq:
        return Protocol::LocalUpgradeAccess;

      case MemoryMessage::EvictDirty:
        return Protocol::LocalEvict;

      case MemoryMessage::PrefetchReadNoAllocReq:
        return Protocol::LocalPrefetchRead;

      case MemoryMessage::InvalidateAck:
        return Protocol::InvAck;

      case MemoryMessage::InvUpdateAck:
        return Protocol::InvUpdateAck;

      case MemoryMessage::DowngradeAck:
        return Protocol::DowngradeAck;

      case MemoryMessage::DownUpdateAck:
        return Protocol::DowngradeUpdateAck;

      default:
        return Protocol::ProtocolError;
    }
  }

public:
  ////////////////////////////////////
  //
  // incoming message handlers...
  //

  void recvNetwork(NetworkTransport trans) {
    theProtocolEngine->enqueue(trans[ProtocolMessageTag]);
  }

  void recvDirectory(DirectoryTransport trans) {
    // just queue it - this response is being polled for
    fromDir.push_back(trans);
  }

  void recvCpu(MemoryTransport trans) {
    boost::intrusive_ptr<tPacket> request = translateMemoryTransport(trans);
    theProtocolEngine->enqueue(request);
  }

  uint32_t queueSize(tVC VC) {
    return theProtocolEngine->queueSize(VC);
  }

  ////////////////////////////////////
  //
  // sending...
  //
  // NOTE: sends should never block. The protocol engine
  // should always be able to queue up all the messages it
  // may have to send in the lifetime of a transaction.
  //

  // perform the actual send
  void
  send(tMessageType message_type,
       const node_id_t       dest,
       const tAddress        address,
       const node_id_t       requester,
       const unsigned        InvalidationsCount,
       const unsigned        HomeNodeSharerFlag,
       const bool            aPrefetchRead,
       boost::intrusive_ptr<TransactionTracker> aTransactionTracker) {
    DBG_(Iface, ( << theProtocolEngine->engineName()
                  << " Sending message " << message_type
                  << " to node " << dest
                  << " for addr=0x" << &std::hex << (uint64_t) address
                  << &std::dec
                  << " requester=" << requester
                  << " inv=" << InvalidationsCount
                  << " HomeSharer=" << HomeNodeSharerFlag));

    // package the message data
    intrusive_ptr<tPacket> packet(
      new tPacket
      ( address
        , dest
        , networkMessageTypeToVC(message_type)
        , myNodeId()
        , requester
        , message_type
        , InvalidationsCount
        , HomeNodeSharerFlag
        , aPrefetchRead
        , aTransactionTracker
      )
    );

    // create the network routing piece
    intrusive_ptr<NetworkMessage> netMsg (new NetworkMessage());
    netMsg->src = myNodeId();
    netMsg->dest = dest;
    netMsg->vc = ( networkMessageTypeToVC(message_type) - 2);
    netMsg->src_port = 0; //ProtocolEngine is at port 0
    netMsg->dst_port = 0; //ProtocolEngine is at port 0
    netMsg->size     =  carriesData(message_type) ? 64 : 0;

    // now make the transport and queue it
    NetworkTransport transport;
    transport.set(ProtocolMessageTag, packet);
    transport.set(NetworkMessageTag, netMsg);
    transport.set(TransactionTrackerTag, aTransactionTracker);
    toNetwork.push_back(transport);
  }

  ////////////////////////////////////
  //
  // CPU operation
  //
  // NOTE: when data are provided with a miss reply, they should be
  // written in the cache even if the request wan an upgrade.
  //

  // Perform cpu operation
  // If data come from a message, the pMsgData pointer should be non-null.
  // It is the service provider's responsibility to copy the data before CpuOp returns.
  // If data come from memory, it is the service provider's responsibility to read them.
  void
  CpuOp(const tCpuOpType     operation,
        const tAddress       address,
        const bool           anAnyInvs,
        boost::intrusive_ptr<TransactionTracker> aTransactionTracker) {
    DBG_(Iface, ( << theProtocolEngine->engineName()
                  << " Cpu Operation " << operation
                  << " addr=0x" << &std::hex << (uint64_t) address
                  << &std::dec
                  << (anAnyInvs == 0 ? " NoInvalidations" : " SomeInvalidations")
                ));

    // If data come from a message, the pointer should be non-null
    // NOTE: in Kraken we do not pass data around. Reintroduce the assert after data support is provided.
    // DBG_Assert((data_src != DATA_FROM_MSG) || (pMsgData != NULL));

    // create a memory message
    intrusive_ptr<MemoryMessage> msg;
    switch (operation) {
      case CPU_INVALIDATE:
        msg = (new MemoryMessage(MemoryMessage::Invalidate, MemoryAddress(address)));
        break;

      case CPU_DOWNGRADE:
        msg = (new MemoryMessage(MemoryMessage::Downgrade, MemoryAddress(address)));
        break;

      case MISS_REPLY:
        msg = (new MemoryMessage(MemoryMessage::MissReply, MemoryAddress(address)));
        msg->reqSize() = 64;
        break;

      case MISS_WRITABLE_REPLY:
        msg = (new MemoryMessage(MemoryMessage::MissReplyWritable, MemoryAddress(address)));
        msg->reqSize() = 64;
        break;

      case UPGRADE_REPLY:
        msg = (new MemoryMessage(MemoryMessage::UpgradeReply, MemoryAddress(address)));
        msg->reqSize() = 64;
        break;

      case PREFETCH_READ_REPLY:
        msg = (new MemoryMessage(MemoryMessage::PrefetchReadReply, MemoryAddress(address)));
        msg->reqSize() = 64;
        break;

      default:
        DBG_Assert(false);
    }
    if (anAnyInvs) {
      msg->setInvs();
    }

    // JZ -> Set core to identify source of the request based on TransactionTracker
    DBG_Assert( aTransactionTracker->initiator() );
    msg->coreIdx() = aTransactionTracker->initiator().get();

    // send it up to the CPU
    MemoryTransport transport;
    transport.set(MemoryMessageTag, msg);
    transport.set(TransactionTrackerTag, aTransactionTracker);
    toCpu.push_back(transport);
  }

  ////////////////////////////////////
  //
  // Directory Lock operation
  //

  // Perform lock operation
  // To avoid race conditions, there should be a way to achieve mutual exclusion between
  // the local cpu and the home engine. For the sake of flexibility, we choose to decouple
  // the locking operation from memory, and provide enough information to achieve fine
  // grain locking (cache line granularity). In reality, it may be implemented as a membus
  // locking mechanism, or have a lock manager at the memory controller. To simplify strange
  // race conditions we decide to implement a lock manager at the local engine (cache controller).
  // There should be a way for the service providers to locate the appropriate directory.

  // LockOp signature
  void
  LockOp(const tLockOpType  operation,
         const tAddress     address) {
    DBG_(Iface, ( << theProtocolEngine->engineName()
                  << " Lock Operation " << operation
                  << " addr=0x" << &std::hex << (uint64_t) address));

    tMemOpType mem_op;
    if (operation == LOCK)
      mem_op = MEM_LOCK;
    else if (operation == UNLOCK)
      mem_op = MEM_UNLOCK;
    else {
      DBG_Assert(operation == LOCK_SQUASH);
      mem_op = MEM_LOCK_SQUASH;
    }

    DirectoryCommand::DirectoryCommand cmd = translateSrvcToDir(mem_op);
    intrusive_ptr<DirectoryMessage> msg (new DirectoryMessage(cmd, MemoryAddress(address)));
    DirectoryTransport transport;
    transport.set(DirectoryMessageTag, msg);
    toDir.push_back(transport);
  }

  bool hasDirectoryResponse() {
    return ! fromDir.empty() ;
  }

  std::pair< tAddress, boost::intrusive_ptr<tDirEntry const> > dequeueDirectoryResponse() {
    std::pair< tAddress, boost::intrusive_ptr<tDirEntry const> > ret_val;
    DirectoryTransport trans = fromDir.front();
    DBG_Assert(trans[DirectoryMessageTag]);
    ret_val.first = trans[DirectoryMessageTag]->addr;
    if (trans[DirectoryMessageTag]->op == DirectoryCommand::Found) {
      DBG_Assert( trans[DirectoryEntryTag] );
      ret_val.second = trans[DirectoryEntryTag];
    }
    fromDir.pop_front();
    return ret_val;
  }

  ////////////////////////////////////
  //
  // Memory operation
  //

  // Perform memory operation
  // If data come from a message, the pMsgData pointer should be non-null.
  // It is the service provider's responsibility to copy the data before MemOp returns.
  // Only directory writes are implemented. We use a global directory pointer for that.
  // There should be a way for the service providers to locate the appropriate directory.

  // MemOp signature for directory reads
  void
  MemOp(const tMemOpType operation,
        const tMemOpDest dest,
        const tAddress   address) {
    DBG_(Iface, ( << theProtocolEngine->engineName()
                  << " Memory Operation " << operation
                  << " addr=0x" << &std::hex << (uint64_t) address
                  << &std::dec
                  << " dest " << dest));

    DBG_Assert((dest == DIRECTORY) && (operation == READ));

    DirectoryCommand::DirectoryCommand cmd = translateSrvcToDir(operation);
    intrusive_ptr<DirectoryMessage> msg (new DirectoryMessage(cmd, MemoryAddress(address)));
    DirectoryTransport transport;
    transport.set(DirectoryMessageTag, msg);
    toDir.push_back(transport);
  }

  // MemOp signature for writes
  void
  MemOp(const tMemOpType operation,
        const tMemOpDest dest,
        const tAddress   address,
        const tDirEntry  dir_entry,
        const void   *   pMsgData = NULL) {
    DBG_(Iface, ( << theProtocolEngine->engineName()
                  << " Memory Operation " << operation
                  << " addr=0x" << &std::hex << (uint64_t) address
                  << &std::dec
                  << " dest " << dest
                  << " ptr_data=0x" << &std::hex << pMsgData));

    // If data come from a message, the pointer should be non-null
    // DBG_Assert(((dest != DATA_FROM_MESSAGE) && (dest != DIRECTORY_AND_DATA_FROM_MESSAGE)) || (pMsgData != NULL));

    if (dest == DIRECTORY || dest == DIRECTORY_AND_DATA_FROM_MESSAGE) {
      if (operation == READ) {
        DBG_Assert(0);  // should use the other signature
      } else if (operation == WRITE) {
        intrusive_ptr<DirectoryMessage> msg (new DirectoryMessage(DirectoryCommand::Set, MemoryAddress(address)));
        intrusive_ptr<DirectoryEntry> entry (new DirectoryEntry(dir_entry));
        DirectoryTransport transport;
        transport.set(DirectoryMessageTag, msg);
        transport.set(DirectoryEntryTag, entry);
        toDir.push_back(transport);

      }
    }

    return;
  }

  ////////////////////////////////////////////////////////////////////////
  //
  // time
  //
  uint64_t getCycleCount(void) const     {
    return Flexus::Core::theFlexus->cycleCount();
  }

};  // class ServiceFlexus

}  // namespace nProtocolEngine

#endif // _SERVICE_FLEXUS_H_
