#ifndef _HE_EXEC_ENGINE_H_
#define _HE_EXEC_ENGINE_H_

#include <core/stats.hpp>

#include "exec_engine.hpp"
#include "protocol_engine.hpp"
#include "tSrvcProvider.hpp"
#include "input_q_cntl.hpp"
#include "ProtSharedTypes.hpp"

namespace nProtocolEngine {

namespace Stat = Flexus::Stat;

class tHEExecEngine : public tMicrocodeEmulator {

private:
  //
  // Microcode definitions
  // NOTE: The values should match the opcodes in the respective xe.mdef file.
  // Long term goal: automatically set them by reading the mdef file.
  //
  static const unsigned HE_INSTR_SEND     = 0;    // send a reply packet (requester = destination node)
  static const unsigned HE_INSTR_SEND_FWD = 1;    // send a fwd packet (requester = tsrf.requester)
  static const unsigned HE_INSTR_RECEIVE  = 2;    // receive a packet
  static const unsigned HE_INSTR_TEST     = 3;    // test things
  static const unsigned HE_INSTR_SET      = 4;    // set directory fields or internal registers
  static const unsigned HE_INSTR_UNLOCK   = 5;    // unlock directory entry
  static const unsigned HE_INSTR_CPU_OP   = 6;    // operation on cpu
  static const unsigned HE_INSTR_WRITE_DIRECTORY = 7; // write to directory
  static const unsigned HE_INPUT_Q_OP     = 8;    // operation on input message queue

  //
  // Basic entry points for locally activated actions
  //
  static const unsigned HE_EP_Halt                       = 0;  // microinstruction at address 0 should be a NOOP
  static const unsigned HE_EP_Local_Read                 = 1;  // Read miss by local processor, dir state is modified
  static const unsigned HE_EP_Local_WriteAccess_Shared   = 2;  // Write miss by local processor, dir state is shared
  static const unsigned HE_EP_Local_WriteAccess_Modified = 3;  // Write miss by local processor, dir state is modified
  static const unsigned HE_EP_Local_UpgradeAccess        = 4;  // Upgrade miss by local processor, dir state is shared
  static const unsigned HE_EP_Local_Prefetch_Read        = 5;  // Prefetch read miss by local processor, dir state is modified

  //
  // Entry points for remotely activated actions
  //
  static const unsigned HE_EP_ReadReq_Invalid     = 16;    // VC 0
  static const unsigned HE_EP_ReadReq_Shared      = 17;    // VC 0
  static const unsigned HE_EP_ReadReq_Modified    = 18;    // VC 0
  static const unsigned HE_EP_WriteReq_Invalid    = 19;    // VC 0
  static const unsigned HE_EP_WriteReq_Shared     = 20;    // VC 0
  static const unsigned HE_EP_WriteReq_Modified   = 21;    // VC 0
  static const unsigned HE_EP_UpgradeReq_Invalid  = 22;    // VC 0
  static const unsigned HE_EP_UpgradeReq_Shared   = 23;    // VC 0
  static const unsigned HE_EP_UpgradeReq_Modified = 24;    // VC 0
  static const unsigned HE_EP_FlushReq            = 25;    // VC 0   Contains data
  static const unsigned HE_EP_WritebackReq        = 26;    // VC 1   Contains data

  static const unsigned HE_EP_Error               = 31;    // ERROR IN PROTOCOL!!!

  //
  // Outgoing messages
  //
  // SEND
  static const unsigned HE_TX_ReadAck                = 0;      // VC 1    Contains data
  static const unsigned HE_TX_WriteAck               = 1;      // VC 1    Contains data and #inval acks to expect
  static const unsigned HE_TX_UpgradeAck             = 2;      // VC 1    Contains #inval acks to expect
  static const unsigned HE_TX_WritebackAck           = 3;      // VC 2
  static const unsigned HE_TX_FlushAck               = 4;      // VC 1

  static const unsigned HE_TX_Error                  = 7;      // ERROR IN PROTOCOL!!!

