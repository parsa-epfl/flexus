#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "he_exec_engine.hpp"
#include "util.hpp"
#include "protocol_engine.hpp"
#include "ProtSharedTypes.hpp"
#include "tsrf.hpp"
#include "tSrvcProvider.hpp"

#include <core/debug/debug.hpp>

#define DBG_DefineCategories HEExecEngine
#define DBG_DeclareCategories ProtocolEngine
#define DBG_SetDefaultOps AddCat(ProtocolEngine) AddCat(HEExecEngine)
#include DBG_Control()

namespace nProtocolEngine {

using namespace Protocol;

void tHEExecEngine::handle_HE_INSTR_SEND(tThread aThread, const uint32_t args) {
  // NOTE: they should match xe.mdef argument fields
  const unsigned packet_type      = (args >> 4) & 0x7;
  const unsigned dest             = (args >> 10) & 0x3;
  const node_id_t home_node = theSrvcProv.nodeForAddress(aThread.address());

  if (aThread.transactionTracker()) {
    aThread.transactionTracker()->setNetworkTrafficRequired(true);
    aThread.transactionTracker()->setFillLevel(eRemoteMem);
  }

  if (dest == 2 /* ToSharers */) {
    unsigned messages_sent = 0;

    //Determine number of messages that will be sent, and set the invalidation
    //count and any-invs flags.
    for (uint32_t i = 0; i < theSrvcProv.numNodes(); i++) {
      if ( aThread.isSharer(i) && (aThread.requester() != i) && (home_node != i)) {
        messages_sent++;
      }
    }

    // set the invalidations counter and any-invs flag
    aThread.setInvalidationCount(messages_sent);
    aThread.setAnyInvalidations( messages_sent > 0 || aThread.isSharer(home_node) );

    //Send the messages
    for (uint32_t i = 0; i < theSrvcProv.numNodes(); i++) {
      if ( aThread.isSharer(i) && (aThread.requester() != i) && (home_node != i)) {

        DBG_(Iface, (  << theEngineName << " [" << aThread << "]" << " Send to sharer node " << i << " packet " << packet_type ));

        theSrvcProv.send
        ( map_EngineMessageType_to_MessageType(packet_type, HE_INSTR_SEND)   // type
          , i // dest node
          , aThread.address()
          , aThread.requester()
          , aThread.invalidationCount()
          , aThread.anyInvalidations()
          , aThread.isPrefetch()
          , aThread.transactionTracker()
        );
      }
    }

  } else {

    node_id_t dest_node = 0;
    std::string dest_name;
    switch (dest) {
      case 0: //ToRequester
        dest_node = aThread.requester();
        dest_name = "requester";
        break;
      case 1: //ToOwner
        dest_node = aThread.owner();
        dest_name = "owner";
        break;
      case 3: //ToRespondent
        dest_node = aThread.respondent();
        dest_name = "respondent";
        break;
      default:
        DBG_Assert(false);
    }

    DBG_(Iface, (  << theEngineName << " [" << aThread << "]" << " Send to " << dest_name << " node " << dest_node << " packet " << packet_type ));

    // perform the actual send
    theSrvcProv.send
    ( map_EngineMessageType_to_MessageType(packet_type, HE_INSTR_SEND)
      , dest_node
      , aThread.address()
      , aThread.requester()
      , aThread.invalidationCount()
      , aThread.anyInvalidations()
      , aThread.isPrefetch()
      , aThread.transactionTracker()
    );

  }

  DBG_Assert(packet_type != HE_TX_Error);
}

void tHEExecEngine::handle_HE_INSTR_SEND_FWD(tThread aThread, const uint32_t args) {
  // NOTE: they should match xe.mdef argument fields
  const unsigned packet_type = (args & 0x0F0) >> 4;
  const unsigned dest = (args & 0xC00) >> 10;
  const node_id_t home_node = theSrvcProv.nodeForAddress(aThread.address());

  if (aThread.transactionTracker()) {
    aThread.transactionTracker()->setNetworkTrafficRequired(true);
    aThread.transactionTracker()->setFillLevel(eRemoteMem);
  }

  if (dest == 2 /* ToSharers */) {
    unsigned messages_sent = 0;

    //Determine number of messages that will be sent, and set the invalidation
    //count and any-invs flags.
    for (uint32_t i = 0; i < theSrvcProv.numNodes(); i++) {
      if ( aThread.isSharer(i) && (aThread.requester() != i) && (home_node != i)) {
        messages_sent++;
      }
    }

    // set the invalidations counter and any-invs flag
    aThread.setInvalidationCount(messages_sent);
    aThread.setAnyInvalidations( messages_sent > 0 || aThread.isSharer(home_node) );

    //Send the messages
    for (uint32_t i = 0; i < theSrvcProv.numNodes(); i++) {
      if ( aThread.isSharer(i) && (aThread.requester() != i) && (home_node != i)) {

        DBG_(Iface, (  << theEngineName << " [" << aThread << "]" << " SendFwd to sharer node " << i << " packet " << packet_type ));

        theSrvcProv.send
        ( map_EngineMessageType_to_MessageType(packet_type, HE_INSTR_SEND_FWD)   // type
          , i // dest node
          , aThread.address()
          , aThread.requester()
          , aThread.invalidationCount()
          , aThread.anyInvalidations()
          , aThread.isPrefetch()
          , aThread.transactionTracker()
        );
      }
    }

  } else {

    node_id_t dest_node = 0;
    std::string dest_name;
    switch (dest) {
      case 0: //ToRequester
        dest_node = aThread.requester();
        dest_name = "requester";
        break;
      case 1: //ToOwner
        dest_node = aThread.owner();
        dest_name = "owner";
        break;
      case 3: //ToRespondent
        dest_node = aThread.respondent();
        dest_name = "respondent";
        break;
      default:
        DBG_Assert(false);
    }
    DBG_(Iface, (  << theEngineName << " [" << aThread << "]" << " Send to " << dest_name << " node " << dest_node << " packet " << packet_type ));

    // perform the actual send
    theSrvcProv.send
    ( map_EngineMessageType_to_MessageType(packet_type, HE_INSTR_SEND_FWD)
      , dest_node
      , aThread.address()
      , aThread.requester()
      , aThread.invalidationCount()
      , aThread.anyInvalidations()
      , aThread.isPrefetch()
      , aThread.transactionTracker()
    );

  }

  DBG_Assert(packet_type != HE_TX_FWD_Error);
}

void tHEExecEngine::handle_HE_INSTR_RECEIVE(tThread aThread, const uint32_t args) {
  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " Receive... will suspend execution for a while"));

  theThreadScheduler.waitForPacket(aThread);
}

