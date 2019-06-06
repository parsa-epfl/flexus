#include <components/uArchARM/uArchARM.hpp>

#define FLEXUS_BEGIN_COMPONENT uArchARM
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Util.hpp>
#include <components/CommonQEMU/MessageQueues.hpp>
using nCommonUtil::log_base2;
#include <components/MTManager/MTManager.hpp>
#include <core/debug/debug.hpp>
#include <core/qemu/mai_api.hpp>
#include "microArch.hpp"
#include "uArchInterfaces.hpp"

#define DBG_DefineCategories uArchCat, Special
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

using namespace Flexus;
using namespace Flexus::Qemu;
//namespace Qemu = Flexus::Qemu;
//namespace API = Qemu::API;

// Msutherl: Wrapper function to interact with QEMU API.
uint8_t QEMU_read_phys_memory_wrapper_byte(API::conf_object_t* aCPU, API::physical_address_t aPAddr, int aSize) {
    uint8_t tmp_arr[aSize];
    API::QEMU_read_phys_memory(tmp_arr, aPAddr, aSize);
    return tmp_arr[0];
}
uint16_t QEMU_read_phys_memory_wrapper_hword(API::conf_object_t* aCPU, API::physical_address_t aPAddr, int aSize) {
    uint8_t tmp_arr[aSize];
    uint16_t ret;
    assert( aSize == 2 );
    API::QEMU_read_phys_memory(tmp_arr, aPAddr, aSize);
    std::memcpy(&ret,tmp_arr,aSize);
    return ret;
}
uint32_t QEMU_read_phys_memory_wrapper_word(API::conf_object_t* aCPU, API::physical_address_t aPAddr, int aSize) {
    uint8_t tmp_arr[aSize];
    uint32_t ret;
    assert( aSize == 4 );
    API::QEMU_read_phys_memory(tmp_arr, aPAddr, aSize);
    std::memcpy(&ret,tmp_arr,aSize);
    return ret;
}
uint64_t QEMU_read_phys_memory_wrapper_dword(API::conf_object_t* aCPU, API::physical_address_t aPAddr, int aSize) {
    uint8_t tmp_arr[aSize];
    uint64_t ret;
    assert( aSize == 8 );
    API::QEMU_read_phys_memory(tmp_arr, aPAddr, aSize);
    std::memcpy(&ret,tmp_arr,aSize);
    return ret;
}


namespace nuArchARM {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using namespace RMCTypes;

// ustiugov: extern array declared in RMCdefines.h
const char* br_points[] = {
     "BLANK",            // 0
     "WQUEUE",           // 1
     "CQUEUE",           // 2
     "BUFFER",           // 3
     "PTENTRY",          // 4
     "NEWWQENTRY",       // 5
     "WQENTRYDONE",      // 6
     "ALL_SET",          // 7
     "TID_COUNTERS",     // 8
     "CONTEXT",          // 9
     "CONTEXTMAP",       // 10
     "WQHEAD",           // 11
     "WQTAIL",           // 12
     "CQHEAD",           // 13
     "CQTAIL",           // 14
     "SIM_BREAK",        // 15
     "NETPIPE_START",    // 16
     "NETPIPE_END",      // 17
     "RMC_DEBUG_BP",     // 18
     "BENCHMARK_END",     // 19
     "BUFFER_SIZE",      // 20
     "CONTEXT_SIZE",     // 21
     "NEWWQENTRY_START", // 22
     "NEW_SABRE",        //23
     "SABRE_SUCCESS",    //24
     "SABRE_ABORT",      //25
     "OBJECT_WRITE",		//26
     "LOCK_SPINNING",    //27
     "CS_START",          //28
     "MEASUREMENT",      //29
     "MEASUREMENT",    //30
     "SEND_BUF",
     "RECV_BUF",
     "MSG_BUF_ENTRY_SIZE",
     "NUM_NODES",
     "MSG_BUF_ENTRY_COUNT", //35
     "SENDREQRECEIVED",
     "SENDENQUEUED"
 };
 
 //these variables are shared among all instances of uArch
 uint32_t CORES = 0, CORES_PER_RMC = 0, MACHINES = 0, CORES_PER_MACHINE = 0;
 uint32_t FLEXI_APP_REQ_SIZE;
 bool ENABLE_SABRE_PREFETCH, RGP_PREFETCH_SABRE;
 uint32_t SABRE_PREFETCH_LIMIT;
 
 enum SABReToRRPPDistr_t {
         eRandom,
         eMatching   //match source RGP backend to destination RRPP
     } SABReToRRPPDistr;
 
 //common stats for SABRes
 uint64_t succeeded_SABRes = 0;
 uint64_t failed_SABRes = 0;
 uint64_t object_writes = 0;
 uint64_t SABRElat[MAX_CORE_COUNT], SABRElatTot[MAX_CORE_COUNT], SABReCount[MAX_CORE_COUNT]; //stats to measure critical section duration of SABRe; only meaningful for sync
 uint64_t CSlat[MAX_CORE_COUNT], CSlatTot[MAX_CORE_COUNT], CSCount[MAX_CORE_COUNT];  //stats to measure critical section duration of writers (for SABRe experiments)
 std::ofstream SABReLatFile;  //A file to dump the latency breakdown of each SABRe

class uArch_QemuObject_Impl  {
  std::shared_ptr<microArch> theMicroArch;
public:
  uArch_QemuObject_Impl(Flexus::Qemu::API::conf_object_t * /*ignored*/ ) {}

  void setMicroArch(std::shared_ptr<microArch> aMicroArch) {
    theMicroArch = aMicroArch;
  }

  void testCkptRestore() {
    DBG_Assert(theMicroArch);
    theMicroArch->testCkptRestore();
  }

  void printROB() {
    DBG_Assert(theMicroArch);
    theMicroArch->printROB();
  }
  void printMemQueue() {
    DBG_Assert(theMicroArch);
    theMicroArch->printMemQueue();
  }
  void printSRB() {
    DBG_Assert(theMicroArch);
    theMicroArch->printSRB();
  }
  void printMSHR() {
    DBG_Assert(theMicroArch);
    theMicroArch->printMSHR();
  }
  void pregs() {
    DBG_Assert(theMicroArch);
    theMicroArch->pregs();
  }
  void pregsAll() {
    DBG_Assert(theMicroArch);
    theMicroArch->pregsAll();
  }
  void resynchronize() {
    DBG_Assert(theMicroArch);
    theMicroArch->resynchronize();
  }
  void printRegMappings(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printRegMappings(aRegSet);
  }
  void printRegFreeList(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printRegFreeList(aRegSet);
  }
  void printRegReverseMappings(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printRegReverseMappings(aRegSet);
  }
  void printRegAssignments(std::string aRegSet) {
    DBG_Assert(theMicroArch);
    theMicroArch->printAssignments(aRegSet);
  }

};

class uArch_QemuObject : public Flexus::Qemu::AddInObject <uArch_QemuObject_Impl> {

  typedef Flexus::Qemu::AddInObject<uArch_QemuObject_Impl> base;
public:
  static const Flexus::Qemu::Persistence  class_persistence = Flexus::Qemu::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "uArchARM";
  }
  static std::string classDescription() {
    return "uArchARM object";
  }

  uArch_QemuObject() : base() { }
  uArch_QemuObject(Flexus::Qemu::API::conf_object_t * aQemuObject) : base(aQemuObject) {}
  uArch_QemuObject(uArch_QemuObject_Impl * anImpl) : base(anImpl) {}
};

Qemu::Factory<uArch_QemuObject> theuArchQemuFactory;

//RMC STUFF - BEGIN
static uint32_t RMCcount = 0, DONEcount = 0;
//RMC STUFF - END

//STATS
uint64_t WQstartTS[MAX_CORE_COUNT];		//one per core
uint64_t TotalWQProcessing[MACHINE_COUNT];
uint64_t TotalWQTransferWait[MACHINE_COUNT];
uint32_t WQCount[MACHINE_COUNT], WQTransferCount[MACHINE_COUNT];

uint64_t CQstartTS[MAX_CORE_COUNT];		//one per RMC
uint64_t TotalCQTransfer[MACHINE_COUNT];		//time for RMC to write CQ entry / time to transfer CQ msg from frontend to backend
uint64_t TotalCQProcessing[MACHINE_COUNT];		//time between RMC CQ write and app notifying reception
uint32_t CQCount[MACHINE_COUNT], CQTransferCount[MACHINE_COUNT];
uint64_t CQCountPC[MAX_CORE_COUNT]; //per core

uint64_t TotalRRPLatency[MACHINE_COUNT]; //time between a context read scheduling and its completion
uint64_t RRPIssueLatency[MACHINE_COUNT]; //time between a context read scheduling and its issuing to memory
uint32_t RRPBlockCount[MACHINE_COUNT];

int64_t E2EstartTS[MAX_CORE_COUNT];		//one per core
uint64_t TotalE2ELat[MACHINE_COUNT];
uint32_t transCount[MACHINE_COUNT], minE2Elat[MACHINE_COUNT], maxE2Elat[MACHINE_COUNT];

//for messaging with shared CQ
typedef struct sharedCQinfoEntry {
    QP_table_entry_t theQPEntry;
    std::tr1::unordered_map<uint64_t, uint64_t> translations;
} sharedCQinfoEntry_t;
sharedCQinfoEntry_t sharedCQinfo[MAX_CORE_COUNT];
 
//for bandwidth
uint64_t RCPDataBlocksHandled[MACHINE_COUNT], RCPDataBlocksHandledInterval[MACHINE_COUNT];
uint64_t RRPPDataBlocksHandled[MACHINE_COUNT], RRPPDataBlocksHandledInterval[MACHINE_COUNT];
uint64_t RRPPDataBlocksOverhead[MACHINE_COUNT];  //to keep track of the additional memory accesses required for SABRes, which should not be accounted towards the effective useful BW.

//for messaging uBenchmark
uint64_t NETPIPE_START_COUNT = 0;
uint64_t NETPIPE_COMPLETE_COUNT = 0;
uint64_t RECV_COMPLETE_COUNT = 0;
uint64_t SENDS_RECEIVED = 0;

//stats for messaging
#define MESSAGING_STATS
#ifdef MESSAGING_STATS
uint64_t REQUESTS_COMPLETED[MAX_CORE_COUNT];

/*Messaging time instrumentation:
 * start_time: cycle number when last packet of request is received
 * queuing_time: time between reception of whole request until dispatched to a free core from the RMC
 * dispatch_time: time between dispatch from RMC and reception from target core's RCP
 * notification_time: time between RCP receives new SEND req and breakpoint 101 (i.e., when core starts processing the req)
 * exec_time_synth: time between breakpoints 101 and 102, during which core spins to emulate synthetic service time
 * exec_time_extra: time between breakpoint 102 and reception of RECV request from RMC, which also marks the core as free to receive new SEND req
 */
typedef struct request_time_entry{
    uint32_t core_id, queueing_time, dispatch_time, notification_time, exec_time_synth, exec_time_extra, start_time;
    uint64_t uniqueID;
    uint32_t msg_size, packet_counter;
} request_time_entry_t;
std::list<uint64_t> REQUEST_TIMESTAMP[MAX_CORE_COUNT];   //this holds the mapping of core id to entry key for the RequestTimer map
std::list<uint64_t> sharedCQUniqueIDlist;
std::tr1::unordered_map<uint64_t, request_time_entry_t> RequestTimer;
std::tr1::unordered_map<uint64_t, request_time_entry_t>::iterator RequestTimerIter;
std::ofstream MessagingLatFile, coreOccupFile, MsgStatsCsv;  //A file to dump the latency breakdown of each messaging req and core occupancies
uint32_t IDLE_TIME[MAX_CORE_COUNT], BUSY_TIME[MAX_CORE_COUNT], CORE_TIMESTAMP[MAX_CORE_COUNT];  //a timestamp per core, used to keep track of how long a core has been idle and occupied 
#endif
//STATS END
 
class FLEXUS_COMPONENT(uArchARM) {
    FLEXUS_COMPONENT_IMPL(uArchARM);

    std::shared_ptr<microArch> theMicroArch;
    uArch_QemuObject theuArchObject;
    //RMC STUFF - BEGIN

    //For STATISTICS - begin
    //std::tr1::unordered_map<uint8_t, uint64_t> cycleCounter;	//cycles from enqueuing WQRead in ReqQueue to CQWrite update
    std::tr1::unordered_map<uint32_t, uint64_t> RTcycleCounter;	//cycles for roundtrip: app writing WQ entry, till app reads corresponding CQ entry
    std::tr1::unordered_map<uint32_t, uint64_t>::iterator cycleCounterIter;
    uint64_t theCycleCount, transactionCounter, latencyAvg, latSum;
    uint64_t RTtransactionCounter, RTlatencyAvg, RTminLat, RTmaxLat, RTlatSum, RTmaxLatNoExtremes;

    uint64_t perRRPPLatency, perRRPPIssueLatency, perRRPPblockCount;

    std::tr1::unordered_map<uint64_t, uint64_t> RRPPtimers;
    uint64_t RRPPLatSum, RRPPLatCounter;

    uint64_t waiting4WQentry, waiting4WQentryAvg, waiting4WQentryCounter, waiting4WQentrySum;
    uint32_t print_freq;	//cycles

    uint64_t RGPMemByteCount, RCPMemByteCount, RRPPMemByteCount, RGPNWByteCount, RCPNWByteCount, RRPPincNWByteCount, RRPPoutNWByteCount;
    uint64_t SABResSent;        //per RGP (backend)

    //#ifdef FLOW_CONTROL_STATS
    uint64_t requestsPushed;  //to the memory hierarchy
    uint64_t networkDataPacketsPushed;
    //#endif
    //For statistics - end

    bool isRMC;		//to distinguish between cores and potential RMCs
    bool LoadFromSimics; // to determine whether we restore from a simics or flexus trace checkpoint
    bool fakeRMC;	//to distinguish between real RMCs and placeholder RMCs
    bool isRRPP, isRGP;

    uint32_t MACHINE_ID;        //Every component should mark the id of the machine to which it belongs upon initialization (for multinode)
    std::tr1::unordered_map<uint64_t,uint16_t> ctxToBEs; //for every ctx, keeps track of which BE is servicing at least one QP of that context, needed for messaging -- value used as a bitset: up to 16 RRPPs per node
    std::tr1::unordered_map<uint64_t,uint16_t>::iterator ctxToBEsIter;

    //%%%%%%%% FOR SPLIT-RMC
    bool hasFrontend, hasBackend;	//these are only relevant if cfg.SplitRGP is set
    std::list<MemoryTransport> RGPinQueue, RCPinQueue;	//for RGP and RCP backends, respectively
    std::list<MemoryTransport> CQWaitQueue[MAX_CORE_COUNT];	//for received RRPP send requests, buffer before load balancing to cores
    uint32_t outstandingFETransfers;	//to track outstanding transfers and cap them - only for Split RMC. Limit per RGP Frontend
    uint32_t outstandingPackets;    //to track outstanding packets per RGP BE and cap them
    uint32_t prefetchTokens;   //keeps track of outstanding SABRe prefetches
    //A SABRe prefetch is issued early for the first block of a SABRe,
    //to ameliorate the latency overhead of the first synchronous read
    bool streamBufferAvailable;  //for RRPP Stream Buffer optimization
    std::tr1::unordered_map<uint32_t, uint32_t> bufferSizes;	//only needed for flexi apps and splitRMC (for RCP frontend)
    //%%%%%%%%

    //&&&&&&&&
    Flexus::SharedTypes::MemoryTransport	theMagicTransport;		//for magic interface of RMCs with cores' caches
    boost::scoped_ptr< nMessageQueues::DelayFifo< MemoryTransport > > delayQueue;
    std::list<MemoryTransport> prefetches, inFlightPrefetches;
    //&&&&&&&&
    //
    /* Msutherl: Data structures for local-RMC dispatch */
    std::vector< size_t > local_rmc_indices;

    uint32_t RMCid, loadRMCid;													//This RMC's id
    RMCStatus status;
    uint64_t wqSize, cqSize;		//# of entries
    uint32_t TLBsize;
    uint8_t uniqueTID, WQHTreceived;
    std::vector < std::pair < uint64_t, uint64_t > > RMC_TLB;		//for LRU order
    API::conf_object_t * theCPU;									//the CPU this RMC is coupled with

    std::tr1::unordered_map<uint64_t, RMCPacket>::iterator packetStoreIter;
    std::tr1::unordered_map<uint64_t, aBlock>::iterator dataStoreIter;
    RMCPacket packet;

    uint64_t iMask, jMask, kMask, PObits, PTbaseV[MAX_CORE_COUNT];					//for Page Tables - PTbaseV is the Vaddress of the base of the PT
    std::tr1::unordered_map<uint64_t, uint64_t> PTv2p[MAX_CORE_COUNT];				//translations for PT pages
    std::tr1::unordered_map<uint64_t, uint64_t> bufferv2p[MAX_CORE_COUNT];
    std::tr1::unordered_map<uint64_t, uint64_t> contextv2p[MAX_CORE_COUNT];
    std::tr1::unordered_map<uint64_t, uint64_t> sendbufv2p[MAX_CORE_COUNT];	//translations for send buf
    std::tr1::unordered_map<uint64_t, uint64_t> recvbufv2p[MAX_CORE_COUNT];	//translations for recv buf
    std::tr1::unordered_map<uint16_t, uint16_t> lastCoreUsed;   //this hashmap is used by every RRPP and keeps track of the last core that was used to dispatch a request
    std::tr1::unordered_map<uint64_t, std::pair <uint64_t, uint64_t> > contextMap[MAX_CORE_COUNT];			//context IDs to context start V address - one map per machine
    //pair: first is base address, second is context size
    std::tr1::unordered_map<uint64_t, aBlock> tempDataStore;
    std::tr1::unordered_map<uint64_t, uint64_t>::iterator tempAddr;
    std::list<MAQEntry> afterPWEntries;
    std::list<MAQEntry> MAQ, prefetchMAQ;
    std::list<MAQEntry>::iterator MAQiter, prev;
    MAQEntry aMAQEntry;
    bool pendingPagewalk;

    //RMC dedicated SRAMs
    std::tr1::unordered_map<uint16_t, ITT_table_entry_t> ITT_table;
    std::tr1::unordered_map<uint16_t, ITT_table_entry_t>::iterator ITT_table_iter;
    std::tr1::unordered_map<uint16_t, QP_table_entry_t> QP_table;
    std::tr1::unordered_map<uint16_t, QP_table_entry_t>::iterator QP_table_iter;
    std::tr1::unordered_map<uint16_t, messaging_struct_t> messaging_table;
    std::tr1::unordered_map<uint16_t, messaging_struct_t>::iterator messaging_table_iter;
    std::list<uint16_t> tidFreelist, QPidFreelist, tidReadyList;
    std::list<uint16_t>::iterator QPidFreelistIter;

    ATT_entry_t ATT[ATT_SIZE];		//size limited by ATT_SIZE in Common/RMC_SRAMS.hpp
    std::list<uint16_t> ATTfreelist, ATTpriolist;
    std::tr1::unordered_map<uint64_t, uint16_t> ATTtags;	//For source unrolling; need to snoop key of ATT entries to match unrolled SABRe requests to their corresponding entry
    std::list<uint16_t>::iterator ATTlistIter;

#ifdef RRPP_FAKE_DATA_ACCESS
    boost::scoped_ptr< nMessageQueues::DelayFifo< RMCPacket > > memoryLoopback;
#endif

    //pipelines
    pStage RGPip[7], RCPip[4], RRPPip[6];
    pStage RGPipFE, RCPipFE;		//special pStages for the end of the Frontend pipelines. For the Split-RMC case.
    RGData1_t RGData1; RGData2_t RGData2; RGData3_t RGData3, RGData3a; RGData4_t RGData4; RGData5_t RGData5; RGData6_t RGData6;
    RCData0_t RCData0; RCData1_t RCData1; RCData2_t RCData2; RCData3_t RCData3;
    RRPData0_t RRPData0, RRPData5; RRPData2_t RRPData2; RRPData3_t RRPData3;
    std::list<RMCPacket> RRPData6;
    //RMC STUFF - END

    public:
    FLEXUS_COMPONENT_CONSTRUCTOR(uArchARM)
        : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

    bool isQuiesced() const {
        return !theMicroArch || theMicroArch->isQuiesced();
    }

    void initStats() {

        theCycleCount = 0;
        transactionCounter = 0;
        latencyAvg = 0;
        latSum = 0;
        RTtransactionCounter = 0;
        RTlatencyAvg = 0;
        RTmaxLat = 0;
        RTmaxLatNoExtremes = 0;
        RTminLat = 999999999;
        RTlatSum = 0;

        waiting4WQentry = 0;
        waiting4WQentryAvg = 0;
        waiting4WQentryCounter = 0;
        waiting4WQentrySum = 0;

        RRPPLatSum = 0;
        RRPPLatCounter = 0;

        RGPMemByteCount = 0;
        RCPMemByteCount = 0;
        RRPPMemByteCount = 0;
        RGPNWByteCount = 0;
        RCPNWByteCount = 0;
        RRPPincNWByteCount = 0;
        RRPPoutNWByteCount = 0;

        perRRPPLatency = perRRPPIssueLatency = perRRPPblockCount = 0;

        print_freq = STATS_PRINT_FREQ;

        SABResSent = RMCid;

        int i;
        for (i=0; i<MACHINE_COUNT; i++  ) {
            TotalE2ELat[i] = 0;
            minE2Elat[i] = 9999999;
            maxE2Elat[i] = 0;
        }
        //#ifdef FLOW_CONTROL_STATS
        requestsPushed = 0;
        networkDataPacketsPushed = 0;
        //#endif
    }


    void initialize() {
        uArchOptions_t options;
        std::srand(0);

        options.ROBSize              = cfg.ROBSize;
        options.SBSize               = cfg.SBSize;
        options.NAWBypassSB          = cfg.NAWBypassSB;
        options.NAWWaitAtSync        = cfg.NAWWaitAtSync;
        options.retireWidth          = cfg.RetireWidth;
        options.numMemoryPorts       = cfg.MemoryPorts;
        options.numSnoopPorts        = cfg.SnoopPorts;
        options.numStorePrefetches   = cfg.StorePrefetches;
        options.prefetchEarly        = cfg.PrefetchEarly;
        options.spinControlEnabled   = cfg.SpinControl;
        options.consistencyModel     = (nuArchARM::eConsistencyModel)cfg.ConsistencyModel;
        options.coherenceUnit        = cfg.CoherenceUnit;
        options.breakOnResynchronize = cfg.BreakOnResynchronize;
        //    options.validateMMU          = cfg.ValidateMMU;
        options.speculativeOrder     = cfg.SpeculativeOrder;
        options.speculateOnAtomicValue   = cfg.SpeculateOnAtomicValue;
        options.speculateOnAtomicValuePerfect   = cfg.SpeculateOnAtomicValuePerfect;
        options.speculativeCheckpoints = cfg.SpeculativeCheckpoints;
        options.checkpointThreshold   = cfg.CheckpointThreshold;
        options.earlySGP             = cfg.EarlySGP;    /* CMU-ONLY */
        options.trackParallelAccesses = cfg.TrackParallelAccesses; /* CMU-ONLY */
        options.inOrderMemory        = cfg.InOrderMemory;
        options.inOrderExecute       = cfg.InOrderExecute;
        options.onChipLatency        = cfg.OnChipLatency;
        options.offChipLatency       = cfg.OffChipLatency;
        options.name                 = statName();
        options.node                 = flexusIndex();

        options.numIntAlu = cfg.NumIntAlu;
        options.intAluOpLatency = cfg.IntAluOpLatency;
        options.intAluOpPipelineResetTime = cfg.IntAluOpPipelineResetTime;

        options.numIntMult = cfg.NumIntMult;
        options.intMultOpLatency = cfg.IntMultOpLatency;
        options.intMultOpPipelineResetTime = cfg.IntMultOpPipelineResetTime;
        options.intDivOpLatency = cfg.IntDivOpLatency;
        options.intDivOpPipelineResetTime = cfg.IntDivOpPipelineResetTime;

        options.numFpAlu = cfg.NumFpAlu;
        options.fpAddOpLatency = cfg.FpAddOpLatency;
        options.fpAddOpPipelineResetTime = cfg.FpAddOpPipelineResetTime;
        options.fpCmpOpLatency = cfg.FpCmpOpLatency;
        options.fpCmpOpPipelineResetTime = cfg.FpCmpOpPipelineResetTime;
        options.fpCvtOpLatency = cfg.FpCvtOpLatency;
        options.fpCvtOpPipelineResetTime = cfg.FpCvtOpPipelineResetTime;

        options.numFpMult = cfg.NumFpMult;
        options.fpMultOpLatency = cfg.FpMultOpLatency;
        options.fpMultOpPipelineResetTime = cfg.FpMultOpPipelineResetTime;
        options.fpDivOpLatency = cfg.FpDivOpLatency;
        options.fpDivOpPipelineResetTime = cfg.FpDivOpPipelineResetTime;
        options.fpSqrtOpLatency = cfg.FpSqrtOpLatency;
        options.fpSqrtOpPipelineResetTime = cfg.FpSqrtOpPipelineResetTime;

        //ALEX - begin
        options.isRMC = cfg.IsRMC;
        isRMC = cfg.IsRMC;
        LoadFromSimics = cfg.LoadFromSimics;
        fakeRMC = true;
        isRGP = false;
        isRRPP = false;
        hasFrontend = false;
        hasBackend = false;
        outstandingFETransfers = 0;
        outstandingPackets = 0;
        prefetchTokens = PREFETCH_TOKENS;
        if (cfg.RRPPPerTile) DBG_Assert(false, ( << "One RRPP per tile not supported yet"));
        delayQueue.reset( new nMessageQueues::DelayFifo< MemoryTransport >(100));
        DBG_Assert(cfg.PerCoreReqQueue > 0);
        //ALEX - end

        theMicroArch = microArch::construct ( options
                , ll::bind( &uArchARMComponent::squash, this, ll::_1)
                , ll::bind( &uArchARMComponent::redirect, this, ll::_1)
                , ll::bind( &uArchARMComponent::changeState, this, ll::_1, ll::_2)
                , ll::bind( &uArchARMComponent::feedback, this, ll::_1)
                , ll::bind( &uArchARMComponent::signalStoreForwardingHit, this, ll::_1)
                , ll::bind( &uArchARMComponent::resyncMMU, this, ll::_1)
                );

        if (!isRMC) { //ALEX
            theuArchObject = theuArchQemuFactory.create( (std::string("uarcharm-") + boost::padded_string_cast < 2, '0' > (flexusIndex())).c_str() );
            theuArchObject->setMicroArch(theMicroArch);
        }

        //RMC STUFF - BEGIN
        Qemu::Processor cpu = Qemu::Processor::getProcessor(flexusIndex());
        DBG_( Dev, Comp(*this) ( << "flexusIndex: " << flexusIndex() ) );

        if (isRMC) {
#ifdef SABRE_IS_RREAD
            DBG_( Dev, Comp(*this) ( << "WARNING: SABRE_IS_RREAD is set. This means all SABRe requests will automatically be switched to normal RREADS by the RMCs." ) );
#endif
#ifdef RREAD_IS_SABRE
            DBG_( Dev, Comp(*this) ( << "WARNING: RREAD_IS_SABRE is set. This means all RREAD requests will automatically be switched to SABRes by the RMCs." ) );
#endif
#ifdef SYNC_OPERATION
            DBG_( Dev, Comp(*this) ( << "WARNING: SYNC_OPERATION is set. This means that RMCs will always limit the per-WQ requests that are in flight to one, regardless of the entries enqueued by the app." ) );
#endif

            CORES = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
            MACHINES = cfg.MachineCount;
            DBG_Assert(cfg.MachineCount <= MACHINE_COUNT, ( << "Simulation started for " << cfg.MachineCount << " machines, and MACHINE_COUNT is set to " << MACHINE_COUNT
                        << ". Please increase the MACHINE_COUNT define so that MACHINE_COUNT > cfg.MachineCount."));
            CORES_PER_RMC = (uint32_t)floor(sqrt(CORES / MACHINES));
            CORES_PER_MACHINE = CORES / MACHINES;
            MACHINE_ID = flexusIndex() / CORES_PER_MACHINE;
            DBG_Assert(CORES % MACHINES == 0);
            DBG_Assert(CORES_PER_RMC*CORES_PER_RMC*MACHINES == CORES);
            DBG_( Dev, ( << "CORES_PER_MACHINE = " << CORES_PER_MACHINE << ", CORES_PER_RMC = " << CORES_PER_RMC << ", MACHINE_ID = " << MACHINE_ID ) );

            /* Msutherl: Make dispatch list:
             * - RMC ids should go from [MACHINE_ID,MACHINE_ID+CORES_PER_MACHINE-1] 
             *   with inclusive endpoints. e.g., 4 cores, 2 machines:
             *   machine 0: (0,3)
             *   machine 1: (4-7) 
             *   */
            size_t init_index = MACHINE_ID * CORES_PER_MACHINE;
            for(size_t i = init_index; i < (init_index + CORES_PER_MACHINE); i++) {
                local_rmc_indices.push_back(i);
            }

            initStats();

            RMCid = RMCcount++;

            //Will now distinguish between real and fake RMCs
            if (cfg.SplitRGP) {
                loadRMCid = RMCid;
                isRGP = true;
                hasFrontend = true;
                DBG_(Dev, ( << " This RMC has an RGP frontend and an RCP backend."));
            } else if (cfg.RGPPerTile) {
                loadRMCid = RMCid;
                isRGP = true;
            } else {	//RGPs across edge
                if (RMCid%CORES_PER_RMC == 0) {
                    //loadRMCid = RMCid/CORES_PER_RMC + CORES;
                    loadRMCid = RMCid + CORES;
                    isRGP = true;
                } else if (cfg.RRPPWithMCs && RMCid%CORES_PER_RMC == CORES_PER_RMC-1) {
                    loadRMCid = RMCid/CORES_PER_RMC + CORES + CORES_PER_RMC - 1;	//WARNING! Assuming MCs are located on the rightmost column
                } else {
                    DBG_( Dev, Comp(*this) ( << "NOTE: This RMC is fake." ) );
                    return;
                }
            }
            //Now also mark which of the RMCs have an RRPP
            if (cfg.RRPPWithMCs) {
                if (RMCid%CORES_PER_RMC == CORES_PER_RMC-1) {
                    isRRPP = true;
                    if (cfg.SplitRGP) {
                        hasBackend = true;		//collocate RGP backend with RRPPs
                        DBG_(Dev, ( << " This RMC has an RGP backend and an RCP frontend."));
                    }
                }
            } else {
                if (RMCid%CORES_PER_RMC == 0) {
                    isRRPP = true;
                    if (cfg.SplitRGP) {
                        hasBackend = true;		//collocate RGP backend with RRPPs
                        DBG_(Dev, ( << " This RMC has an RGP backend and an RCP frontend."));
                    }
                }
            }
            fakeRMC = false;

            WQHTreceived = 0;
            if (!cfg.SplitRGP) {
                DBG_( Dev, Comp(*this) ( << "This uArch is RMC-" << RMCid << " and will service " << CORES_PER_RMC	<< " cores" ) );
            }
            if (NO_PAGE_WALKS) DBG_( Crit, Comp(*this) ( << "WARNING: RMC page walks are disabled!" ) );

            if (isRRPP) {
                if (cfg.SingleRMCDispatch) {
                    DBG_Assert(!cfg.DataplaneEmulation);
                    lastCoreUsed[RMCid] = local_rmc_indices.front();
                    DBG_( Dev, ( << "Incoming SEND dispatch will be balanced centrally from RRPP-" 
                                << getRMCDispatcherID_LocalMachine()<< " to all " << CORES_PER_MACHINE << " available cores."
                                << "First core for dispatch: " << lastCoreUsed[RMCid] ));
                } else if (cfg.DataplaneEmulation) {
                    DBG_( Dev, ( << "Implementing partitioned dataplanes for all incoming SEND requests." ));
                } else {
                    DBG_( Dev, ( << "Incoming SEND  will be dispatched from this RRPP to all " 
                                << CORES_PER_RMC << " cores on the same row (cores " 
                                << RMCid << " to " << RMCid + CORES_PER_RMC << ")"));
                    lastCoreUsed[RMCid] = RMCid; // FIXME: msutherl: i don't think dataplanes wwill work in multinode right now.
                }
            }
            TLBsize = cfg.RMCTLBEntries;

            FLEXI_APP_REQ_SIZE = cfg.FlexiAppReqSize;
            ENABLE_SABRE_PREFETCH = cfg.RRPPPrefetchSABRe;
            RGP_PREFETCH_SABRE = cfg.RGPPrefetchSABRe;
            SABRE_PREFETCH_LIMIT = cfg.PrefetchBlockLimit;

            pendingPagewalk = false;

            packet.pl_size = 64;

            //for Page Tables

            uint64_t i, theBase = 0;
            PObits = 64 - PT_I - PT_J - PT_K;

            for (i=0; i<PT_I; i++) {
                theBase = theBase << 1;
                theBase = theBase | 1;
            }
            iMask = theBase << (PObits+PT_J+PT_K);

            theBase = 0;
            for (i=0; i<PT_J; i++) {
                theBase = theBase << 1;
                theBase = theBase | 1;
            }
            jMask = theBase << (PObits+PT_K);

            theBase = 0;
            for (i=0; i<PT_K; i++) {
                theBase = theBase << 1;
                theBase = theBase | 1;
            }
            kMask = theBase << PObits;

            //Pipelines
            for (i=0; i<7; i++) {
                RGPip[i].inputFree = true;
            }
            for (i=0; i<4; i++) {
                RCPip[i].inputFree = true;
            }
            for (i=0; i<6; i++) {
                RRPPip[i].inputFree = true;
            }
            RGPipFE.inputFree = true;
            RCPipFE.inputFree = true;

            for (i=1; i< TID_RANGE; i++) {
                tidFreelist.push_back(i);
            }

            for (i=0; i<ATT_SIZE; i++) {
                ATTfreelist.push_back(i);
            }

            if (!RMCid) {
                //Determine how to distribute SABRes to remote RRPPs
                if (strcasecmp(cfg.SABReToRRPPMatching.c_str(), "random") == 0) {
                    SABReToRRPPDistr = eRandom;
                    DBG_(Dev, ( << "SABRes will be matched to random remote RRPP."));
                } else if (strcasecmp(cfg.SABReToRRPPMatching.c_str(), "matching") == 0) {
                    SABReToRRPPDistr = eMatching;
                    DBG_(Dev, ( << "SABRes generated by RGP backend X will be mapped to remote RRPP X."));
                } else {
                    DBG_Assert(false, ( << "Unknown SABRe distribution parameter '" << cfg.SABReToRRPPMatching
                                << "'. Accepted options: random, matching." ));
                }

                DBG_(Dev, ( << "Opening file sabre_latencies.txt to dump SABRe statistics."));
                SABReLatFile.open("sabre_latencies.txt");
                SABReLatFile << "#All latencies in cycles\nRMCid | Version read | Data read | Version validation\n==============================\n";
#ifdef MESSAGING_STATS
                MsgStatsCsv.open("msg_lat.csv");
                MessagingLatFile.open("messaging_latencies.txt");
                MessagingLatFile << "#All latencies in cycles\n#start cycle | finish cycle | core_id | queuing time | RRPP->RCP | RCP-> core | exec time (synthetic) | exec time (extra) | total \"busy\" core time | end-to-end time\n#=============================================================================================================================\n";
                MsgStatsCsv << "#start cycle;finish cycle;core_id;queuing time;RRPP->RCP;RCP-> core;exec time (synthetic);exec time (extra);total \"busy\" core time;end-to-end time;\n";
                coreOccupFile.open("core_occupancies.txt");
                coreOccupFile << "# timestamp | completed req cnt | core_id | cycles_idle | cycles_busy\n#=========================================\n";
#endif
            }
        }

#ifdef RRPP_FAKE_DATA_ACCESS
        if (isRRPP) memoryLoopback.reset( new nMessageQueues::DelayFifo< RMCPacket >(100));
#endif
        return;
        //RMC STUFF - END
    }

    void finalize() {}

