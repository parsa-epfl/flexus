
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

struct ReadConstantAction : public BaseSemanticAction
{
    eOperandCode theOperandCode;
    uint64_t theVal;

    ReadConstantAction(SemanticInstruction* anInstruction, int64_t aVal, eOperandCode anOperandCode)
      : BaseSemanticAction(anInstruction, 1)
      , theOperandCode(anOperandCode)
      , theVal(aVal)
    {
    }

    void doEvaluate()
    {
        DBG_(VVerb, (<< "Reading constant: 0x" << std::hex << theVal << std::dec << " into " << theOperandCode));
        theInstruction->setOperand(theOperandCode, theVal);

        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ReadConstant " << theVal << " to " << theOperandCode;
    }
};

simple_action
readConstantAction(SemanticInstruction* anInstruction, uint64_t aVal, eOperandCode anOperandCode)
{
    ReadConstantAction* act = new ReadConstantAction(anInstruction, aVal, anOperandCode);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

} // namespace nDecoder
