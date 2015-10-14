/*
   InorderSimicsFeeder.hpp

   Stephen Somogyi
   Carnegie Mellon University

   22Jul2003 - initial version (derived from in-order SimicsTraceFeederImpl.hpp)

   This is both a module for Simics and Flexus.  As a Simics module,
   it snoops all memory accesses.  These accesses are converted into
   Flexus trace instructions.  The module is also a Flexus
   fetch component, and injects these instructions into the Flexus
   pipeline.

*/

#include <components/InorderSimicsFeeder/InorderSimicsFeeder.hpp>

#include <iomanip>

#include <memory>

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

#include <core/simics/configuration_api.hpp>
#include <core/simics/mai_api.hpp>
#include <core/simics/simics_interface.hpp>
#include <core/stats.hpp>

#include <components/Common/DoubleWord.hpp>
#include <components/Common/Transports/InstructionTransport.hpp>
#include <components/Common/Slices/ArchitecturalInstruction.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#define DBG_DefineCategories Feeder
#define DBG_SetDefaultOps AddCat(Feeder)
#include DBG_Control()

namespace nInorderSimicsFeeder {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;

using namespace Simics::API;

using boost::intrusive_ptr;

typedef ArchitecturalInstruction Instruction;

}

#include <components/InorderSimicsFeeder/DebugTrace.hpp>
#include <components/InorderSimicsFeeder/StoreBuffer.hpp>
#include <components/InorderSimicsFeeder/TraceConsumer.hpp>
#include <components/InorderSimicsFeeder/CycleManager.hpp>
#include <components/InorderSimicsFeeder/SimicsTracer.hpp>
#include <components/InorderSimicsFeeder/MemoryTrace.hpp>

#define FLEXUS_BEGIN_COMPONENT InorderSimicsFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nInorderSimicsFeeder {

using namespace Flexus;

/*****************************************************************
 * Here the "Flexus" part of the implementation begins.
 *****************************************************************/

class FLEXUS_COMPONENT(InorderSimicsFeeder) {
  FLEXUS_COMPONENT_IMPL( InorderSimicsFeeder );

  //The Simics objects (one for each processor) for getting trace data
  int32_t theNumCPUs;
  int32_t theNumVMs;
  SimicsTracer * theTracers;

  std::vector< std::shared_ptr<SimicsTraceConsumer> > theConsumers;

  boost::scoped_ptr<SimicsCycleManager> theSimicsCycleManager;

