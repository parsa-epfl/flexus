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
    virtual void initializeComponents() = 0;

    // The main cycle function
    virtual void doCycle()                = 0;
    virtual void setCycle(uint64_t cycle) = 0;
    virtual bool quiescing() const      = 0;
    // fast-forward through cycles
    // Note: does not tick stats, check watchdogs, or call drives

    // Simulator state inquiry
    virtual uint64_t cycleCount() const = 0;

    // Watchdog Functions
    virtual void reset_core_watchdog(uint32_t) = 0;

    virtual void terminateSimulation()              = 0;

    virtual void setDebug(std::string const& aDebugSeverity) = 0;
    virtual void setStatInterval(std::string const& aValue)      = 0;
    virtual void setStopCycle(std::string const& aValue)                                                 = 0;

    virtual void writeMeasurement(std::string const& aMeasurement, std::string const& aFilename)         = 0;
};

extern FlexusInterface* theFlexus;

void
flexus_qmp(int cmd, const char* arg);

}

#endif // FLEXUS_CORE_FLEXUS_HPP__INCLUDED
