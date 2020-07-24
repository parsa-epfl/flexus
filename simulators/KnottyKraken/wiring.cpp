//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>

// This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
std::string theSimulatorName = "KnottyKraken v1.0";
}

#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/CMPCache/CMPCache.hpp>
#include <components/Cache/Cache.hpp>
#include <components/FetchAddressGenerate/FetchAddressGenerate.hpp>
#include <components/MMU/MMU.hpp>
#include <components/MagicBreakQEMU/MagicBreak.hpp>
#include <components/MemoryLoopback/MemoryLoopback.hpp>
#include <components/MemoryMap/MemoryMap.hpp>
#include <components/MultiNic/MultiNic2.hpp>
#include <components/NetShim/MemoryNetwork.hpp>
#include <components/SplitDestinationMapper/SplitDestinationMapper.hpp>
#include <components/armDecoder/armDecoder.hpp>
#include <components/uArchARM/uArchARM.hpp>
#include <components/uFetch/PortCombiner.hpp>
#include <components/uFetch/uFetch.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION(MMU, "mmu", theMMUCfg);
CREATE_CONFIGURATION(FetchAddressGenerate, "fag", theFAGCfg);
CREATE_CONFIGURATION(uFetch, "ufetch", theuFetchCfg);
CREATE_CONFIGURATION(PortCombiner, "combiner", theCombinerCfg);
CREATE_CONFIGURATION(armDecoder, "decoder", theDecoderCfg);
CREATE_CONFIGURATION(uArchARM, "uarcharm", theuArchCfg);

CREATE_CONFIGURATION(Cache, "L1d", theL1dCfg);
CREATE_CONFIGURATION(CMPCache, "L2", theL2Cfg);
CREATE_CONFIGURATION(MultiNic2, "nic", theNicCfg);
CREATE_CONFIGURATION(MemoryNetwork, "network", theNetworkCfg);

CREATE_CONFIGURATION(SplitDestinationMapper, "net-mapper", theNetMapperCfg);

CREATE_CONFIGURATION(MemoryLoopback, "memory", theMemoryCfg);
CREATE_CONFIGURATION(MemoryMap, "memory-map", theMemoryMapCfg);
CREATE_CONFIGURATION(MagicBreak, "magic-break", theMagicBreakCfg);

