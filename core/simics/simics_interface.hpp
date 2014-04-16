#ifndef FLEXUS_CORE_SIMICS_INTERFACE_HPP_INCLUDED
#define FLEXUS_CORE_SIMICS_INTERFACE_HPP_INCLUDED

#include <string>
#include <cstring>

#include <core/debug/debug.hpp>

#include <core/flexus.hpp>

#include <core/metaprogram.hpp>
#include <core/simics/configuration_api.hpp>
#include <core/simics/event_api.hpp>
#include <core/simics/hap_api.hpp>
#include <core/simics/control_api.hpp>
#include <core/flexus.hpp>

namespace Flexus {
namespace Simics {

class SimicsInterfaceImpl {
public:
  void doCycleEvent(API::conf_object_t * anObj) {
    DBG_(VVerb, Core() ( << "Entering Flexus." ) );
    try {
      Core::theFlexus->doCycle();
      EventQueue(anObj).post(theEveryCycleEvent);
    } catch (std::exception & e) {
      DBG_(Crit, Core() ( << "Exception in Flexus: " << e.what() ) );
      BreakSimulation(e.what());
    }
    DBG_(VVerb, Core() ( << "Returing to Simics " ) );
  }

  void enableCycleHook() {
    if (! theIsEnabled) {
      DBG_(VVerb, Core() ( << "Enabling cycle hook" ) );
      theIsEnabled = true;
      EventQueue queue;
      queue.post(theEveryCycleEvent);
    }
  }

  void disableCycleHook() {
    DBG_(VVerb, Core() ( << "Disabling cycle hook" ) );
    if (theIsEnabled) {
      theIsEnabled = false;
      EventQueue queue;
      while (queue) {
        queue.clean(theInitialCycleEvent);
        queue.clean(theEveryCycleEvent);
        queue = queue.nextQueue();
      }
    }
  }

  void continueSimulation(API::conf_object_t * ignored) {
    if (! Flexus::Core::theFlexus->initialized()) {
      Flexus::Core::theFlexus->initializeComponents();
    }
  }

  void stopSimulation(API::conf_object_t *, long long, char *) {
  }

  Flexus::Core::index_t getSystemWidth() {
    int cpu_count = 0;
    int client_cpu_count = 0;
    // added by PLotfi to support SPECweb2009 workloads
    // in SPECweb2009 in addition to client and server machines, a third group of machines exists to represent backend. The name of these mahines starts with "besim"
    int besim_cpu_count = 0;
    // end PLotfi
    API::conf_object_t * queue = API::SIM_next_queue(NULL);
    while (queue != NULL) {
      if (std::strstr(queue->name, "client") != 0) {
        ++client_cpu_count;
      } else if (std::strstr(queue->name, "besim") != 0) { // added by PLotfi
        ++besim_cpu_count;
      } else {
        ++cpu_count;
      }
      queue = API::SIM_next_queue(queue);
    }
    return cpu_count;
  }

private:
  //These must follow the definition of doCycle();
  typedef MemberFnEvent<SimicsInterfaceImpl, &SimicsInterfaceImpl::doCycleEvent> do_cycle_event_t;
  do_cycle_event_t theInitialCycleEvent;
  do_cycle_event_t theEveryCycleEvent;

  HapToMemFnBinding<HAPs::Core_Continuation, SimicsInterfaceImpl, &SimicsInterfaceImpl::continueSimulation> theContinueSimulationHAP;
  HapToMemFnBinding<HAPs::Core_Simulation_Stopped, SimicsInterfaceImpl, &SimicsInterfaceImpl::stopSimulation> theStopSimulationHAP;

  bool theIsEnabled;

  Simics::API::conf_object_t * thisObject;

public:
  SimicsInterfaceImpl(Simics::API::conf_object_t * anObject )
    : theInitialCycleEvent(do_cycle_event_t::timing_cycles, 0, this)
    , theEveryCycleEvent(do_cycle_event_t::timing_cycles, 1, this)
    , theContinueSimulationHAP(this)
    , theStopSimulationHAP(this)
    , theIsEnabled(true) {
    EventQueue queue;
    queue.post(theInitialCycleEvent);
  }
};

class SimicsInterface_Obj : public AddInObject<SimicsInterfaceImpl> {
  typedef AddInObject<SimicsInterfaceImpl> base;
public:
  static const Persistence  class_persistence = Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "SimicsInterface";
  }
  static std::string classDescription() {
    return "Simics Interface class";
  }

  SimicsInterface_Obj() : base() {}
  SimicsInterface_Obj(Simics::API::conf_object_t * anObject) : base(anObject) {}
  SimicsInterface_Obj(SimicsInterfaceImpl * anImpl) : base(anImpl) {}

};

typedef Factory< SimicsInterface_Obj > SimicsInterfaceFactory;

extern SimicsInterface_Obj theSimicsInterface;

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_CORE_SIMICS_INTERFACE_HPP_INCLUDED