    public:
    FLEXUS_PORT_ALWAYS_AVAILABLE(DispatchIn);
    void push( interface::DispatchIn const &, boost::intrusive_ptr< AbstractInstruction > & anInstruction ) {
        DBG_(VVerb, (<<"Get the inst in uArchARM: "));
        DBG_Assert( !isRMC, ( << "RMC's DispatchIn port was invoked. Should do something?") );
        theMicroArch->dispatch(anInstruction);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableDispatchOut);
    std::pair<int, bool> pull(AvailableDispatchOut const &) {
        DBG_Assert( !isRMC, ( << "RMC's AvailableDispatchOut port was invoked. Should do something?") );
        return std::make_pair( theMicroArch->availableROB(), theMicroArch->isSynchronized() );
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const &) {
        DBG_Assert( !isRMC, ( << "WARNING: RMC's AvailableDispatchOut port was invoked. Should do something?") );
        return theMicroArch->isStalled();
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
    int32_t pull(ICount const &) {
        DBG_Assert( !isRMC,  ( << "WARNING: RMC's ICount port was invoked. Should do something?") );
        return theMicroArch->iCount();
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryIn);
    void push( interface::MemoryIn const &, MemoryTransport & aTransport) {
        if (aTransport[MemoryMessageTag]->type() == MemoryMessage::WQMessage) {
            DBG_Assert(cfg.SplitRGP && hasBackend);
            DBG_(SPLIT_RGP_DEBUG, ( << "RMC-" << RMCid << " backend got a WQMessage in MemoryIn. Pushing in RGPinQueue."));
            //DBG_(Tmp, ( << "RMC-" << RMCid << " backend got a WQMessage in MemoryIn. Pushing in RGPinQueue. " << *aTransport[MemoryMessageTag]));
            RGPinQueue.push_back(aTransport);
            return;
        } else if (aTransport[MemoryMessageTag]->type() == MemoryMessage::CQMessage) {
            DBG_Assert(cfg.SplitRGP && hasFrontend);
            DBG_(SPLIT_RGP_DEBUG, ( << "RMC-" << RMCid << " Got a CQMessage in MemoryIn. Pushing in RCPinQueue."));
            if (aTransport[MemoryMessageTag]->op == SEND_TO_SHARED_CQ) {
                DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << " Got a CQMessage with a SEND_TO_SHARED_CQ operation."));
            }
            RCPinQueue.push_back(aTransport);
            return;
        } else if (aTransport[MemoryMessageTag]->type() == MemoryMessage::RRPPFwd) {
            DBG_Assert(isRRPP);
            DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << " Got an RRPPFwd in MemoryIn. Pushing in CQWaitQueue[0]"));
            CQWaitQueue[0].push_back(aTransport);
            DBG_Assert(!cfg.DataplaneEmulation);
            return;
        }
        //&&&&&&
        if (aTransport[MemoryMessageTag]->type() == MemoryMessage::StorePrefetchReply) {
            std::list<MemoryTransport>::iterator pIter = inFlightPrefetches.begin();

            while (pIter != inFlightPrefetches.end()) {
                MemoryTransport aTrans(*pIter);
                if (aTrans[MemoryMessageTag]->address() == aTransport[MemoryMessageTag]->address()) {
                    DBG_(MAGIC_CONNECTIONS_DEBUG, Comp(*this) ( << " Prefetch completed") );
                    inFlightPrefetches.erase(pIter);
                    return;
                }
                pIter++;
            }
        }
        //&&&&&&
        DBG_( VVerb, ( << "uArch received a reply msg: " << aTransport[MemoryMessageTag]) );
        handleMemoryMessage(aTransport);

        if (isRMC) RMC_MemoryIn(aTransport);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE( WritePermissionLost );
    void push( interface::WritePermissionLost const &, PhysicalMemoryAddress & anAddress) {
        DBG_Assert( !isRMC,  ( << "WARNING: RMC's WritePermissionLost port was invoked. Should do something?") );
        theMicroArch->writePermissionLost(anAddress);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(dTranslationIn);
    void push( interface::dTranslationIn const &,
            TranslationPtr& aTranslate ) {

        PhysicalMemoryAddress magicTranslation = Flexus::Qemu::Processor::getProcessor(theMicroArch->core())->translateVirtualAddress(aTranslate->theVaddr);

        if( aTranslate->thePaddr == magicTranslation ) {
            DBG_(Dev, ( << "Magic QEMU translation == MMU Translation. Vaddr = "
                        << std::hex << aTranslate->theVaddr
                        << std::dec << ", Paddr = "
                        << std::hex << aTranslate->thePaddr << std::dec));
        } else {
            DBG_Assert(false, ( << "ERROR: Magic QEMU translation NOT EQUAL TO MMU Translation. Vaddr = " << std::hex << aTranslate->theVaddr
                        << std::dec << ", PADDR_MMU = "
                        << std::hex << aTranslate->thePaddr
                        << std::dec << ", PADDR_QEMU = "
                        << std::hex << magicTranslation << std::dec));
        }


        theMicroArch->pushTranslation(aTranslate);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryRequestIn);
    void push( interface::MemoryRequestIn const &,
            TranslationPtr& aTranslation ) {

        theMicroArch->issueMMU(aTranslation);
    }

    public:
    //The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
    //passing each instruction to its FetchOut port.
    void drive(interface::uArchDrive const &) {
        doCycle();
    }

    private:
    struct ResynchronizeWithQemuException {};
    uint64_t printDebugCounter;

    size_t getRMCDispatcherID_LocalMachine() {
        return ((MACHINE_ID * CORES_PER_MACHINE) + cfg.RMCDispatcherID);
    }

    void squash(eSquashCause aSquashReason) {
        FLEXUS_CHANNEL( SquashOut ) << aSquashReason;
    }
    void resyncMMU(int32_t aNode) {
        bool value = true;
        FLEXUS_CHANNEL(ResyncOut) << value;
    }

    void changeState(int32_t aTL, int32_t aPSTATE) {
        CPUState state;
        state.theTL = aTL;
        state.thePSTATE = aPSTATE;
        FLEXUS_CHANNEL( ChangeCPUState ) << state;
    }

    void redirect(VirtualMemoryAddress aPC) {
        VirtualMemoryAddress redirect_addr = aPC;
        FLEXUS_CHANNEL( RedirectOut ) << redirect_addr;
    }

    void feedback(boost::intrusive_ptr<BranchFeedback> aFeedback) {
        FLEXUS_CHANNEL( BranchFeedbackOut ) << aFeedback;
    }

    void signalStoreForwardingHit(bool garbage) {
        bool value = true;
        FLEXUS_CHANNEL( StoreForwardingHitSeen ) << value;
    }

    void doCycle() {
        if (!theCycleCount && !RMCid && !messaging_table.empty()) {
            messaging_table.begin()->second.sharedCQ_size = 0;
            if (!sharedCQinfo[MACHINE_ID].translations.empty()) messaging_table.begin()->second.sharedCQ_size = MAX_NUM_SHARED_CQ;
            FLEXUS_CHANNEL(MessagingInfo) << messaging_table.begin()->second;
        }
        theCycleCount++;

        if (cfg.Multithread) {
            if (nMTManager::MTManager::get()->runThisEX(flexusIndex())) {
                theMicroArch->cycle();
            } else {
                theMicroArch->skipCycle();
            }
        } else {
            theMicroArch->cycle();
        }
        sendMemoryMessages();
        requestTranslations();
        //RMC STUFF - BEGIN
        if (!fakeRMC) {
            DBG_( VVerb, Comp(*this) ( << "Calling RMC_drive() for RMC-" << RMCid ) );
            if (theCycleCount % RMC_FREQ_DIVIDER == 0) {
                RMC_drive();
            }
            if (print_freq) print_freq--;
            if (!print_freq) {
                printStats();
                print_freq = STATS_PRINT_FREQ;
            }
        }
        //RMC STUFF - END
    }

    void requestTranslations() {
        while (FLEXUS_CHANNEL( dTranslationOut ).available()) {
            TranslationPtr op(theMicroArch->popTranslation());
            if (! op ) break;
            FLEXUS_CHANNEL(dTranslationOut) << op;
        }
    }

    void sendMemoryMessages() {
        while (FLEXUS_CHANNEL( MemoryOut_Request ).available()) {
            boost::intrusive_ptr< MemOp > op(theMicroArch->popMemOp());
            if (! op ) break;

            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;

            DBG_(Dev, (<< "Sending Memory Request: " <<  op->theOperation  << "  -- vaddr: " << op->theVAddr
                        << "  -- paddr: " << op->thePAddr << "  --  Instruction: " <<  op->theInstruction
                        << " --  PC: " << op->thePC << " -- size: " << op->theSize));

            if (op->theNAW) {
                DBG_Assert( op->theOperation == kStore );
                operation = new MemoryMessage(MemoryMessage::NonAllocatingStoreReq, op->thePAddr, op->thePC);
            } else {

                switch ( op->theOperation ) {
                    case kLoad:
                        //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newLoad(op->thePAddr, op->thePC);
                        break;

                    case kAtomicPreload:
                        //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newAtomicPreload(op->thePAddr, op->thePC);
                        break;

                    case kStorePrefetch:
                        //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newStorePrefetch(op->thePAddr, op->thePC, op->theValue);
                        break;

                    case kStore:
                        //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newStore(op->thePAddr, op->thePC, op->theValue);
                        break;

                    case kRMW:
                        //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newRMW(op->thePAddr, op->thePC, op->theValue);
                        break;

                    case kCAS:
                        //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newCAS(op->thePAddr, op->thePC, op->theValue);
                        break;
                    case kPageWalkRequest:
                        operation = MemoryMessage::newPWRequest(op->thePAddr);
                        operation->setPageWalk();
                        break;
                    default:
                        DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
                }

            }
            if (op->theOperation != kPageWalkRequest)
                operation->theInstruction = op->theInstruction;

            operation->reqSize() = op->theSize;
            if (op->theTracker) {
                transport.set(TransactionTrackerTag, op->theTracker);
            } else {
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                tracker->setAddress( op->thePAddr );
                tracker->setInitiator(flexusIndex());
                tracker->setSource("uArchARM");
                tracker->setOS(false); //TWENISCH - need to set this properly
                transport.set(TransactionTrackerTag, tracker);
                op->theTracker = tracker;
            }

            transport.set(MemoryMessageTag, operation);
            transport.set(uArchStateTag, op);

            if (op->theNAW && (op->thePAddr & 63) != 0) {
                //Auto-reply to the unaligned parts of NAW
                transport[MemoryMessageTag]->type() = MemoryMessage::NonAllocatingStoreReply;
                handleMemoryMessage( transport );
            } else {
                FLEXUS_CHANNEL( MemoryOut_Request) << transport;
            }
            //&&&&&&&&&&&&&&&
            if (FLEXUS_CHANNEL( MemoryOut_Request ).available()) {
                std::list<MemoryTransport>::iterator pIter = prefetches.begin();
                if (pIter != prefetches.end()) {
                    DBG_(MAGIC_CONNECTIONS_DEBUG, Comp(*this) ( << " Pushing a prefetch to the cache") );
                    MemoryTransport aTrans(*pIter);
                    aTrans[MemoryMessageTag]->type() = MemoryMessage::StorePrefetchReq;
                    inFlightPrefetches.push_back(aTrans);
                    prefetches.erase(pIter);
                    FLEXUS_CHANNEL( MemoryOut_Request) << aTrans;
                }
            }
            //&&&&&&&&&&&&&&
        }

        while (FLEXUS_CHANNEL( MemoryOut_Snoop).available()) {
            boost::intrusive_ptr< MemOp > op(theMicroArch->popSnoopOp());
            if (! op ) break;

            DBG_( Iface, ( << "Send Snoop: " << *op) );

            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;

            PhysicalMemoryAddress pc;

            switch ( op->theOperation ) {
                case kInvAck:
                    DBG_( Verb, ( << "Send InvAck.") );
                    operation = new MemoryMessage(MemoryMessage::InvalidateAck, op->thePAddr);
                    break;

                case kDowngradeAck:
                    DBG_( Verb, ( << "Send DowngradeAck.") );
                    operation = new MemoryMessage(MemoryMessage::DowngradeAck, op->thePAddr);
                    break;

                case kProbeAck:
                    operation = new MemoryMessage(MemoryMessage::ProbedNotPresent, op->thePAddr);
                    break;

                case kReturnReply:
                    operation = new MemoryMessage(MemoryMessage::ReturnReply, op->thePAddr );
                    break;

                default:
                    DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
            }

            operation->reqSize() = op->theSize;
            if (op->theTracker) {
                transport.set(TransactionTrackerTag, op->theTracker);
            } else {
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                tracker->setAddress( op->thePAddr );
                tracker->setInitiator(flexusIndex());
                tracker->setSource("uArchARM");
                transport.set(TransactionTrackerTag, tracker);
            }

            transport.set(MemoryMessageTag, operation);

            FLEXUS_CHANNEL( MemoryOut_Snoop) << transport;
        }

    }

    void handleMemoryMessage( MemoryTransport & aTransport) {
        boost::intrusive_ptr< MemOp > op;
        boost::intrusive_ptr<MemoryMessage> msg (aTransport[MemoryMessageTag]);

        // For Invalidates and Downgrades, the uArchState isn't for us, it's for the original requester
        // So in those cases we always want to construct a new MemOp based on the MemoryMesage
        if (msg->isPageWalk()){

        }
        if (aTransport[uArchStateTag] && msg->type() != MemoryMessage::Invalidate && msg->type() != MemoryMessage::Downgrade) {
            op = aTransport[uArchStateTag];
        } else {
            op = new MemOp();
            op->thePAddr = msg->address();
            op->theSize = eSize(msg->reqSize());
            op->theTracker = aTransport[TransactionTrackerTag];
        }

        switch (msg->type()) {
            case MemoryMessage::LoadReply:
                op->theOperation = kLoadReply;
                break;

            case MemoryMessage::AtomicPreloadReply:
                op->theOperation = kAtomicPreloadReply;
                break;

            case MemoryMessage::StoreReply:
                op->theOperation = kStoreReply;
                break;

            case MemoryMessage::NonAllocatingStoreReply:
                op->theOperation = kStoreReply;
                break;

            case MemoryMessage::StorePrefetchReply:
                op->theOperation = kStorePrefetchReply;
                break;

            case MemoryMessage::Invalidate:
                op->theOperation = kInvalidate;
                break;

            case MemoryMessage::Downgrade:
                op->theOperation = kDowngrade;
                break;

            case MemoryMessage::Probe:
                op->theOperation = kProbe;
                break;

            case MemoryMessage::RMWReply:
                op->theOperation = kRMWReply;
                break;

            case MemoryMessage::CmpxReply:
                op->theOperation = kCASReply;
                break;

            case MemoryMessage::ReturnReq:
                op->theOperation = kReturnReq;
                break;
            case MemoryMessage::StoreDataReply:	//ALEX
            case MemoryMessage::LoadDataReply:
            case MemoryMessage::LoadDataInLLCReply:
            case MemoryMessage::StoreDataInLLCReply:
                //does the microArch need to do anything?
                break;


            default:
                DBG_Assert( false,  ( << "Unhandled Memory Message type: " << msg->type() ) );
        }

        //RMC STUFF - BEGIN
        if (isRMC && (msg->type() == MemoryMessage::StoreReply
                    || msg->type() == MemoryMessage::RMWReply
                    || msg->type() == MemoryMessage::StoreDataInLLCReply
                    || msg->type() == MemoryMessage::LoadDataInLLCReply
                    || msg->type() == MemoryMessage::StoreDataReply
                    || msg->type() == MemoryMessage::LoadDataReply))
            return;
        //RMC STUFF - END

        theMicroArch->pushMemOp(op);
    }

    //RMC STUFF - BEGIN
    //This part contains all of the RMC's functionalities
    public:
    void RMC_MemoryIn( MemoryTransport & aTransport) {		//Responses from the cache
        //FIXME: WHAT HAPPENS WITH MORE THAN ONE CONCURRENT REQUESTS TO THE SAME ADDRESS?
        MemoryTransport transport;
        boost::intrusive_ptr<MemoryMessage> operation;
        boost::intrusive_ptr<MemoryMessage> msg (aTransport[MemoryMessageTag]);
        boost::intrusive_ptr<TransactionTracker> tracker;
        uint64_t theAddress = msg->address();
        DBG_( RMC_DEBUG, ( << "RMC-" << RMCid << " got a message in MemoryIn: " << *(aTransport[MemoryMessageTag])));

        if (ENABLE_SABRE_PREFETCH) {
            MAQiter = prefetchMAQ.begin();
            while (MAQiter != prefetchMAQ.end()) {
                if ((MAQiter->thePAddr  == theAddress) && MAQiter->dispatched) break;
                MAQiter++;
            }
            if (MAQiter != prefetchMAQ.end()) {
                DBG_( SABRE_DEBUG, ( << "\t This is an RRPP prefetch reply. "));
                prefetchMAQ.erase(MAQiter);
            }
            //fall through; if an outstanding normal request for the same address is in the real MAQ, pass the data.
        }

        MAQiter = MAQ.begin();
        while (MAQiter != MAQ.end()) {
            if ((MAQiter->thePAddr  == theAddress) && MAQiter->dispatched) break;
            MAQiter++;
        }

        if (MAQiter == MAQ.end()) {
            if (aTransport[MemoryMessageTag]->type() != MemoryMessage::Invalidate && aTransport[MemoryMessageTag]->type() != MemoryMessage::Downgrade) {
                DBG_(RMC_DEBUG, (<< "WARNING: RMC-" << RMCid << " got reply with no match in the MAQ! " << *aTransport[MemoryMessageTag]));
            }
            return;
        }
        switch (aTransport[MemoryMessageTag]->type()) {
            case MemoryMessage::LoadReply:
            case MemoryMessage::StoreReply:
                DBG_( RMC_DEBUG, ( << "\t Received " << *(aTransport[MemoryMessageTag])));
                break;
            case MemoryMessage::StoreDataInLLCReply:
            case MemoryMessage::LoadDataInLLCReply:
            case MemoryMessage::StoreDataReply:
            case MemoryMessage::LoadDataReply:
                DBG_( RMC_DEBUG, ( << "\t Received " << *(aTransport[MemoryMessageTag])));
                break;
            case MemoryMessage::Downgrade:
            case MemoryMessage::Invalidate:
                DBG_( RMC_DEBUG, ( << "Warning: Received a Snoop Message for an address that is in the MAQ. MAQ request of type " << MAQiter->type << ") " << *(aTransport[MemoryMessageTag])));
                break;
            default:
                DBG_Assert(false, ( << "How did this MemoryTransport end up in the MAQ? Incoming: " << *aTransport[MemoryMessageTag] << " while in MAQ: " << *(MAQiter->theTransport[MemoryMessageTag])));
        }

        DBG_( RMC_DEBUG, ( << "RMC-" << RMCid << " - Got reply from memory for request of type " << std::dec << MAQiter->type << " with tid " << std::dec << (int)MAQiter->tid));

        switch (MAQiter->type) {
            case BufferReadForSend: {
                                        std::list<RG5LQ_t>::iterator iter = RGData5.LoadQ.begin();		//Note: Can definitely make this faster
                                        while (iter != RGData5.LoadQ.end() && iter->PAddr != MAQiter->thePAddr) {
                                            iter++;
                                        }
                                        DBG_Assert(iter != RGData5.LoadQ.end());
                                        theCPU = API::QEMU_get_cpu_by_index(iter->QP_id);
                                        DBG_( RGP_DEBUG, ( << "Read the local buffer for a SEND request."));
                                        uint64_t theDataBufferAddress = QEMU_read_phys_memory_wrapper_dword(theCPU, iter->PAddr+16, 8);
                                        DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a BufferReadForSend. Reading address 0x" << std::hex << iter->PAddr+16));
                                        DBG_( RGP_DEBUG, ( << "\tData buffer address: 0x" << std::hex << theDataBufferAddress << ". Waking up entry in unroll queue..."));
                                        std::list<RG4LQ_t>::iterator unrollQiter = RGData4.UnrollQ.begin();
                                        while (unrollQiter != RGData4.UnrollQ.end()) {
                                            if (unrollQiter->packet.tid == iter->packet.tid) {
                                                unrollQiter->sendBufAllocated = true;
                                                unrollQiter->ready = true;
                                                unrollQiter->packet.offset = theDataBufferAddress;
                                            }
                                        }
                                        RGData5.LoadQ.erase(iter);
                                        break;
                                    }
            case BufferReadForRequest:	{	//RGP
                                            std::list<RG5LQ_t>::iterator iter = RGData5.LoadQ.begin();		//Note: Can definitely make this faster
                                            while (iter != RGData5.LoadQ.end() && iter->PAddr != MAQiter->thePAddr) {
                                                iter++;
                                            }
                                            DBG_Assert(iter != RGData5.LoadQ.end());
                                            theCPU = API::QEMU_get_cpu_by_index(iter->QP_id);
                                            DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a BufferReadForRequest. Reading address 0x" << std::hex << iter->PAddr));
                                            DBG_( RGP_DEBUG, ( << "Read data from local buffer for an RWRITE or SEND."));
                                            for (uint8_t i=0; i<iter->packet.pl_size; i++) {
                                                iter->packet.payload.data[i] = (uint8_t)QEMU_read_phys_memory_wrapper_byte(theCPU, iter->PAddr+i, 1);
                                                DBG_( RGP_DEBUG, ( << "\t 0x" << std::hex << iter->PAddr+i << ": data["<< std::dec << (uint32_t) i<< "] = 0x" << std::hex << (int)iter->packet.payload.data[i]));
                                            }
                                            iter->completed = true;
                                            //stats
                                            RGPMemByteCount += 64;
                                            RCPDataBlocksHandledInterval[MACHINE_ID]++;
                                            break;
                                        }
            case WQRead:	{			//RGP
                                DBG_Assert(!cfg.MagicCacheConnection);
                                DBG_( RGP_FRONTEND_DEBUG, ( << "\tWQRead: Notifying RGP stage 2 about completion"));

                                std::list<RG2LQ_t>::iterator iter = RGData2.LoadQ.begin();
                                while (iter != RGData2.LoadQ.end() && iter->WQaddr != MAQiter->thePAddr) {
                                    iter++;
                                }
                                DBG_Assert(iter != RGData2.LoadQ.end());
                                theCPU = API::QEMU_get_cpu_by_index(iter->QP_id);
                                DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a WQRead. Reading address 0x" << std::hex << iter->WQaddr));
                                iter->WQEntry1 = QEMU_read_phys_memory_wrapper_dword(theCPU, iter->WQaddr, 8);
                                iter->WQEntry2 = QEMU_read_phys_memory_wrapper_dword(theCPU, iter->WQaddr+8, 8);
                                iter->completed = true;
                                break;
                            }
            case CQWrite:	{			//RCP
                                DBG_Assert(!cfg.MagicCacheConnection);
                                DBG_(RCP_DEBUG, ( << "CQWrite completed! Written " << MAQiter->pl_size << " CQ entries to memory:"));
                                DBG_(WQ_CQ_ACTIONS_DEBUG, ( << "CQWrite completed! Written " << MAQiter->pl_size << " CQ entries to memory:"));

                                //stats
                                TotalCQProcessing[MACHINE_ID] += theCycleCount - CQstartTS[MAQiter->tid];
                                //DBG_(Tmp, ( << "RCP-" << RMCid << " CQWrite completed! latency: " << theCycleCount - CQstartTS[MAQiter->tid] << " cycles"));

                                CQstartTS[MAQiter->tid] = theCycleCount;		//2nd setting of CQstartTS (for non-SplitRGP)
                                theCPU = API::QEMU_get_cpu_by_index(MAQiter->tid);
                                DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a CQWrite. Writing to address 0x" << std::hex << MAQiter->CQEntries[0]));
                                for (uint32_t i=0; i<MAQiter->pl_size; i++) {
                                    API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr+i * sizeof(cq_entry_t), MAQiter->CQEntries[i], sizeof(cq_entry_t));
                                    DBG_(RCP_DEBUG, ( << "\t" << (std::bitset<8 * sizeof(cq_entry_t)>)MAQiter->CQEntries[i] << " to 0x" << std::hex << MAQiter->thePAddr+i * sizeof(cq_entry_t) ));
                                }
                                break;
                            }
            case BufferWriteForReply:	{ //RCP
                                            theCPU = API::QEMU_get_cpu_by_index(MAQiter->tid);
                                            DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a BufferWriteForReply. Writing to address 0x" << std::hex << MAQiter->packet.payload.data[0]));
                                            for (int i=0; i<64; i++) {
                                                API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr+i, MAQiter->packet.payload.data[i], 1);
                                            }
                                            ITT_table_iter = ITT_table.find(MAQiter->ITTidx);
                                            DBG_Assert(ITT_table_iter != ITT_table.end());
                                            DBG_Assert(ITT_table_iter->second.counter > 0);
                                            ITT_table_iter->second.counter--;
                                            if (ITT_table_iter->second.counter == 0) {
                                                tidReadyList.push_back(MAQiter->ITTidx);      //notify the RCP to write the CQ entry
                                                if (ITT_table_iter->second.isSABRe)
                                                    prefetchTokens = (prefetchTokens+1) % (PREFETCH_TOKENS+1);
                                            }
                                            DBG_(RCP_DEBUG, ( << "RCP-" << RMCid << " completed BufferWriteForReply. Wrote data to PAddr 0x" << std::hex << MAQiter->thePAddr));
                                            //stats
                                            RCPMemByteCount += 64;
                                            RCPDataBlocksHandledInterval[MACHINE_ID]++;
                                            break;
                                        }
            case ContextRead:	{ //RRPP
                                    DBG_( RRPP_DEBUG, ( << "\t ContextRead completed: " << *(MAQiter->theTransport[MemoryMessageTag])));
                                    theCPU = API::QEMU_get_cpu_by_index(MACHINE_ID * CORES_PER_MACHINE);
                                    uint64_t theIndex = pidMux(MAQiter->packet.offset, MAQiter->packet.dst_nid, MAQiter->packet.tid, MAQiter->packet.RMCid);
                                    std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter = RRPData3.LSQ.find(theIndex);
                                    DBG_Assert(LSQiter != RRPData3.LSQ.end());
                                    DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a ContextRead. Reading address 0x" << std::hex << MAQiter->thePAddr));
                                    for (uint8_t i=0; i<64; i++) {
                                        LSQiter->second.packet.payload.data[i] = QEMU_read_phys_memory_wrapper_byte(theCPU, MAQiter->thePAddr+i, 1);
                                    }
                                    LSQiter->second.completed = true;
                                    //stats
                                    RRPPMemByteCount += 64;
                                    RRPPDataBlocksHandledInterval[MACHINE_ID]++;
                                    break;
                                }
            case ContextWrite:	 { //RRPP
                                     theCPU = API::QEMU_get_cpu_by_index(MACHINE_ID * CORES_PER_MACHINE);
                                     uint64_t theIndex = pidMux(MAQiter->packet.offset, MAQiter->packet.dst_nid, MAQiter->packet.tid, MAQiter->packet.RMCid);
                                     std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter = RRPData3.LSQ.find(theIndex);
                                     DBG_Assert(LSQiter != RRPData3.LSQ.end());
                                     DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a ContextWrite. Writing to address 0x" << std::hex << MAQiter->thePAddr));
                                     for (uint8_t i=0; i<64; i++) {
                                         API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr+i, LSQiter->second.packet.payload.data[i], 1);
                                     }
                                     LSQiter->second.completed = true;

                                     DBG_( RRPP_DEBUG, ( << "\t ContextWrite to physical address 0x" << std::hex << MAQiter->thePAddr
                                                 << " completed: " << *(MAQiter->theTransport[MemoryMessageTag])));
                                     DBG_( RRPP_DEBUG, ( << "\tAddress | Data:"));
                                     for (uint8_t i=0; i<64; i++) {
                                         DBG_( RRPP_DEBUG, ( << "\t0x" << std::hex << MAQiter->thePAddr+i << " | " << std::dec <<
                                                     QEMU_read_phys_memory_wrapper_byte(theCPU, MAQiter->thePAddr+i, 1)));
                                     }

                                     //stats
                                     RRPPMemByteCount += 64;
                                     RRPPDataBlocksHandledInterval[MACHINE_ID]++;
                                     break;
                                 }
            case RecvBufUpdate: {//RRPP for SEND msgs - metadata
                                    theCPU = API::QEMU_get_cpu_by_index(MACHINE_ID * CORES_PER_MACHINE);
                                    uint64_t theIndex = pidMux(MAQiter->packet.SABRElength, MAQiter->packet.dst_nid, MAQiter->packet.tid, MAQiter->packet.RMCid);
                                    std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter = RRPData3.LSQ.find(theIndex);
                                    DBG_Assert(LSQiter != RRPData3.LSQ.end());
                                    DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a RecvBufUpdate. Incrementing counter at address 0x" << std::hex << MAQiter->thePAddr));
                                    uint64_t counter = QEMU_read_phys_memory_wrapper_dword(theCPU, MAQiter->thePAddr, 8);
                                    if (counter == MAQiter->packet.SABRElength) {
                                        DBG_(MESSAGING_DEBUG, ( << "WARNING!! This should not happen; msg length is " << MAQiter->packet.SABRElength << " and want to add " << LSQiter->second.counter
                                                    << ", but current counter is already " << counter << ". Resetting to zero..."));
                                        counter = 0;
                                    }
                                    DBG_Assert(counter + LSQiter->second.counter <= MAQiter->packet.SABRElength, (<< "msg length is " << MAQiter->packet.SABRElength << ", but current counter is "
                                                << counter << " and want to add " << LSQiter->second.counter << " more."));
                                    API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr, counter + LSQiter->second.counter, 8);

                                    DBG_( RRPP_DEBUG, ( << "\t RecvBufUpdate to physical address 0x" << std::hex << MAQiter->thePAddr
                                                << ". Value was " << counter << ", and " << LSQiter->second.counter << " updates to it waiting in LSQ. Updating to "
                                                << (counter + LSQiter->second.counter)));
                                    if (counter + LSQiter->second.counter == MAQiter->packet.SABRElength) {
                                        DBG_( RRPP_DEBUG, ( << "\t All packets of SEND msg received; need to notify a core through its CQ"));
                                        RRPData6.push_back(MAQiter->packet);
                                    }
                                    RRPData3.LSQ.erase(LSQiter);
                                    break;
                                }
            case RecvBufWrite: {//RRPP for SEND msgs - data
                                   theCPU = API::QEMU_get_cpu_by_index(MACHINE_ID * CORES_PER_MACHINE);
                                   uint64_t theIndex = pidMux(MAQiter->packet.pkt_index, MAQiter->packet.dst_nid, MAQiter->packet.tid, MAQiter->packet.RMCid);
                                   std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter = RRPData3.LSQ.find(theIndex);
                                   DBG_Assert(LSQiter != RRPData3.LSQ.end());
                                   DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a RecvBufWrite. Writing to address 0x" << std::hex << MAQiter->thePAddr));
                                   for (uint8_t i=0; i<64; i++) {
                                       API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr+i, LSQiter->second.packet.payload.data[i], 1);
                                   }
                                   LSQiter->second.completed = true;

                                   DBG_( RRPP_DEBUG, ( << "\t RecvBufWrite to physical address 0x" << std::hex << MAQiter->thePAddr
                                               << " completed: " << *(MAQiter->theTransport[MemoryMessageTag])));
                                   DBG_( RRPP_DEBUG, ( << "\tAddress | Data:"));
                                   uint32_t step=4;        
                                   for (uint8_t i=0; i<64; i+=step) {
                                       DBG_( RRPP_DEBUG, ( << "\t0x" << std::hex << MAQiter->thePAddr+i << " | 0x" << std::hex <<
                                                   QEMU_read_phys_memory_wrapper_word(theCPU, MAQiter->thePAddr+i, step)));
                                   }
                                   break;
                               }
            case SendBufFree: {
                                  theCPU = API::QEMU_get_cpu_by_index(MACHINE_ID * CORES_PER_MACHINE);
                                  uint64_t theIndex = pidMux(MAQiter->packet.offset, MAQiter->packet.dst_nid, MAQiter->packet.tid, MAQiter->packet.RMCid);
                                  std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter = RRPData3.LSQ.find(theIndex);
                                  DBG_Assert(LSQiter != RRPData3.LSQ.end());
                                  DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a SendBufFree. Zeroing out valid bit at address 0x" << std::hex << MAQiter->thePAddr));
                                  uint8_t theSRBit = QEMU_read_phys_memory_wrapper_byte(theCPU, MAQiter->thePAddr, 1);
                                  DBG_Assert(theSRBit == 1, ( << "Expected valid SEND entry in SEND buffer, but valid bit was " << theSRBit << "."));
                                  theSRBit = 0;   //zero out the valid bit
                                  DBG_( RRPP_DEBUG, ( << "\t SendBufFree to physical address 0x" << std::hex << MAQiter->thePAddr
                                              << " completed: " << *(MAQiter->theTransport[MemoryMessageTag])
                                              << ". Zeroing the valid bit..." ));
                                  API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr, theSRBit, 1);

                                  LSQiter->second.completed = true;
                                  break;
                              }
            case ContextPrefetch: {
                                      DBG_Assert(false);
                                  }
            default:
                                  DBG_Assert(false);
        }
        prev = MAQiter;
        //%%%%REMOVE - Sanity check that could be quite expensive
        if (MAQiter != MAQ.end()) {
            MAQiter++;
            while (MAQiter != MAQ.end()) {
                if ((MAQiter->thePAddr  == theAddress) && MAQiter->dispatched) {
                    DBG_Assert(false);  //this should never happen, because I moved the check inside driveMAQ()
                    DBG_(Crit, ( <<"RMC-" << RMCid << "MAQ: WARNING! TWO REQUESTS WITH SAME ADDRESS 0x" << std::hex << theAddress << " DISPATCHED!!"));
                    DBG_(Crit, ( << "\tThe two requests are of type\n\t" << prev->type << " (" << *(prev->theTransport[MemoryMessageTag]) << ") and type\n\t" 
                                <<  MAQiter->type << " (" << *(MAQiter->theTransport[MemoryMessageTag]) << ")" ));
                }
                MAQiter++;
            }
        }
        /////
        MAQ.erase(prev);
    }

    void driveMAQ() {	//push pending transactions to cache

        //if (!FLEXUS_CHANNEL(MemoryOut_Request).available() && RMCid == 2) DBG_(<>, ( << "RMC-" << RMCid << " Cannot push to RMC cache!"));
        //&&&&&&&&&&&&&	MagicCacheConnection
        if (cfg.MagicCacheConnection && delayQueue->ready()) {
            MemoryTransport trans( delayQueue->dequeue());
            MAQiter = MAQ.begin();
            while (MAQiter != MAQ.end()) {
                if ((MAQiter->thePAddr  == (uint64_t)trans[MemoryMessageTag]->address()) && MAQiter->dispatched) break;
                MAQiter++;
            }
            DBG_Assert(MAQiter != MAQ.end());

            switch (MAQiter->type) {
                case WQRead:	{			//RGP
                                    DBG_( RGP_FRONTEND_DEBUG, ( << "\tWQRead: Notifying RGP stage 2 about completion"));
                                    std::list<RG2LQ_t>::iterator iter = RGData2.LoadQ.begin();
                                    while (iter != RGData2.LoadQ.end() && iter->WQaddr != MAQiter->thePAddr) {
                                        iter++;
                                    }
                                    DBG_Assert(iter != RGData2.LoadQ.end());
                                    theCPU = API::QEMU_get_cpu_by_index(iter->QP_id);
                                    iter->WQEntry1 = QEMU_read_phys_memory_wrapper_dword(theCPU, iter->WQaddr, 8);
                                    iter->WQEntry2 = QEMU_read_phys_memory_wrapper_dword(theCPU, iter->WQaddr+8, 8);
                                    iter->completed = true;
                                    DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a WQRead. Reading address 0x" << std::hex << iter->WQaddr));
                                    break;
                                }
                case CQWrite:	{			//RCP
                                    DBG_(RCP_DEBUG, ( << "CQWrite completed! Written " << MAQiter->pl_size << " CQ entries to memory:"));
                                    CQstartTS[MAQiter->tid] = theCycleCount;		//3rd time CQstart TS is set (for SplitRGP only)
                                    theCPU = API::QEMU_get_cpu_by_index(MAQiter->tid);
                                    for (uint32_t i=0; i<MAQiter->pl_size; i++) {
                                        API::QEMU_write_phys_memory(theCPU, MAQiter->thePAddr+i * sizeof(cq_entry_t), MAQiter->CQEntries[i], sizeof(cq_entry_t));
                                        DBG_(RCP_DEBUG, ( << "\t" << (std::bitset<8 * sizeof(cq_entry_t)>)MAQiter->CQEntries[i] << " to 0x" << std::hex << MAQiter->thePAddr+i * sizeof(cq_entry_t) ));
                                        DBG_Assert(MAQiter->CQEntries[i] == QEMU_read_phys_memory_wrapper_dword(theCPU, MAQiter->thePAddr+i * sizeof(cq_entry_t), sizeof(cq_entry_t)));
                                    }
                                    DBG_(MEMORY_ACCESS_DEBUG, ( << "RMC-"<<RMCid<<": MAQ completing a CQWrite. Writing to address 0x" << std::hex << MAQiter->thePAddr));
                                    break;
                                }
                default:
                                DBG_Assert(false);
            }
            MAQ.erase(MAQiter);
        }
        //&&&&&&&&&&&&&
        //#ifdef FLOW_CONTROL_STATS
        bool idleCycle = true;
        if (!FLEXUS_CHANNEL(MemoryOut_Request).available()) { 
            idleCycle = false;
        }
        //#endif
        MAQiter = MAQ.begin();
        while (MAQiter != MAQ.end() && FLEXUS_CHANNEL(MemoryOut_Request).available()) {
            if (!MAQiter->dispatched) {
                prev = MAQ.begin();
                bool raceCondition = false;
                while (prev != MAQiter) {
                    if (prev->dispatched && prev->thePAddr == MAQiter->thePAddr) { //should stall this access! could cause trouble - should be INFREQUENT
                        raceCondition = true;
                        DBG_(Crit, ( <<"RMC-" << RMCid << "MAQ: WARNING! NEW REQUEST WITH SAME ADDRESS 0x" << std::hex << MAQiter->thePAddr << " PENDING!! (will hurt performance if frequent occurrence..)"));
                        DBG_(Crit, ( << "\tThe two requests are of type\n\t" << MAQiter->type << " (" << *(MAQiter->theTransport[MemoryMessageTag]) << " - waiting to be dispatched) and type\n\t" 
                                    << prev->type << " (" << *(prev->theTransport[MemoryMessageTag]) << " - already dispatched)" ));
                        break;
                    }
                    prev++;
                }             
                if (raceCondition) break;   
                //if (RMCid == 2) DBG_( Dev, ( << "RMC-" << RMCid << "'s MAQ pushing request of type " << MAQiter->type << " to L1 cache"));
                if (cfg.MagicCacheConnection && (MAQiter->type == WQRead || MAQiter->type == CQWrite)) {
                    if (MAQiter->type == WQRead) {
                        DBG_(MAGIC_CONNECTIONS_DEBUG, ( << "RMC-" << RMCid << " Sending a WQRead though the MagicCacheAccess interface"));
                    } else {
                        DBG_(MAGIC_CONNECTIONS_DEBUG, ( << "RMC-" << RMCid << " Sending a CQWrite though the MagicCacheAccess interface"));
                    }
                    theMagicTransport = MAQiter->theTransport;
                    FLEXUS_CHANNEL(MagicCacheAccess) << MAQiter->theTransport;
                    FLEXUS_CHANNEL(MagicCoreCommunication) << MAQiter->theTransport;
                } else {
                    if (MAQiter->type == CQWrite) {
                        CQstartTS[MAQiter->tid] = theCycleCount;		//1st setting of CQstartTS (for non-splitRGP)
                    }
                    DBG_( RMC_DEBUG, ( << "\t RMC-" << RMCid << "'s MAQ sending to mem: " << *(MAQiter->theTransport[MemoryMessageTag])
                                << " for entry of type " << MAQiter->type));
                    MAQiter->theTransport[MemoryMessageTag]->setFromRMC();
                    if (MAQiter->type == ContextRead || MAQiter->type == RecvBufUpdate || MAQiter->type == RecvBufWrite || MAQiter->type == SendBufFree) {
                        MAQiter->theTransport[MemoryMessageTag]->setFromRRPP();
                    }
                    FLEXUS_CHANNEL(MemoryOut_Request) << MAQiter->theTransport;

                    if (MAQiter->type == ContextRead)	{ //stats
                        uint64_t theIndex = pidMux(MAQiter->packet.offset, MAQiter->packet.dst_nid, MAQiter->packet.tid, MAQiter->packet.RMCid);
                        std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter = RRPData3.LSQ.find(theIndex);
                        DBG_Assert(LSQiter != RRPData3.LSQ.end());
                        LSQiter->second.TS_issued = theCycleCount;
                    }
                    //#ifdef FLOW_CONTROL_STATS
                    idleCycle = false;
                    requestsPushed++;
                    //#endif
                }
                MAQiter->dispatched = true;
            }
            MAQiter++;
        }
#ifdef FLOW_CONTROL_STATS
        if (idleCycle && isRRPP) {
#ifdef SINGLE_MACHINE
            if (MACHINE_ID != SINGLE_MACHINE_ID)
#endif
                DBG_(FC_DBG_LVL, ( << "FLOW CONTROL STATS: RMC-" << RMCid << " had an idle cycle; no request pushed to memory, while memory interface was available."));
        }
#endif
    }

    void printStats() {
        if (cfg.ServiceTimeMeasurement) {
            if (RMCid == getRMCDispatcherID_LocalMachine()) {
                DBG_(Dev, ( << "*** Service time measurement stats ***\nRequests serviced in " 
                            << theCycleCount << " cycles: " << REQUESTS_COMPLETED[SERVICE_TIME_MEASUREMENT_CORE]
                            << "\nAverage service time (cycles): " 
                            << float(theCycleCount)/REQUESTS_COMPLETED[SERVICE_TIME_MEASUREMENT_CORE]));
            }
            if (RMCid == getRMCDispatcherID_LocalMachine()) {
                DBG_(Dev, ( << "Depth of request queue is " << CQWaitQueue[0].size() ));
            } 
        }
        if (isRRPP) {
            DBG_(SABRE_STATS, (<< "RMC-" << RMCid << " ATT occupancy: " << ATTtags.size() << " / " << ATT_SIZE
                        << " (" << std::setprecision(2) << ((float)ATTtags.size()/ATT_SIZE) << "%)"));
            //#ifdef FLOW_CONTROL_STATS
            DBG_(STATS_DEBUG, ( << "FLOW CONTROL STATS: RRPP-" << RMCid << " pushed "
                        << ((float)requestsPushed/theCycleCount) << " requests per cycle to the memory hierarchy ("
                        << (((float)requestsPushed * 64 / pow(1024,3)) / ((float)theCycleCount / 2 / pow(10,9))) << " GBps)"
                        << "\nData packets pushed to network: " << networkDataPacketsPushed << ", memory accesses (blocks): " <<  perRRPPblockCount
                        << "\nAverage RRPP processing per block: "
                        << (perRRPPblockCount ? boost::lexical_cast<std::string>(perRRPPLatency/perRRPPblockCount) : "-") << " cycles"
                        << "\nAverage RRPP block access scheduling-to-issue latency: "
                        << (perRRPPblockCount ? boost::lexical_cast<std::string>(perRRPPIssueLatency/perRRPPblockCount) : "-") << " cycles"));

            //DBG_(Tmp, ( << "\tRRPP-" << RMCid << " - Replies sent to network: " << requestsPushed));
            //#endif
        }
        if (!RMCid) {
#ifdef MESSAGING_STATS
            uint32_t i,j;
            uint64_t totalSum = 0, partialSum = 0 ;
            DBG_(PROGRESS_DEBUG, ( << "\nMESSAGING STATS:"));
            DBG_(PROGRESS_DEBUG, ( << "Total number of requests completed: " << RECV_COMPLETE_COUNT));
            DBG_(PROGRESS_DEBUG, ( << "Breakdown per RRPP:\n====================="));
            for (i = 0; i<CORES/CORES_PER_RMC; i++) {
                partialSum = 0;
                for (j=i*CORES_PER_RMC;j<(i+1)*CORES_PER_RMC;j++) partialSum+=REQUESTS_COMPLETED[j];
                DBG_(PROGRESS_DEBUG, ( << "RRPP-" << i*CORES_PER_RMC << ": " << partialSum));
                totalSum+=partialSum;
            }
            DBG_Assert(RECV_COMPLETE_COUNT == totalSum);
            if (RECV_COMPLETE_COUNT >= 100000) API::QEMU_break_simulation("100K ops done!");
#endif
            DBG_(SABRE_STATS, ( << "*****SABRE STATS*****\n"
                        << " Succeeded SABREs:" << succeeded_SABRes
                        << "\n Failed SABREs:" << failed_SABRes
                        << "\n Completed object writes:" << object_writes
                        << "\n******************"));
            /* int i;
               DBG_(SABRE_STATS, (<< "Avg SABRe critical sections (cycles):"));    //This is only meaningful for synchronous requests
               for (i=0; i<64; i++) {
               if (SABReCount[i])
               DBG_(SABRE_STATS, (<< "\tRRPP " << i << ": " << (SABRElatTot[i]/SABReCount[i])));
               }
               DBG_(SABRE_STATS, (<< "# SABRes serviced"));
               for (i=0; i<64; i++) {
               if (SABReCount[i])
               DBG_(SABRE_STATS, (<< "\tRRPP " << i << ": " << SABReCount[i]));
               }
               DBG_(SABRE_STATS, (<< "Avg writer critical sections (cycles):"));
               for (i=0; i<64; i++) {
               if (CSCount[i])
               DBG_(SABRE_STATS, (<< "\tCPU " << i << ": " << (CSlatTot[i]/CSCount[i])));
               }
               DBG_(SABRE_STATS, (<< "")); //empty line in output
               */
        }

        DBG_( STATS_DEBUG, ( << "RMC-" << RMCid << "\n*********** ROUNDTRIP STATS (E2E) ************\n"
                    << "transaction counter for this RMC: "	<< RTtransactionCounter
                    << "\nAverage latency: " << RTlatencyAvg
                    << " cycles. \nMin latency: " << RTminLat
                    << " cycles. \nMax latency: " << RTmaxLat
                    << " cycles. \nMax latency (no extremes): " << RTmaxLatNoExtremes
                    << "\n**********************************************\n"));
        DBG_( STATS_DEBUG, ( << "\nRGP Data BW: " << (RGPMemByteCount/theCycleCount)
                    << " Bytes per cycle\nRGP NW BW: " << (RGPNWByteCount/theCycleCount)
                    << " Bytes per cycle\nRCP Data BW: " << (RCPMemByteCount/theCycleCount)
                    << " Bytes per cycle\nRCP NW BW: " << (RCPNWByteCount/theCycleCount)
                    << " Bytes per cycle\nRRPP Data BW: " <<	(RRPPMemByteCount/theCycleCount)
                    << " Bytes per cycle\nRRPP incoming NW BW: " << (RRPPincNWByteCount/theCycleCount)
                    << " Bytes per cycle\nRRPP outgoing NW BW: " << (RRPPoutNWByteCount/theCycleCount)
                    << " Bytes per cycle\n"));

        //        if (FLEXI_APP_REQ_SIZE) {
        if (CQCountPC[RMCid]) {
            DBG_( STATS_DEBUG, ( << " RCP " << RMCid << " BANDWIDTH STATS -- Total number of transfers handled by this RCP: "
                        << CQCountPC[RMCid]  << ". RCP BW (GBps): "
                        << std::setw(4) << std::fixed << std::setprecision(2)
                        << (((float)CQCountPC[RMCid] * FLEXI_APP_REQ_SIZE / theCycleCount) * 2*pow(10,9)/pow(1024,3) ) ));
        }
        //        }

        if (!(RMCid % CORES_PER_MACHINE)) {
            if (cfg.SplitRGP) {
                DBG_( STATS_DEBUG, ( << "MACHINE " << MACHINE_ID << " LATENCY STATS\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
                            << "Average processing time to enqueue WQ entry: "
                            << (WQCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalWQProcessing[MACHINE_ID]/WQCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage time to transfer WQ entry to RMC: "
                            << (WQTransferCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalWQTransferWait[MACHINE_ID]/WQTransferCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage time to transfer CQ entry to RMC: "  //for non-SplitRGP, this is the time between CQWrite from RMC side until CQ read from app
                            << (CQCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalCQTransfer[MACHINE_ID]/CQCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage time for app to read new CQ entry: " 	//for non-SplitRGP, this is the time between CQWrite issuing and completion, from RMC side
                            << (CQCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalCQProcessing[MACHINE_ID]/CQCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage RRPP processing per block: "
                            << (RRPBlockCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalRRPLatency[MACHINE_ID]/RRPBlockCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage RRPP block access scheduling-to-issue latency: "
                            << (RRPBlockCount[MACHINE_ID] ? boost::lexical_cast<std::string>(RRPIssueLatency[MACHINE_ID]/RRPBlockCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage E2E latency is: "
                            << (transCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalE2ELat[MACHINE_ID]/transCount[MACHINE_ID]) : "-") << " cycles"
                            << "\n(min lat: " << minE2Elat[MACHINE_ID] << ", max lat: " << maxE2Elat[MACHINE_ID] << ")"
                            << "\n(total number of transfers: " << transCount[MACHINE_ID]
                            << ", transfer size: " << FLEXI_APP_REQ_SIZE << ")\n"));
            } else {
                DBG_( STATS_DEBUG, ( << "MACHINE " << MACHINE_ID << " LATENCY STATS\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
                            << "Average processing time to enqueue WQ entry: "
                            << (WQCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalWQProcessing[MACHINE_ID]/WQCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage time for RMC to read new WQ entry: "
                            << (WQTransferCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalWQTransferWait[MACHINE_ID]/WQTransferCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage time for RMC to write CQEntry: "
                            << (CQCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalCQProcessing[MACHINE_ID]/CQCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage time for app to read CQEntry: "
                            << (CQTransferCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalCQTransfer[MACHINE_ID]/CQTransferCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage RRPP processing per block: "
                            << (RRPBlockCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalRRPLatency[MACHINE_ID]/RRPBlockCount[MACHINE_ID]) : "-") << " cycles"
                            << "\nAverage E2E latency is: "
                            << (transCount[MACHINE_ID] ? boost::lexical_cast<std::string>(TotalE2ELat[MACHINE_ID]/transCount[MACHINE_ID]) : "-") << " cycles"
                            << "\n(total number of transfers: " << transCount[MACHINE_ID] << ", transfer size: " << FLEXI_APP_REQ_SIZE << ")\n"));
            }
        }

        if (!(RMCid % CORES_PER_MACHINE)) {
            RCPDataBlocksHandled[MACHINE_ID] += RCPDataBlocksHandledInterval[MACHINE_ID];
            RRPPDataBlocksHandled[MACHINE_ID] += RRPPDataBlocksHandledInterval[MACHINE_ID];
            DBG_( STATS_DEBUG, ( << "MACHINE " << MACHINE_ID << " BANDWIDTH STATS\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
                        << "Total number of data blocks handled by RCPs | by RRPPs (total/useful) | Total cycle count \n\t\t\t\t\t"
                        << RCPDataBlocksHandled[MACHINE_ID] << "\t\t" << RRPPDataBlocksHandled[MACHINE_ID] << " / "
                        << (RRPPDataBlocksHandled[MACHINE_ID] - RRPPDataBlocksOverhead[MACHINE_ID]) << "\t\t" << theCycleCount
                        << "\n RCP BW | RRPP BW (useful) | TOTAL BW (GBps) \n"
                        << std::setw(4) << std::fixed << std::setprecision(2)
                        << (((float)RCPDataBlocksHandled[MACHINE_ID] * 64 / theCycleCount) * 2*pow(10,9)/pow(1024,3) )
                        << "\t" << (((float)(RRPPDataBlocksHandled[MACHINE_ID] - RRPPDataBlocksOverhead[MACHINE_ID]) * 64 / theCycleCount) * 2*pow(10,9)/pow(1024,3) )
                        << "\t" << (((float)(RCPDataBlocksHandled[MACHINE_ID]+RRPPDataBlocksHandled[MACHINE_ID]) * 64 / theCycleCount) * 2*pow(10,9)/pow(1024,3) )
                        << " (overall)\n"
                        << (((float)RCPDataBlocksHandledInterval[MACHINE_ID] * 64 / STATS_PRINT_FREQ) * 2*pow(10,9)/pow(1024,3) )
                        << "\t" << (((float)(RRPPDataBlocksHandledInterval[MACHINE_ID]) * 64 / STATS_PRINT_FREQ) * 2*pow(10,9)/pow(1024,3) )
                        << "\t" << (((float)(RCPDataBlocksHandledInterval[MACHINE_ID]+RRPPDataBlocksHandledInterval[MACHINE_ID]) * 64 / STATS_PRINT_FREQ) * 2*pow(10,9)/pow(1024,3) )
                        << " (this time window)\n"
                        ));
            RCPDataBlocksHandledInterval[MACHINE_ID] = 0;
            RRPPDataBlocksHandledInterval[MACHINE_ID] = 0;
        }
    }

    int totalCQWaitQueueSize() {
        int i;
        int totalCQWaitSize = 0;
        for (i=0; i<MAX_CORE_COUNT; i++) {
            totalCQWaitSize+=CQWaitQueue[i].size();
        }
        return totalCQWaitSize;
    }

    void memCheck() {
        if (theCycleCount % 10000) return;
        //check the size of all dynamic data structures used by the RMC
        DBG_(MEMCHECK_DEBUG, ( << "RMC-" << RMCid << " DEBUG MSG -- MEMORY CHECK! Sizes of dynamic data structures below:\n"
                    << "\nSENDS_RECEIVED / SENDS_SERVICED: " << SENDS_RECEIVED << " / " << RECV_COMPLETE_COUNT 
                    << "\nRequestTimer: " << RequestTimer.size()
                    << "\nRTcycleCounter: " << RTcycleCounter.size()
                    << "\nRRPPtimers: " << RRPPtimers.size()
                    << "\nctxToBEs: " <<   ctxToBEs.size()
                    << "\nRGPinQueue: " <<                RGPinQueue.size()
                    << "\nRCPinQueue: " <<                RCPinQueue.size()
                    << "\nCQWaitQueue: " <<                totalCQWaitQueueSize() //CQWaitQueue.size()
                    << "\nbufferSizes: " <<                bufferSizes.size()
                    << "\nprefetches: " <<                prefetches.size()
                    << "\ninFlightPrefetches: " <<           inFlightPrefetches.size()
                    << "\nRMC_TLB: " <<                 RMC_TLB.size()
                    << "\ntempDataStore: " <<                 tempDataStore.size()
                    << "\nafterPWEntries: " <<                afterPWEntries.size()
                    << "\nMAQ: " <<                 MAQ.size()
                    << "\nprefetchMAQ: " <<                 prefetchMAQ.size()
                    << "\nITT_table: " <<                 ITT_table.size()
                    << "\nQP_table: " <<                 QP_table.size()
                    << "\nmessaging_table: " <<                 messaging_table.size()
                    << "\ntidFreelist: " <<                 tidFreelist.size()
                    << "\nQPidFreelist: " <<                QPidFreelist.size()
                    << "\ntidReadyList: " <<                tidReadyList.size()
                    << "\nATTfreelist: " <<                 ATTfreelist.size()
                    << "\nATTpriolist: " <<                 ATTpriolist.size()
                    << "\nATTtags: " <<                 ATTtags.size()
                    << "\nRGData2.LoadQ: " <<                 RGData2.LoadQ.size()
                    << "\nRGData4.UnrollQ: " <<                RGData4.UnrollQ.size()
                    << "\nRGData5.LoadQ: " <<                 RGData5.LoadQ.size()
                    << "\nRCData3.theCQBuffers: " <<                RCData3.theCQBuffers.size()
                    << "\nRRPData3.LSQ: " <<                 RRPData3.LSQ.size()
                    << "\nRRPData6: " <<                 RRPData6.size()    ));
        int i;
        for (i=0; i<MAX_CORE_COUNT; i++) {
            if (REQUEST_TIMESTAMP[i].size() > 10) {
                DBG_(Tmp, (<< "REQUEST_TIMESTAMP[" << i << "]: " << REQUEST_TIMESTAMP[i].size() ));
            }
        }
        //Don't check; shouldn't be growing
        // PTv2p, bufferv2p, contextv2p, sendbufv2p, recvbufv2p, contextMap
    }

    void RMC_drive() {		//Drive the three pipelines

        //if (theCycleCount % 2 != RMCid % 2) return;		//Everything operates at half of the processor's frequency, i.e., 1GHz

        DBG_( VVerb, ( << "RMC-" << RMCid << " drive called. Running transactions: " << MAQ.size()
                    << " | pending PageWalk: " << pendingPagewalk  ));
        //Drive the three pipelines and handle packets
        if (isRGP) driveRCP();
        if (isRRPP) driveRRPP();
        //handleIncomingPackets();
        //sendPackets();

        if (isRGP
#ifdef SINGLE_MACHINE
                && MACHINE_ID == SINGLE_MACHINE_ID
#endif
           ) driveRGP();

        driveMAQ();
        drivePWalker();
        //memCheck(); //check the sizes of dynamic data structures
    }

    void driveRGP() {
        //RGPip stage 6: form packet and push to NI
        //DBG_(<>, (<<"RMC-" <<RMCid << "Checking Request queue:"));
        //if (!FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REQUEST).available()) DBG_(<>, ( << "OutBuffer not available!"));
        //DBG_(<>, (<<"RMC-" <<RMCid << "Checking Reply queue:"));
        //if (!FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REPLY).available()) DBG_(<>, ( << "OutBuffer not available!"));
        //%%%%%%%%	SPLIT-RMC STUFF
        if (cfg.SplitRGP && !hasBackend) goto RGP_FRONTEND_LABEL;
        //%%%%%%%%

        if (!RGPip[6].inputFree
                && FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REQUEST).available()
                && outstandingPackets < OUTSTANDING_PACKET_LIMIT) {
#ifdef TRACE_LATENCY
            DBG_(Dev, ( << "RMC-" << RMCid << "'s RGP STAGE 6: pushing REQUEST packet with packet index 0x" << std::hex <<  RGData6.packet.pkt_index << " to RMCConnector"));
#endif
            DBG_(RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 6: pushing REQUEST packet with offset 0x" << std::hex << RGData6.packet.offset
                        << " and routing hint 0x" << RGData6.packet.routing_hint << " to RMCConnector"));
            //outQueueReq.push_back(RGData6.packet);
            RGData6.packet.RMCid = RMCid;
            FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REQUEST) << RGData6.packet;
            //if (RGData6.packet.op != SABRE)
            outstandingPackets++;
            RGPip[6].inputFree = true;
            //stats
            if (RGData6.packet.carriesPayload())
                RGPNWByteCount += 75;
            else
                RGPNWByteCount += 11;	//header size
        }
#ifdef FLOW_CONTROL_STATS
        else if (!RGPip[6].inputFree && !FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REQUEST).available()) {
            DBG_(Dev, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 6: Stalling because network backpressures"));
        } else if (!RGPip[6].inputFree && outstandingPackets >= OUTSTANDING_PACKET_LIMIT) {
            DBG_(FC_DBG_LVL, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 6: Stalling because this RMC reached its limit of outstanding messages (" << OUTSTANDING_PACKET_LIMIT << ")"));
        } else if (outstandingPackets < OUTSTANDING_PACKET_LIMIT) {
            DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 6: Can have more oustanding messages, but got nothing to push."));
        }
#endif
        //RGPip stage 5
        //First, check if there is a completed entry in the LoadQueue
        if (RGPip[6].inputFree) {
            std::list<RG5LQ_t>::iterator iter = RGData5.LoadQ.begin();
            while (iter != RGData5.LoadQ.end() && iter->completed == false) {
                iter++;
            }
            if (iter != RGData5.LoadQ.end()) {
                RGPip[6].inputFree = false;
                RGData6.packet = iter->packet;
                RGData5.LoadQ.erase(iter);
                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 5: Read local data for RWRITE or SEND. Pushing request to outbuffer"));
            }
        }
        //Second, check if there is a new message from the previous stage of the pipeline
        if (!RGPip[5].inputFree && RGPip[6].inputFree) {		//stall conditions
            if (RGData5.incData.packet.op == RREAD || RGData5.incData.packet.op == SABRE || RGData5.incData.packet.op == RECV) {
                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 5: Got " << RGData5.incData.packet.op << ". Pushing it to RMCConnector"));
                RGData6.packet = RGData5.incData.packet;
                RGPip[6].inputFree = false;
                RGPip[5].inputFree = true;
            } else if ((RGData5.incData.packet.op == RWRITE || RGData5.incData.packet.op == SEND) && !RGData5.full() ) {
                if (RGData5.incData.packet.op == RWRITE) {DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 5: Got RWRITE. Need to read local data first"));}
                else {DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 5: Got SEND. Need to read local data first"));}
                RGData5.incData.completed = false;
                RGData5.incData.VAddr = RGData5.incData.packet.bufaddr;

                tempAddr = bufferv2p[RGData5.incData.QP_id].find(RGData5.incData.VAddr & PAGE_BITS);
                DBG_Assert( tempAddr != bufferv2p[RGData5.incData.QP_id].end(), ( << "Should have found the translation of 0x" << std::hex << RGData5.incData.VAddr
                            << " for QP " << RGData5.incData.QP_id) );
                RGData5.incData.PAddr = tempAddr->second + RGData5.incData.VAddr - (RGData5.incData.VAddr & PAGE_BITS);
                bool pageWalk = probeTLB(RGData5.incData.VAddr);		//FIXME: for now this always returns false
                handleTLB(RGData5.incData.VAddr, RGData5.incData.PAddr, BOGUS, _RGP);

                RGData5.LoadQ.push_back(RGData5.incData);

                MemoryTransport transport;
                boost::intrusive_ptr<MemoryMessage> operation;
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                //operation = MemoryMessage::newLoad(PhysicalMemoryAddress(RGData5.incData.PAddr), VirtualMemoryAddress(RGData5.incData.VAddr));
                operation = new MemoryMessage(MemoryMessage::DATA_LOAD_OP, PhysicalMemoryAddress(RGData5.incData.PAddr), VirtualMemoryAddress(RGData5.incData.VAddr));
                operation->reqSize() = 8;
                tracker->setAddress( PhysicalMemoryAddress(RGData5.incData.PAddr) );
                tracker->setInitiator(flexusIndex());
                tracker->setSource("RMC");
                tracker->setOS(false);
                transport.set(TransactionTrackerTag, tracker);

                transport.set(MemoryMessageTag, operation);

                aMAQEntry.theTransport = transport;
                aMAQEntry.type = BufferReadForRequest;
                aMAQEntry.dispatched = false;
                aMAQEntry.thePAddr = RGData5.incData.PAddr;
                aMAQEntry.theVAddr = RGData5.incData.VAddr;
                aMAQEntry.bufAddrV = RGData5.incData.VAddr;
                aMAQEntry.src_nid = RMCid;
                aMAQEntry.tid = RGData5.incData.packet.tid;
                aMAQEntry.offset = RGData5.incData.packet.offset;
                aMAQEntry.pl_size = RGData5.incData.packet.pl_size;
                aMAQEntry.pid = _RGP;

                MAQ.push_back(aMAQEntry);

                RGPip[5].inputFree = true;
            } else if (!RGData5.full()) {
                DBG_Assert(false , ( << "Undexpected op type: " << RGData5.incData.packet.op));
            }
#ifdef false  //in case a 1st load is needed to locate src buffer address for send
            else if (RGData5.incData.packet.op == SEND /*&& FLEXUS_CHANNEL( MemoryOut_Request ).available()*/) {
                /* //will be needed if I decide to move all the enqueuing logic to hardware
                //send AllocateSendEntryReq messages to all other RGP BEs to request an entry in the send queue
                MemoryTransport transport;
                boost::intrusive_ptr<MemoryMessage> operation;
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                operation = new MemoryMessage(MemoryMessage::AllocateSendEntryReq);
                operation->nid = RGData5.incData.packet.dst_nid;

                tracker->setInitiator(flexusIndex());
                tracker->setSource("RMC");
                tracker->setOS(false);
                transport.set(TransactionTrackerTag, tracker);
                transport.set(MemoryMessageTag, operation);

                DBG_(SPLIT_RGP_DEBUG, ( << " RGP-" << RMCid << " Frontend pushing AllocateSendEntryReq out."));
                //DBG_(Tmp, ( << " RGP-" << RMCid << " Frontend pushing WQMessage out: " << *transport[MemoryMessageTag]));
                FLEXUS_CHANNEL( MemoryOut_Request ) << transport;
                RGPip[5].inputFree = true;
                */

                //Read lbuf location from send_buf_entry_t
                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 5: Got a SEND. Need to read location of local data first"));
                RGData5.incData.completed = false;
                RGData5.incData.VAddr = RGData5.incData.packet.bufaddr;

                tempAddr = sendbufv2p[MACHINE_ID].find(RGData5.incData.VAddr & PAGE_BITS);
                DBG_Assert( tempAddr != sendbufv2p[MACHINE_ID].end(), ( << "Should have found the translation of " << std::dec << RGData5.incData.VAddr) );
                RGData5.incData.PAddr = tempAddr->second + RGData5.incData.VAddr - (RGData5.incData.VAddr & PAGE_BITS);
                bool pageWalk = probeTLB(RGData5.incData.VAddr);		//FIXME: for now this always returns false
                handleTLB(RGData5.incData.VAddr, RGData5.incData.PAddr, BOGUS, _RGP);

                RGData5.LoadQ.push_back(RGData5.incData);

                MemoryTransport transport;
                boost::intrusive_ptr<MemoryMessage> operation;
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                //operation = MemoryMessage::newLoad(PhysicalMemoryAddress(RGData5.incData.PAddr), VirtualMemoryAddress(RGData5.incData.VAddr));
                operation = new MemoryMessage(MemoryMessage::DATA_LOAD_OP, PhysicalMemoryAddress(RGData5.incData.PAddr), VirtualMemoryAddress(RGData5.incData.VAddr));
                operation->reqSize() = 8;
                tracker->setAddress( PhysicalMemoryAddress(RGData5.incData.PAddr) );
                tracker->setInitiator(flexusIndex());
                tracker->setSource("RMC");
                tracker->setOS(false);
                transport.set(TransactionTrackerTag, tracker);

                transport.set(MemoryMessageTag, operation);

                aMAQEntry.theTransport = transport;
                aMAQEntry.type = BufferReadForSend;
                aMAQEntry.dispatched = false;
                aMAQEntry.thePAddr = RGData5.incData.PAddr;
                aMAQEntry.theVAddr = RGData5.incData.VAddr;
                aMAQEntry.bufAddrV = RGData5.incData.VAddr;
                aMAQEntry.src_nid = RMCid;
                aMAQEntry.tid = RGData5.incData.packet.tid;
                aMAQEntry.offset = RGData5.incData.packet.offset;
                aMAQEntry.pl_size = RGData5.incData.packet.pl_size;
                aMAQEntry.pid = _RGP;

                MAQ.push_back(aMAQEntry);

                RGPip[5].inputFree = true;
            }
#endif
#ifdef FLOW_CONTROL_STATS
            else if (RGData5.incData.packet.op == RWRITE && RGData5.full() ) {
                DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 5: Stalling because Load Queue for BufferRead for RWRITEs is full!"));
            }
#endif
        }
        //RGPip stage 4 - This is the Unroll queue stage
        if (!RGPip[4].inputFree) {		//First, check if new packet to enqueue in Unroll Queue
            if (!RGData4.full()) {
                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 4: Enqueuing packet in Unroll Queue"));
                if (RGData4.incData.packet.pl_size > 1)
                    DBG_( RGP_DEBUG, ( << "\t Request will have to be unrolled into " << RGData4.incData.packet.pl_size << " packets"));
                RGData4.UnrollQ.push_back(RGData4.incData);

                RGPip[4].inputFree = true;
            }
#ifdef FLOW_CONTROL_STATS
            else {
                DBG_(FC_DBG_LVL, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 4: Stalling because UnrollQ is full!"));
            }
#endif
        }

        if (!RGData4.UnrollQ.empty() && RGPip[5].inputFree) {	//Second, push a packet out
            //Unroll policy: unrolling one request beginning-to-end
            std::list<RG4LQ_t>::iterator unrollQiter = RGData4.UnrollQ.begin();
            if (RGP_PREFETCH_SABRE && prefetchTokens) {
                while (unrollQiter != RGData4.UnrollQ.end()) {       //Caution: Could this deplete my tokens and throttle throughput?
                    if (unrollQiter->pleasePrefetch) {
                        DBG_( Tmp, ( << "RMC-" << RMCid << "'s RGP STAGE 4: Found a SABRe waiting in the queue. Issuing a prefetch for first block!!!"));
                        unrollQiter->pleasePrefetch = false;
                        prefetchTokens--;
                        break;
                    }
                    unrollQiter++;
                }
            }

            if (unrollQiter == RGData4.UnrollQ.end()) unrollQiter = RGData4.UnrollQ.begin();

            while (unrollQiter != RGData4.UnrollQ.end() && !unrollQiter->ready) unrollQiter++;
            if (unrollQiter != RGData4.UnrollQ.end()) {

                RMCPacket aPacket = unrollQiter->packet;

                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 4: Pushing packet to STAGE 5"));

                /* //in case a 1st load is needed to locate src buffer address for send
                   if (aPacket.op == SEND) {
                   if (!unrollQiter->sendBufAllocated) { //special case for messaging
                   DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 4: This is a SEND request; need to allocate entry in SEND BUF "));
                   unrollQiter->ready = false;
                   } else {
                //set up transfer for SEND, similar to an RWRITE; now source buffer is known (address in offset field)
                //compute destination address for recv buffer based on target node, msgs per node, buf entry size
                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'computing recv buffer address for SEND request: "));
                recvBufAddress = messaging_table[aPacket.context].num_nodes
                uint64_t internal_offset = (aPacket.bufaddr - messaging_table[aPacket.context].send_buf_addr) / sizeof(send_buf_entry_t);
                uint64_t external_offset = aPacket.dst_nid * (messaging_table[aPacket.context].msg_entry_size + BLOCK_SIZE) * messaging_table[aPacket.context].msg_buf_entry_count;
                uint64_t recvBufAddress = external_offset + internal_offset;
                aPacket.bufaddr = aPacket.offset;
                aPacket.offset = recvBufAddress;
                }
                }
                */
                if (unrollQiter->ready) {
                    RGData5.incData.packet = aPacket;
                    RGData5.incData.packet.pl_size = 64;		//Now this is in bytes
                    RGData5.incData.QP_id = unrollQiter->QP_id;
                    RGPip[5].inputFree = false;
                    if (aPacket.pl_size != 1) {
                        if (unrollQiter->packet.op != SABRE && unrollQiter->packet.op != SEND)  unrollQiter->packet.routing_hint+=64;	//All packets of a SABRe need to have the same routing hint, to end up at the same RRPP
                        unrollQiter->packet.bufaddr+=64;
                        if (unrollQiter->packet.op != SEND) unrollQiter->packet.offset+=64;   //for SEND, offset is the index to the recv buffer; don't change across packets of same msg
                        unrollQiter->packet.pkt_index++;
                        unrollQiter->packet.pl_size--;
                        DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 4: Not done with request yet; pushing it back in Unroll Queue (unrolls remaining: "
                                    << unrollQiter->packet.pl_size <<")"));
                    } else {
                        RGData4.UnrollQ.pop_front();
                    }
                } else {
                    DBG_Assert(false); //should always be ready in current code version
                }
            }
        }
#ifdef FLOW_CONTROL_STATS
        else if (RGData4.UnrollQ.empty()) {
            DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 4: Nothing to unroll; the unroll queue is empty!"));
        }
#endif

        //RGPip stage 3
        if (!RGPip[3].inputFree && RGPip[4].inputFree) {		//stall conditions
            if (RGData3.WQEntry.SR == RGData3.WQ_SR) {
                DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 3: New valid WQ entry found (from QP "
                            << RGData3.QP_id << ")!\n\top field value is: " << (int)RGData3.WQEntry.op
                            << "\n\tnid field value is: " << (int)RGData3.WQEntry.nid
                            << "\n\tcid field value is: " << (int)RGData3.WQEntry.cid
                            << "\n\tbuf_address field value is: 0x" << std::hex << (uint64_t)RGData3.WQEntry.buf_addr
                            << "\n\toffset field value is: 0x" << std::hex << (uint64_t)RGData3.WQEntry.offset << " (block offset -- 0x" << (RGData3.WQEntry.offset<<6) << " Byte offset)"
                            << "\n\tlength field value is: " << std::dec << (uint64_t)RGData3.WQEntry.length << " (" << (RGData3.WQEntry.length << 6) << " Bytes)"));
                DBG_Assert(RGData3.WQEntry.length > 0, (<< "RMC-" << RMCid << " Something funny is happening... read appearingly valid WQ entry, but length field is zero!"));

                if (!tidFreelist.empty()) {
                    QP_table_iter = QP_table.find(RGData3.QP_id);
                    if (!cfg.SplitRGP) { //if splitRGP, this is is done in the last stage of the frontend
                        DBG_Assert(QP_table_iter != QP_table.end());
                        if (FLEXI_APP_REQ_SIZE) {
                            RGData3.WQEntry.buf_addr = QP_table_iter->second.lbuf_base + (RGData3.WQEntry.buf_addr * FLEXI_APP_REQ_SIZE) % QP_table_iter->second.lbuf_size;

                            std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter;
                            cntxtIter = contextMap[MACHINE_ID].find(RGData3.WQEntry.cid);
                            DBG_Assert(cntxtIter != contextMap[MACHINE_ID].end(), ( << "Request for access to unregistered context " << RGData3.WQEntry.cid));
                            uint64_t ctx_size = cntxtIter->second.second;

                            RGData3.WQEntry.offset = ((rand()%ctx_size));	//RANDOM ADDRESS
                            RGData3.WQEntry.offset >>=6;
                            RGData3.WQEntry.offset <<=6;
                            DBG_Assert(RGData3.WQEntry.offset%64 == 0);
                            if (RGData3.WQEntry.offset + FLEXI_APP_REQ_SIZE > ctx_size) RGData3.WQEntry.offset -= FLEXI_APP_REQ_SIZE;
                            RGData3.WQEntry.length = FLEXI_APP_REQ_SIZE >> 6;
                            DBG_(RGP_DEBUG, ( << "\tRUNNING IN FLEXI MODE with FLEXI_APP_REQ_SIZE = " << FLEXI_APP_REQ_SIZE
                                        << " and context size = " << ctx_size
                                        << "! This means that actually: "
                                        << "\n\tbuf_address = 0x" << std::hex << (uint64_t)RGData3.WQEntry.buf_addr
                                        << "\n\toffset = " << std::dec << (uint64_t)RGData3.WQEntry.offset
                                        << "\n\tlength = " << (uint64_t)RGData3.WQEntry.length));
                        } else {
                            RGData3.WQEntry.offset <<=6;
                        }
                    } else { //if not splitRGP
                        RGData3.WQEntry.offset <<=6;
                    }
                    uint16_t tid = *(tidFreelist.begin());
                    tidFreelist.pop_front();

                    ITT_table_entry_t anITTEntry;
                    anITTEntry.counter = RGData3.WQEntry.length;
                    anITTEntry.size = RGData3.WQEntry.length;
                    anITTEntry.QP_id = RGData3.QP_id;
                    anITTEntry.WQ_index = RGData3.WQ_tail;
                    anITTEntry.lbuf_addr = RGData3.WQEntry.buf_addr;
                    anITTEntry.success = true;
                    anITTEntry.isSABRe = false;
                    DBG_(RGP_DEBUG, ( << "Initializing ITT entry " << tid << " corresponding to QP " << anITTEntry.QP_id << "'s entry "
                                << anITTEntry.WQ_index << ", with counter = " <<  anITTEntry.counter));
                    ITT_table[tid] = anITTEntry;

                    RemoteOp theOp = RREAD; //bogus init
                    switch (RGData3.WQEntry.op) {
                        case RMC_WRITE:
                            theOp = RWRITE;
                            break;
                        case RMC_READ:
#ifdef RREAD_IS_SABRE
                            theOp = SABRE;         //For testing reasons (transforms regular outgoing RREAD requests into SABRes)
                            ITT_table[tid].isSABRe = true;
#else
                            theOp = RREAD;
#endif
                            break;
                        case RMC_SABRE:
#ifdef SABRE_IS_RREAD
                            theOp = RREAD;	    //For testing reasons (transforms outgoing SABRe requests into regular RREADS)
#else
                            theOp = SABRE;
                            ITT_table[tid].isSABRe = true;
#endif
                            break;
                        case RMC_SEND: {
                                           theOp = SEND;
                                           RGData3.WQEntry.offset >>=6;    //undo shift; this is used as index to recv buffer, not address offset

                                           DBG_( MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 3: New SEND entry found in WQ "
                                                       << RGData3.QP_id << "!\n\toffset: 0x" << std::hex << (uint64_t)RGData3.WQEntry.offset
                                                       << "\n\tlength: " << std::dec << (uint64_t)RGData3.WQEntry.length << " (" << (RGData3.WQEntry.length << 6) << " Bytes)"));

                                           //Make sure the entry in the send buffer is marked as valid.
                                           uint64_t sendBufAddrV = messaging_table[RGData3.WQEntry.cid].send_buf_addr
                                               + RGData3.WQEntry.nid * messaging_table[RGData3.WQEntry.cid].msg_buf_entry_count * sizeof(send_buf_entry_t) //node offset
                                               + RGData3.WQEntry.offset * sizeof(send_buf_entry_t);
                                           tempAddr = sendbufv2p[MACHINE_ID].find(sendBufAddrV & PAGE_BITS);
                                           DBG_Assert( tempAddr != sendbufv2p[MACHINE_ID].end(), ( << "Should have found the translation of 0x" << std::hex << sendBufAddrV) );
                                           uint64_t sendBufAddrP = tempAddr->second + sendBufAddrV - (sendBufAddrV & PAGE_BITS);
                                           theCPU = API::QEMU_get_cpu_by_index(RGData3.QP_id);
                                           uint8_t validField = QEMU_read_phys_memory_wrapper_byte(theCPU, sendBufAddrP, 1);

                                           if (cfg.SelfCleanSENDs) { //this is a hack just for facilitating the RMCTrafficGenerator
                                               if (validField != 0) {
                                                   API::QEMU_write_phys_memory(theCPU, sendBufAddrP, 0, 1);
                                               }
                                           } else {
                                               if (validField != 1) {
                                                   DBG_(Crit, ( << "*WARNING*: Expected valid entry at Paddress 0x" << std::hex << sendBufAddrP << " but value was " << (int) validField << ". Setting from Simics."));
                                                   API::QEMU_write_phys_memory(theCPU, sendBufAddrP, 1, 1);
                                               }
                                           }
                                           break;
                                       }
                        case RMC_RECV:
                                       if (cfg.SingleRMCDispatch) {
                                           DBG_Assert(RMCid == getRMCDispatcherID_LocalMachine() );   //all RMC RECV requests have to be routed through the centralized dispatcher!
                                       }
                                       DBG_( MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 3: New RECV entry found in WQ "
                                                   << RGData3.QP_id << "!\n\tbuf_addr: 0x" << std::hex << (uint64_t)RGData3.WQEntry.buf_addr));
                                       ITT_table[tid].size = 1;
                                       theOp = RECV;
#ifdef MESSAGING_STATS 
                                       //Update stats!!
                                       REQUESTS_COMPLETED[RGData3.QP_id]++;
                                       RECV_COMPLETE_COUNT++;
                                       DBG_Assert(!REQUEST_TIMESTAMP[RGData3.QP_id].empty());
                                       RequestTimerIter = RequestTimer.find(*(REQUEST_TIMESTAMP[RGData3.QP_id].begin()));
                                       DBG_Assert(RequestTimerIter != RequestTimer.end());
                                       DBG_Assert(RequestTimerIter->second.core_id == RGData3.QP_id);

                                       if( RequestTimerIter->second.exec_time_extra == 0) {
                                           RequestTimerIter->second.exec_time_extra = 3735928559;
                                       } else {
                                           RequestTimerIter->second.exec_time_extra = theCycleCount - RequestTimerIter->second.exec_time_extra;
                                       }
                                       dumpLatenciesToFile(RequestTimerIter->second);
                                       /*//DEBUG FOR LONG QUEUING LATENCIES
                                         if (RequestTimerIter->second.queueing_time > 10000) {
                                         DBG_(Tmp, (<< "CPU-" << RMCid << " packet with uniqueID " << RequestTimerIter->first << " finished execution. Execution time: " << RequestTimerIter->second.execution_time
                                         << ", Queueing time: " << RequestTimerIter->second.queueing_time << ". Arrived at cycle " << RequestTimerIter->second.start_time ));}*/
                                       DBG_(MESSAGING_DEBUG, ( << "\tCore " << RGData3.QP_id << " finished processing incoming SEND request. \
                                                   Erasing entry with unique ID " << RequestTimerIter->first << " from RequestTimer"));
                                       REQUEST_TIMESTAMP[RGData3.QP_id].erase(REQUEST_TIMESTAMP[RGData3.QP_id].begin());
                                       RequestTimer.erase(RequestTimerIter);
#endif

                                       break;
                        default:
                                       DBG_Assert(false, (<< "Unknown RMC op!"));
                    }

                    RGData4.incData.packet = RMCPacket(tid, MACHINE_ID, RGData3.WQEntry.nid, RGData3.WQEntry.cid, (uint64_t)RGData3.WQEntry.offset,
                            theOp, (uint64_t)RGData3.WQEntry.buf_addr, (uint64_t)0, (uint16_t)RGData3.WQEntry.length);
                    RGData4.incData.pleasePrefetch = false;
                    RGData4.incData.packet.routing_hint = (uint64_t)RGData3.WQEntry.offset;

                    //Here, length is the total length of the transaction in terms of blocks. Unroll stage will change it accordingly
                    if (theOp == SABRE) {
                        switch (SABReToRRPPDistr) {
                            case eRandom:
                                RGData4.incData.packet.routing_hint = std::rand();  //assigning this SABRe to an RRPP picked at random
                                break;
                            case eMatching:
                                RGData4.incData.packet.routing_hint = (RMCid >> log_base2(CORES_PER_RMC)) << (6 + log_base2(CORES_PER_RMC));
                                break;
                            default:
                                DBG_Assert(false);
                        }
                        //RGData4.incData.packet.routing_hint = (SABResSent % CORES_PER_RMC) << (6+log_base2(CORES_PER_RMC));    //round-robin
                        //RGData4.incData.packet.routing_hint = (RMCid/CORES_PER_RMC) << (6+log_base2(CORES_PER_RMC));        //send the SABRe to the RRPP with the same RMCid
                        SABResSent++;
                        RGData4.incData.pleasePrefetch = true;
                        RGData4.incData.packet.SABRElength = RGData3.WQEntry.length;
                    } else if (theOp == SEND){
                        RGData4.incData.packet.routing_hint <<= 6;
                        RGData4.incData.packet.SABRElength = RGData3.WQEntry.length;
                    } else if (theOp == RECV && sharedCQinfo[MACHINE_ID].translations.empty()) {  //if shared CQ is not used
                        //decrement the cur_jobs counter of the originating QP
                        DBG_Assert(QP_table[RGData3.QP_id].cur_jobs>0);
                        DBG_(MESSAGING_DEBUG, ( << "RECV side-effect: Decrementing QP " << RGData3.QP_id << "'s cur_job counter by one (current value:" << QP_table[RGData3.QP_id].cur_jobs << ")"));
                        QP_table[RGData3.QP_id].cur_jobs--;
                    }
                    RGData4.incData.QP_id = RGData3.QP_id;
                    RGData4.incData.ready = true;
                    DBG_( RGP_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 3: Created a packet with: "
                                << "\n\ttid: " << (int)RGData4.incData.packet.tid
                                << "\n\tsrc_nid: " << (int)RGData4.incData.packet.src_nid		//Source node is always 0
                                << "\n\tdst_nid: " << (int)RGData4.incData.packet.dst_nid
                                << "\n\top: " << (RemoteOp)RGData4.incData.packet.op
                                << "\n\tcid: " << (int)RGData4.incData.packet.context
                                << "\n\tbuf_address : 0x" << std::hex << (uint64_t)RGData4.incData.packet.bufaddr
                                << "\n\toffset: 0x" << std::hex << (uint64_t)RGData4.incData.packet.offset
                                << "\n\tpl_size: " << std::dec << (uint16_t)RGData4.incData.packet.pl_size << " (cache blocks)"));

                    if (!cfg.SplitRGP) { //if splitRGP, this is is done in the last stage of the frontend
                        //Update WQ tail
                        QP_table_iter->second.WQ_tail++;
                        if (QP_table_iter->second.WQ_tail >= wqSize) {
                            QP_table_iter->second.WQ_tail = 0;
                            QP_table_iter->second.WQ_SR ^= 1;
                        }
                    }
                    RGPip[3].inputFree = true;
                    RGPip[4].inputFree = false;
                }
#ifdef FLOW_CONTROL_STATS
                else {
                    DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RGP STAGE 3: No free tid! Stalling RGP."));
                }
#endif

                //stats
                //if (/*(cfg.SplitRGP && theCycleCount - WQstartTS[RGData3.QP_id] < 200) || (!cfg.SplitRGP &&*/( theCycleCount - WQstartTS[RGData3.QP_id] < 500)) {	//leaving out extreme values. Dunno how they occur...
                TotalWQTransferWait[MACHINE_ID] += theCycleCount - WQstartTS[RGData3.QP_id];
                WQTransferCount[MACHINE_ID]++;
                //} else {
                //	E2EstartTS[RGData3.QP_id] = -1;
                //}
                //DBG_(Tmp, ( << "RMC-" << RMCid << "'s RGP STAGE 3: WQTransfer time: " << theCycleCount - WQstartTS[RGData3.QP_id] << " cycles. Start: " <<  WQstartTS[RGData3.QP_id] << ", end: " << theCycleCount));

            } else {
                if (cfg.SplitRGP) DBG_Assert(false, ( << "WQEntry does not contain a valid entry. This should never happen in Split-RMC mode"));
                DBG_( RGP_FRONTEND_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 3: WQEntry does not contain a valid entry"));
                RGPip[3].inputFree = true;
                DBG_( Verb, ( << "\t Current SR is " << (int)RGData3.WQ_SR << " and entry's fields are: "
                            << "\n\tSR: " << (int)RGData3.WQEntry.SR
                            << "\n\top: " << (int)RGData3.WQEntry.op
                            << "\n\tnid field value is: " << (int)RGData3.WQEntry.nid
                            << "\n\tcid field value is: " << (int)RGData3.WQEntry.cid
                            << "\n\tbuf_address field value is: 0x" << std::hex << (uint64_t)RGData3.WQEntry.buf_addr
                            << "\n\toffset field value is: " << std::dec << (uint64_t)RGData3.WQEntry.offset << "(block offset)"
                            << "\n\tlength field value is: " << (uint64_t)RGData3.WQEntry.length));
            }
            //notify RGP stage 0
            if (!cfg.SplitRGP) QPidFreelist.push_back(RGData3.QP_id);
        }

        //%%%%%%%%	SPLIT-RMC STUFF
        if (cfg.SplitRGP && hasBackend) {		//Need to feed the RGP backend (fill out RGPip[3])
            std::list<MemoryTransport>::iterator RGPqIter = RGPinQueue.begin();
            if (RGPqIter != RGPinQueue.end() && RGPip[3].inputFree) {
                DBG_(SPLIT_RGP_DEBUG, ( << "RGP-" << RMCid << " Feeding RGP Backend from RGPinQueue."));
                RGData3.WQEntry.op = (*RGPqIter)[MemoryMessageTag]->op;
                RGData3.WQEntry.cid = (*RGPqIter)[MemoryMessageTag]->cid;
                RGData3.WQEntry.nid = (*RGPqIter)[MemoryMessageTag]->nid;
                RGData3.WQEntry.buf_addr = (*RGPqIter)[MemoryMessageTag]->buf_addr;
                RGData3.WQEntry.offset = (*RGPqIter)[MemoryMessageTag]->offset;
                RGData3.WQEntry.length = (*RGPqIter)[MemoryMessageTag]->length;
                RGData3.WQEntry.SR = 1;				//Setting both WQEntry.SR and WQ_SR to 1 (always valid)
                RGData3.QP_id = (*RGPqIter)[MemoryMessageTag]->QP_id;
                RGData3.WQ_tail = (*RGPqIter)[MemoryMessageTag]->WQ_idx;
                RGData3.WQ_SR = 1;

                RGPinQueue.erase(RGPqIter);
                RGPip[3].inputFree = false;
            }
        }
        //%%%%%%%%
        //If splitRGP, this is the pipeline's splitting point!! Need to create one extra stage above, and one extra stage below
        //%%%%%%%%	SPLIT-RMC STUFF
RGP_FRONTEND_LABEL:
        if (cfg.SplitRGP && !hasFrontend) return;
        if (cfg.SplitRGP && FLEXUS_CHANNEL( MemoryOut_Request ).available() && !RGPipFE.inputFree) {
            //Check input (RGData3a). If valid entry, create msg and push it out
            if (RGData3a.WQEntry.SR == RGData3a.WQ_SR) {
                DBG_(SPLIT_RGP_DEBUG, ( << "RGP-" << RMCid << " Frontend: New valid WQ entry found for QP_id " << RGData3a.QP_id << "'s entry " << RGData3a.WQ_tail <<"!"
                            << "\n\t(outstanding FE Transfers: " << outstandingFETransfers << ")"));
                if (outstandingFETransfers < OUTSTANDING_FE_TRANSFERS_LIMIT) { //FLOW CONTROL. Limit outstanding Msgs
                    DBG_(RGP_FRONTEND_DEBUG, ( << "RGP-" << RMCid << " Frontend: New valid WQ entry found for QP_id " << RGData3a.QP_id << "'s entry " << RGData3a.WQ_tail <<"!"));
                    DBG_Assert( RGData3a.WQEntry.valid, ( << "RGP-" << RMCid << " found valid WQ entry, but app hasn't set valid bit to 1"));
                    QP_table_iter = QP_table.find(RGData3a.QP_id);
                    DBG_Assert(QP_table_iter != QP_table.end());
                    //FOR FLEXI APPS - Random address generation!!
                    if (FLEXI_APP_REQ_SIZE) {
                        //FIXME: Buffer size is aggregate, i.e., for all threads. Need to manually divide by # threads (atm, 15). Bring commented line back.
                        //RGData3a.WQEntry.buf_addr = QP_table_iter->second.lbuf_base + (RGData3a.WQEntry.buf_addr * FLEXI_APP_REQ_SIZE) % (QP_table_iter->second.lbuf_size/15);
                        RGData3a.WQEntry.buf_addr = QP_table_iter->second.lbuf_base + (RGData3a.WQEntry.buf_addr * FLEXI_APP_REQ_SIZE) % QP_table_iter->second.lbuf_size;

                        std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter;
                        cntxtIter = contextMap[MACHINE_ID].find(RGData3a.WQEntry.cid);
                        DBG_Assert(cntxtIter != contextMap[MACHINE_ID].end(), ( << "Request for access to unregistered context " << RGData3a.WQEntry.cid));
                        uint64_t ctx_size = cntxtIter->second.second;

                        RGData3a.WQEntry.offset = ((rand()%ctx_size));	//RANDOM ADDRESS
                        //DBG_(Tmp, ( << "Context size = 0x" << std::hex << QP_table_iter->second.ctx_size << ", randomized offset = 0x" << RGData3a.WQEntry.offset ));
                        RGData3a.WQEntry.offset >>=6;
                        RGData3a.WQEntry.offset <<=6;
                        DBG_Assert(RGData3a.WQEntry.offset%64 == 0);
                        if (RGData3a.WQEntry.offset + FLEXI_APP_REQ_SIZE > ctx_size) RGData3a.WQEntry.offset -= FLEXI_APP_REQ_SIZE;
                        RGData3a.WQEntry.length = FLEXI_APP_REQ_SIZE / 64;
                        DBG_(SPLIT_RGP_DEBUG, ( << "\tRUNNING IN FLEXI MODE with FLEXI_APP_REQ_SIZE = " << FLEXI_APP_REQ_SIZE
                                    << " and context size = " << ctx_size
                                    << "! This means that actually: "
                                    << "\n\tbuf_address = 0x" << std::hex << (uint64_t)RGData3a.WQEntry.buf_addr
                                    << "\n\toffset = " << std::dec << (uint64_t)RGData3a.WQEntry.offset
                                    << "\n\tlength = " << (uint64_t)RGData3a.WQEntry.length));
                    }

                    //Update WQ tail
                    QP_table_iter->second.WQ_tail++;
                    if (QP_table_iter->second.WQ_tail >= wqSize) {
                        QP_table_iter->second.WQ_tail = 0;
                        QP_table_iter->second.WQ_SR ^= 1;
                    }

                    MemoryTransport transport;
                    boost::intrusive_ptr<MemoryMessage> operation;
                    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                    operation = new MemoryMessage(MemoryMessage::WQMessage);
                    operation->op = RGData3a.WQEntry.op;
                    operation->nid = RGData3a.WQEntry.nid;
                    operation->cid = RGData3a.WQEntry.cid;
                    operation->offset = RGData3a.WQEntry.offset;
                    operation->length = RGData3a.WQEntry.length;
                    operation->buf_addr = RGData3a.WQEntry.buf_addr;
                    operation->QP_id = RGData3a.QP_id;
                    operation->WQ_idx = RGData3a.WQ_tail;
                    if (cfg.SingleRMCDispatch && RGData3a.WQEntry.op == RMC_RECV) {
                        operation->target_backend = getRMCDispatcherID_LocalMachine();
                        DBG_(MESSAGING_DEBUG, ( << "RGP Frontend " << RMCid << " found new valid RECV request in WQ. SingleRMCDispatch option dictates forwarding request to RGP Backend "
                                    << getRMCDispatcherID_LocalMachine() ));
                    } else operation->target_backend = -1;

                    //tracker->setAddress(  PhysicalMemoryAddress(1) );		//bogus
                    tracker->setInitiator(flexusIndex());
                    tracker->setSource("RMC");
                    tracker->setOS(false);
                    transport.set(TransactionTrackerTag, tracker);
                    transport.set(MemoryMessageTag, operation);

                    DBG_(SPLIT_RGP_DEBUG, ( << " RGP-" << RMCid << " Frontend pushing WQMessage out."));
                    //DBG_(Tmp, ( << " RGP-" << RMCid << " Frontend pushing WQMessage out: " << *transport[MemoryMessageTag]));
                    FLEXUS_CHANNEL( MemoryOut_Request ) << transport;

                    outstandingFETransfers++; //+=RGData3a.WQEntry.length;
                    RGPipFE.inputFree = true;
#ifndef SYNC_OPERATION
                    QPidFreelist.push_back(RGData3a.QP_id);			//Allow RGP stage 0 to poll again on same WQ
#endif
                }
#ifdef FLOW_CONTROL_STATS
                else {
                    DBG_(FC_DBG_LVL, ( << "FLOW CONTROL STATS: RGP-" << RMCid << " Frontend has request to push to backend, but there are too many outstanding requests already ("
                                << OUTSTANDING_FE_TRANSFERS_LIMIT << " requests outstanding)."));
                }
#endif
            } else {        //Not a valid WQ entry
                QPidFreelist.push_back(RGData3a.QP_id);			//Allow RGP stage 0 to poll again on same WQ
                DBG_( RGP_FRONTEND_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 3: WQEntry does not contain a valid entry"));
                RGPipFE.inputFree = true;
#ifdef FLOW_CONTROL_STATS
                DBG_( Crit, ( << "FLOW CONTROL STATS: RGP frontend is spinning; WQEntry does not contain a valid entry (i.e., the RGP couldn't find useful work to schedule)"));
#endif
            }
        }
        //%%%%%%%%%

        //RGPip stage 2
        if ((!cfg.SplitRGP && RGPip[3].inputFree)
                || (cfg.SplitRGP && RGPipFE.inputFree)) {		//First, check for previous entries that may have completed
            std::list<RG2LQ_t>::iterator iter = RGData2.LoadQ.begin();
            while (iter != RGData2.LoadQ.end() && iter->completed == false) {
                iter++;
            }

            if (iter != RGData2.LoadQ.end()) {
                WQEntry_t tempWQentry;
                tempWQentry.op = (iter->WQEntry1 >> 58) & 0x3F;
                tempWQentry.SR = (iter->WQEntry1 >> 57) & 1;
                tempWQentry.valid = (iter->WQEntry1 >> 56) & 1;
                tempWQentry.buf_addr = (iter->WQEntry1 >> 14) & 0x3FFFFFFFFFF;
                tempWQentry.cid = (iter->WQEntry1 >> 10) & 0xF;
                tempWQentry.nid = iter->WQEntry1 & 0x3FF;
                tempWQentry.offset = (iter->WQEntry2 >> 24) & 0xFFFFFFFFFF;
                tempWQentry.length = iter->WQEntry2 & 0xFFFFFF;
                //%%%%%%%%	SPLIT-RMC STUFF
                if (cfg.SplitRGP) {				//Don't touch RGData3, as it's used by the backend pipeline, which is an independent pipeline in this case. Use RGData3a instead.
                    RGData3a.WQEntry.op = tempWQentry.op;
                    RGData3a.WQEntry.cid = tempWQentry.cid;
                    RGData3a.WQEntry.nid = tempWQentry.nid;
                    RGData3a.WQEntry.buf_addr = tempWQentry.buf_addr;
                    RGData3a.WQEntry.offset = tempWQentry.offset;
                    RGData3a.WQEntry.length = tempWQentry.length;
                    RGData3a.WQEntry.SR = tempWQentry.SR;
                    RGData3a.WQEntry.valid = tempWQentry.valid;
                    RGData3a.QP_id = iter->QP_id;
                    RGData3a.WQ_tail = iter->WQ_tail;
                    RGData3a.WQ_SR = iter->WQ_SR;
                    RGPipFE.inputFree = false;

                    DBG_( RGP_FRONTEND_DEBUG, ( << "RMC-" << RMCid << "'s RGP STAGE 2 (Frontend):Read head WQ entry.\n\top field value is: " << (int)RGData3a.WQEntry.op
                                << "\n\tnid field value is: " << (int)RGData3a.WQEntry.nid
                                << "\n\tcid field value is: " << (int)RGData3a.WQEntry.cid
                                << "\n\tbuf_address field value is: 0x" << std::hex << (uint64_t)RGData3a.WQEntry.buf_addr
                                << "\n\toffset field value is: 0x" << std::hex << (uint64_t)RGData3a.WQEntry.offset
                                << "\n\tlength field value is: " << std::dec << (uint64_t)RGData3a.WQEntry.length));
                } else {
                    //%%%%%%%%%
                    RGData3.WQEntry.op = tempWQentry.op;
                    RGData3.WQEntry.cid = tempWQentry.cid;
                    RGData3.WQEntry.nid = tempWQentry.nid;
                    RGData3.WQEntry.buf_addr = tempWQentry.buf_addr;
                    RGData3.WQEntry.offset = tempWQentry.offset;
                    RGData3.WQEntry.length = tempWQentry.length;
                    RGData3.WQEntry.SR = tempWQentry.SR;
                    RGData3.WQEntry.valid = tempWQentry.valid;
                    RGData3.QP_id = iter->QP_id;
                    RGData3.WQ_tail = iter->WQ_tail;
                    RGData3.WQ_SR = iter->WQ_SR;
                    RGPip[3].inputFree = false;
                }
                DBG_(RGP_FRONTEND_DEBUG, (<< "RMC-" << RMCid << "'s RGP STAGE 2 has a completed WQ entry read for PAddr 0x" << std::hex << iter->WQaddr
                            << ".\nThe fields read are: WQEntry1 = " << iter->WQEntry1 << ", WQEntry2 = " << iter->WQEntry2));
                RGData2.LoadQ.erase(iter);
            }
        }
        if (!RGPip[2].inputFree && !RGData2.full()) {		//Second, insert new entry that came from stage 1, if any
            RGData2.incData.completed = false;
            RGData2.LoadQ.push_back(RGData2.incData);

            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;
            boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
            operation = MemoryMessage::newLoad(PhysicalMemoryAddress(RGData2.incData.WQaddr), VirtualMemoryAddress(0xdeadbeef));
            operation->reqSize() = 8;
            tracker->setAddress( PhysicalMemoryAddress(RGData2.incData.WQaddr) );
            tracker->setInitiator(flexusIndex());
            tracker->setSource("RMC");
            tracker->setOS(false);
            transport.set(TransactionTrackerTag, tracker);
            transport.set(MemoryMessageTag, operation);

            aMAQEntry.theTransport = transport;
            aMAQEntry.type = WQRead;
            aMAQEntry.dispatched = false;
            aMAQEntry.thePAddr = RGData2.incData.WQaddr;
            aMAQEntry.theVAddr = 0xdeadbeef;
            aMAQEntry.bufAddrV = 222;	//bogus
            aMAQEntry.src_nid = RMCid;
            aMAQEntry.tid = 0;	//bogus
            aMAQEntry.offset = 222;	//bogus
            aMAQEntry.pl_size = 0;
            aMAQEntry.pid = _RGP;

            MAQ.push_back(aMAQEntry);
            DBG_(RGP_FRONTEND_DEBUG, (<< "RMC-" << RMCid << "'s RGP STAGE 2 received address for WQ entry poll (0x" << std::hex << RGData2.incData.WQaddr
                        << ") and sent a load request to MAQ. Transport enqueued: " << *(aMAQEntry.theTransport[MemoryMessageTag])));
            RGPip[2].inputFree = true;
        }
        //RGPip stage 1
        if (!RGPip[1].inputFree && RGPip[2].inputFree) {		//stall conditions
            RGData2.incData.WQaddr = RGData1.WQ_base + RGData1.WQ_tail * sizeof(wq_entry_t);
            RGData2.incData.QP_id = RGData1.QP_id;
            RGData2.incData.WQ_tail = RGData1.WQ_tail;
            RGData2.incData.WQ_SR = RGData1.WQ_SR;
            DBG_(RGP_FRONTEND_DEBUG, (<< "RMC-" << RMCid << "'s RGP STAGE 1 computed address for WQ entry poll: 0x" << std::hex << RGData2.incData.WQaddr));

            RGPip[1].inputFree = true;
            RGPip[2].inputFree = false;
        }
        //RGPip stage 0
        if (RGPip[1].inputFree && QPidFreelist.size() && theCycleCount % RGP_POLL_FREQ == 0 /*HOW OFTEN TO POLL!*/) {		//stall conditions
            //DBG_(Tmp, ( << "RMC-" << RMCid << " with free list begin id =" << *(QPidFreelist.begin()) ) );
            QP_table_iter = QP_table.find(*(QPidFreelist.begin()));
            DBG_Assert(QP_table_iter != QP_table.end());
            //NOTE: The following code block is used to only activate RGPX servicing QPidX ONLY and disable all other RMC actions
#ifdef SINGLE_RMC  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            uint32_t RGPx, QPidx = ACTIVE_RGP;//= 6;
            if (!cfg.SplitRGP) { //edge placement
                RGPx = 32;    //Set accordingly
            } else {
                RGPx = QPidx;
            }
            if (RMCid != RGPx) return;
            while (QPidFreelist.begin() != QPidFreelist.end() && *(QPidFreelist.begin()) != QPidx) QPidFreelist.pop_front();
            if (QPidFreelist.begin() == QPidFreelist.end()) return;
            DBG_Assert(*(QPidFreelist.begin()) == QPidx);
            QP_table_iter = QP_table.find(QPidx);
            if (QP_table_iter == QP_table.end()) return;
#endif    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            //code block end
            QPidFreelist.pop_front();
            RGData1.QP_id = QP_table_iter->first;
            RGData1.WQ_base = QP_table_iter->second.WQ_base;
            RGData1.WQ_tail = QP_table_iter->second.WQ_tail;
            RGData1.WQ_SR = QP_table_iter->second.WQ_SR;
            DBG_(RGP_FRONTEND_DEBUG, (<< "RMC-" << RMCid << "'s RGP stage 0 initiating new WQ read for QP_id " << RGData1.QP_id
                        << ", wq_base=0x" << std::hex << RGData1.WQ_base << std::dec << ", wq_tail=" << RGData1.WQ_tail
                        << ", wq_sr=" << (int)RGData1.WQ_SR));
            RGPip[1].inputFree = false;
        }
    }

    void driveRCP() {
        //RCPip stage 3
        std::tr1::unordered_map<uint64_t, CQ_buffer_t>::iterator iter;
        //%%%%%%%%	SPLIT-RMC STUFF
        if (cfg.SplitRGP && !hasFrontend) goto RCP_FRONTEND_LABEL;
        //%%%%%%%%
        iter = RCData3.theCQBuffers.begin();
        while (iter != RCData3.theCQBuffers.end()) {
            iter->second.cycle_counter++;
            if (iter->second.cycle_counter >= CQ_DELAY_LIMIT) {	//need to flush it to memory
                //FIXME: Need to check for dependencies with data store
                //DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: Need to flush buffer with base PAddr 0x" << std::hex << iter->first));

#ifdef TRACE_LATENCY
                DBG_(Dev, ( << "RMC-" << RMCid << "'s RCP STAGE 3: sending CQ write to memory " ));
#endif

                MemoryTransport transport;
                boost::intrusive_ptr<MemoryMessage> operation;
                boost::intrusive_ptr<TransactionTracker> tracker;
                tracker = new TransactionTracker;
                tracker->setInitiator(flexusIndex());
                tracker->setSource("RMC");
                tracker->setOS(false);
                //uint64_t pAddr = (iter->first) | (iter->second.first << 1);
                uint64_t pAddr = (iter->first) | (iter->second.first << 3);
                operation = MemoryMessage::newStore(PhysicalMemoryAddress(pAddr), VirtualMemoryAddress(0xdeadbeef), DataWord(iter->second.theEntries[iter->second.first]));
                DBG_(RCP_DEBUG, ( << "\tSending store to PAddr 0x" << std::hex << pAddr << " (first entry in CQ buffer: " << std::dec << iter->second.first
                            << ", #CQ entries to write: " << iter->second.entry_counter << ")"));
                operation->reqSize() = sizeof(cq_entry_t);
                tracker->setAddress( PhysicalMemoryAddress(pAddr) );
                transport.set(TransactionTrackerTag, tracker);
                transport.set(MemoryMessageTag, operation);

                aMAQEntry.theTransport = transport;
                aMAQEntry.type = CQWrite;
                aMAQEntry.dispatched = false;
                aMAQEntry.thePAddr = pAddr;
                aMAQEntry.theVAddr = 0xdeadbeef;
                aMAQEntry.bufAddrV = 0xdeadbeef;
                aMAQEntry.src_nid = RMCid;
                aMAQEntry.tid = RCData3.QP_id;		//using tid field here as QP_id
                aMAQEntry.offset = 0;		//offset
                aMAQEntry.pl_size = iter->second.entry_counter;		//Number of bytes that have to be written
                aMAQEntry.pid = _RCP;
                DBG_(RCP_DEBUG, ( << "\tCopying CQEntries from buffer to CQEntry:"));
                for (int i = iter->second.first; i < iter->second.first + iter->second.entry_counter; i++) {
                    aMAQEntry.CQEntries[i-iter->second.first] = iter->second.theEntries[i];
                    DBG_(RCP_DEBUG, ( << "\taMAQEntry.CQEntries[" << (i-iter->second.first) << "] = " << (std::bitset<64>)iter->second.theEntries[i]));
                }

                MAQ.push_back(aMAQEntry);

                RCData3.theCQBuffers.erase(iter);
            }
            iter++;
        }
        if (!RCPip[3].inputFree) {
            RCData3.PAddr = RCData3.CQ_base + RCData3.CQ_head * sizeof(cq_entry_t);
            if (RCData3.op == SEND_TO_SHARED_CQ) {  //RCData3.PAddr is a virtual address in this case
                tempAddr = sharedCQinfo[MACHINE_ID].translations.find(RCData3.PAddr & PAGE_BITS);
                DBG_Assert(tempAddr != sharedCQinfo[MACHINE_ID].translations.end(), ( << "RMC- " << RMCid << ": Should have found the translation of address 0x" << std::hex
                            << (RCData3.PAddr & PAGE_BITS) << " for the shared CQ") );
                RCData3.PAddr = tempAddr->second + RCData3.PAddr - (RCData3.PAddr & PAGE_BITS);
            }

            //Write CQ to cache - batching
            //uint16_t entry = ((uint16_t)RCData3.CQ_SR << 15) | RCData3.WQ_index;	//CQ entries became 2 bytes long since protocol v2.2
            uint64_t entry =   ((uint64_t)(((uint16_t)RCData3.CQ_SR << 15) | RCData3.WQ_index)) << 48;    //CQ entries became 8 bytes long since protocol v2.3

            if (RCData3.op == SEND || RCData3.op == SEND_TO_SHARED_CQ) {
                entry = entry | ((uint64_t)0x007F << 56);    //place 0x7F in the "success" field to indicate a SEND request
                entry = entry | RCData3.buf_addr;
                DBG_Assert((RCData3.buf_addr >> 48) == 0);
                DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: SEND transfer to be delivered to core via QPid " << RMCid
                            << ". Entry to be written in CQ address 0x" << std::hex << RCData3.PAddr << ": 0x" << entry ));
                DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: SEND transfer to be delivered to core via QPid " << RMCid
                            << ". Entry to be written in CQ address 0x" << std::hex << RCData3.PAddr << " (index: " << std::dec <<  RCData3.CQ_head
                            << "): 0x" << std::hex << entry ));
            } else if (RCData3.success) {
                entry = entry | ((uint64_t)0x0100 << 48);
                DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: Transfer with tid " << RCData3.WQ_index
                            << " completed successfully. CQ entry is 0x" << std::hex << entry));
            } else {
                entry = entry & ((uint64_t)0x80FF << 48); //Expose atomicity failure to app
                DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: Atomicity violation for Transfer with tid "
                            << RCData3.WQ_index << ". CQ entry is 0x" << std::hex << entry));
            }

            //uint64_t theAddr = (RCData3.PAddr >> 5) << 5;		//16 entries of 2 bytes each
            uint64_t theAddr = (RCData3.PAddr >> 3) << 3;		//8 entries of 8 bytes each

            iter = RCData3.theCQBuffers.find(theAddr);
            DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: SR is " << (int) RCData3.CQ_SR << ", WQ index is " << (std::bitset<8>)RCData3.WQ_index));
            DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: Will buffer CQ entry with value " << (std::bitset<64>)entry
                        << " in CQEntry (target PAddr: 0x" << std::hex << RCData3.PAddr    << ")" ));
            DBG_(RCP_BACKEND_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 3: Writing CQ " << RCData3.QP_id << "'s CQ head ("<< RCData3.CQ_head
                        <<"). Tid = " << RCData3.WQ_index << ".\n\tCQ entry with value " << (std::bitset<64>)entry
                        << " to target PAddr: 0x" << std::hex << RCData3.PAddr));
            if (iter != RCData3.theCQBuffers.end()) {
                //iter->second.theEntries[RCData3.PAddr & 0x1F] = entry;
                iter->second.theEntries[RCData3.PAddr & 7] = entry;
                iter->second.entry_counter++;
                DBG_(RCP_DEBUG, ( << "\tCQ entry written to buffer with base address 0x" << std::hex << iter->first
                            << ", position " << std::dec  <<(RCData3.PAddr & 0x07)));

                RCPip[3].inputFree = true;

            } else if (!RCData3.isFull()) {
                CQ_buffer_t aCQEntry;
                aCQEntry.cycle_counter = 0;
                aCQEntry.entry_counter = 1;
                //aCQEntry.first = (RCData3.PAddr & 0x1E) >> 1;	//CQ entries became 2 bytes long since protocol v2.2
                aCQEntry.first = (RCData3.PAddr & 0x38) >> 3;	//CQ entries became 8 bytes long since protocol v2.3
                aCQEntry.theEntries[aCQEntry.first] = entry;
                aCQEntry.QP_id = RCData3.QP_id;
                RCData3.theCQBuffers[theAddr] = aCQEntry;
                DBG_(RCP_DEBUG, ( << "\tCQ entry written to new buffer with base address 0x" << std::hex << theAddr
                            << ", position " << std::dec  <<aCQEntry.first));

                RCPip[3].inputFree = true;
            }
