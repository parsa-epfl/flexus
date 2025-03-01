#include "core/flexus.hpp"

#include "core/boost_extensions/padded_string_cast.hpp"
#include "core/component.hpp"
#include "core/configuration.hpp"
#include "core/debug/debug.hpp"
#include "core/drive_reference.hpp"
#include "core/exception.hpp"
#include "core/performance/profile.hpp"
#include "core/qemu/configuration_api.hpp"
#include "core/qemu/qmp_api.hpp"
#include "core/stats.hpp"
#include "core/target.hpp"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

static std::string config_file;

namespace Flexus {
namespace Core {

using Flexus::Wiring::theDrive;
using namespace std::chrono;

class FlexusImpl : public FlexusInterface
{
  private:
    uint64_t cpu_watchdog_timeout;
    std::vector<uint32_t> cpu_watchdogs;

    bool theInitialized;
    uint64_t theCycleCount;
    uint64_t theStatInterval;
    uint64_t theStopCycle;
    uint64_t cycle_delay_log;
    Stat::StatCounter theCycleCountStat;

    typedef std::vector<std::function<void()>> void_fn_vector;
    void_fn_vector theTerminateFunctions;

    bool theQuiesceRequested;
    bool theSaveRequested;

  public:
    // Initialization functions
    void initializeComponents();

    // The main cycle function
    void doCycle();
    void setCycle(uint64_t cycle);
    void advanceCycles(int64_t aCycleCount);
    void invokeDrives();

    // Simulator state inquiry
    bool quiescing() const { return theQuiesceRequested; }
    uint64_t cycleCount() const { return theCycleCount; }
    bool initialized() const { return theInitialized; }

    // Watchdog Functions
    void check_cpu_watchdogs(void);
    void reset_core_watchdog(uint32_t);

    // Flexus command line interface
    void setStopCycle(uint64_t aValue);
    void setStatInterval(uint64_t aValue);
    void set_log_delay(uint64_t aValue);
    void parseConfiguration(std::string const& aFilename);
    void writeMeasurement(std::string const& aMeasurement, std::string const& aFilename);
    void doLoad(std::string const& aDirName);
    void doSave(std::string const& aDirName);
    void setDebug(std::string const& aDebugSeverity);
    void terminateSimulation();

