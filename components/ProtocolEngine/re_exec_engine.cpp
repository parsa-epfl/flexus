#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <core/debug/debug.hpp>

#include "re_exec_engine.hpp"
#include "util.hpp"
#include "protocol_engine.hpp"
#include "ProtSharedTypes.hpp"
#include "tsrf.hpp"
#include "tSrvcProvider.hpp"

#define DBG_DefineCategories REExecEngine
#define DBG_DeclareCategories ProtocolEngine
#define DBG_SetDefaultOps AddCat(ProtocolEngine) AddCat(REExecEngine)
#include DBG_Control()

namespace nProtocolEngine {

using namespace Protocol;

void tREExecEngine::handle_RE_INSTR_SEND(tThread aThread, const uint32_t args) {
  // NOTE: they should match xe.mdef argument fields
  unsigned packet_type = (args & 0x0F0) >> 4;
  unsigned dest = (args & 0xC00) >> 10;

  node_id_t dest_node = 0;
  std::string dest_node_name;
  switch (dest) {
    case 0: //ToRequester
      dest_node = aThread.requester();
      dest_node_name = "requester";
      break;
    case 1:
      dest_node = theSrvcProv.nodeForAddress(aThread.address());
      dest_node_name = "directory";
      break;
    default:
      DBG_Assert(false);
  }

  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " Send to " << dest_node_name << " node " << dest_node
                << " packet " << packet_type
              ));

  // perform the actual send
  theSrvcProv.send(map_EngineMessageType_to_MessageType(packet_type, RE_INSTR_SEND),
                   dest_node,
                   aThread.address(),
                   aThread.requester(),
                   aThread.invalidationCount(),
                   aThread.anyInvalidations(),
                   aThread.isPrefetch(),
                   aThread.transactionTracker());

  DBG_Assert(packet_type != RE_TX_Error,
             ( << theEngineName
               << " Thread[" << aThread << "]"
               << " Send to " << dest_node_name
               << " directory= " << (theSrvcProv.nodeForAddress(aThread.address()))
               << " requester= " << aThread.requester()
               << " packet " << packet_type
             ));

}

void tREExecEngine::handle_RE_INSTR_SEND_RACER(tThread aThread, const uint32_t args) {

  // NOTE: they should match xe.mdef argument fields
  unsigned packet_type = (args & 0x0F0) >> 4;
  unsigned dest = (args & 0xC00) >> 10;

  node_id_t dest_node = 0;
  std::string dest_node_name;
  switch (dest) {
    case 0: //ToRequester
      dest_node = aThread.invAckReceiver();
      dest_node_name = "InvAckReceiver";
      break;
    case 1:
      dest_node = aThread.fwdRequester();
      dest_node_name = "FwdRequester";
      break;
    case 2:
      dest_node = aThread.racer();
      dest_node_name = "racer";
      break;
    default:
      DBG_Assert(false);
  }

  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " Send to " << dest_node_name << " node " << dest_node
                << " packet " << packet_type
              ));

  // perform the actual send
  theSrvcProv.send(map_EngineMessageType_to_MessageType(packet_type, RE_INSTR_SEND_RACER), // type
                   dest_node,                                                               // destination
                   aThread.address(),                                                       // address
                   aThread.requester(),                                                     // requester
                   aThread.invalidationCount(),                                             // inval count
                   aThread.anyInvalidations(),                                              // any invalidations
                   aThread.isPrefetch(),
                   aThread.transactionTracker());

  DBG_Assert(packet_type != RE_TX_Error,
             ( << theEngineName
               << " [" << aThread << "]"
               << " Send to dest " << dest
               << " node " << aThread.requester()
               << " packet " << packet_type
             ));
}

void tREExecEngine::handle_RE_INSTR_RECEIVE(tThread aThread, const uint32_t args) {
  DBG_(Iface, ( << theEngineName << " [" << aThread << "] Receive... will suspend execution for a while "));
  theThreadScheduler.waitForPacket(aThread);
}

