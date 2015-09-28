
#include <iostream>
#include <fstream>
#include <sstream>

#include <set>
#include <list>

#include <core/boost_extensions/padded_string_cast.hpp>

#include <core/types.hpp>
#include <core/debug/debug.hpp>

#include <core/simics/configuration_api.hpp>
#include <core/simics/mai_api.hpp>

#include <components/DecoupledFeederV/SimicsTracer.hpp>

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

namespace nDecoupledFeederV {

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
  boost::function< void(int, MemoryMessage &) > toL1D;
  boost::function< void(int, MemoryMessage &, uint32_t) > toL1I;
  boost::function< void(int, MemoryMessage &) > toNAW;

public:
  SimicsTracerImpl(API::conf_object_t * anUnderlyingObject)
    : theUnderlyingObject(anUnderlyingObject)
    , theMemoryMessage(MemoryMessage::LoadReq)
  {}

  API::conf_object_t * cpu() const {
    return theCPU;
  }

  // Initialize the tracer to the desired CPU
  void init(API::conf_object_t * aCPU, index_t anIndex, boost::function< void(int, MemoryMessage &) > aToL1D, boost::function< void(int, MemoryMessage &, uint32_t) > aToL1I, boost::function< void(int, MemoryMessage &) > aToNAW) {
    theCPU = aCPU;
    theIndex = anIndex;
    toL1D = aToL1D;
    toL1I = aToL1I;
    toNAW = aToNAW;

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

    //debugTransaction(mem_trans);

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
      return k_no_stall; //Not a memory operation
    }
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
  static std::string className() { return "DecoupledFeederV"; }
  static std::string classDescription() { return "Flexus's decoupled feeder."; }

  SimicsTracer() : base() { }
  SimicsTracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  SimicsTracer(SimicsTracerImpl * anImpl) : base(anImpl) {}
};

class DMATracerImpl {
  API::conf_object_t * theUnderlyingObject;
  API::conf_object_t * theMapObject;
  int32_t theVM;

  MemoryMessage theMemoryMessage;
  boost::function< void(int, MemoryMessage &) > toDMA;

public:
  DMATracerImpl(API::conf_object_t * anUnderlyingObjec)
    : theUnderlyingObject(anUnderlyingObjec)
    , theMemoryMessage(MemoryMessage::WriteReq)
  {}

  // Initialize the tracer to the desired CPU
  void init(API::conf_object_t * aMapObject, int32_t aVM, boost::function< void(int, MemoryMessage &) > aToDMA) {
    theMapObject = aMapObject;
    toDMA = aToDMA;
    theVM = aVM;

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
    toDMA(theVM, theMemoryMessage);
    return k_no_stall; //Never stalls
  }

};  // class DMATracerImpl

class DMATracer : public Simics::AddInObject <DMATracerImpl> {
  typedef Simics::AddInObject<DMATracerImpl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  static std::string className() { return "DMATracer"; }
  static std::string classDescription() { return "Flexus's DMA tracer."; }

  DMATracer() : base() { }
  DMATracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  DMATracer(DMATracerImpl * anImpl) : base(anImpl) {}
};

