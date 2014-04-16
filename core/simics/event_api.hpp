#ifndef FLEXUS_SIMICS_EVENT_API_HPP_INCLUDED
#define FLEXUS_SIMICS_EVENT_API_HPP_INCLUDED

#include <boost/function.hpp>

#include <core/simics/trampoline.hpp>
#include <core/exception.hpp>

namespace Flexus {
namespace Simics {

namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#include FLEXUS_SIMICS_API_HEADER(event)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#include FLEXUS_SIMICS_API_ARCH_HEADER
#include FLEXUS_SIMICS_API_HEADER(processor)
#undef restrict

}
}

class EventQueue;

namespace Detail {
struct Event {
  typedef  API::event_handler_t simics_fn_type;
  typedef  void (*free_fn_ptr)(API::conf_object_t *, void *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *);
  };
  static const char * hap_name;
};

class EventImpl {
public:
  enum Sync { eSyncMachine = API::Sim_Sync_Machine, eSyncProcessor = API::Sim_Sync_Processor};

  struct timing_seconds_tag {};
  struct timing_cycles_tag {};
  struct timing_steps_tag {};
  struct timing_stacked_tag {};

  static timing_seconds_tag timing_seconds;
  static timing_cycles_tag timing_cycles;
  static timing_steps_tag timing_steps;
  static timing_stacked_tag timing_stacked;

private:
  friend class Flexus::Simics::EventQueue;
  enum { eSeconds, eCycles, eSteps, eStacked } theTiming;
  union {
    double Seconds;
    long long Cycles;
    long long Steps;
  } theDelta;
  Sync theSync;
  API::event_handler_t theHandler;
  void * theParameter;
protected:
  EventImpl(timing_seconds_tag, double aDeltaTimeSeconds, API::event_handler_t aHandler, void * aParameter , Sync aSync ) :
    theTiming(eSeconds),
    theSync(aSync),
    theHandler(aHandler),
    theParameter(aParameter) {
    theDelta.Seconds = aDeltaTimeSeconds;
  }

  EventImpl(timing_cycles_tag, long long aDelta, API::event_handler_t aHandler, void * aParameter , Sync aSync ) :    theTiming(eCycles),
    theSync(aSync),
    theHandler(aHandler),
    theParameter(aParameter) {
    theDelta.Cycles = aDelta;
  }

  EventImpl(timing_steps_tag, long long aDelta, API::event_handler_t aHandler, void * aParameter , Sync aSync ) :
    theTiming(eSteps),
    theSync(aSync),
    theHandler(aHandler),
    theParameter(aParameter) {
    theDelta.Steps = aDelta;
  }

  EventImpl(timing_stacked_tag, API::event_handler_t aHandler, void * aParameter) :
    theTiming(eStacked),
    theHandler(aHandler),
    theParameter(aParameter) { }

};
}

class EventQueue {
  API::conf_object_t * theQueueOwner;

public:
  EventQueue() {
    theQueueOwner = API::SIM_next_queue(0);
  }

  explicit EventQueue(API::conf_object_t * anOwner) :
    theQueueOwner(anOwner) {}

  void post(Detail::EventImpl & anEvent) {
    if (anEvent.theTiming == Detail::EventImpl::eSeconds) {
      API::SIM_time_post(
        theQueueOwner,
        anEvent.theDelta.Seconds,
        static_cast<API::sync_t>(anEvent.theSync),
        anEvent.theHandler,
        anEvent.theParameter);
    } else if (anEvent.theTiming == Detail::EventImpl::eCycles) {
      API::SIM_time_post_cycle(
        theQueueOwner,
        anEvent.theDelta.Cycles,
        static_cast<API::sync_t>(anEvent.theSync),
        anEvent.theHandler,
        anEvent.theParameter);
    } else if (anEvent.theTiming == Detail::EventImpl::eSteps) {
      API::SIM_step_post(
        theQueueOwner,
        anEvent.theDelta.Steps,
        anEvent.theHandler,
        anEvent.theParameter);
    } else {
      API::SIM_stacked_post(
        theQueueOwner,
        anEvent.theHandler,
        anEvent.theParameter);
    }
  }

  void clean(Detail::EventImpl & anEvent) {
    if (anEvent.theTiming == Detail::EventImpl::eSeconds || anEvent.theTiming == Detail::EventImpl::eCycles) {
      API::SIM_time_clean(
        theQueueOwner,
        static_cast<API::sync_t>(anEvent.theSync),
        anEvent.theHandler,
        anEvent.theParameter);
    } else {
      API::SIM_step_clean(
        theQueueOwner,
        anEvent.theHandler,
        anEvent.theParameter);
    }
  }

  EventQueue nextQueue() {
    return EventQueue(SIM_next_queue(theQueueOwner));
  }

  operator bool() {
    return theQueueOwner;
  }
};

template <Detail::Event::free_fn_ptr FreeFn>
struct FreeFnEvent : public Detail::EventImpl {
  FreeFnEvent(timing_seconds_tag aTiming, double aDeltaTimeSeconds, void * aParameter = 0, Sync aSync = eSyncProcessor) :
    EventImpl(
      aTiming,
      aDeltaTimeSeconds,
      & make_signature_from_free_fn<API::event_handler_t>::with<FreeFn>::trampoline,
      aParameter,
      aSync) {}

  template<class Timing>
  FreeFnEvent(Timing aTiming, long long aDelta, void * aParameter = 0, Sync aSync = eSyncProcessor) :
    EventImpl(
      aTiming,
      aDelta,
      & make_signature_from_free_fn<API::event_handler_t>::with<FreeFn>::trampoline,
      aParameter) {}

  FreeFnEvent(timing_stacked_tag aTiming, void * aParameter = 0) :
    EventImpl(
      aTiming,
      & make_signature_from_free_fn<API::event_handler_t>::with<FreeFn>::trampoline,
      aParameter)  {}
};

template <class T, typename Detail::Event::member<T>::member_fn_ptr MemberFn>
struct MemberFnEvent : public Detail::EventImpl {
  MemberFnEvent(timing_seconds_tag aTiming, double aDeltaSeconds, T * aT, Sync aSync = eSyncProcessor) :
    EventImpl(
      aTiming,
      aDeltaSeconds,
      make_signature_from_free_fn<API::event_handler_t>::with<MemberFnEvent::reorder_parameters>::trampoline,
      static_cast<void *>(aT),
      aSync) {}

  template<class Timing>
  MemberFnEvent(Timing aTiming, long long aDelta, T * aT, Sync aSync = eSyncProcessor) :
    EventImpl(
      aTiming,
      aDelta,
      make_signature_from_free_fn<API::event_handler_t>::with<MemberFnEvent::reorder_parameters>::trampoline,
      static_cast<void *>(aT),
      aSync) {}

  MemberFnEvent(timing_stacked_tag aTiming, T * aT) :
    EventImpl(
      aTiming,
      make_signature_from_free_fn<API::event_handler_t>::with<MemberFnEvent::reorder_parameters>::trampoline,
      static_cast<void *>(aT)) {}

  static void reorder_parameters(API::conf_object_t * anObj, void * aT) {
    (static_cast<T *>(aT)->*MemberFn)(anObj);
  }
};

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_SIMICS_EVENT_API_HPP_INCLUDED

