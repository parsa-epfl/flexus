#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
    //Simulator Name
    std::string theSimulatorName = "KnottyKraken v1.1 - w.TimingRMCs";
}

#define THREADS_PER_CORE   1
//#define USE_DRAMSIM
//#define TRAFFIC_GENERATOR

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#ifndef USE_DRAMSIM
  #include <components/MemoryLoopback/MemoryLoopback.hpp>
#else
  #include <components/DRAMController/DRAMController.hpp>
#endif
  #include <components/MemoryMap/MemoryMap.hpp>
  #include <components/MagicBreakQEMU/MagicBreak.hpp>
  #include <components/Cache/Cache.hpp>
  #include <components/NetShim/MemoryNetwork.hpp>
  #include <components/MultiNic/MultiNic3.hpp>
  #include <components/CMPCache/CMPCache.hpp>
  #include <components/FetchAddressGenerate/FetchAddressGenerate.hpp>
  #include <components/uFetch/uFetch.hpp>
  #include <components/uFetch/PortCombiner.hpp>
  #include <components/armDecoder/armDecoder.hpp>
  #include <components/uArchARM/uArchARM.hpp>
  #include <components/SplitDestinationMapper/SplitDestinationMapper.hpp>
  //#include <components/WhiteBox/WhiteBox.hpp>
  #include <components/MMU/MMU.hpp>
#ifdef TRAFFIC_GENERATOR
  #include <components/RMCTrafficGenerator/RMCTrafficGenerator.hpp>
#else
  #include <components/RMCConnector/RMCConnector.hpp>
#endif
  #include <components/LBAProducer/LBAProducer.hpp>

#include FLEXUS_END_DECLARATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

  //CREATE_CONFIGURATION( WhiteBox, "white-box", theWhiteBoxCfg );
  CREATE_CONFIGURATION( MMU , "mmu", theMMUCfg );
  CREATE_CONFIGURATION( FetchAddressGenerate, "fag", theFAGCfg );
  CREATE_CONFIGURATION( uFetch, "ufetch", theuFetchCfg );
  CREATE_CONFIGURATION( PortCombiner, "combiner", theCombinerCfg );
  CREATE_CONFIGURATION( armDecoder, "decoder", theDecoderCfg );
  CREATE_CONFIGURATION( uArchARM, "uarch", theuArchCfg );
  CREATE_CONFIGURATION( uArchARM, "RMC", theRMCCfg );
  CREATE_CONFIGURATION( Cache, "L1d", theL1dCfg );
  CREATE_CONFIGURATION( Cache, "RMCL1", theRMCL1Cfg );
  CREATE_CONFIGURATION( CMPCache, "L2", theL2Cfg );
  CREATE_CONFIGURATION( MultiNic3, "nic", theNicCfg );
  CREATE_CONFIGURATION( MemoryNetwork, "network", theNetworkCfg );
  CREATE_CONFIGURATION( SplitDestinationMapper, "net-mapper", theNetMapperCfg );
#ifndef USE_DRAMSIM
  CREATE_CONFIGURATION( MemoryLoopback, "memory", theMemoryCfg );
#else
  CREATE_CONFIGURATION( DRAMController, "memory", theMemoryCfg );
#endif
  CREATE_CONFIGURATION( MemoryMap, "memory-map", theMemoryMapCfg );
  CREATE_CONFIGURATION( MagicBreak, "magic-break", theMagicBreakCfg );
#ifdef TRAFFIC_GENERATOR
  CREATE_CONFIGURATION( RMCTrafficGenerator, "rmc-connector", theRMCConnectorCfg );
#else
  CREATE_CONFIGURATION( RMCConnector, "rmc-connector", theRMCConnectorCfg );