#ifdef FLOW_CONTROL_STATS
            else {
                DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RCP STAGE 3: CQBuffers are full. Stalling RCP."));
            }
#endif
        }

        //RCPip stage 2
        if (!RCPip[2].inputFree && RCPip[3].inputFree) {

            DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 2: Transaction completed. Need to write CQ_head" ));

            RCData3.WQ_index = RCData2.WQ_index;
            RCData3.QP_id = RCData2.QP_id;
            RCData3.success = RCData2.success;
            RCData3.op = RCData2.op;
            RCData3.buf_addr = RCData2.buf_addr;

            if (RCData2.op != SEND_TO_SHARED_CQ) {
                QP_table_iter = QP_table.find(RCData2.QP_id);
                DBG_Assert(QP_table_iter != QP_table.end());
                RCData3.CQ_base = QP_table_iter->second.CQ_base;
                RCData3.CQ_head = QP_table_iter->second.CQ_head;
                RCData3.CQ_SR = QP_table_iter->second.CQ_SR;

                QP_table_iter->second.CQ_head++;
                if (QP_table_iter->second.CQ_head >= cqSize) {
                    QP_table_iter->second.CQ_head = 0;
                    QP_table_iter->second.CQ_SR ^= 1;
                }
            } else {  //shared CQ case
                DBG_Assert(!sharedCQinfo[MACHINE_ID].translations.empty());
                RCData3.CQ_base = sharedCQinfo[MACHINE_ID].theQPEntry.CQ_base;
                RCData3.CQ_head = sharedCQinfo[MACHINE_ID].theQPEntry.CQ_head;
                RCData3.CQ_SR = sharedCQinfo[MACHINE_ID].theQPEntry.CQ_SR;
                DBG_(RCP_DEBUG, ( << "\tThis is a SEND_TO_SHARD_CQ; need to write to shared CQ with base VAddr 0x" << std::hex << RCData3.CQ_base << " and index " << std::dec << RCData3.CQ_head ));
                sharedCQinfo[MACHINE_ID].theQPEntry.CQ_head++;    //THIS IS A CHEAT! Normally, RCPs would have to communicate/synchronize for that. Benefitting the baseline
                if (sharedCQinfo[MACHINE_ID].theQPEntry.CQ_head >= MAX_NUM_SHARED_CQ) {
                    sharedCQinfo[MACHINE_ID].theQPEntry.CQ_head = 0;
                    sharedCQinfo[MACHINE_ID].theQPEntry.CQ_SR ^= 1;
                }
            }

            RCPip[3].inputFree = false;
            RCPip[2].inputFree = true;
        }

        //%%%%%%%%	SPLIT-RMC STUFF
        if (cfg.SplitRGP && hasFrontend) {		//Need to feed the RCP backend (fill out RCData3)
            std::list<MemoryTransport>::iterator RCPqIter = RCPinQueue.begin();
            if (RCPqIter != RCPinQueue.end() && RCPip[2].inputFree) {
                DBG_(SPLIT_RGP_DEBUG, ( << "RCP-" << RMCid << " Feeding RCP Backend with entry from RCPinQueue."));
                RCData2.QP_id = (*RCPqIter)[MemoryMessageTag]->QP_id;
                RCData2.WQ_index = (*RCPqIter)[MemoryMessageTag]->WQ_idx;
                RCData2.success = (*RCPqIter)[MemoryMessageTag]->success;
                RCData2.op = (*RCPqIter)[MemoryMessageTag]->op;

                if (RCData2.op == SEND || RCData2.op == SEND_TO_SHARED_CQ) {
                    RCData2.WQ_index = 0;
                    RCData2.buf_addr = (*RCPqIter)[MemoryMessageTag]->buf_addr;
#ifdef MESSAGING_STATS
                    uint64_t uniqueID = (*RCPqIter)[MemoryMessageTag]->offset;
                    REQUEST_TIMESTAMP[RMCid].push_back(uniqueID);
                    RequestTimerIter = RequestTimer.find(uniqueID);
                    DBG_Assert(RequestTimerIter != RequestTimer.end());
                    DBG_Assert(RequestTimerIter->second.core_id == RMCid);
                    RequestTimerIter->second.dispatch_time = theCycleCount - RequestTimerIter->second.dispatch_time;
                    RequestTimerIter->second.notification_time = theCycleCount;
                    DBG_(MESSAGING_DEBUG, (<< "RCP-" << RMCid << " received new " << (RemoteOp)(*RCPqIter)[MemoryMessageTag]->op << " request w/ VAddr 0x" << std::hex << RCData2.buf_addr
                                << ". Message's unique ID is " << std::dec << uniqueID));
#endif
                } else {
                    //stats
                    TotalCQTransfer[MACHINE_ID] += theCycleCount - CQstartTS[RCData2.QP_id];
                    CQstartTS[RCData2.QP_id] = theCycleCount;			//2nd time CQstartTS is set (for SplitRGP only)

                    //DBG_Assert(outstandingFETransfers >= ((*RCPqIter)[MemoryMessageTag]->length));
                    DBG_Assert(outstandingFETransfers > 0);
                    outstandingFETransfers--;   // -= ((*RCPqIter)[MemoryMessageTag]->length);	//decrement the number of outstanding transfers
                }
                RCPinQueue.erase(RCPqIter);
                RCPip[2].inputFree = false;

#ifdef SYNC_OPERATION
                QPidFreelist.push_back(RCData2.QP_id);			//Allow RGP stage 0 to poll again on same WQ
#endif
            }
            //%%%%%%%%
        } else {  //no split RMC
            if (!tidReadyList.empty()) {
                ITT_table_iter = ITT_table.find(tidReadyList.front());
                DBG_Assert(ITT_table_iter != ITT_table.end());

                RCData2.QP_id = ITT_table_iter->second.QP_id;
                RCData2.WQ_index = ITT_table_iter->second.WQ_index;
                RCData2.success = ITT_table_iter->second.success;

                RCPip[2].inputFree = false;
                //free tid entry
                ITT_table.erase(ITT_table_iter);
                tidFreelist.push_back(tidReadyList.front());
                tidReadyList.pop_front();
            }
        }
        //If splitRCP, this is the pipeline's splitting point!! Need to create one extra stage above, and one extra stage below
        //RCPip stage 1
        //%%%%%%%%	SPLIT-RMC STUFF
