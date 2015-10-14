#include <set>

#include <core/boost_extensions/padded_string_cast.hpp>

#include <core/types.hpp>
#include <core/debug/debug.hpp>

#include <core/simics/configuration_api.hpp>
#include <core/simics/mai_api.hpp>

#include <components/DecoupledFeeder/SimicsTracer.hpp>

namespace Flexus {
namespace Simics {
namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict

#include FLEXUS_SIMICS_API_ARCH_HEADER

#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_HEADER(front)
#include FLEXUS_SIMICS_API_HEADER(event)
#undef printf
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(breakpoints)
} //extern "C"
} //namespace API
} //namespace Simics
} //namespace Flexus

#include <core/stats.hpp>
namespace Stat = Flexus::Stat;

namespace nDecoupledFeeder {

using namespace Flexus::SharedTypes;

namespace Simics = Flexus::Simics;
namespace API = Simics::API;

std::set< API::conf_object_t *> theTimingModels;
const uint32_t kLargeMemoryQueue = 16000;

struct TracerStats {
  Stat::StatCounter theMemOps_stat;
  Stat::StatCounter theIOOps_stat;
  Stat::StatCounter theUncacheableOps_stat;
  Stat::StatCounter theFetches_stat;
  Stat::StatCounter theRMWOps_stat;
  Stat::StatCounter theCASOps_stat;
  Stat::StatCounter theLoadOps_stat;
  Stat::StatCounter theStoreOps_stat;
  Stat::StatCounter thePrefetchOps_stat;

  int64_t theIOOps;
  int64_t theUncacheableOps;
  int64_t theFetches;
  int64_t theRMWOps;
  int64_t theCASOps;
  int64_t theLoadOps;
  int64_t theStoreOps;
  int64_t thePrefetchOps;

  TracerStats(std::string const & aName)
    : theMemOps_stat(aName + "MemOps")
    , theIOOps_stat(aName + "IOOps")
    , theUncacheableOps_stat(aName + "UncacheableOps")
    , theFetches_stat(aName + "Fetches")
    , theRMWOps_stat(aName + "RMWOps")
    , theCASOps_stat(aName + "CASOps")
    , theLoadOps_stat(aName + "LoadOps")
    , theStoreOps_stat(aName + "StoreOps")
    , thePrefetchOps_stat(aName + "PrefetchOps")

    , theIOOps(0)
    , theUncacheableOps(0)
    , theFetches(0)
    , theRMWOps(0)
    , theCASOps(0)
    , theLoadOps(0)
    , theStoreOps(0)
    , thePrefetchOps(0)
  {}

  void update() {
    theMemOps_stat += theLoadOps + theStoreOps + theCASOps + theRMWOps + thePrefetchOps;
    theIOOps_stat += theIOOps;
    theUncacheableOps_stat += theUncacheableOps;
    theFetches_stat += theFetches;
    theRMWOps_stat += theRMWOps;
    theCASOps_stat += theCASOps;
    theLoadOps_stat += theLoadOps;
    theStoreOps_stat += theStoreOps;
    thePrefetchOps_stat += thePrefetchOps;
    theIOOps = 0;
    theUncacheableOps = 0;
    theFetches = 0;
    theRMWOps = 0;
    theCASOps = 0;
    theLoadOps = 0;
    theStoreOps = 0;
    thePrefetchOps = 0;
  }
};

#if FLEXUS_TARGET_IS(v9)
#define IS_PRIV(mem_trans) (mem_trans->priv)
#else //!FLEXUS_TARGET_IS(v9)
#define IS_PRIV(mem_trans) (mem_trans->mode == API::Sim_CPU_Mode_Supervisor)
#endif //FLEXUS_TARGET_IS(v9)

class SimicsTracerImpl {
  API::conf_object_t * theUnderlyingObject;
  API::conf_object_t * theCPU;
  int32_t theIndex;
  API::conf_object_t * thePhysMemory;
  API::conf_object_t * thePhysIO;

  TracerStats * theUserStats;
  TracerStats * theOSStats;
  TracerStats * theBothStats;

  MemoryMessage theMemoryMessage;
  std::function< void(int, MemoryMessage &) > toL1D;
  std::function< void(int, MemoryMessage &, uint32_t) > toL1I;
  std::function< void(int, MemoryMessage &) > toNAW;

