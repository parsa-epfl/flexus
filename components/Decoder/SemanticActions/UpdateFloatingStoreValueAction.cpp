
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

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;
using nuArch::Instruction;

struct UpdateFloatingStoreValueAction : public BaseSemanticAction
{
    eSize theSize;

    UpdateFloatingStoreValueAction(SemanticInstruction* anInstruction, eSize aSize)
      : BaseSemanticAction(anInstruction, 2)
      , theSize(aSize)
    {
    }

    void doEvaluate()
    {
        switch (theSize) {
            case kWord: {
                bits value = theInstruction->operand<bits>(kfResult0);
                DBG_(VVerb, (<< *this << " updating store value=" << value));
                core()->updateStoreValue(boost::intrusive_ptr<Instruction>(theInstruction), value);
                break;
            }
            case kDoubleWord: {
                bits value = theInstruction->operand<bits>(kfResult0);
                value <<= 32;
                value |= theInstruction->operand<bits>(kfResult1);
                DBG_(VVerb, (<< *this << " updating store value=" << value));
                core()->updateStoreValue(boost::intrusive_ptr<Instruction>(theInstruction), value);
                break;
            }
            default: DBG_Assert(false);
        }
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " UpdateFloatingStoreValue";
    }
};

multiply_dependant_action
updateFloatingStoreValueAction(SemanticInstruction* anInstruction, eSize aSize)
{
    BaseSemanticAction* act = new UpdateFloatingStoreValueAction(anInstruction, aSize);
    anInstruction->addNewComponent(act);
    std::vector<InternalDependance> dependances;
    dependances.push_back(act->dependance(0));
    dependances.push_back(act->dependance(1));
    return multiply_dependant_action(act, dependances);
}

} // namespace nDecoder
