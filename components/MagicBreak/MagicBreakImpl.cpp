#include <components/MagicBreak/MagicBreak.hpp>

#define FLEXUS_BEGIN_COMPONENT MagicBreak
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <fstream>

#include <components/MagicBreak/breakpoint_tracker.hpp>
#include <core/simics/configuration_api.hpp>
#include <core/stats.hpp>

namespace nMagicBreak {

namespace Stat = Flexus::Stat;

using namespace Flexus;
using namespace Core;
using namespace Simics;
using namespace API;
using namespace SharedTypes;

class ConsoleBreakString_SimicsObject_Impl  {
  boost::intrusive_ptr< ConsoleStringTracker > theStringTracker;
public:
  ConsoleBreakString_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) : theStringTracker(0) {}

  void setConsoleStringTracker(boost::intrusive_ptr< ConsoleStringTracker > aTracker) {
    theStringTracker = aTracker;
  }

  void addString(std::string const & aString) {
    DBG_Assert(theStringTracker);
    theStringTracker->addString(std::string(aString));
  }

};

class ConsoleBreakString_SimicsObject : public Simics::AddInObject <ConsoleBreakString_SimicsObject_Impl> {
  typedef Simics::AddInObject<ConsoleBreakString_SimicsObject_Impl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "ConsoleBreakString";
  }
  static std::string classDescription() {
    return "ConsoleBreakString object";
  }

  ConsoleBreakString_SimicsObject() : base() { }
  ConsoleBreakString_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  ConsoleBreakString_SimicsObject(ConsoleBreakString_SimicsObject_Impl * anImpl) : base(anImpl) {}

  template <class Class>
  static void defineClass(Class & aClass) {

    aClass.addCommand
    ( & ConsoleBreakString_SimicsObject_Impl::addString
      , "add-break-string"
      , "Add a string to the list of console strings that will halt simulation"
      , "string"
    );
  }

};

Simics::Factory<ConsoleBreakString_SimicsObject> theConBreakFactory;

class RegressionTesting_SimicsObject_Impl  {
  boost::intrusive_ptr< RegressionTracker > theRegressionTracker;
public:
  RegressionTesting_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) : theRegressionTracker(0) {}

  void setRegressionTracker(boost::intrusive_ptr< RegressionTracker > aTracker) {
    theRegressionTracker = aTracker;
  }

  void enable() {
    DBG_Assert(theRegressionTracker);
    theRegressionTracker->enable();
  }

};

class RegressionTesting_SimicsObject : public Simics::AddInObject <RegressionTesting_SimicsObject_Impl> {
  typedef Simics::AddInObject<RegressionTesting_SimicsObject_Impl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "RegressionTesting";
  }
  static std::string classDescription() {
    return "RegressionTesting object";
  }

  RegressionTesting_SimicsObject() : base() { }
  RegressionTesting_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  RegressionTesting_SimicsObject(RegressionTesting_SimicsObject_Impl * anImpl) : base(anImpl) {}

  template <class Class>
  static void defineClass(Class & aClass) {

    aClass.addCommand
    ( & RegressionTesting_SimicsObject_Impl::enable
      , "enable"
      , "Enable regression testing"
    );
  }

};

Simics::Factory<RegressionTesting_SimicsObject> theRegressionTestingFactory;

class IterationTracker_SimicsObject_Impl  {
  boost::intrusive_ptr< IterationTracker > theIterationTracker; //Non-owning pointer
public:
  IterationTracker_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) : theIterationTracker(0) {}

  void setIterationTracker(boost::intrusive_ptr< IterationTracker > anIterTracker) {
    theIterationTracker = anIterTracker;
  }

  void setIterationCount(int32_t aCpu, int32_t aCount) {
    DBG_Assert(theIterationTracker);
    theIterationTracker->setIterationCount(aCpu, aCount);
  }

  void printIterationCounts() {
    DBG_Assert(theIterationTracker);
    theIterationTracker->printIterationCounts(std::cout);
  }

  void enable() {
    DBG_Assert(theIterationTracker);
    theIterationTracker->enable();
  }

  void enableCheckpoints() {
    DBG_Assert(theIterationTracker);
    theIterationTracker->enableCheckpoints();
  }

  void endOnIteration(int32_t anInteration) {
    DBG_Assert(theIterationTracker);
    theIterationTracker->endOnIteration(anInteration);
  }

  void saveState(std::ostream & aStream) {
    DBG_Assert(theIterationTracker);
    theIterationTracker->saveState(aStream);
  }

  void loadState(std::istream & aStream) {
    DBG_Assert(theIterationTracker);
    theIterationTracker->loadState(aStream);
  }

};

