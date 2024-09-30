
#include "coreModelImpl.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

CoreImpl::CoreImpl(uArchOptions_t options,
                   std::function<int(bool)> _advance,
                   std::function<void(eSquashCause)> _squash,
                   std::function<void(VirtualMemoryAddress)> _redirect,
                   std::function<void(boost::intrusive_ptr<BranchFeedback>)> _feedback,
                   std::function<void(bool)> _signalStoreForwardingHit,
                   std::function<void(int32_t)> _mmuResync)
  : theName(options.name)
  , theNode(options.node)
  ,
  //, translate(xlat)
  advance_fn(_advance)
  , squash_fn(_squash)
  , redirect_fn(_redirect)
  , feedback_fn(_feedback)
  , signalStoreForwardingHit_fn(_signalStoreForwardingHit)
  , mmuResync_fn(_mmuResync)
  , thePendingTrap(kException_None)
  , theBypassNetwork(kxRegs_Total + 3 * options.ROBSize, kvRegs + 4 * options.ROBSize, kccRegs + 2 * options.ROBSize)
  , theLastGarbageCollect(0)
  , thePreserveInteractions(false)
  , theMemoryPortArbiter(*this, options.numMemoryPorts, options.numStorePrefetches)
  , theROBSize(options.ROBSize)
  , theRetireWidth(options.retireWidth)
  , theInterruptSignalled(false)
  , thePendingInterrupt(kException_None)
  , theInterruptInstruction(0)
  , theMemorySequenceNum(0)
  , theLSQCount(0)
  , theSBCount(0)
  , theSBNAWCount(0)
  , theSBSize(options.SBSize)
  , theNAWBypassSB(options.NAWBypassSB)
  , theNAWWaitAtSync(options.NAWWaitAtSync)
  , theConsistencyModel(options.consistencyModel)
  , theCoherenceUnit(options.coherenceUnit)
  , theSustainedIPCCycles(0)
  , theKernelPanicCount(0)
  , theSpinning(false)
  , theSpinDetectCount(0)
  , theSpinRetireSinceReset(0)
  , theSpinControlEnabled(options.spinControlEnabled)
  , thePrefetchEarly(options.prefetchEarly)
  , theSBLines_Permission_falsecount(0)
  , theSpeculativeOrder(options.speculativeOrder)
  , theSpeculateOnAtomicValue(options.speculateOnAtomicValue)
  , theSpeculateOnAtomicValuePerfect(options.speculateOnAtomicValuePerfect)
  , theValuePredictInhibit(false)
  , theIsSpeculating(false)
  , theIsIdle(false)
  , theRetiresSinceCheckpoint(0)
  , theAllowedSpeculativeCheckpoints(options.speculativeCheckpoints)
  , theCheckpointThreshold(options.checkpointThreshold)
  , theAbortSpeculation(false)
  , theTSOBReplayStalls(0)
  , theSLATHits_Load(theName + "-SLATHits:Load")
  , theSLATHits_Store(theName + "-SLATHits:Store")
  , theSLATHits_Atomic(theName + "-SLATHits:Atomic")
  , theSLATHits_AtomicAvoided(theName + "-SLATHits:AtomicAvoided")
  , theValuePredictions(theName + "-ValuePredict")
  , theValuePredictions_Successful(theName + "-ValuePredict:Success")
  , theValuePredictions_Failed(theName + "-ValuePredict:Fail")
  , theDG_HitSB(theName + "-SBLostPermission:Downgrade")
  , theInv_HitSB(theName + "-SBLostPermission:Invalidate")
  , theRepl_HitSB(theName + "-SBLostPermission:Replacement")
  , theEarlySGP(options.earlySGP)
  , /* CMU-ONLY */
  theTrackParallelAccesses(options.trackParallelAccesses)
  , /* CMU-ONLY */
  theInOrderMemory(options.inOrderMemory)
  , theInOrderExecute(options.inOrderExecute)
  , theIdleThisCycle(false)
  , theIdleCycleCount(0)
  , theBBVTracker(/*BBVTracker::createBBVTracker(aNode)*/ 0)
  , /* CMU-ONLY */
  theOnChipLatency(options.onChipLatency)
  , theOffChipLatency(options.offChipLatency)
  , //  , theValidateMMU(options.validateMMU)
  theNumMemoryPorts(options.numMemoryPorts)
  , theNumSnoopPorts(options.numSnoopPorts)
  , theMispredictCycles(0)
  , // for IStall and mispredict stats
  theMispredictCount(0)
  , // for IStall and mispredict stats
  theCommitNumber(0)
  , theCommitCount(theName + "-Commits")
  , theCommitCount_NonSpin_User(theName + "-Commits:NonSpin:User")
  , theCommitCount_NonSpin_System(theName + "-Commits:NonSpin:System")
  , theCommitCount_NonSpin_Trap(theName + "-Commits:NonSpin:Trap")
  , theCommitCount_NonSpin_Idle(theName + "-Commits:NonSpin:Idle")
  , theCommitCount_Spin_User(theName + "-Commits:Spin:User")
  , theCommitCount_Spin_System(theName + "-Commits:Spin:System")
  , theCommitCount_Spin_Trap(theName + "-Commits:Spin:Trap")
  , theCommitCount_Spin_Idle(theName + "-Commits:Spin:Idle")
  , theLSQOccupancy(theName + "-Occupancy:LSQ")
  , theSBOccupancy(theName + "-Occupancy:SB")
  , theSBUniqueOccupancy(theName + "-Occupancy:SB:UniqueBlocks")
  , theSBNAWOccupancy(theName + "-Occupancy:SBNAW")
  , theSpinCount(theName + "-Spins")
  , theSpinCycles(theName + "-SpinCycles")
  , theStorePrefetches(theName + "-StorePrefetches")
  , theAtomicPrefetches(theName + "-AtomicPrefetches")
  , theStorePrefetchConflicts(theName + "-StorePrefetchConflicts")
  , theStorePrefetchDuplicates(theName + "-StorePrefetchDuplicates")
  , thePartialSnoopLoads(theName + "-PartialSnoopLoads")
  , theRaces(theName + "-Races")
  , theRaces_LoadReplayed_User(theName + "-Races:LoadReplay:User")
  , theRaces_LoadReplayed_System(theName + "-Races:LoadReplay:System")
  , theRaces_LoadForwarded_User(theName + "-Races:LoadFwd:User")
  , theRaces_LoadForwarded_System(theName + "-Races:LoadFwd:System")
  , theRaces_LoadAlreadyOrdered_User(theName + "-Races:LoadOrderOK:User")
  , theRaces_LoadAlreadyOrdered_System(theName + "-Races:LoadOrderOK:System")
  , theSpeculations_Started(theName + "-Speculations:Started")
  , theSpeculations_Successful(theName + "-Speculations:Successful")
  , theSpeculations_Rollback(theName + "-Speculations:Rollback")
  , theSpeculations_PartialRollback(theName + "-Speculations:PartialRollback")
  , theSpeculations_Resync(theName + "-Speculations:Resync")
  , theSpeculativeCheckpoints(theName + "-SpeculativeCheckpoints")
  , theMaxSpeculativeCheckpoints(theName + "-SpeculativeCheckpoints:Max")
  , theROBOccupancyTotal(theName + "-ROB-Occupancy:Total")
  , theROBOccupancySpin(theName + "-ROB-Occupancy:Spin")
  , theROBOccupancyNonSpin(theName + "-ROB-Occupancy:NonSpin")
  , theSideEffectOnChip(theName + "-SideEffects:OnChip")
  , theSideEffectOffChip(theName + "-SideEffects:OffChip")
  , theNonSpeculativeAtomics(theName + "-NonSpeculativeAtomics")
  , theRequiredDiscards(theName + "-Discards:Required")
  , theRequiredDiscards_Histogram(theName + "-Discards:Required:Histogram")
  , theNearestCkptDiscards(theName + "-Discards:ToCkpt")
  , theNearestCkptDiscards_Histogram(theName + "-Discards:ToCkpt:Histogram")
  , theSavedDiscards(theName + "-Discards:Saved")
  , theSavedDiscards_Histogram(theName + "-Discards:Saved:Histogram")
  , theCheckpointsDiscarded(theName + "-RollbackOverNCheckpoints")
  , theRollbackToCheckpoint(theName + "-RollbackToCheckpoint")
  , theWP_CoherenceMiss(theName + "-WP:CoherenceMisses")
  , theWP_SVBHit(theName + "-WP:SVBHits")
  , theResync_GarbageCollect(theName + "-Resync:GarbageCollect")
  , theResync_AbortedSpec(theName + "-Resync:AbortedSpec")
  , theResync_BlackBox(theName + "-Resync:BlackBox")
  , theResync_MAGIC(theName + "-Resync:MAGIC")
  , theResync_ITLBMiss(theName + "-Resync:ITLBMiss")
  , theResync_UnimplementedAnnul(theName + "-Resync:UnimplementedAnnul")
  , theResync_RDPRUnsupported(theName + "-Resync:RDPRUnsupported")
  , theResync_WRPRUnsupported(theName + "-Resync:WRPRUnsupported")
  , theResync_MEMBARSync(theName + "-Resync:MEMBARSync")
  , theResync_UnexpectedException(theName + "-Resync:UnexpectedException")
  , theResync_Interrupt(theName + "-Resync:Interrupt")
  , theResync_DeviceAccess(theName + "-Resync:DeviceAccess")
  , theResync_FailedValidation(theName + "-Resync:FailedValidation")
  , theResync_FailedHandleTrap(theName + "-Resync:FailedHandleTrap")
  , theResync_SideEffectLoad(theName + "-Resync:SideEffectLoad")
  , theResync_SideEffectStore(theName + "-Resync:SideEffectStore")
  , theResync_Unknown(theName + "-Resync:Unknown")
  , theResync_CPUHaltedState(theName + "-Resync:CPUHalted")
  , theFalseITLBMiss(theName + "-FalseITLBMiss")
  , theEpochs(theName + "-MLPEpoch")
  , theEpochs_Instructions(theName + "-MLPEpoch:Instructions")
  , theEpochs_Instructions_Avg(theName + "-MLPEpoch:Instructions:Avg")
  , theEpochs_Instructions_StdDev(theName + "-MLPEpoch:Instructions:StdDev")
  , theEpochs_Loads(theName + "-MLPEpoch:Loads")
  , theEpochs_Loads_Avg(theName + "-MLPEpoch:Loads:Avg")
  , theEpochs_Loads_StdDev(theName + "-MLPEpoch:Loads:StdDev")
  , theEpochs_Stores(theName + "-MLPEpoch:Stores")
  , theEpochs_Stores_Avg(theName + "-MLPEpoch:Stores:Avg")
  , theEpochs_Stores_StdDev(theName + "-MLPEpoch:Stores:StdDev")
  , theEpochs_OffChipStores(theName + "-MLPEpoch:Stores:OffChip")
  , theEpochs_OffChipStores_Avg(theName + "-MLPEpoch:Stores:OffChip:Avg")
  , theEpochs_OffChipStores_StdDev(theName + "-MLPEpoch:Stores:OffChip:StdDev")
  , theEpochs_StoreAfterOffChipStores(theName + "-MLPEpoch:StoresAfterOffChipStore")
  , theEpochs_StoreAfterOffChipStores_Avg(theName + "-MLPEpoch:StoresAfterOffChipStore:Avg")
  , theEpochs_StoreAfterOffChipStores_StdDev(theName + "-MLPEpoch:StoresAfterOffChipStore:StdDev")
  , theEpochs_StoresOutstanding(theName + "-MLPEpoch:StoresOutstanding")
  , theEpochs_StoresOutstanding_Avg(theName + "-MLPEpoch:StoresOutstanding:Avg")
  , theEpochs_StoresOutstanding_StdDev(theName + "-MLPEpoch:StoresOutstanding:StdDev")
  , theEpochs_StoreBlocks(theName + "-MLPEpoch:StoreBlocks")
  , theEpochs_LoadOrStoreBlocks(theName + "-MLPEpoch:LoadOrStoreBlocks")
  , theEpoch_StartInsn(0)
  , theEpoch_LoadCount(0)
  , theEpoch_StoreCount(0)
  , theEpoch_OffChipStoreCount(0)
  , theEpoch_StoreAfterOffChipStoreCount(0)
  , theEmptyROBCause(kSync)
  , theLastCycleIStalls(0)
  , theRetireCount(0)
  , theTimeBreakdown(theName + "-TB")
  , theMix_Total(theName + "-Mix:Total")
  , theMix_Exception(theName + "-Mix:Exception")
  , theMix_Load(theName + "-Mix:Load")
  , theMix_Store(theName + "-Mix:Store")
  , theMix_Atomic(theName + "-Mix:Atomic")
  , theMix_Branch(theName + "-Mix:Branch")
  , theMix_MEMBAR(theName + "-Mix:MEMBAR")
  , theMix_Computation(theName + "-Mix:Computation")
  , theMix_Synchronizing(theName + "-Mix:Synchronizing")
  , theAtomicVal_RMWs(theName + "-AtmVal:RMW")
  , theAtomicVal_RMWs_Zero(theName + "-AtmVal:RMW:Zero")
  , theAtomicVal_RMWs_NonZero(theName + "-AtmVal:RMW:NonZero")
  , theAtomicVal_CASs(theName + "-AtmVal:CAS")
  , theAtomicVal_CASs_Mismatch(theName + "-AtmVal:CAS:Mismatch")
  , theAtomicVal_CASs_MismatchAfterMismatch(theName + "-AtmVal:CAS:Mismatch:AfterMismatch")
  , theAtomicVal_CASs_Match(theName + "-AtmVal:CAS:Match")
  , theAtomicVal_CASs_MatchAfterMismatch(theName + "-AtmVal:CAS:Match:AfterMismatch")
  , theAtomicVal_CASs_Zero(theName + "-AtmVal:CAS:Zero")
  , theAtomicVal_CASs_NonZero(theName + "-AtmVal:CAS:NonZero")
  , theAtomicVal_LastCASMismatch(false)
  , theCoalescedStores(theName + "-CoalescedStores")
  , intAluOpLatency(options.intAluOpLatency)
  , intAluOpPipelineResetTime(options.intAluOpPipelineResetTime)
  , intMultOpLatency(options.intMultOpLatency)
  , intMultOpPipelineResetTime(options.intMultOpPipelineResetTime)
  , intDivOpLatency(options.intDivOpLatency)
  , intDivOpPipelineResetTime(options.intDivOpPipelineResetTime)
  , fpAddOpLatency(options.fpAddOpLatency)
  , fpAddOpPipelineResetTime(options.fpAddOpPipelineResetTime)
  , fpCmpOpLatency(options.fpCmpOpLatency)
  , fpCmpOpPipelineResetTime(options.fpCmpOpPipelineResetTime)
  , fpCvtOpLatency(options.fpCvtOpLatency)
  , fpCvtOpPipelineResetTime(options.fpCvtOpPipelineResetTime)
  , fpMultOpLatency(options.fpMultOpLatency)
  , fpMultOpPipelineResetTime(options.fpMultOpPipelineResetTime)
  , fpDivOpLatency(options.fpDivOpLatency)
  , fpDivOpPipelineResetTime(options.fpDivOpPipelineResetTime)
  , fpSqrtOpLatency(options.fpSqrtOpLatency)
  , fpSqrtOpPipelineResetTime(options.fpSqrtOpPipelineResetTime)
  ,
  // Each FU starts ready to accept an operation
  intAluCyclesToReady(options.numIntAlu, 0)
  , intMultCyclesToReady(options.numIntMult, 0)
  , fpAluCyclesToReady(options.numFpAlu, 0)
  , fpMultCyclesToReady(options.numFpMult, 0)
  , trace_fname("core_" + std::to_string(theNode) + "_retinsts.txt")
{

    // Msutherl - for MMU verification. Remove when done
    theQEMUCPU = Flexus::Qemu::Processor::getProcessor(theNode);

    // original constructor continues here...
    prepareMemOpAccounting();

    std::vector<uint32_t> reg_file_sizes;
    reg_file_sizes.resize(kLastMapTableCode + 2);
    reg_file_sizes[xRegisters] = kxRegs_Total + 3 * theROBSize;
    reg_file_sizes[vRegisters] = kvRegs + 4 * theROBSize;
    reg_file_sizes[ccBits]     = kccRegs + 2 * theROBSize;
    theRegisters.initialize(reg_file_sizes);

    // Map table for xRegisters
    theMapTables.push_back(std::make_shared<PhysicalMap>(kxRegs_Total, reg_file_sizes[xRegisters]));

    // Map table for vRegisters
    theMapTables.push_back(std::make_shared<PhysicalMap>(kvRegs, reg_file_sizes[vRegisters]));

    // Map table for ccBits
    theMapTables.push_back(std::make_shared<PhysicalMap>(kccRegs, reg_file_sizes[ccBits]));

    reset();

    theCommitUSArray[0] = &theCommitCount_NonSpin_User;
    theCommitUSArray[1] = &theCommitCount_NonSpin_System;
    theCommitUSArray[2] = &theCommitCount_NonSpin_Trap;
    theCommitUSArray[3] = &theCommitCount_NonSpin_Idle;
    theCommitUSArray[4] = &theCommitCount_Spin_User;
    theCommitUSArray[5] = &theCommitCount_Spin_System;
    theCommitUSArray[6] = &theCommitCount_Spin_Trap;
    theCommitUSArray[7] = &theCommitCount_Spin_Idle;

    for (int32_t i = 0; i < codeLastCode; ++i) {
        std::stringstream user_name;
        user_name << theName << "-InsnCount:User:" << eInstructionCode(i);
        std::stringstream system_name;
        system_name << theName + "-InsnCount:System:" << eInstructionCode(i);
        std::stringstream trap_name;
        trap_name << theName + "-InsnCount:Trap:" << eInstructionCode(i);
        std::stringstream idle_name;
        idle_name << theName + "-InsnCount:Idle:" << eInstructionCode(i);

        theCommitsByCode[0].push_back(new Stat::StatCounter(user_name.str()));
        theCommitsByCode[1].push_back(new Stat::StatCounter(system_name.str()));
        theCommitsByCode[2].push_back(new Stat::StatCounter(trap_name.str()));
        theCommitsByCode[3].push_back(new Stat::StatCounter(idle_name.str()));
    }

    kTBUser   = theTimeBreakdown.addClass("User");
    kTBSystem = theTimeBreakdown.addClass("System");
    kTBTrap   = theTimeBreakdown.addClass("Trap");
    kTBIdle   = theTimeBreakdown.addClass("Idle");

    theLastStallCause = nXactTimeBreakdown::kUnknown;
    theCycleCategory  = kTBUser;

    cpuHalted = false;

    if (collectTrace) { trace_stream = std::ofstream(trace_fname, std::ofstream::out | std::ofstream::trunc); }
}

