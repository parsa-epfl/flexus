/*
   SimicsTraceFeederImpl.hpp

   Stephen Somogyi
   Carnegie Mellon University

   17Mar2003 - initial version (derived from Virtutech's trace.c)

   This is both a module for Simics and Flexus.  As a Simics module,
   it snoops all memory accesses.  These accesses are converted into
   Flexus trace instructions.  The module is also a Flexus
   fetch component, and injects these instructions into the Flexus
   pipeline.

*/

#include <iomanip>

#include <boost/mpl/alias.hpp>
#include <boost/mpl/list.hpp>
#include <boost/weak_ptr.hpp>

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

#include <core/Simics/configuration_api.hpp>
#include <core/Simics/mai_api.hpp>

#define FLEXUS_BEGIN_COMPONENT SimicsTraceFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace Flexus {
namespace SimicsTraceFeeder {

typedef AssertOnly AssertionDebugSetting;

using namespace Flexus::Core;
using namespace Flexus::MemoryPool;
using namespace Flexus::SharedTypes;

using namespace Flexus::Simics::API;

using std::setbase;

using boost::weak_ptr;
using boost::intrusive_ptr;

typedef ArchitecturalInstruction Instruction;

ostream & operator <<(ostream & anOstream, const Command & aCommand) {
  char const * command_types[] =
  { "Oracle Next Instruction" };
  anOstream << command_types[0] << ": ";
  return anOstream;
}

struct SimicsTraceConsumer {
  enum FeederStatus {
    Idle,        // no operation ready for fetch; no transaction in progress
    Ready,       // the operation is waiting to be executed
    InProgress,  // the transaction is in progress
    Complete     // the current operation has completed
  };

  //Current status
  FeederStatus theStatus;
  intrusive_ptr<Instruction> theReadyInstruction;

  SimicsTraceConsumer()
    : theStatus(Idle)
  {}

  bool isIdle() const {
    return theStatus == Idle;
  }

  bool isReady() const {
    return theStatus == Ready;
  }

  bool isInProgress() const {
    return theStatus == InProgress;
  }

  bool isComplete() const {
    return theStatus == Complete;
  }

  void acknowledgeComplete() {
    FLEXUS_ASSERT(AssertionDebugSetting, (isComplete()));
    theStatus = Idle;
  }

  void releaseInstruction(Instruction const & anInstruction) {
    FLEXUS_ASSERT(AssertionDebugSetting, (isInProgress()));
    FLEXUS_ASSERT(AssertionDebugSetting, (&anInstruction == theReadyInstruction.get()));
    theStatus = Complete;
  }

  void consumeInstruction(intrusive_ptr<Instruction> anInstruction) {
    FLEXUS_ASSERT(AssertionDebugSetting, (isIdle()));
    theStatus = Ready;
    theReadyInstruction = anInstruction;
  }
};

class SimicsX86MemoryOpImpl : public SimicsX86MemoryOp { /*, public UseMemoryPool<SimicsX86MemoryOpImpl , LocalResizing>*/
  // A memory operation needs to know its address, data, and operation type
  PhysicalMemoryAddress thePhysicalAddress;
  PhysicalMemoryAddress thePhysicalInstructionAddress;    // i.e. the PC
  data_word theData;
  eOpType theOperation;
  SimicsTraceConsumer & theConsumer;
  bool theReleaseFlag;
  bool thePerformed;

public:
  explicit SimicsX86MemoryOpImpl(SimicsTraceConsumer & aConsumer)
    : SimicsX86MemoryOp(this)
    , thePhysicalAddress(0)
    , thePhysicalInstructionAddress(0)
    , theData(0)
    , theOperation(Read)
    , theConsumer(aConsumer)
    , theReleaseFlag(false)
    , thePerformed(false)
  {}

  //SimicsX86MemoryOp Interface functions
  //Query for type of operation
  bool isMemory() {
    return true;
  }
  bool isLoad() {
    return (theOperation == Read);
  }
  bool isStore() {
    return (theOperation == Write);
  }
  bool isPerformed() {
    return thePerformed;
  }