class SimicsTracerManagerImpl : public SimicsTracerManager {
  int32_t theNumCPUs;
  int32_t theNumVMs;
  bool theVirtualized;
  bool theClientServer;
  SimicsTracer * theTracers;
  DMATracer * theDMATracer;
  boost::function< void(int, MemoryMessage &) > toL1D;
  boost::function< void(int, MemoryMessage &, uint32_t) > toL1I;
  boost::function< void(int, MemoryMessage &) > toDMA;
  boost::function< void(int, MemoryMessage &) > toNAW;

public:
  SimicsTracerManagerImpl(int32_t aNumCPUs, boost::function< void(int, MemoryMessage &) > aToL1D, boost::function< void(int, MemoryMessage &, uint32_t) > aToL1I, boost::function< void(int, MemoryMessage &) > aToDMA, boost::function< void(int, MemoryMessage &) > aToNAW)
    : theNumCPUs(aNumCPUs)
    , theNumVMs(1)
    , theVirtualized(false)
    , theClientServer(false)
    , toL1D(aToL1D)
    , toL1I(aToL1I)
    , toDMA(aToDMA)
    , toNAW(aToNAW) {
    DBG_( Dev, ( << "Initializing SimicsTracerManager for " << aNumCPUs << " cpus."  ) );
    theNumCPUs = aNumCPUs;

    //Dump translation caches
    Simics::APIFwd::SIM_flush_all_caches();

    detectVirtualized();

    if (!theVirtualized) {
      detectClientServer();
    }

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
  void detectVirtualized() {
    theVirtualized = false;
    if ((Simics::API::SIM_get_object("machine0_cpu0") != 0)
        || (Simics::API::SIM_get_object("machine0_server_cpu0") != 0)) {
      theVirtualized = true;
    }
  }

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

    std::list<API::conf_object_t *> cpu_list;

    if (theVirtualized) {
      int32_t maxNumVMs = 8; // ?????????????? WHY DO WE HAVE THIS VARIABLE SET TO EIGHT ????????????? 
      int32_t ii = 0;
      int32_t vm = 0;
      for (; vm < maxNumVMs && ii < theNumCPUs; vm++) {
        bool client_server = false;
        std::string name("machine");
        name += boost::lexical_cast<std::string>(vm);
        name += "_cpu0";
        //DBG_(Crit, ( << "Looking for " << name ));
        if (API::SIM_get_object(name.c_str()) == 0) {
          name = "machine";
          name += boost::lexical_cast<std::string>(vm);
          name += "_server_cpu0";
          //DBG_(Crit, ( << "Looking for " << name ));
          if (API::SIM_get_object(name.c_str()) != 0) {
            client_server = true;
          } else {
            DBG_Assert(false, ( << "Unable to find 'machine" << vm << "_cpu0' or 'machine" << vm << "_server_cpu0" ));
          }
        }
        int32_t num_remaining = theNumCPUs - ii;
        for (int32_t i = 0; i < num_remaining && ii < theNumCPUs; i++, ii++) {
          std::string name("machine");
          name += boost::lexical_cast<std::string>(vm);
          if (client_server) {
            name += "_server";
          }
          name += "_cpu" + boost::lexical_cast<std::string>(i);

          //DBG_(Crit, ( << "Looking for " << name ));
          API::conf_object_t * cpu = API::SIM_get_object( name.c_str() );
          if (cpu == 0) {
            break;
          }
          cpu_list.push_back(cpu);
        }
      }

      DBG_Assert(ii == theNumCPUs, ( << "Only found " << ii << " CPUs not " << theNumCPUs ));
      theNumVMs = vm;

      std::string line;
      std::string str ("\"");
      std::string str2 (":threads\"");
      std::string str3 ("-mapper:manual_organization\"");
      std::string str4 ("-mapper:organization\"");
      std::ifstream myfile ("user-postload.simics");
      std::stringstream ss (std::stringstream::in | std::stringstream::out);
      char *charThreads,*charManual;
      int found,start,end,numThreads,manual;

      // We read the user-postload.simics to read the number of threads and the way of manual organization of the VMs
      if (myfile.is_open()){
        DBG_(Dev, ( << "Reading user-postload.simics"));
        while ( myfile.good() ){
          getline (myfile,line);
          ss << line << std::endl;
        }
        myfile.close();
      } 
      else
        DBG_Assert(false, ( << "Cannot read the user-postload.simics"));

      // Search for the number of threads and read the value of this parameter
      found=(ss.str()).find(str2)+str2.length();
      start=(ss.str()).find(str,found);
      end=(ss.str()).find(str,start+1);
      std::string strThreads = (ss.str()).substr(start+1,end-start-1);
      charThreads = new char[strThreads.length() + 1];
      strcpy(charThreads, strThreads.c_str());
      numThreads = atoi(charThreads);
      delete charThreads; 
      DBG_(Dev, ( << "Number of threads per core: " << numThreads));

      // Search whether we have manual organization or not
      found=(ss.str()).find(str3)+str3.length();
      start=(ss.str()).find(str,found);
      end=(ss.str()).find(str,start+1);
      std::string strManual = (ss.str()).substr(start+1,end-start-1);
      charManual = new char[strManual.length() + 1];
      strcpy(charManual, strManual.c_str());
      manual = atoi(charManual);
      delete charManual;
      
      if (manual == 0){
        DBG_(Dev, ( << "Automatic organization of equal-size VMs"));
        int cores_per_vm = (theNumCPUs/numThreads) / theNumVMs;
        int sqrt_cores_per_vm = (int)std::ceil(std::sqrt(cores_per_vm));
        int sqrt_numVMs = (int)std::ceil(std::sqrt(theNumVMs));
        int vms_per_row,vms_per_col,num_vm_rows,num_vm_cols;
        if (sqrt_numVMs*sqrt_numVMs == theNumVMs){ // Square number of VMs
          vms_per_row = (int)std::ceil(std::sqrt(theNumVMs));
          vms_per_col = vms_per_row;
          if (sqrt_cores_per_vm*sqrt_cores_per_vm == cores_per_vm){ // Square number of cores per VM
            num_vm_rows = (int)std::ceil(std::sqrt(cores_per_vm));
            num_vm_cols = num_vm_rows;
          } 
          else { // Non square number of cores per VM. Dividing by 2 it becomes square number. 
                 // e.g. 32 --> 32/2 = 16 and hence 8 rows of 4 cores each. this has to be done like that 
	         // so the VMs are mapped correctly according to the chip topology. 	
            num_vm_cols = (int) std::ceil(std::sqrt(cores_per_vm/2));
            num_vm_rows = cores_per_vm/num_vm_cols;
          }
        }
        else { // Non square number of VMs. Dividing by it becomes square number.
       	       // e.g. 8 --> 8/2 = 4 and hence 2 rows of 4 VMs each. 
          vms_per_row = (int)std::ceil(std::sqrt(theNumVMs/2));
          vms_per_col = theNumVMs/vms_per_row;
          if (sqrt_cores_per_vm*sqrt_cores_per_vm == cores_per_vm){ // Square number of cores per VM
            num_vm_rows = (int)std::ceil(std::sqrt(cores_per_vm));
            num_vm_cols = num_vm_rows;
          }
          else { // Non square number of cores per VM. Dividing by 2 it becomes square number. 
	         // e.g. 32 --> 32/2 = 16 and hence 4 rows of 8 cores each.  
            num_vm_rows = (int) std::ceil(std::sqrt(cores_per_vm/2));
            num_vm_cols = cores_per_vm/num_vm_rows;
          }
        }  
        int num_rows = num_vm_rows*vms_per_col;
        for (int32_t i = 0; i < theNumCPUs; i++) {
          int j = i / numThreads;
          int vm = j / cores_per_vm;
          int core = j % cores_per_vm;
          int row_in_vm = core / (num_vm_cols);
          int col_in_vm = core % (num_vm_cols);
          int row_of_vm = vm / vms_per_row;
          int col_of_vm = vm % vms_per_row;
          int abs_row = row_in_vm + row_of_vm*num_vm_rows;
          int abs_col = col_in_vm + col_of_vm*num_vm_cols;
          int new_index = abs_row*(theNumCPUs/num_rows) + abs_col*numThreads + i%numThreads;
          DBG_(Crit, ( << "DecoupledFeederV connects Flexus cpu" << new_index << " to Simics object " << cpu_list.front()->name ));
          std::string feeder_name("flexus-feeder");
          if (new_index >= 1) {
            feeder_name += '-' + boost::padded_string_cast < 2, '0' > (new_index);
          }
          theTracers[new_index] = tracer_factory.create(feeder_name);
          theTracers[new_index]->init(cpu_list.front(), new_index, toL1D, toL1I, toNAW);
          cpu_list.pop_front();
        }
      }
      else {
        DBG_(Dev, ( << "Manual organization of VMs according to the configuration file"));
        std::vector<int> organization(theNumCPUs/numThreads);
        int i=0,j=0;
        // Search for the organization parameter
        found=(ss.str()).find(str4)+str4.length();
        start=(ss.str()).find(str,found);
    	end=(ss.str()).find(str,start+1);
    	std::string strOrg = (ss.str()).substr(start+1,end-start-1);
    	char *charOrg = new char[strOrg.length()+1];
    	strcpy(charOrg, strOrg.c_str());
    	std::string *tokens = new std::string[theNumVMs];
    	char *temp=strtok(charOrg,";");
    	while (temp!=nullptr){
      	  tokens[i++]=temp;
      	  temp = strtok (nullptr, ";");
        }
        delete charOrg;
        // the organization vector will have the new indices and the machine vector will have them in muliple rows; one for each VM
        for (i=0;i<theNumVMs;i++){
          char *charMachine = new char[tokens[i].length()+1];
          strcpy(charMachine,tokens[i].c_str());
          temp=strtok(charMachine,",");
          while (temp!=nullptr){
            organization[j++]=atoi(temp);
            temp = strtok (nullptr,",");
          }
          delete charMachine;
        }

        for (int32_t i = 0; i < theNumCPUs; i++) {
          int new_index = organization[i/numThreads]*numThreads+i%numThreads;
          DBG_(Crit, ( << "DecoupledFeederV connects Flexus cpu" << new_index << " to Simics object " << cpu_list.front()->name ));
          std::string feeder_name("flexus-feeder");
          if (new_index >= 1) {
            feeder_name += '-' + boost::padded_string_cast < 2, '0' > (new_index);
          }
          theTracers[new_index] = tracer_factory.create(feeder_name);
          theTracers[new_index]->init(cpu_list.front(), new_index, toL1D, toL1I, toNAW);
          cpu_list.pop_front();
	}
      }
    } else {
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
            DBG_(Crit, ( << "DecoupledFeederV connects Flexus cpu" << ii << " to Simics object " << simics_name ));
            break;
          } else {
            DBG_(Crit, ( << "DecoupledFeederV cannot locate " << simics_name << " object. No such object in Simics" ));
          }
        }
        DBG_Assert(cpu != 0, ( << "DecoupledFeederV cannot locate " << name << " object. No such object in Simics" ));
        DBG_Assert(simics_cpu_no < max_simics_cpu_no, ( << "DecoupledFeederV cannot locate all Simics cpu objects." ));