RCP_FRONTEND_LABEL:
        if (cfg.SplitRGP && !hasBackend) return;
        if (cfg.SplitRGP && FLEXUS_CHANNEL( MemoryOut_Snoop ).available()
                && !tidReadyList.empty()) {     //a CQ entry is ready to get written

            ITT_table_iter = ITT_table.find(tidReadyList.front());
            DBG_Assert(ITT_table_iter != ITT_table.end());

            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;
            boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
            operation = new MemoryMessage(MemoryMessage::CQMessage);
            operation->QP_id = ITT_table_iter->second.QP_id;
            operation->WQ_idx = ITT_table_iter->second.WQ_index;
            operation->success = ITT_table_iter->second.success;
            operation->length = ITT_table_iter->second.size;

            DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP Backend: Completed transfer with tid " << tidReadyList.front()
                        << " corresponding to QP " << operation->QP_id << ", WQ entry " << ITT_table_iter->second.WQ_index));

            //tracker->setAddress(  PhysicalMemoryAddress(1) );		//bogus
            tracker->setInitiator(flexusIndex());
            tracker->setSource("RMC");
            tracker->setOS(false);
            transport.set(TransactionTrackerTag, tracker);
            transport.set(MemoryMessageTag, operation);

            DBG_(SPLIT_RGP_DEBUG, ( << "RCP-" << RMCid << " RCP Frontend sending new CQMessage out."));
            CQstartTS[ITT_table_iter->second.QP_id] = theCycleCount;			//1st time CQstartTS is set (for SplitRGP only)
            FLEXUS_CHANNEL( MemoryOut_Snoop ) << transport;

            //free tid entry
            ITT_table.erase(ITT_table_iter);
            tidFreelist.push_back(tidReadyList.front());
            tidReadyList.pop_front();
        }
#ifdef FLOW_CONTROL_STATS
        else if (!FLEXUS_CHANNEL( MemoryOut_Snoop ).available()) {
            DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RCP Backend: Stalling because memory port is not available for CQMessage!"));
        }