  bool theWhiteBoxDebug;
  int32_t  theWhiteBoxPeriod;
  bool theSendNonAllocatingStores;

public:
  SimicsTracerImpl(API::conf_object_t * anUnderlyingObject)
    : theUnderlyingObject(anUnderlyingObject)
    , theMemoryMessage(MemoryMessage::LoadReq)
  {}

  API::conf_object_t * cpu() const {
    return theCPU;
  }

  // Initialize the tracer to the desired CPU
  void init(API::conf_object_t * aCPU
            , index_t anIndex
            , std::function< void(int, MemoryMessage &) > aToL1D
            , std::function< void(int, MemoryMessage &, uint32_t) > aToL1I
            , std::function< void(int, MemoryMessage &) > aToNAW
            , bool aWhiteBoxDebug
            , int32_t aWhiteBoxPeriod
            , bool aSendNonAllocatingStores
 	
           ) {
    theCPU = aCPU;
    theIndex = anIndex;
    toL1D = aToL1D;
    toL1I = aToL1I;
    toNAW = aToNAW;
    theWhiteBoxDebug = false;
    theWhiteBoxPeriod = aWhiteBoxPeriod;
    theSendNonAllocatingStores = aSendNonAllocatingStores;

    theUserStats = new TracerStats(boost::padded_string_cast < 2, '0' > (theIndex) + "-feeder-User:");
    theOSStats = new TracerStats(boost::padded_string_cast < 2, '0' > (theIndex) + "-feeder-OS:");
    theBothStats = new TracerStats(boost::padded_string_cast < 2, '0' > (theIndex) + "-feeder-");

    API::attr_value_t attr;

    attr.kind = API::Sim_Val_String;
    attr.u.string = theCPU->name;
    API::SIM_set_attribute(theUnderlyingObject, "queue", &attr);

    attr.kind = API::Sim_Val_Object;
    attr.u.object = theUnderlyingObject;

    /* Tell memory we have a mem hier */
    thePhysMemory = API::SIM_get_attribute(theCPU, "physical_memory").u.object;
    API::SIM_set_attribute(thePhysMemory, "timing_model", &attr);
    if (theTimingModels.count(thePhysMemory) > 0) {
      DBG_Assert( false, ( << "Two CPUs connected to the same memory timing_model: " << thePhysMemory->name) );
    }
    theTimingModels.insert(thePhysMemory);

#if FLEXUS_TARGET_IS(v9)
    /* Tell memory we have a mem hier */
    thePhysIO = API::SIM_get_attribute(theCPU, "physical_io").u.object;
    API::SIM_set_attribute(thePhysIO, "timing_model", &attr);
    if (theTimingModels.count(thePhysIO) > 0) {
      DBG_Assert( false, ( << "Two CPUs connected to the same I/O timing_model: " << thePhysIO->name) );
    }
    theTimingModels.insert(thePhysIO);
#else
    thePhysIO = 0;
#endif

  }

  void updateStats() {
    theUserStats->update();
    theOSStats->update();
    theBothStats->update();
  }

  API::cycles_t insn_fetch(Simics::APIFwd::memory_transaction_t * mem_trans) {
    const int32_t k_no_stall = 0;

    mem_trans->s.block_STC = 1;

    theMemoryMessage.address() = PhysicalMemoryAddress( mem_trans->s.physical_address );
    theMemoryMessage.pc() = VirtualMemoryAddress( mem_trans->s.logical_address );
    theMemoryMessage.type() = MemoryMessage::FetchReq;
    theMemoryMessage.priv() = IS_PRIV(mem_trans);
    theMemoryMessage.tl() = API::SIM_read_register(theCPU, 46 /* kTL */);
    uint32_t opcode = API::SIM_read_phys_memory(theCPU, mem_trans->s.physical_address, 4);

    IS_PRIV(mem_trans) ?  theOSStats->theFetches++ : theUserStats->theFetches++ ;
    theBothStats->theFetches++;

    DBG_ (VVerb, ( << "sending fetch[" << theIndex << "] op: " << opcode << " message: " << theMemoryMessage) );
    toL1I(theIndex, theMemoryMessage, opcode);

    return k_no_stall; //Never stalls
  }