void tREExecEngine::handle_RE_INSTR_RECEIVE_DEFERRED_JUMP(tThread aThread, const uint32_t args) {
  DBG_(Iface, ( << theEngineName << " [" << aThread << "] ReceiveDeferredJump... decoding 2nd level"));
  aThread.resolveDeferredJump();
}

void tREExecEngine::handle_RE_INSTR_TEST(tThread aThread, const uint32_t args) {

  // NOTE: they should match xe.mdef argument fields
  unsigned dest = ((args >> 4) & 0xF);

  switch (dest) {
    case 0: // InvalidationAckCount
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test InvalidationAckCount " << aThread.invalidationCount()));
      aThread.relativeJump( !! aThread.invalidationCount());
      break;
    case 1: // InvalidationReceived
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test InvalidationReceived " << aThread.invReceived()));
      aThread.relativeJump(aThread.invReceived());
      break;

    case 2: // InvalidationAckReceiver
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test InvalidationAckReceiver " << aThread.invAckReceiver()));
      aThread.relativeJump(aThread.invAckReceiver());
      break;

    case 3: // InvAckForFwdExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test InvAckForFwdExpected " << aThread.invAckForFwdExpected()));
      aThread.relativeJump(aThread.invAckForFwdExpected());
      break;

    case 4: // DowngradeAckExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test DowngradeAckExpected " << aThread.downgradeAckExpected()));
      aThread.relativeJump(aThread.downgradeAckExpected());
      break;

    case 5: // RequestReplyToRacer
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test RequestReplyToRacer " << aThread.requestReplyToRacer()));
      aThread.relativeJump(aThread.requestReplyToRacer());
      break;

    case 6: // FwdReqOrStaleAckExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test FwdReqOrStaleAckExpected " << aThread.fwdReqOrStaleAckExpected()));
      aThread.relativeJump(aThread.fwdReqOrStaleAckExpected());
      break;

    case 7: // FwdRequester
      DBG_Assert(false, ( << "This switch statement branch probably doesn't work for > 32 cpus") );
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test FwdRequester " << aThread.fwdRequester()));
      aThread.relativeJump(aThread.fwdRequester());
      break;

    case 8: // WritebackAckExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test WritebackAckExpected " << aThread.writebackAckExpected()));
      aThread.relativeJump(aThread.writebackAckExpected());
      break;

    case 9: // AnyInvalidations
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test AnyInvalidations" << aThread.anyInvalidations()));
      aThread.relativeJump(aThread.anyInvalidations());
      break;

    case 10: // IsPrefetchRead
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Test IsPrefetchRead " << aThread.isPrefetch()));
      aThread.relativeJump(aThread.isPrefetch());
      break;

    default:
      DBG_Assert(false);
  }

}

void tREExecEngine::handle_RE_INSTR_SET_RACER(tThread aThread, const uint32_t args) {

  // NOTE: they should match xe.mdef argument fields
  unsigned dest = (args >> 4) & 0x07;
  unsigned val  = (args >> 9) & 0x07;

  // get destination
  switch (dest) {
    case 0: // InvAckForFwdExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set InvAckForFwdExpected (acks) val=" << val));
      aThread.setInvAckForFwdExpected( val );
      break;

    case 1: // DowngradeAckExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set DowngradeAckExpected (acks) val=" << val));
      aThread.setDowngradeAckExpected(val);
      break;

    case 2: // RequestReplyToRacer
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set RequestReplyToRacer (acks) val=" << val));
      aThread.setRequestReplyToRacer(val);
      break;

    case 3: // FwdReqOrStaleAckExpected
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set FwdReqOrStaleAckExpected (acks) val=" << val));
      aThread.setFwdReqOrStaleAckExpected(val);
      break;

    case 4: { // fwd requester
      node_id_t dest_node = 0;
      std::string dest_node_name;

      switch (val) {
        case 0: // ToRequester
          dest_node = aThread.requester();
          dest_node_name = "ToRequester";
          break;

        case 1: // ToOwner
          dest_node = aThread.owner();
          dest_node_name = "ToOwner";
          break;

        case 2: //ToDirectory
          dest_node = theSrvcProv.nodeForAddress(aThread.address());
          dest_node_name = "ToDirectory";
          break;

        case 3: // ToRespondent
          dest_node = aThread.respondent();
          dest_node_name = "ToRespondent";
          break;

        case 4: // ToRacer
          dest_node = aThread.racer();
          dest_node_name = "ToRacer";
          break;

        default:
          DBG_Assert(false);
      }

      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set FwdRequester " << dest_node_name << " node " << dest_node));
      aThread.setFwdRequester(dest_node);
      break;
    }

    // WritebackAckExpected
    case 5:
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set WritebackAckExpected (acks) val=" << val));
      aThread.setWritebackAckExpected(val);
      break;

      // everything else
    default:
      DBG_Assert(0);
  }

}