#endif
        //%%%%%%%%
        //This stage does not push anything forward. Just writes back the data if the packet has payload.
        if (!RCPip[1].inputFree) {
            ITT_table_iter = ITT_table.find(RCData1.packet.tid);
            DBG_Assert(ITT_table_iter != ITT_table.end());

            if (!RCData1.packet.succeeded) {
                ITT_table_iter->second.success = false;
                DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 1: SABREREPLY is invalid (and thus carries no payload)."));
            }

#ifdef NO_DATA_WRITEBACK
            if (false) {
#else
                if (RCData1.packet.carriesPayload() && !cfg.OneSided) {
#endif
                    if (!cfg.SplitRGP) {
                        QP_table_iter = QP_table.find(ITT_table_iter->second.QP_id);
                        DBG_Assert(QP_table_iter != QP_table.end());
                        if (FLEXI_APP_REQ_SIZE != 0) {	//flexi_mode
                            DBG_Assert(QP_table_iter->second.lbuf_size);
                            RCData1.lbuf_addr = ITT_table_iter->second.lbuf_addr + (RCData1.packet.pkt_index * 64) % QP_table_iter->second.lbuf_size;
                        } else {
                            RCData1.lbuf_addr = ITT_table_iter->second.lbuf_addr + RCData1.packet.pkt_index * 64;
                        }
                    } else {//%%%%%%%%	SPLIT-RMC STUFF
                        if (FLEXI_APP_REQ_SIZE != 0) {    //flexi_mode
                            std::tr1::unordered_map<uint32_t, uint32_t>::iterator bufIter = bufferSizes.find(ITT_table_iter->second.QP_id);
                            DBG_Assert(bufIter != bufferSizes.end(), (<< "RMC-" << RMCid << " was looking for, but couldn't find, the associated buffer of QP_id " << ITT_table_iter->second.QP_id));
                            DBG_Assert(bufIter->second);
                            RCData1.lbuf_addr = ITT_table_iter->second.lbuf_addr + (RCData1.packet.pkt_index * 64) % bufIter->second;
                        } else {
                            RCData1.lbuf_addr = ITT_table_iter->second.lbuf_addr + RCData1.packet.pkt_index * 64;
                        }
                    }
                    tempAddr = bufferv2p[ITT_table_iter->second.QP_id].find(RCData1.lbuf_addr & PAGE_BITS);
                    DBG_Assert( tempAddr != bufferv2p[ITT_table_iter->second.QP_id].end(), ( << "RMC- " << RMCid << ": Should have found the translation of address 0x" << std::hex
                                << (RCData1.lbuf_addr & PAGE_BITS) << " for QP id " << std::dec << ITT_table_iter->second.QP_id) );

                    uint64_t pAddr = tempAddr->second + RCData1.lbuf_addr - (RCData1.lbuf_addr & PAGE_BITS);

                    bool pageWalk = probeTLB(RCData1.lbuf_addr);	//FIXME: This always returns false for now

                    handleTLB( RCData1.lbuf_addr, pAddr, BOGUS, _RCP);

                    //! FAKING CACHE-BLOCK-INTERFACE CACHE
                    MemoryTransport transport;
                    boost::intrusive_ptr<MemoryMessage> operation;
                    boost::intrusive_ptr<TransactionTracker> tracker;
                    tracker = new TransactionTracker;
                    tracker->setInitiator(flexusIndex());
                    tracker->setSource("RMC");
                    tracker->setOS(false);

                    operation = new MemoryMessage(MemoryMessage::DATA_STORE_OP, PhysicalMemoryAddress(pAddr), VirtualMemoryAddress(RCData1.lbuf_addr), DataWord(RCData1.packet.payload.data[0]));

                    operation->reqSize() = 8;
                    tracker->setAddress( PhysicalMemoryAddress(pAddr) );
                    transport.set(TransactionTrackerTag, tracker);
                    transport.set(MemoryMessageTag, operation);

                    aMAQEntry.theTransport = transport;
                    aMAQEntry.type = BufferWriteForReply;
                    aMAQEntry.ITTidx = RCData1.packet.tid;
                    aMAQEntry.dispatched = false;
                    aMAQEntry.thePAddr = pAddr;
                    aMAQEntry.theVAddr = RCData1.lbuf_addr;
                    aMAQEntry.bufAddrV = RCData1.packet.bufaddr;
                    aMAQEntry.src_nid = RCData1.packet.src_nid;
                    aMAQEntry.tid = ITT_table_iter->second.QP_id;        //using tid field here as QP_id
                    aMAQEntry.offset = RCData1.packet.offset;
                    aMAQEntry.pl_size = RCData1.packet.pl_size;
                    aMAQEntry.pid = _RCP;
                    aMAQEntry.packet.payload = RCData1.packet.payload;

                    switch(RCData1.packet.op ) {
                        case RRREPLY:
                            DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 1: Writing back data carried in RRREPLY to buffer with VAddr 0x"
                                        << std::hex << RCData1.lbuf_addr << " and PAddr 0x" << pAddr));
                            break;
                        case SABREREPLY:
                            DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 1: Writing back data carried in SABREREPLY to buffer with PAddr 0x"
                                        << std::hex << pAddr));
                            break;
                        default:
                            DBG_Assert(false);
                    }
                    MAQ.push_back(aMAQEntry);
                } else {    //no payload
                    ITT_table_iter->second.counter--;
                    DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 1: Updating ITT. Remaining packets for this transfer: " << ITT_table_iter->second.counter));
                    if (ITT_table_iter->second.counter == 0) {
                        DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 1: Transfer with tid " << RCData1.packet.tid << " completed. Need to write CQ."));
                        tidReadyList.push_back(RCData1.packet.tid);      //notify the RCP to write the CQ entry
                        if (ITT_table_iter->second.isSABRe) {
                            prefetchTokens = (prefetchTokens+1) % (PREFETCH_TOKENS+1);
                        }
                    }
                }

                RCPip[1].inputFree = true;
            }
            //RCPip stage 0
            if (!RCPip[0].inputFree && RCPip[1].inputFree ) {	//The NI directly feeds RCPip[0] and sets RCPip[0].inputFree to false
#ifdef TRACE_LATENCY
                DBG_(Dev, ( << "RMC-" << RMCid << "'s RCP received reply packet with packet index " << RCData0.packet.pkt_index << " to NI" ));
#endif
                DBG_Assert(RCData0.packet.isReply());
                ITT_table_iter = ITT_table.find(RCData0.packet.tid);
                DBG_Assert(ITT_table_iter != ITT_table.end());

                if (RCData0.packet.op == SABREREPLY) {
                    if (!RCData0.packet.succeeded) ITT_table_iter->second.success = false;
                } else {
                    RCData0.packet.succeeded = true;
                }
                RCData1.packet = RCData0.packet;

                DBG_(RCP_DEBUG, ( << "RMC-" << RMCid << "'s RCP STAGE 0: Got a "<< RCData0.packet.op << " with tid " << std::dec << (int) RCData0.packet.tid));

                //if (RCData0.packet.op != SABREREPLY)
                outstandingPackets--;

                RCPip[0].inputFree = true;
                RCPip[1].inputFree = false;
                //stats
                if (RCData0.packet.carriesPayload())
                    RCPNWByteCount += 75;
                else
                    RCPNWByteCount += 11;	//header size
            }
        }

        std::pair<uint32_t, uint32_t> dispatchDecision(int aQPid) {
            //three dispatch policies currently:
            //- SingleRMCDispatch: can send to ALL cores
            //- DataplaneEmulation: can send to only ONE core (QPid == core_id)
            //- else, distribute across cores on same row as RRPP
            uint32_t min_jobs = 1000000, targetCore = 0, core;
            if (cfg.ServiceTimeMeasurement) { //steer all load to single core
                DBG_Assert(cfg.SingleRMCDispatch);  //currently only support this dispatch mode for ServiceTimeMeasurement
                targetCore = local_rmc_indices.front();
                QP_table_iter = QP_table.find(targetCore);
                DBG_Assert(QP_table_iter != QP_table.end());
                min_jobs = QP_table_iter->second.cur_jobs;
                if (min_jobs < 2) {
                    DBG_(Verb, ( << "WARNING: This is in service time measurement mode and loaded core's work queue depth is " << min_jobs << " - may be underutilized!"));
                }
            } else if (cfg.SingleRMCDispatch) {
                size_t i;
                /* FIXME: Remove single-core dispatch wlays to core 0
                   for (i=0; i < CORES_PER_MACHINE; i++) {
                   core = (lastCoreUsed[RMCid] + i) % CORES_PER_MACHINE;
                   QP_table_iter = QP_table.find(core);
                //if (QP_table_iter != QP_table.end()) DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 7: Looking up core " << core << "'s load... it's " << QP_table_iter->second.cur_jobs));
                if (QP_table_iter != QP_table.end() && (uint32_t)QP_table_iter->second.cur_jobs < min_jobs) {
                min_jobs = QP_table_iter->second.cur_jobs;
                targetCore = core;
                }
                }*/
                targetCore = lastCoreUsed[RMCid];
                QP_table_iter = QP_table.find(targetCore);
                if (QP_table_iter != QP_table.end()) {
                    min_jobs = QP_table_iter->second.cur_jobs;
                } else {
                    DBG_Assert(false, ( << "Mark you broke it."));
                }
                DBG_Assert(RMCid == getRMCDispatcherID_LocalMachine(), ( << "RRPP " << RMCid << " is about to dispatch a SEND request, while operating in SingleRMCDispatch mode with RMCDispatcherID = " << getRMCDispatcherID_LocalMachine() )); 
            } else if (cfg.DataplaneEmulation) {      //essentially, this always returns identity (targetCore = QPid)
                targetCore = aQPid;
                QP_table_iter = QP_table.find(targetCore);
                DBG_Assert(QP_table_iter != QP_table.end());
                min_jobs = QP_table_iter->second.cur_jobs;
            } else {
                uint32_t i;
                for (i=0; i < CORES_PER_RMC; i++) {
                    core = RMCid + (lastCoreUsed[RMCid] + i) % CORES_PER_RMC;
                    QP_table_iter = QP_table.find(core);
                    //if (QP_table_iter != QP_table.end()) DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 7: Looking up core " << core << "'s load... it's " << QP_table_iter->second.cur_jobs));
                    if (QP_table_iter != QP_table.end() && (uint32_t)QP_table_iter->second.cur_jobs < min_jobs) {
                        min_jobs = QP_table_iter->second.cur_jobs;
                        targetCore = core;
                    }
                } 
                DBG_Assert(RMCid <= targetCore && RMCid + CORES_PER_RMC > targetCore, ( << "Target core does not map to same row as this RRPP!"));
            }
            return std::make_pair(targetCore, min_jobs);    
        }

        void driveRRPP() {
            //RRPPPip stage 7: check if it's time to dispatch a waiting SEND request to a core. Need to pick a CQ to notify a core, which will service the send
            int theQPid;
            for (theQPid = 0; theQPid < MAX_CORE_COUNT; theQPid++) {    //cycle through all CQWaitQueues of this RRPP 
                if (!CQWaitQueue[theQPid].empty() && FLEXUS_CHANNEL( MemoryOut_Snoop ).available()) {
                    DBG_Assert(!QP_table.empty());
                    std::pair<uint32_t, uint32_t> dispDec = dispatchDecision(theQPid);
                    uint32_t min_jobs = dispDec.second, targetCore = dispDec.first;
                    DBG_(VVerb, ( << "Best candidate for SEND waiting in CQWaitQueue " << theQPid
                                << " is core " << targetCore << " with " << min_jobs << " currently queued."));     

                    DBG_Assert(min_jobs != 1000000);
                    if (min_jobs < cfg.PerCoreReqQueue || (cfg.ServiceTimeMeasurement && min_jobs < 3)) {   //only send a new job to a core if it has 0 or 1 jobs enqueued (when it was 2, changed to 1)
                        std::list<MemoryTransport>::iterator CQWaitQueueIter = CQWaitQueue[theQPid].begin();    //take first waiting req
                        QP_table_iter = QP_table.find(targetCore);
                        DBG_Assert(QP_table_iter != QP_table.end());
                        QP_table_iter->second.cur_jobs++;

                        lastCoreUsed[RMCid] = targetCore;   //update last core used

#ifdef MESSAGING_STATS
                        RequestTimerIter = RequestTimer.find((*CQWaitQueueIter)[MemoryMessageTag]->offset);
                        DBG_Assert(RequestTimerIter != RequestTimer.end());
                        RequestTimerIter->second.core_id = targetCore;
                        //REQUEST_TIMESTAMP[targetCore].push_back(RequestTimerIter->second.uniqueID); //instead of this, carry unique ID in message (see below, operation->offset)
                        DBG_(MESSAGING_DEBUG, ( << "Assigning SEND msg with unique ID " <<  RequestTimerIter->second.uniqueID << " to core " << targetCore));
                        RequestTimerIter->second.queueing_time = theCycleCount - RequestTimerIter->second.queueing_time;
                        RequestTimerIter->second.dispatch_time = theCycleCount;

                        /*//DEBUG FOR LONG QUEUING LATENCIES
                          if (theCycleCount - RequestTimerIter->second.queueing_time > 10) {
                          DBG_(Tmp, (<< "RMC-" << RMCid << " Dispatching packet with uniqueID " << RequestTimerIter->first << " with queuing time " << theCycleCount - RequestTimerIter->second.start_time
                          << " to core " <<  targetCore << " (Queue occupancy: " << CQWaitQueue.size() << "). Status of other cores:"));  //pls!! what are the other cores doing??
                          core = RMCid;
                          while (core < RMCid + CORES_PER_RMC) {
                          if (core != targetCore) {
                          RequestTimerIter = RequestTimer.begin();
                          while (RequestTimerIter != RequestTimer.end()) {
                          if (RequestTimerIter->second.core_id == core) break;
                          RequestTimerIter++;
                          }
                          if (RequestTimerIter != RequestTimer.end()) {
                          DBG_(Tmp, ( << "\tCore " << core << " is currently processing a request that arrived on cycle " << RequestTimerIter->second.start_time << " --- "
                          << theCycleCount - RequestTimerIter->second.start_time << " cycles ago"));
                          }
                          }
                          core++;
                          }
                          }*/
                        //DBG_(Tmp, ( << "Pushing request with uniqueID " << targetCore << " to core " << RequestTimerIter->second.uniqueID << "'s' QP."))
#endif

                        boost::intrusive_ptr<MemoryMessage> operation;
                        MemoryTransport transport;
                        boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                        operation = new MemoryMessage(MemoryMessage::CQMessage);
                        operation->QP_id = targetCore;
                        operation->op = SEND;
                        operation->buf_addr = (*CQWaitQueueIter)[MemoryMessageTag]->buf_addr + 64; //+64 is important! Because the first 64B of the recv buffer are only used by the RMC
                        operation->success = true;
                        operation->length = (*CQWaitQueueIter)[MemoryMessageTag]->length;
                        operation->offset = RequestTimerIter->second.uniqueID;  //Piggyback uniqueID on offset field
                        tracker->setInitiator(flexusIndex());
                        tracker->setSource("RMC");
                        tracker->setOS(false);
                        transport.set(TransactionTrackerTag, tracker);
                        (*CQWaitQueueIter).set(MemoryMessageTag, operation);

                        FLEXUS_CHANNEL( MemoryOut_Snoop ) << (*CQWaitQueueIter);

                        DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 7: Received an RRPPFwd message. Picked QP " << operation->QP_id
                                    << " with load " << min_jobs << " to notify for completion of a SEND with VAddr = 0x" << std::hex << operation->buf_addr ));

                        CQWaitQueue[theQPid].erase(CQWaitQueueIter);
                    } else {
                        DBG_(VVerb, ( << "RMC-" << RMCid << "'s RRPP STAGE 7 has " << CQWaitQueue[theQPid].size() 
                                    << " jobs queued, but all corresponding cores have more than 1 jobs already waiting ("
                                    << min_jobs << " jobs waiting)"));
                    }
                }
            }
            if (!(theCycleCount%100000)) DBG_(PROGRESS_DEBUG, ( << "RRPP-"<< RMCid << " CQWaitQueue occupancy: " << totalCQWaitQueueSize()));
            //RRPPPip stage 6; only for the completion of send messages - this stage gets triggered by MAQ when all of the message's RecvBufUpdate have completed
            if (!RRPData6.empty() && FLEXUS_CHANNEL( MemoryOut_Snoop ).available()) {
                RMCPacket aPacket = *(RRPData6.begin());
                RRPData6.pop_front();
                DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 6: Completed SEND msg will be forwarded to a CQ"));
                DBG_(MESSAGING_DEBUG, ( << "\t total # of received SEND messages: " << SENDS_RECEIVED));
                MemoryTransport transport;
                boost::intrusive_ptr<MemoryMessage> operation;
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;

                tracker->setInitiator(flexusIndex());
                tracker->setSource("RMC");
                tracker->setOS(false);
                transport.set(TransactionTrackerTag, tracker);

                DBG_Assert(cfg.SplitRGP); //the following code for RRPPPip stage 6 currently assumes split RMC

                if (!sharedCQinfo[MACHINE_ID].translations.empty()) {     //single shared CQ for SENDs is used
                    //send this to this RMC's RCP
                    operation = new MemoryMessage(MemoryMessage::CQMessage);
                    operation->QP_id = RMCid; //send to this RMC's RCP
                    operation->op = SEND_TO_SHARED_CQ;
                    operation->buf_addr = aPacket.bufaddr;
                    operation->success = true;
                    operation->length = aPacket.SABRElength;
                    transport.set(MemoryMessageTag, operation);
                    DBG_(MESSAGING_DEBUG, ( << "\t USING SHARED CQ! Pushing CQMessage to RCP " << operation->QP_id ));
                    FLEXUS_CHANNEL( MemoryOut_Snoop ) << transport;
#ifdef MESSAGING_STATS
                    operation-> offset = aPacket.uniqueID;    //USE OFFSET FIELD AS UNIQUEID!!!
                    sharedCQUniqueIDlist.push_back(aPacket.uniqueID);
                    DBG_(MESSAGING_DEBUG, ( << "Pushing packet with unique ID " << aPacket.uniqueID << " to sharedCQUniqueIDlist "));
#endif
                } else {  //use private CQs for SENDs
                    if (cfg.SingleRMCDispatch && RMCid != getRMCDispatcherID_LocalMachine()) {
                        operation = new MemoryMessage(MemoryMessage::RRPPFwd);
                        operation->QP_id = getRMCDispatcherID_LocalMachine();
                        operation->op = SEND;
                        operation->buf_addr = aPacket.bufaddr;
                        operation->success = true;
                        operation->length = aPacket.SABRElength;
#ifdef MESSAGING_STATS
                        operation-> offset = aPacket.uniqueID;    //USE OFFSET FIELD AS UNIQUEID!!!
#endif
                        transport.set(MemoryMessageTag, operation);
                        DBG_(MESSAGING_DEBUG, ( << "\t forwarding RRPPFwd to RRPP " << operation->QP_id << " (which is the central dispatch coordinator), which will send it to the corresponding core" ));
                        FLEXUS_CHANNEL( MemoryOut_Snoop ) << transport;
                    } else if (QP_table.empty()) {    //No QP found in this RRPP's row
                        uint16_t theBitSet = ctxToBEs[aPacket.context];
                        DBG_Assert(theBitSet, ( << "No core with a CQ belonging to context " << aPacket.context << " found"));
                        int i = 0;
                        while (i < 16) {
                            if (theBitSet & 1) break;
                            theBitSet >>= 1;
                            i++;
                        }
                        operation = new MemoryMessage(MemoryMessage::RRPPFwd);
                        operation->QP_id = MACHINE_ID * CORES_PER_MACHINE + i * CORES_PER_RMC;
                        operation->op = SEND;
                        operation->buf_addr = aPacket.bufaddr;
                        operation->success = true;
                        operation->length = aPacket.SABRElength;
#ifdef MESSAGING_STATS
                        operation-> offset = aPacket.uniqueID;    //USE OFFSET FIELD AS UNIQUEID!!!
#endif
                        transport.set(MemoryMessageTag, operation);
                        DBG_(MESSAGING_DEBUG, ( << "\t forwarding RRPPFwd to RRPP " << operation->QP_id << ", which will send it to the corresponding core" ));
                        FLEXUS_CHANNEL( MemoryOut_Snoop ) << transport;
                    } else {  //push into CQWaitQueue
                        if (cfg.SingleRMCDispatch) {
                            DBG_Assert(RMCid == getRMCDispatcherID_LocalMachine(), (
                                        << "SingleRMCDispatch is enabled with single RMC set to " 
                                        << getRMCDispatcherID_LocalMachine() << ", but RMC " << RMCid 
                                        << " is authorized to dispatch SEND requests as well!"));
                        }
                        operation = new MemoryMessage(MemoryMessage::CQMessage);
                        //operation->QP_id = targetCore;  //will be determined in the next stage
                        operation->op = SEND;
                        operation->buf_addr = aPacket.bufaddr;
                        operation->success = true;
                        operation->length = aPacket.SABRElength;
#ifdef MESSAGING_STATS
                        operation-> offset = aPacket.uniqueID;    //USE OFFSET FIELD AS UNIQUEID!!!
#endif
                        transport.set(MemoryMessageTag, operation);
                        //if cfg.DataplaneEmulation, pick one queue at random! In all other cases, use queue 0
                        int theSelectedQP = 0;
                        if (cfg.DataplaneEmulation) {
                            DBG_Assert(!cfg.SingleRMCDispatch);
                            theSelectedQP = RMCid + rand() % CORES_PER_RMC;
                            DBG_(MESSAGING_DEBUG, ( << "\t Operating in dataplane mode; picked core " << theSelectedQP));
                        }
                        DBG_(MESSAGING_DEBUG, ( << "\t pushing CQMessage to CQWaitQueue[" << theSelectedQP << "]" ));
                        CQWaitQueue[theSelectedQP].push_back(transport);
                    }
                }
            }

#ifdef RRPP_FAKE_DATA_ACCESS
            if (memoryLoopback->ready() && FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REPLY).available()) {
                RRPData5.packet = memoryLoopback->dequeue();
                switch (RRPData5.packet.op) {
                    case RREAD: {
                                    RRPData5.packet.op = RRREPLY;
                                    networkDataPacketsPushed++;
                                    break;
                                }
                    case RWRITE: {
                                     RRPData5.packet.op = RWREPLY;
                                     break;
                                 }
                    case SABRE: {
                                    RRPData5.packet.op = SABREREPLY;
                                    networkDataPacketsPushed++;
                                    break;
                                }
                    default:
                                DBG_Assert(false, ( << "unknown operation in packet"));
                }
                RRPData5.packet.succeeded = true;
                RRPData5.packet.swapSrcDst();
                FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REPLY) << RRPData5.packet;
            }
            return;
#endif

            //RRPPip stage 5 -> create packet & shoot to NI
            if (!RRPPip[5].inputFree && FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REPLY).available()) {
                DBG_Assert(RRPData5.packet.isReply());
                FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REPLY) << RRPData5.packet;
                RRPPip[5].inputFree = true;
                //complete RRPP timing for the packet
                std::tr1::unordered_map<uint64_t, uint64_t>::iterator timerIter;
                uint64_t uniqueID;
                if (RRPData5.packet.op == SENDREPLY) {
                    uniqueID = pidMux(RRPData5.packet.pkt_index, RRPData5.packet.dst_nid, RRPData5.packet.tid, RRPData5.packet.RMCid);
                    timerIter = RRPPtimers.find(uniqueID);
                    DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 5: Looking for RRPPtimer w/ pkt_index " << RRPData5.packet.pkt_index
                                << ", src_nid " << RRPData5.packet.dst_nid <<  ", tid " << RRPData5.packet.tid << ". UniqueID: " << uniqueID));
                } else {
                    uniqueID = pidMux(RRPData5.packet.offset, RRPData5.packet.dst_nid, RRPData5.packet.tid, RRPData5.packet.RMCid);
                    timerIter = RRPPtimers.find(uniqueID);
                    DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 5: Looking for RRPPtimer w/ offset " << RRPData5.packet.offset
                                << ", src_nid " << RRPData5.packet.dst_nid <<  ", tid " << RRPData5.packet.tid << ". UniqueID: " << uniqueID));
                }
                DBG_Assert(timerIter != RRPPtimers.end());
                RRPPLatSum += (theCycleCount - timerIter->second);
                RRPPLatCounter++;
                RRPPtimers.erase(timerIter);
                uint64_t RRPPLatAvg = RRPPLatSum / RRPPLatCounter;
                FLEXUS_CHANNEL(RRPPLatency) << RRPPLatAvg;
                //stats
                if (RRPData5.packet.carriesPayload()) {
                    RRPPoutNWByteCount += 75;
                    networkDataPacketsPushed++;
                } else
                    RRPPoutNWByteCount += 11;	//header size
            }
#ifdef FLOW_CONTROL_STATS
            else if (!RRPPip[5].inputFree && !FLEXUS_CHANNEL_ARRAY(RMCPacketOut, REPLY).available()) {
                DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RRPP STAGE 5: Network is backpressuring!"));
            }
#endif

            //RRPPip stage 4 -> latch completed request
            if (RRPPip[5].inputFree ) {
                std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t>::iterator LSQiter;
                LSQiter =  RRPData3.LSQ.begin();
                while (LSQiter != RRPData3.LSQ.end()) {
                    if (LSQiter->second.completed) {
                        DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: reply arrived to LSQ."));
                        RRPData5.packet = LSQiter->second.packet;
                        RRPPip[5].inputFree = false;
                        if (RRPData5.packet.op == SABREREPLY) {
                            //Mark ATT entry, put in ATTpriolist
                            uint16_t ATTidx = LSQiter->second.ATTidx;
                            ATT[ATTidx].read_blocks++;
                            if (!ATT[ATTidx].lock_read) {	//this is the SABRE's lock
                                ATT[ATTidx].versionAcquire1TS = theCycleCount;
                                if (ENABLE_SABRE_PREFETCH) {    //ATTidx is likely still in priolist; first make sure it's not in there anymore
                                    std::list<uint16_t>::iterator ptr = ATTpriolist.begin();
                                    while (*ptr != ATTidx && ptr != ATTpriolist.end()) ptr++;
                                    if (ptr != ATTpriolist.end()) ATTpriolist.erase(ptr);
                                    //kill prefetches that haven't been scheduled
                                    prefetchMAQ.clear();
                                }
                                SABRElat[RMCid] = theCycleCount;    //start of critical section
                                ATT[ATTidx].lock_read = true;
                                ATT[ATTidx].version = 0;

#ifdef HARDWARE_LOCK_CHECK    //if lock is currently held, fail - lock location is defined by LOCK_LOCATION
                                if (LSQiter->second.packet.payload.data[LOCK_LOCATION/8] & LOCK_MASK) {
                                    ATT[ATTidx].valid = false;
                                    DBG_(Tmp, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: SABRe's first access finds LOCK already held by a writer; early abort for ATTidx "
                                                << ATTidx << "! (lock value = " << (int)(LSQiter->second.packet.payload.data[LOCK_LOCATION/8] & LOCK_MASK) <<")"));
                                }
#endif
                                for (int i=VERSION_LOCATION; i<VERSION_LOCATION+VERSION_LENGTH; i++) {
                                    ATT[ATTidx].version |= ((uint64_t)LSQiter->second.packet.payload.data[i] << 8*(7-i));
                                }
                                //DBG_(Tmp, (<< "Read header of object (LSQiter = " << LSQiter->first << "). Version is " << ATT[ATTidx].version << ", and following bytes are:"));
                                //int byte;
                                //for (byte = 8; byte < 12; byte++)
                                //    DBG_(Tmp, (<< "\t " << (int)LSQiter->second.packet.payload.data[byte]));
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Pushing ATTidx " << ATTidx << " in priolist"));

                                if (!ATT[ATTidx].ready) {
                                    ATT[ATTidx].ready = true;
                                    ATTpriolist.push_front(ATTidx);
                                }
                                if (ATT[ATTidx].valid) DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Read SABRe's version: " << ATT[ATTidx].version << " (ATTidx = " << ATTidx << ")"));
                            } else if (ATT[ATTidx].last_data_read)	{ //this is the SABRE's last access for the version check
                                DBG_Assert(!cfg.SABReStreamBuffers); //If SABRe stream buffers used, last validation stage not used
                                ATT[ATTidx].versionAcquire2TS = theCycleCount;
                                uint64_t theLat =  theCycleCount - SABRElat[RMCid];    //end of critical section
                                SABReCount[RMCid]++;
                                SABRElatTot[RMCid]+=theLat;
                                if (ATT[ATTidx].SABRE_size > 1) { //exception: single-block SABRes
                                    RRPData5.packet =  ATT[ATTidx].lastPacket;	//will reply to src with last data packet, which also indicates atomicity success
                                    RRPPDataBlocksOverhead[MACHINE_ID]++;                   //re-checking the version is just overhead; should not be accounted towards application's useful bandwidth
                                }
                                uint64_t new_version = 0;
#ifdef HARDWARE_LOCK_CHECK
                                if (LSQiter->second.packet.payload.data[LOCK_LOCATION/8] & LOCK_MASK) { //check lock again
                                    ATT[ATTidx].valid = false;
                                    DBG_(Tmp, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: SABRe's validation finds LOCK acquired by writer. Abort for ATTidx " << ATTidx << "!"));
                                } else
#endif
                                    if (ATT[ATTidx].SABRE_size > 1) { //check versions if this is not a single-block SABRe
                                        for (int i=VERSION_LOCATION; i<VERSION_LOCATION+VERSION_LENGTH; i++) {
                                            new_version |= ((uint64_t)LSQiter->second.packet.payload.data[i] << 8*(7-i));
                                        }
                                        DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Checking object version... Initial version: " << ATT[ATTidx].version
                                                    << ", new version: " << new_version));
                                        if (new_version != ATT[ATTidx].version) {	//atomic read fail
                                            ATT[ATTidx].valid = false;
                                            DBG_(Tmp, ( << "\tRMC-" << RMCid << "'s RRPP STAGE 4: Version mismatch - SABRe abort!"));
                                        }
                                    }
                                if (ATT[ATTidx].valid) DBG_(RRPP_DEBUG, ( << "\tRMC-" << RMCid << "'s RRPP STAGE 4: SABRe validated!"));

                                std::tr1::unordered_map<uint64_t, uint16_t>::iterator ATTptr = ATTtags.find(ATT[ATTidx].SABREkey);
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Erasing ATTidx " << ATTidx << ". "));
                                DBG_Assert(ATTptr != ATTtags.end(), ( << "ATT entry " << ATTidx << " has a SABREkey of " << ATT[ATTidx].SABREkey << " but couldn't find this entry in the ATTtags."));
                                ATTtags.erase(ATTptr);
                                ATTfreelist.push_back(ATTidx);	//Free ATT entry

                                //dump stats for this completed SABRe
                                SABReLatFile << "RRPP" << RMCid << "\tArrival: " << ATT[ATTidx].arrivalTS << "\tEnd: "
                                    << ATT[ATTidx].versionIssue2TS << "\tTotal: " <<  (ATT[ATTidx].versionIssue2TS-ATT[ATTidx].arrivalTS) << "\tOrigin: " << (int)RRPData5.packet.RMCid << "\n\t"
                                    << (ATT[ATTidx].versionAcquire1TS - ATT[ATTidx].versionIssue1TS) << "\t"
                                    << (ATT[ATTidx].dataUnrollEndTS - ATT[ATTidx].dataUnrollStartTS) << "\t"
                                    << (ATT[ATTidx].versionAcquire2TS - ATT[ATTidx].versionIssue2TS) << "\n";

                            } else if (ATT[ATTidx].SABRE_size == ATT[ATTidx].read_blocks) {	//this is the SABRE's last packet - don't send reply packet yet; first validate if still valid
                                ATT[ATTidx].dataUnrollEndTS = theCycleCount;
                                if (ATT[ATTidx].valid) {
                                    if (!cfg.SABReStreamBuffers) {
                                        ATT[ATTidx].last_data_read = true;
                                        ATT[ATTidx].lastPacket = LSQiter->second.packet;
                                        RRPPip[5].inputFree = true;

                                        DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Read SABRe's last data block for ATTidx " << ATTidx << ". Will now validate."));
                                        DBG_(SABRE_DEBUG, ( << "\tRMC-" << RMCid << "'s RRPP STAGE 4: Pushing ATTidx " << ATTidx << " in priolist"));

                                        if (!ATT[ATTidx].ready) {           //give priority
                                            ATT[ATTidx].ready = true;
                                            ATTpriolist.push_front(ATTidx);
                                        }
                                    } else {
                                        DBG_(RRPP_DEBUG, ( << "\tRMC-" << RMCid << "'s RRPP STAGE 4: SABRe completed succesfully!"));

                                        std::tr1::unordered_map<uint64_t, uint16_t>::iterator ATTptr = ATTtags.find(ATT[ATTidx].SABREkey);
                                        DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Erasing ATTidx " << ATTidx << ". "));
                                        DBG_Assert(ATTptr != ATTtags.end(), ( << "ATT entry " << ATTidx << " has a SABREkey of " << ATT[ATTidx].SABREkey << " but couldn't find this entry in the ATTtags."));
                                        ATTtags.erase(ATTptr);
                                        ATTfreelist.push_back(ATTidx);	//Free ATT entry
                                        if (cfg.SABReStreamBuffers) {
                                            uint32_t param = ATTidx;
                                            FLEXUS_CHANNEL( FreeStrBuf ) << param;
                                        }

                                        //dump stats for this completed SABRe
                                        SABReLatFile << "RRPP" << RMCid << "\tArrival: " << ATT[ATTidx].arrivalTS << "\tEnd: "
                                            << ATT[ATTidx].versionIssue2TS << "\tTotal: " <<  (ATT[ATTidx].versionIssue2TS-ATT[ATTidx].arrivalTS) << "\tOrigin: " << (int)RRPData5.packet.RMCid << "\n\t"
                                            << (ATT[ATTidx].versionAcquire1TS - ATT[ATTidx].versionIssue1TS) << "\t"
                                            << (ATT[ATTidx].dataUnrollEndTS - ATT[ATTidx].dataUnrollStartTS) << "\t"
                                            << (ATT[ATTidx].versionAcquire2TS - ATT[ATTidx].versionIssue2TS) << "\n";
                                    }
                                } else {		//If it's already not valid, don't bother to validate again
                                    std::tr1::unordered_map<uint64_t, uint16_t>::iterator ATTptr = ATTtags.find(ATT[ATTidx].SABREkey);
                                    DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Erasing ATTidx " << ATTidx << " before re-reading version. SABRe already invalid."));
                                    DBG_Assert(ATTptr != ATTtags.end(), ( << "ATT entry " << ATTidx << " has a SABREkey of " << ATT[ATTidx].SABREkey << " but couldn't find this entry in the ATTtags."));
                                    ATTtags.erase(ATTptr);
                                    ATTfreelist.push_back(ATTidx);	//Free ATT entry
                                    if (cfg.SABReStreamBuffers) {
                                        uint32_t param = ATTidx;
                                        FLEXUS_CHANNEL( FreeStrBuf ) << param;
                                    }
                                }
                            } else {	//normal unrolled SABRe request
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Read a SABRe's data block. Send to requester. ("
                                            << ATT[ATTidx].SABRE_size - ATT[ATTidx].read_blocks << " blocks remaining)."));
                            }
                            RRPData5.packet.succeeded = ATT[ATTidx].valid;
                            if (!RRPPip[5].inputFree)
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 4: Sending out packet "
                                            << ((ATT[ATTidx].read_blocks <= ATT[ATTidx].SABRE_size) ? ATT[ATTidx].read_blocks : ATT[ATTidx].SABRE_size)
                                            << "/" << ATT[ATTidx].SABRE_size << " for ATTidx " << ATTidx << "."));
                        } else { //not a SABRe
                            RRPData5.packet.succeeded = true;
                        }
                        //begin stats
                        TotalRRPLatency[MACHINE_ID] += theCycleCount - LSQiter->second.TS_scheduled;
                        RRPIssueLatency[MACHINE_ID] += LSQiter->second.TS_issued - LSQiter->second.TS_scheduled;
                        RRPBlockCount[MACHINE_ID]++;
                        perRRPPLatency += theCycleCount - LSQiter->second.TS_scheduled;
                        perRRPPIssueLatency += LSQiter->second.TS_issued - LSQiter->second.TS_scheduled;
                        perRRPPblockCount++;
                        //end stats
                        RRPData3.LSQ.erase(LSQiter);
                        break;
                    } else {
                        LSQiter++;
                    }
                }
            }
            //RRPPip stage 3 -> issue memory request
            if (!RRPPip[3].inputFree ) {
                DBG_Assert(!RRPData3.isFull());
                MemoryTransport transport;
                boost::intrusive_ptr<MemoryMessage> operation;
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                switch (RRPData3.packet.op) {
                    case RREAD: {
                                    RRPData3.packet.op = RRREPLY;
                                    aMAQEntry.type = ContextRead;
                                    //operation = MemoryMessage::newLoad(PhysicalMemoryAddress(RRPData1.PAddr), VirtualMemoryAddress(RRPData1.VAddr));
                                    operation = new MemoryMessage(MemoryMessage::DATA_LOAD_OP, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr));
                                    break;
                                }
                    case RWRITE: {
                                     RRPData3.packet.op = RWREPLY;
                                     aMAQEntry.type = ContextWrite;

                                     operation = new MemoryMessage(MemoryMessage::DATA_STORE_L1, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr), DataWord(RRPData3.packet.payload.data[0]));

                                     break;
                                 }
                    case SABRE: {
                                    RRPData3.packet.op = SABREREPLY;
                                    aMAQEntry.type = ContextRead;
                                    if (RRPData3.cacheMe)
                                        operation = new MemoryMessage(MemoryMessage::DATA_LOAD_L1, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr));
                                    else
                                        operation = new MemoryMessage(MemoryMessage::DATA_LOAD_OP, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr));
                                    DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 3: Handling a SABRe"));
                                    break;
                                }
                    case PREFETCH: {
                                       DBG_Assert(ENABLE_SABRE_PREFETCH);
                                       DBG_Assert(RRPData3.cacheMe);
                                       aMAQEntry.type = ContextPrefetch;     //Prefetch op
                                       //operation = new MemoryMessage(MemoryMessage::PrefetchReadAllocReq, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr));  //PrefetchReadAllocReq not supported...
                                       operation = new MemoryMessage(MemoryMessage::DATA_LOAD_L1, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr));
                                       DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 3: Handling a SABRe prefetch"));
                                       break;
                                   }
                    case SEND: {
                                   RRPData3.packet.op = SENDREPLY;
                                   aMAQEntry.type = RecvBufWrite;

                                   operation = new MemoryMessage(MemoryMessage::DATA_STORE_L1, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr), DataWord(0));
                                   break;
                               }
                    case RECV: {
                                   RRPData3.packet.op = RECVREPLY;
                                   aMAQEntry.type = SendBufFree;

                                   operation = new MemoryMessage(MemoryMessage::DATA_STORE_L1, PhysicalMemoryAddress(RRPData3.PAddr), VirtualMemoryAddress(RRPData3.VAddr), DataWord(RRPData3.packet.payload.data[0]));

                                   break;
                               }
                    default:
                               DBG_Assert(false, ( << "RRPP STAGE 3: unknown operation in packet (" << RRPData3.packet.op <<")"));
                }

                tracker->setInitiator(flexusIndex());
                tracker->setSource("RMC");
                tracker->setOS(false);
                operation->reqSize() = 8;
                tracker->setAddress( PhysicalMemoryAddress(RRPData3.PAddr) );
                DBG_Assert(RRPData3.PAddr);
                transport.set(TransactionTrackerTag, tracker);
                transport.set(MemoryMessageTag, operation);

                aMAQEntry.theTransport = transport;
                aMAQEntry.dispatched = false;
                aMAQEntry.thePAddr = RRPData3.PAddr;
                aMAQEntry.theVAddr = RRPData3.VAddr;
                aMAQEntry.bufAddrV = RRPData3.packet.bufaddr;
                aMAQEntry.src_nid = RRPData3.packet.src_nid;
                aMAQEntry.tid = RRPData3.packet.tid;
                aMAQEntry.offset = RRPData3.packet.offset;
                aMAQEntry.pl_size = RRPData3.packet.pl_size;
                aMAQEntry.pid = _RRPP;
                aMAQEntry.packet = RRPData3.packet;

                RRP_LSQ_entry_t anLSQEntry;
                anLSQEntry.packet = aMAQEntry.packet;
                anLSQEntry.completed = false;
                anLSQEntry.ATTidx = RRPData3.ATTidx;
                anLSQEntry.TS_scheduled = theCycleCount;
                anLSQEntry.counter = 0;

                if (!ATT[RRPData3.ATTidx].valid) {	//skip accesing memory; SABRe is already invalid
                    anLSQEntry.completed = true;
                } else {
                    if (ENABLE_SABRE_PREFETCH && aMAQEntry.type == ContextPrefetch)
                        prefetchMAQ.push_back(aMAQEntry);
                    else
                        MAQ.push_back(aMAQEntry);
                }
                DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 3: pushing a request of type " << aMAQEntry.type
                            << " for PAddr 0x" << std::hex << aMAQEntry.thePAddr << " to MAQ"));
                if (RRPData3.packet.op == SENDREPLY) {   //send is special. Need to also issue a RecvBufUpdate
                    uint64_t theLSQidx = pidMux(aMAQEntry.packet.pkt_index, aMAQEntry.packet.dst_nid, aMAQEntry.packet.tid, aMAQEntry.packet.RMCid);
                    DBG_Assert(RRPData3.LSQ.find(theLSQidx) == RRPData3.LSQ.end());
                    RRPData3.LSQ[theLSQidx] = anLSQEntry;

                    //now also issue a RecvBufUpdate
                    theLSQidx = pidMux(aMAQEntry.packet.SABRElength, aMAQEntry.packet.dst_nid, aMAQEntry.packet.tid, aMAQEntry.packet.RMCid);
                    if (RRPData3.LSQ.find(theLSQidx) == RRPData3.LSQ.end()) {
                        RRPData3.LSQ[theLSQidx] = anLSQEntry;
                        RRPData3.LSQ[theLSQidx].counter = 1;
                        aMAQEntry.type = RecvBufUpdate;
                        aMAQEntry.theVAddr = RRPData3.VAddr - aMAQEntry.packet.pkt_index * BLOCK_SIZE - BLOCK_SIZE;  //address of recv buf entry's metadata
                        tempAddr = recvbufv2p[MACHINE_ID].find(aMAQEntry.theVAddr & PAGE_BITS);
                        DBG_Assert( tempAddr != recvbufv2p[MACHINE_ID].end(), ( << "Should have found the translation: page 0x" << std::hex << (aMAQEntry.theVAddr & PAGE_BITS) << " addr 0x" << aMAQEntry.theVAddr ) );
                        aMAQEntry.thePAddr = tempAddr->second + aMAQEntry.theVAddr - (aMAQEntry.theVAddr & PAGE_BITS);

                        operation = new MemoryMessage(MemoryMessage::DATA_STORE_L1, PhysicalMemoryAddress(aMAQEntry.thePAddr), VirtualMemoryAddress(aMAQEntry.theVAddr), DataWord(RRPData3.packet.payload.data[0]));
                        tracker->setInitiator(flexusIndex());
                        tracker->setSource("RMC");
                        tracker->setOS(false);
                        operation->reqSize() = 8;
                        tracker->setAddress( PhysicalMemoryAddress(aMAQEntry.thePAddr) );
                        transport.set(TransactionTrackerTag, tracker);
                        transport.set(MemoryMessageTag, operation);

                        aMAQEntry.theTransport = transport;
                        aMAQEntry.packet.bufaddr = aMAQEntry.theVAddr; //this is info needed by RRPP STAGE 6
                        MAQ.push_back(aMAQEntry);
                        DBG_(RRPP_DEBUG, ( << "\t...enqueuing a " << aMAQEntry.type
                                    << " for PAddr 0x" << std::hex << aMAQEntry.thePAddr << " to MAQ as well (pkt_idx = " << RRPData3.packet.pkt_index << ")"));
                    } else {
                        DBG_(RRPP_DEBUG, ( << "\t...found an already queued RecvBufUpdate for this SEND msg. Incrementing counter to "
                                    << (RRPData3.LSQ[theLSQidx].counter+1) << " (pkt_idx = " << RRPData3.packet.pkt_index << ")"));
                        RRPData3.LSQ[theLSQidx].counter++;
                    }
                } else if (aMAQEntry.type != ContextPrefetch ) { //Prefetches don't need an LSQ entry
                    uint64_t theLSQidx = pidMux(aMAQEntry.packet.offset, aMAQEntry.packet.dst_nid, aMAQEntry.packet.tid, aMAQEntry.packet.RMCid);
                    DBG_Assert(RRPData3.LSQ.find(theLSQidx) == RRPData3.LSQ.end());
                    RRPData3.LSQ[theLSQidx] = anLSQEntry;
                }
                RRPPip[3].inputFree = true;
            }

            //RRPPip stage 2    -> translation
            if (!RRPPip[2].inputFree && RRPPip[3].inputFree && !RRPData3.isFull()) {

                RRPData3.VAddr = RRPData2.VAddr;
                RRPData3.ATTidx = RRPData2.ATTidx;
                RRPData3.cacheMe = RRPData2.cacheMe;
                RRPData3.packet = RRPData2.packet;
                if (RRPData3.packet.op == SEND) {
                    tempAddr = recvbufv2p[MACHINE_ID].find(RRPData3.VAddr & PAGE_BITS);
                    DBG_Assert( tempAddr != recvbufv2p[MACHINE_ID].end(), ( << "Should have found the translation: page 0x" << std::hex << (RRPData3.VAddr & PAGE_BITS) << " addr 0x" << RRPData3.VAddr ) );
                } else if (RRPData3.packet.op == RECV) {
                    tempAddr = sendbufv2p[MACHINE_ID].find(RRPData3.VAddr & PAGE_BITS);
                    DBG_Assert( tempAddr != sendbufv2p[MACHINE_ID].end(), ( << "Should have found the translation: page 0x" << std::hex << (RRPData3.VAddr & PAGE_BITS) << " addr 0x" << RRPData3.VAddr ) );
                } else {
                    tempAddr = contextv2p[MACHINE_ID].find(RRPData3.VAddr & PAGE_BITS);
                    DBG_Assert( tempAddr != contextv2p[MACHINE_ID].end(), ( << "Should have found the translation: page 0x" << std::hex << (RRPData3.VAddr & PAGE_BITS) << " addr 0x" << RRPData3.VAddr ) );
                }
                RRPData3.PAddr = tempAddr->second + RRPData3.VAddr - (RRPData3.VAddr & PAGE_BITS);
                bool pageWalk = probeTLB(RRPData3.VAddr);        //FIXME: This always returns false for now
                handleTLB(RRPData3.VAddr, RRPData3.PAddr, BOGUS, _RRPP);

                DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 2: Translation of VAddr 0x" << std::hex << RRPData3.VAddr << " is 0x" << RRPData3.PAddr));

                RRPPip[2].inputFree = true;
                RRPPip[3].inputFree = false;
            }