  API::cycles_t trace_mem_hier_operate(API::conf_object_t * space, API::map_list_t * map, API::generic_transaction_t * aMemTrans) {
    Simics::APIFwd::memory_transaction_t * mem_trans = reinterpret_cast<Simics::APIFwd::memory_transaction_t *>(aMemTrans);

    const int32_t k_no_stall = 0;

    
    if (space == thePhysIO) {
      mem_trans->s.block_STC = 1;
      IS_PRIV(mem_trans) ?  theOSStats->theIOOps++ : theUserStats->theIOOps++ ;
      theBothStats->theIOOps++;
      return k_no_stall; //Not a memory operation
    }

#if FLEXUS_TARGET_IS(x86)
    if (mem_trans->io ) {
      mem_trans->s.block_STC = 1;
      //Count data accesses
      IS_PRIV(mem_trans) ?  theOSStats->theIOOps++ : theUserStats->theIOOps++ ;
      theBothStats->theIOOps++;
      return k_no_stall; //Not a memory operation
    }
#endif

#if FLEXUS_TARGET_IS(v9)
    if (! mem_trans->cache_physical ) {
      mem_trans->s.block_STC = 1;
      //Count data accesses
      IS_PRIV(mem_trans) ?  theOSStats->theUncacheableOps++ : theUserStats->theUncacheableOps++ ;
      theBothStats->theUncacheableOps++;
      return k_no_stall; //Not a memory operation
    }
#endif

#if 0
#if FLEXUS_TARGET_IS(v9)
    if (mem_trans->address_space == 0x71 ) {
      //BLK stores to ASI 71.
      //These stores update the data caches on hit, but do not allocate on
      //miss.  The best way to model these in
      theMemoryMessage.address() = PhysicalMemoryAddress( mem_trans->s.physical_address );
      theMemoryMessage.pc() = VirtualMemoryAddress( API::SIM_get_program_counter(theCPU) );
      theMemoryMessage.priv() = IS_PRIV(mem_trans);
      theMemoryMessage.reqSize() = mem_trans->s.size;
      theMemoryMessage.type() = MemoryMessage::NonAllocatingStoreReq;
      toNAW(theIndex, theMemoryMessage);
      // NOTE: with the current implementation the LRU position of a block is not updated upon a NAW hit. (NIKOS)
      // We may want to fix the NAW hits, but I don't expect it'll make a measurable difference.
      return k_no_stall; //Not a memory operation
    }
#endif
#endif

    if (mem_trans->s.type == API::Sim_Trans_Instr_Fetch) {
      return insn_fetch(mem_trans);
    }

    //Ensure that we see future accesses to this block
    mem_trans->s.block_STC = 1;

    //Minimal memory message implementation
    theMemoryMessage.address() = PhysicalMemoryAddress( mem_trans->s.physical_address );
    API::logical_address_t pc_logical = API::SIM_get_program_counter(theCPU);
    theMemoryMessage.pc() = VirtualMemoryAddress( pc_logical );
    theMemoryMessage.priv() = IS_PRIV(mem_trans);

    //Set the type field of the memory operation
    if (mem_trans->s.atomic) {
      //Need to determine opcode, as this may be an RMW or CAS

#if FLEXUS_TARGET_IS(v9)
      // record the opcode

      API::physical_address_t pc = API::SIM_logical_to_physical(theCPU, API::Sim_DI_Instruction, pc_logical);
      uint32_t op_code = API::SIM_read_phys_memory(theCPU, pc, 4);

      //LDD(a)            is 11-- ---0 -001 1--- ---- ---- ---- ----
      //STD(a)            is 11-- ---0 -011 1--- ---- ---- ---- ----

      //LDSTUB(a)/SWAP(a) is 11-- ---0 -11- 1--- ---- ---- ---- ----
      //CAS(x)A           is 11-- ---1 111- 0--- ---- ---- ---- ----

      const uint32_t kLDD_mask = 0xC1780000;
      const uint32_t kLDD_pattern = 0xC0180000;

      const uint32_t kSTD_mask = 0xC1780000;
      const uint32_t kSTD_pattern = 0xC038000;

      const uint32_t kRMW_mask = 0xC1680000;
      const uint32_t kRMW_pattern = 0xC0680000;

      const uint32_t kCAS_mask = 0xC1E80000;
      const uint32_t kCAS_pattern = 0xC1E00000;

      if ((op_code & kLDD_mask) == kLDD_pattern) {
        theMemoryMessage.type() = MemoryMessage::LoadReq;
        IS_PRIV(mem_trans) ?  theOSStats->theLoadOps++ : theUserStats->theLoadOps++ ;
        theBothStats->theLoadOps++;
      } else if ((op_code & kSTD_mask) == kSTD_pattern) {
        theMemoryMessage.type() = MemoryMessage::StoreReq;
        IS_PRIV(mem_trans) ?  theOSStats->theStoreOps++ : theUserStats->theStoreOps++ ;
        theBothStats->theStoreOps++;
      } else if ((op_code & kCAS_mask) == kCAS_pattern)  {
        theMemoryMessage.type() = MemoryMessage::CmpxReq;
        IS_PRIV(mem_trans) ?  theOSStats->theCASOps++ : theUserStats->theCASOps++ ;
        theBothStats->theCASOps++;
      } else if ((op_code & kRMW_mask) == kRMW_pattern) {
        theMemoryMessage.type() = MemoryMessage::RMWReq;
        IS_PRIV(mem_trans) ?  theOSStats->theRMWOps++ : theUserStats->theRMWOps++ ;
        theBothStats->theRMWOps++;
      } else {
        DBG_Assert( false, ( << "Unknown atomic operation. Opcode: " << std::hex << op_code << " pc: " << pc << std::dec ) );
      }
#else
      //Assume all atomic x86 operations are RMWs.  This may not be true
      theMemoryMessage.type() = MemoryMessage::RMWReq;
      IS_PRIV(mem_trans) ?  theOSStats->theRMWOps++ : theUserStats->theRMWOps++ ;
      theBothStats->theRMWOps++;
#endif //v9

    } else {
      switch (mem_trans->s.type) {
        case API::Sim_Trans_Load:
          theMemoryMessage.type() = MemoryMessage::LoadReq;
          IS_PRIV(mem_trans) ?  theOSStats->theLoadOps++ : theUserStats->theLoadOps++ ;
          theBothStats->theLoadOps++;
          break;
        case API::Sim_Trans_Store:
          theMemoryMessage.type() = MemoryMessage::StoreReq;
          IS_PRIV(mem_trans) ?  theOSStats->theStoreOps++ : theUserStats->theStoreOps++ ;
          theBothStats->theStoreOps++;
          break;
        case API::Sim_Trans_Prefetch:
          //Prefetches are currently mapped as loads
          theMemoryMessage.type() = MemoryMessage::LoadReq;
          IS_PRIV(mem_trans) ?  theOSStats->thePrefetchOps++ : theUserStats->thePrefetchOps++ ;
          theBothStats->thePrefetchOps++;
          break;
        case API::Sim_Trans_Instr_Fetch:
          DBG_Assert(false);
        default:
          DBG_(Crit, ( << "unhandled transaction type.  Transaction follows:"));
          debugTransaction(mem_trans);
          DBG_Assert(false);
          break;
      }
    }
#if FLEXUS_TARGET_IS(v9)
    if (mem_trans->address_space == 0x71 ) {
      //BLK stores to ASI 71.
      //These stores update the data caches on hit, but do not allocate on
      //miss.  The best way to model these in
      theMemoryMessage.reqSize() = mem_trans->s.size;
      if(theSendNonAllocatingStores) theMemoryMessage.type() = MemoryMessage::NonAllocatingStoreReq;
      else {
       DBG_Assert(theMemoryMessage.type() = MemoryMessage::StoreReq);
      }
    }
#endif

    toL1D(theIndex, theMemoryMessage);

    return k_no_stall; //Never stalls
  }

