


#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "CMP.L2SharedNUCA.Inorder v1.0";
}


#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/InorderSimicsFeeder/InorderSimicsFeeder.hpp>
#include <components/BPWarm/BPWarm.hpp>
#include <components/CMPCache/CMPCache.hpp>
#include <components/MemoryLoopback/MemoryLoopback.hpp>
#include <components/MemoryMap/MemoryMap.hpp>
#include <components/MagicBreak/MagicBreak.hpp>
#include <components/IFetch/IFetch.hpp>
#include <components/Execute/Execute.hpp>
#include <components/Cache/Cache.hpp>
#include <components/NetShim/MemoryNetwork.hpp>
#include <components/MultiNic/MultiNic2.hpp>
#include <components/VMMapper/VMMapper.hpp>
#include <components/SplitDestinationMapper/SplitDestinationMapper.hpp>
#include <components/WhiteBox/WhiteBox.hpp>

#include FLEXUS_END_DECLARATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( WhiteBox, "white-box", theWhiteBoxCfg );

CREATE_CONFIGURATION( InorderSimicsFeeder, "feeder", theFeederCfg );
CREATE_CONFIGURATION( BPWarm, "bpwarm", theBPWarmCfg );
CREATE_CONFIGURATION( IFetch, "fetch", theFetchCfg );
CREATE_CONFIGURATION( Execute, "execute", theExecuteCfg );
CREATE_CONFIGURATION( Cache, "L1i", theL1iCfg );
CREATE_CONFIGURATION( Cache, "L1d", theL1dCfg );

