


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "SGPDSMFlex v1.0";
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
#include <components/LocalEngine/LocalEngine.hpp>
#include <components/ProtocolEngine/ProtocolEngine.hpp>
#include <components/NetShim/NetShim.hpp>
#include <components/Nic/Nic1.hpp>
#include <components/Directory/Directory.hpp>
#include <components/Directory/DirMux2.hpp>

#include <components/SpatialPrefetcher/SpatialPrefetcher.hpp>
#include <components/TraceTracker/TraceTrackerComponent.hpp>
#include <components/SimplePrefetchController/SimplePrefetchController.hpp>
#include <components/StreamController/StreamController.hpp>
#include <components/Cache/Delay.hpp>

#include FLEXUS_END_DECLARATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( InorderSimicsFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( BPWarm, "bpwarm", theBPWarmCfg );
CREATE_CONFIGURATION( IFetch, "fetch", theFetchCfg );
CREATE_CONFIGURATION( Execute, "execute", theExecuteCfg );
CREATE_CONFIGURATION( Cache, "L1i", theL1iCfg );
CREATE_CONFIGURATION( Cache, "L1d", theL1dCfg );
CREATE_CONFIGURATION( Cache, "L2", theL2Cfg );
CREATE_CONFIGURATION( IDCacheMux, "idmux", theIDMuxCfg );
CREATE_CONFIGURATION( LocalEngine, "local_eng", theLocalEngineCfg );
CREATE_CONFIGURATION( ProtocolEngine, "prot_eng", theProtocolEngineCfg );
CREATE_CONFIGURATION( Directory, "directory", theDirectoryCfg );
CREATE_CONFIGURATION( DirMux2, "dirmux", theDirMuxCfg );
CREATE_CONFIGURATION( Nic1, "nic", theNicCfg );
CREATE_CONFIGURATION( NetShim, "network", theNetworkCfg );

CREATE_CONFIGURATION( SpatialPrefetcher, "spatial-prefetch", theSpatialPrefetcherCfg );
CREATE_CONFIGURATION( TraceTrackerComponent, "trace-tracker", theTraceTrackerCfg );
CREATE_CONFIGURATION( SimplePrefetchController, "l2prefetcher", theL2PrefetcherCfg );

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

  theFeederCfg.TraceFile.initialize("");
  theFeederCfg.UseTrace.initialize(false);

  theFetchCfg.StallInstructions.initialize(true);

  theExecuteCfg.SBSize.initialize(64);
  theExecuteCfg.ROBSize.initialize(64);
  theExecuteCfg.LSQSize.initialize(32);
  theExecuteCfg.MemoryWidth.initialize(4);
  theExecuteCfg.SequentialConsistency.initialize(false);
  theExecuteCfg.StorePrefetching.initialize(true);

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
  theL1dCfg.FastEvictClean.initialize(false);
  theL1iCfg.MAFTargetsPerRequest.initialize(0);
  theL1iCfg.PreQueueSizes.initialize(4);

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
  theL1dCfg.EvictClean.initialize(true);  //Needs to be true for SimplePrefetch dup tags
  theL1dCfg.FastEvictClean.initialize(false);
  theL1dCfg.MAFTargetsPerRequest.initialize(0);
  theL1dCfg.PreQueueSizes.initialize(4);


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


  theDirectoryCfg.Latency.initialize(240);
  theDirectoryCfg.FastLatency.initialize(1);
  theDirectoryCfg.NumBanks.initialize(64);
  theDirectoryCfg.CoheBlockSize.initialize(64);


  theProtocolEngineCfg.SpuriousSharerOpt.initialize(true);
  theProtocolEngineCfg.Remote.initialize(true);
  theProtocolEngineCfg.TSRFSize.initialize(64);
  theProtocolEngineCfg.VChannels.initialize(3);
  theProtocolEngineCfg.CPI.initialize(4);


  theNicCfg.VChannels.initialize(3);
  theNicCfg.RecvCapacity.initialize(4);
  theNicCfg.SendCapacity.initialize(1);

  theNetworkCfg.NetworkTopologyFile.initialize("16node-torus.topology");
  theNetworkCfg.NumNodes.initialize( getSystemWidth() );
  theNetworkCfg.VChannels.initialize( 3 );

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
  theMagicBreakCfg.TerminateOnEndOfParallel.initialize(true);
  theMagicBreakCfg.EnableIterationCounts.initialize(false);

  theSpatialPrefetcherCfg.UsageEnable.initialize(false);
  theSpatialPrefetcherCfg.RepetEnable.initialize(false);
  theSpatialPrefetcherCfg.TimeRepetEnable.initialize(false);
  theSpatialPrefetcherCfg.BufFetchEnable.initialize(false);
  theSpatialPrefetcherCfg.PrefetchEnable.initialize(false);
  theSpatialPrefetcherCfg.ActiveEnable.initialize(false);
  theSpatialPrefetcherCfg.CacheLevel.initialize(eL1);
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
  theSpatialPrefetcherCfg.BufSize.initialize(0);
  theSpatialPrefetcherCfg.StreamDescs.initialize(0);
  theSpatialPrefetcherCfg.DelayedCommits.initialize(false);

  theTraceTrackerCfg.Enable.initialize(true);
  theTraceTrackerCfg.NumNodes.initialize( getSystemWidth() );
  theTraceTrackerCfg.BlockSize.initialize(64);
  theTraceTrackerCfg.Sharing.initialize(false);
  theTraceTrackerCfg.SharingLevel.initialize(eL2);

  theL2PrefetcherCfg.MAFSize.initialize(32);
  theL2PrefetcherCfg.QueueSizes.initialize(4);
  theL2PrefetcherCfg.PrefetchQueueSize.initialize(257);
  theL2PrefetcherCfg.L1Size.initialize(65536);
  theL2PrefetcherCfg.L1Associativity.initialize(2);
  theL2PrefetcherCfg.L1BlockSize.initialize(64);

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
//All component Instances are created here.  This section
//also creates handles for each component


