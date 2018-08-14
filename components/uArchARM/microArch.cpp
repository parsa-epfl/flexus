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


#include <core/types.hpp>
#include <list>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#define __STDC_CONSTANT_MACROS
 #include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;
#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/flexus.hpp>
#include <core/stats.hpp>
namespace Stat = Flexus::Stat;
#include "microArch.hpp"
#include "coreModel.hpp"
#include "ValueTracker.hpp"
#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()
#include "../../core/qemu/mai_api.hpp"
#include <components/armDecoder/armBitManip.hpp>

namespace nuArchARM {

using Flexus::Core::theFlexus;
using namespace std::chrono;

/* For A64 the FPSCR is split into two logically distinct registers,
 * FPCR and FPSR and they still use non-overlapping bits.
 */

static const int32_t kClientIPC = 1;

static const int32_t kFPCR = API::FPCR; //  Floating-point Control Register
static const int32_t kFPSR = API::FPSR; //  Floating-point Status Register

static const int32_t kPSTATE = 41;

static const int32_t kWZR = 51; //WZR	32 bits	Zero register
static const int32_t kXZR = 52; //XZR	64 bits	Zero register
static const int32_t kWSP = 53; //WSP	32 bits	Current stack pointer
static const int32_t kSP = 54; //SP	64 bits	Current stack pointer
static const int32_t kPC = 55; //PC	64 bits	Program counter

static const int32_t kSP_EL0 = 70;
static const int32_t kSP_EL1 = 71;
static const int32_t kSP_EL2 = 72;
static const int32_t kSP_EL3 = 73;

static const int32_t kELR_EL1 = 80;
static const int32_t kELR_EL2 = 81;
static const int32_t kELR_EL3 = 82;

static const int32_t kSPSR_EL1 = 90;
static const int32_t kSPSR_EL2 = 91;
static const int32_t kSPSR_EL3 = 92;


class microArchImpl : public microArch {

  std::string theName;

  std::unique_ptr<CoreModel> theCore;
  int32_t theAvailableROB;
  Flexus::Qemu::Processor theCPU;
  Stat::StatCounter theResynchronizations;
  Stat::StatCounter theResyncInstructions;
  Stat::StatCounter theOtherResyncs;
  Stat::StatCounter theExceptions;
  int32_t theExceptionRaised;
  bool theBreakOnResynchronize;
  bool theDriveClients;
  Flexus::Qemu::Processor theClientCPUs[1];
  int32_t theNumClients;
  int32_t theNode;
  std::function< void(eSquashCause)> squash;
  std::function< void(VirtualMemoryAddress)> redirect;
  std::function< void(int, int)> changeState;
  std::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback;
  std::function< void( bool )> signalStoreForwardingHit;

public:
  microArchImpl( uArchOptions_t options
                 , std::function< void(eSquashCause)> _squash
                 , std::function< void(VirtualMemoryAddress)> _redirect
                 , std::function< void(int, int)> _changeState
                 , std::function< void( boost::intrusive_ptr<BranchFeedback> )> _feedback
                 , std::function<void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> _notifyTMS /* CMU-ONLY */
                 , std::function< void(bool) > _signalStoreForwardingHit
               )
    : theName(options.name)
    , theCore(CoreModel::construct(
                options
                , ll::bind( &microArchImpl::translate, this, ll::_1, ll::_2 )
                , ll::bind( &microArchImpl::advance, this, ll::_1 )
                , _squash
                , _redirect
                , _changeState
                , _feedback
                , _notifyTMS /* CMU-ONLY */
                , _signalStoreForwardingHit
              )
             )
    , theAvailableROB ( 0 )
    , theResynchronizations( options.name  + "-ResyncsCaught" )
    , theResyncInstructions( options.name  + "-ResyncsCaught:Instruction" )
    , theOtherResyncs( options.name  + "-ResyncsCaught:Other" )
    , theExceptions( options.name  + "-ResyncsCaught:Exception" )
    , theExceptionRaised(0)
    , theBreakOnResynchronize(options.breakOnResynchronize)
    , theDriveClients(false)
    , theNumClients(0)
    , theNode(options.node)
    , squash(_squash)
    , redirect(_redirect)
    , changeState(_changeState)
    , feedback(_feedback)
    , signalStoreForwardingHit(_signalStoreForwardingHit)
  {

    theCPU = Flexus::Qemu::Processor::getProcessor(theNode);
    if (Flexus::Qemu::ProcessorMapper::numClients() > 0 ){
      if (theNode == 0) {
        setupDriveClients();
      }
    }		

    DBG_( Tmp, ( << "CORE:  Initializing MMU ")  );
//    theCPU->initializeMMUs();

    if (! theCPU->mai_mode()) {
      DBG_Assert( false , ( << "Simics appears to be running without the -ma option.  You must launch Simics with -ma to run out-of-order simulations (node " << theNode << ")" ) );
    }
    DBG_( Crit, ( << theName << " connected to " << (static_cast<Flexus::Qemu::API::conf_object_t *>(theCPU))->name ));
    theAvailableROB = theCore->availableROB();
    DBG_( Tmp, ( << "CORE:  Resetting Architectural State ")  );

    resetArchitecturalState();
    if (theBreakOnResynchronize && (theNode == 0)) {
      DBG_( Crit, ( << "Simulation will stop on unexpected synchronizations" ) );
    }
  }

