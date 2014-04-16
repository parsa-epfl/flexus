

#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>

#define NUM_L2_TILES              0
#define NUM_L2_TILES_PER_MC       4
#define NUM_CORES_PER_TORUS_ROW   4
#define SIZE_INSTR_CLUSTER        4
#define PAGE_SIZE                 8192
#define CACHE_LINE_SIZE           64

//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "CMPNUCAFlex.OoO v3.0";
}

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

#include<components/CmpCoreNetworkInterface/CmpCoreNetworkInterface.hpp>
#include<components/CmpL2NetworkInterface/CmpL2NetworkInterface.hpp>
#include<components/CmpNetworkControl/CmpNetworkControl.hpp>
#include <components/CmpNetShim/CmpNetShim.hpp>

#include <components/WhiteBox/WhiteBox.hpp>

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

CREATE_CONFIGURATION( CmpCoreNetworkInterface, "core-network-interface", theCmpCoreNetworkInterfaceCfg );
CREATE_CONFIGURATION( CmpL2NetworkInterface, "l2-network-interface", theCmpL2NetworkInterfaceCfg );
CREATE_CONFIGURATION( CmpNetworkControl, "network-control", theCmpNetworkControlCfg );
CREATE_CONFIGURATION( NetShim, "network", theNetworkCfg );

