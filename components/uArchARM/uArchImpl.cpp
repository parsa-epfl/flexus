// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#include <components/uArchARM/uArchARM.hpp>

#define FLEXUS_BEGIN_COMPONENT uArchARM
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/MTManager/MTManager.hpp>

#include "uArchInterfaces.hpp"

#include <core/debug/debug.hpp>
#include "microArch.hpp"

#include <core/qemu/mai_api.hpp>

#define DBG_DefineCategories uArchCat, Special
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArchARM {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

class uArch_QemuObject_Impl  {
  std::shared_ptr<microArch> theMicroArch;
public:
  uArch_QemuObject_Impl(Flexus::Qemu::API::conf_object_t * /*ignored*/ ) {}

  void setMicroArch(std::shared_ptr<microArch> aMicroArch) {
    theMicroArch = aMicroArch;
  }

  void printROB() {
    DBG_Assert(theMicroArch);
    theMicroArch->printROB();
  }
  void printMemQueue() {
    DBG_Assert(theMicroArch);
    theMicroArch->printMemQueue();
  }
  void printSRB() {
    DBG_Assert(theMicroArch);
    theMicroArch->printSRB();
  }
  void printMSHR() {
    DBG_Assert(theMicroArch);
    theMicroArch->printMSHR();
  }
  void pregs() {
    DBG_Assert(theMicroArch);
    theMicroArch->pregs();
  }
  void pregsAll() {
    DBG_Assert(theMicroArch);
    theMicroArch->pregsAll();
  }
  void resynchronize() {
    DBG_Assert(theMicroArch);
    theMicroArch->resynchronize();
  }
  void printRegMappings(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printRegMappings(aRegSet);
  }
  void printRegFreeList(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printRegFreeList(aRegSet);
  }
  void printRegReverseMappings(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printRegReverseMappings(aRegSet);
  }
  void printRegAssignments(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printAssignments(aRegSet);
  }

};

class uArch_QemuObject : public Qemu::AddInObject <uArch_QemuObject_Impl> {

  typedef Qemu::AddInObject<uArch_QemuObject_Impl> base;
public:
  static const Qemu::Persistence  class_persistence = Qemu::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "uArchARM";
  }
  static std::string classDescription() {
    return "uArchARM object";
  }

  uArch_QemuObject() : base() { }
  uArch_QemuObject(Qemu::API::conf_object_t * aQemuObject) : base(aQemuObject) {}
  uArch_QemuObject(uArch_QemuObject_Impl * anImpl) : base(anImpl) {}
};

Qemu::Factory<uArch_QemuObject> theuArchQemuFactory;

class FLEXUS_COMPONENT(uArchARM) {
  FLEXUS_COMPONENT_IMPL(uArchARM);