  void setupDriveClients() {
    int32_t i = 0;
    int32_t qemu_cpu_no = 0;
    const int32_t max_qemu_cpu_no = 1024;
    bool is_vm_configuration = false;
    int max_vm = 1;
    int vm = 0;
    if (Flexus::Qemu::API::QEMU_get_object("cpu0")!=0){
      DBG_(Dev, ( << "Found cpu0 using vm configuration."));
      is_vm_configuration = true;
      max_vm=16;
    }

    while (true) {
      Flexus::Qemu::API::conf_object_t * client = 0;
     // for ( ; qemu_cpu_no < max_qemu_cpu_no; qemu_cpu_no++) {

        std::string client_cpu("cpu0");
        client = Flexus::Qemu::API::QEMU_get_object(client_cpu.c_str());
        if (client != 0) {
          DBG_( Dev, ( << theName << " microArch will drive " << client_cpu << " with fixed IPC " << kClientIPC) );
          theClientCPUs[i] = Flexus::Qemu::Processor(client);
          ++i;
          ++qemu_cpu_no;
          break;
        } else {
            assert(false);
        }
     // }


    }
    theNumClients = i;
    theDriveClients = true;
  }

  void driveClients() {
    DBG_( Tmp, ( << theName << " Driving " << theNumClients << " client CPUs at IPC: " << kClientIPC ) );
    for (int32_t i = 0; i < theNumClients; ++i) {
      for (int32_t ipc = 0; ipc < kClientIPC; ++ipc) {
        if ( theClientCPUs[i]->advance(true)) {
            theCore->setSuccess(false);
        }
      }
    }
  }

  void dispatch(boost::intrusive_ptr< AbstractInstruction > anInstruction) {
    FLEXUS_PROFILE();
    try {
      boost::intrusive_ptr< Instruction > insn(dynamic_cast< Instruction *>( anInstruction.get() ));
//      DBG_(Tmp,(<< "\e[1;35m" <<"DISPATCH: instruction class: "<<insn->instClassName()<< "\e[0m"));
      theCore->dispatch(insn);
    } catch (...) {
      DBG_( Crit, ( << "Unable to cast from AbstractInstruction to Instruction") );
    }
  }

  bool canPushMemOp() {
    return theCore->canPushMemOp();
  }