        theTracers[ii]->init(cpu, ii, toL1D, toL1I, toNAW);
      }
    }
  }

  void createDMATracer() {

    if (!theVirtualized) {
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
      theDMATracer = new DMATracer[1];
      theDMATracer[0] = tracer_factory.create(tracer_name);
      DBG_( Crit, ( << "Connecting to DMA memory map" ) );
      theDMATracer[0]->init(dma_map_object, 0, toDMA);
    } else {
      theDMATracer = new DMATracer[theNumVMs];
      for (int32_t vm = 0; vm < theNumVMs; vm++) {
        std::string dma_map_name = "dma_mem" + boost::lexical_cast<std::string>(vm);
        API::conf_object_t * dma_map_object = API::SIM_get_object( dma_map_name.c_str() );
        API::SIM_clear_exception();
        if (! dma_map_object) {
          bool client_server = false;
          DBG_( Dev, ( << "Creating DMA map object" ) );
          std::string cpu_name = "machine" + boost::lexical_cast<std::string>(vm);
          cpu_name += "_cpu0";
          std::string cpu_mem_name = cpu_name + "_mem";
          API::conf_object_t * cpu0_mem = API::SIM_get_object( cpu_mem_name.c_str() );
          API::conf_object_t * cpu0 = API::SIM_get_object( cpu_name.c_str() );
          if ( ! cpu0_mem ) {
            cpu_name = "machine" + boost::lexical_cast<std::string>(vm);
            cpu_name += "_server_cpu0";
            cpu_mem_name = "machine" + boost::lexical_cast<std::string>(vm);
            cpu_mem_name += "_server_cpu0_mem";
            client_server = true;
            cpu0_mem = API::SIM_get_object(cpu_mem_name.c_str());
            cpu0 = API::SIM_get_object(cpu_name.c_str());
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
          dma_map_object = SIM_create_object( memory_space, dma_map_name.c_str(), map_list );
          DBG_Assert( dma_map_object );

          API::conf_class_t * schizo = API::SIM_get_class( "serengeti-schizo" );
          API::attr_value_t dma_attr;
          dma_attr.kind = API::Sim_Val_Object;
          dma_attr.u.object = dma_map_object;

#if SIM_VERSION > 1300
          API::attr_value_t all_objects = Simics::API::SIM_get_all_objects();
          DBG_Assert(all_objects.kind == Simics::API::Sim_Val_List);
          for (int32_t i = 0; i < all_objects.u.list.size; ++i) {
            API::attr_value_t queue_attr = API::SIM_get_attribute( all_objects.u.list.vector[i].u.object, "queue");
            if (all_objects.u.list.vector[i].u.object->class_data == schizo && queue_attr.u.object == cpu0 ) {
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

        std::string tracer_name = "dma-tracer" + boost::lexical_cast<std::string>(vm);
        theDMATracer[vm] = tracer_factory.create(tracer_name);
        DBG_( Crit, ( << "Connecting to DMA memory map" ) );
        theDMATracer[vm]->init(dma_map_object, vm, toDMA);
      }
    }
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

SimicsTracerManager * SimicsTracerManager::construct(int32_t aNumCPUs, boost::function< void(int, MemoryMessage &) > toL1D, boost::function< void(int, MemoryMessage &, uint32_t) > toL1I, boost::function< void(int, MemoryMessage &) > toDMA, boost::function< void(int, MemoryMessage &) > toNAW) {
  return new SimicsTracerManagerImpl(aNumCPUs, toL1D, toL1I, toDMA, toNAW);
}

} //end nDecoupledFeederV