  std::shared_ptr<microArch> theMicroArch;
  uArch_QemuObject theuArchObject;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(uArchARM)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return !theMicroArch || theMicroArch->isQuiesced();
  }

  void initialize() {
    uArchOptions_t
    options;

    options.ROBSize              = cfg.ROBSize;
    options.SBSize               = cfg.SBSize;
    options.NAWBypassSB          = cfg.NAWBypassSB;
    options.NAWWaitAtSync        = cfg.NAWWaitAtSync;
    options.retireWidth          = cfg.RetireWidth;
    options.numMemoryPorts       = cfg.MemoryPorts;
    options.numSnoopPorts        = cfg.SnoopPorts;
    options.numStorePrefetches   = cfg.StorePrefetches;
    options.prefetchEarly        = cfg.PrefetchEarly;
    options.spinControlEnabled   = cfg.SpinControl;
    options.consistencyModel     = (nuArchARM::eConsistencyModel)cfg.ConsistencyModel;
    options.coherenceUnit        = cfg.CoherenceUnit;
    options.breakOnResynchronize = cfg.BreakOnResynchronize;
    options.validateMMU          = cfg.ValidateMMU;
    options.speculativeOrder     = cfg.SpeculativeOrder;
    options.speculateOnAtomicValue   = cfg.SpeculateOnAtomicValue;
    options.speculateOnAtomicValuePerfect   = cfg.SpeculateOnAtomicValuePerfect;
    options.speculativeCheckpoints = cfg.SpeculativeCheckpoints;
    options.checkpointThreshold   = cfg.CheckpointThreshold;
    options.earlySGP             = cfg.EarlySGP;    /* CMU-ONLY */
    options.trackParallelAccesses = cfg.TrackParallelAccesses; /* CMU-ONLY */
    options.inOrderMemory        = cfg.InOrderMemory;
    options.inOrderExecute       = cfg.InOrderExecute;
    options.onChipLatency        = cfg.OnChipLatency;
    options.offChipLatency       = cfg.OffChipLatency;
    options.name                 = statName();
    options.node                 = flexusIndex();

    options.numIntAlu = cfg.NumIntAlu;
    options.intAluOpLatency = cfg.IntAluOpLatency;
    options.intAluOpPipelineResetTime = cfg.IntAluOpPipelineResetTime;

    options.numIntMult = cfg.NumIntMult;
    options.intMultOpLatency = cfg.IntMultOpLatency;
    options.intMultOpPipelineResetTime = cfg.IntMultOpPipelineResetTime;
    options.intDivOpLatency = cfg.IntDivOpLatency;
    options.intDivOpPipelineResetTime = cfg.IntDivOpPipelineResetTime;

    options.numFpAlu = cfg.NumFpAlu;
    options.fpAddOpLatency = cfg.FpAddOpLatency;
    options.fpAddOpPipelineResetTime = cfg.FpAddOpPipelineResetTime;
    options.fpCmpOpLatency = cfg.FpCmpOpLatency;
    options.fpCmpOpPipelineResetTime = cfg.FpCmpOpPipelineResetTime;
    options.fpCvtOpLatency = cfg.FpCvtOpLatency;
    options.fpCvtOpPipelineResetTime = cfg.FpCvtOpPipelineResetTime;

    options.numFpMult = cfg.NumFpMult;
    options.fpMultOpLatency = cfg.FpMultOpLatency;
    options.fpMultOpPipelineResetTime = cfg.FpMultOpPipelineResetTime;
    options.fpDivOpLatency = cfg.FpDivOpLatency;
    options.fpDivOpPipelineResetTime = cfg.FpDivOpPipelineResetTime;
    options.fpSqrtOpLatency = cfg.FpSqrtOpLatency;
    options.fpSqrtOpPipelineResetTime = cfg.FpSqrtOpPipelineResetTime;

    theMicroArch = microArch::construct ( options
                                            , ll::bind( &uArchARMComponent::squash, this, ll::_1)
                                            , ll::bind( &uArchARMComponent::redirect, this, ll::_1)
                                            , ll::bind( &uArchARMComponent::changeState, this, ll::_1, ll::_2)
                                            , ll::bind( &uArchARMComponent::feedback, this, ll::_1)
                                            , ll::bind( &uArchARMComponent::signalStoreForwardingHit, this, ll::_1)
                                        );

    theuArchObject = theuArchQemuFactory.create( (std::string("uarcharm-") + boost::padded_string_cast < 2, '0' > (flexusIndex())).c_str() );
    theuArchObject->setMicroArch(theMicroArch);

  }

  void finalize() {}

