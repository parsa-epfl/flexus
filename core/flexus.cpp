//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#include <core/target.hpp>

#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/debug/debug.hpp>
#include <core/performance/profile.hpp>

#include <core/drive_reference.hpp>
#include <core/metaprogram.hpp>
#include <core/qemu/qmp_api.hpp>
#include <functional>

#include <core/stats.hpp>

#include <core/exception.hpp>

#include <core/boost_extensions/padded_string_cast.hpp>

// #ifndef __STDC_CONSTANT_MACROS
// #define __STDC_CONSTANT_MACROS
// #endif
// #include <boost/date_time/posix_time/posix_time.hpp>

#include <core/flexus.hpp>
#include <core/qemu/configuration_api.hpp>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

static std::string config_file;

namespace Flexus {
namespace Core {

using Flexus::Wiring::theDrive;
using namespace std::chrono;

class FlexusImpl : public FlexusInterface {
private:
  uint64_t theWatchdogTimeout;
  std::vector<uint32_t> theWatchdogCounts;
  uint32_t theNumWatchdogs;
  bool theInitialized;
  uint64_t theCycleCount;
  uint64_t theStatInterval;
  uint64_t theRegionInterval;
  uint64_t theProfileInterval;
  uint64_t theTimestampInterval;
  uint64_t theStopCycle;
  Stat::StatCounter theCycleCountStat;

  std::string theCurrentStatRegionName;
  uint32_t theCurrentStatRegion;

  typedef std::vector<std::function<void()>> void_fn_vector;
  void_fn_vector theTerminateFunctions;

  bool theWatchdogWarning;
  bool theQuiesceRequested;
  bool theSaveRequested;
  std::string theSaveName;

  bool theFastMode;

  int32_t theBreakCPU;
  uint64_t theBreakInsn;
  int32_t theSaveCtr;

public:
  // Initialization functions
  void initializeComponents();

  // The main cycle function
  void doCycle();
  void advanceCycles(int64_t aCycleCount);
  void invokeDrives();

  // Simulator state inquiry
  bool isFastMode() const {
    return theFastMode;
  }
  bool isQuiesced() const {
    if (!initialized()) {
      return true;
    }
    return ComponentManager::getComponentManager().isQuiesced();
  }
  bool quiescing() const {
    return theQuiesceRequested;
  }
  uint64_t cycleCount() const {
    return theCycleCount;
  }
  bool initialized() const {
    return theInitialized;
  }

  // Watchdog Functions
  void setWatchdogTimeout(std::string const &aTimeoutStr);
  void watchdogCheck();
  void watchdogIncrement();
  void watchdogReset(uint32_t anIndex);

  // Debugging support functions
  int32_t breakCPU() const {
    return theBreakCPU;
  }
  int32_t breakInsn() const {
    return theBreakInsn;
  }

