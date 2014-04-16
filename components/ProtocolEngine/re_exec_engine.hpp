#ifndef _RE_EXEC_ENGINE_H_
#define _RE_EXEC_ENGINE_H_

#include <core/stats.hpp>

#include "exec_engine.hpp"
#include "protocol_engine.hpp"
#include "tSrvcProvider.hpp"
#include "input_q_cntl.hpp"
#include "ProtSharedTypes.hpp"

namespace nProtocolEngine {

namespace Stat = Flexus::Stat;

class tREExecEngine : public tMicrocodeEmulator {

private:
  //
  // Microcode definitions
  // NOTE: The values should match the opcodes in the respective xe.mdef file.
  // Long term goal: automatically set them by reading the mdef file.
  //
  static const unsigned RE_INSTR_SEND                  =  0;    // send a packet
  static const unsigned RE_INSTR_RECEIVE               =  1;    // receive a packet
  static const unsigned RE_INSTR_RECEIVE_DEFERRED_JUMP =  2;    // receive a 2-level decoded packet
  static const unsigned RE_INSTR_TEST                  =  3;    // test things
  static const unsigned RE_INSTR_SET                   =  4;    // set directory fields or internal registers
  static const unsigned RE_INSTR_ARITH_OP              =  5;    // arithmetic on protocol engine registers
  static const unsigned RE_INSTR_CPU_OP                =  6;    // operation on cpu

  static const unsigned RE_INSTR_REFUSE                =  8;    // refuse input message
  static const unsigned RE_INSTR_SEND_RACER            =  9;    // send a packet to a racer
  static const unsigned RE_INSTR_SET_RACER             = 10;    // set directory fields or internal registers

  //
  // Basic entry points for locally activated actions
  //
  static const unsigned RE_EP_Halt                = 0;  // microinstruction at address 0 should be a NOOP
  static const unsigned RE_EP_Local_Read          = 1;
  static const unsigned RE_EP_Local_WriteAccess   = 2;
  static const unsigned RE_EP_Local_UpgradeAccess = 3;
  static const unsigned RE_EP_Local_DropHint      = 4;
  static const unsigned RE_EP_Local_Evict         = 5;
  static const unsigned RE_EP_Local_Flush         = 6;
  static const unsigned RE_EP_Local_Prefetch_Read = 7;

  //
  // Entry points for remotely activated actions
  //
  static const unsigned RE_EP_LocalInvalidationReq   =  8;  // VC 1
  static const unsigned RE_EP_RemoteInvalidationReq  =  9;  // VC 1
  static const unsigned RE_EP_ForwardedReadReq       = 10;  // VC 1
  static const unsigned RE_EP_ForwardedWriteReq      = 11;  // VC 1
  static const unsigned RE_EP_RecallReadReq          = 12;  // VC 1
  static const unsigned RE_EP_RecallWriteReq         = 13;  // VC 1
  static const unsigned RE_EP_ReadAck                = 14;  // VC 1 Contains data
  static const unsigned RE_EP_WriteAck               = 15;  // VC 1 Contains data and #inval acks to expect
  static const unsigned RE_EP_UpgradeAck             = 16;  // VC 1 #inval acks to expect
  static const unsigned RE_EP_WritebackAck           = 17;  // VC 2
  static const unsigned RE_EP_WritebackStaleReadAck  = 18;  // VC 2
  static const unsigned RE_EP_WritebackStaleWriteAck = 19;  // VC 2
  static const unsigned RE_EP_FlushAck               = 20;  // VC 1
  static const unsigned RE_EP_RemoteInvalidationAck  = 21;  // VC 2
  static const unsigned RE_EP_ReadFwd                = 22;  // VC 2 Contains Data
  static const unsigned RE_EP_WriteFwd               = 23;  // VC 2 Contains Data

  static const unsigned RE_EP_Error                  = 31;  // ERROR IN PROTOCOL!!!

  //
  // Outgoing messages
  //
  static const unsigned RE_TX_ReadReq                =  0;      // VC 0
  static const unsigned RE_TX_WriteReq               =  1;      // VC 0
  static const unsigned RE_TX_UpgradeReq             =  2;      // VC 0
  static const unsigned RE_TX_FlushReq               =  3;      // VC 0 Contains data
  static const unsigned RE_XX_RemoteInvalidationAck  =  4;      // VC 2               NOTE: Can be sent or received. Make sure values match!!!
  static const unsigned RE_XX_ReadFwd                =  5;      // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!
  static const unsigned RE_XX_WriteFwd               =  6;      // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!
  static const unsigned RE_TX_WritebackReq           =  7;      // VC 1 Contains data
  static const unsigned RE_TX_ForwardedWriteAck      =  8;      // VC 2
  static const unsigned RE_TX_ForwardedReadAck       =  9;      // VC 2 Contains data
  static const unsigned RE_TX_RecallReadAck          = 10;      // VC 2 Contains data
  static const unsigned RE_TX_RecallWriteAck         = 11;      // VC 2 Contains data
  static const unsigned RE_TX_LocalInvalidationAck   = 12;      // VC 2

