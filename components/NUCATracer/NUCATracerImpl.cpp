#include <components/NUCATracer/NUCATracer.hpp>

#include <core/simics/api_wrappers.hpp>
#include <core/simics/mai_api.hpp>

#define FLEXUS_BEGIN_COMPONENT NUCATracer
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "common.hpp"
#include "coordinator.hpp"
#include <sys/stat.h>

#include <components/WhiteBox/WhiteBoxIface.hpp>

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
namespace API = Flexus::Simics::API;

namespace nNUCATracer {

using namespace Flexus;

class FLEXUS_COMPONENT(NUCATracer) {
  FLEXUS_COMPONENT_IMPL( NUCATracer );

  TraceCoordinator * theCoordinator;
public:
  FLEXUS_COMPONENT_CONSTRUCTOR(NUCATracer)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return true;
  }

  // Initialization
  void initialize() {
    if (cfg.OnSwitch) {
      mkdir(cfg.TraceOutPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      theCoordinator = new tCoordinator(cfg.TraceOutPath.c_str(), false /* create the files */ );
      theFlexus->onTerminate( std::bind( &TraceCoordinator::finalize, theCoordinator) );
    }
  }

  // Ports
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Reads);
  void push(interface::L2Reads const &, index_t anIndex, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      theCoordinator->accessL2( theFlexus->cycleCount(), eRead, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Writes);
  void push(interface::L2Writes const &, index_t anIndex, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      theCoordinator->accessL2( theFlexus->cycleCount(), eWrite, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Fetches);
  void push(interface::L2Fetches const &, index_t anIndex, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      theCoordinator->accessL2( theFlexus->cycleCount(), ::eFetch, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L1CleanEvicts);
  void push(interface::L1CleanEvicts const &, index_t anIndex, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      theCoordinator->accessL2( theFlexus->cycleCount(), eL1CleanEvict, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L1DirtyEvicts);
  void push(interface::L1DirtyEvicts const &, index_t anIndex, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      theCoordinator->accessL2( theFlexus->cycleCount(), eL1DirtyEvict, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L1IEvicts);
  void push(interface::L1IEvicts const &, index_t anIndex, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      theCoordinator->accessL2( theFlexus->cycleCount(), eL1IEvict, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Reads);
  void push(interface::Reads const &, MemoryMessage & message ) {
    if (cfg.OnSwitch && message.address() != 0) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message << " ft: " << message.fillType()));
      theCoordinator->accessOC( theFlexus->cycleCount(), eRead, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Writes);
  void push(interface::Writes const &, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0 ) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message << " ft: " << message.fillType()));
      theCoordinator->accessOC( theFlexus->cycleCount(), eWrite, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Fetches);
  void push(interface::Fetches const &, MemoryMessage & message) {
    if (cfg.OnSwitch && message.address() != 0 ) {
      Simics::Processor cpu = Simics::Processor::current();
      tThreadId tid = nWhiteBox::WhiteBox::getWhiteBox()->get_thread_t(Simics::APIFwd::SIM_get_processor_number(cpu));
      DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message << " ft: " << message.fillType()));
      theCoordinator->accessOC( theFlexus->cycleCount(), ::eFetch, message.coreIdx(), tid, message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );
    }
  }

};

}//End namespace nFastBus

FLEXUS_COMPONENT_INSTANTIATOR( NUCATracer, nNUCATracer);
FLEXUS_PORT_ARRAY_WIDTH( NUCATracer, L2Reads ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( NUCATracer, L2Writes ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( NUCATracer, L2Fetches) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( NUCATracer, L1CleanEvicts ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( NUCATracer, L1DirtyEvicts ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( NUCATracer, L1IEvicts ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NUCATracer

#define DBG_Reset
#include DBG_Control()