void tHEExecEngine::handle_HE_INSTR_TEST(tThread aThread, const uint32_t args) {
  // NOTE: they should match xe.mdef argument fields
  const unsigned test = (args & 0x070) >> 4;

  // what to test
  switch (test) {
    case 0: // InvalidationsCount
      DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Test InvalidationsCount"));
      aThread.relativeJump(!! aThread.invalidationCount());
      break;

    case 1: { // CurrentOwnerInTempReg
      const unsigned current_owner = aThread.owner();
      const unsigned temp_reg = aThread.tempReg();
      DBG_(Iface, ( << theEngineName << " [" << aThread << "]"
                    << " Test CurrentOwnerInTempReg"
                    << " current_owner=" << current_owner
                    << " temp_reg=" << temp_reg
                  ));
      aThread.relativeJump(current_owner == temp_reg);
      break;
    }

    case 2: // RequesterIsSharer
      DBG_(Iface, ( << theEngineName
                    << " [" << aThread << "]"
                    << " Test RequesterIsSharer"));
      aThread.relativeJump( aThread.isSharer(aThread.requester()) );
      break;

    case 4: // IsPrefetch
      DBG_(Iface, ( << theEngineName
                    << " [" << aThread << "]"
                    << " Test IsPrefetchRead "
                    << aThread.isPrefetch()));
      aThread.relativeJump(aThread.isPrefetch());
      break;

      // ConfigurationReg
    case 7:
      DBG_(Iface, ( << theEngineName
                    << " [" << aThread << "]"
                    << " Test ConfigurationReg"));
      aThread.relativeJump(configurationReg());
      break;

    default:
      DBG_Assert(false, ( << theEngineName << " [" << aThread << "]" << " Invalid HE TEST: " << test));
  }
}