  void release() {
    FLEXUS_ASSERT(AssertionDebugSetting, (theReleaseFlag == false));
    theReleaseFlag = true;
    theConsumer.releaseInstruction(*this);
  }

  //SimicsX86MemoryOpImpl Interface functions
  //Set operation types
  void setIsStore(bool isStore) {
    if (isStore) {
      theOperation = Write;
    } else {
      theOperation = Read;
    }
  }

  //Set the address for a memory operation
  void setAddress(PhysicalMemoryAddress addr) {
    thePhysicalAddress = addr;
  }
  void setPhysInstAddress(PhysicalMemoryAddress addr) {
    thePhysicalInstructionAddress = addr;
  }

  //Mark the instruction as performed
  void perform() {
    thePerformed = true;
  }

  //Get the address for a memory operation
  PhysicalMemoryAddress physicalMemoryAddress() const {
    return thePhysicalAddress;
  }

}; //End SimicsX86MemoryOpImpl

inline SimicsX86MemoryOpImpl * SimicsX86MemoryOp::getImpl() {
  return theImpl;
}

//Forwarding functions from SimicsX86MemoryOp
inline bool SimicsX86MemoryOp::isMemory() const {
  return theImpl->isMemory();
}
inline bool SimicsX86MemoryOp::isLoad() const {
  return theImpl->isLoad();
}
inline bool SimicsX86MemoryOp::isStore() const {
  return theImpl->isStore();
}
inline bool SimicsX86MemoryOp::isBranch() const {
  return false;
}
inline bool SimicsX86MemoryOp::isInterrupt() const {
  return false;
}
inline bool SimicsX86MemoryOp::requiresSync() const {
  return false;
}
inline PhysicalMemoryAddress SimicsX86MemoryOp::physicalMemoryAddress() const {
  return theImpl->physicalMemoryAddress();
}
inline void SimicsX86MemoryOp::execute() {}
inline void SimicsX86MemoryOp::perform() {
  theImpl->perform();
}
inline void SimicsX86MemoryOp::commit() {}
inline void SimicsX86MemoryOp::squash() {}
inline void SimicsX86MemoryOp::release() {
  theImpl->release();
}
inline bool SimicsX86MemoryOp::isValid() const {
  return true;
}
inline bool SimicsX86MemoryOp::isFetched() const {
  return true;
}
inline bool SimicsX86MemoryOp::isDecoded() const {
  return true;
}
inline bool SimicsX86MemoryOp::isExecuted() const {
  return true;
}
inline bool SimicsX86MemoryOp::isPerformed() const {
  return theImpl->isPerformed();
}
inline bool SimicsX86MemoryOp::isCommitted() const {
  return true;
}
inline bool SimicsX86MemoryOp::isSquashed() const {
  return false;
}
inline bool SimicsX86MemoryOp::isExcepted() const {
  return false;
}
inline bool SimicsX86MemoryOp::wasTaken() const {
  return false;
}
inline bool SimicsX86MemoryOp::wasNotTaken() const {
  return false;
}
inline bool SimicsX86MemoryOp::canExecute() const {
  return true;
}
inline bool SimicsX86MemoryOp::canPerform() const {
  return true;
}

ostream & operator <<(ostream & anOstream, const SimicsX86MemoryOp & aMemOp) {
  anOstream << "Not implemented yet for SimicsX86MemoryOp!" << std::endl;
  return anOstream;
}

/*****************************************************************
 * Types for the "Simics" part of the implementation.
 *****************************************************************/

typedef enum {
  TR_Reserved = 0, TR_Data = 1, TR_Instruction = 2, TR_Exception = 3,
  TR_Execute = 4, TR_Reserved_2, TR_Reserved_3, TR_Reserved_4,
  TR_Reserved_5, TR_Reserved_6, TR_Reserved_7, TR_Reserved_8,
  TR_Reserved_9, TR_Reserved_10, TR_Reserved_11, TR_Reserved_12
} trace_type_t;

/*****************************************************************
 * Here the "Simics" part of the implementation begins.
 *****************************************************************/

class SimicsTracerImpl  {
public:
  int32_t theTestAttribute;

