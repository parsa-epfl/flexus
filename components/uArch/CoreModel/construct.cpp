#include "coreModelImpl.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

CoreImpl::CoreImpl( uArchOptions_t options
                    , std::function< void (Flexus::Qemu::Translation &, bool) > xlat
                    , std::function<int(bool)> _advance
                    , std::function< void(eSquashCause)> _squash
                    , std::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> _redirect
                    , std::function< void(int, int)> _change_mode
                    , std::function< void( boost::intrusive_ptr<BranchFeedback> )> _feedback
                    , std::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> ) > _notifyTMS /* CMU-ONLY */
                    , std::function< void( bool )> _signalStoreForwardingHit
                  )
  : theName(options.name)
  , theNode(options.node)
  , translate(xlat)
  , advance_fn(_advance)
  , squash_fn(_squash)
  , redirect_fn(_redirect)
  , change_mode_fn(_change_mode)
  , feedback_fn(_feedback)
  , notifyTMS_fn(_notifyTMS) /* CMU-ONLY */
  , signalStoreForwardingHit_fn(_signalStoreForwardingHit)
  , thePendingTrap(0)
  , thePopTLRequested(false)
  , theBypassNetwork( krRegs_Total + 2 * options.ROBSize, kfRegs + 4 * options.ROBSize, 5 + options.ROBSize)
  , theLastGarbageCollect(0)
  , thePreserveInteractions(false)
  , theMemoryPortArbiter(*this, options.numMemoryPorts, options.numStorePrefetches)
  , theROBSize(options.ROBSize)
  , theRetireWidth(options.retireWidth)
  , theInterruptSignalled(false)
  , thePendingInterrupt(0)
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
  , theSpeculativeOrder( options.speculativeOrder)
  , theSpeculateOnAtomicValue( options.speculateOnAtomicValue )
  , theSpeculateOnAtomicValuePerfect( options.speculateOnAtomicValuePerfect )
  , theValuePredictInhibit(false)
  , theIsSpeculating(false)
  , theIsIdle(false)
  , theRetiresSinceCheckpoint(0)
  , theAllowedSpeculativeCheckpoints( options.speculativeCheckpoints )
  , theCheckpointThreshold( options.checkpointThreshold )
  , theAbortSpeculation(false)
  , theTSOBReplayStalls(0)
  , theSLATHits_Load(theName  + "-SLATHits:Load" )
  , theSLATHits_Store(theName  + "-SLATHits:Store" )
  , theSLATHits_Atomic(theName  + "-SLATHits:Atomic" )
  , theSLATHits_AtomicAvoided(theName  + "-SLATHits:AtomicAvoided" )
  , theValuePredictions(theName  + "-ValuePredict" )
  , theValuePredictions_Successful(theName  + "-ValuePredict:Success" )
  , theValuePredictions_Failed(theName  + "-ValuePredict:Fail" )
  , theDG_HitSB(theName  + "-SBLostPermission:Downgrade" )
  , theInv_HitSB(theName  + "-SBLostPermission:Invalidate" )
  , theRepl_HitSB(theName  + "-SBLostPermission:Replacement" )
  , theEarlySGP(options.earlySGP)   /* CMU-ONLY */
  , theTrackParallelAccesses(options.trackParallelAccesses) /* CMU-ONLY */
  , theInOrderMemory(options.inOrderMemory)
  , theInOrderExecute(options.inOrderExecute)
  , theIdleThisCycle(false)
  , theIdleCycleCount(0)
  , theBBVTracker( /*BBVTracker::createBBVTracker(aNode)*/ 0 )  /* CMU-ONLY */
  , theOnChipLatency(options.onChipLatency)
  , theOffChipLatency(options.offChipLatency)
  , theValidateMMU(options.validateMMU)
  , theNumMemoryPorts(options.numMemoryPorts)
  , theNumSnoopPorts(options.numSnoopPorts)
  , theMispredictCycles ( 0 ) // for IStall and mispredict stats
  , theMispredictCount ( 0 )    // for IStall and mispredict stats
  , theCommitNumber(0)
  , theCommitCount( theName  + "-Commits" )
  , theCommitCount_NonSpin_User( theName  + "-Commits:NonSpin:User" )
  , theCommitCount_NonSpin_System( theName  + "-Commits:NonSpin:System" )
  , theCommitCount_NonSpin_Trap( theName  + "-Commits:NonSpin:Trap" )
  , theCommitCount_NonSpin_Idle( theName  + "-Commits:NonSpin:Idle" )
  , theCommitCount_Spin_User( theName  + "-Commits:Spin:User" )
  , theCommitCount_Spin_System( theName  + "-Commits:Spin:System" )
  , theCommitCount_Spin_Trap( theName  + "-Commits:Spin:Trap" )
  , theCommitCount_Spin_Idle( theName  + "-Commits:Spin:Idle" )
  , theLSQOccupancy ( theName  + "-Occupancy:LSQ" )
  , theSBOccupancy ( theName  + "-Occupancy:SB" )
  , theSBUniqueOccupancy ( theName  + "-Occupancy:SB:UniqueBlocks" )
  , theSBNAWOccupancy ( theName  + "-Occupancy:SBNAW" )
  , theSpinCount( theName + "-Spins" )
  , theSpinCycles( theName + "-SpinCycles" )
  , theStorePrefetches( theName + "-StorePrefetches" )
  , theAtomicPrefetches( theName + "-AtomicPrefetches" )
  , theStorePrefetchConflicts( theName + "-StorePrefetchConflicts" )
  , theStorePrefetchDuplicates( theName + "-StorePrefetchDuplicates" )
  , thePartialSnoopLoads( theName + "-PartialSnoopLoads" )
  , theRaces( theName + "-Races" )
  , theRaces_LoadReplayed_User( theName + "-Races:LoadReplay:User" )
  , theRaces_LoadReplayed_System( theName + "-Races:LoadReplay:System" )
  , theRaces_LoadForwarded_User( theName + "-Races:LoadFwd:User" )
  , theRaces_LoadForwarded_System( theName + "-Races:LoadFwd:System" )
  , theRaces_LoadAlreadyOrdered_User( theName + "-Races:LoadOrderOK:User" )
  , theRaces_LoadAlreadyOrdered_System( theName + "-Races:LoadOrderOK:System" )
  , theSpeculations_Started( theName + "-Speculations:Started" )
  , theSpeculations_Successful( theName + "-Speculations:Successful" )
  , theSpeculations_Rollback( theName + "-Speculations:Rollback" )
  , theSpeculations_PartialRollback( theName + "-Speculations:PartialRollback" )
  , theSpeculations_Resync( theName + "-Speculations:Resync" )
  , theSpeculativeCheckpoints( theName + "-SpeculativeCheckpoints" )
  , theMaxSpeculativeCheckpoints( theName + "-SpeculativeCheckpoints:Max" )
  , theROBOccupancyTotal( theName + "-ROB-Occupancy:Total" )
  , theROBOccupancySpin( theName + "-ROB-Occupancy:Spin" )
  , theROBOccupancyNonSpin( theName + "-ROB-Occupancy:NonSpin" )
  , theSideEffectOnChip( theName + "-SideEffects:OnChip" )
  , theSideEffectOffChip( theName + "-SideEffects:OffChip" )
  , theNonSpeculativeAtomics( theName + "-NonSpeculativeAtomics" )
  , theRequiredDiscards ( theName + "-Discards:Required" )
  , theRequiredDiscards_Histogram ( theName + "-Discards:Required:Histogram" )
  , theNearestCkptDiscards ( theName + "-Discards:ToCkpt" )
  , theNearestCkptDiscards_Histogram ( theName + "-Discards:ToCkpt:Histogram" )
  , theSavedDiscards ( theName + "-Discards:Saved" )
  , theSavedDiscards_Histogram ( theName + "-Discards:Saved:Histogram" )
  , theCheckpointsDiscarded( theName + "-RollbackOverNCheckpoints" )
  , theRollbackToCheckpoint( theName + "-RollbackToCheckpoint" )
  , theWP_CoherenceMiss( theName + "-WP:CoherenceMisses" )
  , theWP_SVBHit( theName + "-WP:SVBHits" )
  , theResync_GarbageCollect ( theName + "-Resync:GarbageCollect" )
  , theResync_AbortedSpec ( theName + "-Resync:AbortedSpec" )
  , theResync_BlackBox ( theName + "-Resync:BlackBox" )
  , theResync_MAGIC ( theName + "-Resync:MAGIC" )
  , theResync_ITLBMiss ( theName + "-Resync:ITLBMiss" )
  , theResync_UnimplementedAnnul ( theName + "-Resync:UnimplementedAnnul" )
  , theResync_RDPRUnsupported ( theName + "-Resync:RDPRUnsupported" )
  , theResync_WRPRUnsupported ( theName + "-Resync:WRPRUnsupported" )
  , theResync_MEMBARSync ( theName + "-Resync:MEMBARSync" )
  , theResync_POPCUnsupported ( theName + "-Resync:POPCUnsupported" )
  , theResync_UnexpectedException ( theName + "-Resync:UnexpectedException" )
  , theResync_Interrupt ( theName + "-Resync:Interrupt" )
  , theResync_DeviceAccess ( theName + "-Resync:DeviceAccess" )
  , theResync_FailedValidation ( theName + "-Resync:FailedValidation" )
  , theResync_FailedHandleTrap ( theName + "-Resync:FailedHandleTrap" )
  , theResync_UnsupportedLoadASI( theName + "-Resync:UnsupportedASI:Load" )
  , theResync_UnsupportedAtomicASI( theName + "-Resync:UnsupportedASI:Atomic" )
  , theResync_SideEffectLoad( theName + "-Resync:SideEffectLoad" )
  , theResync_SideEffectStore( theName + "-Resync:SideEffectStore" )
  , theResync_Unknown ( theName + "-Resync:Unknown" )
  , theFalseITLBMiss ( theName + "-FalseITLBMiss" )
  , theEpochs  ( theName + "-MLPEpoch" )
  , theEpochs_Instructions( theName + "-MLPEpoch:Instructions" )
  , theEpochs_Instructions_Avg( theName + "-MLPEpoch:Instructions:Avg" )
  , theEpochs_Instructions_StdDev( theName + "-MLPEpoch:Instructions:StdDev" )
  , theEpochs_Loads ( theName + "-MLPEpoch:Loads" )
  , theEpochs_Loads_Avg ( theName + "-MLPEpoch:Loads:Avg" )
  , theEpochs_Loads_StdDev ( theName + "-MLPEpoch:Loads:StdDev" )
  , theEpochs_Stores ( theName + "-MLPEpoch:Stores" )
  , theEpochs_Stores_Avg ( theName + "-MLPEpoch:Stores:Avg" )
  , theEpochs_Stores_StdDev ( theName + "-MLPEpoch:Stores:StdDev" )
  , theEpochs_OffChipStores ( theName + "-MLPEpoch:Stores:OffChip" )
  , theEpochs_OffChipStores_Avg ( theName + "-MLPEpoch:Stores:OffChip:Avg" )
  , theEpochs_OffChipStores_StdDev ( theName + "-MLPEpoch:Stores:OffChip:StdDev" )
  , theEpochs_StoreAfterOffChipStores( theName + "-MLPEpoch:StoresAfterOffChipStore" )
  , theEpochs_StoreAfterOffChipStores_Avg( theName + "-MLPEpoch:StoresAfterOffChipStore:Avg" )
  , theEpochs_StoreAfterOffChipStores_StdDev( theName + "-MLPEpoch:StoresAfterOffChipStore:StdDev" )
  , theEpochs_StoresOutstanding( theName + "-MLPEpoch:StoresOutstanding" )
  , theEpochs_StoresOutstanding_Avg( theName + "-MLPEpoch:StoresOutstanding:Avg" )
  , theEpochs_StoresOutstanding_StdDev( theName + "-MLPEpoch:StoresOutstanding:StdDev" )
  , theEpochs_StoreBlocks ( theName + "-MLPEpoch:StoreBlocks" )
  , theEpochs_LoadOrStoreBlocks ( theName + "-MLPEpoch:LoadOrStoreBlocks" )
  , theEpoch_StartInsn(0)
  , theEpoch_LoadCount(0)
  , theEpoch_StoreCount(0)
  , theEpoch_OffChipStoreCount(0)
  , theEpoch_StoreAfterOffChipStoreCount(0)
  , theEmptyROBCause( kSync )
  , theLastCycleIStalls( 0 )
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
  , theCoalescedStores( theName + "-CoalescedStores" )
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
  // Each FU starts ready to accept an operation
  , intAluCyclesToReady(options.numIntAlu, 0)
  , intMultCyclesToReady(options.numIntMult, 0)
  , fpAluCyclesToReady(options.numFpAlu, 0)
  , fpMultCyclesToReady(options.numFpMult, 0) {

  // original constructor continues here...
  prepareMemOpAccounting();

  std::vector<uint32_t> reg_file_sizes;
  reg_file_sizes.resize(kLastMapTableCode + 2);
  reg_file_sizes[rRegisters] = krRegs_Total + 2 * theROBSize;
  reg_file_sizes[fRegisters] = kfRegs + 4 * theROBSize;
  reg_file_sizes[ccBits] = 5 + theROBSize;
  theRegisters.initialize(reg_file_sizes);

  //Map table for rRegisters
  theMapTables.push_back
  ( std::make_shared<PhysicalMap>( krRegs_Total , reg_file_sizes[rRegisters])
  );

  //Map table for fRegisters
  theMapTables.push_back
  ( std::make_shared<PhysicalMap>( kfRegs, reg_file_sizes[fRegisters])
  );

  //Map table for ccBits
  theMapTables.push_back
  ( std::make_shared<PhysicalMap>( 5, reg_file_sizes[ccBits])
  );

  reset();

  theCommitUSArray[0] = &theCommitCount_NonSpin_User;
  theCommitUSArray[1] = &theCommitCount_NonSpin_System;
  theCommitUSArray[2] = &theCommitCount_NonSpin_Trap;
  theCommitUSArray[3] = &theCommitCount_NonSpin_Idle;
  theCommitUSArray[4] = &theCommitCount_Spin_User;
  theCommitUSArray[5] = &theCommitCount_Spin_System;
  theCommitUSArray[6] = &theCommitCount_Spin_Trap;
  theCommitUSArray[7] = &theCommitCount_Spin_Idle;

  for ( int32_t i = 0; i < codeLastCode; ++i) {
    std::stringstream user_name;
    user_name << theName << "-InsnCount:User:" << eInstructionCode(i);
    std::stringstream system_name;
    system_name << theName + "-InsnCount:System:" << eInstructionCode(i);
    std::stringstream trap_name;
    trap_name << theName + "-InsnCount:Trap:" << eInstructionCode(i);
    std::stringstream idle_name;
    idle_name << theName + "-InsnCount:Idle:" << eInstructionCode(i);

    theCommitsByCode[0].push_back(new Stat::StatCounter( user_name.str() ));
    theCommitsByCode[1].push_back(new Stat::StatCounter( system_name.str() ));
    theCommitsByCode[2].push_back(new Stat::StatCounter( trap_name.str() ));
    theCommitsByCode[3].push_back(new Stat::StatCounter( idle_name.str() ));
  }

  kTBUser = theTimeBreakdown.addClass("User");
  kTBSystem = theTimeBreakdown.addClass("System");
  kTBTrap = theTimeBreakdown.addClass("Trap");
  kTBIdle = theTimeBreakdown.addClass("Idle");

  theLastStallCause = nXactTimeBreakdown::kUnknown;
  theCycleCategory = kTBUser;
}