unsigned tHEExecEngine::extractSourceValue(tThread aThread, unsigned aSourceReg, unsigned aTransferType) {
  switch (aSourceReg) {
    case 0: //Requester
      return aThread.requester();
    case 1: //Owner
      return aThread.owner();
    case 2: //Respondent or HomeNode
      if (aTransferType == 1 /*single bit*/) {
        return theSrvcProv.nodeForAddress(aThread.address());
      } else {
        return aThread.respondent();
      }
    case 4: //TempReg
      return aThread.tempReg();
    default: //Can occur when setting dir state or temp reg
      return aThread.requester();
  }
}

void tHEExecEngine::handle_HE_INSTR_SET(tThread aThread, const uint32_t args) {
  // NOTE: they should match xe.mdef argument fields
  unsigned destination_reg = (args & 0x070) >> 4;
  unsigned transfer_type = (args & 0x180) >> 7;
  unsigned source_reg  = (args & 0x600) >> 9;
  bool single_bit_value = !!(args & 0x800);

  unsigned source_value = extractSourceValue(aThread, source_reg, transfer_type);

  switch (destination_reg) {
    case 0: //Directory State
      DBG_Assert(transfer_type == 0);  // value
      DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Set DirState " << static_cast<tDirState>(source_reg) ));
      aThread.setDirState( static_cast<tDirState>(source_reg) );
      break;

    case 1: //Owner
      DBG_Assert(transfer_type == 0);  // value
      DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Set Owner " << source_value));
      aThread.setOwner(source_value);
      break;

    case 2: //Sharers bits
      if (transfer_type == 1 /*single bit*/ ) {
        if (single_bit_value) {
          DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Set Sharer " << source_value));
          aThread.addSharer(source_value);
        } else {
          DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Clear Sharer " << source_value));
          aThread.clearSharer(source_value);
        }
      } else {
        DBG_Assert( transfer_type == 0 );
        DBG_Assert( single_bit_value == 0 );
        DBG_(Iface, ( << theEngineName << " [" << aThread << "] Clear all Sharers"));
        aThread.clearSharers();
      }
      break;

    case 3: //Temp Reg
      DBG_Assert(transfer_type == 0);  // value
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set TempReg " << source_value));
      aThread.setTempReg(source_value);
      break;

    case 4: //Invalidation Count
      switch ( transfer_type ) {
        case 0: //Value
          DBG_Assert( single_bit_value == 0 );
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set InvalidationsCount 0"));
          aThread.setInvalidationCount(0);
          break;
        case 2: //Decrement
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Decrement InvalidationsCount to " << aThread.invalidationCount()));
          DBG_Assert(aThread.invalidationCount() > 0);
          aThread.setInvalidationCount(aThread.invalidationCount() - 1);
          break;
        default:
          DBG_Assert( false, ( << "Unsupported transfer_type: " << transfer_type) );
      }
      break;

    case 5: //AnyInvalidations
      DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Set AnyInvalidations " << single_bit_value ));
      DBG_Assert(transfer_type == 0);  // value
      aThread.setAnyInvalidations(single_bit_value);
      break;

    case 6: //IsPrefetchRead
      DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Set IsPrefetchRead " << single_bit_value ));
      DBG_Assert(transfer_type == 0);  // value
      aThread.setPrefetch(single_bit_value);
      break;

    default:
      DBG_Assert( false, ( << "Invalidation destination register " << destination_reg << " in HE SET" ) );
  }

}