  //Useful debugging stuff for tracing every instruction
  void debugTransaction(Simics::APIFwd::memory_transaction_t * mem_trans) {
    API::logical_address_t pc_logical = API::SIM_get_program_counter(theCPU);
    API::physical_address_t pc = API::SIM_logical_to_physical(theCPU, API::Sim_DI_Instruction, pc_logical);
    API::tuple_int_string_t * retval = API::SIM_disassemble(theCPU, pc , 0);
    (void)retval; //suppress unused variable warning

    DBG_(Tmp, SetNumeric( (ScaffoldIdx) theIndex)
         ( << "Mem Heir Instr: " << retval->string << " pc: " << &std::hex << pc)
        );

    if (API::SIM_mem_op_is_data(&mem_trans->s)) {
      if (API::SIM_mem_op_is_write(&mem_trans->s)) {
        DBG_(Tmp, SetNumeric( (ScaffoldIdx) theIndex)
             ( << "  Write @" << &std::hex << mem_trans->s.physical_address << &std::dec << '[' << mem_trans->s.size << ']'
               << " type=" << mem_trans->s.type
               << (  mem_trans->s.atomic ? " atomic" : "" )
               << (  mem_trans->s.may_stall ? "" : " no-stall" )
               << (  mem_trans->s.inquiry ? " inquiry" : "")
               << (  mem_trans->s.speculative ? " speculative" : "")
               << (  mem_trans->s.ignore ? " ignore" : "")
             )
            );
      } else {
        if ( mem_trans->s.type == API::Sim_Trans_Prefetch) {
          DBG_(Tmp, SetNumeric( (ScaffoldIdx) theIndex)
               ( << "  Prefetch @" << &std::hex << mem_trans->s.physical_address << &std::dec << '[' << mem_trans->s.size  << ']'
                 << " type=" << mem_trans->s.type
                 << (  mem_trans->s.atomic ? " atomic" : "" )
                 << (  mem_trans->s.may_stall ? "" : " no-stall" )
                 << (  mem_trans->s.inquiry ? " inquiry" : "")
                 << (  mem_trans->s.speculative ? " speculative" : "")
                 << (  mem_trans->s.ignore ? " ignore" : "")
               )
              );
        } else {
          DBG_(Tmp, SetNumeric( (ScaffoldIdx) theIndex)
               ( << "  Read @" << &std::hex << mem_trans->s.physical_address << &std::dec << '[' << mem_trans->s.size  << ']'
                 << " type=" << mem_trans->s.type
                 << (  mem_trans->s.atomic ? " atomic" : "" )
                 << (  mem_trans->s.may_stall ? "" : " no-stall" )
                 << (  mem_trans->s.inquiry ? " inquiry" : "")
                 << (  mem_trans->s.speculative ? " speculative" : "")
                 << (  mem_trans->s.ignore ? " ignore" : "")
               )
              );
        }
      }
    }
  }

};  // class SimicsTracerImpl

class SimicsTracer : public Simics::AddInObject <SimicsTracerImpl> {
  typedef Simics::AddInObject<SimicsTracerImpl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  static std::string className() {
    return "DecoupledFeeder";
  }
  static std::string classDescription() {
    return "Flexus's decoupled feeder.";
  }

