#include <core/simulator_layout.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Transports/NetworkTransport.hpp>
#include <components/CommonQEMU/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/uFetch/uFetchTypes.hpp>

//RMC STUFF - BEGIN

#include <components/CommonQEMU/RMCEntry.hpp>
//#include <components/CommonQEMU/Slices/RMCPacket.hpp>
#include <components/CommonQEMU/rmc_breakpoints.hpp>
#include <components/CommonQEMU/RMC_SRAMS.hpp>
#include <son-common/libsonuma/flexus_include.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/export.hpp>
#include <fstream>
#include <string>
#include <bitset>

#include "RMCpipelines.hpp"

#include <stdlib.h>
#ifdef __GNUC__
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif

//%%%%%%%% FOR SPLIT-RMC
#define OUTSTANDING_FE_TRANSFERS_LIMIT 10    //Flow control: # of outstanding transfers, regardless of transfer size (per RGP FE)
//%%%%%%%%
/* Flow control: # of outstanding remote cache block access requests
 * (per RGP BE). Size to match RCP BE's bandwidth.
 * For an E2E latency of X cycles, and a peak BW of ~20GBps per RCP
 * need a limit of about 0.17 * X packets in flight
 * e.g., for two single-core machines w/ an E2E lat of ~300 cycles => ~51
 * TODO: For SABRes, it probably makes sense to increase the # of
 * outstanting reqs, so that the latency of the 1st sync access gets
 * TODO: For SABRes, it probably makes sense to increase the # of
 * outstanting reqs, so that the latency of the 1st sync access gets
>>>>>>> SABRes++
 * masked. Maybe makes sense to make the transfer (rather than the packet)
 * the unit of congestion management.
 * The following number is ***CRITICAL*** for performance. Too small number can throttle the peak throughput, while too big a number
 * can create performance problems (bandwidth collapse, etc.)
 */
#define OUTSTANDING_PACKET_LIMIT 256 //60 //256 //60 - SABRes need much higher # (e.g., 256 vs. 60)
#define PREFETCH_TOKENS		  5		//How many of the outstanding reqs can be given to SABRe prefetches
#define MACHINE_COUNT 2     //This just defines the sizing of structures for stats. If more machines needed, just increase this value.
#define RMC_FREQ_DIVIDER 2  //divides the frequency of the core by this number, and uses that as the RMCs' frequency
#define RGP_POLL_FREQ   10  //how frequently the RGP polls for new WQ entries (in CPU cycles)

#define MAX_CORE_COUNT 64

#define SERVICE_TIME_MEASUREMENT_CORE 0 /* Msutherl: for 1 core */

//#define PRINT_PER_TRANSFER_LATENCY        //if enabled, prints the E2E latency of every completed transfer - works for sync ops
//#define TRACE_LATENCY
//#define FLOW_CONTROL_STATS   //enables flow-control-related debug messages
#define FC_DBG_LVL  Iface

//==========================
//HACKS FOR TESTING

//#define SYNC_OPERATION  //force synchronous operation of RMC on each WQ - only one in-flight entry at a time
//#define SABRE_IS_RREAD  //Do NOT enable this, unless you want your SABRes to become normal RREADs :-)
//#define RREAD_IS_SABRE  //Do NOT enable this, unless you want your normal RREADs to become SABRes :-)
//#define SINGLE_RMC      //only one RCP will be processing new WQ requests. The id of the active RGP is defined below:
#ifdef SINGLE_RMC
    #define ACTIVE_RGP 0
#endif
//#define NO_DATA_WRITEBACK //Data received by RCPs in RRREPLIES get dropped without getting written back in the source node's memory hierarchy
//#define RRPP_FAKE_DATA_ACCESS //RRPPs don't really access the memory hierarchy; they just reply after a fixed amount of time
#ifdef RRPP_FAKE_DATA_ACCESS
    #define RRPP_RESPONSE_TIME 200
#endif
//#define SINGLE_MACHINE  //For multinode; disable RGPs of all machines other than the machine specified below
#ifdef SINGLE_MACHINE
    #define SINGLE_MACHINE_ID   0
#endif
//===========================

#define STATS_PRINT_FREQ 100000	//cycles
#define MEMCHECK_DEBUG  Iface   //this enables messages of internal RMC queues to check for leaks