void tHEExecEngine::handle_HE_INSTR_UNLOCK(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " DirEntry UnLock"));
  theSrvcProv.LockOp(UNLOCK, aThread.address());
}

std::pair<tCpuOpType, std::string> determineHECPUOp( uint32_t args ) {
  unsigned op = (args >> 4) & 0xF;
  switch (op) {
    case 0:
      return std::make_pair( CPU_INVALIDATE, std::string("Invalidate") );
    case 1:
      return std::make_pair( CPU_DOWNGRADE, std::string("Downgrade") );
    case 8:
      return std::make_pair( MISS_REPLY, std::string("MissReply") );
    case 9:
      return std::make_pair( MISS_WRITABLE_REPLY, std::string("MissWriteableReply") );
    case 10:
      return std::make_pair( UPGRADE_REPLY, std::string("UpgradeReply") );
    case 11:
      return std::make_pair( PREFETCH_READ_REPLY, std::string("PrefetchReadReply") );
    default:
      DBG_Assert( false,  ( << "Invalid HE CPU_OP: " << args) );
      return std::make_pair(CPU_INVALIDATE, std::string("Invalidate"));
  }
}

void tHEExecEngine::handle_HE_INSTR_CPU_OP(tThread aThread, const uint32_t args) {

  tCpuOpType operation;
  std::string operation_descr;
  boost::tie( operation, operation_descr) = determineHECPUOp( args );

  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " Cpu " << operation_descr
                << (aThread.anyInvalidations() ? " SomeInvalidations" : " NoInvalidations")));

  theSrvcProv.CpuOp(operation, aThread.address(), aThread.anyInvalidations(), aThread.transactionTracker());

}

void tHEExecEngine::handle_HE_INSTR_WRITE_DIRECTORY(tThread aThread) {
  aThread.updateDirEntry();
  DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Write Directory " << aThread.dirEntry() ));
  theSrvcProv.MemOp(WRITE, DIRECTORY, aThread.address(), aThread.dirEntry());

}

/* CMU-ONLY-BLOCK-BEGIN */
void tHEExecEngine::doPredictorNotify(tThread aThread) {
  if (! theSrvcProv.isAddressLocal( aThread.address() ) ) {
    return;
  }

  switch (aThread.type()) {
    case FlushReq:
      DBG_(Iface, ( << theEngineName << " Sending eFlush (FlushReq) " << aThread << " by " << aThread.respondent()) );
      theSrvcProv.predNotifyFlush(aThread.address(), aThread.respondent(), aThread.packet()->transactionTracker());
      break;

    case LocalFlush:
      DBG_(Iface, ( << theEngineName << " Sending eFlush (LocalFlush) " << aThread << " by " << theSrvcProv.myNodeId()) );
      theSrvcProv.predNotifyFlush(aThread.address(), theSrvcProv.myNodeId(), aThread.transactionTracker());
      break;

    case ReadReq:
      if (aThread.isPrefetch()) {
        DBG_(Iface, ( << theEngineName << " Sending eReadPredicted (ReadReq) " << aThread << " by " << aThread.requester()) );
        theSrvcProv.predNotifyReadPredicted(aThread.address(), aThread.requester(), aThread.transactionTracker());
      } else {
        DBG_(Iface, ( << theEngineName << " Sending eReadNonPredicted (ReadReq) " << aThread << " by " << aThread.requester()) );
        theSrvcProv.predNotifyReadNonPredicted(aThread.address(), aThread.requester(), aThread.transactionTracker());
      }
      break;

    case LocalRead:
      DBG_(Iface, ( << theEngineName << " Sending eReadNonPredicted (LocalRead) " << aThread << " by " << theSrvcProv.myNodeId()) );
      theSrvcProv.predNotifyReadNonPredicted(aThread.address(), theSrvcProv.myNodeId(), aThread.transactionTracker());
      break;

    case LocalPrefetchRead:
      DBG_(Iface, ( << theEngineName << " Sending eReadPredicted (LocalPrefetchRead) " << aThread << " by " << theSrvcProv.myNodeId()) );
      theSrvcProv.predNotifyReadPredicted(aThread.address(), theSrvcProv.myNodeId(), aThread.transactionTracker());
      break;

    case WriteReq:
    case UpgradeReq:
      DBG_(Iface, ( << theEngineName << " Sending eWrite (Write|UpgradeReq) " << aThread << " by " << aThread.requester()) );
      theSrvcProv.predNotifyWrite(aThread.address(), aThread.requester(), aThread.transactionTracker());
      break;

    case LocalWriteAccess:
    case LocalUpgradeAccess:
      DBG_(Iface, ( << theEngineName << " Sending eWrite (LocalWrite|LocalUpgrade) " << aThread << " by " << theSrvcProv.myNodeId()) );
      theSrvcProv.predNotifyWrite(aThread.address(), theSrvcProv.myNodeId(), aThread.transactionTracker());
      break;

    default:
      break;
  }
}
/* CMU-ONLY-BLOCK-END */

