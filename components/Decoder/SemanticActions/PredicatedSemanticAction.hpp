
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iomanip>
#include <iostream>
namespace ll = boost::lambda;

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/none.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

namespace nDecoder {

using namespace nuArch;

struct PredicatedSemanticAction : public BaseSemanticAction
{
    bool thePredicate;

    PredicatedSemanticAction(SemanticInstruction* anInstruction, int32_t aNumArgs, bool anInitialPredicate);

    struct Pred : public DependanceTarget
    {
        PredicatedSemanticAction& theAction;
        Pred(PredicatedSemanticAction& anAction)
          : theAction(anAction)
        {
        }
        void satisfy(int32_t anArg)
        {
            SEMANTICS_TRACE;
            theAction.predicate_on(anArg);
        }
        void squash(int32_t anArg) { theAction.predicate_off(anArg); }
    } thePredicateTarget;

    InternalDependance predicate() { return InternalDependance(&thePredicateTarget, 0); }

    virtual void squash(int);
    virtual void predicate_off(int);
    virtual void predicate_on(int);
    virtual void evaluate();
};

} // namespace nDecoder
