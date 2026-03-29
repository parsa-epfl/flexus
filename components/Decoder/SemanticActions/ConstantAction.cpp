
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
#include "PredicatedSemanticAction.hpp"

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

struct ConstantAction : public PredicatedSemanticAction
{
    bits theConstant;
    eOperandCode theResult;
    boost::optional<eOperandCode> theBypass;

    ConstantAction(SemanticInstruction* anInstruction,
                   bits aConstant,
                   eOperandCode aResult,
                   boost::optional<eOperandCode> aBypass)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theConstant(aConstant)
      , theResult(aResult)
      , theBypass(aBypass)
    {
        setReady(0, true);
    }

    void doEvaluate()
    {
        theInstruction->setOperand(theResult, static_cast<bits>(theConstant));
        DBG_(VVerb, (<< *this << " applied"));
        if (theBypass) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
            core()->bypass(name, theConstant);
        }
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " store constant " << theConstant << " in " << theResult;
    }
};

predicated_action
constantAction(SemanticInstruction* anInstruction,
               uint64_t aConstant,
               eOperandCode aResult,
               boost::optional<eOperandCode> aBypass)
{
    ConstantAction* act = new ConstantAction(anInstruction, aConstant, aResult, aBypass);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