class IterationTracker_SimicsObject : public Simics::AddInObject <IterationTracker_SimicsObject_Impl> {
  typedef Simics::AddInObject<IterationTracker_SimicsObject_Impl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "IterationTracker";
  }
  static std::string classDescription() {
    return "IterationTracker object";
  }

  IterationTracker_SimicsObject() : base() { }
  IterationTracker_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  IterationTracker_SimicsObject(IterationTracker_SimicsObject_Impl * anImpl) : base(anImpl) {}

  template <class Class>
  static void defineClass(Class & aClass) {

    aClass.addCommand
    ( & IterationTracker_SimicsObject_Impl::setIterationCount
      , "set-iteration-count"
      , "Set the iteration count for a cpu"
      , "cpu"
      , "count"
    );

    aClass.addCommand
    ( & IterationTracker_SimicsObject_Impl::printIterationCounts
      , "print-counts"
      , "Print out iteration counts"
    );

  }

};

Simics::Factory<IterationTracker_SimicsObject> theIterationTrackerFactory;

class FLEXUS_COMPONENT(MagicBreak) {
  FLEXUS_COMPONENT_IMPL(MagicBreak);

  std::vector< boost::intrusive_ptr< BreakpointTracker > > theTrackers;
  IterationTracker_SimicsObject theIterationTrackerObject;
  RegressionTesting_SimicsObject theRegressionTestingObject;
  ConsoleBreakString_SimicsObject theConsoleStringObject;
  std::ofstream theTransactionsOut;
  boost::intrusive_ptr<CycleTracker> theCycleTracker;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MagicBreak)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
    boost::intrusive_ptr<IterationTracker> tracker(BreakpointTracker::newIterationTracker());
    theTrackers.push_back(tracker);
    theIterationTrackerObject = theIterationTrackerFactory.create("iteration-tracker");
    theIterationTrackerObject->setIterationTracker(tracker);

    boost::intrusive_ptr<RegressionTracker> reg_tracker(BreakpointTracker::newRegressionTracker());
    theTrackers.push_back(reg_tracker);
    theRegressionTestingObject = theRegressionTestingFactory.create("regression-testing");
    theRegressionTestingObject->setRegressionTracker(reg_tracker);

    try {
      boost::intrusive_ptr<ConsoleStringTracker> con_tracker(BreakpointTracker::newConsoleStringTracker());
      theTrackers.push_back(con_tracker);
      theConsoleStringObject = theConBreakFactory.create("console-tracker");
      theConsoleStringObject->setConsoleStringTracker(con_tracker);
    } catch (SimicsException e) {
      DBG_(Crit, ( << "Cannot support graphical console. Need to switch to string-based terminal"));
      exit(1);
    }
  }

  bool isQuiesced() const {
    return true; //MagicBreakComponent is always quiesced
  }

  void saveState(std::string const & aDirName) {
    std::string fname( aDirName );
    fname += "/" + statName();
    std::ofstream ofs(fname.c_str());

    theIterationTrackerObject->saveState ( ofs );

    ofs.close();
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ifstream ifs(fname.c_str());
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found." )  );
    } else {
      theIterationTrackerObject->loadState ( ifs );
      ifs.close();
    }
  }

  void initialize() {
    if (cfg.EnableIterationCounts) {
      theIterationTrackerObject->enable();
      if (cfg.CheckpointOnIteration) {
        theIterationTrackerObject->enableCheckpoints();
      }
      theIterationTrackerObject->endOnIteration(cfg.TerminateOnIteration);
    }
    if (cfg.TerminateOnMagicBreak >= 0) {
      theTrackers.push_back(BreakpointTracker::newTerminateOnMagicBreak(cfg.TerminateOnMagicBreak));
    }
    if (cfg.EnableTransactionCounts) {
      theTrackers.push_back(BreakpointTracker::newTransactionTracker(cfg.TransactionType, cfg.TerminateOnTransaction, cfg.TransactionStatsInterval, cfg.CheckpointEveryXTransactions, cfg.FirstTransactionIs, cfg.CycleMinimum));
      theTransactionsOut.open("transactions.out");
      Stat::getStatManager()->openLoggedPeriodicMeasurement("Transactions", 1000000, Stat::Accumulate, theTransactionsOut, "(DB2.*)|(JBB.*)|(WEB.*)");
    }
    if (cfg.StopCycle > 0 || cfg.CkptCycleInterval > 0) {
      theCycleTracker = BreakpointTracker::newCycleTracker( cfg.StopCycle, cfg.CkptCycleInterval, cfg.CkptCycleName );
    }
    theTrackers.push_back(BreakpointTracker::newSimPrintHandler());
    theTrackers.push_back(BreakpointTracker::newPacketTracker(8083 /*SpecWEB port*/, 0x24 /*Server MAC byte*/, 0x25 /*Client MAC byte*/));
  }

  void finalize() {}

  void drive(interface::TickDrive const &) {
    if (theCycleTracker) {
      theCycleTracker->tick();
    }
  }

};

}//End namespace nMagicBreak

FLEXUS_COMPONENT_INSTANTIATOR( MagicBreak, nMagicBreak);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MagicBreak

#define DBG_Reset
#include DBG_Control()
