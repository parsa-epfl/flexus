


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "Minotaur2 1.0";
}


#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/DecoupledFeeder/DecoupledFeeder.hpp>
#include <components/BPWarm/BPWarm.hpp>
#include <components/FastCache/FastCache.hpp>
#include <components/FastBus/FastBus.hpp>
#include <components/FastSpatialPrefetcher/FastSpatialPrefetcher.hpp>
#include <components/TraceTracker/TraceTrackerComponent.hpp>
#include <components/MagicBreak/MagicBreak.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( DecoupledFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( FastCache, "L1d", theL1dCfg );
CREATE_CONFIGURATION( FastCache, "L1i", theL1ICfg );
CREATE_CONFIGURATION( FastCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( FastBus, "bus", theBusCfg );
CREATE_CONFIGURATION( SpatialPrefetcher, "spatial-prefetch", theSpatialPrefetcherCfg );
CREATE_CONFIGURATION( TraceTrackerComponent, "trace-tracker", theTraceTrackerCfg );
CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );
CREATE_CONFIGURATION( BPWarm, "bpwarm", theBPWarmCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  DBG_( Dev, ( << " initializing Parameters..." ) );

  theFeederCfg.SimicsQuantum.initialize(100);
  theFeederCfg.TrackIFetch.initialize(false);
  theFeederCfg.HousekeepingPeriod.initialize(1000);
  theFeederCfg.SystemTickFrequency.initialize(0.0);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1DCfg.MTWidth.initialize( 1 );
  theL1dCfg.Size.initialize(64 * K);
  theL1dCfg.Associativity.initialize(2);
  theL1dCfg.BlockSize.initialize(64);
  theL1dCfg.CleanEvictions.initialize(false);
  theL1dCfg.CacheLevel.initialize(eL1);
  theL1dCfg.TraceTracker.initialize(true);
  theL1dCfg.NotifyReads.initialize(false);
  theL1dCfg.NotifyWrites.initialize(false);

  theL1ICfg.MTWidth.initialize( 1 );
  theL1ICfg.Size.initialize(64 * K);
  theL1ICfg.Associativity.initialize(2);
  theL1ICfg.BlockSize.initialize(64);
  theL1ICfg.CleanEvictions.initialize(false);
  theL1ICfg.CacheLevel.initialize(eL1I);
  theL1ICfg.TraceTracker.initialize(false);
  theL1ICfg.NotifyReads.initialize(false);
  theL1ICfg.NotifyWrites.initialize(false);

  theL2Cfg.MTWidth.initialize( 1 );
  theL2Cfg.Size.initialize(8 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.CleanEvictions.initialize(false);
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.TraceTracker.initialize(true);
  theL2Cfg.NotifyReads.initialize(false);
  theL2Cfg.NotifyWrites.initialize(false);

  theBusCfg.BlockSize.initialize(64);
  theBusCfg.InvalAll.initialize(false);
  theBusCfg.SavePageMap.initialize(false);

  theSpatialPrefetcherCfg.UsageEnable.initialize(false);
  theSpatialPrefetcherCfg.RepetEnable.initialize(false);
  theSpatialPrefetcherCfg.TimeRepetEnable.initialize(false);
  theSpatialPrefetcherCfg.BufFetchEnable.initialize(false);
  theSpatialPrefetcherCfg.PrefetchEnable.initialize(false);
  theSpatialPrefetcherCfg.ActiveEnable.initialize(false);
  theSpatialPrefetcherCfg.CacheLevel.initialize(eL2);
  theSpatialPrefetcherCfg.BlockSize.initialize(64);
  theSpatialPrefetcherCfg.SgpBlocks.initialize(8);
  theSpatialPrefetcherCfg.RepetType.initialize(1);
  theSpatialPrefetcherCfg.RepetFills.initialize(false);
  theSpatialPrefetcherCfg.SparseOpt.initialize(false);
  theSpatialPrefetcherCfg.PhtSize.initialize(0);
  theSpatialPrefetcherCfg.PhtAssoc.initialize(0);
  theSpatialPrefetcherCfg.CptType.initialize(0);
  theSpatialPrefetcherCfg.CptSize.initialize(0);
  theSpatialPrefetcherCfg.CptAssoc.initialize(0);
  theSpatialPrefetcherCfg.CptSparse.initialize(false);

  theTraceTrackerCfg.Enable.initialize(true);
  theTraceTrackerCfg.NumNodes.initialize( getSystemWidth() );
  theTraceTrackerCfg.BlockSize.initialize(64);
  theTraceTrackerCfg.Sharing.initialize(false);
  theTraceTrackerCfg.SharingLevel.initialize(eL2);

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
  theMagicBreakCfg.TerminateOnEndOfParallel.initialize(true);
  theMagicBreakCfg.EnableIterationCounts.initialize(false);

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
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1ICfg, theL1I, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL2Cfg, theL2, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT( FastBus, theBusCfg, theBus );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( SpatialPrefetcher, theSpatialPrefetcherCfg, theSpatial, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT( TraceTrackerComponent, theTraceTrackerCfg, theTraceTracker );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==

WIRE(theFeeder, ToBPred,                theBPWarm, ITraceIn)
WIRE(theFeeder, ToL1I,                  theL1I, FetchRequestIn)
WIRE(theL1I, RequestOut,                theL2, FetchRequestIn)

WIRE(theFeeder, ToL1D,                  theSpatial, RequestInL1)
WIRE(theSpatial, RequestOutL1,          theL1d, RequestIn)
WIRE(theL1d, RequestOut,                theSpatial, RequestInL2)
WIRE(theSpatial, RequestOutL2,          theL2, RequestIn)
WIRE(theL2, RequestOut,                 theBus, FromCaches)

WIRE(theBus, ToSnoops,                  theL2, SnoopIn)
WIRE(theL2, SnoopOut,                   theSpatial, SnoopInL2)
WIRE(theSpatial, SnoopOutL2,            theL1d, SnoopIn)
WIRE(theL1d, SnoopOut,                  theSpatial, SnoopInL1)


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theMagicBreak, TickDrive )
, DRIVE( theL1d, UpdateStatsDrive )
, DRIVE( theL1I, UpdateStatsDrive )
, DRIVE( theL2, UpdateStatsDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