void tHEExecEngine::handle_HE_INPUT_Q_OP(tThread aThread, const uint32_t args) {
  // retrieve VC of queue we are interested in
  unsigned op = (args & 0x030) >> 4;

  if (op == 0) {
    //Dequeue
    DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " InputQueueOp DEQUEUE"));
    doPredictorNotify(aThread); /* CMU-ONLY */
  } else {
    //Refuse
    DBG_Assert( op == 1 );
    DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " InputQueueOp REFUSE"));
    theThreadScheduler.refusePacket(aThread);
  }

}

void
tHEExecEngine::execute(tThread aThread,
                       const unsigned      op_code,
                       const uint32_t args,
                       const uint32_t ms_pc ) {
  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " Execute op=" << op_code
                << " args=0x" << &std::hex << (args >> 4)));
  //
  // Micro code operation decoding
  //
  switch (op_code) {                      // decode instruction *****
    case HE_INSTR_SEND:      // send a reply packet (requester = packet destination node)
      handle_HE_INSTR_SEND(aThread, args);
      break;

    case HE_INSTR_SEND_FWD:        // send a fwd packet (request = tsrf.request)
      handle_HE_INSTR_SEND_FWD(aThread, args);
      break;

    case HE_INSTR_RECEIVE:           // receive a packet
      handle_HE_INSTR_RECEIVE(aThread, args);
      break;

    case HE_INSTR_TEST:              // test things
      handle_HE_INSTR_TEST(aThread, args);
      break;

    case HE_INSTR_SET:               // set directory fields or internal registers
      handle_HE_INSTR_SET(aThread, args);
      break;

    case HE_INSTR_UNLOCK:            // unlock directory entry
      handle_HE_INSTR_UNLOCK(aThread);
      break;

    case HE_INSTR_CPU_OP:            // operation on cpu
      handle_HE_INSTR_CPU_OP(aThread, args);
      break;

    case HE_INSTR_WRITE_DIRECTORY:            // write state to directory
      handle_HE_INSTR_WRITE_DIRECTORY(aThread);
      break;

    case HE_INPUT_Q_OP:            // operation on input message queue
      handle_HE_INPUT_Q_OP(aThread, args);
      break;

    default:
      DBG_Assert(false);
      break;
  }
}

void tHEExecEngine::deliverReply(tThread aThread, tMessageType aType) {
  const uint32_t internal_type = map_MessageType_to_EngineMessageType(aType, HE_INSTR_RECEIVE);

  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " deliver_reply_to_microcode"
                << " type=" << aType
                << " internal_type=" << internal_type));

  aThread.relativeJump(internal_type);
}

