
#include <algorithm>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant/get.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
using namespace boost::multi_index;
#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <core/performance/profile.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
namespace Stat = Flexus::Stat;

#include "../BypassNetwork.hpp"
#include "../CoreModel.hpp"
#include "../MapTable.hpp"
#include "../RegisterFile.hpp"
#include "../systemRegister.hpp"
#include "FPStatRegisters.hpp" // Msutherl
#include "PSTATE.hpp"
#include "SCTLR_EL.hpp"
#include "bbv.hpp" /* CMU-ONLY */
#include "coreModelTypes.hpp"

#include <components/CommonQEMU/Slices/PredictorMessage.hpp> /* CMU-ONLY */
#include <components/CommonQEMU/Transports/TranslationTransport.hpp>
#include <components/CommonQEMU/XactTimeBreakdown.hpp>

// Msutherl, Oct'18
#include <core/qemu/mai_api.hpp>

namespace nuArch {

#define QEMU_EXCP_INTERRUPT 0x10000 /* async interruption */
#define QEMU_EXCP_HLT       0x10001 /* hlt instruction reached */
#define QEMU_EXCP_DEBUG     0x10002 /* cpu stopped after a breakpoint or singlestep */
#define QEMU_EXCP_HALTED    0x10003 /* cpu is halted (waiting for external event) */
#define QEMU_EXCP_YIELD     0x10004 /* cpu wants to yield timeslice to another */
#define QEMU_EXCP_ATOMIC    0x10005 /* stop-the-world and emulate atomic */

/* Custom defined flag, they are not used in Flexus, but may help understand
 * QEMU behaviour when debuging */
#define EXCP_QFLEX_IDLE    0x10010
#define EXCP_QFLEX_UNPLUG  0x10011
#define EXCP_QFLEX_STOP    0x10012
#define EXCP_QFLEX_UNKNOWN 0x10013

using nXactTimeBreakdown::TimeBreakdown;

struct ExceptionRecord
{
    eExceptionType type; // Exception class
    uint32_t syndrome;   // Syndrome record
    uint64_t vaddress;   // Virtual fault address
    bool ipavalid;       // Physical fault address is valid
    uint64_t ipaddress;  // Physical fault address for second stage fault
};

static std::map<uint8_t, std::map<PhysicalMemoryAddress, uint64_t>> GLOBAL_EXCLUSIVE_MONITOR;

class CoreImpl : public CoreModel
{
    // CORE STATE
    //==========================================================================
    // Simulation
    std::string theName;
    uint32_t theNode;

    // Msutherl - removed translate as a call to microArch,
    // now internal to CoreModel
    // std::function< void (Flexus::Qemu::Translation &) > translate;
    std::function<int(bool)> advance_fn;
    std::function<void(eSquashCause)> squash_fn;
    std::function<void(VirtualMemoryAddress)> redirect_fn;
    std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback_fn;
    std::function<void(bool)> signalStoreForwardingHit_fn;
    std::function<void(int32_t)> mmuResync_fn;

    // register renaming  architectural -> physical
    // Map Tables
    std::vector<std::shared_ptr<PhysicalMap>> theMapTables;

    // Register Files
    RegisterFile theRegisters;
    int32_t theRoundingMode;

    uint64_t thePC;
    bool theAARCH64;
    uint32_t thePSTATE;

    // MMU Internal State:
    // ===================================================================
  private:
    //  std::shared_ptr<mmu_t> theMMU;
    //  bool mmuInitialized;
    Flexus::Qemu::Processor theQEMUCPU;

    // ===================================================================
    //  std::multimap<int, SysRegInfo*> * theSystemRegisters;

    uint32_t theDCZID_EL0;
    uint64_t theSCTLR_EL[4];
    uint64_t theHCR_EL2;

    // MARK: Change these to structs with overloads
    CImpl_FPSR theFPSR;
    CImpl_FPCR theFPCR;
    Flexus::Qemu::API::exception_t theEXP;

    std::map<PhysicalMemoryAddress, uint64_t> theLocalExclusivePhysicalMonitor;
    std::map<VirtualMemoryAddress, uint64_t> theLocalExclusiveVirtualMonitor;

    uint64_t theSP_el[4];
    uint64_t theELR_EL[4];
    uint64_t theSPSR_EL[4];

    eExceptionType thePendingTrap;
    boost::intrusive_ptr<Instruction> theTrapInstruction;

    // Bypass Network
    BypassNetwork theBypassNetwork;

    uint64_t theLastGarbageCollect;

    // Semantic Action & Effect Management
    action_list_t theActiveActions;
    action_list_t theRescheduledActions;