//debug levels
#define RMC_DEBUG               Verb
#define STATS_DEBUG             Verb	//Tmp
#define SABRE_STATS             Verb  //Tmp
#define PW_DEBUG                VVerb
#define WQ_CQ_ACTIONS_DEBUG     Trace //Msgs for a) when app starts enqueueing WQ, b) when WQ enqueuing finishes, c) when RMC writes CQ, d) when app reads CQ
#define BUFFER_OCCUP_DEBUG      VVerb
#define RGP_DEBUG               Trace//Trace
#define RGP_FRONTEND_DEBUG      Trace 
#define RMC_BACKPRESSURE        Verb
#define LOAD_STATE_DEBUG        Verb
#define MAGIC_CONNECTIONS_DEBUG Verb

#define SABRE_DEBUG             Trace
#define BREAKPOINTS_DEBUG       Tmp//Trace
#define BREAKPOINTS_DEBUG_VERB  Trace
#define SPLIT_RGP_DEBUG         Trace
#define RCP_DEBUG               Trace
#define RCP_BACKEND_DEBUG       Trace//Trace
#define RRPP_DEBUG              Trace
#define RRPP_STREAM_BUFFERS     Trace //Trace
#define MESSAGING_DEBUG         Tmp//Trace
#define PROGRESS_DEBUG          Trace 
#define SEND_LOCK_DEBUG         Verb

#define MEMORY_ACCESS_DEBUG     Iface     //print messages about memory accesses the RMC performs

#define NO_PAGE_WALKS 1					//CHANGE!!!

#define DATA_STORE_OP 		StoreReq    //StoreDataInLLC //StoreData
#define DATA_STORE_REPLY 	StoreReply  //StoreDataInLLCReply //StoreDataReply
#define DATA_LOAD_OP		LoadReq     //LoadDataInLLC //LoadData
#define DATA_LOAD_REPLY		LoadRep     //LoadDataInLLCReply	//LoadDataReply

#define DATA_LOAD_L1	LoadReq		//data gets allocated in RMC's L1D cache
#define DATA_STORE_L1	StoreReq

#define REPLY 0
#define REQUEST 1

typedef enum RMCStatus
{
	INIT,
	READY
} RMCStatus;

typedef enum pipelineId
{
	_RGP,
	_RCP,
	_RRPP
} pipelineId;


typedef enum pendingOpType
{
	Pagewalk1,				//0
	Pagewalk2,				//1
	Pagewalk3,				//2
//RGP
	WQHRead,				//3
	WQTRead,				//4
	WQRead,					//5
	WQTWrite,				//6
	BufferReadForRequest,	//7
	UnrollRRead,			//8
	UnrollRWrite,			//9
//RRPP
	ContextRead,		//10
	ContextWrite,	//11
	ContextRMW, //12
    ContextPrefetch,		//13
//RCP
    CQWrite1,               //14
    CQWrite,                //15
    CQHRead,                //16
    CQHUpdate,              //17
    BufferWriteForReply,    //18
//RGP
    BufferReadForSend,
//RRPP,
    RecvBufWrite,
    RecvBufUpdate,
    SendBufFree,
	BOGUS
} pendingOpType;
std::ostream & operator << (std::ostream & s, pendingOpType const & anOpType);

typedef struct {
	Flexus::SharedTypes::MemoryTransport theTransport;
	pendingOpType type;
	bool dispatched;
	//bool completed;
	uint64_t thePAddr;
	uint64_t theVAddr;
	uint64_t bufAddrV;
	uint16_t src_nid;
	uint16_t tid;
	uint64_t offset;	//to track multiple packets of requests larger than a block
	uint64_t pl_size;	//for packets
    uint32_t ITTidx;	//only used by RCP for BufferWriteForReply
	pipelineId pid;
	Flexus::SharedTypes::RMCPacket packet;	//only for UnrollRRead, UnrollRWrite
    //uint16_t CQEntries[16];		//only used for CQWrite  //protocol v2.2
  uint64_t CQEntries[8];  //protocol v2.3
} MAQEntry;

//RMC STUFF - END

