#ifndef _PROTOCOL_ENGINE_H_
#define _PROTOCOL_ENGINE_H_

#include "ProtSharedTypes.hpp"
#include "tsrf.hpp"
#include "input_q_cntl.hpp"
#include "thread_scheduler.hpp"

class tSrvcProvider;

namespace nProtocolEngine {

struct tProtocolEngineBase {
  virtual std::string const & engineName() = 0;
  virtual void enqueue(boost::intrusive_ptr<tPacket> packet) = 0;
  virtual uint32_t queueSize(const tVC aVC) const = 0;
  virtual ~tProtocolEngineBase () {}
};

template <class ExecEngine>
class tProtocolEngine : public tProtocolEngineBase {

private:
  std::string theEngineName;                          // name of the protocol engine (for debug)

  //The parts of the protocol engine
  tTsrf                         theTsrf;            // Transaction State Register File
  tInputQueueController         theInputQCntl;      // input queue
  tThreadScheduler              theThreadScheduler; // thread scheduler
  ExecEngine                    theExecEngine;      // execution engine (overloaded to home or remote)

  Stat::StatCounter statBusyCycles;
  Stat::StatCounter statIdleCycles;

public:
  // constructor
  tProtocolEngine(std::string const & name, tSrvcProvider & srvcProv, const unsigned aTSRFSize)
    : theEngineName(name)
    , theTsrf(name, aTSRFSize)
    , theInputQCntl(name, srvcProv, theThreadScheduler)
    , theThreadScheduler(name, theTsrf, theInputQCntl, srvcProv, theExecEngine)
    , theExecEngine(name, srvcProv, theInputQCntl, theThreadScheduler)
    , statBusyCycles(name + "-BusyCycles")
    , statIdleCycles(name + "-IdleCycles")
  { }

  // destructor
  ~tProtocolEngine() {}

  bool isQuiesced() const {
    return  theTsrf.isQuiesced()
            &&    theInputQCntl.isQuiesced()
            &&    theThreadScheduler.isQuiesced()
            ;
  }

  // initialization
  void init() {
    theExecEngine.init();
  }

  // printout stats
  void dump_mcd_stats() const {
    theExecEngine.dump_mcd_stats();
  }

  // dump counters to xe.cnt file
  void dump_mcd_counters() const {
    theExecEngine.dump_mcd_counters();
  }

  // enqueue remote request
  void enqueue(boost::intrusive_ptr<tPacket> packet) {
    theInputQCntl.enqueue(packet);
  }

  // find if there is space available

  // get size of the queue
  uint32_t queueSize(const tVC VC) const {
    return theInputQCntl.size(VC);
  }

  ///////////////////////////////////////////////////////////
  //
  // main execution loop routine
  //
public:
  std::string const & engineName() {
    return theEngineName;
  }

  void do_cycle() {
    theThreadScheduler.processQueues();

    if (theThreadScheduler.ready()) {
      statBusyCycles++;
      tThread thread = theThreadScheduler.activate();
      theExecEngine.runThread(thread);
      theThreadScheduler.reschedule(thread);
    } else {
      statIdleCycles++;
    }
  }

};

}  // namespace nProtocolEngine

#endif // _PROTOCOL_ENGINE_H_

