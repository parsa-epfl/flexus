

#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "TMSCMPFlex.OoO v1.0";
}

#define FLEXUS_RuntimeDbgSev_CMPCache Verb
#define FLEXUS_RuntimeDbgSev_Cache Verb
#define FLEXUS_RuntimeDbgSev_uArch Verb

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include<components/CMPCache/CMPCache.hpp>
#include<components/CMPBus/CMPBus.hpp>
#include<components/Cache/Cache.hpp>
#include<components/PrefetchBuffer/PrefetchBuffer.hpp>
#include<components/TMSController/TMSController.hpp>
#include<components/TMSController/TMSChipIface.hpp>
#include<components/TMSController/TMSOnChipIface.hpp>
#include<components/TMSIndex/TMSIndex.hpp>
#include<components/CMOB/CMOB.hpp>
#include<components/MemoryLoopback/MemoryLoopback.hpp>
#include<components/MemoryMap/MemoryMap.hpp>
#include<components/MagicBreak/MagicBreak.hpp>
#include<components/FetchAddressGenerate/FetchAddressGenerate.hpp>
#include<components/uFetch/uFetch.hpp>
#include<components/v9Decoder/v9Decoder.hpp>
#include<components/uArch/uArch.hpp>
#include<components/TraceTracker/TraceTrackerComponent.hpp>
#include<components/MTManager/MTManagerComponent.hpp>
#include<components/StridePrefetcher/StridePrefetcher.hpp>

#include FLEXUS_END_DECLARATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( FetchAddressGenerate, "fag", theFAGCfg );
CREATE_CONFIGURATION( uFetch, "ufetch", theuFetchCfg );
CREATE_CONFIGURATION( v9Decoder, "decoder", theDecoderCfg );
CREATE_CONFIGURATION( uArch, "uarch", theuArchCfg );

CREATE_CONFIGURATION( Cache, "L1d", theL1dCfg );

CREATE_CONFIGURATION( PrefetchBuffer, "ocsvb", theOnChipSVBCfg );
CREATE_CONFIGURATION( TMSOnChipIface, "octmsiface", theTMSOnChipIfaceCfg );
CREATE_CONFIGURATION( TMSController, "octms", theTMSOnChipControllerCfg );
CREATE_CONFIGURATION( CMOB, "occmob", theOnChipCMOBCfg);

CREATE_CONFIGURATION( PrefetchBuffer, "svb", theSVBCfg );
CREATE_CONFIGURATION( TMSController, "tms", theTMSControllerCfg );
CREATE_CONFIGURATION( TMSChipIface, "tmsiface", theTMSChipIfaceCfg );
CREATE_CONFIGURATION( CMOB, "cmob", theCMOBCfg);
CREATE_CONFIGURATION( TMSIndex, "tindex", theTIndexCfg );
CREATE_CONFIGURATION( TMSIndex, "pindex", thePIndexCfg );
CREATE_CONFIGURATION( Cache, "tindex-cache", theTIndexCacheCfg );