  // Flexus command line interface
  void printCycleCount();
  void setStopCycle(std::string const &aValue);
  void setStatInterval(std::string const &aValue);
  void setRegionInterval(std::string const &aValue);
  void setBreakCPU(int32_t aCPU);
  void setBreakInsn(std::string const &aValue);
  void setProfileInterval(std::string const &aValue);
  void setTimestampInterval(std::string const &aValue);
  void printProfile();
  void resetProfile();
  void writeProfile(std::string const &aFilename);
  void printConfiguration();
  void writeConfiguration(std::string const &aFilename);
  void parseConfiguration(std::string const &aFilename);
  void setConfiguration(std::string const &aName, std::string const &aValue);
  void printMeasurement(std::string const &aMeasurement);
  void listMeasurements();
  void writeMeasurement(std::string const &aMeasurement, std::string const &aFilename);
  void enterFastMode();
  void leaveFastMode();
  void quiesce();
  void quiesceAndSave(uint32_t aSaveNum);
  void quiesceAndSave();
  void saveState(std::string const &aDirName);
  void saveJustFlexusState(std::string const &aDirName);
  void loadState(std::string const &aDirName);
  void doLoad(std::string const &aDirName);
  void doSave(std::string const &aDirName, bool justFlexus = false);
  void backupStats(std::string const &aFilename) const;
  void saveStats(std::string const &aFilename) const;
  void saveStatsUncompressed(std::ofstream &anOstream) const;
  void saveStatsCompressed(std::ofstream &anOstream) const;
  void reloadDebugCfg();
  void addDebugCfg(std::string const &aFilename);
  void setDebug(std::string const &aDebugSeverity);
  void enableCategory(std::string const &aComponent);
  void disableCategory(std::string const &aComponent);
  void listCategories();
  void enableComponent(std::string const &aComponent, std::string const &anIndex);
  void disableComponent(std::string const &aComponent, std::string const &anIndex);
  void listComponents();
  void printDebugConfiguration();
  void writeDebugConfiguration(std::string const &aFilename);
  void onTerminate(std::function<void()>);
  void terminateSimulation();
  void log(std::string const &aName, std::string const &anInterval, std::string const &aRegEx);
  void printMMU(int32_t aCPU);

public:
  FlexusImpl(Qemu::API::conf_object_t *anObject)
      : theWatchdogTimeout(100000), theNumWatchdogs(0), theInitialized(false), theCycleCount(0),
        theStatInterval(100), theRegionInterval(100000000), theProfileInterval(1000000),
        theTimestampInterval(100000), theStopCycle(2000000000), theCycleCountStat("sys-cycles"),
        theWatchdogWarning(false), theQuiesceRequested(false), theSaveRequested(false),
        theFastMode(false), theBreakCPU(-1), theBreakInsn(0), theSaveCtr(1) {
    Flexus::Dbg::Debugger::theDebugger->connectCycleCount(&theCycleCount);
  }
  virtual ~FlexusImpl() {
  }
};

void FlexusImpl::printMMU(int32_t aCPU) {
  DBG_(Crit, (<< "printMMU not implemented yet. Still need to port mai_api.hpp "));
}

void FlexusImpl::initializeComponents() {
  DBG_(VVerb, (<< "Inititializing Flexus components..."));
  Stat::getStatManager()->initialize();
  theCurrentStatRegion = 0;
  theCurrentStatRegionName =
      std::string("Region ") + boost::padded_string_cast<3, '0'>(theCurrentStatRegion++);
  Stat::getStatManager()->openMeasurement(theCurrentStatRegionName);
  parseConfiguration(config_file);
  writeConfiguration("configuration.out");
  ConfigurationManager::getConfigurationManager().checkAllOverrides();
  ComponentManager::getComponentManager().initComponents();
  theInitialized = true;
}

void FlexusImpl::advanceCycles(int64_t aCycleCount) {
  theCycleCount += aCycleCount;
  theCycleCountStat += aCycleCount;

  if (theQuiesceRequested && isQuiesced()) {
    DBG_(Crit, (<< "Flexus is quiesced as of cycle " << theCycleCount));
    theQuiesceRequested = false;
    if (theSaveRequested) {
      theSaveRequested = false;
      doSave(theSaveName);
    } else {
      Qemu::API::qemu_api.stop("Flexus is quiesced.");
      return;
    }
  }

  // Check how much time has elapsed every 1024*1024 cycles
  static uint64_t last_timestamp = 0;
  if (!theCycleCount || theCycleCount - last_timestamp >= theTimestampInterval) {
    system_clock::time_point now(system_clock::now());
    auto tt = system_clock::to_time_t(now);
    DBG_(Dev, Core()(<< "Timestamp: " << std::asctime(std::localtime(&tt))));
    last_timestamp = theCycleCount;
  }

  if ((theStopCycle > 0) && (theCycleCount >= theStopCycle)) {
    DBG_(Dev, (<< "Reached target cycle count. Ending simulation."));
    terminateSimulation();
  }

  static uint64_t last_stats = 0;
  if (theCycleCount - last_stats >= theStatInterval) {
    DBG_(Dev, Core()(<< "Saving stats at: " << theCycleCount));
    backupStats("stats_db");

#ifdef WRITE_ALL_MEASUREMENT_OUT
    writeMeasurement("all", "all.measurement.out");
#endif

    last_stats = theCycleCount;
  }

  static uint64_t last_region = 0;
  if (theCycleCount - last_region >= theRegionInterval) {
    Stat::getStatManager()->closeMeasurement(theCurrentStatRegionName);
    theCurrentStatRegionName =
        std::string("Region ") + boost::padded_string_cast<3, '0'>(theCurrentStatRegion++);
    Stat::getStatManager()->openMeasurement(theCurrentStatRegionName);

    last_region = theCycleCount;
  }

  static uint64_t last_profile = 0;
  if (theProfileInterval > 0 && theCycleCount - last_profile >= theProfileInterval) {
    // DBG_(Dev, Core() ( << "Writing profile at: " << theCycleCount));
    DBG_(Dev, Core()(<< "Profiling disabled"));

    // writeProfile("profile.out");
    // resetProfile();
    last_profile = theCycleCount;
  }

  Flexus::Dbg::Debugger::theDebugger->checkAt();

  Stat::getStatManager()->tick(aCycleCount);
}

void FlexusImpl::invokeDrives() {
  theDrive.doCycle();
}

void FlexusImpl::doCycle() {
  FLEXUS_PROFILE();

  FLEXUS_DBG("--------------START FLEXUS CYCLE " << theCycleCount << " ------------------------");

  advanceCycles(1);

  uint32_t recent_watchdog_count = ((uint32_t)theCycleCount) & 0xFF;
  if (recent_watchdog_count == 0) {
    // Check for watchdog timeout
    watchdogCheck();
    watchdogIncrement();
  }

  invokeDrives();

  FLEXUS_DBG("--------------FINISH FLEXUS CYCLE " << theCycleCount - 1
                                                  << " ------------------------");
}

void FlexusImpl::setWatchdogTimeout(std::string const &aTimeoutStr) {
  std::istringstream ss(aTimeoutStr, std::istringstream::in);
  ss >> theWatchdogTimeout;
}

void FlexusImpl::watchdogCheck() {
  for (uint32_t i = 0; i < theNumWatchdogs; ++i) {
    // We get 10k cycles of Iface trace after a watchdog timeout before we
    // assert and kill Flexus
    if (!(theWatchdogCounts[i] < (uint32_t)(0.8 * theWatchdogTimeout))) {
      //  if (!( theWatchdogCounts[i] < 90000)) {

      if (!theWatchdogWarning) {
        theWatchdogWarning = true;
        DBG_(Crit, (<< "Watchdog timer expired.  No progress by CPU " << i << " for  "
                    << theWatchdogCounts[i] << "cycles"));
        Flexus::Dbg::Debugger::theDebugger->setMinSev(
            Dbg::Severity(DBG_internal_Sev_to_int(Iface)));
      }
    }
    DBG_Assert(theWatchdogCounts[i] < theWatchdogTimeout + 10,
               Core()(<< "Watchdog timer expired.  No progress by CPU " << i << " for  "
                      << theWatchdogCounts[i] << "cycles"));
  }
}

void FlexusImpl::watchdogIncrement() {
  for (auto &aWatchdogCount: theWatchdogCounts) {
    aWatchdogCount += 255;
  }
}

void FlexusImpl::watchdogReset(uint32_t anIndex) {
  if (anIndex >= theNumWatchdogs) {
    theNumWatchdogs = anIndex + 1;
    theWatchdogCounts.resize(theNumWatchdogs, 0);
  } else {
    theWatchdogCounts[anIndex] = 0;
  }
}

void FlexusImpl::printCycleCount() {
  DBG_(Crit, Cat(Stats) Set((Source) << "flexus")(<< "Cycle count: " << theCycleCount));
}

void FlexusImpl::setStopCycle(std::string const &aValue) {
  theStopCycle = boost::lexical_cast<uint64_t>(aValue);
}

void FlexusImpl::setStatInterval(std::string const &aValue) {
  theStatInterval = boost::lexical_cast<uint64_t>(aValue);
  DBG_(Dev, Set((Source) << "flexus")(<< "Set stat interval to : " << theStatInterval));
}

void FlexusImpl::setRegionInterval(std::string const &aValue) {
  theRegionInterval = boost::lexical_cast<uint64_t>(aValue);
  DBG_(Dev, Set((Source) << "flexus")(<< "Set region interval to : " << theRegionInterval));
}

void FlexusImpl::setBreakCPU(int32_t aCPU) {
  theBreakCPU = aCPU;
  DBG_(Dev, Set((Source) << "flexus")(<< "Set break CPU to : " << aCPU));
}

void FlexusImpl::setBreakInsn(std::string const &aValue) {
  theBreakInsn = boost::lexical_cast<uint64_t>(aValue);
  DBG_(Dev, Set((Source) << "flexus")(<< "Set break instruction # to : " << theBreakInsn));
}

void FlexusImpl::setProfileInterval(std::string const &aValue) {
  theProfileInterval = boost::lexical_cast<uint64_t>(aValue);
  DBG_(Dev, Set((Source) << "flexus")(<< "Set profile interval to : " << theProfileInterval));
}

void FlexusImpl::setTimestampInterval(std::string const &aValue) {
  theTimestampInterval = boost::lexical_cast<uint64_t>(aValue);
  DBG_(Dev, Set((Source) << "flexus")(<< "Set timestamp interval to : " << theTimestampInterval));
}

void FlexusImpl::enterFastMode() {
  if (!theFastMode) {
    theFastMode = true;
    DBG_(Dev, Set((Source) << "flexus")(<< "Fast Mode enabled"));
  }
}

void FlexusImpl::leaveFastMode() {
  if (theFastMode) {
    theFastMode = false;
    DBG_(Dev, Set((Source) << "flexus")(<< "Fast Mode disabled"));
  }
}

void FlexusImpl::printProfile() {
  nProfile::ProfileManager::profileManager()->report(std::cout);
}

void FlexusImpl::resetProfile() {
  nProfile::ProfileManager::profileManager()->reset();
}

void FlexusImpl::writeProfile(std::string const &aFilename) {
  std::ofstream out(aFilename.c_str(), std::ios::app);
  out << "Profile as of " << theCycleCount << std::endl;
  nProfile::ProfileManager::profileManager()->report(out);
  out << std::endl;
}

void FlexusImpl::printConfiguration() {
  ConfigurationManager::getConfigurationManager().printConfiguration(std::cout);
}

void FlexusImpl::writeConfiguration(std::string const &aFilename) {
  std::ofstream out(aFilename.c_str());
  ConfigurationManager::getConfigurationManager().printConfiguration(out);
}

void FlexusImpl::parseConfiguration(std::string const &aFilename) {
  std::ifstream in(aFilename.c_str());
  ConfigurationManager::getConfigurationManager().parseConfiguration(in);
}

void FlexusImpl::setConfiguration(std::string const &aName, std::string const &aValue) {
  ConfigurationManager::getConfigurationManager().set(aName, aValue);
}

void FlexusImpl::printMeasurement(std::string const &aMeasurement) {
  Stat::getStatManager()->printMeasurement(aMeasurement, std::cout);
}

void FlexusImpl::listMeasurements() {
  Stat::getStatManager()->listMeasurements(std::cout);
}

void FlexusImpl::log(std::string const &aName, std::string const &anInterval,
                     std::string const &aRegEx) {
  uint64_t interval = boost::lexical_cast<uint64_t>(anInterval);
  DBG_(Dev, Set((Source) << "flexus")(<< "Logging: " << aRegEx << " every " << anInterval
                                      << " cycles as measurement " << aName << " to "
                                      << aName + ".out"));
  std::string filename(aName + ".out");
  std::ofstream *log = new std::ofstream(filename.c_str()); // Leaks intentionally
  Stat::getStatManager()->openLoggedPeriodicMeasurement(
      aName.c_str(), interval, Stat::accumulation_type::Accumulate, *log, aRegEx.c_str());
}

void FlexusImpl::writeMeasurement(std::string const &aMeasurement, std::string const &aFilename) {
  std::ofstream out(aFilename.c_str());
  Stat::getStatManager()->printMeasurement(aMeasurement, out);
  out.close();
}

void FlexusImpl::quiesce() {
  if (!isQuiesced()) {
    DBG_(Crit, (<< "Flexus will quiesce when simulation continues"));
    theQuiesceRequested = true;
  }
}

void FlexusImpl::quiesceAndSave() {
  if (!isQuiesced()) {
    DBG_(Crit, (<< "Flexus will quiesce when simulation continues"));
    theQuiesceRequested = true;
    theSaveRequested = true;
    theSaveName = "newckpt-" + boost::padded_string_cast<4, '0'>(theSaveCtr);
  } else {
    doSave("newckpt-" + boost::padded_string_cast<4, '0'>(theSaveCtr));
  }
  ++theSaveCtr;
}

void FlexusImpl::quiesceAndSave(uint32_t aSaveNum) {
  if (!isQuiesced()) {
    DBG_(Crit, (<< "Flexus will quiesce when simulation continues"));
    theQuiesceRequested = true;
    theSaveRequested = true;
    theSaveName = "ckpt-" + boost::padded_string_cast<4, '0'>(aSaveNum);
  } else {
    doSave("ckpt-" + boost::padded_string_cast<4, '0'>(aSaveNum));
  }
}

void FlexusImpl::saveState(std::string const &aDirName) {
  if (isQuiesced()) {
    doSave(aDirName);
  } else {
    DBG_(Crit, (<< "Flexus cannot save unless the system is quiesced. Use the "
                   "command \"flexus.quiesce\" to quiesce the system"));
  }
}

void FlexusImpl::saveJustFlexusState(std::string const &aDirName) {
  if (isQuiesced()) {
    doSave(aDirName, true);
  } else {
    DBG_(Crit, (<< "Flexus cannot save unless the system is quiesced. Use the "
                   "command \"flexus.quiesce\" to quiesce the system"));
  }
}

void FlexusImpl::loadState(std::string const &aDirName) {
  if (isQuiesced()) {
    doLoad(aDirName);
  } else {
    DBG_(Crit, (<< "Flexus cannot load unless the system is quiesced. Use the "
                   "command \"flexus.quiesce\" to quiesce the system"));
  }
}

void FlexusImpl::doLoad(std::string const &aDirName) {
  DBG_(Crit, (<< "Loading Flexus state from subdirectory " << aDirName));
  ComponentManager::getComponentManager().doLoad(aDirName);
}

void FlexusImpl::doSave(std::string const &aDirName, bool justFlexus) {
  if (justFlexus) {
    DBG_(Crit, (<< "Saving Flexus state in subdirectory " << aDirName));
  } else {
    DBG_(Crit, (<< "Saving Flexus and Qemu state in subdirectory " << aDirName));
  }
  ComponentManager::getComponentManager().doSave(aDirName);
  DBG_(Crit, (<< "Saving Flexus state in subdirectory " << aDirName));
}

void FlexusImpl::backupStats(std::string const &aFilename) const {
  std::string fullName = aFilename + std::string(".out.gz");
  std::string last1Name = aFilename + std::string(".001.out.gz");
  remove(last1Name.c_str());
  rename(fullName.c_str(), last1Name.c_str());

  saveStats(fullName);

  remove(last1Name.c_str());
}

void FlexusImpl::saveStats(std::string const &aFilename) const {
  std::ofstream anOstream(aFilename.c_str(), std::ios::binary);
  size_t loc = aFilename.rfind(".gz");
  if (loc == std::string::npos) {
    saveStatsUncompressed(anOstream);
  } else {
    saveStatsCompressed(anOstream);
  }
}

void FlexusImpl::saveStatsUncompressed(std::ofstream &anOstream) const {
  Stat::getStatManager()->save(anOstream);
  anOstream.close();
}

void FlexusImpl::saveStatsCompressed(std::ofstream &anOstream) const {
  boost::iostreams::filtering_ostream out;
  out.push(boost::iostreams::gzip_compressor());
  out.push(anOstream);
  Stat::getStatManager()->save(out);
  out.reset();
}

void FlexusImpl::reloadDebugCfg() {
  Flexus::Dbg::Debugger::theDebugger->reset();
  Flexus::Dbg::Debugger::theDebugger->initialize();
}

void FlexusImpl::addDebugCfg(std::string const &aFilename) {
  Flexus::Dbg::Debugger::theDebugger->addFile(aFilename);
}

void FlexusImpl::setDebug(std::string const &aDebugSeverity) {
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
    std::cout << "Unknown debug severity: " << aDebugSeverity << ". Severity unchanged."
              << std::endl;
  }
}

void FlexusImpl::enableCategory(std::string const &aCategory) {
  if (Flexus::Dbg::Debugger::theDebugger->setCategory(aCategory, true)) {
    DBG_(Dev, (<< "Enabled debugging for " << aCategory));
  } else {
    DBG_(Dev, (<< "No category named " << aCategory));
  }
}
void FlexusImpl::disableCategory(std::string const &aCategory) {
  if (Flexus::Dbg::Debugger::theDebugger->setCategory(aCategory, false)) {
    DBG_(Dev, (<< "Disabled debugging for " << aCategory));
  } else {
    DBG_(Dev, (<< "No category named " << aCategory));
  }
}

void FlexusImpl::listCategories() {
  Flexus::Dbg::Debugger::theDebugger->listCategories(std::cout);
}
void FlexusImpl::enableComponent(std::string const &aComponent, std::string const &anIndexStr) {
  int32_t anIndex = -1;
  if (anIndexStr != "all") {
    try {
      anIndex = boost::lexical_cast<int>(anIndexStr);
    } catch (boost::bad_lexical_cast &ignored) {
      DBG_(Dev, (<< "Invalid component index " << anIndexStr));
      return;
    }
  }
  if (Flexus::Dbg::Debugger::theDebugger->setComponent(aComponent, anIndex, true)) {
    DBG_(Dev, (<< "Enabled debugging for " << aComponent << "[" << anIndexStr << "]"));
  } else {
    DBG_(Dev, (<< "No such component " << aComponent << "[" << anIndexStr << "]"));
  }
}
void FlexusImpl::disableComponent(std::string const &aComponent, string const &anIndexStr) {
  int32_t anIndex = -1;
  if (anIndexStr != "all") {
    try {
      anIndex = boost::lexical_cast<int>(anIndexStr);
    } catch (boost::bad_lexical_cast &ignored) {
      DBG_(Dev, (<< "Invalid component index " << anIndexStr));
      return;
    }
  }
  if (Flexus::Dbg::Debugger::theDebugger->setComponent(aComponent, anIndex, false)) {
    DBG_(Dev, (<< "Disabled debugging for " << aComponent << "[" << anIndexStr << "]"));
  } else {
    DBG_(Dev, (<< "No such component " << aComponent << "[" << anIndexStr << "]"));
  }
}
void FlexusImpl::listComponents() {
  Flexus::Dbg::Debugger::theDebugger->listComponents(std::cout);
}

void FlexusImpl::printDebugConfiguration() {
  Flexus::Dbg::Debugger::theDebugger->printConfiguration(std::cout);
}

void FlexusImpl::writeDebugConfiguration(std::string const &aFilename) {
  std::ofstream out(aFilename.c_str());
  Flexus::Dbg::Debugger::theDebugger->printConfiguration(out);
}

void FlexusImpl::onTerminate(std::function<void()> aFn) {
  theTerminateFunctions.push_back(aFn);
}

void FlexusImpl::terminateSimulation() {
  system_clock::time_point now(system_clock::now());
  auto tt = system_clock::to_time_t(now);
  DBG_(Dev, Core()(<< "Terminating simulation. Timestamp: " << std::asctime(std::localtime(&tt))));
  DBG_(Dev, Core()(<< "Saving final stats_db."));
  ;
  for (void_fn_vector::iterator iter = theTerminateFunctions.begin();
       iter != theTerminateFunctions.end(); ++iter) {
    (*iter)();
  }

  Flexus::Stat::getStatManager()->finalize();
  backupStats("stats_db");

  ComponentManager::getComponentManager().finalizeComponents();

#ifdef WRITE_ALL_MEASUREMENT_OUT
  writeMeasurement("all", "all.measurement.out");
#endif
  Flexus::Qemu::API::qemu_api.stop("Simulation terminated by flexus.");
}

class Flexus_Obj : public Flexus::Qemu::AddInObject<FlexusImpl> {
  typedef Flexus::Qemu::AddInObject<FlexusImpl> base;

public:
  static const Flexus::Qemu::Persistence class_persistence = Flexus::Qemu::Session;
  static std::string className() {
    return "Flexus";
  }
  static std::string classDescription() {
    return "Flexus main class";
  }