CREATE_CONFIGURATION( WhiteBox, "white-box", theWhiteBoxCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  DBG_( Dev, ( << " initializing Parameters..." ) );

  theFAGCfg.Threads.initialize( 1 );
  theFAGCfg.MaxFetchAddress.initialize(10);
  theFAGCfg.MaxBPred.initialize(2);

  theuFetchCfg.Threads.initialize( 1 );
  theuFetchCfg.FAQSize.initialize(32);
  theuFetchCfg.MaxFetchLines.initialize(2);
  theuFetchCfg.MaxFetchInstructions.initialize(10);
  theuFetchCfg.ICacheLineSize.initialize(CACHE_LINE_SIZE);
  theuFetchCfg.PerfectICache.initialize(false);
  theuFetchCfg.PrefetchEnabled.initialize(true);
  theuFetchCfg.Size.initialize(65536);
  theuFetchCfg.Associativity.initialize(4);
  theuFetchCfg.MissQueueSize.initialize(4);
  theuFetchCfg.CleanEvict.initialize( true );
  theuFetchCfg.DecoupleInstrDataSpaces.initialize(true); /* CMU-ONLY */

  theDecoderCfg.Multithread.initialize(false);
  theDecoderCfg.FIQSize.initialize(16);
  theDecoderCfg.DispatchWidth.initialize(8);

  theuArchCfg.Multithread.initialize(false);
  theuArchCfg.ROBSize.initialize(256);
  theuArchCfg.SBSize.initialize(64);
  theuArchCfg.RetireWidth.initialize(8);
  theuArchCfg.SnoopPorts.initialize(1);
  theuArchCfg.MemoryPorts.initialize(4);
  theuArchCfg.StorePrefetches.initialize(30);
  theuArchCfg.ConsistencyModel.initialize(1); //TSO
  theuArchCfg.CoherenceUnit.initialize(CACHE_LINE_SIZE);
  theuArchCfg.BreakOnResynchronize.initialize(false);
  theuArchCfg.SpinControl.initialize(true);
  theuArchCfg.SpeculativeOrder.initialize(false);
  theuArchCfg.SpeculateOnAtomicValue.initialize(false);
  theuArchCfg.SpeculateOnAtomicValuePerfect.initialize(false);
  theuArchCfg.SpeculativeCheckpoints.initialize(false);
  theuArchCfg.PrefetchEarly.initialize(false);
  theuArchCfg.CheckpointThreshold.initialize(0);
  theuArchCfg.EarlySGP.initialize(false); /* CMU-ONLY */
  theuArchCfg.TrackParallelAccesses.initialize(false); /* CMU-ONLY */
  theuArchCfg.NAWBypassSB.initialize(false);
  theuArchCfg.NAWWaitAtSync.initialize(false);
  theuArchCfg.InOrderMemory.initialize(false);
  theuArchCfg.InOrderExecute.initialize(false);
  theuArchCfg.OffChipLatency.initialize(400);  //Roughly 100ns or two memory round-trips
  theuArchCfg.OnChipLatency.initialize(3);
  theuArchCfg.ValidateMMU.initialize(false);
  theuArchCfg.PurgePorts.initialize(1); /* CMU-ONLY */

  // Various functional unit parameters - only the operation latencies do anything at this point
  // Operation latencies are taken from Appendix C of Intel 64 and IA-32 Architectures Optimization Reference Manual, December 2008
  // Newest processor that data is given for was used, which is usually Nehalem
  theuArchCfg.NumIntAlu.initialize(4);
  theuArchCfg.IntAluOpLatency.initialize(1);
  theuArchCfg.IntAluOpPipelineResetTime.initialize(1);

  theuArchCfg.NumIntMult.initialize(2);
  theuArchCfg.IntMultOpLatency.initialize(3);
  theuArchCfg.IntMultOpPipelineResetTime.initialize(1);
  theuArchCfg.IntDivOpLatency.initialize(16);				// Midpoint of the given range of 11 - 21 cycles
  theuArchCfg.IntDivOpPipelineResetTime.initialize(6);

  theuArchCfg.NumFpAlu.initialize(1);
  theuArchCfg.FpAddOpLatency.initialize(3);
  theuArchCfg.FpAddOpPipelineResetTime.initialize(1);
  theuArchCfg.FpCmpOpLatency.initialize(1);
  theuArchCfg.FpCmpOpPipelineResetTime.initialize(1);
  theuArchCfg.FpCvtOpLatency.initialize(4);				// There are a whole bunch of convert instructions with different latencies, x87 version latency is not given so I chose a random SSE one
  theuArchCfg.FpCvtOpPipelineResetTime.initialize(1);

  theuArchCfg.NumFpMult.initialize(1);
  theuArchCfg.FpMultOpLatency.initialize(5);
  theuArchCfg.FpMultOpPipelineResetTime.initialize(2);
  theuArchCfg.FpDivOpLatency.initialize(6);
  theuArchCfg.FpDivOpPipelineResetTime.initialize(5);
  theuArchCfg.FpSqrtOpLatency.initialize(6);
  theuArchCfg.FpSqrtOpPipelineResetTime.initialize(5);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1dCfg.Cores.initialize(1);
  theL1dCfg.Size.initialize(64 * K);
  theL1dCfg.Associativity.initialize(2);
  theL1dCfg.BlockSize.initialize(CACHE_LINE_SIZE);
  theL1dCfg.PageSize.initialize(PAGE_SIZE);  /* CMU-ONLY */
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
  theL1dCfg.Placement.initialize("private");
  theL1dCfg.PrivateWithASR.initialize(false); /* CMU-ONLY */

  theL2Cfg.Cores.initialize( getSystemWidth() );
  theL2Cfg.Size.initialize(1 * M);
  theL2Cfg.Associativity.initialize(16);
  theL2Cfg.BlockSize.initialize(CACHE_LINE_SIZE);
  theL2Cfg.PageSize.initialize(PAGE_SIZE);  /* CMU-ONLY */
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
  theL2Cfg.NumL2Tiles.initialize(NUM_L2_TILES == 0 ? getSystemWidth() : NUM_L2_TILES);
  theL2Cfg.L2InterleavingGranularity.initialize(CACHE_LINE_SIZE);
  theL2Cfg.Placement.initialize("shared");
  theL2Cfg.PrivateWithASR.initialize(false); /* CMU-ONLY */
  theL2Cfg.AllowOffChipStreamFetch.initialize(false);
  theL2Cfg.BypassBus.initialize(false);

  theMemoryCfg.Delay.initialize(240);
  theMemoryCfg.MaxRequests.initialize(64);
  theMemoryCfg.NumL2TilesPerMC.initialize(NUM_L2_TILES_PER_MC);
  theMemoryCfg.L2InterleavingGranularity.initialize(CACHE_LINE_SIZE);

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
  theTraceTrackerCfg.BlockSize.initialize(CACHE_LINE_SIZE);
  theTraceTrackerCfg.Sharing.initialize(false); /* CMU-ONLY */
  theTraceTrackerCfg.SharingLevel.initialize(Flexus::SharedTypes::eL2); /* CMU-ONLY */

  theCmpNetworkControlCfg.NumCores.initialize( 0 );
  theCmpNetworkControlCfg.NumL2Tiles.initialize(NUM_L2_TILES == 0 ? getSystemWidth() : NUM_L2_TILES);
  theCmpNetworkControlCfg.NumMemControllers.initialize((NUM_L2_TILES == 0 ? getSystemWidth() : NUM_L2_TILES) / NUM_L2_TILES_PER_MC);
  theCmpNetworkControlCfg.VChannels.initialize(2);
  theCmpNetworkControlCfg.PageSize.initialize(PAGE_SIZE);
  theCmpNetworkControlCfg.CacheLineSize.initialize(CACHE_LINE_SIZE);
  theCmpNetworkControlCfg.Floorplan.initialize("tiled-torus");
  theCmpNetworkControlCfg.Placement.initialize("shared");
  theCmpNetworkControlCfg.L2InterleavingGranularity.initialize(CACHE_LINE_SIZE);
  theCmpNetworkControlCfg.NumCoresPerTorusRow.initialize(NUM_CORES_PER_TORUS_ROW);
  theCmpNetworkControlCfg.SizeOfInstrCluster.initialize(SIZE_INSTR_CLUSTER); /* CMU-ONLY */
  theCmpNetworkControlCfg.SizeOfPrivCluster.initialize(SIZE_INSTR_CLUSTER); /* CMU-ONLY */

  theCmpCoreNetworkInterfaceCfg.VChannels.initialize(2);
  theCmpCoreNetworkInterfaceCfg.RecvCapacity.initialize(4);
  theCmpCoreNetworkInterfaceCfg.SendCapacity.initialize(4);
  theCmpCoreNetworkInterfaceCfg.RequestVc.initialize(0);
  theCmpCoreNetworkInterfaceCfg.SnoopVc.initialize(1);
  theCmpCoreNetworkInterfaceCfg.ReplyVc.initialize(1);

  theCmpL2NetworkInterfaceCfg.NumL2Tiles.initialize(NUM_L2_TILES == 0 ? getSystemWidth() : NUM_L2_TILES);
  theCmpL2NetworkInterfaceCfg.NumMemControllers.initialize((NUM_L2_TILES == 0 ? getSystemWidth() : NUM_L2_TILES) / NUM_L2_TILES_PER_MC);
  theCmpL2NetworkInterfaceCfg.L2InterleavingGranularity.initialize(CACHE_LINE_SIZE);
  theCmpL2NetworkInterfaceCfg.Placement.initialize("shared");
  theCmpL2NetworkInterfaceCfg.VChannels.initialize(2);
  theCmpL2NetworkInterfaceCfg.RecvCapacity.initialize(4);
  theCmpL2NetworkInterfaceCfg.SendCapacity.initialize(4);
  theCmpL2NetworkInterfaceCfg.RequestVc.initialize(0);
  theCmpL2NetworkInterfaceCfg.SnoopVc.initialize(1);
  theCmpL2NetworkInterfaceCfg.ReplyVc.initialize(1);

  theNetworkCfg.VChannels.initialize(2);
  theNetworkCfg.NumNodes.initialize(getSystemWidth() + (NUM_L2_TILES == 0 ? getSystemWidth() : NUM_L2_TILES) );
  theNetworkCfg.NetworkTopologyFile.initialize("16node-tiled-torus.topology");

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FetchAddressGenerate, theFAGCfg, theFAG, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uFetch, theuFetchCfg, theuFetch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( v9Decoder, theDecoderCfg, theDecoder, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uArch, theuArchCfg, theuArch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);

#if (NUM_L2_TILES == 0)
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MemoryLoopback, theMemoryCfg, theMemory, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, NUM_L2_TILES_PER_MC );
#else
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MemoryLoopback, theMemoryCfg, theMemory, FIXED, FIXED, NUM_L2_TILES / NUM_L2_TILES_PER_MC );
#endif
FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
FLEXUS_INSTANTIATE_COMPONENT( TraceTrackerComponent, theTraceTrackerCfg, theTraceTracker );