void
CoreImpl::resetCore()
{

    mapTable(xRegisters).reset();
    mapTable(vRegisters).reset();
    mapTable(ccBits).reset();
    theRegisters.reset();

    theRoundingMode = 0;
    thePSTATE       = 0;

    thePendingTrap     = kException_None;
    theTrapInstruction = 0;

    theInterruptSignalled   = false;
    thePendingInterrupt     = kException_None;
    theInterruptInstruction = 0;

    theBypassNetwork.reset();

    theActiveActions      = action_list_t();
    theRescheduledActions = action_list_t();

    theDispatchInteractions.clear();
    thePreserveInteractions = false;

    theROB.clear();

    theSquashRequested = false;
    theSquashReason    = eSquashCause(0);
    theSquashInclusive = false;

    theRedirectRequested = false;
    theRedirectPC        = VirtualMemoryAddress(0);
    theDumpPC            = VirtualMemoryAddress(0);

    clearLSQ();

    thePartialSnoopersOutstanding = 0;
}

void
CoreImpl::reset()
{
    FLEXUS_PROFILE();

    cleanMSHRS(0);

    resetCore();

    theSRB.clear();

    // theBranchFeedback is NOT cleared

    clearSSB();

    if (theIsSpeculating) {
        DBG_(Iface, (<< theName << " Speculation aborted by sync"));
        accountResyncSpeculation();
    } else {
        accountResync();
    }

    theSLAT.clear();
    theIsSpeculating = false;
    theIsIdle        = false;
    theCheckpoints.clear();
    theOpenCheckpoint         = 0;
    theRetiresSinceCheckpoint = 0;

    theAbortSpeculation = false;

    theKernelPanicCount = 0;

    if (theSpinning) { // Note: will be false on first call to reset()
        theSpinCount += theSpinDetectCount;
        theSpinning = false;
        theSpinCycles += theFlexus->cycleCount() - theSpinStartCycle;
    }

    theSpinning = false;
    theSpinMemoryAddresses.clear();
    theSpinPCs.clear();
    theSpinDetectCount      = 0;
    theSpinRetireSinceReset = 0;

    // theMemoryPorts and theSnoopPorts are not reset.
    // theMemoryReplies is not reset

    theIdleCycleCount = 0;
    theIdleThisCycle  = false;

    while (!theTranslationQueue.empty()) {
        theTranslationQueue.pop();
    }
}