void tREExecEngine::handle_RE_INSTR_SET(tThread aThread, const uint32_t args) {

  // NOTE: they should match xe.mdef argument fields
  unsigned dest = (args >> 4) & 0x03;
  unsigned type = (args >> 6) & 0x03;
  unsigned val  = (args >> 8) & 0x07;
  unsigned single_bit = (args >> 11) & 0x01;

  // get destination
  switch (dest) {

    case 0: // invalidation acks count
      switch (type) {
          // clear invalidation count
        case 0:
          DBG_Assert( single_bit == 0 );
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Clear InvalidationsCount"));
          aThread.setInvalidationCount(0);
          break;

          // decrement
        case 2:
          DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Decrement InvalidationsCount " << aThread.invalidationCount()));
          DBG_Assert(aThread.invalidationCount() > 0);
          aThread.setInvalidationCount(aThread.invalidationCount() - 1);
          break;

          // increment
        case 3:
          DBG_(Iface, ( << theEngineName << " [" << aThread << "]" << " Increment InvalidationsCount " << aThread.invalidationCount()));
          aThread.setInvalidationCount(aThread.invalidationCount() + 1);
          break;

          // everything else
        default:
          DBG_Assert(false);
      }
      break;

    case 1: // invalidation received flag
      DBG_Assert( type == 0 ); //set value
      switch (val) {
        case 0:
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set InvalidationReceived ReceivedNoInval"));
          break;
        case 1:
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set InvalidationReceived ReceivedLocalInval"));
          break;
        case 2:
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set InvalidationReceived ReceivedRemoteInval"));
          break;
        case 3:
          DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set InvalidationReceived ReceivedInvalHandled"));
          break;
        default:
          DBG_Assert(false);
      }
      aThread.setInvReceived(val);
      break;

      // invalidation ack receiver
    case 2: {
      DBG_Assert(type == 0);  // value

      node_id_t dest_node = 0;
      std::string dest_node_name;

      switch (val) {
        case 0: // ToRequester
          dest_node = aThread.requester();
          dest_node_name = "ToRequester";
          break;

        case 1: // ToOwner
          dest_node = aThread.owner();
          dest_node_name = "ToOwner";
          break;

        case 2: //ToDirectory
          dest_node = theSrvcProv.nodeForAddress(aThread.address());
          dest_node_name = "ToDirectory";
          break;

        case 3: // ToRespondent
          dest_node = aThread.respondent();
          dest_node_name = "ToRespondent";
          break;

        case 4: // ToRacer
          dest_node = aThread.racer();
          dest_node_name = "ToRacer";
          break;

        default:
          DBG_Assert(0);
      }

      DBG_(Iface, ( << theEngineName
                    << " [" << aThread << "]"
                    << " Set InvAckReceiver " << dest_node_name << " node " << dest_node));

      aThread.setInvAckReceiver(dest_node);
      break;
    }

    // is prefetch read
    case 3:
      DBG_Assert(type == 0);  // value
      DBG_(Iface, ( << theEngineName << " [" << aThread << "] Set IsPrefetchRead " << single_bit));
      aThread.setPrefetch(single_bit);
      break;

      // everything else
    default:
      DBG_Assert(false);
  }

}