    std::list<boost::intrusive_ptr<Interaction>> theDispatchInteractions;
    bool thePreserveInteractions;

    // Resource arbitration
    MemoryPortArbiter theMemoryPortArbiter;

    // Reorder Buffer
    rob_t theROB;
    int32_t theROBSize;

    // Speculative Retirement Buffer
    rob_t theSRB;

    // Retirement
    uint32_t theRetireWidth;

    // Interrupt processing
    bool theInterruptSignalled;
    eExceptionType thePendingInterrupt;
    boost::intrusive_ptr<Instruction> theInterruptInstruction;

    // Branch Feedback
    std::list<boost::intrusive_ptr<BranchFeedback>> theBranchFeedback;

    // Squash and Redirect control
    bool theSquashRequested;
    eSquashCause theSquashReason;
    rob_t::iterator theSquashInstruction;
    bool theSquashInclusive;

    bool theRedirectRequested;
    VirtualMemoryAddress theRedirectPC;
    VirtualMemoryAddress theDumpPC;

    // Load Store Queue and associated memory control
    uint64_t theMemorySequenceNum;

  public:
    memq_t theMemQueue;

  private:
    int64_t theLSQCount;
    int64_t theSBCount; // Includes SSB
    int64_t theSBNAWCount;
    int64_t theSBSize;
    bool theNAWBypassSB;
    bool theNAWWaitAtSync;
    MSHRs_t theMSHRs;
    std::list<TranslationPtr> thePageWalkReissues;
    std::list<TranslationPtr> thePageWalkRequests;
    eConsistencyModel theConsistencyModel;
    uint64_t theCoherenceUnit;
    uint32_t thePartialSnoopersOutstanding;

    // Spin detection and control
    uint64_t theSustainedIPCCycles;
    uint64_t theKernelPanicCount;
    uint64_t theInterruptReceived;

    // Spin detection and control
    bool theSpinning;
    std::set<PhysicalMemoryAddress> theSpinMemoryAddresses;
    std::set<VirtualMemoryAddress> theSpinPCs;
    uint32_t theSpinDetectCount;
    uint32_t theSpinRetireSinceReset;
    uint64_t theSpinStartCycle;
    bool theSpinControlEnabled;

    // Outsanding prefetch tracking
    bool thePrefetchEarly;
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>> theOutstandingStorePrefetches;

    std::map<PhysicalMemoryAddress, std::pair<int, bool>> theSBLines_Permission;
    int32_t theSBLines_Permission_falsecount;

    // Speculation control
    bool theSpeculativeOrder;
    bool theSpeculateOnAtomicValue;
    bool theSpeculateOnAtomicValuePerfect;
    bool theValuePredictInhibit;
    bool theIsSpeculating;
    bool theIsIdle;
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint> theCheckpoints;
    boost::intrusive_ptr<Instruction> theOpenCheckpoint;
    int32_t theRetiresSinceCheckpoint;
    int32_t theAllowedSpeculativeCheckpoints;
    int32_t theCheckpointThreshold;
    bool theAbortSpeculation;
    int32_t theTSOBReplayStalls;
    boost::intrusive_ptr<Instruction> theViolatingInstruction;
    SpeculativeLoadAddressTracker theSLAT;
    Stat::StatCounter theSLATHits_Load;
    Stat::StatCounter theSLATHits_Store;
    Stat::StatCounter theSLATHits_Atomic;
    Stat::StatCounter theSLATHits_AtomicAvoided;
    Stat::StatCounter theValuePredictions;
    Stat::StatCounter theValuePredictions_Successful;
    Stat::StatCounter theValuePredictions_Failed;

    Stat::StatCounter theDG_HitSB;
    Stat::StatCounter theInv_HitSB;
    Stat::StatCounter theRepl_HitSB;

    bool theEarlySGP;              /* CMU-ONLY */
    bool theTrackParallelAccesses; /* CMU-ONLY */
  public:
    bool theInOrderMemory;
    bool theInOrderExecute;

  private:
    bool theIdleThisCycle;
    int32_t theIdleCycleCount;
    BBVTracker* theBBVTracker; /* CMU-ONLY */
    uint32_t theOnChipLatency;
    uint32_t theOffChipLatency;
    //  bool theValidateMMU;

  public:
    std::list<boost::intrusive_ptr<MemOp>> theMemoryPorts;
    uint32_t theNumMemoryPorts;
    std::list<boost::intrusive_ptr<MemOp>> theSnoopPorts;
    uint32_t theNumSnoopPorts;

    std::queue<TranslationPtr> theTranslationQueue;

  private:
    std::list<boost::intrusive_ptr<MemOp>> theMemoryReplies;