#endif
  CREATE_CONFIGURATION( LBAProducer, "lba-producer", theLBAProducerCfg );

  //You may optionally initialize configuration parameters from within this
  //function.  This initialization occur before the command line is processed,
  //so they will be overridden from the command line.
  //
  //Return value indicates whether simulation should abort if any parameters
  //are left at their default values;
  bool initializeParameters() {
    DBG_( Dev, (<< " initializing Parameters..." ) );

    theFAGCfg.Threads.initialize(1);
    theFAGCfg.MaxFetchAddress.initialize(10);
    theFAGCfg.MaxBPred.initialize(2);

    theuFetchCfg.Threads.initialize(1);
    theuFetchCfg.FAQSize.initialize(32);
    theuFetchCfg.MaxFetchLines.initialize(2);
    theuFetchCfg.MaxFetchInstructions.initialize(10);
    theuFetchCfg.ICacheLineSize.initialize(64);
    theuFetchCfg.PerfectICache.initialize(false);
    theuFetchCfg.PrefetchEnabled.initialize(true);
    theuFetchCfg.Size.initialize(65536);
    theuFetchCfg.Associativity.initialize(2);
    theuFetchCfg.MissQueueSize.initialize(4);
    theuFetchCfg.CleanEvict.initialize(false);
    theuFetchCfg.SendAcks.initialize(true);
    theuFetchCfg.UseReplyChannel.initialize(true);

    theDecoderCfg.Multithread.initialize(false);
    theDecoderCfg.FIQSize.initialize(16);
    theDecoderCfg.DispatchWidth.initialize(8);

    theuArchCfg.Multithread.initialize(false);
    theuArchCfg.ROBSize.initialize(256);
    theuArchCfg.SBSize.initialize(64);
    theuArchCfg.NAWBypassSB.initialize(true);
    theuArchCfg.NAWWaitAtSync.initialize(false);
    theuArchCfg.RetireWidth.initialize(8);
    theuArchCfg.SnoopPorts.initialize(1);
    theuArchCfg.MemoryPorts.initialize(4);
    theuArchCfg.StorePrefetches.initialize(30);
    theuArchCfg.PrefetchEarly.initialize(false);
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

    theuArchCfg.EarlySGP.initialize(false); // CMU-ONLY
    theuArchCfg.TrackParallelAccesses.initialize(false); // CMU-ONLY
    theuArchCfg.ValidateMMU.initialize(false);
    
    theuArchCfg.IsRMC.initialize(false);
    theuArchCfg.MagicCacheConnection.initialize(false);    
    
    //bogus
	theuArchCfg.RMCTLBEntries.initialize(32);
	theuArchCfg.RMCGZipFlexpoints.initialize(false);
	theuArchCfg.RMCTextFlexpoints.initialize(true);
	theuArchCfg.FlexiAppReqSize.initialize(0);
	theuArchCfg.OneSided.initialize(0);
	theuArchCfg.EnableSingleRMCLoading.initialize(0);
	theuArchCfg.SplitRGP.initialize(0);
    theuArchCfg.LoadFromSimics.initialize(false);
    theuArchCfg.MachineCount.initialize(1);
    theuArchCfg.PrefetchBlockLimit.initialize(0);
    theuArchCfg.RGPPrefetchSABRe.initialize(0);
    theuArchCfg.RRPPPrefetchSABRe.initialize(0);
    theuArchCfg.RGPPerTile.initialize(false);
    theuArchCfg.RRPPPerTile.initialize(false);
    theuArchCfg.RRPPWithMCs.initialize(false);
    theuArchCfg.SABReToRRPPMatching.initialize("bogus");
    theuArchCfg.SABReStreamBuffers.initialize(false);
    theuArchCfg.SelfCleanSENDs.initialize(false);
    theuArchCfg.SingleRMCDispatch.initialize(false);
    theuArchCfg.RMCDispatcherID.initialize(0);
    theuArchCfg.DataplaneEmulation.initialize(false);
    theuArchCfg.ServiceTimeMeasurement.initialize(false);
    theuArchCfg.PerCoreReqQueue.initialize(1);
	
    theRMCCfg.IsRMC.initialize(true);
    theRMCCfg.MachineCount.initialize(1);
    //bogus
    theRMCCfg.Multithread.initialize(false);
    theRMCCfg.ROBSize.initialize(256);
    theRMCCfg.SBSize.initialize(64);
    theRMCCfg.NAWBypassSB.initialize(true);
    theRMCCfg.NAWWaitAtSync.initialize(false);
    theRMCCfg.RetireWidth.initialize(8);
    theRMCCfg.SnoopPorts.initialize(1);
    theRMCCfg.MemoryPorts.initialize(4);
    theRMCCfg.StorePrefetches.initialize(30);
    theRMCCfg.PrefetchEarly.initialize(false);
    theRMCCfg.ConsistencyModel.initialize(1); //TSO
    theRMCCfg.CoherenceUnit.initialize(64);
    theRMCCfg.BreakOnResynchronize.initialize(false);
    theRMCCfg.SpinControl.initialize(true);
    theRMCCfg.SpeculativeOrder.initialize(false);
    theRMCCfg.SpeculateOnAtomicValue.initialize(false);
    theRMCCfg.SpeculateOnAtomicValuePerfect.initialize(false);
    theRMCCfg.SpeculativeCheckpoints.initialize(0);
    theRMCCfg.CheckpointThreshold.initialize(0);
    theRMCCfg.InOrderExecute.initialize(false);
    theRMCCfg.InOrderMemory.initialize(false);
    theRMCCfg.OffChipLatency.initialize(320);
    theRMCCfg.OnChipLatency.initialize(3);
    theRMCCfg.EarlySGP.initialize(false); // CMU-ONLY
    theRMCCfg.TrackParallelAccesses.initialize(false); // CMU-ONLY
    theRMCCfg.ValidateMMU.initialize(false);
    theRMCCfg.FpAddOpLatency.initialize(1);
    theRMCCfg.FpAddOpPipelineResetTime.initialize(1);
    theRMCCfg.FpCmpOpLatency.initialize(1);
    theRMCCfg.FpCmpOpPipelineResetTime.initialize(1);
    theRMCCfg.FpCvtOpLatency.initialize(1);
    theRMCCfg.FpCvtOpPipelineResetTime.initialize(1);
    theRMCCfg.NumFpMult.initialize(1);
    theRMCCfg.FpMultOpLatency.initialize(1);
    theRMCCfg.FpMultOpPipelineResetTime.initialize(1);
    theRMCCfg.FpDivOpLatency.initialize(1);
    theRMCCfg.FpDivOpPipelineResetTime.initialize(1);
    theRMCCfg.FpSqrtOpLatency.initialize(1);
    theRMCCfg.FpSqrtOpPipelineResetTime.initialize(1);
    theRMCCfg.NumIntAlu.initialize(1);
    theRMCCfg.IntAluOpLatency.initialize(1);
    theRMCCfg.IntAluOpPipelineResetTime.initialize(1);
    theRMCCfg.NumIntMult.initialize(1);
    theRMCCfg.IntMultOpLatency.initialize(1);
    theRMCCfg.IntMultOpPipelineResetTime.initialize(1);
    theRMCCfg.IntDivOpLatency.initialize(1);
    theRMCCfg.IntDivOpPipelineResetTime.initialize(1);  
    theRMCCfg.NumFpAlu.initialize(1);
    theRMCCfg.LoadFromSimics.initialize(false);
    theRMCCfg.SingleRMCDispatch.initialize(false);
    theRMCCfg.RMCDispatcherID.initialize(0);
    theRMCCfg.PerCoreReqQueue.initialize(1);
    theRMCCfg.DataplaneEmulation.initialize(false);
    theRMCCfg.ServiceTimeMeasurement.initialize(false);

    static const int K = 1024;

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
	theL1dCfg.isRMCCache.initialize(false);
	theL1dCfg.hasStreamBuffers.initialize(false);
	
	theRMCL1Cfg.Cores.initialize( 1 );
    theRMCL1Cfg.BlockSize.initialize(64);
    theRMCL1Cfg.Ports.initialize(4);
    theRMCL1Cfg.PreQueueSizes.initialize(4);
    theRMCL1Cfg.TagLatency.initialize(0);
    theRMCL1Cfg.TagIssueLatency.initialize(1);
    theRMCL1Cfg.DataLatency.initialize(2);
    theRMCL1Cfg.DataIssueLatency.initialize(1);
    theRMCL1Cfg.CacheLevel.initialize(eL1);
    theRMCL1Cfg.QueueSizes.initialize(8);
    theRMCL1Cfg.MAFSize.initialize(32);
    theRMCL1Cfg.EvictBufferSize.initialize(8);
    theRMCL1Cfg.SnoopBufferSize.initialize(8);
    theRMCL1Cfg.ProbeFetchMiss.initialize(false);
    theRMCL1Cfg.EvictClean.initialize(true);
    theRMCL1Cfg.BusTime_NoData.initialize(1);
    theRMCL1Cfg.BusTime_Data.initialize(2);
    theRMCL1Cfg.MAFTargetsPerRequest.initialize(0);
    theRMCL1Cfg.FastEvictClean.initialize(false);
    theRMCL1Cfg.NoBus.initialize(false);
    theRMCL1Cfg.Banks.initialize(1);
    theRMCL1Cfg.TraceAddress.initialize(0);
    theRMCL1Cfg.CacheType.initialize("InclMESI2PhaseWB");
    theRMCL1Cfg.ArrayConfiguration.initialize("STD:size=65536:assoc=4:repl=LRU");
	theRMCL1Cfg.EvictOnSnoop.initialize(true);
	theRMCL1Cfg.UseReplyChannel.initialize(true);
	theRMCL1Cfg.isRMCCache.initialize(true);

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
    theL2Cfg.MachineCount.initialize(1);

    theNicCfg.VChannels.initialize(3);
    theNicCfg.RecvCapacity.initialize(4);
    theNicCfg.SendCapacity.initialize(1);
    theNicCfg.MachineCount.initialize(1);

    //theNetworkCfg.NetworkTopologyFile.initialize("16node-torus.topology");
    theNetworkCfg.NetworkTopologyFile.initialize("64x3-mesh.topology");
    theNetworkCfg.NumNodes.initialize( 3*getSystemWidth() );
    theNetworkCfg.VChannels.initialize( 3 );
    theNetworkCfg.VChannels.initialize( 3 );
    theNetworkCfg.randomDirection.initialize(false);

	theNetMapperCfg.Cores.initialize(16);
	theNetMapperCfg.Directories.initialize(16);
	theNetMapperCfg.MemControllers.initialize(4);
	theNetMapperCfg.DirInterleaving.initialize(64);
	theNetMapperCfg.MemInterleaving.initialize(4096);
	theNetMapperCfg.DirLocation.initialize("Distributed");
	theNetMapperCfg.MemLocation.initialize("4,7,8,11");
	theNetMapperCfg.MemReplyToDir.initialize(true);
	theNetMapperCfg.MemAcksNeedData.initialize(true);
    theNetMapperCfg.MachineCount.initialize(1);

	theMemoryMapCfg.PageSize.initialize(8 * K);
	theMemoryMapCfg.NumNodes.initialize( getSystemWidth() );
	theMemoryMapCfg.RoundRobin.initialize(true);
	theMemoryMapCfg.CreatePageMap.initialize(true);
	theMemoryMapCfg.ReadPageMap.initialize(true);

#ifndef USE_DRAMSIM
	theMemoryCfg.Delay.initialize(240);
	theMemoryCfg.MaxRequests.initialize(128);
	theMemoryCfg.UseFetchReply.initialize(true);
#else
	theMemoryCfg.Frequency.initialize(2000);
	theMemoryCfg.Size.initialize(2048);
	theMemoryCfg.DynamicSize.initialize(true);
	theMemoryCfg.Interleaving.initialize(4096);
	theMemoryCfg.MaxRequests.initialize(128);
	theMemoryCfg.MaxReplies.initialize(8);
	theMemoryCfg.UseFetchReply.initialize(true);
	theMemoryCfg.MemorySystemFile.initialize("system.ini");
	theMemoryCfg.DeviceFile.initialize("DDR3_micron_64M_8B_x4_sg15.ini");
#endif

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
    theMagicBreakCfg.MachineCount.initialize(1);   
   
    theLBAProducerCfg.EnableServiceTimeGen.initialize(0);

    theMMUCfg.Cores.initialize(1);
    theMMUCfg.iTLBSize.initialize(64);
    theMMUCfg.dTLBSize.initialize(64);
    theMMUCfg.PerfectTLB.initialize(true);
 
    return true; //Abort simulation if parameters are not initialized
  }

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
  //All component Instances are created here.  This section
  //also creates handles for each component
  //FLEXUS_INSTANTIATE_COMPONENT( WhiteBox, theWhiteBoxCfg, theWhiteBox );

  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FetchAddressGenerate, theFAGCfg, theFAG, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uFetch, theuFetchCfg, theuFetch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( PortCombiner, theCombinerCfg, theuFetchCombiner, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( armDecoder, theDecoderCfg, theDecoder, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uArchARM, theuArchCfg, theuArch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uArchARM, theRMCCfg, theRMC, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theRMCL1Cfg, theRMCL1, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( CMPCache, theL2Cfg, theL2, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1 );		//need to make it aware of 2x L1d caches
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MMU , theMMUCfg, theMMU, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
#ifndef USE_DRAMSIM
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MemoryLoopback, theMemoryCfg, theMemory, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, 8 );	//8 for 64 cores, 2 for 16 cores
#else
  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( DRAMController, theMemoryCfg, theMemory, SCALE_WITH_SYSTEM_WIDTH, DIVIDE, 4 );	//8 for 64 cores, 4 for 16 cores, 2 for 4 cores
#endif

  FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MultiNic3, theNicCfg, theNic, SCALE_WITH_SYSTEM_WIDTH,MULTIPLY,3);	
  FLEXUS_INSTANTIATE_COMPONENT( MemoryNetwork, theNetworkCfg, theNetwork );
  FLEXUS_INSTANTIATE_COMPONENT( MemoryMap, theMemoryMapCfg, theMemoryMap );
  FLEXUS_INSTANTIATE_COMPONENT( MagicBreak, theMagicBreakCfg, theMagicBreak );
  FLEXUS_INSTANTIATE_COMPONENT( SplitDestinationMapper, theNetMapperCfg, theNetMapper );		//might need to change port width for L1D caches