unsigned getArithOperand(tThread aThread, unsigned operand) {
  switch (operand) {
    case 0: // InvalidationAckCount
      return aThread.invalidationCount();

    case 1: // MsgInvalidationsCount
      // get value of message inval count field
      return aThread.packet()->invalidationCount();

    case 2: // AnyInvalidations
      return aThread.anyInvalidations();

    case 3: // MsgAnyInvalidations
      // get value of message inval count field
      return aThread.packet()->anyInvalidations();

    default:
      DBG_Assert(false);
      return 0;
  }
}

void tREExecEngine::handle_RE_INSTR_ARITH_OP(tThread aThread, const uint32_t args) {
  DBG_(Iface, ( << theEngineName << " [" << aThread << "] Arithmetic operation"));

  // NOTE: they should match xe.mdef argument fields
  unsigned opcode  = (args & 0x030) >> 4;
  unsigned left  = (args & 0x0C0) >> 6;
  unsigned right = (args & 0x300) >> 8;
  unsigned dest  = (args & 0xC00) >> 10;

  unsigned left_val = getArithOperand(aThread, left);
  unsigned right_val = getArithOperand(aThread, right);
  unsigned result = 0;

  //
  // compute arith operation
  //
  switch (opcode) {
    case 0: // BITOR
      result = left_val | right_val;
      break;

    case 1: // ADD
      result = left_val + right_val;
      break;

    case 2: // SUB
      result = left_val - right_val;
      break;

    default:
      DBG_Assert(0);
  }

  //
  // pick destination
  //
  if (dest == 0) {
    // ToInvalidationAckCount
    aThread.setInvalidationCount(result);
  } else {
    // ToAnyInvalidations
    DBG_Assert(dest == 1);
    aThread.setAnyInvalidations(result);
  }
}

std::pair<tCpuOpType, std::string> determineRECPUOp( uint32_t args ) {
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
      DBG_Assert( false,  ( << "Invalid RE CPU_OP: " << args) );
      return std::make_pair(CPU_INVALIDATE, std::string("Invalidate"));
  }
}

void tREExecEngine::handle_RE_INSTR_CPU_OP(tThread aThread, const uint32_t args) {

  tCpuOpType operation;
  std::string operation_descr;
  std::tie( operation, operation_descr) = determineRECPUOp( args );

  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " Cpu " << operation_descr
                << (aThread.anyInvalidations() ? " SomeInvalidations" : " NoInvalidations")));

  theSrvcProv.CpuOp(operation, aThread.address(), aThread.anyInvalidations(), aThread.transactionTracker());

}

void tREExecEngine::handle_RE_INSTR_REFUSE(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " [" << aThread << "] Cpu InputQueueOp refuse message"));
  theThreadScheduler.refusePacket(aThread);
}

void
tREExecEngine::execute(tThread aThread,
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
  switch (op_code) {                              // decode instruction *****
    case RE_INSTR_SEND:            // send a packet
      handle_RE_INSTR_SEND(aThread, args);
      break;

    case RE_INSTR_SEND_RACER:            // send a packet
      handle_RE_INSTR_SEND_RACER(aThread, args);
      break;

    case RE_INSTR_RECEIVE:           // receive a packet
      handle_RE_INSTR_RECEIVE(aThread, args);
      break;

    case RE_INSTR_RECEIVE_DEFERRED_JUMP:  // receive a packet on a race with an evict
      handle_RE_INSTR_RECEIVE_DEFERRED_JUMP(aThread, args);
      break;

    case RE_INSTR_TEST:              // test things
      handle_RE_INSTR_TEST(aThread, args);
      break;

    case RE_INSTR_SET_RACER:               // set cache state or internal registers
      handle_RE_INSTR_SET_RACER(aThread, args);
      break;

    case RE_INSTR_SET:               // set cache state or internal registers
      handle_RE_INSTR_SET(aThread, args);
      break;

    case RE_INSTR_ARITH_OP:
      handle_RE_INSTR_ARITH_OP(aThread, args);
      break;

    case RE_INSTR_CPU_OP:            // operation on cpu
      handle_RE_INSTR_CPU_OP(aThread, args);
      break;

    case RE_INSTR_REFUSE:
      handle_RE_INSTR_REFUSE(aThread);
      break;

    default: {
      DBG_Assert(0);
    }
  }
}

