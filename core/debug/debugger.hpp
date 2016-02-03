#include <vector>
#include <string>
#include <memory>
#include <queue>

#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include <core/boost_extensions/lexical_cast.hpp>

#include <core/debug/severity.hpp>
#include <core/debug/category.hpp>
#include <core/debug/field.hpp>
#include <core/debug/entry.hpp>
#include <core/debug/target.hpp>

#ifndef FLEXUS_CORE_DEBUG_DEBUGGER_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_DEBUGGER_HPP_INCLUDED

namespace Flexus {
namespace Dbg {

struct At {
  int64_t theCycle;
  Action * theAction;

  At( int64_t aCycle, Action * anAction)
    : theCycle(aCycle)
    , theAction(anAction)
  {}

  friend bool operator < (At const & aLeft, At const & aRight) {
    return aLeft.theCycle > aRight.theCycle;
  }
};

class Debugger {
  std::vector<Target *> theTargets; //Owns all targets
  std::map<std::string, bool *> theCategories;
  std::map<std::string, std::vector< bool *> > theComponents;

  int64_t theCount;
  uint64_t * theCycleCount;

  std::priority_queue<At> theAts; //Owns all targets

public:
  static Debugger * theDebugger;
  Severity theMinimumSeverity;

  Debugger()
    : theCount(0)
    , theCycleCount(0)
    , theMinimumSeverity(SevDev) {
  }

  ~Debugger();
  void initialize();
  void addFile(std::string const &);
  static void constructDebugger();

  int64_t count() {
    return ++theCount;
  }

  int64_t cycleCount() {
    if (theCycleCount)
      return *theCycleCount;
    else
      return 0;
  }

  void connectCycleCount(uint64_t * aCount) {
    theCycleCount = aCount;
  }

  void process(Entry const & anEntry);
  void printConfiguration(std::ostream & anOstream);
  void add(Target * aTarget);
  void registerCategory(std::string const & aCategory, bool * aSwitch);
  bool setAllCategories(bool aValue);
  bool setCategory(std::string const & aCategory, bool aValue);
  void listCategories(std::ostream & anOstream);
  void registerComponent(std::string const & aComponent, uint32_t anIndex, bool * aSwitch);
  bool setAllComponents(int32_t anIndex, bool aValue);
  bool setComponent(std::string const & aCategory, int32_t anIndex, bool aValue);
  void listComponents(std::ostream & anOstream);
  void addAt(int64_t aCycle, Action * anAction);
  void checkAt();
  void doNextAt();
  void reset();

  void setMinSev(Severity aSeverity) {
    theMinimumSeverity = aSeverity;
  }
};

struct DebuggerConstructor {
  DebuggerConstructor() {
    Debugger::constructDebugger();
  }
};

} //Dbg
} //Flexus

namespace {
Flexus::Dbg::DebuggerConstructor construct_debugger;
}

#endif //FLEXUS_CORE_DEBUG_DEBUGGER_HPP_INCLUDED