void CoreImpl::resetv9() {

  theWindowMap.reset();
  theArchitecturalWindowMap.reset();
  mapTable(rRegisters).reset();
  mapTable(fRegisters).reset();
  mapTable(ccBits).reset();
  theRegisters.reset();

  theRoundingMode = 0;
  theTBA = 0;
  thePSTATE = 0;
  theTL = 0;
  thePIL = 0;
  theCANSAVE = 0;
  theCANRESTORE = 0;
  theCLEANWIN = 0;
  theOTHERWIN = 0;
  theWSTATE = 0;
  theVER = 0;
  for (int32_t i = 0; i < 5; ++i) {
    theTPC[i] = 0;
    theTNPC[i] = 0;
    theTSTATE[i] = 0;
    theTT[i] = 0;
  }

  thePendingTrap = 0;
  thePopTLRequested = false;
  theTrapInstruction = 0;

  theInterruptSignalled = false;
  thePendingInterrupt = 0;
  theInterruptInstruction = 0;

  theBypassNetwork.reset();

  theActiveActions = action_list_t();
  theRescheduledActions = action_list_t();

  theDispatchInteractions.clear();
  thePreserveInteractions = false;

  theROB.clear();

  theSquashRequested = false;
  theSquashReason = eSquashCause(0);
  theSquashInclusive = false;

  theRedirectRequested = false;
  theRedirectPC = VirtualMemoryAddress(0);
  theRedirectNPC = VirtualMemoryAddress(0);

  clearLSQ();

  thePartialSnoopersOutstanding = 0;

}