CREATE_CONFIGURATION( CMPCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( MultiNic2, "nic", theNicCfg );
CREATE_CONFIGURATION( MemoryNetwork, "network", theNetworkCfg );

CREATE_CONFIGURATION( SplitDestinationMapper, "net-mapper", theNetMapperCfg );

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
  theFeederCfg.StallCap.initialize(0);

  theBPWarmCfg.Cores.initialize(1);

  theFetchCfg.StallInstructions.initialize(true);

  theExecuteCfg.SBSize.initialize(64);
  theExecuteCfg.ROBSize.initialize(64);
  theExecuteCfg.LSQSize.initialize(32);
  theExecuteCfg.MemoryWidth.initialize(4);
  theExecuteCfg.SequentialConsistency.initialize(true);
  theExecuteCfg.StorePrefetching.initialize(false);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1iCfg.Cores.initialize( 1 );
  theL1iCfg.BlockSize.initialize(64);
  theL1iCfg.Ports.initialize(4);
  theL1iCfg.PreQueueSizes.initialize(4);
  theL1iCfg.TagLatency.initialize(0);
  theL1iCfg.TagIssueLatency.initialize(1);
  theL1iCfg.DataLatency.initialize(2);
  theL1iCfg.DataIssueLatency.initialize(1);
  theL1iCfg.CacheLevel.initialize(eL1I);
  theL1iCfg.QueueSizes.initialize(8);
  theL1iCfg.MAFSize.initialize(32);
  theL1iCfg.EvictBufferSize.initialize(8);
  theL1iCfg.SnoopBufferSize.initialize(8);
  theL1iCfg.ProbeFetchMiss.initialize(false);
  theL1iCfg.EvictClean.initialize(true);
  theL1iCfg.BusTime_NoData.initialize(1);
  theL1iCfg.BusTime_Data.initialize(2);
  theL1iCfg.MAFTargetsPerRequest.initialize(0);
  theL1iCfg.FastEvictClean.initialize(false);
  theL1iCfg.NoBus.initialize(false);
  theL1iCfg.Banks.initialize(1);
  theL1iCfg.TraceAddress.initialize(0);
  theL1iCfg.CacheType.initialize("InclMESI2PhaseWB");
  theL1iCfg.ArrayConfiguration.initialize("STD:size=65536:assoc=4:repl=LRU");
  theL1iCfg.EvictOnSnoop.initialize(true);
  theL1iCfg.UseReplyChannel.initialize(true);

  theL1dCfg.Cores.initialize( 1 );
  theL1dCfg.BlockSize.initialize(64);
  theL1dCfg.Ports.initialize(4);
  theL1dCfg.PreQueueSizes.initialize(4);
  theL1dCfg.TagLatency.initialize(0);
  theL1dCfg.TagIssueLatency.initialize(1);
  theL1dCfg.DataLatency.initialize(2);
  theL1dCfg.DataIssueLatency.initialize(1);
  theL1dCfg.CacheLevel.initialize(eL1);
  theL1dCfg.QueueSizes.initialize(8);
  theL1dCfg.MAFSize.initialize(32);
  theL1dCfg.EvictBufferSize.initialize(8);
  theL1dCfg.SnoopBufferSize.initialize(8);
  theL1dCfg.ProbeFetchMiss.initialize(false);
  theL1dCfg.EvictClean.initialize(true);
  theL1dCfg.BusTime_NoData.initialize(1);
  theL1dCfg.BusTime_Data.initialize(2);
  theL1dCfg.MAFTargetsPerRequest.initialize(0);
  theL1dCfg.FastEvictClean.initialize(false);
  theL1dCfg.NoBus.initialize(false);
  theL1dCfg.Banks.initialize(1);
  theL1dCfg.TraceAddress.initialize(0);
  theL1dCfg.CacheType.initialize("InclMESI2PhaseWB");
  theL1dCfg.ArrayConfiguration.initialize("STD:size=65536:assoc=4:repl=LRU");
  theL1dCfg.EvictOnSnoop.initialize(true);
  theL1dCfg.UseReplyChannel.initialize(true);

  theL2Cfg.Cores.initialize(64);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.Banks.initialize(64);
  theL2Cfg.BankInterleaving.initialize(64);
  theL2Cfg.Groups.initialize(1);
  theL2Cfg.GroupInterleaving.initialize(4096);
  theL2Cfg.DirLatency.initialize(3);
  theL2Cfg.DirIssueLatency.initialize(1);
  theL2Cfg.TagLatency.initialize(3);
  theL2Cfg.TagIssueLatency.initialize(1);
  theL2Cfg.DataLatency.initialize(7);
  theL2Cfg.DataIssueLatency.initialize(1);
  theL2Cfg.QueueSize.initialize(8);
  theL2Cfg.MAFSize.initialize(32);
  theL2Cfg.DirEvictBufferSize.initialize(16);
  theL2Cfg.CacheEvictBufferSize.initialize(16);
  theL2Cfg.Policy.initialize("NonInclusiveMESI");
  theL2Cfg.DirectoryType.initialize("infinite");
  theL2Cfg.DirectoryConfig.initialize("");
  theL2Cfg.CacheLevel.initialize(eL2);
  theL2Cfg.EvictClean.initialize(false);
  theL2Cfg.ArrayConfiguration.initialize("STD:size=1048576:assoc=16:repl=LRU");

  theNicCfg.VChannels.initialize(3);
  theNicCfg.RecvCapacity.initialize(4);
  theNicCfg.SendCapacity.initialize(1);

  //theNetworkCfg.NetworkTopologyFile.initialize("16node-torus.topology");
  theNetworkCfg.NetworkTopologyFile.initialize("64x3-mesh.topology");
  theNetworkCfg.NumNodes.initialize( 3 * getSystemWidth() );
  theNetworkCfg.VChannels.initialize( 3 );

  theNetMapperCfg.Cores.initialize(16);
  theNetMapperCfg.Directories.initialize(16);
  theNetMapperCfg.MemControllers.initialize(4);
  theNetMapperCfg.DirInterleaving.initialize(64);
  theNetMapperCfg.MemInterleaving.initialize(4096);
  theNetMapperCfg.DirLocation.initialize("Distributed");
  theNetMapperCfg.MemLocation.initialize("4,7,8,11");
  theNetMapperCfg.MemReplyToDir.initialize(true);
  theNetMapperCfg.MemAcksNeedData.initialize(true);

  theMemoryCfg.Delay.initialize(240);
  theMemoryCfg.MaxRequests.initialize(128);
  theMemoryCfg.UseFetchReply.initialize(true);

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

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
//All component Instances are created here.  This section
//also creates handles for each component
FLEXUS_INSTANTIATE_COMPONENT( WhiteBox, theWhiteBoxCfg, theWhiteBox );

FLEXUS_INSTANTIATE_COMPONENT( InorderSimicsFeeder, theFeederCfg, theFeeder );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1iCfg, theL1i, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( IFetch, theFetchCfg, theFetch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Execute, theExecuteCfg, theExecute, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( BPWarm, theBPWarmCfg, theBPWarm, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1  );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CMPCache, theL2Cfg, theL2, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, 1 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MemoryLoopback, theMemoryCfg, theMemory, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, 4 );
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MultiNic2, theNicCfg, theNic, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 3 );
FLEXUS_INSTANTIATE_COMPONENT( MemoryNetwork, theNetworkCfg, theNetwork );
FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
FLEXUS_INSTANTIATE_COMPONENT( SplitDestinationMapper, theNetMapperCfg, theNetMapper );

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()