    // Stats
    uint64_t theMispredictCycles;
    uint64_t theMispredictCount;

    uint64_t theCommitNumber;
    Stat::StatCounter theCommitCount;

    Stat::StatCounter theCommitCount_NonSpin_User;
    Stat::StatCounter theCommitCount_NonSpin_System;
    Stat::StatCounter theCommitCount_NonSpin_Trap;
    Stat::StatCounter theCommitCount_NonSpin_Idle;
    Stat::StatCounter theCommitCount_Spin_User;
    Stat::StatCounter theCommitCount_Spin_System;
    Stat::StatCounter theCommitCount_Spin_Trap;
    Stat::StatCounter theCommitCount_Spin_Idle;
    Stat::StatCounter* theCommitUSArray[8];
    std::vector<Stat::StatCounter*> theCommitsByCode[4];

    Stat::StatInstanceCounter<int64_t> theLSQOccupancy;
    Stat::StatInstanceCounter<int64_t> theSBOccupancy;
    Stat::StatInstanceCounter<int64_t> theSBUniqueOccupancy;
    Stat::StatInstanceCounter<int64_t> theSBNAWOccupancy;

    Stat::StatCounter theSpinCount;
    Stat::StatCounter theSpinCycles;

    Stat::StatCounter theStorePrefetches;
    Stat::StatCounter theAtomicPrefetches;
    Stat::StatCounter theStorePrefetchConflicts;
    Stat::StatCounter theStorePrefetchDuplicates;

    Stat::StatCounter thePartialSnoopLoads;

    Stat::StatCounter theRaces;
    Stat::StatCounter theRaces_LoadReplayed_User;
    Stat::StatCounter theRaces_LoadReplayed_System;
    Stat::StatCounter theRaces_LoadForwarded_User;
    Stat::StatCounter theRaces_LoadForwarded_System;
    Stat::StatCounter theRaces_LoadAlreadyOrdered_User;
    Stat::StatCounter theRaces_LoadAlreadyOrdered_System;

    Stat::StatCounter theSpeculations_Started;
    Stat::StatCounter theSpeculations_Successful;
    Stat::StatCounter theSpeculations_Rollback;
    Stat::StatCounter theSpeculations_PartialRollback;
    Stat::StatCounter theSpeculations_Resync;

    Stat::StatInstanceCounter<int64_t> theSpeculativeCheckpoints;
    Stat::StatMax theMaxSpeculativeCheckpoints;

    Stat::StatInstanceCounter<int64_t> theROBOccupancyTotal;
    Stat::StatInstanceCounter<int64_t> theROBOccupancySpin;
    Stat::StatInstanceCounter<int64_t> theROBOccupancyNonSpin;
    Stat::StatCounter theSideEffectOnChip;
    Stat::StatCounter theSideEffectOffChip;

    Stat::StatCounter theNonSpeculativeAtomics;
    Stat::StatCounter theRequiredDiscards;
    Stat::StatLog2Histogram theRequiredDiscards_Histogram;
    Stat::StatCounter theNearestCkptDiscards;
    Stat::StatLog2Histogram theNearestCkptDiscards_Histogram;
    Stat::StatCounter theSavedDiscards;
    Stat::StatLog2Histogram theSavedDiscards_Histogram;

    Stat::StatInstanceCounter<int64_t> theCheckpointsDiscarded;
    Stat::StatInstanceCounter<int64_t> theRollbackToCheckpoint;

    Stat::StatCounter theWP_CoherenceMiss;
    Stat::StatCounter theWP_SVBHit;

    Stat::StatCounter theResync_GarbageCollect, theResync_AbortedSpec, theResync_BlackBox, theResync_MAGIC,
      theResync_ITLBMiss, theResync_UnimplementedAnnul, theResync_RDPRUnsupported, theResync_WRPRUnsupported,
      theResync_MEMBARSync, theResync_UnexpectedException, theResync_Interrupt, theResync_DeviceAccess,
      theResync_FailedValidation, theResync_FailedHandleTrap, theResync_SideEffectLoad, theResync_SideEffectStore,
      theResync_Unknown, theResync_CPUHaltedState, theFalseITLBMiss;