  void printStats() {
    std::cout << "Statistics would print here!" << std::endl;
  }

private:
  conf_object_t * theUnderlyingObject;

  // is tracing enabled?
  int32_t trace_enabled;

  // is tracing paused?
  int32_t thePauseFlag;

  //LocalResizingPool<SimicsX86MemoryOpImpl> theInstructionPool;

  // do we trace instructions? (otherwise just data accesses)
  int32_t theTraceInstructionFlag;

  // copy the data of each instruction?
  int32_t theIncludeDataFlag;

#ifdef TARGET_X86
  int32_t print_access_type;
  int32_t print_memory_type;
#endif

  // the current privilege mode
  int32_t priv_mode;

  // the current PID
  int32_t curr_pid;

  // a breakpoint ID so it can be unregistered later
  breakpoint_id_t cntx_bkpt;

  SimicsTraceConsumer * theConsumer;

public:
  //Default Constructor, creates a SimicsTracer that is not connected
  //to an underlying Simics object.
  SimicsTracerImpl(conf_object_t * anUnderlyingObject)
    : trace_enabled(1)
    , thePauseFlag(0)
    , theTraceInstructionFlag(0)
    , theIncludeDataFlag(1)
    , theConsumer(0) {
    pr("[flexustrace module] Created SimicsTracer\n");
    theUnderlyingObject = anUnderlyingObject;

    conf_object_t * phys_mem;
    attr_value_t attr;

    SIM_dump_caches();

    attr.kind = Sim_Val_String;
    attr.u.string = ((conf_object_t *)Simics::Processor::current())->name;

    SIM_set_attribute(theUnderlyingObject, "queue", &attr);

    attr.kind = Sim_Val_Object;
    attr.u.object = theUnderlyingObject;

    /* Tell memory we have a mem hier */
    phys_mem = SIM_get_attribute(Simics::Processor::current(), "physical_memory").u.object;
    SIM_set_attribute(phys_mem, "timing_model", &attr);

    //This is a performance killer
    //#if FLEXUS_TARGET_IS(x86)
    //  attr.kind = Sim_Val_String;
    //  attr.u.string = "instruction-fetch-trace";
    //  SIM_set_attribute(SIM_get_object("sim"), "instruction_profile_mode", &attr);
    //#endif
  }

  void setTraceConsumer(SimicsTraceConsumer & aConsumer) {
    theConsumer = &aConsumer;
  }