#ifdef FLOW_CONTROL_STATS
            else if (!RRPPip[2].inputFree && RRPPip[3].inputFree && RRPData3.isFull()) {
                DBG_(FC_DBG_LVL, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RRPP STAGE 3: Full LSQ. Backpressuring."));
            }
#endif

            //RRPPip stage 1 - Go through the ATT in priority order. If SABRe, unroll it. If normal op, push it forward and free ATT entry.
            if (!ATTpriolist.empty() && RRPPip[2].inputFree) {
                //pick entry in ATT list that has priority
                uint16_t ATTidx =  *(ATTpriolist.begin());
                ATTpriolist.pop_front();

                RRPData2.packet = ATT[ATTidx].packet;
                RRPData2.packet.swapSrcDst();   //PREPARE DESTINATION OF REPLY! src <-> dst
                RRPData2.ATTidx = ATTidx;
                RRPData2.cacheMe = false;
                RRPPip[2].inputFree = false;

                if (ATT[ATTidx].SABRE_size == 0 ) {	//Not a SABRe
#ifdef RREAD_IS_SABRE
                    DBG_Assert(false);
#endif
                    RRPData2.VAddr = ATT[ATTidx].base_Vaddr;
                    RRPData2.isSabre = false;
                    ATTfreelist.push_back(ATTidx);
                } else {//It's a SABRe
#ifdef SABRE_IS_RREAD
                    DBG_Assert(false);
#endif
                    RRPData2.packet.pkt_index = ATT[ATTidx].offset;
                    RRPData2.packet.bufaddr += (ATT[ATTidx].offset << 6);
                    RRPData2.packet.offset += (ATT[ATTidx].offset << 6);
                    RRPData2.isSabre = true;

                    if (cfg.SABReStreamBuffers) {
                        if (!ATT[ATTidx].offset) { //This is the first request for the lock acquirement. need to check for stream buffer availability
                            if (FLEXUS_CHANNEL( AllocateStrBuf ).available()) {
                                DBG_(RRPP_STREAM_BUFFERS, ( << "Stream buffer available to start new SABRe with ATTidx = " << ATTidx ));

                                //**** ADDRESS TRANSLATION - FUNCTIONAL ONLY - to allocate the stream buffer with a base address
                                tempAddr = contextv2p[MACHINE_ID].find(ATT[ATTidx].base_Vaddr & PAGE_BITS);
                                DBG_Assert( tempAddr != contextv2p[MACHINE_ID].end(), ( << "Should have found the translation: page 0x" << std::hex
                                            << (ATT[ATTidx].base_Vaddr & PAGE_BITS) << " addr 0x" << ATT[ATTidx].base_Vaddr ) );
                                uint64_t SABREbaseAddress = tempAddr->second + ATT[ATTidx].base_Vaddr - (ATT[ATTidx].base_Vaddr & PAGE_BITS);
                                handleTLB(ATT[ATTidx].base_Vaddr, SABREbaseAddress, BOGUS, _RRPP);
                                //*****
                                SABReEntry aSABReEntry((uint32_t)ATTidx, SABREbaseAddress, ATT[ATTidx].SABRE_size);
                                //uint32_t param = (uint32_t)ATTidx;
                                FLEXUS_CHANNEL( AllocateStrBuf ) << aSABReEntry;
                                RRPData2.VAddr = ATT[ATTidx].base_Vaddr;
                                ATT[ATTidx].offset++;
                                ATT[ATTidx].ready = true;
                                ATTpriolist.push_front(ATTidx);	//keep priority
                            } else {
                                RRPPip[2].inputFree = true;
                                DBG_(RRPP_STREAM_BUFFERS, ( << "Attempted to allocate a stream buffer for new SABRe with ATTidx, but there is no availability. Will retry." ));
                                ATTpriolist.push_back(ATTidx);	//retry later
                            };
                        } else {  //Normal unroll
                            uint32_t param = (uint32_t)ATTidx;
                            if (!ATT[ATTidx].lock_read) FLEXUS_CHANNEL( AllocateStrBufEntry ) << param;

                            if (ATT[ATTidx].lock_read || streamBufferAvailable) { //can start the SABRe
                                if (ATT[ATTidx].offset < ATT[ATTidx].req_counter) {
                                    RRPData2.VAddr = ATT[ATTidx].base_Vaddr + (ATT[ATTidx].offset << 6);
                                    ATT[ATTidx].offset++;
                                    DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Unrolling SABRe w/ ATTidx " << ATTidx << ": offset = " << ATT[ATTidx].offset
                                                << ", VAddr = 0x" << std::hex << RRPData2.VAddr));
                                    //DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Pushing ATTidx " << ATTidx << " in priolist"));
                                    ATTpriolist.push_front(ATTidx);	//keep priority
                                    DBG_Assert(ATT[ATTidx].ready);
                                } else {
                                    ATT[ATTidx].ready = false;	//waiting for new request packets
                                    RRPPip[2].inputFree = true;
                                }
                                streamBufferAvailable = false;
                            } else {
                                DBG_(RRPP_STREAM_BUFFERS, ( << "Stream buffer asssigned to SABRe with ATTidx " << ATTidx << " is full - stalling this SABRe"));
                                RRPPip[2].inputFree = true;
                                ATTpriolist.push_back(ATTidx);	//retry later
                            }
                        }
                    } else { //if (!cfg.SABReStreamBuffers)
                        //For the first two cases below: remember to add them back in priority list when they complete
                        if (ATT[ATTidx].last_data_read) {	//send request for object's version to validate atomicity
                            ATT[ATTidx].versionIssue2TS = theCycleCount;
                            RRPData2.VAddr = ATT[ATTidx].base_Vaddr;
                            DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Sending last load to validate SABRe (ATTidx " << ATTidx << "). VAddr = 0x" << std::hex << RRPData2.VAddr));
                            ATT[ATTidx].ready = false;
                        } else if (!ATT[ATTidx].offset) { //This is the first request for the lock acquirement
                            ATT[ATTidx].versionIssue1TS = theCycleCount;
                            RRPData2.VAddr = ATT[ATTidx].base_Vaddr;
                            RRPData2.cacheMe = true;
                            DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Sending first SABRe load to read object version (ATTidx " << ATTidx << "). VAddr = 0x" << std::hex << RRPData2.VAddr));
                            ATT[ATTidx].offset++;
                            ATT[ATTidx].ready = false;
                            if (ENABLE_SABRE_PREFETCH && ATT[ATTidx].SABRE_size <= SABRE_PREFETCH_LIMIT) {     //Only prefetch for relatively small transfers that are latency-sensitive
                                ATTpriolist.push_front(ATTidx);	//keep priority to continue with issuing prefetches
                                ATT[ATTidx].ready = true;
                            }
                        } else if (!ATT[ATTidx].lock_read) {    //waiting for lock; keep issuing prefetches for data to mask latency of reading lock
                            DBG_Assert(ENABLE_SABRE_PREFETCH);
                            if (ATT[ATTidx].prefetches_issued < std::min((uint32_t)SABRE_PREFETCH_LIMIT, (uint32_t)ATT[ATTidx].SABRE_size)) {
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Unrolling a prefetch for SABRe's block "
                                            << (ATT[ATTidx].prefetches_issued + 1) << "(ATTidx " << ATTidx << ")"));
                                RRPData2.VAddr = ATT[ATTidx].base_Vaddr + ((ATT[ATTidx].prefetches_issued+1) << 6) ;
                                RRPData2.packet.op = PREFETCH;
                                RRPData2.cacheMe = true;
                                ATT[ATTidx].prefetches_issued++;
                                if (ATT[ATTidx].prefetches_issued < std::min((uint32_t)SABRE_PREFETCH_LIMIT, (uint32_t)ATT[ATTidx].SABRE_size)) {
                                    ATTpriolist.push_front(ATTidx);	//keep priority to continue with issuing prefetches
                                    ATT[ATTidx].ready = true;
                                }
                            }
                        } else {  //normal unroll
                            if (!ATT[ATTidx].dataUnrollStartTS) ATT[ATTidx].dataUnrollStartTS = theCycleCount;
                            if (ATT[ATTidx].offset < ATT[ATTidx].req_counter) {
                                DBG_Assert(ATT[ATTidx].lock_read);
                                RRPData2.VAddr = ATT[ATTidx].base_Vaddr + (ATT[ATTidx].offset << 6);
                                ATT[ATTidx].offset++;
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Unrolling SABRe w/ ATTidx " << ATTidx << ": offset = " << ATT[ATTidx].offset
                                            << ", VAddr = 0x" << std::hex << RRPData2.VAddr));
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Pushing ATTidx " << ATTidx << " in priolist"));
                                ATTpriolist.push_front(ATTidx);	//keep priority
                                DBG_Assert(ATT[ATTidx].ready);
                            } else {
                                ATT[ATTidx].ready = false;	//waiting for new request packets
                                RRPPip[2].inputFree = true;
                            }
                        }
                    } //if !cfg.SABReStreamBuffers
                    if (!RRPPip[2].inputFree) DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 1: Pushing a SABRe packet to stage 2 (ATTidx " << ATTidx << "): offset "
                                << RRPData2.packet.offset << ", pkt idx  " << RRPData2.packet.pkt_index  <<  ", tid " << RRPData2.packet.tid));
                }
            }

            //RRPPip stage 0 - Compute base Vaddr + allocate ATT entry. ALEX: all requests allocate an ATT entry; easier to deal with priorities
            if (!RRPPip[0].inputFree && !ATTfreelist.empty()) {    //The NI directly feeds RRPPip[0] and sets RRPPip[0].inputFree to false
                DBG_Assert(RRPData0.packet.isRequest());
                uint64_t theSABREkey = 0;
                bool backpressure = false;
                if (RRPData0.packet.op == SABRE) {
                    theSABREkey = getSABReKey(RRPData0.packet.src_nid, RRPData0.packet.tid, RRPData0.packet.RMCid);
                    if (ATTtags.find(theSABREkey) == ATTtags.end() && ATTfreelist.empty()) { //SABRe cannot get allocated!
                        DBG_(RMC_BACKPRESSURE, ( << "No free ATT entry for new SABRe!"));
                        backpressure = true;
                    }
                }
                if (!backpressure) {
#ifdef TRACE_LATENCY
                    DBG_(Dev, ( << "RMC-" << RMCid << "'s RRPP received request packet with packet index " << RRPData0.packet.pkt_index ));
#endif
                    DBG_(RRPP_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 0: Got a " << RRPData0.packet.op << " with tid " << std::dec << (int) RRPData0.packet.tid
                                << " and offset 0x" << std::hex << RRPData0.packet.offset << " for context " << RRPData0.packet.context
                                << " from node " << RRPData0.packet.src_nid << "(packet index: " << RRPData0.packet.pkt_index << ")"));
                    if (RRPData0.packet.op == SABRE) {
                        DBG_(RRPP_DEBUG, ( << "\tThe above plus RMCid = " << (int)RRPData0.packet.RMCid << " correspond to key " << theSABREkey << " in the Lock table"));
                    }
                    //compute base VAddr
                    std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter;
                    cntxtIter = contextMap[MACHINE_ID].find(RRPData0.packet.context);

                    DBG_Assert(cntxtIter != contextMap[MACHINE_ID].end(), ( << "Request for access to unregistered context " << RRPData0.packet.context));

#ifndef SABRE_IS_RREAD
                    if (RRPData0.packet.op == SABRE) {
                        std::tr1::unordered_map<uint64_t, uint16_t>::iterator ATTptr = ATTtags.find(theSABREkey);
                        uint16_t ATTidx;
                        if (ATTptr == ATTtags.end()) {	//New SABRe; register
                            ATTidx =  *(ATTfreelist.begin());
                            ATTfreelist.pop_front();
                            ATTpriolist.push_back(ATTidx);	//keeping priorities to process in-order
                            ATT[ATTidx].SABRE_size = RRPData0.packet.SABRElength;
                            ATT[ATTidx].req_counter = 1;			//Will be incremented on arrival of each next packet of that SABRe
                            ATT[ATTidx].prefetches_issued = 0;
                            ATT[ATTidx].offset = 0;
                            ATT[ATTidx].read_blocks	= 0;
                            ATT[ATTidx].lock_read = false;
                            ATT[ATTidx].last_data_read = false;
                            ATT[ATTidx].valid = true;
                            ATT[ATTidx].ready = true;
                            ATT[ATTidx].base_Vaddr = cntxtIter->second.first + RRPData0.packet.offset;
                            ATT[ATTidx].packet = RRPData0.packet;
                            ATT[ATTidx].SABREkey = theSABREkey;
                            ATT[ATTidx].arrivalTS = theCycleCount;
                            ATT[ATTidx].versionIssue1TS = 0;
                            ATT[ATTidx].dataUnrollStartTS = 0;

                            //trick to deal w/ the cornercase of single-block SABRes
                            //the only difference vs. a normal RREAD is an additional check to verify if lock in header is free
                            if (RRPData0.packet.SABRElength == 1) {
                                ATT[ATTidx].last_data_read = true;
                                ATT[ATTidx].lock_read = true;
                            }

                            ATTtags[theSABREkey] = ATTidx;
                            DBG_Assert(RRPData0.packet.pkt_index == 0, (<< "This appears to be the first packet of a new SABRe, but the packet's pkt_index is " << RRPData0.packet.pkt_index << " (!= 0)"));
                            DBG_Assert(ATTtags.size() <= ATT_SIZE);
                            DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 0: Registering new SABRe in ATT with ATTidx " << ATTidx
                                        << ", SABRe length " << RRPData0.packet.SABRElength << ", and context offset " << RRPData0.packet.offset << "."));
                            DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 0: Pushing ATTidx " << ATTidx << " in priolist"));
                        } else {
                            ATTidx = ATTptr->second;
                            DBG_Assert(ATT[ATTidx].packet.tid == RRPData0.packet.tid
                                    && ATT[ATTidx].packet.offset < RRPData0.packet.offset
                                    && ATT[ATTidx].packet.RMCid == RRPData0.packet.RMCid
                                    && ATT[ATTidx].packet.src_nid == RRPData0.packet.src_nid);
                            ATT[ATTidx].req_counter++;
                            DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 0: Request belongs to already registered SABRe with ATTidx "
                                        << ATTptr->second << " (req_counter = " << ATT[ATTidx].req_counter <<")."));
                            DBG_Assert(ATT[ATTidx].SABRE_size >= ATT[ATTidx].req_counter, ( << "Expected SABRe size for ATT[" <<ATTidx<<"] is " << ATT[ATTidx].SABRE_size
                                        << ", but received request packet increased the request counter to " << ATT[ATTidx].req_counter ));
                            if (!ATT[ATTidx].ready && ATT[ATTidx].lock_read) {	//this entry was sleeping, waiting for more request packets. Wake it up!
                                DBG_Assert(!ATT[ATTidx].last_data_read);
                                ATTpriolist.push_back(ATTidx);
                                ATT[ATTidx].ready = true;
                                DBG_(SABRE_DEBUG, ( << "RMC-" << RMCid << "'s RRPP STAGE 0: Pushing ATTidx " << ATTidx << " in priolist. This SABRe had acquired lock and waited for new req packets!"));
                            }
                        }
                    } else {	//Normal request
#endif
                        uint16_t ATTidx =  *(ATTfreelist.begin());
                        ATTfreelist.pop_front();
                        ATTpriolist.push_back(ATTidx);	//keeping priorities to process in-order
                        ATT[ATTidx].ready = true;
                        ATT[ATTidx].SABRE_size = 0;	//means NO SABRE
                        ATT[ATTidx].packet = RRPData0.packet;
                        ATT[ATTidx].valid = true;

                        //for a SEND, data is not written in the context, but rather in the context's allocated RECV buffer.
                        //offset is index to the part of the context's RECV buffer that corresponds to the packet's src node
                        if (RRPData0.packet.op == SEND) {
                            ATT[ATTidx].base_Vaddr = messaging_table[RRPData0.packet.context].recv_buf_addr
                                + RRPData0.packet.src_nid * (messaging_table[RRPData0.packet.context].msg_entry_size + BLOCK_SIZE) * messaging_table[RRPData0.packet.context].msg_buf_entry_count //node offset
                                + RRPData0.packet.offset * (messaging_table[RRPData0.packet.context].msg_entry_size + BLOCK_SIZE) //this BLOCK_SIZE is the 1st entry of each
                                + (RRPData0.packet.pkt_index+1) * BLOCK_SIZE;
                            DBG_(RRPP_DEBUG, ( << "\tBase Vaddr corresponding to SEND request is 0x" << std::hex << ATT[ATTidx].base_Vaddr
                                        << "\nrecv buf base for node " <<  RRPData0.packet.src_nid  << ": 0x"
                                        << (RRPData0.packet.src_nid * (messaging_table[RRPData0.packet.context].msg_entry_size + BLOCK_SIZE)) * messaging_table[RRPData0.packet.context].msg_buf_entry_count
                                        << "\nmsg offset: 0x" << (RRPData0.packet.offset * (messaging_table[RRPData0.packet.context].msg_entry_size + BLOCK_SIZE))
                                        << "\npacket offset: 0x" << ((RRPData0.packet.pkt_index+1) * BLOCK_SIZE) ));
                        } else if (RRPData0.packet.op == RECV) { //for a RECV, the destination to write to is determined by the index (contained in )+ src node
                            ATT[ATTidx].base_Vaddr = messaging_table[RRPData0.packet.context].send_buf_addr
                                + RRPData0.packet.src_nid * messaging_table[RRPData0.packet.context].msg_buf_entry_count * sizeof(send_buf_entry_t) //node offset
                                + RRPData0.packet.bufaddr * sizeof(send_buf_entry_t);
                            DBG_(RRPP_DEBUG, ( << "\tVaddr corresponding to RECV request is 0x" << std::hex << ATT[ATTidx].base_Vaddr
                                        << "\nSend buf base for node " << RRPData0.packet.src_nid  << ": 0x"
                                        << RRPData0.packet.src_nid * messaging_table[RRPData0.packet.context].msg_buf_entry_count * sizeof(send_buf_entry_t)
                                        << "\nmsg offset: 0x" <<  RRPData0.packet.bufaddr * sizeof(send_buf_entry_t)
                                        << "\n(size of send_buf_entry_t: " << std::dec << sizeof(send_buf_entry_t) << " bytes)" ));
                        } else {
                            ATT[ATTidx].base_Vaddr = cntxtIter->second.first + RRPData0.packet.offset;
                            DBG_(RRPP_DEBUG, ( << "\tVaddr corresponding to context " << (uint32_t)RRPData0.packet.context << " is 0x" << std::hex << cntxtIter->second.first
                                        << " and target context virtual address: 0x" << ATT[ATTidx].base_Vaddr));
                        }
#ifndef SABRE_IS_RREAD
                    }
#endif
                    RRPPip[0].inputFree = true;
                    //stats
                    if (RRPData0.packet.carriesPayload())
                        RRPPincNWByteCount += 75;
                    else
                        RRPPincNWByteCount += 11;    //header size
                }
            }
#ifdef FLOW_CONTROL_STATS
            else if (!RRPPip[0].inputFree && ATTfreelist.empty()) {
                DBG_(Crit, ( << "FLOW CONTROL STATS: RMC-" << RMCid << "'s RRPP STAGE 0: No free ATT entry! Stalling"));
            }
#endif
        }

        inline uint64_t getSABReKey(uint16_t src, uint16_t tid, uint8_t rmc_id) {
            uint64_t ret = (((uint64_t)src << 50) | ((uint64_t)tid << 10) | rmc_id);
            //DBG_(Tmp, ( << "\tgetSABReKey = " << ret << ". (src_nid = " << (uint64_t)src << ", tid = " << (uint64_t)tid << ", RMCid = " << (uint64_t)RMCid << ")"));
            return ret;
        }

        //helper function for debugging
        void reverseSABReKey(uint64_t key) {
            uint16_t node, tid, rmc_id;
            rmc_id = key & 0xFF;
            tid = (key >> 10) & 0x3FF;
            node = (key >> 50) & 0x3FF;
            DBG_(SABRE_DEBUG, ( << "SABReKey " << key << " belongs to SABRe with tid " << tid << " originating from node " << node << "'s RMC " << rmc_id));
        }

        void drivePWalker() {	//the Page Walker unit		- FIXME: PTv2p needs an index
            /*		bool found = false;
                    uint64_t vAddr, theOffset, pAddr;
                    MAQiter = MAQ.begin();
                    while (pendingPagewalk && MAQiter != MAQ.end() && !found) {
                    if (MAQiter->completed) {
                    DBG_( RMC_DEBUG, ( << "RMC-" << RMCid << "'s RRPP has a completed Xaction to process:"));
                    switch (MAQiter->type) {
                    case Pagewalk1: {
                    DBG_( RMC_DEBUG, ( << "\t PageWalk 1 for Vaddr 0x" << std::hex << MAQiter->theVAddr));
                    vAddr = (uint64_t)QEMU_read_phys_memory_wrapper_dword(theCPU, MAQiter->thePAddr, 8);	//lvl2 base address
                    theOffset = (MAQiter->theVAddr & jMask) >> (PObits+PT_K);
                    tempAddr = 	PTv2p.find((vAddr + theOffset) & PAGE_BITS);
                    DBG_Assert( tempAddr != PTv2p.end(), ( << "Should have found the translation") );
                    pAddr = tempAddr->second + theOffset - (theOffset & PAGE_BITS);

            //Create a transport: LoadReq to address pAddr
            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;
            operation = MemoryMessage::newLoad(PhysicalMemoryAddress(pAddr), VirtualMemoryAddress(vAddr + theOffset));
            operation->reqSize() = 8;
            boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
            tracker->setAddress( PhysicalMemoryAddress(pAddr) );
            tracker->setInitiator(flexusIndex());
            tracker->setSource("RMC");
            tracker->setOS(false);
            transport.set(TransactionTrackerTag, tracker);
            transport.set(MemoryMessageTag, operation);

            aMAQEntry.theTransport = transport;
            aMAQEntry.type = Pagewalk2;
            aMAQEntry.dispatched = false;
            aMAQEntry.completed = false;
            aMAQEntry.thePAddr = pAddr;
            aMAQEntry.theVAddr = MAQiter->theVAddr;
            aMAQEntry.bufAddrV = MAQiter->bufAddrV;
            aMAQEntry.src_nid = RMCid;
            aMAQEntry.tid = MAQiter->tid;
            aMAQEntry.offset = 222;	//bogus
            aMAQEntry.pl_size = 0;
            aMAQEntry.pid = MAQiter->pid;

            MAQ.push_back(aMAQEntry);

            found = true;
            break;
            }
            case Pagewalk2: {
            DBG_( RMC_DEBUG, ( << "\t PageWalk 2"));
            vAddr = (uint64_t)QEMU_read_phys_memory_wrapper_dword(theCPU, MAQiter->thePAddr, 8);	//lvl3 base address
            theOffset = (MAQiter->theVAddr & kMask) >> PObits;
            tempAddr = 	PTv2p.find((vAddr + theOffset) & PAGE_BITS);
            DBG_Assert( tempAddr != PTv2p.end(), ( << "Should have found the translation") );
            pAddr = tempAddr->second + theOffset - (theOffset & PAGE_BITS);

            //Create a transport: LoadReq to address pAddr
            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;
            operation = MemoryMessage::newLoad(PhysicalMemoryAddress(pAddr), VirtualMemoryAddress(vAddr + theOffset));
            operation->reqSize() = 8;
            boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
            tracker->setAddress( PhysicalMemoryAddress(pAddr) );
            tracker->setInitiator(flexusIndex());
            tracker->setSource("RMC");
            tracker->setOS(false);
            transport.set(TransactionTrackerTag, tracker);
            transport.set(MemoryMessageTag, operation);

            aMAQEntry.theTransport = transport;
            aMAQEntry.type = Pagewalk3;
            aMAQEntry.dispatched = false;
            aMAQEntry.completed = false;
            aMAQEntry.thePAddr = pAddr;
            aMAQEntry.theVAddr = MAQiter->theVAddr;
            aMAQEntry.bufAddrV = MAQiter->bufAddrV;
            aMAQEntry.src_nid = RMCid;
            aMAQEntry.tid = MAQiter->tid;
            aMAQEntry.offset = 222;	//bogus
            aMAQEntry.pl_size = 0;
            aMAQEntry.pid = MAQiter->pid;

            MAQ.push_back(aMAQEntry);

            found = true;
            break;
        }
            case Pagewalk3: {
                                DBG_( RMC_DEBUG, ( << "\t PageWalk 3"));
                                DBG_( RMC_DEBUG, ( << " PageWalk done! Releasing entries..."));
                                pendingPagewalk = false;
                                while (!afterPWEntries.empty()) {
                                    DBG_(RMC_DEBUG, (<< "Action with tid " << (int) afterPWEntries.begin()->tid << " and type "
                                                << (int) afterPWEntries.begin()->type <<" moved to requestQueue"));
                                    MAQ.push_front(*(afterPWEntries.begin()));
                                    afterPWEntries.pop_front();
                                }
                                RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(MAQiter->theVAddr, 42 ));
                                if (RMC_TLB.size() > TLBsize) RMC_TLB.erase(RMC_TLB.begin());

                                found = true;
                                break;
                            }
            default: {}
        }
        }
        if (found) {
            DBG_( RMC_DEBUG, ( << "\t\tREMOVING the completed XAction of type " << prev->type << " from MAQ "));
            MAQ.erase(MAQiter);
        }
        else MAQiter++;
        } */
        }


        bool available( interface::RMCPacketIn const &, index_t anIndex) {
            if (anIndex == REQUEST) {
                return RRPPip[0].inputFree;
            } else {	//REPLY
                return RCPip[0].inputFree;
            }
        }
        void push( interface::RMCPacketIn const &, index_t anIndex, RMCPacket & thePacket) {
            DBG_Assert(isRMC);
            DBG_(RMC_DEBUG, (<< "RMC-"<<RMCid<<": Received packet originating from node " << (int) thePacket.src_nid
                        << " with tid " << (int) thePacket.tid));
            //DBG_(RMC_DEBUG, (<< "\tStarting address of context is 0x" << std::hex << cntxtIter->second << " packet offset is 0x" << std::hex
            //			<< thePacket.offset << " thus target vAddr is 0x" << std::hex << vAddr));
            if (thePacket.op == RREAD || thePacket.op == RMW || thePacket.op == SABRE || thePacket.op == RWRITE || thePacket.op == SEND) {
                uint64_t vAddr;
                std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter;

                cntxtIter = contextMap[MACHINE_ID].find(thePacket.context);
                DBG_Assert(cntxtIter != contextMap[MACHINE_ID].end(), (
                            << "RMC-" << RMCid << " received an access request to unregistered context. thePacket.context = " << thePacket.context
                            << ", MACHINE_ID = " << MACHINE_ID));
                vAddr = cntxtIter->second.first + thePacket.offset;
                if (thePacket.op == SEND) {
                    DBG_(RMC_DEBUG, ( << "\t Packet contains " << thePacket.op << " with offset: " << thePacket.offset
                                << ". Will write data to corresponding location in context " << thePacket.context << "'s RECV buffer."));

                } else {
                    DBG_(RMC_DEBUG, ( << "\t Packet contains " << thePacket.op << " with offset: " << thePacket.offset
                                << ". Will reply with data from buffer with vAddr 0x" << std::hex << vAddr ));
                }
            } else if (thePacket.op == RRREPLY) {
                DBG_(RMC_DEBUG, ( << "\t Packet contains RRREPLY with offset: " << thePacket.offset
                            << ". Will write data to local buffer with vAddr 0x" << std::hex <<  thePacket.bufaddr));
            }
            if (thePacket.op == RREAD || thePacket.op == RWRITE || thePacket.op == RMW || thePacket.op == SABRE || thePacket.op == SEND || thePacket.op == RECV) {
#ifdef RRPP_FAKE_DATA_ACCESS
                memoryLoopback->enqueue(thePacket, RRPP_RESPONSE_TIME);
                return;
#endif
                DBG_Assert(anIndex == REQUEST && RRPPip[0].inputFree);
                //inQueueReq.push_back(thePacket);
                RRPPip[0].inputFree = false;
                RRPData0.packet = thePacket;
                //initialize latency measurement for RRPP
                std::tr1::unordered_map<uint64_t, uint64_t>::iterator timerIter;
                uint64_t uniqueID;
                if (thePacket.op == SEND) {
                    uniqueID = pidMux(thePacket.pkt_index, thePacket.src_nid, thePacket.tid, thePacket.RMCid);
                    DBG_(RRPP_DEBUG, ( << "Registering RRPPtimer w/ pkt_index " << thePacket.pkt_index
                                << ", src_nid " << std::dec << thePacket.src_nid <<  ", tid " << thePacket.tid
                                << ", from RMCid " << (int) thePacket.RMCid << ". Unique ID = " << uniqueID));

                } else {
                    uniqueID = pidMux(thePacket.offset, thePacket.src_nid, thePacket.tid, thePacket.RMCid);
                    DBG_(RRPP_DEBUG, ( << "Registering RRPPtimer w/ offset 0x" << std::hex << thePacket.offset
                                << ", src_nid " << std::dec << thePacket.src_nid <<  ", tid " << thePacket.tid
                                << ", from RMCid " << (int) thePacket.RMCid << ". Unique ID = " << uniqueID));
                }
                DBG_Assert(RRPPtimers.find(uniqueID) == RRPPtimers.end());
                RRPPtimers[uniqueID] = theCycleCount;
#ifdef MESSAGING_STATS
                if (thePacket.op == SEND) {
                    uniqueID =  pidMux(thePacket.SABRElength, thePacket.src_nid, thePacket.tid, thePacket.RMCid);
                    DBG_(MESSAGING_DEBUG, ( << "RMC-" << RMCid 
                                << ": This is a SEND request. Assigning unique ID " << uniqueID
                                << ". Size of RequestTimer (i.e., outstanding SENDs): " << RequestTimer.size()));
                    RRPData0.packet.uniqueID = uniqueID;

                    RequestTimerIter = RequestTimer.find(uniqueID);
                    if (RequestTimerIter != RequestTimer.end()) { 
                        DBG_Assert(RequestTimerIter->second.packet_counter < RequestTimerIter->second.msg_size, 
                                ( << "Collision in uniqueID " << uniqueID << "! Shouldn't be possible")); 
                        DBG_Assert(RequestTimerIter->second.msg_size = thePacket.SABRElength);
                        RequestTimerIter->second.packet_counter++;
                        RequestTimerIter->second.queueing_time = theCycleCount;
                        RequestTimerIter->second.start_time = theCycleCount;
                    } else {    //create new entry
                        request_time_entry_t anEntry;
                        anEntry.queueing_time = theCycleCount;
                        anEntry.dispatch_time = 0;
                        anEntry.notification_time = 0;
                        anEntry.uniqueID = uniqueID;
                        anEntry.core_id = 1337;
                        anEntry.exec_time_synth = 0;
                        anEntry.exec_time_extra = 0;
                        anEntry.start_time = theCycleCount;
                        anEntry.msg_size = thePacket.SABRElength;
                        anEntry.packet_counter = 1;                   
                        RequestTimer[uniqueID] = anEntry;
                        SENDS_RECEIVED++; //stats
                    }
                }
#endif
            } else {
                DBG_Assert(anIndex == REPLY && RCPip[0].inputFree);
                //inQueueRep.push_back(thePacket);
                RCPip[0].inputFree = false;
                RCData0.packet = thePacket;
            }
        }

        inline uint64_t pidMux(uint64_t offset, uint16_t nid, uint16_t tid, uint8_t RMCid) {	//nid & RMCid identify a unique RMC source
            DBG_Assert((tid < 128)); 			//this is limited by TID_RANGE anyway (in RMC_SRAMS.hpp)
            DBG_Assert((nid < 1024)); //the protocol supports much more nodes, but it's unlikely to use more in the near future.
            DBG_Assert(!(offset >> 41));
            DBG_Assert(RMCid < MAX_CORE_COUNT);
            return ((offset << 23)+(RMCid << 17)+(nid << 7)+tid);        //to distinguish between same tids of different nids    -    WARNING: For up to 100 nodes (and overflow danger)
        }