  // SEND_FWD
  static const unsigned HE_TX_LocalInvalidationReq   = 0;      // VC 1
  static const unsigned HE_TX_RemoteInvalidationReq  = 1;      // VC 1
  static const unsigned HE_TX_ForwardedReadReq       = 2;      // VC 1
  static const unsigned HE_TX_ForwardedWriteReq      = 3;      // VC 1
  static const unsigned HE_TX_RecallReadReq          = 4;      // VC 1
  static const unsigned HE_TX_RecallWriteReq         = 5;      // VC 1
  static const unsigned HE_TX_WritebackStaleRdAck    = 6;      // VC 2
  static const unsigned HE_TX_WritebackStaleWrAck    = 7;      // VC 2

  static const unsigned HE_TX_FWD_Error              = 15;     // ERROR IN PROTOCOL!!!

  //
  // Incoming responses
  //
  // Network
  static const unsigned HE_RX_WritebackReq           = 0;      // VC 1   Contains data
  static const unsigned HE_RX_ForwardedWriteAck      = 1;      // VC 2
  static const unsigned HE_RX_ForwardedReadAck       = 2;      // VC 2   Contains data
  static const unsigned HE_RX_RecallReadAck          = 3;      // VC 2   Contains data
  static const unsigned HE_RX_RecallWriteAck         = 4;      // VC 2   Contains data
  static const unsigned HE_RX_LocalInvalidationAck   = 5;      // VC 2
  static const unsigned HE_RX_FlushReq               = 6;      // VC 1   Contains data

  // Caches
  /*
    static const unsigned HE_RX_InvAck                 = 11;     // The cache acknowledges an Invalidate CPU_OP
    static const unsigned HE_RX_InvUpdateAck           = 12;     // Contains data. The cache acknowledges an Invalidate CPU_OP.
    static const unsigned HE_RX_DowngradeAck           = 13;     // The cache acknowledges a Downgrade CPU_OP
    static const unsigned HE_RX_DowngradeUpdateAck     = 14;     // Contains data. The cache acknowledges a Downgrade CPU_OP.
  */
  static const unsigned HE_RX_InvAck                 = 7;      // The cache acknowledges an Invalidate CPU_OP
  static const unsigned HE_RX_InvUpdateAck           = 8;      // Contains data. The cache acknowledges an Invalidate CPU_OP.
  static const unsigned HE_RX_DowngradeAck           = 9;      // The cache acknowledges a Downgrade CPU_OP
  static const unsigned HE_RX_DowngradeUpdateAck     = 10;     // Contains data. The cache acknowledges a Downgrade CPU_OP.

  static const unsigned HE_RX_Error                  = 15;     // Error message.

private:
  //
  // stats for locally activated actions
  //
  Stat::StatCounter HE_Local_Read_cnt;                 // Read miss by local processor, dir state is modified
  Stat::StatCounter HE_Local_WriteAccess_Shared_cnt;   // Write miss by local processor, dir state is shared
  Stat::StatCounter HE_Local_WriteAccess_Modified_cnt; // Write miss by local processor, dir state is modified
  Stat::StatCounter HE_Local_UpgradeAccess_cnt;        // Upgrade miss by local processor, dir state is shared
  Stat::StatCounter HE_Local_Prefetch_Read_cnt;        // Prefetch read miss by local processor, dir state is modified

  //
  // stats for remotely activated actions
  //
  Stat::StatCounter HE_ReadReq_Invalid_cnt;
  Stat::StatCounter HE_ReadReq_Shared_cnt;
  Stat::StatCounter HE_ReadReq_Modified_cnt;
  Stat::StatCounter HE_WriteReq_Invalid_cnt;
  Stat::StatCounter HE_WriteReq_Shared_cnt;
  Stat::StatCounter HE_WriteReq_Modified_cnt;
  Stat::StatCounter HE_UpgradeReq_Invalid_cnt;
  Stat::StatCounter HE_UpgradeReq_Shared_cnt;
  Stat::StatCounter HE_UpgradeReq_Modified_cnt;
  Stat::StatCounter HE_FlushReq_cnt;
  Stat::StatCounter HE_WritebackReq_cnt;

  //////////////// timing stats /////////////////

public:

  // constructor
  tHEExecEngine(std::string const & aName, tSrvcProvider & aSrvcProv, tInputQueueController & anInQCntl, tThreadScheduler & aThreadScheduler)
    : tMicrocodeEmulator(aName, "he.rom", "he.mdef", "he.cnt", 6476, aSrvcProv, anInQCntl, aThreadScheduler)

    , HE_Local_Read_cnt(theEngineName + "-Local-Reads")
    , HE_Local_WriteAccess_Shared_cnt(theEngineName + "-Local-Shared-WriteAccesses")
    , HE_Local_WriteAccess_Modified_cnt(theEngineName + "-Local-Modified-WriteAccesses")
    , HE_Local_UpgradeAccess_cnt(theEngineName + "-Local-UpgradeAccesses")
    , HE_Local_Prefetch_Read_cnt(theEngineName + "-Local-PrefetchReads")

    , HE_ReadReq_Invalid_cnt(theEngineName + "-Invalid-ReadReq")
    , HE_ReadReq_Shared_cnt(theEngineName + "-Shared-ReadReq")
    , HE_ReadReq_Modified_cnt(theEngineName + "-Modified-ReadReq")
    , HE_WriteReq_Invalid_cnt(theEngineName + "-Invalid-WriteReq")
    , HE_WriteReq_Shared_cnt(theEngineName + "-Shared-WriteReq")
    , HE_WriteReq_Modified_cnt(theEngineName + "-Modified-WriteReq")
    , HE_UpgradeReq_Invalid_cnt(theEngineName + "-Invalid-UpgradeReq")
    , HE_UpgradeReq_Shared_cnt(theEngineName + "-Shared-UpgradeReq")
    , HE_UpgradeReq_Modified_cnt(theEngineName + "-Modified-UpgradeReq")
    , HE_FlushReq_cnt(theEngineName + "-FlushReq")
    , HE_WritebackReq_cnt(theEngineName + "-WritebackReq")

  { }

  // initialization
  void init(void)      {
    tMicrocodeEmulator::init();
  }

  // statistics
  void dump_mcd_stats(void) const     {
    tMicrocodeEmulator::dump_mcd_stats();  // print mc stats
  }
  void dump_mcd_counters(void) const  {
    tMicrocodeEmulator::dump_mcd_counters();  // dump counters to xe.cnt file
  }

  ///////////////////////////////////////////////////////////
  //
  // methods to calculate the microcode pc
  // NOTE: some are engine specific
  //

  // deliver response packet to RECEIVE instruction
  void deliverReply(tThread thread, tMessageType type);

  // calculate the pc for the entry point of remote and local requests
  // this is engine specific
  unsigned getEntryPoint(tMessageType type, tThread aThread, tDirState state);

  ///////////////////////////////////////////////////////////
  //
  // main execution loop routines
  //
private:
  // Execute state: work on it
  // actual execution of instructions (engine dependent)
  void execute(tThread aThread,
               const unsigned      op_code,
               const uint32_t args,
               const uint32_t ms_pc);

  void handle_HE_INSTR_SEND(tThread aThread, const uint32_t args);
  void handle_HE_INSTR_SEND_FWD(tThread aThread, const uint32_t args);
  void handle_HE_INSTR_RECEIVE(tThread aThread, const uint32_t args);
  void handle_HE_INSTR_TEST(tThread aThread, const uint32_t args);
  void handle_HE_INSTR_SET(tThread aThread, const uint32_t args);
  void handle_HE_INSTR_UNLOCK(tThread aThread);
  void handle_HE_INSTR_CPU_OP(tThread aThread, const uint32_t args);
  void handle_HE_INSTR_WRITE_DIRECTORY(tThread aThread);
  void handle_HE_INPUT_Q_OP(tThread aThread, const uint32_t args);

  unsigned map_MessageType_to_EngineMessageType(tMessageType type, const unsigned instr) const;
  tMessageType map_EngineMessageType_to_MessageType(unsigned engine_type, const unsigned instr) const;

  void doPredictorNotify(tThread aThread); /* CMU-ONLY */
  void setTransactionType(tMessageType aType, tThread aThread, tDirState aState);
  unsigned extractSourceValue(tThread aThread, unsigned aSourceReg, unsigned aTransferType);

};

}  // namespace nProtocolEngine

#endif // _HE_EXEC_ENGINE_H_