public:
  FLEXUS_PORT_ALWAYS_AVAILABLE(DispatchIn);
  void push( interface::DispatchIn const &, boost::intrusive_ptr< AbstractInstruction > & anInstruction ) {
    DBG_(Tmp, (<<"Get the inst in uArchARM: "));
    theMicroArch->dispatch(anInstruction);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableDispatchOut);
  std::pair<int, bool> pull(AvailableDispatchOut const &) {
    return std::make_pair( theMicroArch->availableROB(), theMicroArch->isSynchronized() );
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &) {
    return theMicroArch->isStalled();
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &) {
    return theMicroArch->iCount();
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryIn);
  void push( interface::MemoryIn const &, MemoryTransport & aTransport) {
    handleMemoryMessage(aTransport);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE( WritePermissionLost );
  void push( interface::WritePermissionLost const &, PhysicalMemoryAddress & anAddress) {
    theMicroArch->writePermissionLost(anAddress);
  }

  // Msutherl
  FLEXUS_PORT_ALWAYS_AVAILABLE(AddressesToTranslate);
  void push( interface::AddressesToTranslate const &,
             TranslatedAddresses& translateUs ) {
      /* TODO: add requests to the cache controller to actually place the TTE descr.
       * requests into the flexus memory hierarchy.
       * - for now, just does the translation walk in theMicroArch 
       */
      // commence
      for( auto& translation : translateUs->internalContainer ) {
          uint8_t flexusCurrentELRegime = 1; // FIXME: should return tr. regime for addr
          if ( theMicroArch->IsTranslationEnabledAtCurrentEL( flexusCurrentELRegime ) ) {
              DBG_(Tmp,(<<" ---- STARTING NEW TRANSLATION ---- "
                          << std::hex << translation.theVaddr << std::dec ));
              translation.ELRegime = flexusCurrentELRegime;
              theMicroArch->translate( translation, false );
          } else {
              DBG_Assert( false, ( << "SORRY, translation is not enabled at EL " << flexusCurrentELRegime ) );
          }
      }

      // fin
      FLEXUS_CHANNEL(TranslationsToReturn) << translateUs;
  }

public:
  //The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
  //passing each instruction to its FetchOut port.
  void drive(interface::uArchDrive const &) {
    doCycle();
  }

private:
  struct ResynchronizeWithQemuException {};

  void squash(eSquashCause aSquashReason) {
    FLEXUS_CHANNEL( SquashOut ) << aSquashReason;
  }

  void changeState(int32_t aTL, int32_t aPSTATE) {
    CPUState state;
    state.theTL = aTL;
    state.thePSTATE = aPSTATE;
    FLEXUS_CHANNEL( ChangeCPUState ) << state;
  }

  void redirect(VirtualMemoryAddress aPC) {
    VirtualMemoryAddress redirect_addr = aPC;
    FLEXUS_CHANNEL( RedirectOut ) << redirect_addr;
  }

  void feedback(boost::intrusive_ptr<BranchFeedback> aFeedback) {
    FLEXUS_CHANNEL( BranchFeedbackOut ) << aFeedback;
  }

  void signalStoreForwardingHit(bool garbage) {
    bool value = true;
    FLEXUS_CHANNEL( StoreForwardingHitSeen ) << value;
  }

  void doCycle() {
    if (cfg.Multithread) {
      if (nMTManager::MTManager::get()->runThisEX(flexusIndex())) {
        theMicroArch->cycle();
      } else {
        theMicroArch->skipCycle();
      }
    } else {
      theMicroArch->cycle();
    }
    sendMemoryMessages();
  }

  void sendMemoryMessages() {
    while (FLEXUS_CHANNEL( MemoryOut_Request ).available()) {
      boost::intrusive_ptr< MemOp > op(theMicroArch->popMemOp());
      if (! op ) break;

      //DBG_Assert( Tmp, ( << "Send Request: " << *op) );

      MemoryTransport transport;
      boost::intrusive_ptr<MemoryMessage> operation;

      if (op->theNAW) {
        DBG_Assert( op->theOperation == kStore );
        operation = new MemoryMessage(MemoryMessage::NonAllocatingStoreReq, op->thePAddr, op->thePC);
      } else {

        switch ( op->theOperation ) {
          case kLoad:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newLoad(op->thePAddr, op->thePC);
            break;

          case kAtomicPreload:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newAtomicPreload(op->thePAddr, op->thePC);
            break;

          case kStorePrefetch:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newStorePrefetch(op->thePAddr, op->thePC, op->theValue);
            break;

          case kStore:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newStore(op->thePAddr, op->thePC, op->theValue);
            break;

          case kRMW:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newRMW(op->thePAddr, op->thePC, op->theValue);
            break;

          case kCAS:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newCAS(op->thePAddr, op->thePC, op->theValue);
            break;

          default:
            DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
        }

      }
      operation->theInstruction = op->theInstruction;

      operation->reqSize() = op->theSize;
      if (op->theTracker) {
        transport.set(TransactionTrackerTag, op->theTracker);
      } else {
        boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
        tracker->setAddress( op->thePAddr );
        tracker->setInitiator(flexusIndex());
        tracker->setSource("uArchARM");
        tracker->setOS(false); //TWENISCH - need to set this properly
        transport.set(TransactionTrackerTag, tracker);
        op->theTracker = tracker;
      }

      transport.set(MemoryMessageTag, operation);
      transport.set(uArchStateTag, op);

      if (op->theNAW && (op->thePAddr & 63) != 0) {
        //Auto-reply to the unaligned parts of NAW
        transport[MemoryMessageTag]->type() = MemoryMessage::NonAllocatingStoreReply;
        handleMemoryMessage( transport );
      } else {
        FLEXUS_CHANNEL( MemoryOut_Request) << transport;
      }
    }

    while (FLEXUS_CHANNEL( MemoryOut_Snoop).available()) {
      boost::intrusive_ptr< MemOp > op(theMicroArch->popSnoopOp());
      if (! op ) break;

      DBG_( Iface, ( << "Send Snoop: " << *op) );

      MemoryTransport transport;
      boost::intrusive_ptr<MemoryMessage> operation;

      PhysicalMemoryAddress pc;

      switch ( op->theOperation ) {
        case kInvAck:
          DBG_( Verb, ( << "Send InvAck.") );
          operation = new MemoryMessage(MemoryMessage::InvalidateAck, op->thePAddr);
          break;

        case kDowngradeAck:
          DBG_( Verb, ( << "Send DowngradeAck.") );
          operation = new MemoryMessage(MemoryMessage::DowngradeAck, op->thePAddr);
          break;

        case kProbeAck:
          operation = new MemoryMessage(MemoryMessage::ProbedNotPresent, op->thePAddr);
          break;

        case kReturnReply:
          operation = new MemoryMessage(MemoryMessage::ReturnReply, op->thePAddr );
          break;

        default:
          DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
      }

      operation->reqSize() = op->theSize;
      if (op->theTracker) {
        transport.set(TransactionTrackerTag, op->theTracker);
      } else {
        boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
        tracker->setAddress( op->thePAddr );
        tracker->setInitiator(flexusIndex());
        tracker->setSource("uArchARM");
        transport.set(TransactionTrackerTag, tracker);
      }

      transport.set(MemoryMessageTag, operation);

      FLEXUS_CHANNEL( MemoryOut_Snoop) << transport;
    }

  }

  void handleMemoryMessage( MemoryTransport & aTransport) {
    boost::intrusive_ptr< MemOp > op;
    boost::intrusive_ptr<MemoryMessage> msg (aTransport[MemoryMessageTag]);

    // For Invalidates and Downgrades, the uArchState isn't for us, it's for the original requester
    // So in those cases we always want to construct a new MemOp based on the MemoryMesage
    if (aTransport[uArchStateTag] && msg->type() != MemoryMessage::Invalidate && msg->type() != MemoryMessage::Downgrade) {
      op = aTransport[uArchStateTag];
    } else {
      op = new MemOp();
      op->thePAddr = msg->address();
      op->theSize = eSize(msg->reqSize());
      op->theTracker = aTransport[TransactionTrackerTag];
    }

    switch (msg->type()) {
      case MemoryMessage::LoadReply:
        op->theOperation = kLoadReply;
        break;

      case MemoryMessage::AtomicPreloadReply:
        op->theOperation = kAtomicPreloadReply;
        break;

      case MemoryMessage::StoreReply:
        op->theOperation = kStoreReply;
        break;

      case MemoryMessage::NonAllocatingStoreReply:
        op->theOperation = kStoreReply;
        break;

      case MemoryMessage::StorePrefetchReply:
        op->theOperation = kStorePrefetchReply;
        break;

      case MemoryMessage::Invalidate:
        op->theOperation = kInvalidate;
        break;

      case MemoryMessage::Downgrade:
        op->theOperation = kDowngrade;
        break;

      case MemoryMessage::Probe:
        op->theOperation = kProbe;
        break;

      case MemoryMessage::RMWReply:
        op->theOperation = kRMWReply;
        break;

      case MemoryMessage::CmpxReply:
        op->theOperation = kCASReply;
        break;

      case MemoryMessage::ReturnReq:
        op->theOperation = kReturnReq;
        break;

      default:
        DBG_Assert( false,  ( << "Unhandled Memory Message type: " << msg->type() ) );
    }

    theMicroArch->pushMemOp(op);
  }
};

}//End namespace nuArchARM

FLEXUS_COMPONENT_INSTANTIATOR(uArchARM, nuArchARM);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uArchARM

#define DBG_Reset
#include DBG_Control()