  SimicsTracer() : base() { }
  SimicsTracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  SimicsTracer(SimicsTracerImpl * anImpl) : base(anImpl) {}
};

class DMATracerImpl {
  API::conf_object_t * theUnderlyingObject;
  API::conf_object_t * theMapObject;

  MemoryMessage theMemoryMessage;
  std::function< void(MemoryMessage &) > toDMA;

public:
  DMATracerImpl(API::conf_object_t * anUnderlyingObjec)
    : theUnderlyingObject(anUnderlyingObjec)
    , theMemoryMessage(MemoryMessage::WriteReq)
  {}

  // Initialize the tracer to the desired CPU
  void init(API::conf_object_t * aMapObject, std::function< void(MemoryMessage &) > aToDMA) {
    theMapObject = aMapObject;
    toDMA = aToDMA;

    API::attr_value_t attr;
    attr.kind = API::Sim_Val_Object;
    attr.u.object = theUnderlyingObject;

    /* Tell memory we have a mem hier */
    API::SIM_set_attribute(theMapObject, "timing_model", &attr);
  }

  void updateStats() {
  }

  API::cycles_t dma_mem_hier_operate(API::conf_object_t * space, API::map_list_t * map, API::generic_transaction_t * aMemTrans) {
    Simics::APIFwd::memory_transaction_t * mem_trans = reinterpret_cast<Simics::APIFwd::memory_transaction_t *>(aMemTrans);

    const int32_t k_no_stall = 0;

    //debugTransaction(mem_trans);
    mem_trans->s.block_STC = 1;

    if (API::SIM_mem_op_is_write(&mem_trans->s)) {
      DBG_( Verb, ( << "DMA To Mem: " << std::hex << mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size ) );
      theMemoryMessage.type() = MemoryMessage::WriteReq;
    } else {
      DBG_( Verb, ( << "DMA From Mem: " << std::hex << mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size ) );
      theMemoryMessage.type() = MemoryMessage::ReadReq;
    }

    theMemoryMessage.address() = PhysicalMemoryAddress( mem_trans->s.physical_address );
    theMemoryMessage.reqSize() = mem_trans->s.size;
    toDMA(theMemoryMessage);
    return k_no_stall; //Never stalls
  }

};  // class DMATracerImpl

class DMATracer : public Simics::AddInObject <DMATracerImpl> {
  typedef Simics::AddInObject<DMATracerImpl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  static std::string className() {
    return "DMATracer";
  }
  static std::string classDescription() {
    return "Flexus's DMA tracer.";
  }

