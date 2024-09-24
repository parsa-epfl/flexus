#include <components/MagicBreakQEMU/MagicBreak.hpp>

#define FLEXUS_BEGIN_COMPONENT MagicBreak
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/MagicBreakQEMU/breakpoint_tracker.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/stats.hpp>
#include <fstream>

namespace nMagicBreak {

namespace Stat = Flexus::Stat;

using namespace Flexus;
using namespace Core;
using namespace Qemu;
using namespace API;
using namespace SharedTypes;

class ConsoleBreakString_QemuObject_Impl
{
    boost::intrusive_ptr<ConsoleStringTracker> theStringTracker;

  public:
    ConsoleBreakString_QemuObject_Impl(Flexus::Qemu::API::conf_object_t* /*ignored*/)
      : theStringTracker(0)
    {
    }

    void setConsoleStringTracker(boost::intrusive_ptr<ConsoleStringTracker> aTracker) { theStringTracker = aTracker; }

    void addString(std::string const& aString)
    {
        DBG_Assert(theStringTracker);
        theStringTracker->addString(std::string(aString));
    }
};

class ConsoleBreakString_QemuObject : public Qemu::AddInObject<ConsoleBreakString_QemuObject_Impl>
{
    typedef Qemu::AddInObject<ConsoleBreakString_QemuObject_Impl> base;

  public:
    static const Qemu::Persistence class_persistence = Qemu::Session;
    static std::string className() { return "ConsoleBreakString"; }
    static std::string classDescription() { return "ConsoleBreakString object"; }

    ConsoleBreakString_QemuObject()
      : base()
    {
    }
    ConsoleBreakString_QemuObject(Qemu::API::conf_object_t* aQemuObject)
      : base(aQemuObject)
    {
    }
    ConsoleBreakString_QemuObject(ConsoleBreakString_QemuObject_Impl* anImpl)
      : base(anImpl)
    {
    }

    template<class Class>
    static void defineClass(Class& aClass)
    {
    }
};

Qemu::Factory<ConsoleBreakString_QemuObject> theConBreakFactory;

class RegressionTesting_QemuObject_Impl
{
    boost::intrusive_ptr<RegressionTracker> theRegressionTracker;

  public:
    RegressionTesting_QemuObject_Impl(Flexus::Qemu::API::conf_object_t* /*ignored*/)
      : theRegressionTracker(0)
    {
    }

    void setRegressionTracker(boost::intrusive_ptr<RegressionTracker> aTracker) { theRegressionTracker = aTracker; }

    void enable()
    {
        DBG_Assert(theRegressionTracker);
        theRegressionTracker->enable();
    }
};

class RegressionTesting_QemuObject : public Qemu::AddInObject<RegressionTesting_QemuObject_Impl>
{
    typedef Qemu::AddInObject<RegressionTesting_QemuObject_Impl> base;

  public:
    static const Qemu::Persistence class_persistence = Qemu::Session;
    static std::string className() { return "RegressionTesting"; }
    static std::string classDescription() { return "RegressionTesting object"; }

    RegressionTesting_QemuObject()
      : base()
    {
    }
    RegressionTesting_QemuObject(Qemu::API::conf_object_t* aQemuObject)
      : base(aQemuObject)
    {
    }
    RegressionTesting_QemuObject(RegressionTesting_QemuObject_Impl* anImpl)
      : base(anImpl)
    {
    }

    template<class Class>
    static void defineClass(Class& aClass)
    {
    }
};

Qemu::Factory<RegressionTesting_QemuObject> theRegressionTestingFactory;

class IterationTracker_QemuObject_Impl
{
    boost::intrusive_ptr<IterationTracker> theIterationTracker; // Non-owning pointer
  public:
    IterationTracker_QemuObject_Impl(Flexus::Qemu::API::conf_object_t* /*ignored*/)
      : theIterationTracker(0)
    {
    }

    void setIterationTracker(boost::intrusive_ptr<IterationTracker> anIterTracker)
    {
        theIterationTracker = anIterTracker;
    }

    void setIterationCount(int32_t aCpu, int32_t aCount)
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->setIterationCount(aCpu, aCount);
    }

