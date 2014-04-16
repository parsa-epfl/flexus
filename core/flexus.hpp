#include <boost/function.hpp>

#ifndef FLEXUS_CORE_FLEXUS_HPP__INCLUDED
#define FLEXUS_CORE_FLEXUS_HPP__INCLUDED

namespace Flexus {
namespace Core {

class FlexusInterface {
public:
  //Initialization functions
  virtual ~FlexusInterface() {}
  virtual void initializeComponents() = 0;

  //The main cycle function
  virtual void doCycle() = 0;
  //fast-forward through cycles
  //Note: does not tick stats, check watchdogs, or call drives
  virtual void advanceCycles(int64_t aCycleCount) = 0;
  virtual void invokeDrives() = 0;

  //Simulator state inquiry
  virtual bool isFastMode() const = 0;
  virtual bool isQuiesced() const = 0;
  virtual bool quiescing() const = 0;
  virtual uint64_t cycleCount() const = 0;
  virtual bool initialized() const = 0;

  //Watchdog Functions
  virtual void watchdogCheck() = 0;
  virtual void watchdogIncrement() = 0;
  virtual void watchdogReset(uint32_t anIndex) = 0;

  //Debugging support functions
  virtual int32_t breakCPU() const = 0;
  virtual int32_t breakInsn() const = 0;

  virtual void onTerminate( boost::function<void () > ) = 0;
  virtual void terminateSimulation() = 0;
  virtual void quiesce() = 0;
  virtual void quiesceAndSave(uint32_t aSaveNum) = 0;
  virtual void quiesceAndSave() = 0;

  virtual void setDebug(std::string const & aDebugSeverity) = 0;

  virtual void setStatInterval(std::string const & aValue) = 0;
  virtual void setProfileInterval(std::string const & aValue) = 0;
  virtual void setTimestampInterval(std::string const & aValue) = 0;
  virtual void setRegionInterval(std::string const & aValue) = 0;

};

extern FlexusInterface * theFlexus;

}  //End Namespace Core
} //namespace Flexus

#endif //FLEXUS_CORE_FLEXUS_HPP__INCLUDED