  DMATracer() : base() { }
  DMATracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  DMATracer(DMATracerImpl * anImpl) : base(anImpl) {}
};

class SimicsTracerManagerImpl : public SimicsTracerManager {
  int32_t theNumCPUs;
  bool theClientServer;
  SimicsTracer * theTracers;
  DMATracer theDMATracer;
  std::function< void(int, MemoryMessage &) > toL1D;
  std::function< void(int, MemoryMessage &, uint32_t) > toL1I;
  std::function< void(MemoryMessage &) > toDMA;
  std::function< void(int, MemoryMessage &) > toNAW;

  bool theWhiteBoxDebug;
  int32_t  theWhiteBoxPeriod;
  bool theSendNonAllocatingStores;
public:
  SimicsTracerManagerImpl(int32_t aNumCPUs
                          , std::function< void(int, MemoryMessage &) > aToL1D
                          , std::function< void(int, MemoryMessage &, uint32_t) > aToL1I
                          , std::function< void(MemoryMessage &) > aToDMA
                          , std::function< void(int, MemoryMessage &) > aToNAW
                          , bool aWhiteBoxDebug
                          , int32_t aWhiteBoxPeriod
                          , bool aSendNonAllocatingStores
                         )
    : theNumCPUs(aNumCPUs)
    , theClientServer(false)
    , toL1D(aToL1D)
    , toL1I(aToL1I)
    , toDMA(aToDMA)
    , toNAW(aToNAW)
    , theWhiteBoxDebug(aWhiteBoxDebug)
    , theWhiteBoxPeriod(aWhiteBoxPeriod) 
    , theSendNonAllocatingStores(aSendNonAllocatingStores) {
    DBG_( Dev, ( << "Initializing SimicsTracerManager."  ) );
    theNumCPUs = aNumCPUs;

    //Dump translation caches
    Simics::APIFwd::SIM_flush_all_caches();

    detectClientServer();

    createTracers();
    createDMATracer();

    DBG_( Dev, ( << "Done initializing SimicsTracerManager."  ) );
  }

  virtual ~SimicsTracerManagerImpl() {}

  void setSimicsQuantum(int64_t aSwitchTime) {
    API::attr_value_t attr;
    attr.kind = API::Sim_Val_Integer;
    attr.u.integer = aSwitchTime;
    API::SIM_set_attribute(API::SIM_get_object("sim"), "cpu_switch_time", &attr);
  }

  void setSystemTick(double aTickFreq) {
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      API::attr_value_t attr;
      attr.kind = API::Sim_Val_Floating;
      attr.u.floating = aTickFreq;
      API::SIM_set_attribute(theTracers[i]->cpu(), "system_tick_frequency", &attr);
    }
  }

  void updateStats() {
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      theTracers[i]->updateStats();
    }
  }

  void enableInstructionTracing() {
    // This function call breaks for Oracle.
    // To enable Istream tracing, use "instruction-fetch-mode instruction-fetch-trace" in user-postload.simics instead.
    // For now, disable this function and turn Istream tracing on in user-postload.simics.
    DBG_( Crit, ( << "NOTE!!! Make sure you use \"instruction-fetch-mode instruction-fetch-trace\" in user-postload.simics to enable Istream tracing." ));
    return; //FIXME

    for (int32_t i = 0; i < theNumCPUs; ++i) {
      DBG_ ( VVerb, ( << "EnableIstreamTrace for cpu" << i ) );
      API::attr_value_t attr;
      attr.kind = API::Sim_Val_String;
      attr.u.string = "instruction-fetch-trace";
      API::set_error_t errorCode = API::SIM_set_attribute(theTracers[i]->cpu(), "instruction_fetch_mode", &attr);
      switch (errorCode) {
        case API::Sim_Set_Ok:
          DBG_ ( Crit, ( << ">>>>> OK" ));
          break;
        case API::Sim_Set_Need_Integer:
          DBG_ ( Crit, ( << ">>>>> Need_Integer" ));
          break;
        case API::Sim_Set_Need_Floating:
          DBG_ ( Crit, ( << ">>>>> Need_Floating" ));
          break;
        case API::Sim_Set_Need_String:
          DBG_ ( Crit, ( << ">>>>> Need_String" ));
          break;
        case API::Sim_Set_Need_List:
          DBG_ ( Crit, ( << ">>>>> Need_List" ));
          break;
        case API::Sim_Set_Need_Data:
          DBG_ ( Crit, ( << ">>>>> Need_Data" ));
          break;
        case API::Sim_Set_Need_Object:
          DBG_ ( Crit, ( << ">>>>> Need_Object" ));
          break;
        case API::Sim_Set_Object_Not_Found:
          DBG_ ( Crit, ( << ">>>>> Object_Not_Found" ));
          break;
        case API::Sim_Set_Interface_Not_Found:
          DBG_ ( Crit, ( << ">>>>> Interface_Not_Found" ));
          break;
        case API::Sim_Set_Illegal_Value:
          DBG_ ( Crit, ( << ">>>>> Illegal_Value" ));
          break;
        case API::Sim_Set_Illegal_Type:
          DBG_ ( Crit, ( << ">>>>> Illegal_Type" ));
          break;
        case API::Sim_Set_Illegal_Index:
          DBG_ ( Crit, ( << ">>>>> Illegal_Index" ));
          break;
        case API::Sim_Set_Attribute_Not_Found:
          DBG_ ( Crit, ( << ">>>>> Attribute_Not_Found" ));
          break;
        case API::Sim_Set_Not_Writable:
          DBG_ ( Crit, ( << ">>>>> Not_Writable" ));
          break;
        case API::Sim_Set_Ignored:
          DBG_ ( Crit, ( << ">>>>> Ignored" ));
          break;
        default:
          DBG_Assert ( false );
      }
    }
  }