    void printIterationCounts()
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->printIterationCounts(std::cout);
    }

    void enable()
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->enable();
    }

    void enableCheckpoints()
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->enableCheckpoints();
    }

    void endOnIteration(int32_t anInteration)
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->endOnIteration(anInteration);
    }

    void saveState(std::ostream& aStream)
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->saveState(aStream);
    }

    void loadState(std::istream& aStream)
    {
        DBG_Assert(theIterationTracker);
        theIterationTracker->loadState(aStream);
    }
};

class IterationTracker_QemuObject : public Qemu::AddInObject<IterationTracker_QemuObject_Impl>
{
    typedef Qemu::AddInObject<IterationTracker_QemuObject_Impl> base;

  public:
    static const Qemu::Persistence class_persistence = Qemu::Session;
    // These constants are defined in Qemu/simics.cpp
    static std::string className() { return "IterationTracker"; }
    static std::string classDescription() { return "IterationTracker object"; }

    IterationTracker_QemuObject()
      : base()
    {
    }
    IterationTracker_QemuObject(Qemu::API::conf_object_t* aQemuObject)
      : base(aQemuObject)
    {
    }
    IterationTracker_QemuObject(IterationTracker_QemuObject_Impl* anImpl)
      : base(anImpl)
    {
    }

    template<class Class>
    static void defineClass(Class& aClass)
    {
    }
};

Qemu::Factory<IterationTracker_QemuObject> theIterationTrackerFactory;

class FLEXUS_COMPONENT(MagicBreak)
{
    FLEXUS_COMPONENT_IMPL(MagicBreak);

    std::vector<boost::intrusive_ptr<BreakpointTracker>> theTrackers;
    IterationTracker_QemuObject theIterationTrackerObject;
    RegressionTesting_QemuObject theRegressionTestingObject;
    ConsoleBreakString_QemuObject theConsoleStringObject;
    boost::intrusive_ptr<CycleTracker> theCycleTracker;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(MagicBreak)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
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
        } catch (QemuException& e) {
            DBG_(Crit,
                 (<< "Cannot support graphical console. Need to switch to "
                     "string-based terminal"));
            exit(1);
        }
    }

    bool isQuiesced() const
    {
        return true; // MagicBreakComponent is always quiesced
    }

    void saveState(std::string const& aDirName) {}

    void loadState(std::string const& aDirName) {}

    void initialize()
    {
        if (cfg.EnableIterationCounts) {
            theIterationTrackerObject->enable();
            if (cfg.CheckpointOnIteration) { theIterationTrackerObject->enableCheckpoints(); }
            theIterationTrackerObject->endOnIteration(cfg.TerminateOnIteration);
        }
        if (cfg.TerminateOnMagicBreak >= 0) {
            theTrackers.push_back(BreakpointTracker::newTerminateOnMagicBreak(cfg.TerminateOnMagicBreak));
        }
        if (cfg.EnableTransactionCounts) {
            theTrackers.push_back(BreakpointTracker::newTransactionTracker(cfg.TransactionType,
                                                                           cfg.TerminateOnTransaction,
                                                                           cfg.TransactionStatsInterval,
                                                                           cfg.CheckpointEveryXTransactions,
                                                                           cfg.FirstTransactionIs,
                                                                           cfg.CycleMinimum));
        }
        if (cfg.StopCycle > 0 || cfg.CkptCycleInterval > 0) {
            theCycleTracker =
              BreakpointTracker::newCycleTracker(cfg.StopCycle, cfg.CkptCycleInterval, cfg.CkptCycleName);
        }
        theTrackers.push_back(BreakpointTracker::newSimPrintHandler());
        theTrackers.push_back(BreakpointTracker::newPacketTracker(8083 /*SpecWEB port*/,
                                                                  0x24 /*Server MAC byte*/,
                                                                  0x25 /*Client MAC byte*/));
    }

    void finalize() {}

    void drive(interface::TickDrive const&)
    {
        if (theCycleTracker) { theCycleTracker->tick(); }
    }
};

} // End namespace nMagicBreak

FLEXUS_COMPONENT_INSTANTIATOR(MagicBreak, nMagicBreak);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MagicBreak

#define DBG_Reset
#include DBG_Control()