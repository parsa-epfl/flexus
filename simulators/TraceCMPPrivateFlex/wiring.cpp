


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "TraceCMPPrivateFlex 1.0";
}


#define NUM_L2_SLICES             0
#define NUM_CORES_PER_TORUS_ROW   4
#define SIZE_INSTR_CLUSTER        4

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/DecoupledFeeder/DecoupledFeeder.hpp>
#include <components/BPWarm/BPWarm.hpp>
#include <components/FastCache/FastCache.hpp>
#include <components/FastCMPCache/FastCMPCache.hpp>
#include <components/FastBus/FastBus.hpp>
#include <components/MagicBreak/MagicBreak.hpp>
#include <components/NUCATracer/NUCATracer.hpp>
#include <components/FastCMPNetworkController/FastCMPNetworkController.hpp>
#include <components/WhiteBox/WhiteBox.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( DecoupledFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( FastCache, "L1d", theL1DCfg );
CREATE_CONFIGURATION( FastCache, "L1i", theL1ICfg );
CREATE_CONFIGURATION( FastCMPCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( FastBus, "bus", theBusCfg );
CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );
CREATE_CONFIGURATION( BPWarm, "bpwarm", theBPWarmCfg );
CREATE_CONFIGURATION( NUCATracer, "tracer", theNUCATracerCfg );
CREATE_CONFIGURATION( FastCMPNetworkController, "network", theCMPNetControllerCfg );
CREATE_CONFIGURATION( WhiteBox, "white-box", theWhiteBoxCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  DBG_( Dev, ( << " initializing Parameters..." ) );

#   define BLOCK_SIZE  64
#   define PAGE_SIZE   8192

  theFeederCfg.CMPWidth.initialize(0);
  theFeederCfg.SimicsQuantum.initialize(100);
  theFeederCfg.TrackIFetch.initialize(true);
  theFeederCfg.HousekeepingPeriod.initialize(1000);
  theFeederCfg.SystemTickFrequency.initialize(0.0);
  theFeederCfg.DecoupleInstrDataSpaces.initialize(false); // Must be true for R-NUCA

  theBPWarmCfg.Cores.initialize( 1 );

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1DCfg.MTWidth.initialize( 1 );
  theL1DCfg.Size.initialize(64 * K);
  theL1DCfg.Associativity.initialize(2);
  theL1DCfg.BlockSize.initialize(BLOCK_SIZE);
  theL1DCfg.CleanEvictions.initialize(true);      // Must be true: the implementation depends on this
  theL1DCfg.CacheLevel.initialize(eL1);
  theL1DCfg.TraceTracker.initialize(false);
  theL1DCfg.NotifyReads.initialize(false);
  theL1DCfg.NotifyWrites.initialize(false);

  theL1ICfg.MTWidth.initialize( 1 );
  theL1ICfg.Size.initialize(64 * K);
  theL1ICfg.Associativity.initialize(2);
  theL1ICfg.BlockSize.initialize(BLOCK_SIZE);
  theL1ICfg.CleanEvictions.initialize(true);      // Must be true: the implementation depends on this
  theL1ICfg.CacheLevel.initialize(eL1I);
  theL1ICfg.TraceTracker.initialize(false);
  theL1ICfg.NotifyReads.initialize(false);
  theL1ICfg.NotifyWrites.initialize(false);

  theL2Cfg.CMPWidth.initialize( getSystemWidth() );
  theL2Cfg.Size.initialize(1 * M);
  theL2Cfg.Associativity.initialize(16);
  theL2Cfg.BlockSize.initialize(BLOCK_SIZE);
  theL2Cfg.CleanEvictions.initialize(true);      // Must be true only for private: the implementation depends on this
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.ReplPolicy.initialize("LRU");
  theL2Cfg.ReuseDistStats.initialize(false);
  theL2Cfg.PerfectPlacement.initialize(false);
  theL2Cfg.InvalidateFriendIndices.initialize(false);

  theL2Cfg.NotifyReads.initialize(false);
  theL2Cfg.NotifyWrites.initialize(false);
  theL2Cfg.NotifyFetches.initialize(false);
  theL2Cfg.NotifyL1CleanEvicts.initialize(false);
  theL2Cfg.NotifyL1DirtyEvicts.initialize(false);
  theL2Cfg.NotifyL1IEvicts.initialize(false);

  theCMPNetControllerCfg.NumCores.initialize(getSystemWidth());
  theCMPNetControllerCfg.NumL2Slices.initialize(NUM_L2_SLICES == 0 ? getSystemWidth() : NUM_L2_SLICES);
  theCMPNetControllerCfg.PageSize.initialize(PAGE_SIZE);
  theCMPNetControllerCfg.CacheLineSize.initialize(BLOCK_SIZE);
  theCMPNetControllerCfg.Placement.initialize("private");
  theCMPNetControllerCfg.SizeOfInstrCluster.initialize( SIZE_INSTR_CLUSTER );
  theCMPNetControllerCfg.NumCoresPerTorusRow.initialize( NUM_CORES_PER_TORUS_ROW );
  theCMPNetControllerCfg.DecoupleInstrDataSpaces.initialize(false); // Must be true for R-NUCA
  theCMPNetControllerCfg.WhiteBoxDebug.initialize(false);
  // ASR parameters
  theCMPNetControllerCfg.FramesPerL2Slice.initialize(512 * K / BLOCK_SIZE);
  theCMPNetControllerCfg.OffChipLatencyInHops.initialize(30);
  theCMPNetControllerCfg.RemoteHitLatencyInHops.initialize(4);
  theCMPNetControllerCfg.NLHBSize.initialize(16 * K);
  theCMPNetControllerCfg.VTBSize.initialize(1 * K);
  theCMPNetControllerCfg.MonitorSize.initialize(1 * K);

  theMagicBreakCfg.CkptCycleInterval.initialize(0);
  theMagicBreakCfg.CkptCycleName.initialize(0);
  theMagicBreakCfg.CheckpointOnIteration.initialize(false);
  theMagicBreakCfg.CheckpointEveryXTransactions.initialize(false);
  theMagicBreakCfg.TerminateOnTransaction.initialize(-1);
  theMagicBreakCfg.FirstTransactionIs.initialize(0);
  theMagicBreakCfg.CycleMinimum.initialize(0);
  theMagicBreakCfg.TransactionStatsInterval.initialize(10000);
  theMagicBreakCfg.StopCycle.initialize(0);
  theMagicBreakCfg.EnableTransactionCounts.initialize(false);
  theMagicBreakCfg.TransactionType.initialize(0);
  theMagicBreakCfg.TerminateOnIteration.initialize(-1);
  theMagicBreakCfg.TerminateOnMagicBreak.initialize(-1);
  theMagicBreakCfg.EnableIterationCounts.initialize(false);

  theBusCfg.BlockSize.initialize(BLOCK_SIZE);
  theBusCfg.TrackWrites.initialize(false);
  theBusCfg.TrackReads.initialize(false);
  theBusCfg.TrackDMA.initialize(false);
  theBusCfg.TrackProductions.initialize(false);
  theBusCfg.TrackEvictions.initialize(false);
  theBusCfg.TrackInvalidations.initialize(false);
  theBusCfg.TrackFlushes.initialize(false);
  theBusCfg.InvalAll.initialize(false);
  theBusCfg.PageSize.initialize(PAGE_SIZE);
  theBusCfg.RoundRobin.initialize(true);
  theBusCfg.SavePageMap.initialize(false);

  theNUCATracerCfg.OnSwitch.initialize(false);
  theNUCATracerCfg.TraceOutPath.initialize("/afs/scotch/project/nuca_cache" );

  theFlexus->setStatInterval( "10000000" );     //10M
  theFlexus->setProfileInterval( "10000000" );  //10M
  theFlexus->setTimestampInterval( "1000000" ); //1M

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
//All component Instances are created here.  This section
//also creates handles for each component

FLEXUS_INSTANTIATE_COMPONENT( DecoupledFeeder, theFeederCfg, theFeeder );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( BPWarm, theBPWarmCfg, theBPWarm, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1DCfg, theL1D, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1ICfg, theL1I, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
#if (NUM_L2_SLICES == 0)
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCMPCache, theL2Cfg, theL2, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
#else
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCMPCache, theL2Cfg, theL2, FIXED, FIXED, NUM_L2_SLICES);
#endif
FLEXUS_INSTANTIATE_COMPONENT( FastCMPNetworkController, theCMPNetControllerCfg, theCMPNetController );
FLEXUS_INSTANTIATE_COMPONENT( FastBus, theBusCfg, theBus );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
FLEXUS_INSTANTIATE_COMPONENT( NUCATracer, theNUCATracerCfg, theNUCATracer );
FLEXUS_INSTANTIATE_COMPONENT( WhiteBox, theWhiteBoxCfg, theWhiteBox );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                   TO
//====                                   ==
WIRE(theFeeder, ToBPred,                 theBPWarm, ITraceIn)
WIRE(theFeeder, ToL1D,                   theL1D, RequestIn)
WIRE(theFeeder, ToL1I,                   theL1I, FetchRequestIn)
WIRE(theFeeder, ToDMA,                   theBus, DMA )
WIRE(theFeeder, ToNAW,                   theBus, NonAllocateWrite)

WIRE(theL1D, RequestOut,                 theCMPNetController, RequestInD)
WIRE(theCMPNetController, RequestOutD,   theL2, RequestIn)
WIRE(theL1I, RequestOut,                 theCMPNetController, RequestInI)
WIRE(theCMPNetController, RequestOutI,   theL2, FetchRequestIn)
WIRE(theL2, RequestOut,                  theCMPNetController, RequestFromL2)
WIRE(theCMPNetController, RequestOutMem, theBus, FromCaches)
// WIRE(theCMPNetController, RequestOutMem, theMemory, FromCache)

WIRE(theBus, ToSnoops,                   theCMPNetController, BusSnoopIn)
WIRE(theCMPNetController, ToL2Snoops,    theL2, SnoopIn)
WIRE(theL2, SnoopOutI,                   theCMPNetController, SnoopInI)
WIRE(theCMPNetController, SnoopOutI,     theL1I, SnoopIn)
WIRE(theL2, SnoopOutD,                   theCMPNetController, SnoopInD)
WIRE(theCMPNetController, SnoopOutD,     theL1D, SnoopIn)

WIRE(theBus, Writes,                     theNUCATracer, Writes)
WIRE(theBus, Reads,                      theNUCATracer, Reads)
WIRE(theBus, Fetches,                    theNUCATracer, Fetches)

WIRE(theL2, Reads,                       theNUCATracer, L2Reads)
WIRE(theL2, Writes,                      theNUCATracer, L2Writes)
WIRE(theL2, Fetches,                     theNUCATracer, L2Fetches)
WIRE(theL2, L1CleanEvicts,               theNUCATracer, L1CleanEvicts)
WIRE(theL2, L1DirtyEvicts,               theNUCATracer, L1DirtyEvicts)
WIRE(theL2, L1IEvicts,                   theNUCATracer, L1IEvicts)



#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theMagicBreak, TickDrive )
, DRIVE( theL1D, UpdateStatsDrive )
, DRIVE( theL1I, UpdateStatsDrive )
, DRIVE( theL2, UpdateStatsDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