  cycles_t trace_mem_hier_operate(conf_object_t * space, map_list_t * map, generic_transaction_t * aMemTrans) {
    //NOTE: Multiple return paths
    const int32_t k_no_stall = 0;
    const int32_t k_call_me_next_cycle = 1;
    memory_transaction_t * mem_trans = reinterpret_cast<memory_transaction_t *>(aMemTrans);

    FLEXUS_ASSERT(AssertionDebugSetting, (theConsumer)); //We have a consumer connected

    //Ensure that we see future accesses to this block
    mem_trans->s.block_STC = 1;

    //Check for a variety of quick exit conditions.
    if (thePauseFlag) return k_no_stall;

    // Simics will call this function after the previous returned latency.
    //If we currently have a completed transaction to report to simics
    if ( theConsumer->isComplete() ) {
      // We're all done - return zero cycle latency
      theConsumer->acknowledgeComplete();
      return k_no_stall;
    }
    //If we are in the middle of processing a transaction, tell Simics it's not done yet
    if (! theConsumer->isIdle()) {
      // Ask for a callback next cycle
      return k_call_me_next_cycle;
    }

    // otherwise we must be in the Idle state, and ready to handle a  operation

    //See if this is an instruciton or data access
    if (SIM_mem_op_is_instruction(&mem_trans->s)) {
      //Exit if instruction tracing is shut off
      if (!theTraceInstructionFlag)
        return k_no_stall;

#if 0 //Instruction tracing currently disabled
      //This code needs fixing uo
      /* record the operation */
      current_entry.trace_type = TR_Instruction;
      current_entry.read_or_write = Sim_RW_Read;

#if FLEXUS_TARGET_IS(x86)
      {
        /* Memory operations that cross pages are divided into
           two accesses by Simics. We may want to disassemble
           instructions so we store them in one trace entry.
           mem_trans->page_cross is 0 for non-crossing, 1 for
           1:st half and, 2 for 2:nd half. */

        char * txt = (char *)current_entry.value.text;
        if (mem_trans->page_cross == 2) {
          SIM_c_get_mem_op_value_buf(&mem_trans->s, txt + current_entry.bytes);
          current_entry.bytes += mem_trans->s.size;
          //Deal with page_cross == 1
          //if (mem_trans->page_cross == 1 && SIM_mem_op_is_instruction(&mem_trans->s))
          //  return 0;
        } else {
          SIM_c_get_mem_op_value_buf(&mem_trans->s, txt);
        }
      }
#else
      current_entry.value.text = SIM_get_mem_op_value(&mem_trans->s);
#endif
#endif //0 - disable instruction tracing

    } else if (SIM_mem_op_is_data(&mem_trans->s)) {

      //Check to see if data tracing is enabled
      if (!theIncludeDataFlag)
        return k_no_stall;

      // use the 'required' flag to determine if this is a memory access that
      // we want to trace... begin by assuming that all accesses are interesting
      // This is only used for the x86 target.  It will optimize away for other
      // targets
      int32_t required = 1;

#if FLEXUS_TARGET_IS(x86)
      // Simics x86 supports a multitude of instruction types - by default,
      // assume we don't care about all of them, and just set the 'required'
      // flag for the ones we are interested in
      required = 0;
      switch (mem_trans->access_type) {
        case X86_Vanilla:
          // standard data access - these we definitely care about
          required = 1;
          break;
        case X86_Instruction:
        case X86_Other:
        case X86_Clflush:
        case X86_Fpu_Env:
        case X86_Fpu_State:
        case X86_Idt:
        case X86_Gdt:
        case X86_Ldt:
        case X86_Task_Segment:
        case X86_Task_Switch:
        case X86_Far_Call_Parameter:
        case X86_Stack:
        case X86_Pml4:
        case X86_Pdp:
        case X86_Pd:
        case X86_Pt:
        case X86_Sse:
        case X86_Fpu:
        case X86_Access_Simple:
        case X86_Microcode_Update:
        case X86_Non_Temporal:
        case X86_Prefetch_3DNow:
        case X86_Prefetchw_3DNow:
        case X86_Prefetch_T0:
        case X86_Prefetch_T1:
        case X86_Prefetch_T2:
        case X86_Prefetch_NTA:
        default:
          //All of these are ignored
          break;
      }
#endif //FLEXUS_TARGET_IS(x86)

      if (required) {
        // create a new instruction object
        intrusive_ptr<SimicsX86MemoryOpImpl> new_inst = intrusive_ptr<SimicsX86MemoryOpImpl> (new /*(theInstructionPool)*/ SimicsX86MemoryOpImpl(*theConsumer));

        // first insert the PC
        logical_address_t pc_logical = SIM_get_program_counter(SIM_current_processor());
        physical_address_t pc = SIM_logical_to_physical(SIM_current_processor(), Sim_DI_Instruction, pc_logical);
        new_inst->setPhysInstAddress(PhysicalMemoryAddress(pc));

        // memory addresses - first physical
        new_inst->setAddress(PhysicalMemoryAddress(mem_trans->s.physical_address));

        bool is_write = SIM_mem_op_is_write(&mem_trans->s) == Sim_RW_Write;
        new_inst->setIsStore(is_write);

        theConsumer->consumeInstruction(new_inst);
        return k_call_me_next_cycle;  //Call back next cycle
      } else {
        //Neither instruction nor data access.  We will ignore this.
        /* shouldn't happen? */
        return k_no_stall;
      }
    }
    return k_no_stall;
  }

};

class SimicsTracer : public Simics::AddInObject <SimicsTracerImpl> {
  typedef Simics::AddInObject<SimicsTracerImpl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "SimicsTracer";
  }
  static std::string classDescription() {
    return "A trace class for tracing memory accesses into Flexus.";
  }

