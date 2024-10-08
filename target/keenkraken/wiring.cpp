// FIXME Removed WhiteBox (commented out) of all places used in the simulator

#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>

// This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
// Simulator Name
std::string theSimulatorName = "KeenKraken v2024.05";
} // namespace Flexus

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/BPWarm/BPWarm.hpp>
#include <components/DecoupledFeederQEMU/DecoupledFeeder.hpp>
#include <components/FastCMPCache/FastCMPCache.hpp>
#include <components/FastCache/FastCache.hpp>
#include <components/FastMemoryLoopback/FastMemoryLoopback.hpp>
#include <components/MMU/MMU.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION(DecoupledFeeder, "feeder", theFeederCfg);
CREATE_CONFIGURATION(FastCache, "L1d", theL1DCfg);
CREATE_CONFIGURATION(FastCache, "L1i", theL1ICfg);
CREATE_CONFIGURATION(FastCMPCache, "L2", theL2Cfg);
CREATE_CONFIGURATION(FastMemoryLoopback, "memory", theMemoryCfg);
CREATE_CONFIGURATION(BPWarm, "bpwarm", theBPWarmCfg);
CREATE_CONFIGURATION(MMU, "mmu", theMMUCfg);

// You may optionally initialize configuration parameters from within this
// function.  This initialization occur before the command line is processed,
// so they will be overridden from the command line.
//
// Return value indicates whether simulation should abort if any parameters
// are left at their default values;
bool initializeParameters() {
  DBG_(Dev, (<< " initializing Parameters..."));

  theBPWarmCfg.Cores.initialize(getSystemWidth());
  theBPWarmCfg.BTBSets.initialize(512);
  theBPWarmCfg.BTBWays.initialize(4);

  //  theFeederCfg.SimicsQuantum.initialize(100);
  theFeederCfg.CMPWidth.initialize(getSystemWidth());
  theFeederCfg.HousekeepingPeriod.initialize(1000);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1DCfg.MTWidth.initialize(1);
  theL1DCfg.Size.initialize(32 * K);
  theL1DCfg.Associativity.initialize(2);
  theL1DCfg.BlockSize.initialize(64);
  theL1DCfg.CleanEvictions.initialize(false);
  theL1DCfg.CacheLevel.initialize(eL1);
  theL1DCfg.NotifyReads.initialize(false);
  theL1DCfg.NotifyWrites.initialize(false);

  theL1ICfg.MTWidth.initialize(1);
  theL1ICfg.Size.initialize(32 * K);
  theL1ICfg.Associativity.initialize(2);
  theL1ICfg.BlockSize.initialize(64);
  theL1ICfg.CleanEvictions.initialize(false);
  theL1ICfg.CacheLevel.initialize(eL1I);
  theL1ICfg.NotifyReads.initialize(false);
  theL1ICfg.NotifyWrites.initialize(false);

  theL2Cfg.CMPWidth.initialize(getSystemWidth());
  theL2Cfg.Size.initialize(8 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.CleanEvictions.initialize(false);
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.TraceTracker.initialize(false);
  theL2Cfg.SeparateID.initialize(true);


  theMMUCfg.iTLBSize.initialize(64);
  theMMUCfg.dTLBSize.initialize(64);

  theFlexus->setStatInterval("10000000");     // 10M
  theFlexus->setProfileInterval("10000000");  // 10M
  theFlexus->setTimestampInterval("1000000"); // 1M

  return false; // Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()

// clang-format off
#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
// All component Instances are created here.  This section
// also creates handles for each component

FLEXUS_INSTANTIATE_COMPONENT( DecoupledFeeder, theFeederCfg, theFeeder );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( BPWarm, theBPWarmCfg, theBPWarm, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1DCfg, theL1D, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FastCache, theL1ICfg, theL1I, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT( FastCMPCache, theL2Cfg, theL2 );
FLEXUS_INSTANTIATE_COMPONENT( FastMemoryLoopback, theMemoryCfg, theMemory );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MMU , theMMUCfg, theMMU, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);

//FLEXUS_INSTANTIATE_COMPONENT( WhiteBox, theWhiteBoxCfg, theWhiteBox );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==
WIRE(theFeeder, ToL1D,                  theL1D, RequestIn)
WIRE(theFeeder, ToL1I,                  theL1I, FetchRequestIn)
WIRE(theFeeder, ToBPred,                theBPWarm, ITraceInModern)
WIRE(theFeeder, ToMMU,                  theMMU, TLBReqIn)

WIRE(theL1D, RequestOut,                theL2, RequestIn)
WIRE(theL1I, RequestOut,                theL2, FetchRequestIn)

WIRE(theL2, SnoopOutI,                  theL1I, SnoopIn)
WIRE(theL2, SnoopOutD,                  theL1D, SnoopIn)

WIRE(theL2, RequestOut,                 theMemory, FromCache)

WIRE(theMemory, ToCache,                theL2, SnoopIn)

//Fetch, L1I and Execute


#include FLEXUS_END_COMPONENT_WIRING_SECTION()

#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theL1D, UpdateStatsDrive )
, DRIVE( theL1I, UpdateStatsDrive )
, DRIVE( theL2, UpdateStatsDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()
    // clang-format on