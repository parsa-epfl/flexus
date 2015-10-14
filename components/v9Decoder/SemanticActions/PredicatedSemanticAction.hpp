#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <functional>

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"

namespace nv9Decoder {

using namespace nuArch;

struct PredicatedSemanticAction : public BaseSemanticAction {
  bool thePredicate;

  PredicatedSemanticAction ( SemanticInstruction * anInstruction, int32_t aNumArgs, bool anInitialPredicate );

  struct Pred : public DependanceTarget {
    PredicatedSemanticAction & theAction;
    Pred( PredicatedSemanticAction & anAction)
      : theAction(anAction)
    {}
    void satisfy(int32_t anArg) {
      theAction.predicate_on(anArg);
    }
    void squash(int32_t anArg) {
      theAction.predicate_off(anArg);
    }
  } thePredicateTarget;

  InternalDependance predicate() {
    return InternalDependance(&thePredicateTarget, 0);
  }

  virtual void squash(int);
  virtual void predicate_off(int);
  virtual void predicate_on(int);
  virtual void evaluate();
};

} //nv9Decoder