#if (NUM_L2_TILES == 0)
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CMPCache, theL2Cfg, theL2, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CmpL2NetworkInterface, theCmpL2NetworkInterfaceCfg, theCmpL2NetworkInterface, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
#else
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CMPCache, theL2Cfg, theL2, FIXED, FIXED, NUM_L2_TILES);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CmpL2NetworkInterface, theCmpL2NetworkInterfaceCfg, theCmpL2NetworkInterface, FIXED, FIXED, NUM_L2_TILES );
#endif

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CmpCoreNetworkInterface, theCmpCoreNetworkInterfaceCfg, theCmpCoreNetworkInterface, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );
FLEXUS_INSTANTIATE_COMPONENT( CmpNetworkControl, theCmpNetworkControlCfg, theCmpNetworkControl);
FLEXUS_INSTANTIATE_COMPONENT( NetShim, theNetworkCfg, theNetwork );

FLEXUS_INSTANTIATE_COMPONENT( WhiteBox, theWhiteBoxCfg, theWhiteBox );

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

////////////////////////////// data
// request to mem
WIRE( theuArch, MemoryOut_Request,                theL1d, FrontSideIn_Request                    )
WIRE( theL1d, BackSideOut_Request,                theCmpCoreNetworkInterface, DRequestFromCore   )
WIRE( theCmpL2NetworkInterface, DRequestToL2,     theL2, FrontSideInD_Request                    )
WIRE( theL2, BackSideOut_Request,                 theCmpL2NetworkInterface, RequestFromL2BackSide )
WIRE( theCmpL2NetworkInterface, RequestToMem,     theMemory, LoopbackIn                          )

