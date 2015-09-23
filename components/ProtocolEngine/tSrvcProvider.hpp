#ifndef _TSRVC_PROVIDER_H_
#define _TSRVC_PROVIDER_H_

#include "ProtSharedTypes.hpp"
#include <components/Common/Slices/TransactionTracker.hpp>

namespace nProtocolEngine {

class tSrvcProvider {
public:
  virtual ~tSrvcProvider() {};

  // number of nodes in the system
  virtual unsigned numNodes(void) = 0;

  // This is the id of this particular node
  virtual unsigned myNodeId(void) = 0;

  // the node id with the directory for that address
  virtual unsigned nodeForAddress(const tAddress address) = 0;

  // return true if node has directory for that address
  // return false otherwise
  virtual bool isAddressLocal(const tAddress address) = 0;

  virtual int32_t getCPI() = 0;

  /* CMU-ONLY-BLOCK-BEGIN */
  ////////////////////////////////////
  //
  // Functions used to notify the predictors of certain events
  //
  virtual void predNotifyFlush(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) = 0;
  virtual void predNotifyWrite(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) = 0;
  virtual void predNotifyReadPredicted(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) = 0;
  virtual void predNotifyReadNonPredicted(tAddress addr, node_id_t aNode, boost::intrusive_ptr<TransactionTracker> aTracker) = 0;
  /* CMU-ONLY-BLOCK-END */

  ////////////////////////////////////
  //
  // sending...
  //
  // NOTE: sends should never block. The protocol engine
  // should always be able to queue up all the messages it
  // may have to send in the lifetime of a transaction.
  //

  // perform the actual send
  virtual void
  send(tMessageType          message_type,
       const unsigned        dest,
       const tAddress        address,
       const unsigned        requester,
       const unsigned        InvalidationsCount,
       const unsigned        HomeNodeSharerFlag,
       const bool            aFastMessage,
       boost::intrusive_ptr<TransactionTracker> aTracker) = 0;

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
  virtual void
  CpuOp(const tCpuOpType    operation,
        const tAddress      address,
        const bool          AnyInvalidationsFlag,
        boost::intrusive_ptr<TransactionTracker> aTracker) = 0;

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
  virtual void
  LockOp(const tLockOpType   operation,
         const tAddress      address)   = 0;

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
  virtual void
  MemOp(const tMemOpType operation,
        const tMemOpDest dest,
        const tAddress   address) = 0;

  // MemOp signature for writes
  virtual void
  MemOp(const tMemOpType operation,
        const tMemOpDest dest,
        const tAddress   address,
        const tDirEntry  dir_entry,
        const void   *   pMsgData = nullptr) = 0;

  ////////////////////////////////////////////////////////////////////////
  //
  // time
  //
  virtual uint64_t getCycleCount(void) const = 0;

  virtual bool hasDirectoryResponse() = 0;

  virtual std::pair< tAddress, boost::intrusive_ptr<tDirEntry const> > dequeueDirectoryResponse() = 0;

};  // class tSrvcProvider

}  // namespace nProtocolEngine

#endif // _TSRVC_PROVIDER_H_
