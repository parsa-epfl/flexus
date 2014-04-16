


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "Trace2SGPFlex 1.0";
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
CREATE_CONFIGURATION( SpatialPrefetcher, "spL1", theSPL1Cfg );
CREATE_CONFIGURATION( SpatialPrefetcher, "spL2", theSPL2Cfg );
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
  theL1dCfg.NotifyWrites.initialize(true);

  theL1ICfg.MTWidth.initialize( 1 );
  theL1ICfg.Size.initialize(64 * K);
  theL1ICfg.Associativity.initialize(2);
  theL1ICfg.BlockSize.initialize(64);
  theL1ICfg.CleanEvictions.initialize(false);
  theL1ICfg.CacheLevel.initialize(eL1I);
  theL1ICfg.NotifyReads.initialize(false);
  theL1ICfg.NotifyWrites.initialize(false);

  theL2Cfg.MTWidth.initialize( 1 );
  theL2Cfg.Size.initialize(8 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.CleanEvictions.initialize(false);
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.NotifyReads.initialize(false);
  theL2Cfg.NotifyWrites.initialize(false);

  theBusCfg.BlockSize.initialize(64);
  theBusCfg.TrackUpgrades.initialize(false);
  theBusCfg.TrackConsumptions.initialize(false);
  theBusCfg.InvalAll.initialize(false);
  theBusCfg.Migration.initialize(true);
  theBusCfg.SavePageMap.initialize(false);

  theSPL1Cfg.UsageEnable.initialize(false);
  theSPL1Cfg.RepetEnable.initialize(false);
  theSPL1Cfg.TimeRepetEnable.initialize(false);
  theSPL1Cfg.BufFetchEnable.initialize(false);
  theSPL1Cfg.PrefetchEnable.initialize(false);
  theSPL1Cfg.ActiveEnable.initialize(false);
  theSPL1Cfg.CacheLevel.initialize(eL1);
  theSPL1Cfg.BlockSize.initialize(64);
  theSPL1Cfg.SgpBlocks.initialize(8);
  theSPL1Cfg.RepetType.initialize(1);
  theSPL1Cfg.RepetFills.initialize(false);
  theSPL1Cfg.SparseOpt.initialize(false);
  theSPL1Cfg.PhtSize.initialize(0);
  theSPL1Cfg.PhtAssoc.initialize(0);
  theSPL1Cfg.CptType.initialize(0);
  theSPL1Cfg.CptSize.initialize(0);
  theSPL1Cfg.CptAssoc.initialize(0);
  theSPL1Cfg.CptSparse.initialize(false);

  theSPL2Cfg.UsageEnable.initialize(false);
  theSPL2Cfg.RepetEnable.initialize(false);
  theSPL2Cfg.TimeRepetEnable.initialize(false);
  theSPL2Cfg.BufFetchEnable.initialize(false);
  theSPL2Cfg.PrefetchEnable.initialize(false);
  theSPL2Cfg.ActiveEnable.initialize(false);
  theSPL2Cfg.CacheLevel.initialize(eL2);
  theSPL2Cfg.BlockSize.initialize(64);
  theSPL2Cfg.SgpBlocks.initialize(8);
  theSPL2Cfg.RepetType.initialize(1);
  theSPL2Cfg.RepetFills.initialize(false);
  theSPL2Cfg.SparseOpt.initialize(false);
  theSPL2Cfg.PhtSize.initialize(0);
  theSPL2Cfg.PhtAssoc.initialize(0);
  theSPL2Cfg.CptType.initialize(0);
  theSPL2Cfg.CptSize.initialize(0);
  theSPL2Cfg.CptAssoc.initialize(0);
  theSPL2Cfg.CptSparse.initialize(false);

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

  return false; //Abort simulation if parameters are not initialized
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
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( SpatialPrefetcher, theSPL1Cfg, theSPL1, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( SpatialPrefetcher, theSPL2Cfg, theSPL2, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT( TraceTrackerComponent, theTraceTrackerCfg, theTraceTracker );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==
/*
      WIRE(theFeeder, ToL1D,                  theL1d, RequestIn)
      WIRE(theL1d, RequestOut,                theL2, RequestIn)
      WIRE(theL2, RequestOut,                 theBus, FromCaches)
      WIRE(theBus, ToSnoops,                  theL2, SnoopIn)
      WIRE(theL2, SnoopOut,                   theL1d, SnoopIn)
*/

// for prefetching into L1
/*
      WIRE(theFeeder, ToL1D,                  theSpatial, RequestIn)
      WIRE(theSpatial, RequestOut,            theL1d, RequestIn)
      WIRE(theL1d, RequestOut,                theL2, RequestIn)
      WIRE(theL2, RequestOut,                 theBus, FromCaches)
      WIRE(theBus, ToSnoops,                  theL2, SnoopIn)
      WIRE(theL2, SnoopOut,                   theL1d, SnoopIn)
      WIRE(theL1d, SnoopOut,                  theSpatial, SnoopIn)
*/

// for prefetching into L2

WIRE(theFeeder, ToL1D,                  theSPL1, RequestIn)
WIRE(theSPL1, RequestOut,               theL1d, RequestIn)
WIRE(theFeeder, ToL1I,                  theL1I, FetchRequestIn)
WIRE(theFeeder, ToBPred,                theBPWarm, ITraceIn)
WIRE(theL1d, RequestOut,                theSPL2, RequestIn)
WIRE(theL1I, RequestOut,                theL2, FetchRequestIn)
WIRE(theSPL2, RequestOut,               theL2, RequestIn)
WIRE(theL2, RequestOut,                 theBus, FromCaches)
WIRE(theBus, ToSnoops,                  theL2, SnoopIn)
WIRE(theL2, SnoopOut,                   theSPL2, SnoopIn)
WIRE(theSPL2, SnoopOut,                 theL1d, SnoopIn)
WIRE(theL1d, SnoopOut,                  theSPL1, SnoopIn)

WIRE(theL1d, Writes,                    theBus, ConfirmMigrate)


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theMagicBreak, TickDrive )
, DRIVE( theL1d, UpdateStatsDrive )
, DRIVE( theL1I, UpdateStatsDrive )
, DRIVE( theL2, UpdateStatsDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