// reply to core
WIRE( theMemory, LoopbackOut,                     theCmpL2NetworkInterface, ReplyFromMem         )
WIRE( theCmpL2NetworkInterface, ReplyToL2,        theL2, BackSideIn_Reply                        )
WIRE( theL2, FrontSideOutD_Reply,                 theCmpL2NetworkInterface, DReplyFromL2         )
WIRE( theCmpCoreNetworkInterface, DReplyToCore,   theL1d, BackSideIn_Reply                       )
WIRE( theL1d, FrontSideOut_Reply,                 theuArch, MemoryIn_Reply                       )

// snoop to mem
WIRE( theuArch, MemoryOut_Snoop,                  theL1d, FrontSideIn_Snoop                      )
WIRE( theL1d, BackSideOut_Snoop,                  theCmpCoreNetworkInterface, DSnoopFromCore     )
WIRE( theCmpL2NetworkInterface, DSnoopToL2,       theL2, FrontSideInD_Snoop                      )
WIRE( theL2, BackSideOut_Snoop,                   theCmpL2NetworkInterface, SnoopFromL2BackSide  )

// snoop to core
WIRE( theCmpL2NetworkInterface, SnoopToL2,        theL2, BackSideIn_Request                      )
WIRE( theL2, FrontSideOutD_Request,               theCmpL2NetworkInterface, DRequestFromL2       )
WIRE( theCmpCoreNetworkInterface, DRequestToCore, theL1d, BackSideIn_Request                     )
WIRE( theL1d, FrontSideOut_Request,               theuArch, MemoryIn_Request                     )

////////////////////////////// instructions
// request to mem
WIRE( theuFetch, FetchMissOut,                    theCmpCoreNetworkInterface, IRequestFromCore   )
WIRE( theCmpL2NetworkInterface, IRequestToL2,     theL2, FrontSideInI_Request                    )

// reply to core
WIRE( theL2, FrontSideOutI_Reply,                 theCmpL2NetworkInterface, IReplyFromL2         )
WIRE( theCmpCoreNetworkInterface, IReplyToCore,   theuFetch, FetchMissIn_Reply                   )

// snoop to mem
WIRE( theuFetch, FetchSnoopOut,                   theCmpCoreNetworkInterface, ISnoopFromCore     )
WIRE( theCmpL2NetworkInterface, ISnoopToL2,       theL2, FrontSideInI_Snoop                      )

// snoop to core
WIRE( theL2, FrontSideOutI_Request,               theCmpL2NetworkInterface, IRequestFromL2       )
WIRE( theCmpCoreNetworkInterface, IRequestToCore, theuFetch, FetchMissIn_Request                 )

////////////////////////////// interconnect
WIRE( theCmpCoreNetworkInterface, ToNetwork,      theCmpNetworkControl, FromCore                 )
WIRE( theCmpL2NetworkInterface, ToNetwork,        theCmpNetworkControl, FromL2                   )
WIRE( theCmpNetworkControl, ToCore,               theCmpCoreNetworkInterface, FromNetwork        )
WIRE( theCmpNetworkControl, ToL2,                 theCmpL2NetworkInterface, FromNetwork          )
WIRE( theCmpNetworkControl, ToNetwork,            theNetwork, FromNode                           )
WIRE( theNetwork, ToNode,                         theCmpNetworkControl, FromNetwork              )

/* CMU-ONLY-BLOCK-BEGIN */
////////////////////////////// R-NUCA purges
WIRE( theCmpNetworkControl, PurgeAddrOut,         theuArch, PurgeAddrIn                          )
WIRE( theuArch, MemoryOut_Purge,                  theL1d, FrontSideIn_Purge                      )
WIRE( theuArch, PurgeAckOut,                      theCmpNetworkControl, PurgeAckIn               )
/* CMU-ONLY-BLOCK-END */

#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theuFetch, uFetchDrive )
, DRIVE( theFAG, FAGDrive )
, DRIVE( theuArch, uArchDrive )
, DRIVE( theDecoder, DecoderDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theCmpCoreNetworkInterface, CmpCoreNetworkInterfaceDrive )
, DRIVE( theCmpNetworkControl, CmpNetworkControlDrive )
, DRIVE( theNetwork, NetworkDrive )
, DRIVE( theCmpL2NetworkInterface, CmpL2NetworkInterfaceDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theL2, CacheDrive )
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