#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

//FROM                                  TO
//====                                  ==
//Fetch, L1I and Execute
WIRE( theFetch, FetchIn,                theFeeder, InstructionOutputPort )
WIRE( theFetch, FetchOut,               theBPWarm, InsnIn                )
WIRE( theBPWarm, InsnOut,               theExecute, ExecuteIn            )
WIRE( theFetch, IMemOut,                theL1i, FrontSideIn_Request      )
WIRE( theFetch, IMemSnoopOut,           theL1i, FrontSideIn_Snoop        )

//Execute component and L1 caches
WIRE( theL1i, FrontSideOut_I,           theFetch, IMemIn                 )
WIRE( theExecute, ExecuteMemRequest,    theL1d, FrontSideIn_Request      )
WIRE( theL1d, FrontSideOut_D,           theExecute, ExecuteMemReply      )
WIRE( theExecute, ExecuteMemSnoop,      theL1d, FrontSideIn_Snoop        )

//L1 I-cache to L2
WIRE( theL1i, BackSideOut_Request,      theNetMapper, ICacheRequestIn    )
WIRE( theL1i, BackSideOut_Snoop,        theNetMapper, ICacheSnoopIn      )
WIRE( theL1i, BackSideOut_Reply,        theNetMapper, ICacheReplyIn      )

WIRE( theNetMapper, ICacheSnoopOut,     theL1i, BackSideIn_Request       )
WIRE( theNetMapper, ICacheReplyOut,     theL1i, BackSideIn_Reply         )


//L1 D-cache to L2
WIRE( theL1d, BackSideOut_Request,      theNetMapper, CacheRequestIn     )
WIRE( theL1d, BackSideOut_Snoop,        theNetMapper, CacheSnoopIn       )
WIRE( theL1d, BackSideOut_Reply,        theNetMapper, CacheReplyIn       )

WIRE( theNetMapper, CacheSnoopOut,      theL1d, BackSideIn_Request       )
WIRE( theNetMapper, CacheReplyOut,      theL1d, BackSideIn_Reply         )

WIRE( theNetMapper, MemoryOut,          theMemory, LoopbackIn            )
WIRE( theMemory, LoopbackOut,           theNetMapper, MemoryIn           )

//Directory to NetMapper
WIRE( theNetMapper, DirReplyOut,        theL2, Reply_In                  )
WIRE( theNetMapper, DirSnoopOut,        theL2, Snoop_In                  )
WIRE( theNetMapper, DirRequestOut,      theL2, Request_In                )
WIRE( theL2, Reply_Out,                 theNetMapper, DirReplyIn         )
WIRE( theL2, Snoop_Out,                 theNetMapper, DirSnoopIn         )
WIRE( theL2, Request_Out,               theNetMapper, DirRequestIn       )

//NetMapper to Nic
WIRE( theNetMapper, ToNIC0,             theNic, FromNode0                )
WIRE( theNic, ToNode0,                  theNetMapper, FromNIC0           )
WIRE( theNetMapper, ToNIC1,             theNic, FromNode1                )
WIRE( theNic, ToNode1,                  theNetMapper, FromNIC1           )

//Nodes to Network
WIRE( theNic, ToNetwork,                theNetwork, FromNode             )
WIRE( theNetwork, ToNode,               theNic, FromNetwork              )

#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theFetch, FeederDrive )
, DRIVE( theL1i, CacheDrive )
, DRIVE( theFetch, PipelineDrive )
, DRIVE( theExecute, ExecuteDrive )
, DRIVE( theNic, MultiNicDrive )
, DRIVE( theNetwork, NetworkDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theExecute, CommitDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theL2, CMPCacheDrive )
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