// You may optionally initialize configuration parameters from within this
// function.  This initialization occur before the command line is processed,
// so they will be overridden from the command line.
//
// Return value indicates whether simulation should abort if any parameters
// are left at their default values;
bool initializeParameters() {
  DBG_(Dev, (<< " initializing Parameters..."));

  theFAGCfg.Threads.initialize(1);
  theFAGCfg.MaxFetchAddress.initialize(1);
  theFAGCfg.MaxBPred.initialize(1);

  theuFetchCfg.Threads.initialize(1);
  theuFetchCfg.FAQSize.initialize(1000);
  theuFetchCfg.MaxFetchLines.initialize(2);
  theuFetchCfg.MaxFetchInstructions.initialize(1000);
  theuFetchCfg.ICacheLineSize.initialize(64);
  theuFetchCfg.PerfectICache.initialize(true);
  theuFetchCfg.PrefetchEnabled.initialize(true);
  theuFetchCfg.Size.initialize(65536);
  theuFetchCfg.Associativity.initialize(2);
  theuFetchCfg.MissQueueSize.initialize(4);
  theuFetchCfg.CleanEvict.initialize(false);
  theuFetchCfg.SendAcks.initialize(true);
  theuFetchCfg.UseReplyChannel.initialize(true);
  theuFetchCfg.EvictOnSnoop.initialize(true);

  theDecoderCfg.Multithread.initialize(false);
  theDecoderCfg.FIQSize.initialize(1);
  theDecoderCfg.DispatchWidth.initialize(1);

  theuArchCfg.Multithread.initialize(false);
  theuArchCfg.ROBSize.initialize(100);
  theuArchCfg.SBSize.initialize(64);
  theuArchCfg.NAWBypassSB.initialize(true);
  theuArchCfg.NAWWaitAtSync.initialize(false);
  theuArchCfg.RetireWidth.initialize(1);
  theuArchCfg.SnoopPorts.initialize(1);
  theuArchCfg.MemoryPorts.initialize(4);
  theuArchCfg.StorePrefetches.initialize(30);
  theuArchCfg.PrefetchEarly.initialize(false);
  theuArchCfg.ConsistencyModel.initialize(1); // TSO
  theuArchCfg.CoherenceUnit.initialize(64);
  theuArchCfg.BreakOnResynchronize.initialize(false);
  theuArchCfg.SpinControl.initialize(true);
  theuArchCfg.SpeculativeOrder.initialize(false);
  theuArchCfg.SpeculateOnAtomicValue.initialize(false);
  theuArchCfg.SpeculateOnAtomicValuePerfect.initialize(false);
  theuArchCfg.SpeculativeCheckpoints.initialize(0);
  theuArchCfg.CheckpointThreshold.initialize(0);
  theuArchCfg.InOrderExecute.initialize(true);
  theuArchCfg.InOrderMemory.initialize(false);
  theuArchCfg.OffChipLatency.initialize(320);
  theuArchCfg.OnChipLatency.initialize(3);

  theuArchCfg.EarlySGP.initialize(false);              // CMU-ONLY
  theuArchCfg.TrackParallelAccesses.initialize(false); // CMU-ONLY

  theuArchCfg.FpAddOpLatency.initialize(true);
  theuArchCfg.FpAddOpPipelineResetTime.initialize(true);

  theuArchCfg.FpCmpOpLatency.initialize(true);
  theuArchCfg.FpCmpOpPipelineResetTime.initialize(true);
  theuArchCfg.FpCvtOpLatency.initialize(true);
  theuArchCfg.FpCvtOpPipelineResetTime.initialize(true);

  theuArchCfg.FpDivOpLatency.initialize(true);
  theuArchCfg.FpDivOpPipelineResetTime.initialize(true);
  theuArchCfg.FpMultOpLatency.initialize(true);
  theuArchCfg.FpMultOpPipelineResetTime.initialize(true);

  theuArchCfg.FpSqrtOpLatency.initialize(true);
  theuArchCfg.FpSqrtOpPipelineResetTime.initialize(true);
  theuArchCfg.IntAluOpLatency.initialize(true);
  theuArchCfg.IntAluOpPipelineResetTime.initialize(true);

  theuArchCfg.IntDivOpLatency.initialize(true);
  theuArchCfg.IntDivOpPipelineResetTime.initialize(true);
  theuArchCfg.IntMultOpLatency.initialize(true);
  theuArchCfg.IntMultOpPipelineResetTime.initialize(true);

  theuArchCfg.NumFpAlu.initialize(true);
  theuArchCfg.NumFpMult.initialize(true);
  theuArchCfg.NumIntAlu.initialize(true);
  theuArchCfg.NumIntMult.initialize(true);

  static const int K = 1024;

  theL1dCfg.Cores.initialize(1);
  theL1dCfg.BlockSize.initialize(64);
  theL1dCfg.Ports.initialize(2);
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
  theL1dCfg.CacheType.initialize("InclusiveMESI");
  theL1dCfg.ArrayConfiguration.initialize("STD:size=65536:assoc=4:repl=LRU");
  theL1dCfg.EvictOnSnoop.initialize(true);
  theL1dCfg.UseReplyChannel.initialize(true);

  theL1dCfg.GZipFlexpoints.initialize(false);
  theL1dCfg.TextFlexpoints.initialize(false);
  theL1dCfg.EvictWritableHasData.initialize(false);

  theL2Cfg.Cores.initialize(2);
  theL2Cfg.BlockSize.initialize(64);
  theL2Cfg.Banks.initialize(1);
  theL2Cfg.BankInterleaving.initialize(64);
  theL2Cfg.Groups.initialize(1);
  theL2Cfg.GroupInterleaving.initialize(4096);
  theL2Cfg.DirLatency.initialize(3);
  theL2Cfg.DirIssueLatency.initialize(1);
  theL2Cfg.TagLatency.initialize(1);
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
  theL2Cfg.ArrayConfiguration.initialize("STD:sets=1024:assoc=16:repl=LRU");

  theL2Cfg.ControllerType.initialize("Default");

  theNicCfg.VChannels.initialize(3);
  theNicCfg.RecvCapacity.initialize(4);
  theNicCfg.SendCapacity.initialize(1);

  theNetworkCfg.NetworkTopologyFile.initialize("1x3-mesh.topology");
  theNetworkCfg.NumNodes.initialize(3);
  theNetworkCfg.VChannels.initialize(3);

  theNetMapperCfg.Cores.initialize(1);
  theNetMapperCfg.Directories.initialize(1);
  theNetMapperCfg.Banks.initialize(1);
  theNetMapperCfg.MemControllers.initialize(1);
  theNetMapperCfg.DirInterleaving.initialize(64);
  theNetMapperCfg.MemInterleaving.initialize(4096);
  theNetMapperCfg.DirLocation.initialize("Distributed");
  theNetMapperCfg.MemLocation.initialize("0");
  theNetMapperCfg.MemReplyToDir.initialize(true);
  theNetMapperCfg.MemAcksNeedData.initialize(true);
  theNetMapperCfg.DirXORShift.initialize(false);
  theNetMapperCfg.LocalDir.initialize(false);
  theNetMapperCfg.MemXORShift.initialize(false);
  theNetMapperCfg.TwoPhaseWB.initialize(false);

  theMemoryCfg.Delay.initialize(1);
  theMemoryCfg.MaxRequests.initialize(64);
  theMemoryCfg.UseFetchReply.initialize(true);

  theMemoryMapCfg.PageSize.initialize(8 * K);
  theMemoryMapCfg.NumNodes.initialize(1);
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

  theMMUCfg.Cores.initialize(1);
  theMMUCfg.iTLBSize.initialize(64);
  theMMUCfg.dTLBSize.initialize(64);
  theMMUCfg.PerfectTLB.initialize(true);

  theFlexus->setStatInterval("100000");
  theFlexus->setProfileInterval("10000000");
  theFlexus->setTimestampInterval("50000");

  return true; // true = Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()

// clang-format off
#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
// All component Instances are created here.  This section
// also creates handles for each component
//
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( FetchAddressGenerate, theFAGCfg, theFAG, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uFetch, theuFetchCfg, theuFetch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( PortCombiner, theCombinerCfg, theuFetchCombiner, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( armDecoder, theDecoderCfg, theDecoder, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( uArchARM, theuArchCfg, theuArch, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Cache, theL1dCfg, theL1d, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT_ARRAY( MMU , theMMUCfg, theMMU, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
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
//FAG to Fetch
WIRE( theFAG, FetchAddrOut,             theuFetch, FetchAddressIn         )
WIRE( theFAG, AvailableFAQ,             theuFetch, AvailableFAQOut        )
WIRE( theFAG, uArchHalted,              theuArch, CoreHalted              )

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

//uArch to FAG
WIRE( theuArch, BranchFeedbackOut,      theFAG, BranchFeedbackIn          )
WIRE( theuArch, RedirectOut,            theFAG, RedirectIn                )

//uFetch to IDMux
WIRE( theuFetch, FetchMissOut,          theNetMapper, ICacheRequestIn     )
WIRE( theuFetch, FetchSnoopOut,         theNetMapper, ICacheSnoopIn       )
WIRE( theuFetch, FetchReplyOut,         theNetMapper, ICacheReplyIn       )

//uArch to L1 D cache
WIRE( theuArch, MemoryOut_Request,      theL1d, FrontSideIn_Request       )
WIRE( theuArch, MemoryOut_Snoop,        theL1d, FrontSideIn_Snoop         )
WIRE( theL1d, FrontSideOut_D,           theuArch, MemoryIn                )

WIRE( theNetMapper, ICacheSnoopOut,     theuFetchCombiner, SnoopIn        )
WIRE( theNetMapper, ICacheReplyOut,     theuFetchCombiner, ReplyIn        )

WIRE( theuFetchCombiner, FetchMissOut,  theuFetch, FetchMissIn            )

//L1d to NetMapper
WIRE( theL1d, BackSideOut_Request,       theNetMapper, CacheRequestIn     )
WIRE( theL1d, BackSideOut_Snoop,         theNetMapper, CacheSnoopIn       )
WIRE( theL1d, BackSideOut_Reply,         theNetMapper, CacheReplyIn       )
WIRE( theNetMapper, CacheSnoopOut,      theL1d, BackSideIn_Request        )
WIRE( theNetMapper, CacheReplyOut,      theL1d, BackSideIn_Reply          )

//Memory to NetMapper
WIRE( theNetMapper, MemoryOut,          theMemory, LoopbackIn             )
WIRE( theMemory, LoopbackOut,           theNetMapper, MemoryIn            )

//Directory to NetMapper
WIRE( theNetMapper, DirReplyOut,        theL2, Reply_In                   )
WIRE( theNetMapper, DirSnoopOut,        theL2, Snoop_In                   )
WIRE( theNetMapper, DirRequestOut,      theL2, Request_In                 )
WIRE( theL2, Reply_Out,                 theNetMapper, DirReplyIn          )
WIRE( theL2, Snoop_Out,                 theNetMapper, DirSnoopIn          )
WIRE( theL2, Request_Out,               theNetMapper, DirRequestIn        )

//NetMapper to Nic
WIRE( theNetMapper, ToNIC0,             theNic, FromNode0                 )
WIRE( theNic, ToNode0,                  theNetMapper, FromNIC0            )
WIRE( theNetMapper, ToNIC1,             theNic, FromNode1                 )
WIRE( theNic, ToNode1,                  theNetMapper, FromNIC1            )

//Nodes to Network
WIRE( theNic, ToNetwork,                theNetwork, FromNode              )
WIRE( theNetwork, ToNode,               theNic, FromNetwork               )

#include FLEXUS_END_COMPONENT_WIRING_SECTION()

#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theuFetch, uFetchDrive )
, DRIVE( theFAG, FAGDrive )
, DRIVE( theuArch, uArchDrive )
, DRIVE( theMMU, MMUDrive  )
, DRIVE( theDecoder, DecoderDrive )
, DRIVE( theNic, MultiNicDrive )
, DRIVE( theNetwork, NetworkDrive )
, DRIVE( theMemory, LoopbackDrive )
, DRIVE( theL2, CMPCacheDrive )
, DRIVE( theL1d, CacheDrive )
, DRIVE( theMagicBreak, TickDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()
    // clang-format on
