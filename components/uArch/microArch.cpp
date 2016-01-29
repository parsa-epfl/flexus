#include <list>
#include <iostream>
#include <iomanip>
#include <chrono>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

// #define __STDC_CONSTANT_MACROS
// #include <boost/date_time/posix_time/posix_time.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <components/uArch/uArchInterfaces.hpp>
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

namespace nuArch {

using Flexus::Core::theFlexus;
using namespace std::chrono;

static const int32_t kY = 34;
static const int32_t kCCR = 35;
static const int32_t kFPRS = 36;
static const int32_t kFSR = 37;
static const int32_t kASI = 38;
static const int32_t kTICK = 39;
static const int32_t kGSR = 40;
static const int32_t kSTICK = 43;
static const int32_t kPSTATE = 45;
static const int32_t kAG = 0x1;
static const int32_t kMG = 0x400;
static const int32_t kIG = 0x800;
static const int32_t kTL = 46;
static const int32_t kPIL = 47;
static const int32_t kTPC1 = 48;
static const int32_t kTNPC1 = 58;
static const int32_t kTSTATE1 = 68;
static const int32_t kTT1 = 78;
static const int32_t kTBA = 88;

static const int32_t kVER = 89;
static const int32_t kCWP = 90;
static const int32_t kCANSAVE = 91;
static const int32_t kCANRESTORE = 92;
static const int32_t kOTHERWIN = 93;
static const int32_t kWSTATE = 94;
static const int32_t kCLEANWIN = 95;

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
  Flexus::Qemu::Processor theClientCPUs[64];
  int32_t theNumClients;
  int32_t theNode;
  std::function< void(eSquashCause)> squash;
  std::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> redirect;
  std::function< void(int, int)> changeState;
  std::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback;
  std::function< void( bool )> signalStoreForwardingHit;

public:
  microArchImpl( uArchOptions_t options
                 , std::function< void(eSquashCause)> _squash
                 , std::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> _redirect
                 , std::function< void(int, int)> _changeState
                 , std::function< void( boost::intrusive_ptr<BranchFeedback> )> _feedback
                 , std::function<void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> _notifyTMS /* CMU-ONLY */
                 , std::function< void(bool) > _signalStoreForwardingHit
               )
    : theName(options.name)
    , theCore(CoreModel::construct(
                options
                , [this](auto x, auto y){ return this->translate(x,y); }
                , [this](auto x){ return this->advance(x); }
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
    , signalStoreForwardingHit(_signalStoreForwardingHit) {
#if 0
    bool client_server = false;
    if (Flexus::Qemu::API::SIM_get_object("cpu0") == 0) {
      if (Flexus::Qemu::API::SIM_get_object("server_cpu0") == 0) {
        DBG_Assert( false, ( << theName << "microArch cannot locate cpu0 or server_cpu0 objects." ) );
      } else {
        DBG_( Dev, ( << theName << " microArch detected client-server simulation.  Connecting to server_cpu" << theNode) );
        client_server = true;
      }
    }

    if (client_server) {
      // std::string my_cpu("server_cpu");
      // my_cpu += boost::lexical_cast<std::string>(theNode);

      // theCPU = Flexus::Qemu::Processor::getProcessor(my_cpu);
      theCPU = Flexus::Qemu::Processor::getProcessor(theNode);

      if (theNode == 0) {
        setupDriveClients();
      }
    } else {
      theCPU = Flexus::Qemu::Processor::getProcessor(theNode);
    }
#endif

    DBG_( Crit, ( << "WARNING: uArch trying to figure out if we have a multi-tier setting. Need to fix this when we solve the cpu naming issue."));
/* //ALEX- FIXME
    if ((Flexus::Qemu::API::QEMU_get_object("cpu0") == 0) && (Flexus::Qemu::API::QEMU_get_object("machine0_cpu0")==0)) {
      if ((Flexus::Qemu::API::QEMU_get_object("server_cpu0") == 0) && (Flexus::Qemu::API::QEMU_get_object("machine0_server_cpu0")==0)) {
        DBG_Assert( false, ( << theName << "microArch cannot locate cpu0 or server_cpu0 objects." ) );
      } else {
        DBG_( Dev, ( << theName << " microArch detected client-server simulation.  Connecting to server_cpu" << theNode) );
      }
    }
*/
    theCPU = Flexus::Qemu::Processor::getProcessor(theNode);
    if (Flexus::Qemu::ProcessorMapper::numClients() > 0 ){
      if (theNode == 0) {
        setupDriveClients();
      }
    }		

    theCPU->initializeMMUs();

    theCore->setSTICKInterval( theCPU->getSystemTickInterval() );

    if (! theCPU->mai_mode()) {
      DBG_Assert( false , ( << "Simics appears to be running without the -ma option.  You must launch Simics with -ma to run out-of-order simulations (node " << theNode << ")" ) );
    }

    DBG_( Crit, ( << theName << " connected to " << (static_cast<Flexus::Qemu::API::conf_object_t *>(theCPU))->name ));

    theAvailableROB = theCore->availableROB();

    resetArchitecturalState();

    if (theBreakOnResynchronize && (theNode == 0)) {
      DBG_( Crit, ( << "Simulation will stop on unexpected synchronizations" ) );
    }
  }

  static const int32_t kClientIPC = 8;

  void setupDriveClients() {
    int32_t i = 0;
    int32_t simics_cpu_no = 0;
    const int32_t max_simics_cpu_no = 1024;
    bool is_vm_configuration = false;
    int max_vm = 1;
    int vm = 0;
    if (Flexus::Qemu::API::QEMU_get_object("machine0_client_cpu0")!=0){
      DBG_(Dev, ( << "Found machine0_client_cpu0 using vm configuration."));
      is_vm_configuration = true;
      max_vm=16;
    }

    while (true) {
      Flexus::Qemu::API::conf_object_t * client = 0;
      for ( ; simics_cpu_no < max_simics_cpu_no; simics_cpu_no++) {
        std::string client_cpu("client_cpu");
        if (is_vm_configuration) {
      	  client_cpu = "machine" + boost::lexical_cast<std::string>(vm) + "_client_cpu";
	}
        client_cpu += boost::lexical_cast<std::string>(simics_cpu_no);
        client = Flexus::Qemu::API::QEMU_get_object(client_cpu.c_str());
        if (client != 0) {
          DBG_( Dev, ( << theName << " microArch will drive " << client_cpu << " with fixed IPC " << kClientIPC) );
          theClientCPUs[i] = Flexus::Qemu::Processor(client);
          break;
        }
        // added by PLotfi to support SPECweb2009 workloads
        client_cpu = "besim_cpu";
        if (is_vm_configuration) {
          client_cpu = "machine" + boost::lexical_cast<std::string>(vm) + "_besim_cpu";
        }
        client_cpu += boost::lexical_cast<std::string>(simics_cpu_no);
        client = Flexus::Qemu::API::QEMU_get_object(client_cpu.c_str());
        if (client != 0) {
          DBG_( Dev, ( << theName << " microArch will drive " << client_cpu << " with fixed IPC " << kClientIPC) );
          theClientCPUs[i] = Flexus::Qemu::Processor(client);
          break;
        }
      }
      if (client == 0) {
        simics_cpu_no = 0;
        vm++;
        if (vm < max_vm){
	  continue;
	}
        break;
      }
      ++i;
      ++simics_cpu_no;
    }
    theNumClients = i;
      
    /* LOOK THE DIFF FILE */

    theDriveClients = true;
  }

  void driveClients() {
    DBG_( Verb, ( << theName << " Driving " << theNumClients << " client CPUs at IPC: " << kClientIPC ) );
    for (int32_t i = 0; i < theNumClients; ++i) {
      for (int32_t ipc = 0; ipc < kClientIPC; ++ipc) {
        if ( theClientCPUs[i]->advance(true) == -1) {
          theClientCPUs[i]->advance(false);
        }
      }
    }
  }

  void dispatch(boost::intrusive_ptr< AbstractInstruction > anInstruction) {
    FLEXUS_PROFILE();
    try {
      boost::intrusive_ptr< Instruction > insn(dynamic_cast< Instruction *>( anInstruction.get() ));
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
        DBG_( Verb, ( << "Performing side-effect load to " << op->theVAddr << " ASI: " << op->theASI ));
	ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).access(theCPU->id(), op->thePAddr);
        Flexus::Qemu::Translation xlat;
        xlat.theVaddr = op->theVAddr;
        xlat.theASI = op->theASI;
        xlat.theTL = theCore->getTL();
        xlat.thePSTATE = theCore->getPSTATE();
        xlat.theType = Flexus::Qemu::Translation::eLoad;
        op->theValue = theCPU->readVAddrXendian( xlat, op->theSize );

      } else {
        //Need to get load value from the ValueTracker
	uint64_t val = ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).load(theCPU->id(),op->thePAddr,op->theSize);
        if (op->theReverseEndian) {
          op->theValue = Flexus::Qemu::endianFlip( val, op->theSize );
        } else {
          op->theValue = val;
        }
        if (op->theASI == 0x24 || op->theASI == 0x34 ) {
          //Quad LDD
          op->theExtendedValue = ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).load( theCPU->id(), PhysicalMemoryAddress(op->thePAddr + 8), op->theSize);
          DBG_( Verb, ( << "Performing quad LDD for addr " << op->thePAddr << " val: " << op->theValue << " ext: " << op->theExtendedValue) );
          DBG_Assert( ! op->theReverseEndian, ( << "FIXME: inverse endian QUAD_LDD is not implemented. ") );
        }
      }
    } else if ( op->theOperation == kRMWReply || op->theOperation == kCASReply ) {
      //RMW operations load int32_t theExtendedValue
      ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theCPU->id())).access( theCPU->id(), op->thePAddr);
      Flexus::Qemu::Translation xlat;
      xlat.theVaddr = op->theVAddr;
      xlat.theASI = op->theASI;
      xlat.theTL = theCore->getTL();
      xlat.thePSTATE = theCore->getPSTATE();
      xlat.theType = Flexus::Qemu::Translation::eStore;
      op->theExtendedValue = theCPU->readVAddrXendian( xlat, op->theSize );
    } else if ( op->theOperation == kStoreReply && !op->theSideEffect && ! op->theAtomic ) {
      //Need to inform ValueTracker that this store is complete
      uint64_t value = op->theValue;
      if (op->theReverseEndian) {
        value = Flexus::Qemu::endianFlip(value, op->theSize);
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
      auto now(system_clock::now());
      auto tt = system_clock::to_time_t(now);
      DBG_(Dev, ( << "Timestamp: " << std::put_time(std::localtime(&tt), "%Y-%h-%d %T")));
    }

    if (theDriveClients) {
      driveClients();
    }

    theAvailableROB = theCore->availableROB();
    theCore->skipCycle();

  }

  void cycle() {
    FLEXUS_PROFILE();
    if ((theCPU->id() == 0) && ( (theFlexus->cycleCount() % 10000) == 0) ) {
      auto now(system_clock::now());
      auto tt = system_clock::to_time_t(now);
      DBG_(Dev, ( << "Timestamp: " << std::put_time(std::localtime(&tt), "%Y-%h-%d %T")));
    }

    if (theDriveClients) {
      driveClients();
    }

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
      if (theCore->npc() != static_cast<uint64_t>(theCPU->getNPC())) {
        DBG_( Crit, ( << theName << " NPC mismatch; PC: " << theCPU->getPC() << " NPC expected: " << theCPU->getNPC() << " actual: " << std::hex << theCore->npc() << std::dec )  );
      }
    }
  }

  void translate(Flexus::Qemu::Translation & aTranslation, bool aTakeTrap) const {
    theCPU->translate(aTranslation, aTakeTrap);
  }

