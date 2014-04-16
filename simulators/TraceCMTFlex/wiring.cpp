


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "TraceCMTFlex 1.0";
}


#define THREADS_PER_CORE   4


#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/DecoupledFeeder/DecoupledFeeder.hpp>
#include <components/BPWarm/BPWarm.hpp>
#include <components/FastCache/FastCache.hpp>
#include <components/FastCMPCache/FastCMPCache.hpp>
#include <components/FastMemoryLoopback/FastMemoryLoopback.hpp>
#include <components/MagicBreak/MagicBreak.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( DecoupledFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( FastCache, "L1d", theL1DCfg );
CREATE_CONFIGURATION( FastCache, "L1i", theL1ICfg );
CREATE_CONFIGURATION( FastCMPCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( FastMemoryLoopback, "memory", theMemoryCfg );
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

#   define BLOCK_SIZE  64

  theFeederCfg.SimicsQuantum.initialize(100);
  theFeederCfg.TrackIFetch.initialize(true);
  theFeederCfg.HousekeepingPeriod.initialize(1000);
  theFeederCfg.SystemTickFrequency.initialize(0.0);

  theBPWarmCfg.Cores.initialize( THREADS_PER_CORE );

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1DCfg.MTWidth.initialize( THREADS_PER_CORE );
  theL1DCfg.Size.initialize(64 * K);
  theL1DCfg.Associativity.initialize(2);
  theL1DCfg.BlockSize.initialize(BLOCK_SIZE);
  theL1DCfg.CleanEvictions.initialize(true);
  theL1DCfg.CacheLevel.initialize(eL1);
  theL1DCfg.TraceTracker.initialize(true);

  theL1ICfg.MTWidth.initialize( THREADS_PER_CORE );
  theL1ICfg.Size.initialize(64 * K);
  theL1ICfg.Associativity.initialize(2);
  theL1ICfg.BlockSize.initialize(BLOCK_SIZE);
  theL1ICfg.CleanEvictions.initialize(true);
  theL1ICfg.CacheLevel.initialize(eL1I);
  theL1ICfg.TraceTracker.initialize(false);

  theL2Cfg.CMPWidth.initialize( getSystemWidth() / THREADS_PER_CORE );
  theL2Cfg.Size.initialize(16 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(BLOCK_SIZE);
  theL2Cfg.ReplPolicy.initialize("LRU");
  theL2Cfg.CleanEvictions.initialize(false);
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.ReuseDistStats.initialize(false);
  theL2Cfg.PerfectPlacement.initialize(false);

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
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( BPWarm,    theBPWarmCfg, theBPWarm, SCALE_WITH_SYSTEM_WIDTH, DIVIDE,   THREADS_PER_CORE );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1DCfg,    theL1D,    SCALE_WITH_SYSTEM_WIDTH, DIVIDE,   THREADS_PER_CORE );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1ICfg,    theL1I,    SCALE_WITH_SYSTEM_WIDTH, DIVIDE,   THREADS_PER_CORE );
FLEXUS_INSTANTIATE_COMPONENT( FastCMPCache, theL2Cfg, theL2 );
FLEXUS_INSTANTIATE_COMPONENT( FastMemoryLoopback, theMemoryCfg, theMemory );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==
WIRE(theFeeder, ToBPred,                theBPWarm, ITraceIn)
WIRE(theFeeder, ToL1D,                  theL1D, RequestIn)
WIRE(theFeeder, ToL1I,                  theL1I, FetchRequestIn)
WIRE(theL1D, RequestOut,                theL2, RequestIn)
WIRE(theL1I, RequestOut,                theL2, FetchRequestIn)
WIRE(theL2, RequestOut,                 theMemory, FromCache)
WIRE(theL2, SnoopOutD,                  theL1D, SnoopIn)
WIRE(theL2, SnoopOutI,                  theL1I, SnoopIn)


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theMagicBreak, TickDrive )
, DRIVE( theL1D, UpdateStatsDrive )
, DRIVE( theL1I, UpdateStatsDrive )
, DRIVE( theL2, UpdateStatsDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