void CoreImpl::reset() {
  FLEXUS_PROFILE();

  cleanMSHRS(0);

  resetv9();

  theSRB.clear();

  //theBranchFeedback is NOT cleared

  clearSSB();

  if (theIsSpeculating) {
    DBG_( Iface, ( << theName << " Speculation aborted by sync" ) );
    accountResyncSpeculation();
  } else {
    accountResync();
  }

  theSLAT.clear();
  theIsSpeculating = false;
  theIsIdle = false;
  theCheckpoints.clear();
  theOpenCheckpoint = 0;
  theRetiresSinceCheckpoint = 0;

  theAbortSpeculation = false;

  theKernelPanicCount = 0;

  if (theSpinning) { //Note: will be false on first call to reset()
    theSpinCount += theSpinDetectCount;
    theSpinning = false;
    theSpinCycles += theFlexus->cycleCount() - theSpinStartCycle;
  }

  theSpinning = false;
  theSpinMemoryAddresses.clear();
  theSpinPCs.clear();
  theSpinDetectCount = 0;
  theSpinRetireSinceReset = 0;

  //theMemoryPorts and theSnoopPorts are not reset.
  //theMemoryReplies is not reset

  theIdleCycleCount = 0;
  theIdleThisCycle = false;

}

void CoreImpl::writePR(uint32_t aPR, uint64_t aVal) {
  switch (aPR) {
    case 0: //TPC
      setTPC( aVal, theTL );
      break;
    case 1: //NTPC
      setTNPC( aVal, theTL );
      break;
    case 2: //TSTATE
      setTSTATE( aVal, theTL );
      break;
    case 3: //TT
      setTT( aVal, theTL );
      break;
    case 5: //TBA
      setTBA( aVal );
      break;
    case 6: //PSTATE
      setPSTATE( aVal );
      break;
    case 7: //TL
      setTL( aVal );
      break;
    case 8: //PIL
      setPIL( aVal );
      break;
    case 9: //CWP
      setCWP(aVal);
      break;
    case 10: //CANSAVE
      setCANSAVE(aVal);
      break;
    case 11: //CANRESTORE
      setCANRESTORE(aVal);
      break;
    case 12: //CLEANWIN
      setCLEANWIN(aVal);
      break;
    case 13: //OTHERWIN
      setOTHERWIN(aVal);
      break;
    case 14: //WSTATE
      setWSTATE(aVal);
      break;
    default:
      DBG_( Crit, ( << "Write of unimplemented PR: " << aPR ) );
      break;
  }

}