  Flexus_Obj() : base() {
  }
  Flexus_Obj(Flexus::Qemu::API::conf_object_t *anObject) : base(anObject) {
  }
  Flexus_Obj(FlexusImpl *anImpl) : base(anImpl) {
  }

  template <class Class> static void defineClass(Class &aC) {
    //	aClass = aClass; // normally, command definitions would go here
    // Don't think above does anything
    // in order to add commands to the QEMU command line to interface
    // with Flexus. But we haven't gotten around to this yet, because
    // we're awesome.
  }
};

typedef Qemu::Factory<Flexus_Obj> FlexusFactory;

Flexus_Obj theFlexusObj;
FlexusFactory *theFlexusFactory;
FlexusInterface *theFlexus = 0; // This is initialized from startup.cpp

void setCfg(const char *aFile) {
  config_file = aFile;
}

// Might not be best.
void initFlexus() {
  theFlexus->initializeComponents();
}

void deinitFlexus() {
  DBG_(VVerb, (<< "Cleaning up Flexus"));

  if (theFlexusFactory)
    delete theFlexusFactory;
}

void PrepareFlexusObject() {
  Core::theFlexusFactory = new FlexusFactory();
}

void CreateFlexusObject() {
  theFlexusObj = Core::theFlexusFactory->create("flexus");
  theFlexus = &Core::theFlexusObj;
  if (!theFlexus) {
    DBG_Assert(false, (<< "Unable to create Flexus object in Simics"));
  }
}

void flexus_qmp(int aCMD, const char* anArgs) {
 qmp_flexus_cmd_t cmd = (qmp_flexus_cmd_t)(aCMD);

  try {

    qmp_flexus_i &q = qmp(cmd);

    if (anArgs != NULL)
      q.execute(static_cast<string>(anArgs));
    else
      q.execute("");

  }
  catch (qmp_not_implemented &e)
  {
    DBG_(Crit, (<< "QMP call not implemented!"));
  }

}

} // namespace Core
} // namespace Flexus