  public:
    FlexusImpl(Qemu::API::conf_object_t* anObject)
      : cpu_watchdog_timeout(2000000)
      , theInitialized(false)
      , theCycleCount(0)
      , theStatInterval(10000)
      , theStopCycle(2000000000)
      , cycle_delay_log(0)
      , theCycleCountStat("sys-cycles")
      , theQuiesceRequested(false)
      , theSaveRequested(false)
    {
        Flexus::Dbg::Debugger::theDebugger->connectCycleCount(&theCycleCount, &cycle_delay_log);
    }
    virtual ~FlexusImpl() {}
};

void
FlexusImpl::setCycle(uint64_t cycle)
{
    theCycleCount = cycle;
    theStopCycle += cycle;
    cycle_delay_log += cycle;
}

void
FlexusImpl::initializeComponents()
{
    DBG_(VVerb, (<< "Inititializing Flexus components..."));
    Stat::getStatManager()->initialize();
    parseConfiguration(config_file);
    ConfigurationManager::getConfigurationManager().checkAllOverrides();
    ComponentManager::getComponentManager().initComponents();
    theInitialized = true;

    cpu_watchdogs.reserve(ComponentManager::getComponentManager().systemWidth());

    for (std::size_t i{ 0 }; i < ComponentManager::getComponentManager().systemWidth(); i++)
        cpu_watchdogs.push_back(0);
}

void
FlexusImpl::advanceCycles(int64_t aCycleCount)
{

    static uint64_t advanced_cycle_count = 0;

    theCycleCount += aCycleCount;
    theCycleCountStat += aCycleCount;
    advanced_cycle_count += aCycleCount;

    Qemu::API::qemu_api.tick();

    if ((theStopCycle > 0) && (theCycleCount >= theStopCycle)) {
        DBG_(Dev, (<< "Reached target cycle count. Ending simulation."));
        terminateSimulation();
    }

    static uint64_t last_stats = 0;
    if (theStatInterval && (advanced_cycle_count - last_stats >= theStatInterval)) {
        DBG_(Dev, Core()(<< "Saving stats at: " << theCycleCount));
        std::string report_name =
          "all.measurement." + boost::padded_string_cast<10, '0'>(advanced_cycle_count) + ".log";
        writeMeasurement("all", report_name);
        last_stats = advanced_cycle_count;
    }

    Flexus::Dbg::Debugger::theDebugger->checkAt();

    Stat::getStatManager()->tick(aCycleCount);
}

void
FlexusImpl::invokeDrives()
{
    theDrive.doCycle();
}

void
FlexusImpl::doCycle()
{
    FLEXUS_PROFILE();

    FLEXUS_DBG("--------------START FLEXUS CYCLE " << theCycleCount << " ------------------------");

    advanceCycles(1);

    // Check the watchdog only every 255 cycles
    if ((static_cast<uint32_t>(theCycleCount) & 0xFF) == 0) check_cpu_watchdogs();

    invokeDrives();

    FLEXUS_DBG("--------------FINISH FLEXUS CYCLE " << theCycleCount - 1 << " ------------------------");
}

void
FlexusImpl::check_cpu_watchdogs()
{
    for (auto& watchdog : cpu_watchdogs) {
        DBG_Assert(watchdog < cpu_watchdog_timeout,
                   Core()(<< "Watchdog timer(" << cpu_watchdog_timeout << ") expired.  No progress by CPU for "
                          << watchdog << " cycles"));
        watchdog += 255; // incrementing by 255 because we check this function only every 0xFF cycles
    }
}

void
FlexusImpl::reset_core_watchdog(uint32_t core_idx)
{
    DBG_Assert(core_idx < cpu_watchdogs.size(), (<< "More core that watchdog"));
    cpu_watchdogs[core_idx] = 0;
}

void
FlexusImpl::set_log_delay(uint64_t aValue)
{
    cycle_delay_log = aValue;
    DBG_(Dev, Set((Source) << "flexus")(<< "Set LOG delay to : " << cycle_delay_log));
}

void
FlexusImpl::setStopCycle(uint64_t aValue)
{
    theStopCycle = aValue;
    DBG_(Dev, Set((Source) << "flexus")(<< "Set STOP to : " << theStopCycle));
}

void
FlexusImpl::setStatInterval(uint64_t aValue)
{
    theStatInterval = aValue;
    DBG_(Dev, Set((Source) << "flexus")(<< "Set STAT interval to : " << theStatInterval));
}

void
FlexusImpl::parseConfiguration(std::string const& aFilename)
{
    std::ifstream in(aFilename.c_str());
    ConfigurationManager::getConfigurationManager().parseConfiguration(in);
}

void
FlexusImpl::writeMeasurement(std::string const& aMeasurement, std::string const& aFilename)
{
    std::ofstream out(aFilename.c_str());
    Stat::getStatManager()->printMeasurement(aMeasurement, out);
    out.close();
}

void
FlexusImpl::doLoad(std::string const& aDirName)
{
    DBG_(Crit, (<< "Loading Flexus state from subdirectory " << aDirName));
    ComponentManager::getComponentManager().doLoad(aDirName);
}

void
FlexusImpl::doSave(std::string const& aDirName)
{
    DBG_(Crit, (<< "Saving Flexus state in subdirectory " << aDirName));
    ComponentManager::getComponentManager().doSave(aDirName);
}
void
FlexusImpl::setDebug(std::string const& aDebugSeverity)
{
    if (aDebugSeverity.compare("crit") == 0 || aDebugSeverity.compare("critical") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(Crit)));
        DBG_(Dev, (<< "Switched to Crit debugging."));
    } else if (aDebugSeverity.compare("dev") == 0 || aDebugSeverity.compare("development") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(Dev)));
        DBG_(Dev, (<< "Switched to Dev debugging."));
    } else if (aDebugSeverity.compare("trace") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(Trace)));
        DBG_(Dev, (<< "Switched to Trace debugging."));
    } else if (aDebugSeverity.compare("iface") == 0 || aDebugSeverity.compare("interface") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(Iface)));
        DBG_(Dev, (<< "Switched to Iface debugging."));
    } else if (aDebugSeverity.compare("verb") == 0 || aDebugSeverity.compare("verbose") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(Verb)));
        DBG_(Dev, (<< "Switched to Verb debugging."));
    } else if (aDebugSeverity.compare("vverb") == 0 || aDebugSeverity.compare("veryverbose") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(VVerb)));
        DBG_(Dev, (<< "Switched to VVerb debugging."));
    } else if (aDebugSeverity.compare("inv") == 0 || aDebugSeverity.compare("invocation") == 0) {
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity(DBG_internal_Sev_to_int(Inv)));
        DBG_(Dev, (<< "Switched to Inv debugging."));
    } else {
        std::cout << "Unknown debug severity: " << aDebugSeverity << ". Severity unchanged." << std::endl;
    }
}