  static const unsigned RE_TX_Error                  = 15;      // ERROR IN PROTOCOL!!!

  //
  // Incoming responses
  //
  // RECEIVE (Network)
  static const unsigned RE_RX_ReadAck                  =  0;   // VC 1 Contains data
  static const unsigned RE_RX_WriteAck                 =  1;   // VC 1 Contains data and #inval acks to expect
  static const unsigned RE_RX_UpgradeAck               =  2;   // VC 1 #inval acks to expect
  static const unsigned RE_RX_WritebackAck             =  3;   // VC 2
  // static const unsigned RE_XX_RemoteInvalidationAck =  4;   // VC 2               NOTE: Can be sent or received. Make sure values match!!!
  // static const unsigned RE_XX_ReadFwd               =  5;   // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!
  // static const unsigned RE_XX_WriteFwd              =  6;   // VC 2 Contains Data NOTE: Can be sent or received. Make sure values match!!!
  static const unsigned RE_RX_FlushAck                 =  7;   // VC 1
  static const unsigned RE_RX_LocalInvalidationReq     =  8;   // VC 1
  static const unsigned RE_RX_RemoteInvalidationReq    =  9;   // VC 1

  // RECEIVE to RECEIVE_DEFERRED_JUMP marker
  static const unsigned RE_RX_DeferredJumpMessage      = 10;   // Dummy message to indicate a message is received that needs to be decoded
  // by a 2-level receive instruction (RECEIVE_DEFERRED_JUMP). This is done
  // because a normal HW decoder can be at most 16-way. For anything beyond
  // that it is better to assume a 2-level dcoding scheme. Only rarely received
  // messages end up in that category (currently it includes uncommon races)

  // RECEIVE (Caches)
  static const unsigned RE_RX_InvAck                   = 11;   // The cache acknowledges an Invalidate CPU_OP
  static const unsigned RE_RX_InvUpdateAck             = 12;   // Contains data. The cache acknowledges an Invalidate CPU_OP.
  static const unsigned RE_RX_DowngradeAck             = 13;   // The cache acknowledges a Downgrade CPU_OP
  static const unsigned RE_RX_DowngradeUpdateAck       = 14;   // Contains data. The cache acknowledges a Downgrade CPU_OP.

  // RECEIVE_DEFERRED_JUMP (Network)
  static const unsigned RE_RX_WritebackStaleReadAck    =  0;   // VC 2
  static const unsigned RE_RX_WritebackStaleWriteAck   =  1;   // VC 2
  static const unsigned RE_RX_ForwardedReadReq         =  2;   // VC 1
  static const unsigned RE_RX_ForwardedWriteReq        =  3;   // VC 1
  static const unsigned RE_RX_RecallReadReq            =  4;   // VC 1
  static const unsigned RE_RX_RecallWriteReq           =  5;   // VC 1

  static const unsigned RE_RX_Error                    = 15;   // Error message

private:
  //
  // protocol stats for locally activated actions
  //
  Stat::StatCounter RE_Halt_cnt;
  Stat::StatCounter RE_Local_Read_cnt;
  Stat::StatCounter RE_Local_WriteAccess_cnt;
  Stat::StatCounter RE_Local_UpgradeAccess_cnt;
  Stat::StatCounter RE_Local_DropHint_cnt;
  Stat::StatCounter RE_Local_Evict_cnt;
  Stat::StatCounter RE_Local_Flush_cnt;
  Stat::StatCounter RE_Local_Prefetch_Read_cnt;

  //
  // protocol stats for remotely activated actions
  //
  Stat::StatCounter RE_LocalInvalidationReq_cnt;
  Stat::StatCounter RE_RemoteInvalidationReq_cnt;
  Stat::StatCounter RE_ForwardedReadReq_cnt;
  Stat::StatCounter RE_ForwardedWriteReq_cnt;
  Stat::StatCounter RE_RecallReadReq_cnt;
  Stat::StatCounter RE_RecallWriteReq_cnt;
  Stat::StatCounter RE_ReadAck_cnt;
  Stat::StatCounter RE_WriteAck_cnt;
  Stat::StatCounter RE_UpgradeAck_cnt;
  Stat::StatCounter RE_WritebackAck_cnt;
  Stat::StatCounter RE_WritebackStaleReadAck_cnt;
  Stat::StatCounter RE_WritebackStaleWriteAck_cnt;
  Stat::StatCounter RE_FlushAck_cnt;
  Stat::StatCounter RE_RemoteInvalidationAck_cnt;
  Stat::StatCounter RE_ReadFwd_cnt;
  Stat::StatCounter RE_WriteFwd_cnt;

public:

