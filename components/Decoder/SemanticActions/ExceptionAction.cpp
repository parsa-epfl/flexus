
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
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

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

struct ExceptionAction : public BaseSemanticAction
{

    ExceptionAction(SemanticInstruction* anInstruction)
      : BaseSemanticAction(anInstruction, 1)
    {
    }

    void doEvaluate() {}

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " ExceptionAction "; }
};

simple_action
exceptionAction(SemanticInstruction* anInstruction)
{
    ExceptionAction* act = new ExceptionAction(anInstruction);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

} // namespace nDecoder