void tREExecEngine::deliverReply(tThread aThread, tMessageType type) {
  uint32_t internal_type = map_MessageType_to_EngineMessageType(type, RE_INSTR_RECEIVE);

  DBG_(Iface, ( << theEngineName
                << " [" << aThread << "]"
                << " deliver_reply_to_microcode"
                << " type=" << type
                << " internal_type=" << internal_type));

  // take care of deferred jumps
  if (internal_type == RE_RX_DeferredJumpMessage) {

    unsigned deferred_jump_type = map_MessageType_to_EngineMessageType(type, RE_INSTR_RECEIVE_DEFERRED_JUMP);
    aThread.deferJump(deferred_jump_type);

    DBG_(Iface, ( << theEngineName
                  << " [" << aThread << "]"
                  << " take care of deferred jumps"
                  << " type=" << type
                  << " internal_type=" << internal_type
                  << " deferred_type=" << deferred_jump_type));

  }

  aThread.relativeJump(internal_type);
}

unsigned tREExecEngine::getEntryPoint(tMessageType type, tThread aThread, tDirState state) {
  unsigned pc = 0;

  switch (type) {
    case LocalInvalidationReq:
      pc = RE_EP_LocalInvalidationReq;
      RE_LocalInvalidationReq_cnt ++;
      break;

    case RemoteInvalidationReq:
      pc = RE_EP_RemoteInvalidationReq;
      RE_RemoteInvalidationReq_cnt ++;
      break;

    case ForwardedReadReq:
      pc = RE_EP_ForwardedReadReq;
      RE_ForwardedReadReq_cnt ++;
      break;

    case ForwardedWriteReq:
      pc = RE_EP_ForwardedWriteReq;
      RE_ForwardedWriteReq_cnt ++;
      break;

    case RecallReadReq:
      pc = RE_EP_RecallReadReq;
      RE_RecallReadReq_cnt ++;
      break;

    case RecallWriteReq:
      pc = RE_EP_RecallWriteReq;
      RE_RecallWriteReq_cnt ++;
      break;

    case ReadAck:
      pc = RE_EP_ReadAck;
      RE_ReadAck_cnt ++;
      break;

    case WriteAck:
      pc = RE_EP_WriteAck;
      RE_WriteAck_cnt ++;
      break;

    case UpgradeAck:
      pc = RE_EP_UpgradeAck;
      RE_UpgradeAck_cnt ++;
      break;

    case WritebackAck:
      pc = RE_EP_WritebackAck;
      RE_WritebackAck_cnt ++;
      break;

    case WritebackStaleRdAck:
      pc = RE_EP_WritebackStaleReadAck;
      RE_WritebackStaleReadAck_cnt ++;
      break;

    case WritebackStaleWrAck:
      pc = RE_EP_WritebackStaleWriteAck;
      RE_WritebackStaleWriteAck_cnt ++;
      break;

    case FlushAck:
      pc = RE_EP_FlushAck;
      RE_FlushAck_cnt ++;
      break;

    case RemoteInvalidationAck:
      pc = RE_EP_RemoteInvalidationAck;
      RE_RemoteInvalidationAck_cnt ++;
      break;

    case ReadFwd:
      pc = RE_EP_ReadFwd;
      RE_ReadFwd_cnt ++;
      break;

    case WriteFwd:
      pc = RE_EP_WriteFwd;
      RE_WriteFwd_cnt ++;
      break;

    case LocalRead:
      pc = RE_EP_Local_Read;
      RE_Local_Read_cnt ++;
      break;

    case LocalWriteAccess:
      pc = RE_EP_Local_WriteAccess;
      RE_Local_WriteAccess_cnt ++;
      break;

    case LocalUpgradeAccess:
      pc = RE_EP_Local_UpgradeAccess;
      RE_Local_UpgradeAccess_cnt ++;
      break;

    case LocalFlush:
      pc = RE_EP_Local_Flush;
      RE_Local_Flush_cnt ++;
      break;

    case LocalDropHint:
      pc = RE_EP_Local_DropHint;
      RE_Local_DropHint_cnt ++;
      break;

    case LocalEvict:
      pc = RE_EP_Local_Evict;
      RE_Local_Evict_cnt ++;
      break;

    case LocalPrefetchRead:
      pc = RE_EP_Local_Prefetch_Read;
      RE_Local_Prefetch_Read_cnt ++;
      break;

    default:
      DBG_Assert(0);
  }

  return pc;
}