#ifdef MESSAGING_STATS
        void dumpLatenciesToFile(request_time_entry_t anEntry) {
            if( anEntry.notification_time == 3735928559 || 
                    anEntry.exec_time_synth == 3735928559 ||
                    anEntry.exec_time_extra == 3735928559 ) {
                MessagingLatFile << std::setw(12) << anEntry.start_time << "\t"
                    << std::setw(12) << theCycleCount << "\t" << anEntry.core_id << "\t" 
                    << std::setw(7) << anEntry.queueing_time << "\t" 
                    << std::setw(7) << anEntry.dispatch_time << "\t"
                    << std::setw(7) << std::hex << 0xdeadbeef << std::dec << "\t" 
                    << std::setw(7) << std::hex << 0xdeadbeef << std::dec << "\t" 
                    << std::setw(7) << std::hex << 0xdeadbeef << std::dec << "\t" 
                    << std::setw(7) << std::hex << 0xdeadbeef << std::dec << "\t"
                    << std::setw(7) << theCycleCount - anEntry.start_time << "\n";

                // mark's parseable csv file
                MsgStatsCsv << anEntry.start_time << ";"
                    << theCycleCount << ";" << anEntry.core_id << ";" 
                    << anEntry.queueing_time << ";" 
                    << anEntry.dispatch_time << ";" 
                    << 3735928559 << ";" 
                    << 3735928559 << ";" 
                    <<  3735928559 << ";" 
                    << 3735928559 << ";"
                    << theCycleCount - anEntry.start_time << "\n";
            } else { // clean measurement
                MessagingLatFile << std::setw(12) << anEntry.start_time << "\t"
                    << std::setw(12) << theCycleCount << "\t" << anEntry.core_id << "\t" 
                    << std::setw(7) << anEntry.queueing_time << "\t" 
                    << std::setw(7) << anEntry.dispatch_time << "\t"
                    << std::setw(7) << anEntry.notification_time << "\t" 
                    << std::setw(7) << anEntry.exec_time_synth << "\t" 
                    << std::setw(7) << anEntry.exec_time_extra << "\t" 
                    << std::setw(7) << anEntry.dispatch_time + anEntry.notification_time + anEntry.exec_time_synth + anEntry.exec_time_extra << "\t"
                    << std::setw(7) << theCycleCount - anEntry.start_time << "\n";

                // mark's parseable csv file
                MsgStatsCsv << anEntry.start_time << ";"
                    << theCycleCount << ";" << anEntry.core_id << ";" 
                    << anEntry.queueing_time << ";" 
                    << anEntry.dispatch_time << ";" 
                    << anEntry.notification_time << ";" 
                    << anEntry.exec_time_synth << ";" 
                    << anEntry.exec_time_extra << ";" 
                    << anEntry.dispatch_time + anEntry.notification_time + anEntry.exec_time_synth + anEntry.exec_time_extra << ";"
                    << theCycleCount - anEntry.start_time << "\n";
            }
        }
#endif

        FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
        void push( interface::RequestIn const &, RMCEntry & anEntry) {		//BREAKPOINT HANDLING
            DBG_Assert(isRMC, (<< "This uArch is not an RMC. Shouldn't have received an RMC breakpoing"));
            if (fakeRMC) return;
            uint32_t cpu_id = anEntry.getCPUid();
            if (cfg.MachineCount > 1) {
                if (cpu_id != RMCid) return;
            } else {
                if ( (!cfg.SplitRGP && cpu_id / CORES_PER_RMC != RMCid / CORES_PER_RMC)
                        || (cfg.SplitRGP && cpu_id != RMCid)) {
                    DBG_( Trace, ( << "RMC-" << RMCid << " got magic breakpoint - Ignoring, as this is meant to be for RMC-" << ((cpu_id / CORES_PER_RMC)*CORES_PER_RMC) ));
                    return;
                }
            }
            //DBG_( RMC_DEBUG, ( << "RMC " << RMCid << " got magic breakpoint: new address translation registration request from the app" ));
            uint64_t address, size, type;

            type = anEntry.getType();
            address = anEntry.getAddress();
            size = anEntry.getSize();

            std::map<uint64_t, uint64_t> tempTranslation;
            uint64_t base_address = address & PAGE_BITS;

            DBG_( Trace, ( << "RMC-" << RMCid << " got a Breakpoint of type "<< type <<" from cpu-" << cpu_id
                        << ", with extra parameters: "<< address <<", " << size));
            if ( type == WQUEUE ||
                    type == CQUEUE ||
                    type == BUFFER ||
                    type == CONTEXT ||
                    type == CONTEXTMAP )
            {
                theCPU = API::QEMU_get_cpu_by_index(cpu_id);
                tempTranslation.clear();
                tempTranslation[base_address] = API::QEMU_logical_to_physical(theCPU, API::QEMU_DI_Data, base_address);
                API::QEMU_clear_exception();
                DBG_Assert(tempTranslation[base_address] != 0, ( << "Simics cannot translate virt2phys address for "
                            << br_points[type] << "type." ) );
            }

            switch (type) {
                case WQUEUE:
                case WQHEAD:
                case WQTAIL:
                case CQHEAD:
                case CQTAIL:
                case CQUEUE:
                case PTENTRY:
                case CONTEXT:
                case CONTEXTMAP:
                case TID_COUNTERS:
                    DBG_Assert( false, ( << "Should not receive this breakpoint in timing: " << type) );
                    break;
                case BUFFER:
                    {
                        DBG_( BREAKPOINTS_DEBUG, ( << "Entry type: BUFFER" ));
                        std::map<uint64_t, uint64_t>::iterator iter;
                        for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                            DBG_( BREAKPOINTS_DEBUG, ( << "Registering translation for Buffer. V: "<< std::hex<< iter->first
                                        << ", P: 0x"<< std::hex<<iter->second));
                            bufferv2p[cpu_id][iter->first] = iter->second;
                        }
                        if (size == 0) {
                            QP_table_iter = QP_table.find(cpu_id);
                            DBG_Assert( QP_table_iter != QP_table.end(), ( << "QP table entry is not found for cpu " << cpu_id) );
                            QP_table_entry_t aQPEntry = QP_table_iter->second;
                            aQPEntry.lbuf_base = address;
                            QP_table[cpu_id] = aQPEntry;
                            DBG_( BREAKPOINTS_DEBUG, ( << "Registering lbuf base address V: "<< std::hex<< address << " for CPU-" << std::dec << cpu_id));
                        }
                        break;
                    }
                case BUFFER_SIZE:
                    {
                        DBG_( BREAKPOINTS_DEBUG, ( << "Entry type: BUFFER_SIZE = " << size));
                        QP_table_iter = QP_table.find(cpu_id);
                        DBG_Assert( QP_table_iter != QP_table.end(), ( << "QP table entry is not found for cpu " << cpu_id) );
                        QP_table_entry_t aQPEntry = QP_table_iter->second;
                        aQPEntry.lbuf_size = size;
                        bufferSizes[cpu_id] = size;	//just for flexi apps, needed by RCP frontend
                        break;
                    }
                    /*
                       case CONTEXT:
                       {
                       DBG_( BREAKPOINTS_DEBUG, ( << "Entry type: CONTEXT REGISTRATION" ));
                       std::map<uint64_t, uint64_t>::iterator iter;
                       for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Registering translation for context. contextv2p["<< MACHINE_ID <<"][0x" << std::hex << iter->first
                       << "] = 0x"<< std::hex<<iter->second));
                       contextv2p[MACHINE_ID][iter->first] = iter->second;
                       }
                       break;
                       }
                       case CONTEXTMAP:
                       {
                       std::tr1::unordered_map<uint64_t, uint64_t>::iterator cntxtIter = contextMap[MACHINE_ID].find(size);
                       if (cntxtIter == contextMap[MACHINE_ID].end()) {
                       contextMap[MACHINE_ID][size] = tempTranslation.begin()->first;
                       DBG_( BREAKPOINTS_DEBUG, ( << "RMC(backend)-" << RMCid << ": Establishing context mapping: contextMap[" << MACHINE_ID << "][" << size
                       << "] = 0x" << std::hex << tempTranslation.begin()->first));
                       }
                       break;
                       }
                       */
                case ALL_SET:
                    DBG_(BREAKPOINTS_DEBUG, ( << "ALL_SET: Ignoring..."));
                    break;
                case BENCHMARK_END:
                    DBG_(Dev, ( << "CPU-"<<cpu_id<<" completed its execution. "));
                    API::QEMU_break_simulation("Benchmark end breakpoint ");
                    break;
                case NETPIPE_START:
                    DBG_(MESSAGING_DEBUG, ( << "CPU-" << cpu_id << ": Netpipe action started - op: 0x"<< std::hex<< address <<", count: " << std::dec << size));
                    NETPIPE_START_COUNT++;
                    //DBG_(BREAKPOINTS_DEBUG, ( << "\tglobal count: " << NETPIPE_START_COUNT));
                    //Idle time for this core is over!
                    IDLE_TIME[cpu_id] = theCycleCount - IDLE_TIME[cpu_id];
                    BUSY_TIME[cpu_id] = theCycleCount; 
                    DBG_(MESSAGING_DEBUG, ( << "Received NETPIPE_START(" << size << ", " << address << ") from core " << cpu_id << ". Signaling end of idle time. Idle time: " << IDLE_TIME[cpu_id] << " cycles."));
                    break;
                case NETPIPE_END:
                    DBG_(MESSAGING_DEBUG, ( << "CPU-" << cpu_id << ": Netpipe action completed - op: 0x"<< std::hex << address <<", count: " << std::dec << size));
                    NETPIPE_COMPLETE_COUNT++;
                    DBG_(BREAKPOINTS_DEBUG, ( << "\tglobal count: " << NETPIPE_COMPLETE_COUNT));
                    if (BUSY_TIME[cpu_id]) {
                        BUSY_TIME[cpu_id] = theCycleCount - BUSY_TIME[cpu_id]; 
                        coreOccupFile << std::setw(12) << theCycleCount << "\t" <<  std::setw(10) << NETPIPE_COMPLETE_COUNT << "\t" 
                            << std::setw(3) << cpu_id << "\t" << std::setw(10) << IDLE_TIME[cpu_id] << "\t" << std::setw(10) << BUSY_TIME[cpu_id] << "\n";
                    }
                    DBG_(MESSAGING_DEBUG, ( << "Received NETPIPE_END(" << size << ", " << address << ") from core " << cpu_id << ". Signaling end of busy time. Busy time: " << BUSY_TIME[cpu_id] << " cycles."));
                    IDLE_TIME[cpu_id] = theCycleCount;
                    if (NETPIPE_COMPLETE_COUNT == 50000) API::QEMU_break_simulation("50k requests done!");
                    //if (address == 999 || size == 1000) Simics::API::SIM_break_simulation("1K ops done!");
                    //if (address == 1)	Simics::API::SIM_break_simulation("Superstep 1 started!");	//for PageRank
                    break;
                case NEWWQENTRY:
                    DBG_(WQ_CQ_ACTIONS_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" enqueued a new WQ entry - Opcount = " << size << ", WQ index = " << address));
                    //DBG_(Tmp, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" enqueued a new WQ entry - Opcount = " << size << ", WQ index = " << address));
#ifndef TRACE_LATENCY
                    break;
#else
                    DBG_(BREAKPOINTS_DEBUG_VERB, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" enqueued a new WQ entry - Opcount = " << size << ", WQ index = " << address));
                    //stats
                    DBG_(Iface, ( << "RMC-"<< RMCid << ": Creating entry RTcycleCounter[" << (1000*cpu_id + address) << "] = " << theCycleCount));
                    cycleCounterIter = RTcycleCounter.find(1000*cpu_id + address);
                    if (cycleCounterIter != RTcycleCounter.end())
                        DBG_(BREAKPOINTS_DEBUG, ( << "WARNING: entry RTcycleCounter[" << (1000*cpu_id + address) << "] = " << cycleCounterIter->second
                                    << " already exists! Not sure how this can happen, ignore for now and hope it happens rarely"));
                    RTcycleCounter[1000*cpu_id + address] = theCycleCount;

                    //if (/*(cfg.SplitRGP && theCycleCount - WQstartTS[cpu_id] < 200) || (!cfg.SplitRGP &&*/( theCycleCount - WQstartTS[cpu_id] < 400)) {	//leaving out extreme values. Dunno how they occur...
                    TotalWQProcessing[MACHINE_ID] += theCycleCount - WQstartTS[cpu_id];
                    WQCount[MACHINE_ID]++;
                    //DBG_(Tmp, ( << "\tProcessing time for WQ enqueueuing: " << theCycleCount - WQstartTS[cpu_id]));
                    //} else {
                    //	E2EstartTS[cpu_id] = -1;
                    //}

                    WQstartTS[cpu_id] = theCycleCount;
                    break;
#endif
                case NEWWQENTRY_START:
                    WQstartTS[cpu_id] = theCycleCount;
                    E2EstartTS[cpu_id] = theCycleCount;
                    DBG_(BREAKPOINTS_DEBUG_VERB, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" started enqueuing a new WQ entry - Opcount = " << size << ", WQ index = " << address));
                    DBG_(WQ_CQ_ACTIONS_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" started enqueuing a new WQ entry - Opcount = " << size << ", WQ index = " << address));

                    //DBG_(Tmp, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" started enqueuing a new WQ entry - Opcount = " << size << ", WQ index = " << address));
                    break;
                case SENDREQRECEIVED:
                    DBG_(MESSAGING_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<< cpu_id<<" received new SEND request for recv buffer with address 0x" << std::hex << address));
                    DBG_(MESSAGING_DEBUG, ( << "\t Content of first 8 bytes of address are: " << size));

                    /*if (size) {
                      DBG_(MESSAGING_DEBUG, ( << "\tmsg received from within the app"));
                      } else {
                      DBG_(MESSAGING_DEBUG, ( << "\tmsg received from within the library"));
                      }*/
                    break;
                case SENDENQUEUED: {
                                       DBG_(MESSAGING_DEBUG, ( << "RMC-"<<RMCid << ": CPU-"<<cpu_id<<" just enqueued a new SEND request in send buf of ctx " << address << " with index " << size));
                                       //Make sure the entry in the send buffer is marked as valid.
                                       uint64_t sendBufAddrV = messaging_table[address].send_buf_addr + size * sizeof(send_buf_entry_t);
                                       DBG_(MESSAGING_DEBUG, (<< "Base VAddr of context " << address << "'s send buf is 0x" << std::hex << messaging_table[address].send_buf_addr ));
                                       tempAddr = sendbufv2p[MACHINE_ID].find(sendBufAddrV & PAGE_BITS);
                                       DBG_Assert( tempAddr != sendbufv2p[MACHINE_ID].end(), ( << "Should have found the translation of 0x" << std::hex << sendBufAddrV) );
                                       uint64_t sendBufAddrP = tempAddr->second + sendBufAddrV - (sendBufAddrV & PAGE_BITS);
                                       theCPU = API::QEMU_get_cpu_by_index(cpu_id);
                                       uint8_t validField = QEMU_read_phys_memory_wrapper_byte(theCPU, sendBufAddrP, 1);
                                       if (cfg.SelfCleanSENDs) { //this is a hack just for facilitating the RMCTrafficGenerator
                                           if (validField != 0) {
                                               API::QEMU_write_phys_memory(theCPU, sendBufAddrP, 0, 1);
                                           }
                                       } else {
                                           if (validField != 1) {
                                               DBG_(Crit, ( << "WARNING: Expected valid entry at Paddress 0x" << std::hex << sendBufAddrP << " but value was " << (int)validField << ". Setting from Simics."));
                                               API::QEMU_write_phys_memory(theCPU, sendBufAddrP, 1, 1);
                                           }
                                       }
                                       break;
                                   }
                case WQENTRYDONE:
                case SABRE_SUCCESS:
                case SABRE_ABORT:
                                   {
                                       DBG_(WQ_CQ_ACTIONS_DEBUG, ( << "RMC-"<< RMCid << ": WQ entry done - CPU-" << cpu_id << " confirms the completion of a remote operation. WQ index in CQ entry = "
                                                   << address <<", Opcount = " << size));

                                       if (type == SABRE_SUCCESS) {
                                           DBG_(BREAKPOINTS_DEBUG_VERB, ( << "\tRMC-"<< RMCid << ": successful completion of a SABRe for object with id " << address << "! (op_count: " << size << ")"));
                                           succeeded_SABRes++;
                                       } else if (type == SABRE_ABORT) {
                                           DBG_(Crit, ( << "\tRMC-"<< RMCid << ":aborted SABRe for object with id " << address << "! (value: " << size << ")"));
                                           failed_SABRes++;
                                       }
                                       //if (address == 997) DBG_(Tmp, (<< "CPU-" << RMCid << " Got in the critical section with shared CQ index " << size));
                                       //if (address == 998) DBG_(Tmp, (<< "CPU-" << RMCid << " Just read new SEND request at shared CQ index " << size));

#ifndef TRACE_LATENCY
                                       //DBG_(BREAKPOINTS_DEBUG_VERB, ( << "RMC-"<< RMCid << ": WQ entry done - CPU-" << cpu_id << " confirms the completion of a remote operation. WQ index in CQ entry = " << address <<", Opcount = " << size << "\n\tTotal E2E latency: " << opLatency << "\n"));
                                       break;
#endif //THE FOLLOWING WON'T BE EXECUTED IF NOT TRACING LATENCIES
                                       //stats
                                       uint64_t opLatency;
                                       cycleCounterIter = RTcycleCounter.find(1000*cpu_id + address);
                                       if (cycleCounterIter != RTcycleCounter.end()) {
                                           opLatency = theCycleCount - cycleCounterIter->second;
                                           RTcycleCounter.erase(cycleCounterIter);
                                           DBG_(RMC_DEBUG, ( << "RMC-"<< RMCid << ": Removing entry RTcycleCounter[" << (1000*cpu_id + address) << "]"));
                                       } else {
                                           opLatency = theCycleCount;
                                       }
                                       RTtransactionCounter++;
                                       if (opLatency < 10*RTlatencyAvg || RTlatencyAvg == 0) {
                                           RTlatSum += opLatency;
                                           if (opLatency > RTmaxLatNoExtremes) RTmaxLatNoExtremes = opLatency;
                                           if (opLatency < RTminLat) RTminLat = opLatency;
                                       }
                                       if (opLatency > RTmaxLat) {
                                           RTmaxLat = opLatency;
                                       }
                                       RTlatencyAvg = RTlatSum / RTtransactionCounter;
                                       DBG_(Dev, ( << "RMC-"<< RMCid << ": WQ entry done - CPU-" << cpu_id << " confirms the completion of a remote operation. WQ index in CQ entry = "
                                                   << address <<", Opcount = " << size << "\n Total E2E latency: " << opLatency << "\n"));
                                       //DBG_Assert(CQstartTS[cpu_id] > 0);
                                       //stats
                                       if (cfg.SplitRGP)
                                           TotalCQProcessing[MACHINE_ID] += theCycleCount - CQstartTS[cpu_id];
                                       else {
                                           //if (theCycleCount - CQstartTS[cpu_id] < 200) {			//exclude extreme cases
                                           TotalCQTransfer[MACHINE_ID] += theCycleCount - CQstartTS[cpu_id];
                                           CQTransferCount[MACHINE_ID]++;
                                           //DBG_(Tmp, ( << "RCP-" << RMCid << " CPU-" << cpu_id << " read a valid CQ entry. Latency since RMC write: " << theCycleCount - CQstartTS[cpu_id] << " cycles"));
                                           //} else {
                                           //	E2EstartTS[cpu_id] = -1;
                                           //}
                                       }
                                       CQCount[MACHINE_ID]++;
                                       CQCountPC[RMCid]++;
                                       uint32_t transferLat = theCycleCount-E2EstartTS[cpu_id];
#ifdef PRINT_PER_TRANSFER_LATENCY
                                       DBG_(Tmp, ( << "Completed transfer with opcount = " << size << " - E2E latency: " << transferLat));
#endif
                                       if (E2EstartTS[cpu_id] != -1) {
                                           if (!(transCount[MACHINE_ID] != 0 && (transferLat > 4*(TotalE2ELat[MACHINE_ID]/transCount[MACHINE_ID])))) {		//Ignoring exreme values
                                               TotalE2ELat[MACHINE_ID] += transferLat;
                                               transCount[MACHINE_ID]++;
                                           }
                                       }
                                       if (transferLat > maxE2Elat[MACHINE_ID]) maxE2Elat[MACHINE_ID] = transferLat;
                                       if (transferLat < minE2Elat[MACHINE_ID]) minE2Elat[MACHINE_ID] = transferLat;
                                       /*if (!print_freq) {
                                         DBG_( STATS_DEBUG, ( << "RMC-" << RMCid << "\n*********** ROUNDTRIP STATS (E2E) ************\n"
                                         << "transaction counter for this RMC: "
                                         << RTtransactionCounter << "\nLatency: " << opLatency
                                         << " cycles. \nAverage latency: " << RTlatencyAvg
                                         << " cycles. \nMin latency: " << RTminLat
                                         << " cycles. \nMax latency: " << RTmaxLat
                                         << " cycles. \nMax latency (no extremes): " << RTmaxLatNoExtremes
                                         << "\n**********************************************\n"));
                                         print_freq = STATS_PRINT_FREQ;
                                         }*/
                                       //if (size == 9999) Simics::API::SIM_break_simulation("10K ops done!");
                                       //if (size == 50000) Simics::API::SIM_break_simulation("50K ops done!");
                                       break;
                                   }
                case SIM_BREAK:
                                   DBG_(BREAKPOINTS_DEBUG, (<<"Ignore this breakpoint"));
                                   break;
                case CS_START:
                                   DBG_(BREAKPOINTS_DEBUG, ( << "CPU-" << cpu_id << " entered the critical section for object " << address ));
                                   CSlat[cpu_id] = theCycleCount;
                                   break;
                case OBJECT_WRITE:
                                   DBG_(BREAKPOINTS_DEBUG, ( << "CPU-" << cpu_id << " completed an object write. Object id: " << address <<", Opcount = " << size));
                                   object_writes++;
                                   if (CSlat[cpu_id]) {
                                       CSlatTot[cpu_id] += theCycleCount - CSlat[cpu_id];
                                       CSCount[cpu_id]++;
                                   }
                                   break;
                case LOCK_SPINNING:
                                   DBG_(BREAKPOINTS_DEBUG_VERB, ( <<" Magic break for RMC: CPU " << cpu_id << " is spinning on lock of object " << address << " (lock value = " << size << ")."));
                                   //DBG_(Tmp, ( <<" Magic break for RMC: CPU " << cpu_id << " is spinning on lock of object " << address << " (lock value = " << size << ")."));
                                   //if (theCycleCount % 1000 == 0)
                                   //{DBG_(BREAKPOINTS_DEBUG, ( <<" Magic break for RMC: CPU " << cpu_id << " is spinning on send queue (" << address << ", 0x" << std::hex << size << ")"));}
                                   break;
                case RMC_DEBUG_BP:
                                   DBG_(BREAKPOINTS_DEBUG, ( <<" Debug breakpoint for RMC by CPU " << cpu_id ));
                                   break;
                case MEASUREMENT:
                                   DBG_(BREAKPOINTS_DEBUG, ( << " Magic break: CPU " << cpu_id << " - measurement breakpoint with args: " << address << ", " << size ) );
#ifdef MESSAGING_STATS
                                   //IMPORTANT STATS FOR MESSAGING:
                                   switch(size){
                                       case 101: {
                                                     DBG_(MESSAGING_DEBUG, ( << "Received breakpoint " << 101 << " from core " << RMCid));
                                                     if (!sharedCQinfo[MACHINE_ID].translations.empty()) {
                                                         DBG_Assert(!sharedCQUniqueIDlist.empty());
                                                         uint64_t theUniqueID = sharedCQUniqueIDlist.front();
                                                         sharedCQUniqueIDlist.pop_front();
                                                         DBG_(MESSAGING_DEBUG, ( << "\tStarting processing of SEND request with uniqueID " << theUniqueID));
                                                         REQUEST_TIMESTAMP[RMCid].push_back(theUniqueID);
                                                         RequestTimerIter = RequestTimer.find(theUniqueID);
                                                         DBG_Assert(RequestTimerIter != RequestTimer.end());
                                                         RequestTimerIter->second.core_id = RMCid;
                                                     } else {
                                                         DBG_Assert(!REQUEST_TIMESTAMP[RMCid].empty());
                                                         RequestTimerIter = RequestTimer.find(*(REQUEST_TIMESTAMP[RMCid].begin()));
                                                         DBG_Assert(RequestTimerIter != RequestTimer.end());
                                                         DBG_Assert(RequestTimerIter->second.core_id == RMCid);
                                                     }
                                                     DBG_(MESSAGING_DEBUG, ( << "Started processing of message with unique ID " << (*(REQUEST_TIMESTAMP[RMCid].begin())) ));
                                                     DBG_Assert(RequestTimerIter->second.notification_time > 0);   //can't be zero
                                                     if( RequestTimerIter->second.notification_time == 0 ) {
                                                         /* Msutherl: If we encounter the missing breakpoint issue....
                                                          *    - please remove me and fix this!
                                                          */
                                                         RequestTimerIter->second.notification_time = 3735928559; // 0xbeefomuerto
                                                     } else {
                                                         RequestTimerIter->second.notification_time = theCycleCount - RequestTimerIter->second.notification_time;
                                                     }
                                                     RequestTimerIter->second.exec_time_synth = theCycleCount;
                                                     /*//DEBUG FOR LONG QUEUING LATENCIES
                                                       if (RequestTimerIter->second.queueing_time > 10000) {
                                                       DBG_(Tmp, (<< "CPU-" << RMCid << " packet with uniqueID " << RequestTimerIter->first << " starting execution. Queueing time: " << RequestTimerIter->second.queueing_time ));}*/
                                                     break;
                                                 }
                                       case 102: {
                                                     DBG_(MESSAGING_DEBUG, ( << "Received breakpoint " << 102 << " from core " << RMCid));
                                                     //The request finishes when the CPU posts a RECV, not now
                                                     DBG_Assert(!REQUEST_TIMESTAMP[RMCid].empty());
                                                     RequestTimerIter = RequestTimer.find(*(REQUEST_TIMESTAMP[RMCid].begin()));
                                                     DBG_Assert(RequestTimerIter != RequestTimer.end());
                                                     DBG_Assert(RequestTimerIter->second.core_id == RMCid);
                                                     /*//DEBUG FOR LONG QUEUING LATENCIES
                                                       if (RequestTimerIter->second.queueing_time > 10000) {
                                                       DBG_(Tmp, (<< "CPU-" << RMCid << " packet with uniqueID " << RequestTimerIter->first << " finished execution. Execution time: " << RequestTimerIter->second.execution_time
                                                       << ", Queueing time: " << RequestTimerIter->second.queueing_time << ". Arrived at cycle " << RequestTimerIter->second.start_time ));}*/
                                                     if( RequestTimerIter->second.exec_time_synth == 0 ) {
                                                         RequestTimerIter->second.exec_time_synth = 3735928559;
                                                     } else {
                                                         RequestTimerIter->second.exec_time_synth = theCycleCount - RequestTimerIter->second.exec_time_synth;
                                                     }
                                                     RequestTimerIter->second.exec_time_extra = theCycleCount;
                                                     DBG_(MESSAGING_DEBUG, ( << "\tCore " << RMCid << " finished processing SEND request with unique ID " << (*(REQUEST_TIMESTAMP[RMCid].begin())) ));
                                                     break;
                                                 }
                                   }