    MemOpCounter* theMemOpCounters[2][2][8];
    Stat::StatCounter* theEpochEnd[2][8];
    Stat::StatCounter theEpochs;
    Stat::StatLog2Histogram theEpochs_Instructions;
    Stat::StatAverage theEpochs_Instructions_Avg;
    Stat::StatStdDev theEpochs_Instructions_StdDev;
    Stat::StatLog2Histogram theEpochs_Loads;
    Stat::StatAverage theEpochs_Loads_Avg;
    Stat::StatStdDev theEpochs_Loads_StdDev;
    Stat::StatLog2Histogram theEpochs_Stores;
    Stat::StatAverage theEpochs_Stores_Avg;
    Stat::StatStdDev theEpochs_Stores_StdDev;
    Stat::StatLog2Histogram theEpochs_OffChipStores;
    Stat::StatAverage theEpochs_OffChipStores_Avg;
    Stat::StatStdDev theEpochs_OffChipStores_StdDev;
    Stat::StatLog2Histogram theEpochs_StoreAfterOffChipStores;
    Stat::StatAverage theEpochs_StoreAfterOffChipStores_Avg;
    Stat::StatStdDev theEpochs_StoreAfterOffChipStores_StdDev;
    Stat::StatLog2Histogram theEpochs_StoresOutstanding;
    Stat::StatAverage theEpochs_StoresOutstanding_Avg;
    Stat::StatStdDev theEpochs_StoresOutstanding_StdDev;
    Stat::StatInstanceCounter<int64_t> theEpochs_StoreBlocks;
    Stat::StatInstanceCounter<int64_t> theEpochs_LoadOrStoreBlocks;
    uint64_t theEpoch_StartInsn;
    uint64_t theEpoch_LoadCount;
    uint64_t theEpoch_StoreCount;
    uint64_t theEpoch_OffChipStoreCount;
    uint64_t theEpoch_StoreAfterOffChipStoreCount;
    std::set<uint64_t> theEpoch_StoreBlocks;
    std::set<uint64_t> theEpoch_LoadOrStoreBlocks;

    // Accounting Stats
    enum eEmptyROBCause
    {
        kIStall_PeerL1,
        kIStall_L2,
        kIStall_PeerL2,
        kIStall_L3,
        kIStall_Mem,
        kIStall_Other,
        kMispredict,
        kSync,
        kResync,
        kFailedSpeculation,
        kRaisedException,
        kRaisedInterrupt
    } theEmptyROBCause;

    // fixme
    /*
      std::ostream & operator << (std::ostream & anOstream, eEmptyROBCause aType)
      { const char * const name[] = { "kIStall_PeerL1" , "kIStall_L2" ,
      "kIStall_Mem" , "kIStall_Other" , "kMispredict" , "kSync" , "kResync" ,
      "kFailedSpeculation" , "kRaisedException" , "kRaisedInterrupt"
        };
        anOstream << name[aType];
        return anOstream;
      }
    */

    uint32_t theLastCycleIStalls; // Used to fix accounting error when sync
                                  // instructions are dispatched

    uint32_t theRetireCount; // Count retirements each cycle

    TimeBreakdown theTimeBreakdown;
    nXactTimeBreakdown::eCycleClass theLastStallCause;
    int32_t theCycleCategory;
    int32_t kTBUser;
    int32_t kTBSystem;
    int32_t kTBTrap;
    int32_t kTBIdle;

    Stat::StatCounter theMix_Total;
    Stat::StatCounter theMix_Exception;
    Stat::StatCounter theMix_Load;
    Stat::StatCounter theMix_Store;
    Stat::StatCounter theMix_Atomic;
    Stat::StatCounter theMix_Branch;
    Stat::StatCounter theMix_MEMBAR;
    Stat::StatCounter theMix_Computation;
    Stat::StatCounter theMix_Synchronizing;

    Stat::StatCounter theAtomicVal_RMWs;
    Stat::StatCounter theAtomicVal_RMWs_Zero;
    Stat::StatCounter theAtomicVal_RMWs_NonZero;
    Stat::StatCounter theAtomicVal_CASs;
    Stat::StatCounter theAtomicVal_CASs_Mismatch;
    Stat::StatCounter theAtomicVal_CASs_MismatchAfterMismatch;
    Stat::StatCounter theAtomicVal_CASs_Match;
    Stat::StatCounter theAtomicVal_CASs_MatchAfterMismatch;
    Stat::StatCounter theAtomicVal_CASs_Zero;
    Stat::StatCounter theAtomicVal_CASs_NonZero;
    bool theAtomicVal_LastCASMismatch;

    Stat::StatCounter theCoalescedStores;

    uint32_t intAluOpLatency;
    uint32_t intAluOpPipelineResetTime;

    uint32_t intMultOpLatency;
    uint32_t intMultOpPipelineResetTime;
    uint32_t intDivOpLatency;
    uint32_t intDivOpPipelineResetTime;

    uint32_t fpAddOpLatency;
    uint32_t fpAddOpPipelineResetTime;
    uint32_t fpCmpOpLatency;
    uint32_t fpCmpOpPipelineResetTime;
    uint32_t fpCvtOpLatency;
    uint32_t fpCvtOpPipelineResetTime;