void
FlexusImpl::terminateSimulation()
{
    system_clock::time_point now(system_clock::now());
    auto tt = system_clock::to_time_t(now);
    DBG_(Dev, Core()(<< "Terminating simulation. Timestamp: " << std::asctime(std::localtime(&tt))));
    DBG_(Dev, Core()(<< "Saving final mesurments"));
    ;
    for (void_fn_vector::iterator iter = theTerminateFunctions.begin(); iter != theTerminateFunctions.end(); ++iter) {
        (*iter)();
    }

    Flexus::Stat::getStatManager()->finalize();

    ComponentManager::getComponentManager().finalizeComponents();

    writeMeasurement("all", "all.measurement.end.log");
    Flexus::Qemu::API::qemu_api.stop("Simulation terminated by flexus.");
    exit(0);
}

class Flexus_Obj : public Flexus::Qemu::AddInObject<FlexusImpl>
{
    typedef Flexus::Qemu::AddInObject<FlexusImpl> base;

  public:
    static const Flexus::Qemu::Persistence class_persistence = Flexus::Qemu::Session;
    static std::string className() { return "Flexus"; }
    static std::string classDescription() { return "Flexus main class"; }

    Flexus_Obj()
      : base()
    {
    }
    Flexus_Obj(Flexus::Qemu::API::conf_object_t* anObject)
      : base(anObject)
    {
    }
    Flexus_Obj(FlexusImpl* anImpl)
      : base(anImpl)
    {
    }

    template<class Class>
    static void defineClass(Class& aC)
    {
        //	aClass = aClass; // normally, command definitions would go here
        // Don't think above does anything
        // in order to add commands to the QEMU command line to interface
        // with Flexus. But we haven't gotten around to this yet, because
        // we're awesome.
    }
};

typedef Qemu::Factory<Flexus_Obj> FlexusFactory;

Flexus_Obj theFlexusObj;
FlexusFactory* theFlexusFactory;
FlexusInterface* theFlexus = 0; // This is initialized from startup.cpp

void
setCfg(const char* aFile)
{
    config_file = aFile;
}

// Might not be best.
void
initFlexus()
{
    theFlexus->initializeComponents();
}

void
deinitFlexus()
{
    DBG_(VVerb, (<< "Cleaning up Flexus"));

    if (theFlexusFactory) delete theFlexusFactory;
}

void
PrepareFlexusObject()
{
    Core::theFlexusFactory = new FlexusFactory();
}

void
CreateFlexusObject()
{
    theFlexusObj = Core::theFlexusFactory->create("flexus");
    theFlexus    = &Core::theFlexusObj;
    if (!theFlexus) { DBG_Assert(false, (<< "Unable to create Flexus object in Simics")); }
}

void
flexus_qmp(int aCMD, const char* anArgs)
{
    qmp_flexus_cmd_t cmd = (qmp_flexus_cmd_t)(aCMD);

    try {

        qmp_flexus_i& q = qmp(cmd);

        if (anArgs != NULL)
            q.execute(static_cast<string>(anArgs));
        else
            q.execute("");

    } catch (qmp_not_implemented& e) {
        DBG_(Crit, (<< "QMP call not implemented!"));
    }
}

} // namespace Core
} // namespace Flexus