  void pushMemOp(boost::intrusive_ptr< MemOp > op) {
    FLEXUS_PROFILE();
    if (op->theOperation == kLoadReply || op->theOperation == kAtomicPreloadReply ) {
      if (op->theSideEffect || op->thePAddr > 0x40000000000LL) {
        //Need to get load value from simics
        DBG_Assert(false);
        DBG_( Verb, ( << "Performing side-effect load to " << op->theVAddr));
	ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).access(theCPU->id(), op->thePAddr);
        Flexus::Qemu::Translation xlat;
        xlat.theVaddr = op->theVAddr;
//        xlat.theASI = op->theASI;
        //xlat.theTL = theCore->getTL();
        xlat.thePSTATE = theCore->getPSTATE();
        xlat.theType = Flexus::Qemu::Translation::eLoad;
        op->theValue = bits(theCPU->readVAddrXendian( xlat, op->theSize ));

      } else {
        //Need to get load value from the ValueTracker
    bits val = ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).load(theCPU->id(),op->thePAddr,op->theSize);
        if (op->theReverseEndian) {
          op->theValue = bits(Flexus::Qemu::endianFlip( val.to_ulong(), op->theSize ));
        } else {
          op->theValue = bits(val);
        }
//        if (op->theASI == 0x24 || op->theASI == 0x34 ) {
//          //Quad LDD
//          op->theExtendedValue = bits(ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).load( theCPU->id(), PhysicalMemoryAddress(op->thePAddr + 8), op->theSize));
//          DBG_( Verb, ( << "Performing quad LDD for addr " << op->thePAddr << " val: " << op->theValue << " ext: " << op->theExtendedValue) );
//          DBG_Assert( ! op->theReverseEndian, ( << "FIXME: inverse endian QUAD_LDD is not implemented. ") );
//        }
      }
    } else if ( op->theOperation == kRMWReply || op->theOperation == kCASReply ) {
      //RMW operations load int32_t theExtendedValue
      ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).access( theCPU->id(), op->thePAddr);
      Flexus::Qemu::Translation xlat;
      xlat.theVaddr = op->theVAddr;
      xlat.thePSTATE = theCore->getPSTATE();
      xlat.theType = Flexus::Qemu::Translation::eStore;
      op->theExtendedValue = bits(theCPU->readVAddrXendian( xlat, op->theSize ));
    } else if ( op->theOperation == kStoreReply && !op->theSideEffect && ! op->theAtomic ) {
      //Need to inform ValueTracker that this store is complete
      bits value = op->theValue;
      if (op->theReverseEndian) {
        value = bits(Flexus::Qemu::endianFlip(value.to_ulong(), op->theSize));
        DBG_(Verb, ( << "Performing inverse endian store for addr " << std::hex << op->thePAddr << " val: " << op->theValue << " inv: " << value << std::dec ));
      }
      ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).commitStore( theCPU->id(), op->thePAddr, op->theSize, value);
    }
    theCore->pushMemOp( op );
  }

  boost::intrusive_ptr<MemOp> popMemOp() {
    return theCore->popMemOp();
  }

  boost::intrusive_ptr<MemOp> popSnoopOp() {
    return theCore->popSnoopOp();
  }

  int32_t availableROB() {
    return theAvailableROB;
  }

  bool isSynchronized() {
    return theCore->isSynchronized();
  }

  bool isQuiesced() {
    return theCore->isQuiesced();
  }

  bool isStalled() {
    return theCore->isStalled();
  }

  int32_t iCount() {
    return theCore->iCount();
  }

  void writePermissionLost(PhysicalMemoryAddress anAddress) {
    theCore->loseWritePermission( eLosePerm_Replacement, anAddress );
  }

  void skipCycle() {
    FLEXUS_PROFILE();
    if ((theCPU->id() == 0) && ( (theFlexus->cycleCount() % 10000) == 0) ) {
      boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
      DBG_(Dev, ( << "Timestamp: " << boost::posix_time::to_simple_string(now)));
    }

    if (theDriveClients) {
      driveClients();
    }

    theAvailableROB = theCore->availableROB();
    theCore->skipCycle();
  }

  void cycle() {
    DBG_( Tmp, ( << "\e[1;32m"<< "uARCH: Starting Cycle "<< this << "\e[0m"));
    FLEXUS_PROFILE();
    if ((theCPU->id() == 0) && ( (theFlexus->cycleCount() % 10000) == 0) ) {
      boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
      DBG_(Dev, ( << "Timestamp: " << boost::posix_time::to_simple_string(now)));
    }

    if (theDriveClients /*&& theCore->getSuccess()*/) {
      driveClients();
    }else     
        DBG_( Tmp, ( << "\e[1;32m"<< "uARCH: Not advancing QEMU as FLEXUS is still processing an instruction"<< "\e[0m"));
    try {

        //Record free ROB space for next cycle
      theAvailableROB = theCore->availableROB();
      theCore->cycle(theCPU->getPendingInterrupt());

    } catch ( ResynchronizeWithSimicsException & e) {
      ++theResynchronizations;
      if (theExceptionRaised) {
        //DBG_( Verb, ( << "CPU[" << std::setfill('0') << std::setw(2) << theCPU->id() << "] Exception Raised: " << Flexus::Qemu::API::SIM_get_exception_name(theCPU, theExceptionRaised) << "(" << theExceptionRaised << "). Resynchronizing with Simics.") );
        DBG_( Verb, ( << "CPU[" << std::setfill('0') << std::setw(2) << theCPU->id() << "] Exception Raised: " << theExceptionRaised << ". Resynchronizing with Simics.") );
        ++theExceptions;
      } else if (e.expected) {
        DBG_( Verb, ( << "CPU[" << std::setfill('0') << std::setw(2) << theCPU->id() << "] Resynchronizing Instruction. Resynchronizing with Simics.") );
        ++theResyncInstructions;
      } else {
        DBG_( Verb, ( << "CPU[" << std::setfill('0') << std::setw(2) << theCPU->id() << "] Flexus Implementation missing/wrong. Resynchronizing with Simics.") );
        ++theOtherResyncs;
      }

      resynchronize();
      if ( theBreakOnResynchronize ) {
        DBG_( Dev, ( << "CPU[" << std::setfill('0') << std::setw(2) << theCPU->id() << "] Resynchronize complete\n========================================================\n" ) );
        theCPU->breakSimulation();
      }
      theExceptionRaised = 0;
    }
    if (0) {
      if (theCore->pc() != static_cast<uint64_t>(theCPU->getPC())) {
        DBG_( Crit, ( << theName << " PC mismatch expected: " << theCPU->getPC() << " actual: " << std::hex << theCore->pc() << std::dec )  );
      }
    }
    DBG_( Tmp, ( << "\e[1;32m"<< "uARCH: Ending Cycle "<< this<<"\e[0m"));

  }

  void translate(Flexus::Qemu::Translation & aTranslation, bool aTakeTrap) const {
    theCPU->translate(aTranslation, aTakeTrap);
  }