unsigned tREExecEngine::map_MessageType_to_EngineMessageType(tMessageType type, const unsigned instr) const {
  switch (instr) {
    case RE_INSTR_RECEIVE: {
      switch (type) {

          // Requests From Cache to Directory
        case ReadReq:
          return RE_TX_ReadReq;                // VC 0
        case WriteReq:
          return RE_TX_WriteReq;               // VC 0
        case UpgradeReq:
          return RE_TX_UpgradeReq;             // VC 0
        case FlushReq:
          return RE_TX_FlushReq;               // VC 0 Contains data
        case WritebackReq:
          return RE_TX_WritebackReq;           // VC 1 Contains data
          // case ReadExReq:           // - Not used
          // case WriteAllReq:         // - Not used
          // case ReadAndForgetReq:    // - Not used

          // Requests From Directory to Cache
        case LocalInvalidationReq:
          return RE_RX_LocalInvalidationReq;   // VC 1
        case RemoteInvalidationReq:
          return RE_RX_RemoteInvalidationReq;  // VC 1
        case ForwardedReadReq:
          return RE_RX_DeferredJumpMessage;    // VC 1. Deferred Jump.
        case ForwardedWriteReq:
          return RE_RX_DeferredJumpMessage;    // VC 1. Deferred Jump.
        case RecallReadReq:
          return RE_RX_DeferredJumpMessage;    // VC 1. Deferred Jump.
        case RecallWriteReq:
          return RE_RX_DeferredJumpMessage;    // VC 1. Deferred Jump.

          // Replies From Cache to Directory
        case ForwardedWriteAck:
          return RE_TX_ForwardedWriteAck;      // VC 2
        case ForwardedReadAck:
          return RE_TX_ForwardedReadAck;       // VC 2 Contains data
        case RecallReadAck:
          return RE_TX_RecallReadAck;          // VC 2 Contains data
        case RecallWriteAck:
          return RE_TX_RecallWriteAck;         // VC 2 Contains data
        case LocalInvalidationAck:
          return RE_TX_LocalInvalidationAck;   // VC 2

          // Replies From Directory to Cache
        case ReadAck:
          return RE_RX_ReadAck;                // VC 1 Contains data
        case WriteAck:
          return RE_RX_WriteAck;               // VC 1 Contains data and #inval acks to expect
        case UpgradeAck:
          return RE_RX_UpgradeAck;             // VC 1 #inval acks to expect
        case WritebackAck:
          return RE_RX_WritebackAck;           // VC 2
        case WritebackStaleRdAck:
          return RE_RX_DeferredJumpMessage;    // VC 2. Deferred Jump.
        case WritebackStaleWrAck:
          return RE_RX_DeferredJumpMessage;    // VC 2. Deferred Jump.
        case FlushAck:
          return RE_RX_FlushAck;               // VC 1

          // Replies From Cache to Cache
        case RemoteInvalidationAck:
          return RE_XX_RemoteInvalidationAck;  // VC 2               NOTE: Can be sent or received. Make sure values match!!!
        case ReadFwd:
          return RE_XX_ReadFwd;                // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!
        case WriteFwd:
          return RE_XX_WriteFwd;               // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!

          // Replies from Caches to PE
        case InvAck:
          return RE_RX_InvAck;                // The cache acknowledges an Invalidate CPU_OP
        case InvUpdateAck:
          return RE_RX_InvUpdateAck;          // Contains data. The cache acknowledges an Invalidate CPU_OP.
        case DowngradeAck:
          return RE_RX_DowngradeAck;          // The cache acknowledges a Downgrade CPU_OP
        case DowngradeUpdateAck:
          return RE_RX_DowngradeUpdateAck;    // Contains data. The cache acknowledges a Downgrade CPU_OP.

          // Error in protocol
        case ProtocolError:
          return RE_TX_Error;                  // ERROR IN PROTOCOL!!!
        default:
          return RE_TX_Error;                  // ERROR IN PROTOCOL!!!

      }
    }

    case RE_INSTR_RECEIVE_DEFERRED_JUMP: {
      switch (type) {

          // Requests From Directory to Cache
        case ForwardedReadReq:
          return RE_RX_ForwardedReadReq;       // VC 1
        case ForwardedWriteReq:
          return RE_RX_ForwardedWriteReq;      // VC 1
        case RecallReadReq:
          return RE_RX_RecallReadReq;          // VC 1
        case RecallWriteReq:
          return RE_RX_RecallWriteReq;         // VC 1

          // Replies From Directory to Cache
        case WritebackStaleRdAck:
          return RE_RX_WritebackStaleReadAck;  // VC 2
        case WritebackStaleWrAck:
          return RE_RX_WritebackStaleWriteAck; // VC 2

          // Error in protocol
        case ProtocolError:
          return RE_TX_Error;                  // ERROR IN PROTOCOL!!!
        default:
          return RE_TX_Error;                  // ERROR IN PROTOCOL!!!

      }
    }
  }

  // must have found a match already!!!
  DBG_Assert(0, ( << "instr=" << instr
                  << " type=" << type ));

  return RE_TX_Error;
}