FLEXUS_INSTANTIATE_COMPONENT( InorderSimicsFeeder, theFeederCfg, theFeeder );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( IFetch, theFetchCfg, theFetch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Execute, theExecuteCfg, theExecute, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( BPWarm, theBPWarmCfg, theBPWarm, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1  );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1iCfg, theL1i, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( IDCacheMux, theIDMuxCfg, theIDMux, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL2Cfg, theL2, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1   );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( LocalEngine, theLocalEngineCfg, theLocalEngine, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( ProtocolEngine, theProtocolEngineCfg, theProtocolEngine, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( DirMux2, theDirMuxCfg, theDirMux, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Directory, theDirectoryCfg, theDirectory, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MemoryLoopback, theMemoryCfg, theMemory, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Nic1, theNicCfg, theNic, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT( NetShim, theNetworkCfg, theNetwork );
FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( SpatialPrefetcher, theSpatialPrefetcherCfg, theSpatial, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT( TraceTrackerComponent, theTraceTrackerCfg, theTraceTracker );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( SimplePrefetchController, theL2PrefetcherCfg, theL2P, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );

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

//L1I to IDMux
WIRE( theL1i, BackSideOut_Request,      theIDMux, TopInI                  )
WIRE( theIDMux, TopOutI,                theL1i, BackSideIn                )

//L1D to SVB
WIRE( theL1d, BackSideOut_Request,      theL2P, FrontSideIn_Request       )
WIRE( theL1d, BackSideOut_Snoop,        theL2P, FrontSideIn_Snoop         )
WIRE( theL2P, FrontSideOut,             theL1d, BackSideIn                )

//L2 Prefetcher to IDMux & L2
WIRE( theL2P, BackSideOut_Request,      theIDMux, TopInD                  )
WIRE( theL2P, BackSideOut_Snoop,        theL2, FrontSideIn_Snoop          )
WIRE( theIDMux, TopOutD,                theL2P, BackSideIn                )

//SpatialPrefetcher to L2 Prefetcher and StreamController
WIRE( theSpatial, PrefetchOut_1,        theL2P, MasterIn                  )
WIRE( theSpatial, PrefetchOut_2,        theSC, PredictorIn                )

//StreamController to SVB
//WIRE( theSC, SVBOut,                    theSVB, MasterIn                  )
//WIRE( theSVB, MasterOut,                theSC, SVBIn                      )

//IDMux to L2
WIRE( theIDMux, BottomOut,              theL2, FrontSideIn_Request        )
WIRE( theL2, FrontSideOut,              theIDMux, BottomIn                )

//L2 to LocalEngine
WIRE( theL2, BackSideOut_Request,       theLocalEngine, FromCache_Request )
WIRE( theL2, BackSideOut_Snoop,         theLocalEngine, FromCache_Snoop   )
WIRE( theLocalEngine, ToCache,          theL2, BackSideIn                 )

//LocalEngine to Memory
WIRE( theLocalEngine, ToMemory,         theMemory, LoopbackIn             )
WIRE( theMemory, LoopbackOut,           theLocalEngine, FromMemory        )

//LocalEngine to ProtocolEngines
WIRE( theLocalEngine, ToProtEngines,    theProtocolEngine, FromCpu        )
WIRE( theProtocolEngine, ToCpu,         theLocalEngine, FromProtEngines   )

//LocalEngine to DirMux
WIRE( theLocalEngine, ToDirectory,      theDirMux, TopIn1                 )
WIRE( theDirMux, TopOut1,               theLocalEngine, FromDirectory     )

//ProtocolEngine to DirMux
WIRE( theProtocolEngine, ToDirectory,   theDirMux, TopIn2                 )
WIRE( theDirMux, TopOut2,               theProtocolEngine, FromDirectory  )

//DirMux to Directory
WIRE( theDirMux, BottomOut,             theDirectory, DirRequestIn        )
WIRE( theDirectory, DirReplyOut,        theDirMux, BottomIn               )

//ProtocolEngine to Nic
WIRE( theProtocolEngine, ToNic,         theNic, FromNode0                 )
WIRE( theNic, ToNode0,                  theProtocolEngine, FromNic        )

//Nodes to Network
WIRE( theNic, ToNetwork,                theNetwork, FromNode              )
WIRE( theNetwork, ToNode,               theNic, FromNetwork               )

#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theNetwork, NetworkDrive )
, DRIVE( theProtocolEngine, ProtocolEngineDrive )
, DRIVE( theNic, NicDrive )
, DRIVE( theFetch, FeederDrive )
, DRIVE( theL1i, CacheDrive )
, DRIVE( theFetch, PipelineDrive )
, DRIVE( theExecute, ExecuteDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theDirectory, DirectoryDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theExecute, CommitDrive )
, DRIVE( theL2P, PrefetchDrive )
, DRIVE( theL2, CacheDrive )
, DRIVE( theSpatial, PrefetchDrive )
, DRIVE( theSC, StreamDrive )
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