void tHEExecEngine::setTransactionType(tMessageType aType, tThread aThread, tDirState aState) {
  if (! aThread.transactionTracker()) {
    return;
  }

  aThread.transactionTracker()->setResponder(theSrvcProv.myNodeId());

  switch (aState) {
    case DIR_STATE_INVALID:
      aThread.transactionTracker()->setPreviousState(eInvalid);
      break;
    case DIR_STATE_SHARED:
      aThread.transactionTracker()->setPreviousState(eShared);
      break;
    case DIR_STATE_MODIFIED:
      aThread.transactionTracker()->setPreviousState(eModified);
      break;
  }

  //Determine if the transaction is a read or a write
  switch (aType) {
    case ReadReq:
    case LocalRead:
    case LocalPrefetchRead:
      if (aThread.dirState() == DIR_STATE_MODIFIED) {
        aThread.transactionTracker()->setFillType(eCoherence);
      } else if (aThread.wasModified() && (! aThread.isSharer(aThread.requester()) )) {
        aThread.transactionTracker()->setFillType(eCoherence);
      } else if (! aThread.wasSharer(aThread.requester()) ) {
        aThread.transactionTracker()->setFillType(eCold);
      } else {
        aThread.transactionTracker()->setFillType(eReplacement);
      }
      break;

    case WriteReq:
    case UpgradeReq:
    case LocalWriteAccess:
    case LocalUpgradeAccess:
      if (aThread.dirState() == DIR_STATE_SHARED && aThread.wasModified() && aThread.isOnlySharer(aThread.requester())) {
        aThread.transactionTracker()->setFillType(eReplacement);
      } else if ( !aThread.wasModified() && ( aThread.wasNoOtherSharer(aThread.requester())) ) {
        aThread.transactionTracker()->setFillType(eCold);
      } else {
        aThread.transactionTracker()->setFillType(eCoherence);
      }
      break;

    default:
      //Don't mark the transaction type
      break;
  }
}

unsigned tHEExecEngine::getEntryPoint(tMessageType aType, tThread aThread, tDirState aState) {
  unsigned pc = 0;

  setTransactionType(aType, aThread, aState);

  switch (aType) {
    case ReadReq:
      switch (aState) {
        case DIR_STATE_INVALID:
          pc = HE_EP_ReadReq_Invalid;
          HE_ReadReq_Invalid_cnt ++;
          break;
        case DIR_STATE_SHARED:
          pc = HE_EP_ReadReq_Shared;
          HE_ReadReq_Shared_cnt ++;
          break;
        case DIR_STATE_MODIFIED:
          pc = HE_EP_ReadReq_Modified;
          HE_ReadReq_Modified_cnt ++;
          break;
        default:
          DBG_Assert(false);
      }
      break;

    case WriteReq:
      switch (aState) {
        case DIR_STATE_INVALID:
          pc = HE_EP_WriteReq_Invalid;
          HE_WriteReq_Invalid_cnt ++;
          break;
        case DIR_STATE_SHARED:
          pc = HE_EP_WriteReq_Shared;
          HE_WriteReq_Shared_cnt ++;
          break;
        case DIR_STATE_MODIFIED:
          pc = HE_EP_WriteReq_Modified;
          HE_WriteReq_Modified_cnt ++;
          break;
        default:
          DBG_Assert(false);
      }
      break;

    case UpgradeReq:
      switch (aState) {
        case DIR_STATE_INVALID:
          pc = HE_EP_UpgradeReq_Invalid;
          HE_UpgradeReq_Invalid_cnt ++;
          break;
        case DIR_STATE_SHARED:
          pc = HE_EP_UpgradeReq_Shared;
          HE_UpgradeReq_Shared_cnt ++;
          break;
        case DIR_STATE_MODIFIED:
          pc = HE_EP_UpgradeReq_Modified;
          HE_UpgradeReq_Modified_cnt ++;
          break;
        default:
          DBG_Assert(false);
      }
      break;

    case FlushReq:
      DBG_Assert((aState == DIR_STATE_MODIFIED),
                 ( << theEngineName << " state = " << aState << " thread = " << aThread));
      pc = HE_EP_FlushReq;
      HE_FlushReq_cnt ++;
      break;

    case WritebackReq:
      DBG_Assert((aState == DIR_STATE_MODIFIED),
                 ( << theEngineName << " state = " << aState << " thread = " << aThread));
      pc = HE_EP_WritebackReq;
      HE_WritebackReq_cnt ++;
      break;

    case LocalRead:
      DBG_Assert((aState == DIR_STATE_MODIFIED),
                 ( << theEngineName << " state = " << aState << " thread = " << aThread));
      pc = HE_EP_Local_Read;
      HE_Local_Read_cnt ++;
      break;

    case LocalWriteAccess:
      switch (aState) {
        case DIR_STATE_SHARED:
          pc = HE_EP_Local_WriteAccess_Shared;
          HE_Local_WriteAccess_Shared_cnt ++;
          break;
        case DIR_STATE_MODIFIED:
          pc = HE_EP_Local_WriteAccess_Modified;
          HE_Local_WriteAccess_Modified_cnt ++;
          break;
        default:
          DBG_Assert(0);
      }
      break;

    case LocalUpgradeAccess:
      DBG_Assert(aState == DIR_STATE_SHARED);
      pc = HE_EP_Local_UpgradeAccess;
      HE_Local_UpgradeAccess_cnt ++;
      break;

    case LocalPrefetchRead:
      DBG_Assert(aState == DIR_STATE_MODIFIED);
      pc = HE_EP_Local_Prefetch_Read;
      HE_Local_Prefetch_Read_cnt ++;
      break;

    case LocalFlush:
    case LocalDropHint:
    case LocalEvict:
    default:
      DBG_Assert(0);
  }

  return pc;
}