  // constructor
  tREExecEngine(std::string const & aName,
                tSrvcProvider & aSrvcProv,
                tInputQueueController & anInQCntl,
                tThreadScheduler & aThreadScheduler)
    : tMicrocodeEmulator(aName, "re.rom", "re.mdef", "re.cnt", 6477, aSrvcProv, anInQCntl, aThreadScheduler)
    , RE_Halt_cnt(theEngineName + "-Halts")
    , RE_Local_Read_cnt(theEngineName + "-Local-Reads")
    , RE_Local_WriteAccess_cnt(theEngineName + "-Local-WriteAccesses")
    , RE_Local_UpgradeAccess_cnt(theEngineName + "-Local-UpgradeAccesses")
    , RE_Local_DropHint_cnt(theEngineName + "-Local-DropHints")
    , RE_Local_Evict_cnt(theEngineName + "-Local-Evicts")
    , RE_Local_Flush_cnt (theEngineName + "-Local-Flushes")
    , RE_Local_Prefetch_Read_cnt(theEngineName + "-Local-PrefetchReads")

    , RE_LocalInvalidationReq_cnt(theEngineName + "-Local-InvReq")
    , RE_RemoteInvalidationReq_cnt(theEngineName + "-Remote-InvReq")
    , RE_ForwardedReadReq_cnt(theEngineName + "-FwdReadReq")
    , RE_ForwardedWriteReq_cnt(theEngineName + "-FwdWriteReq")
    , RE_RecallReadReq_cnt(theEngineName + "-RecallReadReq")
    , RE_RecallWriteReq_cnt(theEngineName + "-RecallWriteReq")
    , RE_ReadAck_cnt(theEngineName + "-ReadAcks")
    , RE_WriteAck_cnt(theEngineName + "-WriteAcks")
    , RE_UpgradeAck_cnt(theEngineName + "-UpgradeAcks")
    , RE_WritebackAck_cnt(theEngineName + "-WritebackAcks")
    , RE_WritebackStaleReadAck_cnt(theEngineName + "-WBStaleReadAcks")
    , RE_WritebackStaleWriteAck_cnt(theEngineName + "-WBStaleWriteAcks")
    , RE_FlushAck_cnt(theEngineName + "-FlushAcks")
    , RE_RemoteInvalidationAck_cnt(theEngineName + "-RemoteInvAcks")
    , RE_ReadFwd_cnt(theEngineName + "-ReadForwards")
    , RE_WriteFwd_cnt(theEngineName + "-WriteForwards")
  { }

  // initialization
  void init(void)      {
    tMicrocodeEmulator::init();
    return;
  }

  // statistics
  void dump_mcd_stats(void) const     {
    tMicrocodeEmulator::dump_mcd_stats();  // printout stats
    return;
  }
  void dump_mcd_counters(void) const  {
    tMicrocodeEmulator::dump_mcd_counters();  // dump counters to xe.cnt file
    return;
  }

  ///////////////////////////////////////////////////////////
  //
  // methods to calculate the microcode pc
  // NOTE: some are engine specific
  //

  // deliver response packet to RECEIVE instruction
  void deliverReply(tThread thread, tMessageType type);

  // calculate the pc for the entry point of local and remote requests
  // this is engine specific
  unsigned getEntryPoint(tMessageType type, tThread aThread, tDirState state);

  ///////////////////////////////////////////////////////////
  //
  // main execution loop routines
  //
protected:
  // Execute state: work on it
  // actual execution of instructions (engine dependent)
  void execute(tThread aThread,
               const unsigned      op_code,
               const uint32_t args,
               const uint32_t ms_pc);
private:
  void handle_RE_INSTR_SEND(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_SEND_RACER(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_RECEIVE(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_RECEIVE_DEFERRED_JUMP(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_TEST(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_SET_RACER(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_SET(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_ARITH_OP(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_CPU_OP(tThread aThread, const uint32_t args) ;
  void handle_RE_INSTR_REFUSE(tThread aThread);

  unsigned map_MessageType_to_EngineMessageType(tMessageType type, const unsigned instr) const;
  tMessageType map_EngineMessageType_to_MessageType(unsigned engine_type, const unsigned instr) const;

};

}  // namespace nProtocolEngine

#endif // _RE_EXEC_ENGINE_H_