  SimicsTracer() : base() { }
  SimicsTracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  SimicsTracer(SimicsTracerImpl * anImpl) : base(anImpl) {}

  struct TestAttribute : public Simics::AttributeT < TestAttribute, SimicsTracerImpl, int, & SimicsTracerImpl::theTestAttribute>  {
    static std::string attributeName() {
      return "test";
    }
    static std::string attributeDescription() {
      return "Test attribute";
    }
  };

  struct PrintStatsCommand : public Simics::CommandT < PrintStatsCommand, SimicsTracer, SimicsTracerImpl, void ( *)() , & SimicsTracerImpl::printStats > {
  static std::string commandName() {
    return "command_printStats";
  }
  static std::string commandDescription() {
    return "PrintStats command hook";
  }
  };

  typedef mpl::list < TestAttribute >::type Attributes;
  typedef mpl::list < PrintStatsCommand >::type Commands;
};

Simics::API::set_error_t SetFnTest(void *, Simics::API::conf_object_t *, Simics::API::attr_value_t *, Simics::API::attr_value_t *) {
  std::cout << "In SetFnTest" << std::endl;
  return Simics::API::Sim_Set_Ok;
}

Simics::API::attr_value_t GetFnTest(void *, Simics::API::conf_object_t *, Simics::API::attr_value_t *) {
  std::cout << "In GetFnTest" << std::endl;
  return Simics::AttributeValue(0);
}

/*****************************************************************
 * Here the "Flexus" part of the implementation begins.
 *****************************************************************/

FLEXUS_COMPONENT class SimicsTraceFeederComponent : public SimicsTraceConsumer {
  FLEXUS_COMPONENT_IMPLEMENTATION(SimicsTraceFeederComponent );

  //Total number of memory accesses
  int64_t count;

  //The Simics object for getting trace data
  SimicsTracer theTracer;

  ~SimicsTraceFeederComponent() {
    FLEXUS_MBR_DEBUG() << "Memory accesses traced: " << count << end;
  }

public:
  SimicsTraceFeederComponent()
    : count(0)
  {}

  //Ports
  struct FeederCommandIn : public PushInputPort< FeederCommand >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aFeeder, FeederCommand aCommand) {
      FLEXUS_MBR_DEBUG() << "received feeder command: " << aCommand;
      // ignore Oracle_nextInstruction command - our available() function here
      // is sufficient to tell the fetch unit that we are ready with an instruction
    }

  };

  struct InstructionOutputPort : public PullOutputPort< InstructionTransport >, AvailabilityComputedOnRequest {
    FLEXUS_WIRING_TEMPLATE
    static InstructionTransport pull(self & aFeeder) {
      InstructionTransport aTransport;
      aTransport.set(ArchitecturalInstructionTag, aFeeder.ready_instruction());
      aFeeder.theStatus = InProgress;
      return aTransport;
    }

    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aFeeder) {
      return (aFeeder.theStatus == Ready);
    }
  };

  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;

  intrusive_ptr<Instruction> ready_instruction() {
    FLEXUS_MBR_ASSERT(theStatus == Ready);
    return theReadyInstruction;
  }

  /*
    Initialize. Install ourselves as the default. This is equivalent to
    the init_local() function in an actual Simics module.
  */
  void
  initialize(void) {
    Simics::Factory<SimicsTracer> tracer_factory;

    Simics::API::conf_class_t * trace_class = tracer_factory.getSimicsClass();

    Simics::API::timing_model_interface_t * timing_interface;

    timing_interface = new timing_model_interface_t(); //LEAKS - Need to fix
    timing_interface->operate = Simics::make_signature<Simics::API::operate_func_t>::from_addin_fn<SimicsTracer, SimicsTracerImpl, &SimicsTracerImpl::trace_mem_hier_operate>::trampoline;
    SIM_register_interface(trace_class, "timing-model", timing_interface);

    theTracer = tracer_factory.create("flexus-tracer");
    theTracer->setTraceConsumer(*this);

  }  // end initialize()

};  // end class SimicsTraceFeederComponent

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(SimicsTraceFeederComponentConfiguration);

}  // end Namespace SimicsTraceFeeder
} // end namespace Flexus

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SimicsTraceFeeder