private:
  void detectClientServer() {
    theClientServer = false;
    if (Simics::API::SIM_get_object("cpu0") == 0) {
      if (Simics::API::SIM_get_object("server_cpu0") == 0) {
        DBG_Assert( false, ( << "Cannot locate cpu0 or server_cpu0 objects." ) );
      } else {
        DBG_( Crit, ( << "Detected client-server simulation.  Connecting to server CPUs only." ) );
        theClientServer = true;
      }
    }
  }

  void createTracers() {
    theTracers = new SimicsTracer [ theNumCPUs ];

    //Create SimicsTracer Factory
    Simics::Factory<SimicsTracer> tracer_factory;
    API::conf_class_t * trace_class = tracer_factory.getSimicsClass();
    registerTimingInterface(trace_class);

    //Create SimicsTracer Objects
    const int32_t max_simics_cpu_no = 384;
    for (int32_t ii = 0, simics_cpu_no = 0; ii < theNumCPUs ; ++ii, ++simics_cpu_no) {
      std::string feeder_name("flexus-feeder");
      if (theNumCPUs > 1) {
        feeder_name += '-' + boost::padded_string_cast < 2, '0' > (ii);
      }
      theTracers[ii] = tracer_factory.create(feeder_name);

      std::string name("cpu");
      if (theClientServer) {
        name = "server_cpu";
      }

      API::conf_object_t * cpu = 0;
      for ( ; simics_cpu_no < max_simics_cpu_no; simics_cpu_no++) {
        std::string simics_name = name + boost::lexical_cast<std::string>(simics_cpu_no);

        DBG_( Crit, ( << "Connecting: " << simics_name ) );
        cpu = API::SIM_get_object( simics_name.c_str() );
        if (cpu != 0) {
          DBG_(Crit, ( << "DecoupledFeeder connects Flexus cpu" << ii << " to Simics object " << simics_name ));
          break;
        } else {
          DBG_(Crit, ( << "DecoupledFeeder cannot locate " << simics_name << " object. No such object in Simics" ));
        }
      }
      DBG_Assert(cpu != 0, ( << "DecoupledFeeder cannot locate " << name << " object. No such object in Simics" ));
      DBG_Assert(simics_cpu_no < max_simics_cpu_no, ( << "DecoupledFeeder cannot locate all Simics cpu objects." ));

      theTracers[ii]->init(cpu, ii, toL1D, toL1I, toNAW, theWhiteBoxDebug, theWhiteBoxPeriod, theSendNonAllocatingStores);
    }
  }

  void createDMATracer() {
    API::conf_object_t * dma_map_object = API::SIM_get_object( "dma_mem" );
    API::SIM_clear_exception();
    if (! dma_map_object) {
      bool client_server = false;
      DBG_( Dev, ( << "Creating DMA map object" ) );
      API::conf_object_t * cpu0_mem = API::SIM_get_object("cpu0_mem");
      API::conf_object_t * cpu0 = API::SIM_get_object("cpu0");
      if ( ! cpu0_mem ) {
        client_server = true;
        cpu0_mem = API::SIM_get_object("server_cpu0_mem");
        cpu0 = API::SIM_get_object("server_cpu0");
        if (! cpu0_mem) {
          DBG_( Crit, ( << "Unable to connect DMA because there is no cpu0_mem." ) );
          return;
        }
      }

      API::attr_value_t map_key = API::SIM_make_attr_string("map");
      API::attr_value_t map_value = API::SIM_get_attribute(cpu0_mem, "map");
      API::attr_value_t map_pair = API::SIM_make_attr_list(2, map_key, map_value);
      API::attr_value_t map_list = API::SIM_make_attr_list(1, map_pair);
      API::conf_class_t * memory_space = API::SIM_get_class( "memory-space" );
      dma_map_object = SIM_create_object( memory_space, "dma_mem", map_list );
      DBG_Assert( dma_map_object );

      API::conf_class_t * schizo = API::SIM_get_class( "serengeti-schizo" );
      API::attr_value_t dma_attr;
      dma_attr.kind = API::Sim_Val_Object;
      dma_attr.u.object = dma_map_object;

#if SIM_VERSION > 1300
      API::attr_value_t all_objects = Simics::API::SIM_get_all_objects();
      DBG_Assert(all_objects.kind == Simics::API::Sim_Val_List);
      for (int32_t i = 0; i < all_objects.u.list.size; ++i) {
        if (all_objects.u.list.vector[i].u.object->class_data == schizo && API::SIM_get_attribute( all_objects.u.list.vector[i].u.object, "queue").u.object == cpu0 ) {
          API::SIM_set_attribute(all_objects.u.list.vector[i].u.object, "memory_space", &dma_attr );
        }
      }
#else
      API::object_vector_t all_objects = API::SIM_all_objects();
      for (int32_t i = 0; i < all_objects.size; ++i) {
        if (all_objects.vec[i]->class_data == schizo && API::SIM_get_attribute( all_objects.vec[i], "queue").u.object == cpu0 ) {
          API::SIM_set_attribute(all_objects.vec[i], "memory_space", &dma_attr );
        }
      }
#endif
    }

    //Create SimicsTracer Factory
    Simics::Factory<DMATracer> tracer_factory;
    API::conf_class_t * trace_class = tracer_factory.getSimicsClass();
    registerDMAInterface(trace_class);

    std::string tracer_name("dma-tracer");
    theDMATracer = tracer_factory.create(tracer_name);
    DBG_( Crit, ( << "Connecting to DMA memory map" ) );
    theDMATracer->init(dma_map_object, toDMA);
  }

  void registerTimingInterface(Simics::API::conf_class_t * trace_class) {
    API::timing_model_interface_t * timing_interface;
    timing_interface = new API::timing_model_interface_t(); //LEAKS - Need to fix
    timing_interface->operate = &Simics::make_signature_from_addin_fn2<API::operate_func_t>::with<SimicsTracer, SimicsTracerImpl, &SimicsTracerImpl::trace_mem_hier_operate>::trampoline;
    API::SIM_register_interface(trace_class, "timing-model", timing_interface);
  }

  void registerDMAInterface(Simics::API::conf_class_t * trace_class) {
    API::timing_model_interface_t * timing_interface;
    timing_interface = new API::timing_model_interface_t(); //LEAKS - Need to fix
    timing_interface->operate = &Simics::make_signature_from_addin_fn2<API::operate_func_t>::with<DMATracer, DMATracerImpl, &DMATracerImpl::dma_mem_hier_operate>::trampoline;
    API::SIM_register_interface(trace_class, "timing-model", timing_interface);
  }

};

SimicsTracerManager * SimicsTracerManager::construct(int32_t aNumCPUs
    , std::function< void(int, MemoryMessage &) > toL1D
    , std::function< void(int, MemoryMessage &, uint32_t) > toL1I
    , std::function< void(MemoryMessage &) > toDMA
    , std::function< void(int, MemoryMessage &) > toNAW
    , bool aWhiteBoxDebug
    , int32_t aWhiteBoxPeriod
    , bool aSendNonAllocatingStores
                                                    ) {
  return new SimicsTracerManagerImpl(aNumCPUs, toL1D, toL1I, toDMA, toNAW, aWhiteBoxDebug, aWhiteBoxPeriod, aSendNonAllocatingStores);
}

} //end nDecoupledFeeder
