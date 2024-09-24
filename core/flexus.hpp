#ifndef FLEXUS_CORE_FLEXUS_HPP__INCLUDED
#define FLEXUS_CORE_FLEXUS_HPP__INCLUDED

#include <functional>
#include <stdint.h>
#include <string>

namespace Flexus::Core {

class FlexusInterface
{
  public:
    // Initialization functions
    virtual ~FlexusInterface() {}
    virtual void initializeComponents() = 0;

    // The main cycle function
    virtual void doCycle()                = 0;
    virtual void setCycle(uint64_t cycle) = 0;
    // fast-forward through cycles
    // Note: does not tick stats, check watchdogs, or call drives
    virtual void advanceCycles(int64_t aCycleCount) = 0;
    virtual void invokeDrives()                     = 0;

    // Simulator state inquiry
    virtual bool isFastMode() const     = 0;
    virtual bool isQuiesced() const     = 0;
    virtual bool quiescing() const      = 0;
    virtual uint64_t cycleCount() const = 0;
    virtual bool initialized() const    = 0;

    // Watchdog Functions
    virtual void check_cpu_watchdogs(void) = 0;
    virtual void reset_core_watchdog(uint32_t) = 0;

    // Debugging support functions
    virtual int32_t breakCPU() const  = 0;
    virtual int32_t breakInsn() const = 0;

    virtual void onTerminate(std::function<void()>) = 0;
    virtual void terminateSimulation()              = 0;
    virtual void quiesce()                          = 0;
    virtual void quiesceAndSave(uint32_t aSaveNum)  = 0;
    virtual void quiesceAndSave()                   = 0;

    virtual void setDebug(std::string const& aDebugSeverity) = 0;

    virtual void setStatInterval(std::string const& aValue)      = 0;
    virtual void setProfileInterval(std::string const& aValue)   = 0;
    virtual void setTimestampInterval(std::string const& aValue) = 0;
    virtual void setRegionInterval(std::string const& aValue)    = 0;

    virtual void printCycleCount()                                                                       = 0;
    virtual void setStopCycle(std::string const& aValue)                                                 = 0;
    virtual void setBreakCPU(int32_t aCPU)                                                               = 0;
    virtual void setBreakInsn(std::string const& aValue)                                                 = 0;
    virtual void printProfile()                                                                          = 0;
    virtual void resetProfile()                                                                          = 0;
    virtual void writeProfile(std::string const& aFilename)                                              = 0;
    virtual void printConfiguration()                                                                    = 0;
    virtual void writeConfiguration(std::string const& aFilename)                                        = 0;
    virtual void parseConfiguration(std::string const& aFilename)                                        = 0;
    virtual void setConfiguration(std::string const& aName, std::string const& aValue)                   = 0;
    virtual void writeMeasurement(std::string const& aMeasurement, std::string const& aFilename)         = 0;
    virtual void enterFastMode()                                                                         = 0;
    virtual void leaveFastMode()                                                                         = 0;
    virtual void saveState(std::string const& aDirName)                                                  = 0;
    virtual void saveJustFlexusState(std::string const& aDirName)                                        = 0;
    virtual void loadState(std::string const& aDirName)                                                  = 0;
    virtual void doLoad(std::string const& aDirName)                                                     = 0;
    virtual void doSave(std::string const& aDirName, bool justFlexus = false)                            = 0;
    virtual void reloadDebugCfg()                                                                        = 0;
    virtual void addDebugCfg(std::string const& aFilename)                                               = 0;
    virtual void enableCategory(std::string const& aComponent)                                           = 0;
    virtual void disableCategory(std::string const& aComponent)                                          = 0;
    virtual void listCategories()                                                                        = 0;
    virtual void enableComponent(std::string const& aComponent, std::string const& anIndex)              = 0;
    virtual void disableComponent(std::string const& aComponent, std::string const& anIndex)             = 0;
    virtual void listComponents()                                                                        = 0;
    virtual void printDebugConfiguration()                                                               = 0;
    virtual void writeDebugConfiguration(std::string const& aFilename)                                   = 0;
    virtual void log(std::string const& aName, std::string const& anInterval, std::string const& aRegEx) = 0;
    virtual void printMMU(int32_t aCPU)                                                                  = 0;
};

extern FlexusInterface* theFlexus;

void
flexus_qmp(int cmd, const char* arg);

}

#endif // FLEXUS_CORE_FLEXUS_HPP__INCLUDED