#define FLEXUS_BEGIN_COMPONENT uArchARM
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( ROBSize, uint32_t, "Reorder buffer size", "rob", 256 )
  PARAMETER( SBSize, uint32_t, "Store buffer size", "sb", 256 )
  PARAMETER( NAWBypassSB, bool, "Allow Non-Allocating-Writes to bypass store-buffer", "naw_bypass_sb", false )
  PARAMETER( NAWWaitAtSync, bool, "Force MEMBAR #Sync to wait for non-allocating writes to finish", "naw_wait_at_sync", false )
  PARAMETER( RetireWidth, uint32_t, "Retirement width", "retire", 8 )
  PARAMETER( MemoryPorts, uint32_t, "Memory Ports", "memports", 4 )
  PARAMETER( SnoopPorts, uint32_t, "Snoop Ports", "snoopports", 1 )
  PARAMETER( StorePrefetches, uint32_t, "Simultaneous store prefeteches", "storeprefetch", 16 )
  PARAMETER( PrefetchEarly, bool, "Issue store prefetch requests when address resolves", "prefetch_early", false )
  PARAMETER( ConsistencyModel, uint32_t, "Consistency Model", "consistency", 0 /* SC */ )
  PARAMETER( CoherenceUnit, uint32_t, "Coherence Unit", "coherence", 64 )
  PARAMETER( BreakOnResynchronize, bool, "Break on resynchronizer", "break_on_resynch", false )
  PARAMETER( SpinControl, bool, "Enable spin control", "spin_control", true )
  PARAMETER( SpeculativeOrder, bool, "Speculate on Memory Order", "spec_order", false )
  PARAMETER( SpeculateOnAtomicValue, bool, "Speculate on the Value of Atomics", "spec_atomic_val", false )
  PARAMETER( SpeculateOnAtomicValuePerfect, bool, "Use perfect atomic value prediction", "spec_atomic_val_perfect", false )
  PARAMETER( SpeculativeCheckpoints, int, "Number of checkpoints allowed.  0 for infinite", "spec_ckpts", 0)
  PARAMETER( CheckpointThreshold, int, "Number of instructions between checkpoints.  0 disables periodic checkpoints", "ckpt_threshold", 0)
  PARAMETER( EarlySGP, bool, "Notify SGP Early", "early_sgp", false )   /* CMU-ONLY */
  PARAMETER( TrackParallelAccesses, bool, "Track which memory accesses can proceed in parallel", "track_parallel", false ) /* CMU-ONLY */
  PARAMETER( InOrderMemory, bool, "Only allow ROB/SB head to issue to memory", "in_order_memory", false )
  PARAMETER( InOrderExecute, bool, "Ensure that instructions execute in order", "in_order_execute", false )
  PARAMETER( OnChipLatency, uint32_t, "On-Chip Side-Effect latency", "on-chip-se", 0)
  PARAMETER( OffChipLatency, uint32_t, "Off-Chip Side-Effect latency", "off-chip-se", 0)
  PARAMETER( Multithread, bool, "Enable multi-threaded execution", "multithread", false )

  PARAMETER( NumIntAlu, uint32_t, "Number of integer ALUs", "numIntAlu", 1)
  PARAMETER( IntAluOpLatency, uint32_t, "End-to-end latency of an integer ALU operation", "intAluOpLatency", 1)
  PARAMETER( IntAluOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent integer ALU operations", "intAluOpPipelineResetTime", 1)

  PARAMETER( NumIntMult, uint32_t, "Number of integer MUL/DIV units", "numIntMult", 1)
  PARAMETER( IntMultOpLatency, uint32_t, "End-to-end latency of an integer MUL operation", "intMultOpLatency", 1)
  PARAMETER( IntMultOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent integer MUL operations", "intMultOpPipelineResetTime", 1)
  PARAMETER( IntDivOpLatency, uint32_t, "End-to-end latency of an integer DIV operation", "intDivOpLatency", 1)
  PARAMETER( IntDivOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent integer DIV operations", "intDivOpPipelineResetTime", 1)

  PARAMETER( NumFpAlu, uint32_t, "Number of FP ALUs", "numFpAlu", 1)
  PARAMETER( FpAddOpLatency, uint32_t, "End-to-end latency of an FP ADD/SUB operation", "fpAddOpLatency", 1)
  PARAMETER( FpAddOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP ADD/SUB operations", "fpAddOpPipelineResetTime", 1)
  PARAMETER( FpCmpOpLatency, uint32_t, "End-to-end latency of an FP compare operation", "fpCmpOpLatency", 1)
  PARAMETER( FpCmpOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP compare operations", "fpCmpOpPipelineResetTime", 1)
  PARAMETER( FpCvtOpLatency, uint32_t, "End-to-end latency of an FP convert operation", "fpCvtOpLatency", 1)
  PARAMETER( FpCvtOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP convert operations", "fpCvtOpPipelineResetTime", 1)

  PARAMETER( NumFpMult, uint32_t, "Number of FP MUL/DIV units", "numFpMult", 1)
  PARAMETER( FpMultOpLatency, uint32_t, "End-to-end latency of an FP MUL operation", "fpMultOpLatency", 1)
  PARAMETER( FpMultOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP MUL operations", "fpMultOpPipelineResetTime", 1)
  PARAMETER( FpDivOpLatency, uint32_t, "End-to-end latency of an FP DIV operation", "fpDivOpLatency", 1)
  PARAMETER( FpDivOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP DIV operations", "fpDivOpPipelineResetTime", 1)
  PARAMETER( FpSqrtOpLatency, uint32_t, "End-to-end latency of an FP SQRT operation", "fpSqrtOpLatency", 1)
  PARAMETER( FpSqrtOpPipelineResetTime, uint32_t, "Number of cycles required between subsequent FP SQRT operations", "fpSqrtOpPipelineResetTime", 1)
  PARAMETER( ValidateMMU, bool, "Validate MMU after each instruction", "validate-mmu", false )

  //RMC STUFF - BEGIN
  PARAMETER( IsRMC, bool, "Indicates if this uArch is an RMC", "IsRMC", false)
  PARAMETER( RMCTLBEntries, uint32_t, "Number of RMC TLB entries", "RMCTLBEntries", 5)
  //PARAMETER( CTCacheEntries, uint32_t, "Number of Context Table Cache entries", "CTCacheEntries", 5)
  PARAMETER( RMCTextFlexpoints, bool, "Store RMC flexpoints as text files", "RMCtext_flexpoints", true )
  PARAMETER( RMCGZipFlexpoints, bool, "Compress RMC flexpoints with gzip", "RMCgzip_flexpoints", false )
  PARAMETER( FlexiAppReqSize, uint32_t, "Size of each soNUMA request. Applies only to Flexi apps", "flexi_app_req_size", 64 )
  PARAMETER( OneSided, bool, "One sided ops (don't write back reply data)", "one_sided", false )
  PARAMETER( RGPPerTile, bool, "Put one RGP/RCP per tile. Default is one per row across the first column", "rgp_per_tile", false )
  PARAMETER( RRPPPerTile, bool, "Put one RRPP per tile. Default is one per row across the first column", "rrpp_per_tile", false )
  PARAMETER( RRPPWithMCs, bool, "Put the RRPPs on the MC tiles (last column). Default is on the first column", "rrpps_with_mcs", false )
  PARAMETER( EnableSingleRMCLoading, bool, "When enabled, loads RMCs from a single RMC file (needs to be compatible with flexpoint)", "enable_single_RMC_loading", false )
  PARAMETER( LoadFromSimics, bool, "Enables RMC state loading from a file generated by Simics, if 0 - from trace simulator", "load_from_simics_file", false )
  PARAMETER( SplitRGP, bool, "Enables splitting the RGP/RCP into two to physically decouple them", "splitRGP", false )		//When this option is enabled, there is an RGP frontend and an RCP backend per tile,
																															//and an RGP backend and an RCP frontend per edge
  PARAMETER( MagicCacheConnection, bool, "Enables the RMC to directly check related core's L1d cache", "MagicCacheConnection", false )

  PARAMETER( MachineCount, uint32_t, "Number of machines", "MachineCount", 1 )

  //PARAMETERS FOR SABRES
  PARAMETER( RRPPPrefetchSABRe, bool, "Enable prefetching of blocks when doing SABRes", "RRPPPrefetchSABRe", false )
  PARAMETER( PrefetchBlockLimit, uint32_t, "Upper limit of prefetched cache blocks per SABRe", "PrefetchBlockLimit", 1000 )
  PARAMETER( RGPPrefetchSABRe, bool, "Allow RGP to peek following WQ entries for SABRes to issue prefetch for SABRe's version", "RGPPrefetchSABRe", false )
  PARAMETER( SABReToRRPPMatching, std::string, "How to assign SABRes to remote RRPPs", "SABReToRRPPMatching", "random" )
  PARAMETER( SABReStreamBuffers, bool, "Whether to use Stream Buffer mechanism for SABRes", "SABReStreamBuffers", false )

  PARAMETER( SelfCleanSENDs, bool, "RGP invalidates SEND entry automatically without waiting for RECV", "SelfCleanSENDs", false ) //this is a hack just for facilitating the RMCTrafficGenerator. Useful when single machine is simulated

  PARAMETER( SingleRMCDispatch, bool, "Sigle RCP backend dispatches incoming SEND requests to cores", "SingleRMCDispatch", false )
  PARAMETER( RMCDispatcherID, uint32_t, "ID of the RCP backend handling all SEND dispatches", "RMCDispatcherID", 0 )    //only relevant if previous parameter is set

  PARAMETER( PerCoreReqQueue, uint32_t, "The depth of the request queue built on top of each core", "PerCoreReqQueue", 1 )  //this is for messaging -  a depth of 1 is necessary for true single-queue system, but it introduces bubbles because of the time required to communicate info between the RMCs and the cores

  PARAMETER( DataplaneEmulation, bool, "Maintain separate request queue for incoming SENDs for each core", "DataplaneEmulation", false ) 

  PARAMETER( ServiceTimeMeasurement, bool, "use single core to determine service time", "ServiceTimeMeasurement", false )
  //RMC STUFF - END
);

typedef std::pair<int, bool> dispatch_status;

COMPONENT_INTERFACE(
  PORT( PushInput, boost::intrusive_ptr< AbstractInstruction >, DispatchIn)
  PORT( PullOutput, dispatch_status, AvailableDispatchOut)
  PORT( PullOutput, bool, Stalled)
  PORT( PullOutput, int, ICount)
  PORT( PushOutput, eSquashCause, SquashOut )
  PORT( PushOutput, VirtualMemoryAddress , RedirectOut )
  PORT( PushOutput, CPUState, ChangeCPUState )
  PORT( PushOutput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackOut )
  PORT( PushOutput, PredictorTransport, NotifyTMS ) /* CMU-ONLY */
  PORT( PushOutput, MemoryTransport, MemoryOut_Request )
  PORT( PushOutput, MemoryTransport, MemoryOut_Snoop )
  PORT( PushInput, MemoryTransport, MemoryIn )
  PORT( PushInput, PhysicalMemoryAddress, WritePermissionLost )
  PORT( PushOutput, bool, StoreForwardingHitSeen) // Signal a store forwarding hit in the LSQ to the PowerTracker
  PORT( PushOutput, bool, ResyncOut )

  PORT( PushOutput, TranslationPtr, dTranslationOut )
  PORT( PushInput, TranslationPtr,  dTranslationIn )
  PORT( PushInput, TranslationPtr,  MemoryRequestIn )

  PORT( PushInput, RMCEntry, RequestIn )

  DYNAMIC_PORT_ARRAY( PushInput,  RMCPacket, RMCPacketIn )
  DYNAMIC_PORT_ARRAY( PushOutput, RMCPacket, RMCPacketOut )

 /* PORT( PushOutput, MemoryTransport, ToRGPBackend )		//For Frontend RGPs (per-tile)
  PORT( PushInput, MemoryTransport, FromRCPFrontend )

  PORT( PushOutput, MemoryTransport, ToRCPBackend )		//For Backend RGPs (on edges)
  PORT( PushInput, MemoryTransport, FromRGPFrontend )*/
 // DYNAMIC_PORT_ARRAY( PushOutput, RMCPacket, CheckLinkAvail )
 // DYNAMIC_PORT_ARRAY(PORT( PushInput, uint32_t, LinkAvailRep )

  PORT( PushOutput, uint64_t, RRPPLatency )		//Constantly updates the RMCConnector about the average latency of RRPP
												//(from the moment a packet reaches the inQueueReq to the moment it leaves the outQueueRep)


  //ALEX - for magic connection of RMCs with the caches of cores
  PORT( PushInput, bool, MagicCacheAccessReply )
  PORT( PushOutput, MemoryTransport, MagicCacheAccess )
  PORT( PushOutput, MemoryTransport, MagicCoreCommunication )
  PORT( PushInput, MemoryTransport, MagicRMCCommunication )

  //ALEX - for RRPP communication with the Stream Buffers (for SABRes)
  PORT( PushOutput, SABReEntry, AllocateStrBuf )          //For RMC to check for StreamBuffer availability and allocate one if available
  PORT( PushOutput, uint32_t, FreeStrBuf )
  PORT( PushOutput, uint32_t, AllocateStrBufEntry ) //For RMC to allocate an entry in a particular StreamBuffer (using an ATT index)
  PORT( PushInput, bool, StrBufEntryAvail )        //For replies to the requests that are sent through the previous port
  PORT( PushInput, uint32_t, SABReInval )          //Notify the RMC about an invalidation msg received for a SB entry (message contains corresponding ATT index)

  PORT( PushOutput, messaging_struct_t, MessagingInfo ) //to pass required messaging info to the RMCTrafficGenerator
  PORT( PushInput, uint32_t, CoreControlFlow )    //connects to LBAProducer

  DRIVE( uArchDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT uArchARM
