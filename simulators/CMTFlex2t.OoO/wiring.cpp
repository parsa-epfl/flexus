

#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "CMTFlex.2t.OoO v1.0";
}

#define THREADS_PER_CORE   2

#define FLEXUS_RuntimeDbgSev_CMPCache Verb
#define FLEXUS_RuntimeDbgSev_Cache Verb
#define FLEXUS_RuntimeDbgSev_uArch Verb

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include<components/CMPCache/CMPCache.hpp>
#include<components/Cache/Cache.hpp>
#include<components/MemoryLoopback/MemoryLoopback.hpp>
#include<components/MemoryMap/MemoryMap.hpp>
#include<components/MagicBreak/MagicBreak.hpp>
#include<components/FetchAddressGenerate/FetchAddressGenerate.hpp>
#include<components/uFetch/uFetch.hpp>
#include<components/v9Decoder/v9Decoder.hpp>
#include<components/uArch/uArch.hpp>
#include<components/TraceTracker/TraceTrackerComponent.hpp>
#include<components/MTManager/MTManagerComponent.hpp>

#include FLEXUS_END_DECLARATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( FetchAddressGenerate, "fag", theFAGCfg );
CREATE_CONFIGURATION( uFetch, "ufetch", theuFetchCfg );
CREATE_CONFIGURATION( v9Decoder, "decoder", theDecoderCfg );
CREATE_CONFIGURATION( uArch, "uarch", theuArchCfg );

CREATE_CONFIGURATION( Cache, "L1d", theL1dCfg );

CREATE_CONFIGURATION( CMPCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( MemoryLoopback, "memory", theMemoryCfg );
CREATE_CONFIGURATION( MemoryMap, "memory-map", theMemoryMapCfg );
CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );
CREATE_CONFIGURATION( TraceTrackerComponent, "trace-tracker", theTraceTrackerCfg );
CREATE_CONFIGURATION( MTManagerComponent, "cmt", theMTManagerCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  DBG_( Dev, ( << " initializing Parameters..." ) );

  theFAGCfg.Threads.initialize( THREADS_PER_CORE );
  theFAGCfg.MaxFetchAddress.initialize(10);
  theFAGCfg.MaxBPred.initialize(2);

  theuFetchCfg.Threads.initialize( THREADS_PER_CORE );
  theuFetchCfg.FAQSize.initialize(32);
  theuFetchCfg.MaxFetchLines.initialize(2);
  theuFetchCfg.MaxFetchInstructions.initialize(10);
  theuFetchCfg.ICacheLineSize.initialize(64);
  theuFetchCfg.PerfectICache.initialize(false);
  theuFetchCfg.PrefetchEnabled.initialize(true);
  theuFetchCfg.Size.initialize(65536);
  theuFetchCfg.Associativity.initialize(4);
  theuFetchCfg.MissQueueSize.initialize(4);

  theDecoderCfg.Multithread.initialize(true);
  theDecoderCfg.FIQSize.initialize(16);
  theDecoderCfg.DispatchWidth.initialize(8);

  theuArchCfg.Multithread.initialize(true);
  theuArchCfg.ROBSize.initialize(256);
  theuArchCfg.SBSize.initialize(64);
  theuArchCfg.RetireWidth.initialize(8);
  theuArchCfg.SnoopPorts.initialize(1);
  theuArchCfg.MemoryPorts.initialize(4);
  theuArchCfg.StorePrefetches.initialize(30);
  theuArchCfg.ConsistencyModel.initialize(1); //TSO
  theuArchCfg.CoherenceUnit.initialize(64);
  theuArchCfg.BreakOnResynchronize.initialize(false);
  theuArchCfg.SpinControl.initialize(true);
  theuArchCfg.SpeculativeOrder.initialize(false);
  theuArchCfg.SpeculateOnAtomicValue.initialize(false);
  theuArchCfg.SpeculateOnAtomicValuePerfect.initialize(false);
  theuArchCfg.SpeculativeCheckpoints.initialize(0);
  theuArchCfg.CheckpointThreshold.initialize(0);
  theuArchCfg.EarlySGP.initialize(false); // CMU-ONLY
  theuArchCfg.InOrderMemory.initialize(false);
  theuArchCfg.InOrderExecute.initialize(false);
  theuArchCfg.TrackParallelAccesses.initialize(false); // CMU-ONLY
  theuArchCfg.OffChipLatency.initialize(400);  //Roughly 100ns or two memory round-trips
  theuArchCfg.OnChipLatency.initialize(3);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1dCfg.Cores.initialize( THREADS_PER_CORE );
  theL1dCfg.Size.initialize(64 * K);
  theL1dCfg.Associativity.initialize(2);
  theL1dCfg.BlockSize.initialize(64);
  theL1dCfg.TagLatency.initialize(0);
  theL1dCfg.Ports.initialize(4);
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
  theL1dCfg.EvictClean.initialize(true);
  theL1dCfg.MAFTargetsPerRequest.initialize(0);
  theL1dCfg.PreQueueSizes.initialize(4);
  theL1dCfg.FastEvictClean.initialize(false);
  theL1dCfg.NoBus.initialize(false);
  theL1dCfg.TraceAddress.initialize(0);
  theL1dCfg.Banks.initialize(1);

  theL2Cfg.Cores.initialize( getSystemWidth() / THREADS_PER_CORE );
  theL2Cfg.Size.initialize(16 * M);
  theL2Cfg.Associativity.initialize(8);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.Ports.initialize(1);
  theL2Cfg.TagLatency.initialize(10);
  theL2Cfg.TagIssueLatency.initialize(2);
  theL2Cfg.DuplicateTagLatency.initialize(2);
  theL2Cfg.DuplicateTagIssueLatency.initialize(2);
  theL2Cfg.DataLatency.initialize(12);
  theL2Cfg.DataIssueLatency.initialize(4);
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.QueueSizes.initialize(16);
  theL2Cfg.MAFSize.initialize(40);
  theL2Cfg.EvictBufferSize.initialize(16);
  theL2Cfg.ProbeFetchMiss.initialize(true);
  theL2Cfg.EvictClean.initialize(false);
  theL2Cfg.CacheType.initialize("piranha");
  theL2Cfg.BusTime_NoData.initialize(1);
  theL2Cfg.BusTime_Data.initialize(2);
  theL2Cfg.Banks.initialize(1);
  theL2Cfg.MAFTargetsPerRequest.initialize(0);
  theL2Cfg.PreQueueSizes.initialize(4);
  theL2Cfg.TraceAddress.initialize(0);

  theMemoryCfg.Delay.initialize(240);
  theMemoryCfg.MaxRequests.initialize(64);

  theMemoryMapCfg.PageSize.initialize(8 * K);
  theMemoryMapCfg.NumNodes.initialize( getSystemWidth() );
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

  theTraceTrackerCfg.Enable.initialize(true);
  theTraceTrackerCfg.NumNodes.initialize( getSystemWidth() );
  theTraceTrackerCfg.BlockSize.initialize(64);
  theTraceTrackerCfg.Sharing.initialize(false);
  theTraceTrackerCfg.SharingLevel.initialize(eL2);

  theMTManagerCfg.Cores.initialize( getSystemWidth() / THREADS_PER_CORE ) ;
  theMTManagerCfg.Threads.initialize( THREADS_PER_CORE ) ;
  theMTManagerCfg.FrontEndPolicy.initialize( kFE_ICount ) ;
  theMTManagerCfg.BackEndPolicy.initialize( kBE_SmartRoundRobin ) ;

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FetchAddressGenerate, theFAGCfg, theFAG, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, THREADS_PER_CORE);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uFetch, theuFetchCfg, theuFetch, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, THREADS_PER_CORE);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( v9Decoder, theDecoderCfg, theDecoder, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uArch, theuArchCfg, theuArch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, DIVIDE,   THREADS_PER_CORE );