#ifdef TRAFFIC_GENERATOR
  FLEXUS_INSTANTIATE_COMPONENT( RMCTrafficGenerator, theRMCConnectorCfg, theRMCConnector );
#else
  FLEXUS_INSTANTIATE_COMPONENT( RMCConnector, theRMCConnectorCfg, theRMCConnector );
#endif
  FLEXUS_INSTANTIATE_COMPONENT( LBAProducer, theLBAProducerCfg, theLBAProducer );

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

    // Fetch to MMU
        WIRE( theuFetch, iTranslationOut,       theMMU, iRequestIn                )
        WIRE( theMMU, iTranslationReply,        theuFetch, iTranslationIn         )

// uArch to MMU
        WIRE( theuArch, dTranslationOut,        theMMU, dRequestIn                )
        WIRE( theMMU, dTranslationReply,        theuArch, dTranslationIn          )
        WIRE( theMMU, MemoryRequestOut,         theuArch, MemoryRequestIn         )
        WIRE(theuArch, ResyncOut,               theMMU,   ResyncIn                )
        WIRE(theMMU, ResyncOut,                 theuFetch,   ResyncIn             )

      //Decoder to uArch
      WIRE( theDecoder, AvailableDispatchIn,  theuArch, AvailableDispatchOut    )
      WIRE( theDecoder, DispatchOut,          theuArch, DispatchIn              )
      WIRE( theuArch, SquashOut,              theDecoder, SquashIn              )

      // TODO: Msutherl - this was in simflex, but qflex theDecoder does not have a port
      // called MagicInstrOut. 
      //WIRE( theDecoder, MagicInstrOut,        theLBAProducer, MagicInstructionIn )

      //uArch to FAG
      WIRE( theuArch, BranchFeedbackOut,      theFAG, BranchFeedbackIn          )
      WIRE( theuArch, RedirectOut,            theFAG, RedirectIn                )

      //uFetch to Split Destination Mapper
      WIRE( theuFetch, FetchMissOut,          theNetMapper, ICacheRequestIn     )   
      WIRE( theuFetch, FetchSnoopOut,         theNetMapper, ICacheSnoopIn       )   
      WIRE( theuFetch, FetchReplyOut,         theNetMapper, ICacheReplyIn       )  
	
	  //uArch to L1D cache
      WIRE( theuArch, MemoryOut_Request,      theL1d, FrontSideIn_Request       )
      WIRE( theuArch, MemoryOut_Snoop,        theL1d, FrontSideIn_Snoop         )
      WIRE( theL1d, FrontSideOut_D,           theuArch, MemoryIn               )
      
      //RMC to RMCL1D cache
      WIRE( theRMC, MemoryOut_Request,      theRMCL1, FrontSideIn_Request       )
      WIRE( theRMC, MemoryOut_Snoop,        theRMCL1, FrontSideIn_Snoop         )
      WIRE( theRMCL1, FrontSideOut_D,       theRMC, MemoryIn               )
    /*  WIRE( theRMC, ToRGPBackend ,     		theRMCL1, FromRGPFrontend      )
      WIRE( theRMC, ToRCPBackend,      		theRMCL1, FromRCPFrontend		)
      WIRE( theRMCL1, ToRGPBackend,      	theRMC, FromRGPFrontend        )
      WIRE( theRMCL1, ToRCPBackend,       	theRMC, FromRCPBackend         )*/
      
      WIRE( theNetMapper, ICacheSnoopOut,     theuFetchCombiner, SnoopIn         )
	  WIRE( theNetMapper, ICacheReplyOut,     theuFetchCombiner, ReplyIn         )
	
	  WIRE( theuFetchCombiner, FetchMissOut,  theuFetch, FetchMissIn             )
      
      //L1D Cache to NetMapper
      WIRE( theL1d, BackSideOut_Request,       theNetMapper, CacheRequestIn      )  
      WIRE( theL1d, BackSideOut_Snoop,         theNetMapper, CacheSnoopIn        )  
      WIRE( theL1d, BackSideOut_Reply,         theNetMapper, CacheReplyIn        )  
      WIRE( theNetMapper, CacheReplyOut,      theL1d, BackSideIn_Reply           ) 
      WIRE( theNetMapper, CacheSnoopOut,      theL1d, BackSideIn_Request         ) 
      
       //RMCL1D Cache to NetMapper
      WIRE( theRMCL1, BackSideOut_Request,       theNetMapper, RMCCacheRequestIn      )  
      WIRE( theRMCL1, BackSideOut_Snoop,         theNetMapper, RMCCacheSnoopIn        )  
      WIRE( theRMCL1, BackSideOut_Reply,         theNetMapper, RMCCacheReplyIn        )  
      WIRE( theNetMapper, RMCCacheReplyOut,      theRMCL1, BackSideIn_Reply           ) 
      WIRE( theNetMapper, RMCCacheSnoopOut,      theRMCL1, BackSideIn_Request         ) 
	
      //Memory to NetMapper
      WIRE( theNetMapper, MemoryOut,          theMemory, LoopbackIn             )
      WIRE( theMemory, LoopbackOut,           theNetMapper, MemoryIn            )

      //L2 to NetMapper
      WIRE( theNetMapper, DirReplyOut,        theL2, Reply_In            )
      WIRE( theNetMapper, DirSnoopOut,        theL2, Snoop_In            )
      WIRE( theNetMapper, DirRequestOut,      theL2, Request_In          )
      WIRE( theL2, Reply_Out,         	      theNetMapper, DirReplyIn          )
      WIRE( theL2, Snoop_Out,        		  theNetMapper, DirSnoopIn          )
      WIRE( theL2, Request_Out,       		  theNetMapper, DirRequestIn        )

      //NetMapper to Nic
      WIRE( theNetMapper, ToNIC0,             theNic, FromNode0          )
	  WIRE( theNic, ToNode0,                  theNetMapper, FromNIC0     )
      WIRE( theNetMapper, ToNIC1,             theNic, FromNode1          )
      WIRE( theNic, ToNode1,                  theNetMapper, FromNIC1     )
      WIRE( theNetMapper, ToNIC2,             theNic, FromNode2          )
      WIRE( theNic, ToNode2,                  theNetMapper, FromNIC2     )
      
      //Nodes to Network
      WIRE( theNic, ToNetwork,                theNetwork, FromNode              )
      WIRE( theNetwork, ToNode,               theNic, FromNetwork               )

      WIRE( theMagicBreak, ToTraceRMC,		  theRMC, RequestIn				)
      WIRE( theMagicBreak, ToTraceLBA,        theLBAProducer, FunctionIn )
      WIRE( theLBAProducer, CoreControlFlow,  theRMC, CoreControlFlow    )
      
      //Connections between RMCs
      WIRE( theRMC, RMCPacketOut,			  theRMCConnector, MessageIn		)
      WIRE( theRMCConnector, MessageOut,	  theRMC, RMCPacketIn				)
      //WIRE( theRMC, CheckLinkAvail,			  theRMCConnector, CheckLinkAvail	)
      //WIRE( theRMCConnector, LinkAvailRep,	  theRMC, LinkAvailRep				)
      
      WIRE( theRMC, RRPPLatency,			  theRMCConnector, RRPPLatency		)

        // FIXME: This was for the RMC traffic generator in our NeBuLa simulator.
        // Msutherl: Need to change this (or bring it back if we don't have enough time)
      //WIRE( theRMC, MessagingInfo,			  theRMCConnector, MessagingInfo		)
      
      //Connections between RMC caches and normal caches (for magic messages)
      WIRE( theRMC, MagicCacheAccess,		theL1d, MagicRMCAccess	)
      WIRE( theL1d, MagicRMCAccessReply, 	theRMC, MagicCacheAccessReply )
      WIRE( theRMC, MagicCoreCommunication,		theuArch, MagicRMCCommunication	)
      
#include FLEXUS_END_COMPONENT_WIRING_SECTION()


#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

      DRIVE( theuFetch, uFetchDrive )
    , DRIVE( theFAG, FAGDrive )
    , DRIVE( theuArch, uArchDrive )
    , DRIVE( theMMU, MMUDrive  )
    , DRIVE( theRMC, uArchDrive )
    , DRIVE( theDecoder, DecoderDrive )
    , DRIVE( theNic, MultiNicDrive )
    , DRIVE( theNetwork, NetworkDrive )
#ifndef USE_DRAMSIM
    , DRIVE( theMemory, LoopbackDrive )
#else
	, DRIVE( theMemory, DRAMDrive )
#endif
	, DRIVE( theL2, CMPCacheDrive )
    , DRIVE( theL1d, CacheDrive )
    , DRIVE( theRMCL1, CacheDrive )
    , DRIVE( theMagicBreak, TickDrive )
    , DRIVE( theRMCConnector, ProcessMessages )
    , DRIVE( theLBAProducer, Cycle )

#include FLEXUS_END_DRIVE_ORDER_SECTION()
