


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "TraceDSMCMPFlex 1.0";
}

#define CMP_CHIPS_IN_SYSTEM   4


#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/DecoupledFeeder/DecoupledFeeder.hpp>
#include <components/FastCache/FastCache.hpp>
#include <components/FastCMPCache/FastCMPCache.hpp>
#include <components/FastBus/FastBus.hpp>
#include <components/MagicBreak/MagicBreak.hpp>
#include <components/ReuseDistance/ReuseDistance.hpp>
#include <components/MissClassifier/MissClassifier.hpp>
#include <components/PerfectPlacement/PerfectPlacement.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( DecoupledFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( FastCache, "L1d", theL1DCfg );
CREATE_CONFIGURATION( FastCache, "L1i", theL1ICfg );
CREATE_CONFIGURATION( FastCMPCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( FastBus, "bus", theBusCfg );
CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );

CREATE_CONFIGURATION( ReuseDistance, "L2-ReuseDist", theReuseDistanceCfg );
CREATE_CONFIGURATION( MissClassifier, "L2-MissClass", theMissClassifierCfg );
CREATE_CONFIGURATION( PerfectPlacement, "L2-PerfPlc", thePerfectPlacementCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  DBG_( Dev, ( << " initializing Parameters..." ) );

#   define BLOCK_SIZE  128

  theFeederCfg.SimicsQuantum.initialize(100);
  theFeederCfg.TrackIFetch.initialize(true);
  theFeederCfg.HousekeepingPeriod.initialize(1000);
  theFeederCfg.SystemTickFrequency.initialize(0.0);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1DCfg.MTWidth.initialize( 1 );
  theL1DCfg.Size.initialize(32 * K);
  theL1DCfg.Associativity.initialize(2);
  theL1DCfg.BlockSize.initialize(BLOCK_SIZE);
  theL1DCfg.CleanEvictions.initialize(true);
  theL1DCfg.CacheLevel.initialize(eL1);

  theL1ICfg.MTWidth.initialize( 1 );
  theL1ICfg.Size.initialize(64 * K);
  theL1ICfg.Associativity.initialize(1);
  theL1ICfg.BlockSize.initialize(BLOCK_SIZE);
  theL1ICfg.CleanEvictions.initialize(true);
  theL1ICfg.CacheLevel.initialize(eL1I);

  theL2Cfg.CMPWidth.initialize( getSystemWidth() / CMP_CHIPS_IN_SYSTEM );
  theL2Cfg.Size.initialize(2 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(BLOCK_SIZE);
  theL2Cfg.CleanEvictions.initialize(false);
  theL2Cfg.ReuseDistStats.initialize(true);
  theL2Cfg.PerfectPlacement.initialize(false);

  theReuseDistanceCfg.ReuseWindowSizeInCycles.initialize(100000000);    // 100M cycles
  theReuseDistanceCfg.BlockSize.initialize(BLOCK_SIZE);
  theReuseDistanceCfg.IgnoreBlocksWithMinDist.initialize(0); // turn it off

  theMissClassifierCfg.OnOffSwitch.initialize(true);
  theMissClassifierCfg.BlockSize.initialize(BLOCK_SIZE);

  thePerfectPlacementCfg.Size.initialize(2 * M);
  thePerfectPlacementCfg.Associativity.initialize(8);
  thePerfectPlacementCfg.BlockSize.initialize(BLOCK_SIZE);

  theBusCfg.BlockSize.initialize(BLOCK_SIZE);
  theBusCfg.PageSize.initialize(8192);
  theBusCfg.RoundRobin.initialize(true);

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

  return false; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
//All component Instances are created here.  This section
//also creates handles for each component

FLEXUS_INSTANTIATE_COMPONENT( DecoupledFeeder, theFeederCfg, theFeeder );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1DCfg, theL1D, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1ICfg, theL1I, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCMPCache, theL2Cfg, theL2, FIXED, FIXED, CMP_CHIPS_IN_SYSTEM);
FLEXUS_INSTANTIATE_COMPONENT( FastBus, theBusCfg, theBus );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( ReuseDistance, theReuseDistanceCfg, theReuseDistance, FIXED, FIXED, CMP_CHIPS_IN_SYSTEM );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MissClassifier, theMissClassifierCfg, theMissClassifier, FIXED, FIXED, CMP_CHIPS_IN_SYSTEM );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( PerfectPlacement, thePerfectPlacementCfg, thePerfectPlacement, FIXED, FIXED, CMP_CHIPS_IN_SYSTEM );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==
WIRE(theFeeder, ToL1D,                  theL1D, RequestIn)
WIRE(theFeeder, ToL1I,                  theL1I, FetchRequestIn)
WIRE(theL1D, RequestOut,                theL2, RequestIn)
WIRE(theL1I, RequestOut,                theL2, FetchRequestIn)
WIRE(theL2, RequestOut,                 theBus, FromCaches)
WIRE(theBus, ToSnoops,                  theL2, SnoopIn)
WIRE(theL2, SnoopOutD,                  theL1D, SnoopIn)
WIRE(theL2, SnoopOutI,                  theL1I, SnoopIn)

WIRE(theL2, ReuseDist,                  theReuseDistance, RequestIn)
WIRE(theL2, MissClassifier,             theMissClassifier, RequestIn)
WIRE(theL2, PerfPlc,                    thePerfectPlacement, RequestIn)


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theMagicBreak, TickDrive )
, DRIVE( theL1D, UpdateStatsDrive )
, DRIVE( theL1I, UpdateStatsDrive )
, DRIVE( theL2, UpdateStatsDrive )
, DRIVE( theReuseDistance, UpdateStatsDrive )
, DRIVE( theMissClassifier, UpdateStatsDrive )
, DRIVE( thePerfectPlacement, UpdateStatsDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