uint64_t CoreImpl::readPR(uint32_t aPR) {
  switch (aPR) {
    case 0: //TPC
      return getTPC( theTL );
    case 1: //NTPC
      return getTNPC( theTL );
    case 2: //TSTATE
      return getTSTATE( theTL );
    case 3: //TT
      return getTT( theTL );
    case 5: //TBA
      return getTBA();
    case 6: //PSTATE
      return getPSTATE();
    case 7: //TL
      return getTL();
    case 8: //PIL
      return getPIL();
    case 9: //CWP
      return getCWP();
    case 10: //CANSAVE
      return getCANSAVE();
    case 11: //CANRESTORE
      return getCANRESTORE();
    case 12: //CLEANWIN
      return getCLEANWIN();
    case 13: //OTHERWIN
      return getOTHERWIN();
    case 14: //WSTATE
      return getWSTATE();
    case 31: //VER
      return getVER();
    default:
      DBG_( Crit, ( << "Read of unimplemented PR: " << aPR ) );
      return 0;
  }
  return 0;
}

std::string CoreImpl::prName(uint32_t aPR) {
  switch (aPR) {
    case 0: //TPC
      return "%tpc";
    case 1: //NTPC
      return "%tnpc";
    case 2: //TSTATE
      return "%tstate";
    case 3: //TT
      return "%tt";
    case 5: //TBA
      return "%tba";
    case 6: //PSTATE
      return "%pstate";
    case 7: //TL
      return "%tl";
    case 8: //PIL
      return "%pil";
    case 9: //CWP
      return "%cwp";
    case 10: //CANSAVE
      return "%cansave";
    case 11: //CANRESTORE
      return "%canrestore";
    case 12: //CLEANWIN
      return "%cleanwin";
    case 13: //OTHERWIN
      return "%otherwin";
    case 14: //WSTATE
      return "%wstate";
    case 31: //VER
      return "%ver";
    default:
      return "UnimplementedPR";
  }
}

void CoreImpl::setRoundingMode(uint32_t aRoundingMode) {
  theRoundingMode = aRoundingMode;
}

CoreModel * CoreModel::construct( uArchOptions_t options
                                  , std::function< void (Flexus::Qemu::Translation &, bool) > translate
                                  , std::function<int(bool)> advance
                                  , std::function< void(eSquashCause)> squash
                                  , std::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> redirect
                                  , std::function< void(int, int)> change_mode
                                  , std::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback
                                  , std::function<void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> notifyTMS /* CMU-ONLY */
                                  , std::function< void( bool )> signalStoreForwardingHit
                                ) {

  return new CoreImpl( options
                       , translate
                       , advance
                       , squash
                       , redirect
                       , change_mode
                       , feedback
                       , notifyTMS /* CMU-ONLY */
                       , signalStoreForwardingHit
                     );
}

} //nuArch