  // Memory trace (optional)
  bool traceInit;
  uint64_t theLastTransactionStart;
  nMemoryTrace::MemoryTrace * theMemoryTrace;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(InorderSimicsFeeder)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
    theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    DBG_(Crit, ( << "Initializing InorderSimicsFeeder with " << theNumCPUs << " cpus." ));
    theTracers = new SimicsTracer [ theNumCPUs ];
    doInitialize();
  }

  virtual ~InorderSimicsFeederComponent() {}

  //InstructionOutputPort
  //=====================
  bool available(interface::InstructionOutputPort const &, index_t anIndex) {
    if (!cfg.UseTrace) {
      return (theConsumers[anIndex]->isReady());
    } else {
      return true;
    }
  }

  InstructionTransport pull(interface::InstructionOutputPort const &, index_t anIndex) {
    InstructionTransport aTransport;
    DBG_Assert(anIndex < theConsumers.size() );

    if (!cfg.UseTrace) {
      aTransport.set(ArchitecturalInstructionTag, theConsumers[anIndex]->ready_instruction());
      theConsumers[anIndex]->begin_instruction();
    } else {
      if (!traceInit) {
        traceInit = true;
        DBG_(Crit, ( << "Initializing from Trace file " << cfg.TraceFile ));
        theMemoryTrace = new nMemoryTrace::MemoryTrace(theNumCPUs);
        theMemoryTrace->init(cfg.TraceFile.c_str());
      }
      nMemoryTrace::MemoryOp op(theMemoryTrace->getNextMemoryOp(anIndex));

      // map the MemoryOp into an Instruction
      boost::intrusive_ptr<Instruction> instr( new ArchitecturalInstruction());
      switch (op.type) {
        case nMemoryTrace::NOP:
          instr->setIsNop();
          break;
        case nMemoryTrace::LD:
          instr->setIsLoad();
          break;
        case nMemoryTrace::ST:
          instr->setIsStore();
          break;
        case nMemoryTrace::TSTART:
          // start new transaction
          theLastTransactionStart = Flexus::Core::theFlexus->cycleCount();
          break;
        case nMemoryTrace::TSTOP:
          DBG_(Crit, ( << "Transaction completed in " <<
                       Flexus::Core::theFlexus->cycleCount() - theLastTransactionStart + 1
                       << " cycles"));
          break;
        case nMemoryTrace::MSG:
          // do nothing, shouldn't reach here
          break;
      }
      instr->setAddress((Flexus::SharedTypes::PhysicalMemoryAddress)op.addr);
      instr->setPhysInstAddress((Flexus::SharedTypes::PhysicalMemoryAddress)0xbaadf00d);  // UNIMPLEMENTED (for now)

      instr->setTrace();
      instr->setStartTime(Flexus::Core::theFlexus->cycleCount());

      if (op.type != nMemoryTrace::NOP)
        DBG_(Iface, ( << "From trace: [" << anIndex << "] -- " << *instr));

      // stick it in transport
      aTransport.set(ArchitecturalInstructionTag, instr);

      // end simulation?
      if (theMemoryTrace->isComplete()) {
        Flexus::Core::theFlexus->terminateSimulation();
      }
    }

    return aTransport;
  }

  void registerSnoopInterface(Simics::API::conf_class_t * trace_class) {
    Simics::API::timing_model_interface_t * snoop_interface;

    snoop_interface = new Simics::API::timing_model_interface_t(); //LEAKS - Need to fix
    snoop_interface->operate = &Simics::make_signature_from_addin_fn<Simics::API::operate_func_t>::with<SimicsTracer, SimicsTracerImpl, &SimicsTracerImpl::trace_snoop_operate>::trampoline;
    Simics::API::SIM_register_interface(trace_class, "snoop-memory", snoop_interface);
  }

  void registerTimingInterface(Simics::API::conf_class_t * trace_class) {
    Simics::API::timing_model_interface_t * timing_interface;

    timing_interface = new Simics::API::timing_model_interface_t(); //LEAKS - Need to fix
    timing_interface->operate = &Simics::make_signature_from_addin_fn2<Simics::API::operate_func_t>::with<SimicsTracer, SimicsTracerImpl, &SimicsTracerImpl::trace_mem_hier_operate>::trampoline;
    Simics::API::SIM_register_interface(trace_class, "timing-model", timing_interface);
  }

  bool isQuiesced() const {
    if (theConsumers.empty()) {
      //Not yet initialized, so we must be quiesced
      return true;
    } else {
      for (int32_t i = 0; i < theNumCPUs; ++i) {
        if (theConsumers[i]->isQuiesced()) {
          return true;
        }
      }
    }
    return true;
  }

  void
  initialize(void) {

#ifndef FLEXUS_FEEDER_OLD_SCHEDULING
    DBG_( Dev, ( << "Using new scheduling mechanism."  ) Comp(*this) );
    Flexus::Simics::theSimicsInterface->disableCycleHook();
#endif

  }

  void finalize() {};

  /*
    Initialize. Install ourselves as the default. This is equivalent to
    the init_local() function in an actual Simics module.
  */
  void
  doInitialize(void) {
    DBG_( Dev, ( << "Initializing InorderSimicsFeeder."  ) Comp(*this) );

    Simics::APIFwd::SIM_flush_all_caches();

    int32_t n = Simics::ProcessorMapper::numProcessors();
    DBG_(Crit, ( << "ProcessorMapper found " << n << " CPUs for Flexus." ));

    for (int32_t i = 0; i < theNumCPUs; ++i) {
      std::string name;
      if (theNumCPUs > 1) {
        name = boost::padded_string_cast < 3, '0' > (i) + "-feeder" ;
      } else {
        name = "sys-feeder" ;
      }
      theConsumers.push_back( std::shared_ptr<SimicsTraceConsumer>(new SimicsTraceConsumer(name)) );
    }

#if 0

    bool client_server = false;
    bool virtualized = false;
    if (Simics::API::SIM_get_object("cpu0") == 0) {
      if (Simics::API::SIM_get_object("server_cpu0") == 0) {
        if (Simics::API::SIM_get_object("machine0_cpu0") == 0) {
          if (Simics::API::SIM_get_object("machine0_server_cpu0") == 0) {
            DBG_Assert( false, ( << "InorderSimicsFeeder cannot locate cpu0 or server_cpu0 objects." ) );
          } else {
            virtualized = true;
          }
        } else {
          virtualized = true;
        }
      } else {
        DBG_( Crit, ( << "InorderSimicsFeeder detected client-server simulation.  Connecting to server CPUs only." ) );
        client_server = true;
      }
    }

    DBG_(Crit, ( << "Creating vector with " << theNumCPUs << " cpus" ));
    std::vector<Simics::API::conf_object_t *> cpu_vector(theNumCPUs);
    if (virtualized) {
      DBG_( Crit, ( << "InorderSimicsFeeder detected virtual machine simulation.  Searching for all VMs." ) );
      std::list<Simics::API::conf_object_t *> cpu_list;
      int32_t maxNumVMs = 8;
      int32_t ii = 0;
      int32_t vm = 0;
      for (; vm < maxNumVMs && ii < theNumCPUs; vm++) {
        bool client_server = false;
        std::string name("machine");
        name += boost::lexical_cast<std::string>(vm);
        name += "_cpu0";
        //DBG_(Crit, ( << "Looking for " << name ));
        if (Simics::API::SIM_get_object(name.c_str()) == 0) {
          name = "machine";
          name += boost::lexical_cast<std::string>(vm);
          name += "_server_cpu0";
          //DBG_(Crit, ( << "Looking for " << name ));
          if (Simics::API::SIM_get_object(name.c_str()) != 0) {
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
          Simics::API::conf_object_t * cpu = Simics::API::SIM_get_object( name.c_str() );
          if (cpu == 0) {
            break;
          }
          cpu_list.push_back(cpu);
        }
      }

      DBG_Assert(ii == theNumCPUs, ( << "Only found " << ii << " CPUs not " << theNumCPUs ));

      theNumVMs = vm;

      int32_t vms_per_row = (int)std::ceil(std::sqrt(theNumVMs));
      int32_t cores_per_vm = theNumCPUs / theNumVMs;
      int32_t num_vm_rows = (int)std::ceil(std::sqrt(cores_per_vm));
      int32_t num_rows = num_vm_rows * vms_per_row;
      for (int32_t i = 0; i < theNumCPUs; i++) {
        // re-map flexus index, so VMs are organized in a nice grid
        int32_t vm = i / cores_per_vm;
        int32_t core = i % cores_per_vm;
        int32_t row_in_vm = core / num_vm_rows;
        int32_t col_in_vm = core % num_vm_rows;
        int32_t row_of_vm = vm / vms_per_row;
        int32_t col_of_vm = vm % vms_per_row;

        int32_t abs_row = row_in_vm + row_of_vm * num_vm_rows;
        int32_t abs_col = col_in_vm + col_of_vm * num_vm_rows;

        int32_t new_index = abs_row * num_rows + abs_col;
        cpu_vector[new_index] = cpu_list.front();
        cpu_list.pop_front();
      }
    } else {
      for (int32_t ii = 0; ii < theNumCPUs; ++ii) {
        std::string name("cpu");
        if (client_server) {
          name = "server_cpu";
        }
        name += boost::lexical_cast<std::string>(ii);
        DBG_( Crit, ( << "Connecting: " << name ) );
        Simics::API::conf_object_t * cpu = Simics::API::SIM_get_object( name.c_str() );
        cpu_vector[ii] = cpu;
        DBG_(Crit, ( << "CPU " << ii << " = " << cpu_vector[ii]->name ));
      }
    }
#endif

    theSimicsCycleManager.reset(new SimicsCycleManager(theConsumers));

    Simics::Factory<SimicsTracer> tracer_factory;

    Simics::API::conf_class_t * trace_class = tracer_factory.getSimicsClass();

    registerTimingInterface(trace_class);
    registerSnoopInterface(trace_class);

    for (int32_t ii = 0; ii < theNumCPUs ; ++ii) {
      std::string feeder_name("flexus-feeder");
      if (theNumCPUs > 1) {
        feeder_name += '-' + lexical_cast<std::string> (ii);
      }
      Simics::API::conf_object_t * cpu = Simics::API::SIM_get_processor( Simics::ProcessorMapper::mapFlexusIndex2ProcNum(ii) );
      theTracers[ii] = tracer_factory.create(feeder_name);
      theTracers[ii]->init(cpu, ii, cfg.StallCap);
      theTracers[ii]->setTraceConsumer(theConsumers[ii]);
      theTracers[ii]->setCycleManager(*theSimicsCycleManager);
    }

    /* Enable instruction tracing */
    Simics::API::attr_value_t attr;
    attr.kind = Simics::API::Sim_Val_String;
    attr.u.string = "instruction-fetch-trace";
    Simics::API::SIM_set_attribute(Simics::API::SIM_get_object("sim"), "instruction_profile_mode", &attr);

    // set traceInit to false for now
    // at first instruction pull, we'll really initialize the trace
    traceInit = false;

    DBG_( VVerb, ( << "Done initializing InorderSimicsFeeder."  ) Comp(*this));
  }  // end initialize()

};  // end class SimicsMPTraceFeederComponent

std::string theTraceFile("theTraceFile");

}  // end Namespace nInorderSimicsFeeder

FLEXUS_COMPONENT_INSTANTIATOR( InorderSimicsFeeder, nInorderSimicsFeeder );
FLEXUS_PORT_ARRAY_WIDTH( InorderSimicsFeeder, InstructionOutputPort ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT InorderSimicsFeeder

#define DBG_Reset
#include DBG_Control()