#if 0
// store the group id (must be static because it is used by MM_ZALLOC in static functions)
static int32_t mm_id;

static set_error_t
set_enabled(void * dont_care, conf_object_t * obj, attr_value_t * val, attr_value_t * idx) {
  conf_object_t * cpu_obj;
  trace_mem_hier_object_t * tmho;
  conf_object_t * phys_mem;
  conf_class_t * trace_class;
  attr_value_t attr;
  char buf[128];
  int32_t i;

  if (val->kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;

  if (val->u.integer) {
    /* enable tracing */

    if (bt->trace_enabled) {
      SIM_frontend_exception(SimExc_General, "Tracing already enabled");
      return Sim_Set_Illegal_Value;
    }

    /* dump any cached memory translations, so that we see all memory operations */
    SIM_dump_caches();

    trace_class = SIM_get_class("trace-mem-hier");

    /* create mem hiers for every cpu we will trace on */
    for (i = 0, cpu_obj = SIM_next_queue(0); cpu_obj; cpu_obj = SIM_next_queue(cpu_obj)) {
      sprintf(buf, "trace-mem-hier-%d", i++);

      /* created last time we started ? */
      if (!(tmho = (trace_mem_hier_object_t *)SIM_get_object(buf))) {
        SIM_clear_exception();
        /* no, we'll have to create it here */
        if (!(tmho = (trace_mem_hier_object_t *)SIM_new_object(trace_class, buf))) {
          SIM_frontend_exception(SimExc_General, "Cannot create object");
          return Sim_Set_Illegal_Value;
        }
      }

      tmho->bt = bt;

      //        attr.kind = Sim_Val_String;
      //        attr.u.string = cpu_obj->name;

      //        SIM_set_attribute(&tmho->obj, "queue", &attr);

      attr.kind = Sim_Val_Object;
      attr.u.object = &tmho->obj;

      /* Tell memory we have a mem hier */
      phys_mem = SIM_get_attribute(cpu_obj, "physical_memory").u.object;
      SIM_set_attribute(phys_mem, "timing_model", &attr);
      SIM_set_attribute(phys_mem, "snoop_device", &attr);
    }

    bt->trace_enabled = 1;
    bt->priv_mode = Sim_CPU_Mode_User;  // assume we start in user mode
    bt->curr_pid = 1 << 25;
    get_task_pid(SIM_current_processor(), bt);

    /* Set up privilege and process haps */
    SIM_hap_register_callback("Core_Mode_Change",
                              (str_hap_func_t)privilege_change_handler, bt);
    bt->cntx_bkpt = SIM_breakpoint(SIM_get_object("primary-context"),
                                   Sim_Break_Virtual, Sim_Access_Execute,
                                   SWITCH_TO_ADDR, 1, 0);
    SIM_hap_register_callback_idx("Core_Breakpoint",
                                  (str_hap_func_t)task_pid_cb, bt->cntx_bkpt, bt);

#if defined(TARGET_X86)
    attr.kind = Sim_Val_String;
    attr.u.string = "instruction-fetch-trace";
    SIM_set_attribute(SIM_get_object("sim"), "instruction_profile_mode", &attr);
#else
    attr.kind = Sim_Val_String;
    attr.u.string = "instruction-cache-access-trace";
    SIM_set_attribute(SIM_get_object("sim"), "instruction_profile_mode", &attr);

#if defined(TARGET_IA64)
    if (SIM_get_attribute(SIM_get_object("sim"),
                          "instruction_profile_line_size").u.integer != 16) {
      pr("* The value of conf.sim.instruction_profile_line_size *must* be set to 16\n"
         "  otherwise the trace module will not work proberly. This has to be set\n"
         "  in the configuration which is loaded at startup.\n");
    }
#else /* !TARGET_IA64 */
    if (SIM_get_attribute(SIM_get_object("sim"), "instruction_profile_line_size").u.integer != 4) {
      pr("* The value of conf.sim.instruction_profile_line_size *must* be set to 4\n"
         "  otherwise the trace module will not work proberly. This has to be set\n"
         "  in the configuration which is loaded at startup.\n");
    }
#endif /* !TARGET_IA64 */
#if 0

    attr.kind = Sim_Val_Integer;
    attr.u.integer = 4;
    SIM_set_attribute(SIM_get_object("sim"), "instruction_profile_line_size", &attr);
#endif
#endif
  } else {
    /* disable tracing */

    bt->trace_enabled = 0;

    /* Remove all events from queue */
    for (obj = SIM_next_queue(0); obj ; obj = SIM_next_queue(obj)) {

      attr.kind = Sim_Val_Nil;

      phys_mem = SIM_get_attribute(obj, "physical_memory").u.object;
      SIM_set_attribute(phys_mem, "timing_model", &attr);
      SIM_set_attribute(phys_mem, "snoop_device", &attr);

      if (SIM_clear_exception() != SimExc_No_Exception) {
        pr("Problems clearing attributes: %s\n", SIM_last_error());
        SIM_clear_exception();
      }
    }

    /* kill all mem_hiers */
    i = 0;
    while (1) {
      sprintf(buf, "trace-mem-hier-%d", i++);

      SIM_clear_exception();
      tmho = (trace_mem_hier_object_t *)SIM_get_object(buf);
      if (SIM_get_pending_exception()) {
        SIM_clear_exception();
        break;
      }

      //SIM_delete_object(tmho);
    }

    SIM_hap_unregister_callback("Core_Mode_Change",
                                (str_hap_func_t)privilege_change_handler,
                                bt);
    SIM_delete_breakpoint(bt->cntx_bkpt);
    SIM_hap_unregister_callback("Core_Breakpoint",
                                (str_hap_func_t)task_pid_cb,
                                bt);

  }
  return Sim_Set_Ok;
}

static attr_value_t
get_enabled(void * dont_care, conf_object_t * obj, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  attr_value_t ret;

  ret.kind = Sim_Val_Integer;
  ret.u.integer = bt->trace_enabled;
  return ret;
}

static set_error_t
set_include_data(void * dont_care, conf_object_t * obj, attr_value_t * val, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  if (val->kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;
  bt->include_data = val->u.integer;
  return Sim_Set_Ok;
}

static attr_value_t
get_include_data(void * dont_care, conf_object_t * obj, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  attr_value_t ret;

  ret.kind = Sim_Val_Integer;
  ret.u.integer = bt->include_data;
  return ret;
}

#ifdef TARGET_X86
static set_error_t
set_print_access_type(void * dont_care, conf_object_t * obj, attr_value_t * val, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  if (val->kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;
  bt->print_access_type = val->u.integer;
  return Sim_Set_Ok;
}

static attr_value_t
get_print_access_type(void * dont_care, conf_object_t * obj, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  attr_value_t ret;

  ret.kind = Sim_Val_Integer;
  ret.u.integer = bt->print_access_type;
  return ret;
}

static set_error_t
set_print_memory_type(void * dont_care, conf_object_t * obj, attr_value_t * val, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  if (val->kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;
  bt->print_memory_type = val->u.integer;
  return Sim_Set_Ok;
}

static attr_value_t
get_print_memory_type(void * dont_care, conf_object_t * obj, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  attr_value_t ret;

  ret.kind = Sim_Val_Integer;
  ret.u.integer = bt->print_memory_type;
  return ret;
}
#endif

static set_error_t
set_trace_instructions(void * dont_care, conf_object_t * obj, attr_value_t * val, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  if (val->kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;
  bt->trace_instructions = val->u.integer;
  return Sim_Set_Ok;
}

static attr_value_t
get_trace_instructions(void * dont_care, conf_object_t * obj, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  attr_value_t ret;

  ret.kind = Sim_Val_Integer;
  ret.u.integer = bt->trace_instructions;
  return ret;
}

static set_error_t
set_trace_pause(void * dont_care, conf_object_t * obj, attr_value_t * val, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  if (val->kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;
  bt->pause = val->u.integer;
  return Sim_Set_Ok;
}

static attr_value_t
get_trace_pause(void * dont_care, conf_object_t * obj, attr_value_t * idx) {
  base_trace_t * bt = (base_trace_t *)obj;
  attr_value_t ret;

  ret.kind = Sim_Val_Integer;
  ret.u.integer = bt->pause;
  return ret;
}
#endif

#if 0
init {
  class_data_t base_funcs;
  conf_class_t * base_class;
  class_data_t trace_funcs;
  conf_class_t * trace_class;
  timing_model_interface_t * timing_interface, *snoop_interface;

  SIM_register_attribute(base_class, "enabled", get_enabled, 0, set_enabled, 0, Sim_Attr_Session,
  "<tt>1</tt>|<tt>0</tt> Set to 1 to enable tracing, 0 to disable.");

  SIM_register_attribute(base_class, "pause",
  get_trace_pause, 0, set_trace_pause, 0, Sim_Attr_Session,
  "<tt>1</tt>|<tt>0</tt> Set to 1 to temporarily disable trace output, "
  "0 to re-enable.");

  SIM_register_attribute(base_class, "trace_instructions",
  get_trace_instructions, 0, set_trace_instructions, 0, Sim_Attr_Session,
  "<tt>1</tt>|<tt>0</tt> Set to 1 to enable tracing instructions, 0 to disable.");

  SIM_register_attribute(base_class, "include_data",
  get_include_data, 0, set_include_data, 0, Sim_Attr_Session,
  "<tt>1</tt>|<tt>0</tt> Set to 1 to include data, 0 to disable.");

#ifdef TARGET_X86
  SIM_register_attribute(base_class, "print_access_type",
  get_print_access_type, 0, set_print_access_type, 0, Sim_Attr_Session,
  "<tt>1</tt>|<tt>0</tt> Set to 1 to enable printing the memory access type, 0 to disable.");
  SIM_register_attribute(base_class, "print_memory_type",
  get_print_memory_type, 0, set_print_memory_type, 0, Sim_Attr_Session,
  "<tt>1</tt>|<tt>0</tt> Set to 1 to enable printing the linear address, 0 to disable.");
#endif

}
#endif

#if 0
static integer_t
read_uint32(processor_t * cpu, logical_address_t va) {
  physical_address_t pa = SIM_logical_to_physical(cpu, Sim_DI_Data, va);
  return SIM_read_phys_memory(cpu, pa, 4);
}

static integer_t
pid_from_task_struct(processor_t * cpu, logical_address_t addr) {
  return read_uint32(cpu, addr + TASK_PID_OFF);
}
static integer_t
get_current_pid(processor_t * cpu) {
  /* grab the stack pointer */
  ireg_t esp = SIM_read_register(cpu, SIM_get_register_number("esp"));
  /* get the address for the current task_struct - see
     linux/include/asm-i386/current.h for this method */
  logical_address_t addr  = esp & (~0x1FFF);
  /* finally grab the pid itself */
  return pid_from_task_struct(cpu, addr);
}
static void
get_task_pid(processor_t * cpu, base_trace_t * bt) {
  /* need try/catch block here */
  try {
    bt->curr_pid = get_current_pid(cpu);
  } catch (int64_t rc) {
    printf("invalid memory access during switch_to\n");
  }
}

/*
  This function is called whenever a change occurs in the
  privilege mode.
*/
static void
privilege_change_handler(void * obj, conf_object_t * cpu,
                         integer_t old_mode, integer_t new_mode) {
  base_trace_t * bt = (base_trace_t *)obj;
  if (new_mode == Sim_CPU_Mode_User) {
    bt->priv_mode = 0;
  } else if (new_mode == Sim_CPU_Mode_Supervisor) {
    bt->priv_mode = 1;
  } else {
    printf("changed to invalid privilege mode!\n");
  }
}

/*
  This is called when a process/context switch occurs.
*/
static void
task_pid_cb(void * data, integer_t acc, integer_t bp_id,
            void * reg_ptr, integer_t aSize) {
  base_trace_t * bt = (base_trace_t *)data;
  processor_t * cpu = SIM_current_processor();
  get_task_pid(cpu, bt);
}
#endif

