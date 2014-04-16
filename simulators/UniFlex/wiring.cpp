


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "UniFlex v3.0";
}


#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/InorderSimicsFeeder/InorderSimicsFeeder.hpp>
#include <components/BPWarm/BPWarm.hpp>
#include <components/MemoryLoopback/MemoryLoopback.hpp>
#include <components/MemoryMap/MemoryMap.hpp>
#include <components/MagicBreak/MagicBreak.hpp>
#include <components/IFetch/IFetch.hpp>
#include <components/Execute/Execute.hpp>
#include <components/Cache/Cache.hpp>
#include <components/Cache/IDCacheMux.hpp>

#include FLEXUS_END_DECLARATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( InorderSimicsFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( BPWarm, "bpwarm", theBPWarmCfg );
CREATE_CONFIGURATION( IFetch, "fetch", theFetchCfg );
CREATE_CONFIGURATION( Execute, "execute", theExecuteCfg );
CREATE_CONFIGURATION( Cache, "L1i", theL1iCfg );
CREATE_CONFIGURATION( Cache, "L1d", theL1dCfg );
CREATE_CONFIGURATION( IDCacheMux, "mux", theMuxCfg );

CREATE_CONFIGURATION( Cache, "L2", theL2Cfg );
CREATE_CONFIGURATION( MemoryLoopback, "memory", theMemoryCfg );
CREATE_CONFIGURATION( MemoryMap, "memory-map", theMemoryMapCfg );
CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  DBG_( Dev, ( << " initializing Parameters..." ) );

  DBG_Assert( getSystemWidth() == 1 );

  theFeederCfg.TraceFile.initialize("");
  theFeederCfg.UseTrace.initialize(false);
  theFeederCfg.StallCap.initialize(5000);

  theBPWarmCfg.Cores.initialize(1);

  theFetchCfg.StallInstructions.initialize(true);

  theExecuteCfg.SBSize.initialize(64);
  theExecuteCfg.ROBSize.initialize(64);
  theExecuteCfg.LSQSize.initialize(32);
  theExecuteCfg.MemoryWidth.initialize(4);
  if ( FLEXUS_TARGET_IS(v9) ) {
    theExecuteCfg.SequentialConsistency.initialize(false);
    theExecuteCfg.StorePrefetching.initialize(true);
  } else {
    //x86 only supports SequentialConsistency
    theExecuteCfg.SequentialConsistency.initialize(true);
    theExecuteCfg.StorePrefetching.initialize(false);
  }


  static const int K = 1024;
  static const int M = 1024 * K;

  theL1iCfg.Cores.initialize( 1 );
  theL1iCfg.Size.initialize(64 * K);
  theL1iCfg.Associativity.initialize(2);
  theL1iCfg.BlockSize.initialize(64);
  theL1iCfg.Ports.initialize(1);
  theL1iCfg.TagLatency.initialize(0);
  theL1iCfg.TagIssueLatency.initialize(1);
  theL1iCfg.DataLatency.initialize(0);
  theL1iCfg.DataIssueLatency.initialize(1);
  theL1iCfg.CacheLevel.initialize(eL1I);
  theL1iCfg.QueueSizes.initialize(8);
  theL1iCfg.MAFSize.initialize(32);
  theL1iCfg.EvictBufferSize.initialize(8);
  theL1iCfg.ProbeFetchMiss.initialize(false);
  theL1iCfg.BusTime_NoData.initialize(1);
  theL1iCfg.BusTime_Data.initialize(2);
  theL1iCfg.EvictClean.initialize(false);
  theL1iCfg.FastEvictClean.initialize(false);
  theL1iCfg.MAFTargetsPerRequest.initialize(0);
  theL1iCfg.PreQueueSizes.initialize(4);
  theL1iCfg.Banks.initialize(1);
  theL1iCfg.NoBus.initialize(false);
  theL1iCfg.TraceAddress.initialize(0);

  theL1dCfg.Cores.initialize( 1 );
  theL1dCfg.Size.initialize(64 * K);
  theL1dCfg.Associativity.initialize(2);
  theL1dCfg.BlockSize.initialize(64);
  theL1dCfg.Ports.initialize(1);
  theL1dCfg.TagLatency.initialize(0);
  theL1dCfg.TagIssueLatency.initialize(1);
  theL1dCfg.DataLatency.initialize(0);
  theL1dCfg.DataIssueLatency.initialize(1);
  theL1dCfg.CacheLevel.initialize(eL1);
  theL1dCfg.QueueSizes.initialize(8);
  theL1dCfg.MAFSize.initialize(32);
  theL1dCfg.EvictBufferSize.initialize(8);
  theL1dCfg.ProbeFetchMiss.initialize(false);
  theL1dCfg.BusTime_NoData.initialize(1);
  theL1dCfg.BusTime_Data.initialize(2);
  theL1dCfg.EvictClean.initialize(false);
  theL1dCfg.FastEvictClean.initialize(false);
  theL1dCfg.MAFTargetsPerRequest.initialize(0);
  theL1dCfg.PreQueueSizes.initialize(4);
  theL1dCfg.Banks.initialize(1);
  theL1dCfg.NoBus.initialize(false);
  theL1dCfg.TraceAddress.initialize(0);


  theL2Cfg.Cores.initialize( 1 );
  theL2Cfg.Size.initialize(8 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.Ports.initialize(1);
  theL2Cfg.TagLatency.initialize(10);
  theL2Cfg.TagIssueLatency.initialize(2);
  theL2Cfg.DataLatency.initialize(12);
  theL2Cfg.DataIssueLatency.initialize(4);
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.QueueSizes.initialize(16);
  theL2Cfg.MAFSize.initialize(40);
  theL2Cfg.EvictBufferSize.initialize(16);
  theL2Cfg.ProbeFetchMiss.initialize(true);
  theL2Cfg.BusTime_NoData.initialize(1);
  theL2Cfg.BusTime_Data.initialize(2);
  theL2Cfg.EvictClean.initialize(false);
  theL2Cfg.FastEvictClean.initialize(false);
  theL2Cfg.MAFTargetsPerRequest.initialize(0);
  theL2Cfg.PreQueueSizes.initialize(4);
  theL2Cfg.Banks.initialize(1);
  theL2Cfg.NoBus.initialize(false);
  theL2Cfg.TraceAddress.initialize(0);

  theMemoryCfg.Delay.initialize(240);
  theMemoryCfg.MaxRequests.initialize(64);

  theMemoryMapCfg.PageSize.initialize(8 * K);
  theMemoryMapCfg.NumNodes.initialize( 1 );
  theMemoryMapCfg.RoundRobin.initialize(true);
  theMemoryMapCfg.CreatePageMap.initialize(true);
  theMemoryMapCfg.ReadPageMap.initialize(true);

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

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
//All component Instances are created here.  This section
//also creates handles for each component


FLEXUS_INSTANTIATE_COMPONENT( InorderSimicsFeeder, theFeederCfg, theFeeder );
FLEXUS_INSTANTIATE_COMPONENT( BPWarm, theBPWarmCfg, theBPWarm );
FLEXUS_INSTANTIATE_COMPONENT( Cache, theL1dCfg, theL1d);
FLEXUS_INSTANTIATE_COMPONENT( Cache, theL1iCfg, theL1i);
FLEXUS_INSTANTIATE_COMPONENT( IDCacheMux, theMuxCfg, theMux);
FLEXUS_INSTANTIATE_COMPONENT( IFetch, theFetchCfg, theFetch);
FLEXUS_INSTANTIATE_COMPONENT( Execute, theExecuteCfg, theExecute );
FLEXUS_INSTANTIATE_COMPONENT( Cache, theL2Cfg, theL2 );
FLEXUS_INSTANTIATE_COMPONENT( MemoryLoopback, theMemoryCfg, theMemory );
FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );


#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==
//Fetch, L1I and Execute
WIRE( theFetch, FetchIn,                theFeeder, InstructionOutputPort  )
WIRE( theFetch, FetchOut,               theBPWarm, InsnIn                 )
WIRE( theBPWarm, InsnOut,               theExecute, ExecuteIn             )
WIRE( theFetch, IMemOut,                theL1i, FrontSideIn_Request       )
WIRE( theFetch, IMemSnoopOut,           theL1i, FrontSideIn_Snoop         )

//Execute component and L1 caches
WIRE( theL1i, FrontSideOut,             theFetch, IMemIn                  )
WIRE( theExecute, ExecuteMemRequest,    theL1d, FrontSideIn_Request       )
WIRE( theL1d, FrontSideOut,             theExecute, ExecuteMemReply       )
WIRE( theExecute, ExecuteMemSnoop,      theL1d, FrontSideIn_Snoop         )

//L1 I-cache to Mux
WIRE( theL1i, BackSideOut_Request,      theMux, TopInI                    )
WIRE( theMux, TopOutI,                  theL1i, BackSideIn                )

//L1 D-cache to Mux/L2
WIRE( theL1d, BackSideOut_Request,      theMux, TopInD                    )
WIRE( theL1d, BackSideOut_Snoop,        theL2, FrontSideIn_Snoop          )
WIRE( theMux, TopOutD,                  theL1d, BackSideIn                )

//Mux to L2
WIRE( theMux, BottomOut,                theL2, FrontSideIn_Request        )
WIRE( theL2, FrontSideOut,              theMux, BottomIn                  )


//L2 to Memory
WIRE( theL2, BackSideOut_Request,       theMemory, LoopbackIn             )
WIRE( theMemory, LoopbackOut,           theL2, BackSideIn                 )


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theFetch, FeederDrive )
, DRIVE( theL1i, CacheDrive )
, DRIVE( theFetch, PipelineDrive )
, DRIVE( theExecute, ExecuteDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theExecute, CommitDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theL2, CacheDrive )
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