CREATE_CONFIGURATION( CMPCache, "L2", theL2Cfg );
CREATE_CONFIGURATION( MemoryLoopback, "memory", theMemoryCfg );
CREATE_CONFIGURATION( MemoryMap, "memory-map", theMemoryMapCfg );
CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );
CREATE_CONFIGURATION( TraceTrackerComponent, "trace-tracker", theTraceTrackerCfg );
CREATE_CONFIGURATION( StridePrefetcher, "stride", theStrideCfg );
CREATE_CONFIGURATION( CMPBus, "bus", theBusCfg );

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
  theuFetchCfg.ICacheLineSize.initialize(64);
  theuFetchCfg.PerfectICache.initialize(false);
  theuFetchCfg.PrefetchEnabled.initialize(true);
  theuFetchCfg.CleanEvict.initialize(true);
  theuFetchCfg.Size.initialize(65536);
  theuFetchCfg.Associativity.initialize(4);
  theuFetchCfg.MissQueueSize.initialize(4);

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
  theuArchCfg.CoherenceUnit.initialize(64);
  theuArchCfg.BreakOnResynchronize.initialize(false);
  theuArchCfg.SpinControl.initialize(true);
  theuArchCfg.SpeculativeOrder.initialize(false);
  theuArchCfg.SpeculateOnAtomicValue.initialize(false);
  theuArchCfg.SpeculateOnAtomicValuePerfect.initialize(false);
  theuArchCfg.SpeculativeCheckpoints.initialize(0);
  theuArchCfg.CheckpointThreshold.initialize(0);
  theuArchCfg.InOrderExecute.initialize(false);
  theuArchCfg.InOrderMemory.initialize(false);
  theuArchCfg.OffChipLatency.initialize(320);
  theuArchCfg.OnChipLatency.initialize(3);
  theuArchCfg.InOrderExecute.initialize(false);

  static const int K = 1024;
  static const int M = 1024 * K;

  theL1dCfg.Cores.initialize(1);
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



  theOnChipSVBCfg.MAFSize.initialize(32);
  theOnChipSVBCfg.QueueSizes.initialize(8);
  theOnChipSVBCfg.NumWatches.initialize(0);
  theOnChipSVBCfg.NumEntries.initialize(32);
  theOnChipSVBCfg.ProcessingDelay.initialize(0);
  theOnChipSVBCfg.EvictClean.initialize(true);
  theOnChipSVBCfg.UseStreamFetch.initialize(true);

  theTMSOnChipControllerCfg.MaxPrefetches.initialize(16);
  theTMSOnChipControllerCfg.QueueSize.initialize(8);
  theTMSOnChipControllerCfg.StreamQueues.initialize(8);
  theTMSOnChipControllerCfg.MinBufferCap.initialize(3);
  theTMSOnChipControllerCfg.InitBufferCap.initialize(4);
  theTMSOnChipControllerCfg.MaxBufferCap.initialize(8);
  theTMSOnChipControllerCfg.UsePIndex.initialize(false);
  theTMSOnChipControllerCfg.TIndexInsertProb.initialize(1.0);
  theTMSOnChipControllerCfg.FetchCMOBAt.initialize(4);
  theTMSOnChipControllerCfg.OnChipTMS.initialize(true);
  theTMSOnChipControllerCfg.EnableTMS.initialize(false);

  theTMSOnChipIfaceCfg.IndexName.initialize("TMSon_c32768_p1_mrc");

  theOnChipCMOBCfg.CMOBSize.initialize( 32768 );
  theOnChipCMOBCfg.CMOBLineSize.initialize( 8 );
  theOnChipCMOBCfg.UseMemory.initialize(false);
  theOnChipCMOBCfg.CMOBName.initialize("TMS_CMOBon_c32768_p1_");

  theSVBCfg.MAFSize.initialize(32);
  theSVBCfg.QueueSizes.initialize(8);
  theSVBCfg.NumWatches.initialize(0);
  theSVBCfg.NumEntries.initialize(32);
  theSVBCfg.ProcessingDelay.initialize(0);
  theSVBCfg.EvictClean.initialize(true);
  theSVBCfg.UseStreamFetch.initialize(false);


  theTMSControllerCfg.MaxPrefetches.initialize(16);
  theTMSControllerCfg.QueueSize.initialize(8);
  theTMSControllerCfg.StreamQueues.initialize(8);
  theTMSControllerCfg.MinBufferCap.initialize(4);
  theTMSControllerCfg.InitBufferCap.initialize(8);
  theTMSControllerCfg.MaxBufferCap.initialize(24);
  theTMSControllerCfg.UsePIndex.initialize(false);
  theTMSControllerCfg.TIndexInsertProb.initialize(1.0);
  theTMSControllerCfg.FetchCMOBAt.initialize(4);
  theTMSControllerCfg.OnChipTMS.initialize(false);

  theCMOBCfg.CMOBSize.initialize( 131072 * 12 );
  theCMOBCfg.CMOBLineSize.initialize( 12 );
  theCMOBCfg.UseMemory.initialize(false);
  theCMOBCfg.CMOBName.initialize("TMS_CMOB_p1_");

  theTIndexCfg.BucketsLog2.initialize(17);
  theTIndexCfg.BucketSize.initialize(8);
  theTIndexCfg.QueueSizes.initialize(32);
  theTIndexCfg.UseMemory.initialize(false);
  theTIndexCfg.IndexName.initialize("TMS_p1_tindex");
  theTIndexCfg.FillPrefix.initialize(0);

  thePIndexCfg.BucketsLog2.initialize(14);
  thePIndexCfg.BucketSize.initialize(8);
  thePIndexCfg.QueueSizes.initialize(32);
  thePIndexCfg.UseMemory.initialize(false);
  thePIndexCfg.IndexName.initialize("pindex");
  thePIndexCfg.FillPrefix.initialize(0);

  theTIndexCacheCfg.Cores.initialize(1);
  theTIndexCacheCfg.Size.initialize(16 * K);
  theTIndexCacheCfg.Associativity.initialize(2);
  theTIndexCacheCfg.BlockSize.initialize(64);
  theTIndexCacheCfg.TagLatency.initialize(0);
  theTIndexCacheCfg.Ports.initialize(1);
  theTIndexCacheCfg.TagIssueLatency.initialize(1);
  theTIndexCacheCfg.DataLatency.initialize(0);
  theTIndexCacheCfg.DataIssueLatency.initialize(1);
  theTIndexCacheCfg.CacheLevel.initialize(eL2Prefetcher);
  theTIndexCacheCfg.QueueSizes.initialize(8);
  theTIndexCacheCfg.MAFSize.initialize(32);
  theTIndexCacheCfg.EvictBufferSize.initialize(16);
  theTIndexCacheCfg.ProbeFetchMiss.initialize(false);
  theTIndexCacheCfg.BusTime_NoData.initialize(1);
  theTIndexCacheCfg.BusTime_Data.initialize(1);
  theTIndexCacheCfg.EvictClean.initialize(false);
  theTIndexCacheCfg.MAFTargetsPerRequest.initialize(0);
  theTIndexCacheCfg.PreQueueSizes.initialize(4);
  theTIndexCacheCfg.FastEvictClean.initialize(false);
  theTIndexCacheCfg.NoBus.initialize(true);
  theTIndexCacheCfg.TraceAddress.initialize(0);
  theTIndexCacheCfg.Banks.initialize(1);

  theL2Cfg.Cores.initialize( getSystemWidth() );
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
  theL2Cfg.BypassBus.initialize(true);
  theL2Cfg.AllowOffChipStreamFetch.initialize(true);

  theStrideCfg.NumEntries.initialize(32);
  theStrideCfg.QueueSizes.initialize(8);
  theStrideCfg.MAFSize.initialize(48);
  theStrideCfg.ProcessingDelay.initialize(0);
  theStrideCfg.MaxPrefetchQueue.initialize(16);
  theStrideCfg.MaxOutstandingPrefetches.initialize(16);

  theBusCfg.BusTime_Data.initialize( 9 );
  theBusCfg.BusTime_NoData.initialize( 1 );
  theBusCfg.QueueSize.initialize( 16 );
  theBusCfg.EnableDMA.initialize( true );

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

  return true; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FetchAddressGenerate, theFAGCfg, theFAG, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uFetch, theuFetchCfg, theuFetch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( v9Decoder, theDecoderCfg, theDecoder, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uArch, theuArchCfg, theuArch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( PrefetchBuffer, theSVBCfg, theSVB, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( PrefetchBuffer, theOnChipSVBCfg, theocSVB, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CMOB, theOnChipCMOBCfg, theocCMOB, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( TMSController, theTMSControllerCfg, theTMSc, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( TMSController, theTMSOnChipControllerCfg, theocTMSc, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CMOB, theCMOBCfg, theCMOB, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT( TMSOnChipIface, theTMSOnChipIfaceCfg, theocTMSif );
FLEXUS_INSTANTIATE_COMPONENT( TMSChipIface, theTMSChipIfaceCfg, theTMSif );
FLEXUS_INSTANTIATE_COMPONENT( TMSIndex, theTIndexCfg, theTIndex);
FLEXUS_INSTANTIATE_COMPONENT( TMSIndex, thePIndexCfg, thePIndex);
FLEXUS_INSTANTIATE_COMPONENT( Cache, theTIndexCacheCfg, theTIndexCache);
FLEXUS_INSTANTIATE_COMPONENT( CMPCache, theL2Cfg, theL2 );
FLEXUS_INSTANTIATE_COMPONENT( StridePrefetcher, theStrideCfg, theStride );
FLEXUS_INSTANTIATE_COMPONENT( CMPBus, theBusCfg, theBus);
FLEXUS_INSTANTIATE_COMPONENT( MemoryLoopback, theMemoryCfg, theMemory );
FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
FLEXUS_INSTANTIATE_COMPONENT( TraceTrackerComponent, theTraceTrackerCfg, theTraceTracker );

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

//uFetch to L2 cache
WIRE( theuFetch, FetchMissOut,          theL2, FrontSideInI_Request       )
WIRE( theuFetch, FetchSnoopOut,         theL2, FrontSideInI_Snoop         )
WIRE( theL2, FrontSideOutI,             theuFetch, FetchMissIn            )

//uArch to L1 D cache
WIRE( theuArch, MemoryOut_Request,      theL1d, FrontSideIn_Request       )
WIRE( theuArch, MemoryOut_Snoop,        theL1d, FrontSideIn_Snoop         )
WIRE( theL1d, FrontSideOut,             theuArch, MemoryIn                )

//L1 D-cache to OnChip SVB
WIRE( theL1d, BackSideOut_Request,      theocSVB, FrontSideIn_Request     )
WIRE( theL1d, BackSideOut_Prefetch,     theocSVB, FrontSideIn_Prefetch    )
WIRE( theL1d, BackSideOut_Snoop,        theocSVB, FrontSideIn_Snoop   )
WIRE( theocSVB, FrontSideOut,           theocTMSif, MonitorL1Replies_In)
WIRE( theocTMSif, MonitorL1Replies_Out, theL1d, BackSideIn                )

//OnChip SVB to SVB
WIRE( theocSVB, BackSideOut_Request, theSVB, FrontSideIn_Request )
WIRE( theocSVB, BackSideOut_Snoop,  theSVB, FrontSideIn_Snoop   )
WIRE( theocSVB, BackSideOut_Prefetch, theSVB, FrontSideIn_Prefetch   )
WIRE( theSVB, FrontSideOut,             theocSVB, BackSideIn    )

//uArch to TMSController
WIRE( theuArch, NotifyTMS,              theocTMSc, CoreNotify               )
WIRE( theocTMSc, CoreNotifyOut,         theTMSc, CoreNotify               )

//TMSController to SVB
WIRE( theTMSc, ToSVB,                   theSVB, MasterIn                  )
WIRE( theSVB, MasterOut,                theTMSc, FromSVB                  )
WIRE( theSVB, RecentRequests,           theTMSc, RecentRequests           )

//TMSOcc to ocSVB
WIRE( theocTMSc, ToSVB,                 theocSVB, MasterIn                  )
WIRE( theocSVB, MasterOut,              theocTMSc, FromSVB                  )
WIRE( theocSVB, RecentRequests,         theocTMSc, RecentRequests           )

//SVB to L2
WIRE( theSVB, BackSideOut_Request,      theL2, FrontSideInD_Request       )
WIRE( theSVB, BackSideOut_Prefetch,     theL2, FrontSideInD_Prefetch      )
WIRE( theSVB, BackSideOut_Snoop,        theL2, FrontSideInD_Snoop         )
WIRE( theL2, FrontSideOutD,             theSVB, BackSideIn                )

//StridePrefetcher to L2
WIRE( theL2, BackSideOut_Request,       theStride, FrontSideIn_Request    )
WIRE( theL2, BackSideOut_Prefetch,      theStride, FrontSideIn_Prefetch   )
WIRE( theL2 , BackSideOut_Snoop,        theStride, FrontSideIn_Snoop      )
WIRE( theStride, FrontSideOut,          theL2, BackSideIn                 )

//Bus to StridePrefetcher

//Bus to L2
WIRE( theStride, BackSideOut_Request,   theTMSif, MonitorMemRequests_In   )
WIRE( theStride, BackSideOut_Prefetch,  theTMSif, MonitorMemPrefetch_In   )
WIRE( theTMSif, MonitorMemRequests_Out, theBus, FromCache_Request         )
WIRE( theTMSif, MonitorMemPrefetch_Out, theBus, FromCache_Prefetch        )
WIRE( theStride, BackSideOut_Snoop,     theBus, FromCache_Snoop           )
WIRE( theBus, ToCache,                  theStride, BackSideIn             )

//Bus to Memory
WIRE( theBus, ToMemory,                 theMemory, LoopbackIn             )
WIRE( theMemory, LoopbackOut,           theBus, FromMemory                )

//TMSc to TMSif
WIRE( theTMSif, TMSc_MemRequests,       theTMSc, MonitorMemRequests       )
WIRE( theTMSc, NextCMOBIndex,           theTMSif, TMSc_NextCMOBIndex      )
WIRE( theTMSc, CMOBAppend,              theTMSif, TMSc_CMOBAppend         )
WIRE( theTMSc, CMOBRequest,             theTMSif, TMSc_CMOBRequest        )
WIRE( theTMSif, TMSc_CMOBReply,         theTMSc, CMOBReply                )

//TMSif to CMOB
WIRE( theTMSif, CMOB_NextAppendIndex,   theCMOB, TMSif_NextAppendIndex    )
WIRE( theTMSif, CMOB_Initialize,        theCMOB, TMSif_Initialize         )
WIRE( theTMSif, CMOB_Request,           theCMOB, TMSif_Request            )
WIRE( theCMOB, TMSif_Reply,             theTMSif, CMOB_Reply              )

//TMSc to TIndex
WIRE( theTMSc, ToTIndex,                theTIndex, TMSc_Request           )
WIRE( theTIndex, TMSc_Reply,            theTMSc, FromTIndex               )

//TMSc to PIndex
WIRE( theTMSc, ToPIndex,                thePIndex, TMSc_Request           )
WIRE( thePIndex, TMSc_Reply,            theTMSc, FromPIndex               )

WIRE( thePIndex, PrefixRead,            theCMOB, PrefixRead               )
WIRE( theCMOB, PrefixReadOut,           thePIndex, PrefixReadIn           )

//TMS components to Bus
WIRE( theTIndex, ToMemory,              theTIndexCache, FrontSideIn_Request )
WIRE( theTIndexCache, BackSideOut_Request, theBus, FromIndex                 )
WIRE( theBus, ToIndex,                  theTIndexCache,  BackSideIn       )
WIRE( theTIndexCache, FrontSideOut,     theTIndex, FromMemory             )

WIRE( theCMOB, ToMemory,                theBus, FromCMOB                  )
WIRE( theBus, ToCMOB,                   theCMOB, FromMemory               )

//ocTMSc to ocTMSif
WIRE( theocTMSif, TMSc_L1Replies,       theocTMSc, MonitorMemRequests     )
WIRE( theocTMSc, NextCMOBIndex,         theocTMSif, TMSc_NextCMOBIndex    )
WIRE( theocTMSc, CMOBAppend,            theocTMSif, TMSc_CMOBAppend       )
WIRE( theocTMSc, CMOBRequest,           theocTMSif, TMSc_CMOBRequest      )
WIRE( theocTMSif, TMSc_CMOBReply,       theocTMSc, CMOBReply              )

WIRE( theocTMSc, ToTIndex,              theocTMSif, TMSc_IndexRequest      )
WIRE( theocTMSif, TMSc_IndexReply,      theocTMSc, FromTIndex )

//TMSif to CMOB
WIRE( theocTMSif, CMOB_NextAppendIndex, theocCMOB, TMSif_NextAppendIndex    )
WIRE( theocTMSif, CMOB_Initialize,      theocCMOB, TMSif_Initialize         )
WIRE( theocTMSif, CMOB_Request,         theocCMOB, TMSif_Request            )
WIRE( theocCMOB, TMSif_Reply,           theocTMSif, CMOB_Reply              )


#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theuFetch, uFetchDrive )
, DRIVE( theFAG, FAGDrive )
, DRIVE( theuArch, uArchDrive )
, DRIVE( theDecoder, DecoderDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theTMSc, TMSControllerDrive )
, DRIVE( theCMOB, CMOBDrive )
, DRIVE( theocTMSc, TMSControllerDrive )
, DRIVE( theocCMOB, CMOBDrive )
, DRIVE( thePIndex, IndexDrive )
, DRIVE( theTIndex, IndexDrive )
, DRIVE( theTIndexCache, CacheDrive )
, DRIVE( theocSVB, PrefetchDrive )
, DRIVE( theSVB, PrefetchDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theBus, BusDrive )
, DRIVE( theStride, StrideDrive )
, DRIVE( theL2, CacheDrive )
, DRIVE( theSVB, PrefetchDrive )        //Double-clocked to achieve correct L2 hit latency
, DRIVE( theocSVB, PrefetchDrive )  //Double-clocked to achieve correct L2 hit latency
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()