FLEXUS_INSTANTIATE_COMPONENT( CMPCache, theL2Cfg, theL2 );
FLEXUS_INSTANTIATE_COMPONENT( MemoryLoopback, theMemoryCfg, theMemory );
FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
FLEXUS_INSTANTIATE_COMPONENT( TraceTrackerComponent, theTraceTrackerCfg, theTraceTracker );
FLEXUS_INSTANTIATE_COMPONENT( MTManagerComponent, theMTManagerCfg, theMTManager);

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()


//FROM                                  TO
//====                                  ==
//FAG to Fetch
WIRE( theFAG, FetchAddrOut,             theuFetch, FetchAddressIn         )
WIRE( theFAG, AvailableFAQ,             theuFetch, AvailableFAQOut        )

//Fetch to Decoder
WIRE( theuFetch, AvailableFIQ,          theDecoder, AvailableFIQOut       )
WIRE( theuFetch, FetchBundleOut,        theDecoder, FetchBundleIn         )
WIRE( theDecoder, SquashOut,            theuFetch, SquashIn               )
WIRE( theuArch, ChangeCPUState,         theuFetch, ChangeCPUState         )

//Decoder to uArch
WIRE( theDecoder, AvailableDispatchIn,  theuArch, AvailableDispatchOut    )
WIRE( theDecoder, DispatchOut,          theuArch, DispatchIn              )
WIRE( theuArch, SquashOut,              theDecoder, SquashIn              )

//uArch to FAG
WIRE( theuArch, BranchFeedbackOut,      theFAG, BranchFeedbackIn          )
WIRE( theuArch, RedirectOut,            theFAG, RedirectIn                )

//uFetch to L1 I cache
WIRE( theuFetch, FetchMissOut,          theL2, FrontSideInI_Request       )
WIRE( theuFetch, FetchSnoopOut,         theL2, FrontSideInI_Snoop         )
WIRE( theL2, FrontSideOutI,             theuFetch, FetchMissIn            )

//uArch to L1 D cache
WIRE( theuArch, MemoryOut_Request,      theL1d, FrontSideIn_Request       )
WIRE( theuArch, MemoryOut_Snoop,        theL1d, FrontSideIn_Snoop         )
WIRE( theL1d, FrontSideOut,             theuArch, MemoryIn                )

//L1 D-cache to L2
WIRE( theL1d, BackSideOut_Request,      theL2, FrontSideInD_Request       )
WIRE( theL1d, BackSideOut_Snoop,        theL2, FrontSideInD_Snoop         )
WIRE( theL2, FrontSideOutD,             theL1d, BackSideIn                )

//L2 to Memory
WIRE( theL2, BackSideOut_Request,       theMemory, LoopbackIn             )
WIRE( theMemory, LoopbackOut,           theL2, BackSideIn                 )

//CMT scheduling
WIRE( theMTManager, EXStalled,          theuArch, Stalled                 )
WIRE( theMTManager, DStalled,           theDecoder, Stalled               )
WIRE( theMTManager, FStalled,           theuFetch, Stalled                )
WIRE( theMTManager, FAGStalled,         theFAG, Stalled                   )
WIRE( theMTManager, FAQ_ICount,         theuFetch, ICount                 )
WIRE( theMTManager, FIQ_ICount,         theDecoder, ICount                )
WIRE( theMTManager, ROB_ICount,         theuArch, ICount                  )


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theuFetch, uFetchDrive )
, DRIVE( theFAG, FAGDrive )
, DRIVE( theuArch, uArchDrive )
, DRIVE( theDecoder, DecoderDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theL2, CacheDrive )
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

