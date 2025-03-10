
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

struct OperandAction : public PredicatedSemanticAction
{
    eOperandCode theOperand;
    eOperandCode theResult;
    boost::optional<eOperandCode> theBypass;
    int theOffset;

    OperandAction(SemanticInstruction* anInstruction,
                  eOperandCode anOperand,
                  eOperandCode aResult,
                  int anOffset,
                  boost::optional<eOperandCode> aBypass)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theOperand(anOperand)
      , theResult(aResult)
      , theBypass(aBypass)
      , theOffset(anOffset)
    {
    }

    void doEvaluate()
    {
        DBG_(VVerb, (<< *this << " evaluated"));
        if (theBypass) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
            uint64_t value  = theInstruction->operand<uint64_t>(theOperand);
            theInstruction->setOperand(theResult, value);

            core()->bypass(name, value);
        }
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " store constant " << theOperand << " in " << theResult;
    }
};

predicated_action
operandAction(SemanticInstruction* anInstruction,
              eOperandCode anOperand,
              eOperandCode aResult,
              int anOffset,
              boost::optional<eOperandCode> aBypass)
{
    OperandAction* act = new OperandAction(anInstruction, anOperand, aResult, anOffset, aBypass);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