#endif
                                   break;
                case 29:
                                   DBG_(BREAKPOINTS_DEBUG, ( << " Magic break 29: CPU-" << cpu_id << " - measurement breakpoint with args: " << address << ", " << size) );
                                   //theCPU = API::QEMU_get_cpu_by_index(cpu_id);
                                   //Simics::API::SIM_write_register( theCPU , Simics::API::SIM_get_register_number ( theCPU , "l0") , address+size );
                                   break;
                case 30:
                                   if (size == 0xdeadbeef || size == 5) DBG_(SEND_LOCK_DEBUG, ( << "CPU-" << cpu_id << " spinning on send lock!!! " ) );

                                   if (!(printDebugCounter%100)) DBG_(BREAKPOINTS_DEBUG_VERB, ( << "CPU-" << cpu_id << " progress report (breakpoint 30): " << address << ", " << size  ) );
                                   printDebugCounter++;
                                   break;
                                   /*                switch(size) // ustiugov: here size stands for transfer's success (0, 2) or failure (1, 3) registered by the SW
                                                     {
                                                     case 0:
                                                     case 1:
                                                     DBG_(Tmp, ( << " Magic break 30: CPU " << cpu_id << " - reader measurement breakpoint with args: " << address << ", " << size ) );
                                                     break;
                                                     case 2:
                                                     case 3:
                                                     DBG_(Tmp, ( << " Magic break 30: CPU " << cpu_id << " - writer measurement breakpoint with args: " << address << ", " << size ) );
                                                     break;
                                                     default:
                                                     DBG_Assert( false, ( << "reader or writer? what a heck?" ));
                                                     break;
                                                     }*/
                                   break;
                case 42:
                                   DBG_(BREAKPOINTS_DEBUG, ( << "Received bp#" << type ) );
                                   break;
                default:
                                   DBG_Assert(false, ( <<"Unknown call type received by RMC: " << type));
                                   break;
            }
        }

        //ALEX - Magic interface of RMCs to cores' L1d caches
        FLEXUS_PORT_ALWAYS_AVAILABLE(MagicCacheAccessReply);
        void push( interface::MagicCacheAccessReply const &, bool & aReply) {
            DBG_Assert(!delayQueue->full());
            if (aReply) {DBG_(MAGIC_CONNECTIONS_DEBUG, ( << "Hit of Magic Access in the core's L1 cache!"));}
            else {DBG_(MAGIC_CONNECTIONS_DEBUG, ( << "MISS in the core's L1 cache!"));}
            if (aReply)	{	//L1 hit
                delayQueue->enqueue(theMagicTransport, 5);		//5 cycles for hit in core's L1
            } else {	//L1 miss
                delayQueue->enqueue(theMagicTransport, 5);		//here I assume that the RMC will never have to read a WQ from the LLC (always true for the microbenchmark)
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(MagicRMCCommunication);
        void push( interface::MagicRMCCommunication const &, MemoryTransport & aTransport) {
            DBG_(MAGIC_CONNECTIONS_DEBUG, Comp(*this) ( << " Enqueuing a prefetch on behalf of the RMC") );
            prefetches.push_back(aTransport);
        }

        //ALEX - For RRPP to check StreamBuffer availability
        FLEXUS_PORT_ALWAYS_AVAILABLE(StrBufEntryAvail);
        void push( interface::StrBufEntryAvail const &, bool & available) {
            streamBufferAvailable = available;
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(SABReInval);
        void push( interface::SABReInval const &, uint32_t & anATTidx) {
            DBG_Assert(cfg.SABReStreamBuffers && anATTidx < ATT_SIZE);
            DBG_(RRPP_STREAM_BUFFERS, ( << "ATOMICITY VIOLATION: Received an invalidation for SABRe with ATTidx = " << anATTidx ));
            ATT[anATTidx].valid = false;
            DBG_(RRPP_STREAM_BUFFERS, ( << "\t...Freeing corresponding stream buffer"));
            FLEXUS_CHANNEL( FreeStrBuf ) << anATTidx;
        }

        //ALEX - Incoming updates from LBAProducer about a core's start and end of an incoming SEND request's processing (emulated service times setup)
        FLEXUS_PORT_ALWAYS_AVAILABLE(CoreControlFlow);
        void push( interface::CoreControlFlow const &, uint32_t & aBreakpoint) {
        }

        bool probeTLB(uint64_t VAddr) {
            if (NO_PAGE_WALKS) return false;
            DBG_Assert(false, ( << "Haven't implemented pagewalks in this version yet!"));
            std::vector<std::pair<uint64_t, uint64_t> >::iterator TLBIter = RMC_TLB.begin();
            VAddr = VAddr & PAGE_BITS;
            while (TLBIter != RMC_TLB.end()) {
                if (TLBIter->first == VAddr) break;
                TLBIter++;
            }
            if (TLBIter == RMC_TLB.end()) {	//not found - have to trigger page walk
                DBG_(PW_DEBUG, ( << "\t Miss in the TLB"));
                return true;
            }
            DBG_(RMC_DEBUG, ( << "\t Hit in the TLB"));
            return false;
        }

        bool handleTLB(uint64_t VAddr, uint64_t PAddr, pendingOpType theType, pipelineId pid) {
            if (NO_PAGE_WALKS) return false;
            std::vector<std::pair<uint64_t, uint64_t> >::iterator TLBIter = RMC_TLB.begin();

            DBG_( RMC_DEBUG, ( << "RMC-" << RMCid << ": Checking RMC TLB for V->P translation of 0x" << std::hex << VAddr << "(page address 0x" << std::hex << (VAddr & PAGE_BITS) << ")"));

            VAddr = VAddr & PAGE_BITS;
            PAddr = PAddr & PAGE_BITS;

            while (TLBIter != RMC_TLB.end()) {
                if (TLBIter->first == VAddr) break;
                TLBIter++;
            }
            if (TLBIter == RMC_TLB.end()) {	//not found - trigger page walk
                DBG_Assert(!pendingPagewalk);
                triggerPageWalk(VAddr, theType, pid);
                return true;
            } else {		//found; update LRU
                DBG_( VVerb, ( << "Hit in RMC TLB!"));
                RMC_TLB.erase(TLBIter);
                RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(VAddr, PAddr));
            }
            return false;
        }

        void triggerPageWalk(uint64_t VAddr, pendingOpType theType, pipelineId pid) {
            DBG_( RMC_DEBUG, ( << "RMC-" << RMCid << ": Need to page walk for Vaddress 0x" << std::hex << VAddr));

            uint64_t lvl1index, lvl1offset;

            lvl1offset = (VAddr & iMask) >> (PObits+PT_J+PT_K);

            tempAddr = PTv2p[MACHINE_ID].find((PTbaseV[MACHINE_ID] + lvl1offset) & PAGE_BITS);
            DBG_Assert( tempAddr != PTv2p[MACHINE_ID].end(), ( << "Should have found the translation") );
            lvl1index = tempAddr->second + lvl1offset - (lvl1offset & PAGE_BITS);

            //Create a transport: LoadReq to address lvl1index
            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;
            operation = MemoryMessage::newLoad(PhysicalMemoryAddress(lvl1index), VirtualMemoryAddress(PTbaseV[MACHINE_ID] + lvl1offset));
            operation->reqSize() = 8;
            boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
            tracker->setAddress( PhysicalMemoryAddress(lvl1index) );
            tracker->setInitiator(flexusIndex());
            tracker->setSource("RMC");
            tracker->setOS(false);
            transport.set(TransactionTrackerTag, tracker);
            transport.set(MemoryMessageTag, operation);

            aMAQEntry.theTransport = transport;
            aMAQEntry.type = Pagewalk1;
            aMAQEntry.dispatched = false;
            aMAQEntry.thePAddr = lvl1index;
            aMAQEntry.theVAddr = VAddr;
            aMAQEntry.bufAddrV = 222;	//bogus
            aMAQEntry.src_nid = RMCid;
            aMAQEntry.tid = 0;	//bogus
            aMAQEntry.offset = 0;
            aMAQEntry.pl_size = 0;
            aMAQEntry.pid = pid;

            MAQ.push_back(aMAQEntry);
            DBG_( RMC_DEBUG, ( << "PageWalk start"));

            pendingPagewalk = true;
        }

        void loadState(std::string const & aDirName) {
            if (!isRMC || fakeRMC) {
                DBG_( Dev, ( << "Nothing to load" )  );
                return;
            }

            if (cfg.EnableSingleRMCLoading) {
                DBG_( Dev, ( << "Single RMC loading enabled. Will load state from common file." )  );
                loadStateSingleFile(aDirName);
                return;
            }

            if (MACHINES > 1 && !RMCid) API::QEMU_break_simulation("Simulation for more than one machine and without SingleRMCLoadingEnabled. Be warned; this hasn't been tested yet.");

            std::string fname(aDirName);
            fname += "/";

            if (loadRMCid < 10)
                fname += "0";
            fname += boost::lexical_cast<std::string>(loadRMCid) + "-RMC";

            if (cfg.RMCGZipFlexpoints) {
                fname += ".gz";
            }
            DBG_( Dev, ( << "Loading state of RMC-" << RMCid << " from file " << fname)  );

            std::ios_base::openmode omode = std::ios::in;
            if (!cfg.RMCTextFlexpoints) {
                omode |= std::ios::binary;
            }

            std::ifstream ifs(fname.c_str(), omode);
            if (! ifs.good()) {
                DBG_Assert( false, ( << " saved checkpoint state " << fname << " not found.  Resetting to cold RMC state " )  );
            } else {
                boost::iostreams::filtering_stream<boost::iostreams::input> in;
                if (cfg.RMCGZipFlexpoints) {
                    in.push(boost::iostreams::gzip_decompressor());
                }
                in.push(ifs);

                DBG_( Dev, ( << "Loading RMC state" ));
                //if (LoadFromSimics) {} //TODO
                if (cfg.RMCTextFlexpoints) {
                    char paren;
                    uint64_t key, value, size, size2;
                    uint32_t value32;
                    uint64_t i, j;

                    in >> paren >> value;
                    DBG_Assert(paren == '{', ( << "Error in loading state" ));
                    DBG_Assert(value == PAGE_BITS, ( << "Error in loading state - different page size" ));
                    in >> value;
                    DBG_Assert(value == PT_I, ( << "Error in loading state" ));
                    in >> value;
                    DBG_Assert(value == PT_J, ( << "Error in loading state" ));
                    in >> value >> paren;
                    DBG_Assert(value == PT_K, ( << "Error in loading state" ));

                    //Load RMC_TLB
                    DBG_( Dev, ( << "Now loading RMC TLB from file " << fname << "..." ));
                    in >> paren >> value;
                    if (value != TLBsize) DBG_(Dev, ( << "WARNING: The size of the TLB size in previous flexpoint is different than requested ("
                                << value << " instead of " << TLBsize << ")" ));
                    DBG_Assert(paren == '{', ( << "Error in loading state" ));

                    in >> size >> paren;
                    DBG_Assert(paren == '[', ( << "Error in loading state" ));
                    for (i = 0; i < size; i++) {
                        in >> paren >> key >> value >> paren;
                        DBG_Assert(paren == ')', ( << "Error in loading state" ));
                        RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(key, value));
                    }

                    in >> paren >> paren;

                    //PTv2p translations
                    for (i=0; i<MAX_CORE_COUNT; i++) {
                        in >> paren >> value >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        PTbaseV[i] = value;

                        for (j=0; i<size; j++) {
                            in >> paren >> key >> value >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            PTv2p[i][key] = value;
                        }
                        in >> paren >> paren;
                    }

                    //buffers
                    for (i=0; i<MAX_CORE_COUNT; i++) {
                        in >> paren >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> key >> value >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            bufferv2p[i][key] = value;
                        }
                        in >> paren >> paren;
                    }

                    //context
                    for (i=0; i<MAX_CORE_COUNT; i++) {
                        in >> paren >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> key >> value >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            contextv2p[i][key] = value;
                            DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading contextv2p["<<i<<"]["<< std::hex << key << "] = " << value));
                        }
                        in >> paren >> paren;
                    }

                    //contextMap
                    for (i=0; i<MAX_CORE_COUNT; i++) {
                        in >> paren >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> key >> value >> size2 >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            contextMap[i][key].first = value;
                            contextMap[i][key].second = size2;
                            DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading contextMap["<<i<<"]["<< std::hex << key<<"] = (" << value << ", " << size2 << ")"));
                        }
                        in >> paren >> paren;
                    }
                    DBG_Assert(paren == '}', ( << "Error in loading state" ));

                    RMCPacket aPacket;
                    uint64_t offset, bufaddr, qid, pkt_index;
                    uint16_t context, tid, src_nid, dst_nid;
                    RemoteOp op = RREAD;	//bogus initialization
                    aBlock payload;

                    in >> paren >> size2; 		//outQueue.size()
                    DBG_Assert(paren == '{', ( << "Error in loading state" ));
                    for (i=0; i<size2; i++) {
                        in >> paren >> qid >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> tid >> src_nid >> dst_nid >> context >> offset >> bufaddr >> pkt_index >> value;
                            DBG_Assert(paren == '(', ( << "Error in loading state" ));
                            switch (value) {
                                case 0:
                                    op = RREAD;
                                    break;
                                case 1:
                                    op = RWRITE;
                                    break;
                                case 2:
                                    op = RRREPLY;
                                    break;
                                case 3:
                                    op = RWREPLY;
                                    break;
                                default:
                                    DBG_Assert(false);
                            }
                            if (op == RWRITE || op == RRREPLY) {	//payload has useful info
                                for (uint32_t k = 0; k<64; k++) {
                                    in >> payload.data[k];
                                }
                                aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, payload, bufaddr, pkt_index, 64);
                            } else {
                                aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, bufaddr, pkt_index, 64);
                            }
                            //outQueueReq.push_back(aPacket);
                            DBG_Assert(false, ( << "There is no more outQueueRep in the RMC. This was moved to RMCConnector"));
                            in >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));

                        }
                        in >> paren >> paren;
                        DBG_Assert(paren == '}', ( << "Error in loading state" ));
                    }

                    in >> paren >> paren >> size2; 		//inQueue.size()
                    DBG_Assert(paren == '{', ( << "Error in loading state" ));
                    for (i=0; i<size2; i++) {
                        in >> paren >> qid >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> tid >> src_nid >> dst_nid >> context >> offset >> bufaddr >> pkt_index >> value;
                            DBG_Assert(paren == '(', ( << "Error in loading state" ));
                            switch (value) {
                                case 0:
                                    op = RREAD;
                                    break;
                                case 1:
                                    op = RWRITE;
                                    break;
                                case 2:
                                    op = RRREPLY;
                                    break;
                                case 3:
                                    op = RWREPLY;
                                    break;
                                default:
                                    DBG_Assert(false);
                            }
                            if (op == RWRITE || op == RRREPLY) {	//payload has useful info
                                for (uint32_t k = 0; k<64; k++) {
                                    in >> payload.data[k];
                                }
                                aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, payload, bufaddr, pkt_index, 64);
                            } else {
                                aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, bufaddr, pkt_index, 64);
                            }
                            //inQueueReq.push_back(aPacket);
                            DBG_Assert(false, ( << "There is no more inQueueReq in the RMC. This was moved to RMCConnector"));
                            in >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));

                        }
                        in >> paren >> paren;
                        DBG_Assert(paren == '}', ( << "Error in loading state" ));
                    }
                    in >> paren;
                    DBG_Assert(paren == '}', ( << "Error in loading state" ));

                    in >> value32;

                    //if (CORES == 64) {
                    //	DBG_Assert(value32-CORES == loadRMCid*CORES_PER_RMC, ( << "Error in loading state" ));
                    //} else {
                    DBG_Assert(value32-CORES == RMCid, ( << "Error in loading state" ));
                    //}

                    in >> value;
                    if (value == 0)
                        status = INIT;
                    else
                        status = READY;

                    in >> value;
                    uniqueTID = (uint8_t)value;
                    in >> value;
                    wqSize = value;
                    in >> value;
                    cqSize = value;

                    //Now load the SRAM structures
                    //QP table
                    in >> paren >> value;
                    QP_table_entry_t aQPEntry;
                    int idx;
                    uint8_t WQ_SR, CQ_SR;
                    uint64_t lbuf_base, lbuf_size;
                    for (i=0; i< value; i++) {
                        in >> paren >> idx >> aQPEntry.WQ_base >> aQPEntry.WQ_tail >> WQ_SR >> aQPEntry.CQ_base
                            >> aQPEntry.CQ_head >> CQ_SR >> lbuf_base >> lbuf_size >> paren;
                        aQPEntry.WQ_SR = WQ_SR;
                        aQPEntry.CQ_SR = CQ_SR;
                        aQPEntry.lbuf_base = lbuf_base;
                        aQPEntry.lbuf_size = lbuf_size;
                        aQPEntry.cur_jobs = 0;
                        QP_table[idx] = aQPEntry;
                        QPidFreelist.push_back(idx);
                        if (FLEXI_APP_REQ_SIZE) {
                            DBG_(Dev, ( << "NOTE: Running in FLEXI APP mode with FLEXI_APP_REQ_SIZE = " << FLEXI_APP_REQ_SIZE));
                        }
                    }
                    in >> paren;

                    //ITT table
                    in >> paren >> value;
                    ITT_table_entry_t anITTEntry;
                    for (i=0; i< value; i++) {
                        in >> paren >> idx >> anITTEntry.counter >> anITTEntry.QP_id >> anITTEntry.WQ_index >> anITTEntry.lbuf_addr >> paren;
                        ITT_table[idx] = anITTEntry;
                    }
                    in >> paren;

                    //messaging table
                    in >> paren >> value;
                    messaging_struct_t amessagingEntry;
                    for (i=0; i< value; i++) {
                        in >> paren >> amessagingEntry.ctx_id >> amessagingEntry.num_nodes >> amessagingEntry.send_buf_addr
                            >> amessagingEntry.recv_buf_addr >> amessagingEntry.msg_entry_size >> amessagingEntry.msg_buf_entry_count
                            >> amessagingEntry.SR >> paren;
                        messaging_table[amessagingEntry.ctx_id] = amessagingEntry;
                    }
                    in >> paren;

                    //send buffer translations
                    for (i=0; i<MAX_CORE_COUNT; i++) {
                        in >> paren >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> key >> value >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            sendbufv2p[i][key] = value;
                            DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading sendbufv2p["<<i<<"]["<< std::hex << key << "] = " << value));
                        }
                        in >> paren >> paren;
                    }

                    //recv buffer translations
                    for (i=0; i<MAX_CORE_COUNT; i++) {
                        in >> paren >> size >> paren;
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (j=0; j<size; j++) {
                            in >> paren >> key >> value >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            recvbufv2p[i][key] = value;
                            DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading recvbufv2p["<<i<<"]["<< std::hex << key << "] = " << value));
                        }
                        in >> paren >> paren;
                    }

                    DBG_( Dev, ( << "Loading RMC state finished!" ));

                } else {
                    DBG_Assert(false, ( << "GZipFlexpoints not yet supported for RMC" ));
                }
            }
        }

        void loadStateSingleFile(std::string const & aDirName) {
            //NOTE: For edge RMC structures, we assume that info related to that row is registered (no different cpu-to-edge-RMC mapping currently supported)

            std::string fname(aDirName);

            if (LoadFromSimics) {
                fname += "/rmc_state_" + boost::lexical_cast<std::string>(MACHINE_ID);
            } else {
                fname += "/0" + boost::lexical_cast<std::string>(MACHINE_ID) + "-RMC";
            }

            if (cfg.RMCGZipFlexpoints) {
                fname += ".gz";
            }
            DBG_( Dev, ( << "Loading state of RMC-" << RMCid << " from file " << fname)  );

            std::ios_base::openmode omode = std::ios::in;
            if (!cfg.RMCTextFlexpoints) {
                omode |= std::ios::binary;
            }

            std::ifstream ifs(fname.c_str(), omode);
            if (! ifs.good()) {
                DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to cold RMC state " )  );
            } else {
                boost::iostreams::filtering_stream<boost::iostreams::input> in;
                if (cfg.RMCGZipFlexpoints) {
                    in.push(boost::iostreams::gzip_decompressor());
                }
                in.push(ifs);

                std::map<uint64_t, uint64_t>::iterator iter;
                if (LoadFromSimics) {  //this case is similar to breakpoint registration in Trace mode; just replay recorded breakpoints and register their values properly
                    uint64_t RMCid_simics, address, paddr, bp_type, value;
                    uint32_t cpu_id = 0;

                    while(in >> RMCid_simics >> std::hex >> address >> paddr >> std::dec >> bp_type >> value) {
                        cpu_id = RMCid_simics + MACHINE_ID * CORES_PER_MACHINE;
                        std::map<uint64_t, uint64_t> tempTranslation;

                        DBG_( LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << " got " << br_points[bp_type] << "(" << bp_type << ")"
                                    << ", address=0x" << std::hex << address << ", paddr=0x" << paddr << std::dec << " and value=" << value ));

                        uint64_t base_address = address & PAGE_BITS;

                        tempTranslation.clear();
                        if ( bp_type == WQUEUE ||
                                bp_type == CQUEUE ||
                                bp_type == BUFFER ||
                                bp_type == CONTEXT ||
                                bp_type == CONTEXTMAP ||
                                bp_type == SEND_BUF ||
                                bp_type == RECV_BUF ||
                                bp_type == SHARED_CQUEUE )
                        {
                            tempTranslation[base_address] = paddr;
                            DBG_Assert(tempTranslation[base_address] != 0, ( << "Simics cannot translate virt2phys address for "
                                        << br_points[bp_type] << "type." ) );
                        }

                        //ASSUMPTION: For now, assume one QP per core
                        QP_table_entry_t aQPEntry;
                        // ustiugov: zeroing to enable non-Flexi mode
                        aQPEntry.lbuf_size = 0;

                        bool QP_update = false;
                        if (bp_type == WQUEUE || bp_type == CQUEUE ||
                                bp_type == BUFFER_SIZE || bp_type == CONTEXT_SIZE ||
                                ( bp_type == BUFFER && value == 0 ) )
                        {
                            QP_table_iter = QP_table.find(cpu_id);
                            if (QP_table_iter != QP_table.end())
                                aQPEntry = QP_table_iter->second;
                        }

                        messaging_struct_t amessagingEntry;
                        amessagingEntry.SR = 1; amessagingEntry.ctx_id = value; amessagingEntry.send_buf_addr = 0; amessagingEntry.recv_buf_addr = 0;
                        amessagingEntry.num_nodes = 0; amessagingEntry.msg_buf_entry_count = 0; amessagingEntry.msg_entry_size = 0; amessagingEntry.sharedCQ_size = 0;
                        bool messaging_update = false;
                        if (bp_type >= SEND_BUF && bp_type <= MSG_BUF_ENTRY_COUNT) {
                            messaging_table_iter = messaging_table.find(value);
                            if (messaging_table_iter != messaging_table.end()) {
                                amessagingEntry = messaging_table_iter->second;
                            } else {
                                messaging_update = true;
                                DBG_(LOAD_STATE_DEBUG, (<<"Creating messaging entry for ctx " << value));
                            }
                        }

                        switch(bp_type)
                        {
                            case WQUEUE:
                                wqSize = value;

                                if (wqSize * sizeof(wq_entry_t) > PAGE_SIZE)
                                    DBG_Assert( false, ( << "WARNING: WQ spans more than a single page" ));

                                DBG_( LOAD_STATE_DEBUG, ( << "Entry type: WQUEUE\n\tWQ has " << wqSize << " entries" ));
                                DBG_( LOAD_STATE_DEBUG, ( << "WQ starts at pAddr 0x" << std::hex << tempTranslation[base_address] + address - base_address
                                            << ", vAddr 0x"<< address));

                                QP_update = true;
                                aQPEntry.WQ_base = tempTranslation[base_address] + address - base_address;
                                aQPEntry.WQ_tail = 0;
                                aQPEntry.WQ_SR = 1;        //expect 1 for valid entry
                                break;
                            case CQUEUE:
                                cqSize = value;

                                if (cqSize * sizeof(cq_entry_t) > PAGE_SIZE)
                                    DBG_Assert( false, ( << "WARNING: CQ spans more than a single page" ));

                                DBG_( LOAD_STATE_DEBUG, ( << "Entry type: CQUEUE" ));
                                DBG_( LOAD_STATE_DEBUG, ( << "CQ starts at pAddr 0x"<< std::hex
                                            << tempTranslation[base_address] + address - base_address
                                            << ", vAddr 0x"<<  address));

                                QP_update = true;
                                aQPEntry.CQ_base = tempTranslation[base_address] + address - base_address;
                                aQPEntry.CQ_head = 0;
                                aQPEntry.CQ_SR = 1;        //expect 1 for valid entry
                                break;
                            case SHARED_CQUEUE:
                                DBG_( LOAD_STATE_DEBUG, ( << "Entry type: SHARED_CQUEUE" ));
                                DBG_( LOAD_STATE_DEBUG, ( << "Shared CQ entry at pAddr 0x"<< std::hex
                                            << tempTranslation[base_address] + address - base_address
                                            << ", vAddr 0x"<<  address));
                                aQPEntry.CQ_base = address;   //Virtual address in this case (was physical for normal CQ)
                                aQPEntry.CQ_head = 0;
                                aQPEntry.CQ_SR = 1;        //expect 1 for valid entry
                                if (sharedCQinfo[MACHINE_ID].translations.empty()) {
                                    sharedCQinfo[MACHINE_ID].theQPEntry = aQPEntry;
                                    DBG_(Dev, ( << "soNUMA MESSAGING INFO: Using shared CQ for messaging"));
                                }
                                sharedCQinfo[MACHINE_ID].translations[base_address] = tempTranslation[base_address];
                                break;
                            case CONTEXT_SIZE: {
                                                   if (!isRRPP) { break; }
                                                   DBG_( LOAD_STATE_DEBUG, ( << "Entry type: CONTEXT_SIZE = " << value ));
                                                   std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter = contextMap[MACHINE_ID].find(address);	//WARNING:Only register context by machine_id
                                                   DBG_Assert(cntxtIter != contextMap[MACHINE_ID].end(), ( << "Trying to register size for unregistered context " << address));
                                                   contextMap[MACHINE_ID][address].second = value;
                                                   DBG_( Tmp, ( << "Registering size " << value << " for context " << address));
                                                   break;
                                               }
                            case BUFFER_SIZE:
                                               DBG_( LOAD_STATE_DEBUG, ( << "Entry type: BUFFER_SIZE = " << value));
                                               QP_update = true;
                                               aQPEntry.lbuf_size = value;
                                               bufferSizes[cpu_id] = value;	//just for flexi apps, needed by RCP frontend
                                               break;
                            case BUFFER:
                                               {
                                                   DBG_( LOAD_STATE_DEBUG, ( << "Entry type: BUFFER" ));
                                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Registering translation for Buffer. bufferv2p["<< cpu_id <<"][0x"<< std::hex<< iter->first
                                                                   << "] = 0x" << std::hex<<iter->second));
                                                       if ((!cfg.SplitRGP && isRGP) || (cfg.SplitRGP && hasBackend))
                                                           bufferv2p[cpu_id][iter->first] = iter->second;	//only RGPs will access that
                                                   }
                                                   if (value == 0) {
                                                       QP_update = true;
                                                       aQPEntry.lbuf_base = address;
                                                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Registering lbuf base address V: "<< std::hex<< address << " for RMC-" << std::dec << RMCid));
                                                   }
                                                   break;
                                               }
                            case CONTEXT:
                                               {
                                                   if (!isRRPP) { break; }
                                                   DBG_( LOAD_STATE_DEBUG, ( << "Entry type: CONTEXT REGISTRATION" ));
                                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Registering translation for context. contextv2p["<< MACHINE_ID <<"][0x" << std::hex << iter->first
                                                                   << "] = 0x"<< std::hex<<iter->second));
                                                       contextv2p[MACHINE_ID][iter->first] = iter->second;
                                                   }
                                                   break;
                                               }
                            case CONTEXTMAP:
                                               {
                                                   if (!isRRPP) { break; }
                                                   DBG_( LOAD_STATE_DEBUG, ( << "Entry type: CONTEXT MAPPING" ));
                                                   std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter = contextMap[MACHINE_ID].find(value);
                                                   if (cntxtIter == contextMap[MACHINE_ID].end()) {
                                                       contextMap[MACHINE_ID][value].first = tempTranslation.begin()->first;
                                                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Establishing context mapping: contextMap[" << MACHINE_ID << "][" << value
                                                                   << "] = (0x" << std::hex << tempTranslation.begin()->first << ", <ctx_size>)" ));
                                                   }
                                                   break;
                                               }
                            case ALL_SET:
                                               // ustiugov: do nothing here
                                               /*
                                                  ReadyCount++;
                                                  DBG_(LOAD_STATE_DEBUG, ( <<" RMC " << RMCid <<" is ready to start servicing on behalf of cpu " << cpu_id
                                                  << ". ReadyCount: " << ReadyCount << ", ReadyCount Target: " << READY_COUNT_TARGET << ", ALL_SET id=" << size));
                                                  if (ReadyCount == READY_COUNT_TARGET) {
                                                  if (size == READY_ALL_SET_SIGNAL_ID) {
                                                  status = READY;
                                                  Simics::API::SIM_break_simulation("Ready for checkpointing for timing!");
                                                  } else {
                                                  Simics::API::SIM_break_simulation("Ready for checkpointing!");
                                                  }
                                                  }
                                                  */
                                               break;
                            case NEWWQENTRY_START:
                            case NEWWQENTRY:
                            case NETPIPE_START:
                            case NETPIPE_END:
                                               break;
#if 0
                                               WQstartTS[cpu_id] = theCycleCount;
                                               E2EstartTS[cpu_id] = theCycleCount;
                                               DBG_(BREAKPOINTS_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" started enqueuing a new WQ entry - Opcount = " << value << ", WQ index = " << address));
                                               DBG_(WQ_CQ_ACTIONS_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" started enqueuing a new WQ entry - Opcount = " << value << ", WQ index = " << address));
                                               //DBG_(LOAD_STATE_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" started enqueuing a new WQ entry - Opcount = " << size << ", WQ index = " << address));
                                               break;
                            case NEWWQENTRY:
                                               //#ifdef TRACE_LATENCY
                                               DBG_(WQ_CQ_ACTIONS_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" enqueued a new WQ entry - Opcount = " << value << ", WQ index = " << address));
                                               //DBG_(LOAD_STATE_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" enqueued a new WQ entry - Opcount = " << value << ", WQ index = " << address));

                                               //#endif
                                               DBG_(BREAKPOINTS_DEBUG, ( << "RMC-"<< RMCid << ": CPU-"<<cpu_id<<" enqueued a new WQ entry - Opcount = " << value << ", WQ index = " << address));
                                               //stats
                                               DBG_(BREAKPOINTS_DEBUG, ( << "RMC-"<< RMCid << ": Creating entry RTcycleCounter[" << (1000*cpu_id + address) << "] = " << theCycleCount));
                                               cycleCounterIter = RTcycleCounter.find(1000*cpu_id + address);
                                               if (cycleCounterIter != RTcycleCounter.end())
                                                   DBG_(BREAKPOINTS_DEBUG, ( << "WARNING: entry RTcycleCounter[" << (1000*cpu_id + address) << "] = " << cycleCounterIter->second
                                                               << " already exists! Not sure how this can happen, ignore for now and hope it happens rarely"));
                                               RTcycleCounter[1000*cpu_id + address] = theCycleCount;

                                               //if (/*(cfg.SplitRGP && theCycleCount - WQstartTS[cpu_id] < 200) || (!cfg.SplitRGP &&*/( theCycleCount - WQstartTS[cpu_id] < 400)) {    //leaving out extreme values. Dunno how they occur...
                                               TotalWQProcessing += theCycleCount - WQstartTS[cpu_id];
                                               WQCount++;
                                               //DBG_(LOAD_STATE_DEBUG, ( << "\tProcessing time for WQ enqueueuing: " << theCycleCount - WQstartTS[cpu_id]));
                                               //} else {
                                               //    E2EstartTS[cpu_id] = -1;
                                               //}


                                               WQstartTS[cpu_id] = theCycleCount;
#endif // 0
                                               break;
                            case 25:
                            case 26:
                            case 28:
                            case 29:
                            case 30:
                            case 99:
                            case CONTROL_FLOW:
                            case WQENTRYDONE:
                            case LOCK_SPINNING:
                                               DBG_(LOAD_STATE_DEBUG, ( << "Received bp#" << bp_type ) );
                                               break;
                            case SEND_BUF:
                                               {
                                                   //if (!isRRPP) { break; }
                                                   DBG_( LOAD_STATE_DEBUG, ( << "Entry type: SEND_BUF REGISTRATION" ));
                                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Registering translation for send buf. sendbufv2p["<< MACHINE_ID <<"][0x" << std::hex << iter->first
                                                                   << "] = 0x"<< std::hex<<iter->second));
                                                       sendbufv2p[MACHINE_ID][iter->first] = iter->second;
                                                   }

                                                   if (amessagingEntry.send_buf_addr == 0) {
                                                       messaging_update = true;
                                                       amessagingEntry.send_buf_addr = address;
                                                       DBG_( LOAD_STATE_DEBUG, ( << "Registering send buffer base address V: "<< std::hex<< address << " for context " << std::dec << value));
                                                   }

                                                   break;
                                               }
                            case RECV_BUF:
                                               {
                                                   //if (!isRRPP) { break; }
                                                   DBG_( LOAD_STATE_DEBUG, ( << "Entry type: RECV_BUF REGISTRATION" ));
                                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                                       DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << ": Registering translation for recv buf. recvbufv2p["<< MACHINE_ID <<"][0x" << std::hex << iter->first
                                                                   << "] = 0x"<< std::hex<<iter->second));
                                                       recvbufv2p[MACHINE_ID][iter->first] = iter->second;
                                                   }

                                                   if (amessagingEntry.recv_buf_addr == 0) {
                                                       messaging_update = true;
                                                       amessagingEntry.recv_buf_addr = address;
                                                       DBG_( LOAD_STATE_DEBUG, ( << "Registering recv buffer base address V: "<< std::hex<< address << " for context " << std::dec << value));
                                                       if (ctxToBEs.find(value) == ctxToBEs.end()) {
                                                           ctxToBEs[value] = 0;  //create an entry for this context
                                                       }
                                                   }
                                                   break;
                                               }
                            case MSG_BUF_ENTRY_SIZE: {
                                                         messaging_update = true;
                                                         amessagingEntry.msg_entry_size = address;
                                                         DBG_( LOAD_STATE_DEBUG, ( << "Entry type: MSG_BUF_ENTRY_SIZE for ctx " << value << " (for messaging). Value: " << address ));
                                                         break;
                                                     }
                            case NUM_NODES: {
                                                messaging_update = true;
                                                amessagingEntry.num_nodes = address;
                                                DBG_( LOAD_STATE_DEBUG, ( << "Entry type: NUM_NODES for ctx " << value << " (for messaging). Value: " << address ));
                                                break;
                                            }
                            case MSG_BUF_ENTRY_COUNT: {
                                                          messaging_update = true;
                                                          amessagingEntry.msg_buf_entry_count = address;
                                                          DBG_( LOAD_STATE_DEBUG, ( << "Entry type: MSG_BUF_ENTRY_SIZE for ctx " << value << " (for messaging). Value: " << address ));
                                                          break;
                                                      }
                            case SENDENQUEUED:
                                                      DBG_( LOAD_STATE_DEBUG, ( << "Entry type: SENDENQUEUD with index " << address << " and address 0x" << std::hex << value ));
                                                      break;
                            default:
                                                      DBG_Assert(false, ( << "Tried to load an unknown breakpoint type: " <<  bp_type) );
                                                      break;
                        }
                        if (messaging_update) {// && isRRPP) {
                            DBG_(LOAD_STATE_DEBUG, ( <<" RMC " << RMCid <<" inserting messaging entry for context " << value << ": messaging_table[" << value << "]"));
                            messaging_table[value] = amessagingEntry;
                        }
                        if (!isRGP) return;		//only required by RGPs (and RCPs)
                        if (QP_update) {
                            if (cfg.SplitRGP && isRRPP) {
                                uint16_t theBitSet = 0;
                                ctxToBEsIter = ctxToBEs.begin();
                                while (ctxToBEsIter != ctxToBEs.end()) {
                                    theBitSet = ctxToBEsIter->second;
                                    theBitSet |= (1 << ((cpu_id%CORES_PER_MACHINE)/CORES_PER_RMC));
                                    ctxToBEsIter->second = theBitSet;
                                    DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << " updating its ctxToBEs mapping for context " << ctxToBEsIter->first << ": " << (std::bitset<16>)theBitSet
                                                << "(received registration msg from core " << cpu_id <<")"));
                                    ctxToBEsIter++;
                                }
                            }

                            if ( (!cfg.SplitRGP && ((cfg.RGPPerTile && cpu_id == RMCid) || (!cfg.RGPPerTile && cpu_id >= RMCid && cpu_id <= RMCid + CORES_PER_RMC - 1)))
                                    || (cfg.SplitRGP && hasFrontend && cpu_id == RMCid )) {
                                DBG_(LOAD_STATE_DEBUG, ( <<" RMC(frontend)-" << RMCid <<" inserting QP entry: QP_table[" << cpu_id << "]"));
                                QP_table[cpu_id] = aQPEntry;
                                if ( !QPidFreelist.size() ) { QPidFreelist.push_back(cpu_id); }
                                if (FLEXI_APP_REQ_SIZE) {
                                    DBG_(Dev, ( << "NOTE: Running in FLEXI APP mode with FLEXI_APP_REQ_SIZE = " << FLEXI_APP_REQ_SIZE));
                                }
                            }
                            //////this is for messaging; RRPP needs to know the state of the CQ in terms of queued msgs
                            if (cfg.SplitRGP && isRRPP && 
                                    ( (!cfg.SingleRMCDispatch && (RMCid / CORES_PER_RMC == cpu_id / CORES_PER_RMC))   //RRPPs are servicing RGPs/RCPs of the same row
                                      || (cfg.SingleRMCDispatch && (RMCid == getRMCDispatcherID_LocalMachine()))  //one RRPP servicing ALL RGPs/RCPs of the chip
                                    )
                               )
                            {
                                aQPEntry.cur_jobs = 0;
                                QP_table[cpu_id] = aQPEntry;
                                DBG_( LOAD_STATE_DEBUG, ( << "RMC(backend)-" << RMCid << " inserting QP entry: QP_table[" << cpu_id << "]"));
                            }
                            //////
                        }
                        }

                        //QPidFreelist.push_back(RMCid);

                        DBG_( Dev, ( << "Loading RMC state finished! (from simics)" ));
                        return;
                    }

                    if (cfg.RMCTextFlexpoints) {
                        DBG_( Dev, ( << "Loading RMC state" ));
                        char paren;
                        uint64_t key, value, size, size2;
                        uint32_t value32;
                        uint64_t i, j;

                        in >> paren >> value;
                        DBG_Assert(paren == '{', ( << "Error in loading state" ));
                        DBG_Assert(value == PAGE_BITS, ( << "Error in loading state - different page size" ));
                        in >> value;
                        DBG_Assert(value == PT_I, ( << "Error in loading state" ));
                        in >> value;
                        DBG_Assert(value == PT_J, ( << "Error in loading state" ));
                        in >> value >> paren;
                        DBG_Assert(value == PT_K, ( << "Error in loading state" ));

                        //Load RMC_TLB	-	this will be empty in this case
                        //DBG_( Dev, ( << "Now loading RMC TLB from file " << fname << "..." ));
                        in >> paren >> value;

                        if (value != TLBsize) DBG_(Dev, ( << "WARNING: The size of the TLB size in previous flexpoint is different than requested ("
                                    << value << " instead of " << TLBsize << ")" ));
                        DBG_Assert(paren == '{', ( << "Error in loading state" ));

                        in >> size >> paren;
                        DBG_Assert(!size);	//Should be empty in this loading mode
                        DBG_Assert(paren == '[', ( << "Error in loading state" ));
                        for (i = 0; i < size; i++) {
                            in >> paren >> key >> value >> paren;
                            DBG_Assert(paren == ')', ( << "Error in loading state" ));
                            RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(key, value));
                            DBG_( Dev, ( << "\tLoading TLB key=" << key << ", value=" << value ));
                        }

                        in >> paren >> paren;

                        //PTv2p translations	-	let all RMCs have this info
                        for (i=0; i<MAX_CORE_COUNT; i++) {
                            in >> paren >> value >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            PTbaseV[i] = value;

                            for (j=0; i<size; j++) {
                                in >> paren >> key >> value >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                                PTv2p[i][key] = value;
                                DBG_( LOAD_STATE_DEBUG, ( << "\tLoading TLB key=" << key << ", value=" << value ));
                            }
                            in >> paren >> paren;
                        }

                        //buffers				-	only RGPs will access that
                        for (i=0; i<MAX_CORE_COUNT; i++) {
                            in >> paren >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> key >> value >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                                if ((!cfg.SplitRGP && isRGP) || (cfg.SplitRGP && hasBackend)) {
                                    bufferv2p[i][key] = value;				//only RGPs will access that
                                    DBG_( LOAD_STATE_DEBUG, ( << "Registering translation for Buffer. bufferv2p["<< i <<"][0x"<< std::hex<< key
                                                << "] = 0x" << std::hex<<value));
                                }
                            }
                            in >> paren >> paren;
                        }

                        //context				-	only RRPPs will access that
                        for (i=0; i<MAX_CORE_COUNT; i++) {
                            in >> paren >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> key >> value >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                                if (isRRPP) {
                                    contextv2p[i][key] = value;				//only RRPPs will access that
                                    DBG_( LOAD_STATE_DEBUG, ( << "Registering translation for context. contextv2p["<< i <<"][0x" << std::hex << key
                                                << "] = 0x"<< std::hex<<value));
                                }
                            }
                            in >> paren >> paren;
                        }

                        //contextMap
                        for (i=0; i<MAX_CORE_COUNT; i++) {
                            in >> paren >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> key >> value >> size2 >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                                if (isRRPP) {
                                    contextMap[i][key].first = value;				//only RRPPs will access that
                                    contextMap[i][key].second = size2;				//only RRPPs will access that
                                    DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading contextMap["<<i<<"]["<< key<<"] = (0x" << std::hex << value << ", " << size2 << ")"));
                                }
                            }
                            in >> paren >> paren;
                        }
                        DBG_Assert(paren == '}', ( << "Error in loading state" ));

                        RMCPacket aPacket;
                        uint64_t offset, bufaddr, qid, pkt_index;
                        uint16_t context, tid, src_nid, dst_nid;
                        RemoteOp op = RREAD;	//bogus initialization
                        aBlock payload;

                        in >> paren >> size2; 		//outQueue.size()
                        DBG_Assert(paren == '{', ( << "Error in loading state" ));
                        for (i=0; i<size2; i++) {
                            in >> paren >> qid >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> tid >> src_nid >> dst_nid >> context >> offset >> bufaddr >> pkt_index >> value;
                                DBG_Assert(paren == '(', ( << "Error in loading state" ));
                                switch (value) {
                                    case 0:
                                        op = RREAD;
                                        break;
                                    case 1:
                                        op = RWRITE;
                                        break;
                                    case 2:
                                        op = RRREPLY;
                                        break;
                                    case 3:
                                        op = RWREPLY;
                                        break;
                                    default:
                                        DBG_Assert(false);
                                }
                                if (op == RWRITE || op == RRREPLY) {	//payload has useful info
                                    for (uint32_t k = 0; k<64; k++) {
                                        in >> payload.data[k];
                                    }
                                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, payload, bufaddr, pkt_index, 64);
                                } else {
                                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, bufaddr, pkt_index, 64);
                                }
                                //outQueueReq.push_back(aPacket);
                                DBG_Assert(false, ( << "There is no more outQueueRep in the RMC. This was moved to RMCConnector"));
                                in >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));

                            }
                            in >> paren >> paren;
                            DBG_Assert(paren == '}', ( << "Error in loading state" ));
                        }

                        in >> paren >> paren >> size2; 		//inQueue.size()
                        DBG_Assert(paren == '{', ( << "Error in loading state" ));
                        for (i=0; i<size2; i++) {
                            in >> paren >> qid >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> tid >> src_nid >> dst_nid >> context >> offset >> bufaddr >> pkt_index >> value;
                                DBG_Assert(paren == '(', ( << "Error in loading state" ));
                                switch (value) {
                                    case 0:
                                        op = RREAD;
                                        break;
                                    case 1:
                                        op = RWRITE;
                                        break;
                                    case 2:
                                        op = RRREPLY;
                                        break;
                                    case 3:
                                        op = RWREPLY;
                                        break;
                                    default:
                                        DBG_Assert(false);
                                }
                                if (op == RWRITE || op == RRREPLY) {	//payload has useful info
                                    for (uint32_t k = 0; k<64; k++) {
                                        in >> payload.data[k];
                                    }
                                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, payload, bufaddr, pkt_index, 64);
                                } else {
                                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, bufaddr, pkt_index, 64);
                                }
                                //inQueueReq.push_back(aPacket);
                                DBG_Assert(false, ( << "There is no more inQueueReq in the RMC. This was moved to RMCConnector"));
                                in >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));

                            }
                            in >> paren >> paren;
                            DBG_Assert(paren == '}', ( << "Error in loading state" ));
                        }
                        in >> paren;
                        DBG_Assert(paren == '}', ( << "Error in loading state" ));

                        in >> value32;

                        in >> value;
                        if (value == 0)
                            status = INIT;
                        else
                            status = READY;

                        in >> value;
                        uniqueTID = (uint8_t)value;
                        in >> value;
                        wqSize = value;
                        in >> value;
                        cqSize = value;

                        //Now load the SRAM structures	- here is where the distribution of information needs to be done, according to the intended core-to-RGP mapping!
                        if (!isRGP) return;		//only required by RGPs (and RCPs)
                        if (cfg.RGPPerTile || cfg.SplitRGP) {
                            DBG_( Dev, ( << "RMC-" << RMCid << " is presumed to be servicing only core " << RMCid << ". Loading the related state (QP and ITT tables)..." ));
                        } else {
                            DBG_( Dev, ( << "RMC-" << RMCid << " is presumed to be servicing cores " << RMCid << " to " << (RMCid + CORES_PER_RMC - 1) << ". Loading the related state (QP and ITT tables)..." ));
                        }
                        //QP table
                        in >> paren >> value;
                        QP_table_entry_t aQPEntry;
                        uint32_t idx;
                        uint8_t WQ_SR, CQ_SR;
                        uint64_t lbuf_base, lbuf_size;
                        for (i=0; i< value; i++) {
                            in >> paren >> idx >> aQPEntry.WQ_base >> aQPEntry.WQ_tail >> WQ_SR >> aQPEntry.CQ_base
                                >> aQPEntry.CQ_head >> CQ_SR >> lbuf_base >> lbuf_size >> paren;
                            aQPEntry.WQ_SR = WQ_SR;
                            aQPEntry.CQ_SR = CQ_SR;
                            aQPEntry.lbuf_base = lbuf_base;
                            aQPEntry.lbuf_size = lbuf_size;
                            aQPEntry.cur_jobs = 0;
                            bufferSizes[idx] = lbuf_size;	//just for flexi apps, needed by RCP frontend
                            if ( (!cfg.SplitRGP && ((cfg.RGPPerTile && idx == RMCid) || (!cfg.RGPPerTile && idx >= RMCid && idx <= RMCid + CORES_PER_RMC - 1)))
                                    || (cfg.SplitRGP && hasFrontend && idx == RMCid )) {
                                DBG_( Dev, ( << "\t Loading QP entry with index " << idx << "..." ));
                                DBG_( LOAD_STATE_DEBUG, ( << "WQ_SR=" << WQ_SR << ", CQ_SR=" << CQ_SR << ", lbuf_base=" << std::hex << lbuf_base << std::dec << ", lbuf_size=" << lbuf_size << ", lbuf_size=" << lbuf_size) );
                                QP_table[idx] = aQPEntry;
                                QPidFreelist.push_back(idx);
                                if (FLEXI_APP_REQ_SIZE) {
                                    DBG_(Dev, ( << "NOTE: Running in FLEXI APP mode with FLEXI_APP_REQ_SIZE = " << FLEXI_APP_REQ_SIZE << ", context size = 0x" << std::hex << ", buffer size = 0x" << aQPEntry.lbuf_size));
                                }
                            } else
                                DBG_( LOAD_STATE_DEBUG, ( << "\t Skipping QP entry with index " << idx << "..." ));
                        }
                        in >> paren;

                        //ITT table		-	this should be empty!
                        in >> paren >> value;
                        DBG_Assert(!value, ( << "ITT should be empty when trying to load in SingleRMCLoading mode!"));
                        ITT_table_entry_t anITTEntry;
                        for (i=0; i< value; i++) {
                            in >> paren >> idx >> anITTEntry.counter >> anITTEntry.QP_id >> anITTEntry.WQ_index >> anITTEntry.lbuf_addr >> paren;
                            ITT_table[idx] = anITTEntry;
                        }
                        in >> paren;

                        if (!isRRPP) return;  //only RRPPs store info about send/recv buffers
                        //messaging table
                        in >> paren >> value;
                        messaging_struct_t amessagingEntry;
                        for (i=0; i< value; i++) {
                            in >> paren >> amessagingEntry.ctx_id >> amessagingEntry.num_nodes >> amessagingEntry.send_buf_addr
                                >> amessagingEntry.recv_buf_addr >> amessagingEntry.msg_entry_size >> amessagingEntry.msg_buf_entry_count
                                >> amessagingEntry.SR >> paren;
                            messaging_table[amessagingEntry.ctx_id] = amessagingEntry;
                        }
                        in >> paren;

                        //send buffer translations
                        for (i=0; i<MAX_CORE_COUNT; i++) {
                            in >> paren >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> key >> value >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                                sendbufv2p[i][key] = value;
                                DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading sendbufv2p["<<i<<"]["<< std::hex << key << "] = " << value));
                            }
                            in >> paren >> paren;
                        }

                        //recv buffer translations
                        for (i=0; i<MAX_CORE_COUNT; i++) {
                            in >> paren >> size >> paren;
                            DBG_Assert(paren == '[', ( << "Error in loading state" ));
                            for (j=0; j<size; j++) {
                                in >> paren >> key >> value >> paren;
                                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                                recvbufv2p[i][key] = value;
                                DBG_(LOAD_STATE_DEBUG, ( << "RMC-" << RMCid << ": Loading recvbufv2p["<<i<<"]["<< std::hex << key << "] = " << value));
                            }
                            in >> paren >> paren;
                        }

                        DBG_( Dev, ( << "Loading RMC state finished!" ));

                    } else {
                        DBG_Assert(false, ( << "GZipFlexpoints not yet supported for RMC" ));
                    }
                }
            }

            //RMC STUFF - END
        };

}//End namespace nuArchARM

FLEXUS_COMPONENT_INSTANTIATOR(uArchARM, nuArchARM);

FLEXUS_PORT_ARRAY_WIDTH( uArchARM, RMCPacketOut )   {
  return 2;
}

FLEXUS_PORT_ARRAY_WIDTH( uArchARM, RMCPacketIn )   {
  return 2;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uArchARM

#define DBG_Reset
#include DBG_Control()