    uint32_t fpMultOpLatency;
    uint32_t fpMultOpPipelineResetTime;
    uint32_t fpDivOpLatency;
    uint32_t fpDivOpPipelineResetTime;
    uint32_t fpSqrtOpLatency;
    uint32_t fpSqrtOpPipelineResetTime;

    // Cycles until each FU becomes ready to accept a new operation
    std::vector<uint32_t> intAluCyclesToReady;
    std::vector<uint32_t> intMultCyclesToReady;
    std::vector<uint32_t> fpAluCyclesToReady;
    std::vector<uint32_t> fpMultCyclesToReady;

    /* Msutherl: Additions for RPCProc */
    bool collectTrace;
    std::string trace_fname;
    std::ofstream trace_stream;

    // CONSTRUCTION
    //==========================================================================
  public:
    CoreImpl(uArchOptions_t options,
             std::function<int(bool)> advance,
             std::function<void(eSquashCause)> squash,
             std::function<void(VirtualMemoryAddress)> redirect,
             std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
             std::function<void(bool)> signalStoreForwardingHit,
             std::function<void(int32_t)> mmuResync);

    virtual ~CoreImpl() {}

    // Cycle Processing
    //==========================================================================
  public:
    void skipCycle();
    void cycle(eExceptionType aPendingInterrupt);
    void dumpState(Flexus::SharedTypes::CPU_State&);
    bool checkValidatation();

  private:
    void prepareCycle();
    void arbitrate();
    void evaluate();
    void endCycle();
    void satisfy(InstructionDependance const& aDep);
    void squash(InstructionDependance const& aDep);
    void satisfy(std::list<InstructionDependance>& dependances);
    void squash(std::list<InstructionDependance>& dependances);