tMessageType tREExecEngine::map_EngineMessageType_to_MessageType(unsigned type, const unsigned instr) const {
  switch (type) {

      // Requests From Cache to Directory
    case RE_TX_ReadReq:
      return ReadReq;                // VC 0
    case RE_TX_WriteReq:
      return WriteReq;               // VC 0
    case RE_TX_UpgradeReq:
      return UpgradeReq;             // VC 0
    case RE_TX_FlushReq:
      return FlushReq;               // VC 0 Contains data
    case RE_TX_WritebackReq:
      return WritebackReq;           // VC 1 Contains data

      // Replies From Cache to Directory
    case RE_TX_ForwardedWriteAck:
      return ForwardedWriteAck;      // VC 2
    case RE_TX_ForwardedReadAck:
      return ForwardedReadAck;       // VC 2 Contains data
    case RE_TX_RecallReadAck:
      return RecallReadAck;          // VC 2 Contains data
    case RE_TX_RecallWriteAck:
      return RecallWriteAck;         // VC 2 Contains data
    case RE_TX_LocalInvalidationAck:
      return LocalInvalidationAck;   // VC 2

      // Replies From Cache to Cache
    case RE_XX_RemoteInvalidationAck:
      return RemoteInvalidationAck;  // VC 2               NOTE: Can be sent or received. Make sure values match!!!
    case RE_XX_ReadFwd:
      return ReadFwd;                // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!
    case RE_XX_WriteFwd:
      return WriteFwd;               // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!

      // Error in protocol
    case RE_TX_Error:
      return ProtocolError;          // ERROR IN PROTOCOL!!!

  }

  DBG_Assert(0);

  return ProtocolError;
}

}  // namespace nProtocolEngine
