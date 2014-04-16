#include <iostream>
#include <core/boost_extensions/intrusive_ptr.hpp>

namespace nMagicBreak {

class IterationTracker;
class RegressionTracker;
class CycleTracker;
class ConsoleStringTracker;

struct BreakpointTracker : public boost::counted_base {
public:
  static boost::intrusive_ptr<IterationTracker> newIterationTracker();
  static boost::intrusive_ptr<RegressionTracker> newRegressionTracker();
  static boost::intrusive_ptr<BreakpointTracker> newTransactionTracker( int32_t aTransactionType = 1, int32_t aStopTransactionCount = -1, int32_t aStatInterval = -1, int32_t aCkptInterval = -1, int32_t aFirstTransactionIs = 0, uint64_t aMinCycles = 0 );
  static boost::intrusive_ptr<BreakpointTracker> newTerminateOnMagicBreak(int32_t aMagicBreakpoint);
  static boost::intrusive_ptr<BreakpointTracker> newSimPrintHandler();
  static boost::intrusive_ptr<CycleTracker> newCycleTracker( uint64_t aStopCycle, uint64_t aCkptInterval, uint32_t aCkptNameStart );
  static boost::intrusive_ptr<BreakpointTracker> newPacketTracker( int32_t aSrcPortNumber, char aServerMACCode, char aClientMACCode );
  static boost::intrusive_ptr<ConsoleStringTracker> newConsoleStringTracker();
  virtual ~BreakpointTracker() {};
};

struct IterationTracker : public BreakpointTracker {
  virtual void printIterationCounts(std::ostream & aStream) = 0;
  virtual void setIterationCount(uint32_t aCPU, int32_t aCount) = 0;
  virtual void enable() = 0;
  virtual void endOnIteration(int32_t aCount) = 0;
  virtual void enableCheckpoints() = 0;
  virtual void saveState(std::ostream &) = 0;
  virtual void loadState(std::istream &) = 0;
  virtual ~IterationTracker() {};
};

struct RegressionTracker : public BreakpointTracker {
  virtual void enable() = 0;
  virtual ~RegressionTracker () {};
};

struct ConsoleStringTracker : public BreakpointTracker {
  virtual void addString(std::string const &) = 0;
  virtual ~ConsoleStringTracker () {};
};

struct SimPrintHandler : public BreakpointTracker {
  virtual ~SimPrintHandler() {};
};

struct CycleTracker : public BreakpointTracker {
  virtual void tick() = 0;
  virtual ~CycleTracker() {};
};

} //namespace nMagicBreak