// write to physical register
void
CoreImpl::writePR(uint32_t aPR, uint64_t aVal)
{
    switch (aPR) {
        case 6: // PSTATE
            setPSTATE(aVal);
            break;
        default: DBG_(Crit, (<< "Write of unimplemented PR: " << aPR)); break;
    }
}
// read physical register
uint64_t
CoreImpl::readPR(ePrivRegs aPR)
{
    return getPriv(aPR)->readfn(this);
}
bool
CoreImpl::isAARCH64()
{
    return theAARCH64;
}
void
CoreImpl::setAARCH64(bool aMode)
{
    theAARCH64 = aMode;
}
uint32_t
CoreImpl::getSPSel()
{
    return thePSTATE & PSTATE_SP;
}
void
CoreImpl::setSPSel(uint32_t aVal)
{
    std::unique_ptr<SysRegInfo> ri = getPriv(kSPSel);
    ri->writefn(this, aVal);
}
void
CoreImpl::setSP_el(uint8_t anId, uint64_t aVal)
{
    DBG_Assert(0 <= anId && anId < 4, (<< "Out of bound access with index = " << anId));
    DBG_Assert(anId < 2, (<< "Unhandled simulation of hypervisor mode"));
    theSP_el[anId] = aVal;
}
uint64_t
CoreImpl::getSP_el(uint8_t anId)
{
    DBG_Assert(0 <= anId && anId < 4, (<< "Out of bound access with index = " << anId));
    DBG_Assert(anId < 2, (<< "Unhandled simulation of hypervisor mode"));
    return theSP_el[anId];
}
void
CoreImpl::setSPSR_el(uint8_t anId, uint64_t aVal)
{
    DBG_Assert(0 <= anId && anId < 4, (<< "Out of bound access with index = " << anId));
    DBG_Assert(anId < 2, (<< "Unhandled simulation of hypervisor mode"));
    theSPSR_EL[anId] = aVal;
}
uint64_t
CoreImpl::getSPSR_el(uint8_t anId)
{
    DBG_Assert(0 <= anId && anId < 4, (<< "Out of bound access with index = " << anId));
    DBG_Assert(anId < 2, (<< "Unhandled simulation of hypervisor mode"));
    return theSPSR_EL[anId];
}
uint32_t
CoreImpl::getPSTATE()
{
    return thePSTATE;
}
void
CoreImpl::setPSTATE(uint32_t aPSTATE)
{
    thePSTATE = aPSTATE;
}
uint64_t
CoreImpl::getTPIDR(uint8_t anEL)
{
    return Flexus::Qemu::Processor::getProcessor(theNode).read_register(Flexus::Qemu::API::TPIDR, anEL);
}
void
CoreImpl::setFPSR(uint64_t anFPSR)
{
    theFPSR.set(anFPSR);
}
uint64_t
CoreImpl::getFPSR()
{
    return theFPSR.get();
}
void
CoreImpl::setFPCR(uint64_t anFPCR)
{
    theFPCR.set(anFPCR);
}
uint64_t
CoreImpl::getFPCR()
{
    return theFPCR.get();
}
uint32_t
CoreImpl::getDCZID_EL0()
{
    return theDCZID_EL0;
}
void
CoreImpl::setDCZID_EL0(uint32_t aDCZID_EL0)
{
    theDCZID_EL0 = aDCZID_EL0;
}
void
CoreImpl::setSCTLR_EL(uint8_t anId, uint64_t aSCTLR_EL)
{
    theSCTLR_EL[anId] = aSCTLR_EL;
}
uint64_t
CoreImpl::getSCTLR_EL(uint8_t anId)
{
    DBG_Assert(anId < 2, (<< "Unhandled simulation of hypervisor mode"));
    return theSCTLR_EL[anId];
}
void
CoreImpl::setHCREL2(uint64_t aHCREL2)
{
    theHCR_EL2 = aHCREL2;
}
uint64_t
CoreImpl::getHCREL2()
{
    DBG_Assert(false, (<< "Unhandled simulation of hypervisor mode"));
    return 0;
}
void
CoreImpl::setException(Flexus::Qemu::API::exception_t anEXP)
{
    theEXP = anEXP;
}
Flexus::Qemu::API::exception_t
CoreImpl::getException()
{
    return theEXP;
}
void
CoreImpl::setRoundingMode(uint32_t aRoundingMode)
{
    theRoundingMode = aRoundingMode;
}
uint32_t
CoreImpl::getRoundingMode()
{
    return theRoundingMode;
}
void
CoreImpl::setDAIF(uint32_t aDAIF)
{
    if (aDAIF == 0) { return; }
    thePSTATE = ((thePSTATE & ~PSTATE_DAIF) | (aDAIF & PSTATE_DAIF));
}
void
CoreImpl::setELR_el(uint8_t anEL, uint64_t aVal)
{
    theELR_EL[anEL] = aVal;
}
uint64_t
CoreImpl::getELR_el(uint8_t anEL)
{
    return theELR_EL[anEL];
}
void
CoreImpl::setPC(uint64_t aPC)
{
    thePC = aPC;
}

bool CoreImpl::isPrivileged()
{
    return currentEL() != 0;
}

CoreModel*
CoreModel::construct(uArchOptions_t options,
                     std::function<int(bool)> advance,
                     std::function<void(eSquashCause)> squash,
                     std::function<void(VirtualMemoryAddress)> redirect,
                     std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
                     std::function<void(bool)> signalStoreForwardingHit,
                     std::function<void(int32_t)> mmuResync)
{

    return new CoreImpl(options, advance, squash, redirect, feedback, signalStoreForwardingHit, mmuResync);
}

} // namespace nuArch