private:
  void resynchronize() {
    FLEXUS_PROFILE();

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
    VirtualMemoryAddress next_address(theCPU->getNPC());

    resetArchitecturalState();

    redirect(redirect_address, next_address);

  }

  int32_t advance(bool anAcceptPendingInterrupt) {
    FLEXUS_PROFILE();
    theExceptionRaised = theCPU->advance(anAcceptPendingInterrupt);
    theFlexus->watchdogReset(theCPU->id());
    return theExceptionRaised;
  }

  void resetArchitecturalState() {
    theCore->setPC( theCPU->getPC());
    theCore->setNPC( theCPU->getNPC());

    resetRoundingMode();
    resetPrivilegedRegs();
    changeState(theCPU->readRegister( kTL ), theCPU->readRegister( kPSTATE ) );
    fillRRegisters();
    fillFRegisters();
    fillCCBits();
    theCPU->resyncMMU();
  }

  void resetPrivilegedRegs() {
    resetAG();
    resetMG();
    resetIG();
    resetTPC();
    resetTNPC();
    resetTSTATE();
    resetTT();
    resetTBA();
    resetPSTATE();
    resetTL();
    resetPIL();
    resetCWP();
    resetCANSAVE();
    resetCANRESTORE();
    resetCLEANWIN();
    resetOTHERWIN();
    resetWSTATE();
    resetVER();
    resetFPRS();
    resetFSR();
    resetSTICK();
    resetTICK();
  }

  void resetRoundingMode() {
    uint64_t fsr = theCPU->readRegister( kFSR );
    theCore->setRoundingMode( (fsr >> 30) & 3 );
  }

  void resetAG() {
    uint32_t pstate = theCPU->readRegister( kPSTATE );
    theCore->setAG( pstate & kAG  );
  }

  void resetMG() {
    uint32_t pstate = theCPU->readRegister( kPSTATE );
    theCore->setMG( pstate & kMG  );
  }

  void resetIG() {
    uint32_t pstate = theCPU->readRegister( kPSTATE );
    theCore->setIG( pstate & kIG  );
  }

  void resetTPC() {
    int32_t reg = kTPC1;
    for (int32_t i = 0; i < 5; ++i) {
      uint64_t tpc = theCPU->readRegister( reg );
      theCore->setTPC( tpc, i + 1 );
      ++reg;
    }
  }

  void resetTNPC() {
    int32_t reg = kTNPC1;
    for (int32_t i = 0; i < 5; ++i) {
      uint64_t tnpc = theCPU->readRegister( reg );
      theCore->setTNPC( tnpc, i + 1);
      ++reg;
    }
  }

  void resetTSTATE() {
    int32_t reg = kTSTATE1;
    for (int32_t i = 0; i < 5; ++i) {
      uint64_t tstate = theCPU->readRegister( reg );
      theCore->setTSTATE( tstate, i + 1  );
      ++reg;
    }
  }

  void resetTT() {
    int32_t reg = kTT1;
    for (int32_t i = 0; i < 5; ++i) {
      uint64_t tt = theCPU->readRegister( reg );
      theCore->setTT( tt, i + 1 );
      ++reg;
    }
  }

  void resetTBA() {
    uint64_t tba = theCPU->readRegister( kTBA );
    theCore->setTBA( tba );
  }

  void resetPSTATE() {
    uint64_t pstate = theCPU->readRegister( kPSTATE );
    theCore->setPSTATE( pstate );
  }

  void resetTL() {
    uint32_t tl = theCPU->readRegister( kTL );
    theCore->setTL( tl );
  }

  void resetPIL() {
    uint32_t pil = theCPU->readRegister( kPIL );
    theCore->setPIL( pil );
  }

  void resetCWP() {
    uint32_t cwp = theCPU->readRegister( kCWP );
    theCore->setCWP( cwp );
  }

  void resetCANSAVE() {
    uint32_t cansave = theCPU->readRegister( kCANSAVE );
    theCore->setCANSAVE( cansave);
  }

  void resetCANRESTORE() {
    uint32_t canrestore = theCPU->readRegister( kCANRESTORE );
    theCore->setCANRESTORE( canrestore );
  }

  void resetCLEANWIN() {
    uint32_t cleanwin = theCPU->readRegister( kCLEANWIN );
    theCore->setCLEANWIN( cleanwin );
  }

  void resetOTHERWIN() {
    uint32_t otherwin = theCPU->readRegister( kOTHERWIN );
    theCore->setOTHERWIN( otherwin );
  }

  void resetWSTATE() {
    uint32_t wstate = theCPU->readRegister( kWSTATE );
    theCore->setWSTATE( wstate );
  }

  void resetVER() {
    uint64_t ver = theCPU->readRegister( kVER );
    theCore->setVER( ver );
  }

  void resetFPRS() {
    uint64_t fprs = theCPU->readRegister( kFPRS );
    theCore->setFPRS( fprs );
  }

  void resetSTICK() {
    uint64_t stick = theCPU->readRegister( kSTICK );
    theCore->setSTICK( stick );
  }

  void resetTICK() {
    uint64_t tick = theCPU->readRegister( kTICK );
    theCore->setTICK( tick );
  }

  void resetFSR() {
    uint64_t fsr = theCPU->readRegister( kFSR );
    theCore->setFSR( fsr );
  }

  void fillCCBits() {
    uint32_t ccval = theCPU->readRegister( kCCR );
    std::bitset<8> ccbits(ccval);
    theCore->initializeRegister( ccReg(0), ccbits);

    uint64_t fsr = theCPU->readRegister( kFSR );
    std::bitset<8> fcc0( ( fsr >> 10) & 3 );
    std::bitset<8> fcc1( ( fsr >> 32) & 3 );
    std::bitset<8> fcc2( ( fsr >> 34) & 3 );
    std::bitset<8> fcc3( ( fsr >> 36) & 3 );
    theCore->initializeRegister( ccReg(1), fcc0);
    theCore->initializeRegister( ccReg(2), fcc1);
    theCore->initializeRegister( ccReg(3), fcc2);
    theCore->initializeRegister( ccReg(4), fcc3);
  }

  void fillFRegisters() {
    for (int32_t i = 0; i < 64; i += 2) {
      //uint64_t ret_val = theCPU->readF(i);
      uint64_t ret_val = theCPU->readF(i/2);
      theCore->initializeRegister( fReg(i), ret_val >> 32);
      theCore->initializeRegister( fReg(i + 1), ret_val & 0xFFFFFFFFULL);
    }
  }

  void fillRRegisters() {
    //Global registers
    for (int32_t i = 0; i < 8; ++i) {
      uint64_t val = theCPU->readG(i);
      theCore->initializeRegister( rReg(i), val);
    }

    //Alternate Global registers
    for (int32_t i = 0; i < 8; ++i) {
      uint64_t val = theCPU->readAG(i);
      theCore->initializeRegister( rReg(i + 8) , val);
    }

    //MMU Global registers
    for (int32_t i = 0; i < 8; ++i) {
      uint64_t val = theCPU->readMG(i);
      theCore->initializeRegister( rReg(i + 16) , val);
    }

    //Interrupt Global registers
    for (int32_t i = 0; i < 8; ++i) {
      uint64_t val = theCPU->readIG(i);
      theCore->initializeRegister( rReg(i + 24) , val);
    }

    //Windowed registers
    uint32_t preg = 32;
    for (int32_t w = 7; w >= 0 ; --w) {
      for (int32_t i = 8; i < 24; ++i) {
        uint64_t val = theCPU->readWindowed(w, i);
        theCore->initializeRegister( rReg(preg), val);
        ++preg;
      }
    }

    //Y
    DBG_Assert( kRegY == preg );
    ++preg;
    uint64_t val = theCPU->readRegister( kY );
    theCore->initializeRegister( rReg( kRegY ), val);

    //ASI
    DBG_Assert( kRegASI == preg );
    ++preg;
    val = theCPU->readRegister( kASI );
    theCore->initializeRegister( rReg( kRegASI ), val);

    //GSR
    DBG_Assert( kRegGSR == preg );
    ++preg;
    val = theCPU->readRegister( kGSR );
    theCore->initializeRegister( rReg( kRegGSR ), val);

  }

  void testCkptRestore() {
    v9State state;
    theCore->getv9State(state);
    theCore->resetv9();
    theCore->restorev9State(state);

    theAvailableROB = theCore->availableROB();
    squash(kResynchronize);

    //Obtain new state from simics
    VirtualMemoryAddress redirect_address(theCore->pc());
    VirtualMemoryAddress next_address(theCore->npc());
    redirect(redirect_address, next_address);
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

  void pregsSys(v9State const & state) {
    std::cout << "          %pc                %npc                %tba                %cwp\n";
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.thePC;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theNPC;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTBA;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theCWP;
    std::cout << std::endl;

    std::cout << "          %ccr               %fprs               %fsr                %pstate\n";
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theCCR;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theFPRS;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theFSR;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.thePSTATE;
    std::cout << std::endl;

    std::cout << "          %asi               %gsr                %tl                 %pil\n";
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theASI;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGSR;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTL;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.thePIL;
    std::cout << std::endl;

    std::cout << "          %cansave           %canrestore         %cleanwin           %otherwin\n";
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theCANSAVE;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theCANRESTORE;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theCLEANWIN;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theOTHERWIN;
    std::cout << std::endl;

    std::cout << "          %softint           %wstate             %y                  $globals\n";
    std::cout << "  0x----------------"; //SOFTINT
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWSTATE;
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theY;
    std::cout << "  0x----------------"; //GLOBALS
    std::cout << std::endl;
    std::cout << "          %tick              %tick_cmpr          %stick              %stick_cmpr\n";
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTICK;
    std::cout << "  0x----------------"; //TICK_CMPR
    std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theSTICK;
    std::cout << "  0x----------------"; //STICK_CMPR
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "          %tpc               %tnpc               %tstate             %tt\n";
    for (int32_t i = 0; i < 5; ++i) {
      std::cout << i + 1 << " ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTTE[i].theTPC;
      std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTTE[i].theTNPC;
      std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTTE[i].theTSTATE;
      std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.theTTE[i].theTT;
      std::cout << std::endl;
    }

  }

  void pregsAll() {
    std::cout << "Flexus processor " << theName << std::endl;
    v9State state;
    theCore->getv9State(state);
    std::cout << "      %g (normal   )      %g (alternate)      %g (mmu      )      %g (interrupt)\n";
    for (int32_t i = 1; i < 8; ++i) {
      std::cout << i << " ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ 8 + i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ 16 + i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ 24 + i]  << "  ";
      std::cout << std::endl;
    }
    std::cout << "          %i                  %l                  %o\n";
    for (int32_t i = 0; i < 8; ++i) {
      std::cout << "Window " << i << ":\n";
      for (int32_t j = 0; j < 8 ; ++j) {
        std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWindowRegs[ ((7-i) * 16 + 16 + j) % 128 ] << "  ";
        std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWindowRegs[ ((7-i) * 16 + 8  + j) % 128 ] << "  ";
        std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWindowRegs[ ((7-i) * 16 + 0  + j) % 128 ] << "  ";
        std::cout << std::endl;
      }
    }

    pregsSys(state);
  }

  void pregs() {
    char const * reg_sets[] = { "normal   ", "alternate", "mmu      ", "interrupt" };

    std::cout << "Flexus processor " << theName << std::endl;
    v9State state;
    theCore->getv9State(state);
    std::cout << "    %g " << reg_sets[theCore->selectedRegisterSet()] << "           %o                   %l                  %i" << std::endl;
    for (int32_t i = 0; i < 8; ++i) {
      std::cout << i << " ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[ theCore->selectedRegisterSet()*8 + i]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWindowRegs[ ( (7- state.theCWP) * 16 + 0  + i) % 128 ] << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWindowRegs[ ( (7- state.theCWP) * 16 + 8 + i) % 128 ]  << "  ";
      std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theWindowRegs[ ( (7- state.theCWP) * 16 + 16 + i) % 128 ]  << "  ";
      std::cout << std::endl;
    }
    std::cout << std::endl;

    pregsSys(state);
  }

};

std::shared_ptr<microArch> microArch::construct( uArchOptions_t options
    , std::function< void(eSquashCause) > squash
    , std::function< void(VirtualMemoryAddress, VirtualMemoryAddress) > redirect
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

} //nuArch