unsigned tHEExecEngine::map_MessageType_to_EngineMessageType(tMessageType type, const unsigned instr) const {
  switch (type) {

      // Requests From Cache to Directory
    case ReadReq:
      break;                               // VC 0
    case WriteReq:
      break;                               // VC 0
    case UpgradeReq:
      break;                               // VC 0
    case FlushReq:
      return HE_RX_FlushReq;               // VC 1 Contains data
    case WritebackReq:
      return HE_RX_WritebackReq;           // VC 1 Contains data
      // case ReadExReq:           // - Not used
      // case WriteAllReq:         // - Not used
      // case ReadAndForgetReq:    // - Not used

      // Requests From Directory to Cache
    case LocalInvalidationReq:
      return HE_TX_LocalInvalidationReq;   // VC 1
    case RemoteInvalidationReq:
      return HE_TX_RemoteInvalidationReq;  // VC 1
    case ForwardedReadReq:
      return HE_TX_ForwardedReadReq;       // VC 1
    case ForwardedWriteReq:
      return HE_TX_ForwardedWriteReq;      // VC 1
    case RecallReadReq:
      return HE_TX_RecallReadReq;          // VC 1
    case RecallWriteReq:
      return HE_TX_RecallWriteReq;         // VC 1

      // Replies From Cache to Directory
    case ForwardedWriteAck:
      return HE_RX_ForwardedWriteAck;      // VC 2
    case ForwardedReadAck:
      return HE_RX_ForwardedReadAck;       // VC 2 Contains data
    case RecallReadAck:
      return HE_RX_RecallReadAck;          // VC 2 Contains data
    case RecallWriteAck:
      return HE_RX_RecallWriteAck;         // VC 2 Contains data
    case LocalInvalidationAck:
      return HE_RX_LocalInvalidationAck;   // VC 2

      // Replies From Directory to Cache
    case ReadAck:
      return HE_TX_ReadAck;                // VC 1 Contains data
    case WriteAck:
      return HE_TX_WriteAck;               // VC 1 Contains data and #inval acks to expect
    case UpgradeAck:
      return HE_TX_UpgradeAck;             // VC 1 #inval acks to expect
    case WritebackAck:
      return HE_TX_WritebackAck;           // VC 2
    case WritebackStaleRdAck:
      return HE_TX_WritebackStaleRdAck;    // VC 2
    case WritebackStaleWrAck:
      return HE_TX_WritebackStaleWrAck;    // VC 2
    case FlushAck:
      return HE_TX_FlushAck;               // VC 1

      // Replies From Cache to Cache
    case RemoteInvalidationAck:
      break;                               // VC 2
    case ReadFwd:
      break;                               // VC 2 Contains Data
    case WriteFwd:
      break;                               // VC 2 Contains Data

      // Replies from Caches to PE
    case InvAck:
      return HE_RX_InvAck;                // The cache acknowledges an Invalidate CPU_OP
    case InvUpdateAck:
      return HE_RX_InvUpdateAck;          // Contains data. The cache acknowledges an Invalidate CPU_OP.
    case DowngradeAck:
      return HE_RX_DowngradeAck;          // The cache acknowledges a Downgrade CPU_OP
    case DowngradeUpdateAck:
      return HE_RX_DowngradeUpdateAck;    // Contains data. The cache acknowledges a Downgrade CPU_OP.

      // Error in protocol
    case ProtocolError:
    default: {
      if (instr == HE_INSTR_SEND)
        return HE_TX_Error;                     // ERROR IN PROTOCOL!!!
      else
        return HE_TX_FWD_Error;                 // ERROR IN PROTOCOL!!!
    }
  }

  DBG_Assert(0, ( << "message " << type));
  return HE_TX_Error;
}