    void clearExclusiveLocal();
    void clearExclusiveGlobal();
    void markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker);
    void markExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker);
    void markExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize, uint64_t marker);

    int isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize);
    int isExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize);
    int isExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize);
    bool isROBHead(boost::intrusive_ptr<Instruction> anInstruction);

    // Instruction completion
    //==========================================================================
  public:
    bool sbEmpty() const { return theSBCount == 0; }
    bool sbFull() const
    {
        if (theSBSize == 0) { return false; }
        if (theConsistencyModel == kRMO) {
            return theSBLines_Permission_falsecount >= theSBSize;
        } else {
            return (theSBCount >= theSBSize ||
                    (theSpeculativeOrder ? (static_cast<int>(theSBLines_Permission.size())) > theSBSize / 8 : false));
        }
    }
    bool mayRetire_MEMBARStSt() const;
    bool mayRetire_MEMBARStLd() const;
    bool mayRetire_MEMBARSync() const;

    void spinDetect(memq_t::index<by_insn>::type::iterator);
    void retireMem(boost::intrusive_ptr<Instruction> aCorrespondingInstruction);
    void checkPageFault(boost::intrusive_ptr<Instruction> anInsn);
    void commitStore(boost::intrusive_ptr<Instruction> aCorrespondingInstruction);
    bool checkStoreRetirement(boost::intrusive_ptr<Instruction> aStore);
    void accessMem(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn);
    bool isSpinning() const { return theSpinning; }
    uint32_t outstandingStorePrefetches() const { return theOutstandingStorePrefetches.size(); }

  private:
    void retire();
    void commit();
    void commit(boost::intrusive_ptr<Instruction> anInstruction);
    bool acceptInterrupt();

    // Retirement accounting
    //==========================================================================
    void accountRetire(boost::intrusive_ptr<Instruction> anInst);
    nXactTimeBreakdown::eCycleClass getStoreStallType(boost::intrusive_ptr<TransactionTracker> tracker);
    void chargeStoreStall(boost::intrusive_ptr<Instruction> inst, boost::intrusive_ptr<TransactionTracker> tracker);
    void accountCommit(boost::intrusive_ptr<Instruction> anInst, bool aRaised);
    void completeAccounting();
    void accountAbortSpeculation(uint64_t aCheckpointSequenceNumber);
    void accountStartSpeculation();
    void accountEndSpeculation();
    void accountResyncSpeculation();
    void accountResyncReason(boost::intrusive_ptr<Instruction> anInstruction);
    void accountResync();
    void prepareMemOpAccounting();
    void accountCommitMemOp(boost::intrusive_ptr<Instruction> insn);

    // Squashing & Front-end control
    //==========================================================================
  public:
    bool squashFrom(boost::intrusive_ptr<Instruction> anInsn);
    void redirectFetch(VirtualMemoryAddress anAddress);
    void branchFeedback(boost::intrusive_ptr<BranchFeedback> feedback);

    void takeTrap(boost::intrusive_ptr<Instruction> anInsn, eExceptionType aTrapType);
    void handleTrap();

  private:
    void doSquash();
    void doAbortSpeculation();

    // Dispatch
    //==========================================================================
  public:
    int32_t availableROB() const;
    const uint32_t core() const;
    bool isSynchronized() const { return theROB.empty(); }
    bool isStalled() const;
    bool isHalted() const;
    int32_t iCount() const;
    bool isQuiesced() const
    {
        return theROB.empty() && theBranchFeedback.empty() && theMemQueue.empty() && theMSHRs.empty() &&
               theMemoryPortArbiter.empty() && theMemoryPorts.empty() && theSnoopPorts.empty() &&
               theMemoryReplies.empty() && theActiveActions.empty() && theRescheduledActions.empty() &&
               !theSquashRequested && !theRedirectRequested;
    }

    bool isFullyStalled() const { return theIdleCycleCount > 5; }
    void dispatch(boost::intrusive_ptr<Instruction> anInsn);
    void applyToNext(boost::intrusive_ptr<Instruction> anInsn, boost::intrusive_ptr<Interaction> anInteraction);
    void deferInteraction(boost::intrusive_ptr<Instruction> anInsn, boost::intrusive_ptr<Interaction> anInteraction);

    // Semantic Action & Effect scheduling
    //==========================================================================
  public:
    void create(boost::intrusive_ptr<SemanticAction> anAction);
    void reschedule(boost::intrusive_ptr<SemanticAction> anAction);

    // Bypass Network Interface
    //==========================================================================
    void bypass(mapped_reg aReg, register_value aValue);
    void connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst, std::function<bool(register_value)> fn);

    // Register File Interface
    //==========================================================================
  public:
    void mapRegister(mapped_reg aRegister);
    void unmapRegister(mapped_reg aRegister);
    eResourceStatus requestRegister(mapped_reg aRegister, InstructionDependance const& aDependance);
    eResourceStatus requestRegister(mapped_reg aRegister);
    void squashRegister(mapped_reg aRegister);
    register_value readRegister(mapped_reg aRegister);
    register_value readArchitecturalRegister(reg aRegister, bool aRotate);
    void writeRegister(mapped_reg aRegister, register_value aValue, bool isW);
    void disconnectRegister(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst);
    void initializeRegister(mapped_reg aRegister, register_value aValue);
    void copyRegValue(mapped_reg aSource, mapped_reg aDest);

    // Mapping Table Interface
    //==========================================================================
  private:
    PhysicalMap& mapTable(eRegisterType aMapTable);

  public:
    mapped_reg map(reg aReg);
    std::pair<mapped_reg, mapped_reg> create(reg aReg);
    void free(mapped_reg aReg);
    void restore(reg aName, mapped_reg aReg);

    // Synchronization with Qemu
    //==========================================================================
  public:
    void resetCore();
    void reset();
    void getState(State& aState);
    void restoreState(State& aState);
    void compareState(State& aLeft, State& aRight);
    bool isIdleLoop();
    uint64_t pc() const;
    bool isAARCH64();
    void setAARCH64(bool aMode);
    uint32_t getSPSel();
    void setSPSel(uint32_t aVal);
    void setSP_el(uint8_t anEL, uint64_t aVal);
    uint64_t getSP_el(uint8_t anEL);
    uint32_t getPSTATE();
    void setPSTATE(uint32_t aPSTATE);
    uint64_t getTPIDR(uint8_t anEL);
    void setFPSR(uint64_t anFPSR);
    uint64_t getFPSR();
    void setFPCR(uint64_t anFPCR);
    uint64_t getFPCR();
    uint32_t getDCZID_EL0();
    void setDCZID_EL0(uint32_t aDCZID_EL0);
    void setSCTLR_EL(uint8_t anId, uint64_t aSCTLR_EL);
    uint64_t getSCTLR_EL(uint8_t anId);
    void setHCREL2(uint64_t aHCREL2);
    uint64_t getHCREL2();
    void setException(Flexus::Qemu::API::exception_t anEXP);
    Flexus::Qemu::API::exception_t getException();
    void setRoundingMode(uint32_t aRoundingMode);
    uint32_t getRoundingMode();
    void writePR(uint32_t aPR, uint64_t aVal);
    uint64_t readPR(ePrivRegs aPR);
    void setXRegister(uint32_t aReg, uint64_t aVal);
    uint64_t getXRegister(uint32_t aReg);
    void setPC(uint64_t aPC);
    void setELR_el(uint8_t anEL, uint64_t aVal);
    uint64_t getELR_el(uint8_t anEL);
    void setSPSR_el(uint8_t anEL, uint64_t aVal);
    uint64_t getSPSR_el(uint8_t anEL);
    void setDAIF(uint32_t aDAIF);

    /* Msutherl: API to read system register value using QEMU encoding */
    uint64_t readUnhashedSysReg(uint8_t opc0, uint8_t opc1, uint8_t opc2, uint8_t crn, uint8_t crm);

    // Msutherl: State variable indicating this has no work to do. Set by QEMU upon instruction retire
    bool cpuHalted;

    // Interface to Memory Unit
    //==========================================================================
    void breakMSHRLink(memq_t::index<by_insn>::type::iterator iter);
    eConsistencyModel consistencyModel() const { return theConsistencyModel; }
    bool speculativeConsistency() const { return theSpeculativeOrder; }
    void insertLSQ(boost::intrusive_ptr<Instruction> anInsn,
                   eOperation anOperation,
                   eSize aSize,
                   bool aBypassSB,
                   eAccType type);
    void insertLSQ(boost::intrusive_ptr<Instruction> anInsn,
                   eOperation anOperation,
                   eSize aSize,
                   bool aBypassSB,
                   InstructionDependance const& aDependance,
                   eAccType type);
    void eraseLSQ(boost::intrusive_ptr<Instruction> anInsn);
    void cleanMSHRS(uint64_t aDiscardAfterSequenceNum);
    void clearLSQ();
    void clearSSB();
    int32_t clearSSB(uint64_t aStopAtInsnSeq);
    void pushMemOp(boost::intrusive_ptr<MemOp> anOp);
    bool canPushMemOp();
    boost::intrusive_ptr<MemOp> popMemOp();
    boost::intrusive_ptr<MemOp> popSnoopOp();
    TranslationPtr popTranslation();
    void pushTranslation(TranslationPtr aTranslation);
    uint32_t currentEL();
    void increaseEL();
    void decreaseEL();

    void invalidateCache(eCacheType aType, eShareableDomain aDomain, eCachePoint aPoint);
    void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, eCachePoint aPoint);
    void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, uint32_t aSize, eCachePoint aPoint);

    eAccessResult accessZVA();
    uint32_t readDCZID_EL0();

    bool _SECURE();
    SCTLR_EL _SCTLR(uint32_t anELn);
    PSTATE _PSTATE();

    void SystemRegisterTrap(uint8_t target_el,
                            uint8_t op0,
                            uint8_t op2,
                            uint8_t op1,
                            uint8_t crn,
                            uint8_t rt,
                            uint8_t crm,
                            uint8_t dir);

  private:
    bool hasSnoopBuffer() const { return theSnoopPorts.size() < theNumSnoopPorts; }

  public:
    bool hasMemPort(uint32_t sent) const { return sent < theNumMemoryPorts; }
    eInstructionClass getROBHeadClass() const
    {
        eInstructionClass rob_head = clsComputation;
        if (!theROB.empty()) { rob_head = theROB.front()->instClass(); }
        return rob_head;
    }

    // Checkpoint processing
    //==========================================================================
  public:
    void createCheckpoint(boost::intrusive_ptr<Instruction> anInsn);
    void freeCheckpoint(boost::intrusive_ptr<Instruction> anInsn);
    void requireWritePermission(memq_t::index<by_insn>::type::iterator aWrite);
    void unrequireWritePermission(PhysicalMemoryAddress anAddress);
    void acquireWritePermission(PhysicalMemoryAddress anAddress);
    void loseWritePermission(eLoseWritePermission aReason, PhysicalMemoryAddress anAddress);
    bool isSpeculating() const { return theIsSpeculating; }
    void startSpeculating();
    void checkStopSpeculating();
    void resolveCheckpoint();

    // Invalidation processing
    //==========================================================================
  public:
    void invalidate(PhysicalMemoryAddress anAddress);
    void addSLATEntry(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn);
    void removeSLATEntry(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn);

    // Address and Value resolution for Loads and Stores
    //==========================================================================
  public:
    bits retrieveLoadValue(boost::intrusive_ptr<Instruction> anInsn);
    void setLoadValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue);
    bits retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction> anInsn);
    void resolveVAddr(boost::intrusive_ptr<Instruction> anInsn, VirtualMemoryAddress anAddr);
    void translate(boost::intrusive_ptr<Instruction> anInsn);
    void resolvePAddr(boost::intrusive_ptr<Instruction> anInsn, PhysicalMemoryAddress anAddr);
    void updateStoreValue(boost::intrusive_ptr<Instruction> anInsn,
                          bits aValue,
                          boost::optional<bits> anExtendedValue = boost::none);
    void annulStoreValue(boost::intrusive_ptr<Instruction> anInsn);
    void updateCASValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue, bits aCMPValue);

    // Value forwarding
    //==========================================================================
    void forwardValue(MemQueueEntry const& aStore, memq_t::index<by_insn>::type::iterator aLoad);
    template<class Iter>
    boost::optional<memq_t::index<by_insn>::type::iterator> snoopQueue(Iter iter,
                                                                       Iter end,
                                                                       memq_t::index<by_insn>::type::iterator load);
    boost::optional<memq_t::index<by_insn>::type::iterator> snoopStores(
      memq_t::index<by_insn>::type::iterator aLoad,
      boost::optional<memq_t::index<by_insn>::type::iterator> aCachedSnoopState);
    void updateDependantLoads(memq_t::index<by_insn>::type::iterator anUpdatedStore);
    void applyStores(memq_t::index<by_paddr>::type::iterator aFirstStore,
                     memq_t::index<by_paddr>::type::iterator aLastStore,
                     memq_t::index<by_insn>::type::iterator aLoad);
    void applyAllStores(memq_t::index<by_insn>::type::iterator aLoad);

    // Memory request processing
    //===========================================================================
    void issueStore();
    void issuePartialSnoop();
    void issueAtomic();
    void issueAtomicSpecWrite();
    void valuePredictAtomic();
    void issueSpecial();
    void checkExtraLatencyTimeout();
    void doStore(memq_t::index<by_insn>::type::iterator lsq_entry);
    void resnoopDependantLoads(memq_t::index<by_insn>::type::iterator lsq_entry);
    boost::optional<memq_t::index<by_insn>::type::iterator> doLoad(
      memq_t::index<by_insn>::type::iterator lsq_entry,
      boost::optional<memq_t::index<by_insn>::type::iterator> aCachedSnoopState);
    void updateVaddr(memq_t::index<by_insn>::type::iterator lsq_entry,
                     VirtualMemoryAddress anAddr /*, int32_t anASI */);
    void updatePaddr(memq_t::index<by_insn>::type::iterator lsq_entry, PhysicalMemoryAddress anAddr);

    // MSHR Management
    //===========================================================================
    bool scanAndAttachMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry);
    bool scanAndBlockMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry);
    bool scanAndBlockPrefetch(memq_t::index<by_insn>::type::iterator anLSQEntry);
    bool scanMSHR(memq_t::index<by_insn>::type::iterator anLSQEntry);
    void requestPort(memq_t::index<by_insn>::type::iterator anLSQEntry);
    void requestPort(boost::intrusive_ptr<Instruction> anInstruction);
    void issue(boost::intrusive_ptr<Instruction> anInstruction);
    void issueMMU(TranslationPtr aTranslation);
    void issueStorePrefetch(boost::intrusive_ptr<Instruction> anInstruction);
    void requestStorePrefetch(boost::intrusive_ptr<Instruction> anInstruction);
    void requestStorePrefetch(memq_t::index<by_insn>::type::iterator lsq_entry);
    void killStorePrefetches(boost::intrusive_ptr<Instruction> anInstruction);

    // Memory reply Processing
    //==========================================================================
    void processMemoryReplies();
    void ackInvalidate(MemOp const& anInvalidate);
    void ackDowngrade(MemOp const& aDowngrade);
    void ackProbe(MemOp const& aProbe);
    void ackReturn(MemOp const& aReturn);
    bool processReply(MemOp const& anOperation);
    bool satisfies(eOperation aResponse, eOperation anOperation);
    void complete(MemOp const& anOperation);
    void completeLSQ(memq_t::index<by_insn>::type::iterator lsq_entry, MemOp const& anOperation);

    void startMiss(boost::intrusive_ptr<TransactionTracker> tracker);
    void finishMiss(boost::intrusive_ptr<TransactionTracker> tracker, bool matched_mshr);
    void processTable();

    // Debugging
    //==========================================================================
  public:
    void printROB();
    void printSRB();
    void printMemQueue();
    void printMSHR();
    void printRegMappings(std::string);
    void printRegFreeList(std::string);
    void printRegReverseMappings(std::string);
    void printAssignments(std::string);

  private:
    void dumpROB();
    void dumpSRB();
    void dumpMemQueue();
    void dumpMSHR();
    void dumpActions();
    void dumpCheckpoints();
    void dumpSBPermissions();
};

} // namespace nuArch