private:

  void resynchronize() {
    FLEXUS_PROFILE();

    DBG_( Tmp, ( << "\e[1;35m"  << "Resynchronizing " << "\e[0m")  );


//    DBG_( Tmp, ( << "CORE:  Resetting Core ")  );

    //Clear out all state in theCore
    theCore->reset();
    theAvailableROB = theCore->availableROB();

    if (theExceptionRaised) {
      squash(kException);
    } else {
      squash(kResynchronize);
    }

    //Obtain new state from simics
    VirtualMemoryAddress redirect_address(theCPU->getPC());

    resetArchitecturalState();

    redirect(redirect_address);
  }

  int32_t advance(bool anAcceptPendingInterrupt) {
    FLEXUS_PROFILE();
    theExceptionRaised = theCPU->advance(anAcceptPendingInterrupt);
    theFlexus->watchdogReset(theCPU->id());
    return theExceptionRaised;
  }

void resetArchitecturalState()
{
    theCore->setPC( theCPU->getPC());
    resetRoundingMode();
    //resetSpecialRegs(); // Mark removed until exceptions can be read from QEMU
    fillXRegisters();
    fillVRegisters();
//    theCPU->resyncMMU();
}

  void resetSpecialRegs() {

    resetPSTATE();
    resetCurrentEL();
    resetSP();
    resetEL();
    resetSPSR();
    resetFPCR();
    resetFPSR();

  }

  void resetRoundingMode() {
    uint64_t fpsr = theCPU->readRegister(-1, kFPSR);
    theCore->setRoundingMode( (fpsr >> 30) & 3 );
  }

  void resetPSTATE() {
    uint64_t pstate = theCPU->readRegister( 0, API::PSTATE );
    theCore->setPSTATE( pstate );
  }

  void resetSP() {
      uint64_t sp;
      for (int i=0; i<4;++i)
      {
          sp = theCPU->readRegister( i, API::STACK_POINTER );
          theCore->setSP( sp, i);
      }

  }
  void resetEL() {
      uint64_t el;
      for (int i=0; i<3;++i)
      {
          el = theCPU->readRegister( i, API::EXCEPTION_LINK );
          theCore->setEL( el , i);
      }

  }
  void resetSPSR_EL() {
      uint64_t spsr;
      for (int i=0; i<3;++i)
      {
          spsr = theCPU->readRegister( i, API::SPSR_EL );
          theCore->setSPSR_EL( spsr, i );
      }

  }
  // fills/re-sets the  to the state they are
  // in the VM (i.e. client)
  void resetFPSR() {
    uint64_t fpsr = theCPU->readRegister( 0,  API::FLOATING_POINT );
    theCore->setFPSR( fpsr );
  }

  void resetFPCR() {
    uint64_t fpcr = theCPU->readRegister( 0, API::FLOATING_POINT );
    theCore->setFPCR( fpcr );
  }

  void resetCurrentEL() {
    uint64_t ps = theCore.get()->getPSTATE();
    theCore->setCurrentEL( extract32(ps, 2, 2) );
  }

  void resetSPSR() {
    uint64_t spsr = theCPU->readRegister( 0, API::SPSR );
    theCore->setSPSR( spsr );
  }


  // fills/re-sets global registers to the state they are
  // in the VM (i.e. client)
  void fillXRegisters() {
    for (int32_t i = 0; i < 32; ++i) {
      uint64_t val = theCPU->readRegister(i, API::GENERAL);
      theCore->initializeRegister( xReg(i), val);
    }
  }
  // fills/re-sets floating point registers to the state they are
  // in the VM (i.e. client)
  void fillVRegisters() {
    for (int32_t i = 0; i < 32; ++i) {
      uint64_t val = theCPU->readRegister(i, API::FLOATING_POINT);
      theCore->initializeRegister( vReg(i), val);
    }
  }


  // checkpoint restore

  void testCkptRestore() {
    armState state;
    theCore->getARMState(state);
    theCore->resetARM();
    theCore->restoreARMState(state);

    theAvailableROB = theCore->availableROB();
    squash(kResynchronize);

    //Obtain new state from simics
    VirtualMemoryAddress redirect_address(theCore->pc());
  }

  void printROB() {
    theCore->printROB();
  }
  void printSRB() {
    theCore->printSRB();
  }
  void printMemQueue() {
    theCore->printMemQueue();
  }
  void printMSHR() {
    theCore->printMSHR();
  }
  void printRegMappings(std::string aStr) {
    theCore->printRegMappings(aStr);
  }
  void printRegFreeList(std::string aStr) {
    theCore->printRegFreeList(aStr);
  }
  void printRegReverseMappings(std::string aStr) {
    theCore->printRegReverseMappings(aStr);
  }
  void printAssignments(std::string aStr) {
    theCore->printAssignments(aStr);
  }

  void pregsSys(armState const & state) {
    std::cout << "          %pc                %npc                %tba                %cwp\n";
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.thePC;
    std::cout << "          %ccr               %fpsr               %fpsr                %pstate\n";
    std::cout << std::endl;
  }

  void pregsAll() {
    std::cout << "Flexus processor " << theName << std::endl;
    armState state;
    theCore->getARMState(state);
    std::cout << "      %g (normal   )      %g (alternate)      %g (mmu      )      %g (interrupt)\n";
    for (int32_t i = 1; i < 8; ++i) {
      std::cout << i << " ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ 8 + i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ 16 + i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ 24 + i]  << "  ";
      std::cout << std::endl;
    }
    pregsSys(state);
  }

  void pregs() {
    char const * reg_sets[] = { "normal   ", "alternate", "mmu      ", "interrupt" };
    std::cout << "Flexus processor " << theName << std::endl;
    armState state;
    theCore->getARMState(state);
    for (int32_t i = 0; i < 8; ++i) {
      std::cout << i << " ";
      std::cout << std::endl;
    }
    std::cout << std::endl;

    pregsSys(state);
  }

};

std::shared_ptr<microArch> microArch::construct( uArchOptions_t options
    , std::function< void(eSquashCause) > squash
    , std::function< void(VirtualMemoryAddress) > redirect
    , std::function< void(int, int) > changeState
    , std::function< void( boost::intrusive_ptr<BranchFeedback> ) > feedback
    , std::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> ) > notifyTMS /* CMU-ONLY */
    , std::function< void( bool )> signalStoreForwardingHit
                                                 ) {
  return std::make_shared<microArchImpl>(options
                                      , squash
                                      , redirect
                                      , changeState
                                      , feedback
                                      , notifyTMS /* CMU-ONLY */
                                      , signalStoreForwardingHit
                                     );
}

} //nuArchARM