tMessageType tHEExecEngine::map_EngineMessageType_to_MessageType(unsigned type, const unsigned instr) const {
  switch (instr) {
    case HE_INSTR_SEND:
      switch (type) {
          // Replies From Directory to Cache
        case HE_TX_ReadAck:
          return ReadAck;                // VC 1 Contains data
        case HE_TX_WriteAck:
          return WriteAck;               // VC 1 Contains data and #inval acks to expect
        case HE_TX_UpgradeAck:
          return UpgradeAck;             // VC 1 #inval acks to expect
        case HE_TX_WritebackAck:
          return WritebackAck;           // VC 2
        case HE_TX_FlushAck:
          return FlushAck;               // VC 1

          // Error in protocol
        case HE_TX_Error:
          return ProtocolError;          // ERROR IN PROTOCOL!!!

        default:
          DBG_Assert(0, ( << "message " << type));
      }
      break;

    case HE_INSTR_SEND_FWD:
      switch (type) {

          // Requests From Directory to Cache
        case HE_TX_LocalInvalidationReq:
          return LocalInvalidationReq;   // VC 1
        case HE_TX_RemoteInvalidationReq:
          return RemoteInvalidationReq;  // VC 1
        case HE_TX_ForwardedReadReq:
          return ForwardedReadReq;       // VC 1
        case HE_TX_ForwardedWriteReq:
          return ForwardedWriteReq;      // VC 1
        case HE_TX_RecallReadReq:
          return RecallReadReq;          // VC 1
        case HE_TX_RecallWriteReq:
          return RecallWriteReq;         // VC 1

          // Replies From Directory to Cache
        case HE_TX_WritebackStaleRdAck:
          return WritebackStaleRdAck;    // VC 2
        case HE_TX_WritebackStaleWrAck:
          return WritebackStaleWrAck;    // VC 2

          // Error in protocol
        case HE_TX_FWD_Error:
          return ProtocolError;          // ERROR IN PROTOCOL!!!

        default:
          DBG_Assert(0, ( << "message " << type));
      }

      break;
  }

  DBG_Assert(0, ( << "message " << type));
  return ProtocolError;
}

}  // namespace nProtocolEngine
