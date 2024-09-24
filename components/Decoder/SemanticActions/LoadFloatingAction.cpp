
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

struct LoadFloatingAction : public PredicatedSemanticAction
{
    eSize theSize;
    boost::optional<eOperandCode> theBypass0;
    boost::optional<eOperandCode> theBypass1;

    LoadFloatingAction(SemanticInstruction* anInstruction,
                       eSize aSize,
                       boost::optional<eOperandCode> const& aBypass0,
                       boost::optional<eOperandCode> const& aBypass1)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theSize(aSize)
      , theBypass0(aBypass0)
      , theBypass1(aBypass1)
    {
    }

    void satisfy(int32_t anArg)
    {
        BaseSemanticAction::satisfy(anArg);
        if (!cancelled() && ready() && thePredicate) {
            // Bypass
            doLoad();
        }
    }

    void predicate_on(int32_t anArg)
    {
        PredicatedSemanticAction::predicate_on(anArg);
        if (!cancelled() && ready() && thePredicate) { doLoad(); }
    }

    void doEvaluate() {}

    void doLoad()
    {
        if (ready() && thePredicate) {
            switch (theSize) {
                case kWord: {
                    bits value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
                    theInstruction->setOperand(kfResult0, value & bits(0xFFFFFFFFULL));
                    if (theBypass0) {
                        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
                        core()->bypass(name, value & bits(0xFFFFFFFFULL));
                    }

                    break;
                }
                case kDoubleWord: {
                    bits value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
                    theInstruction->setOperand(kfResult1, value & bits(0xFFFFFFFFULL));
                    theInstruction->setOperand(kfResult0, value >> 32);
                    if (theBypass1) {
                        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass1);
                        core()->bypass(name, value & bits(0xFFFFFFFFULL));
                    }
                    if (theBypass0) {
                        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
                        core()->bypass(name, value >> 32);
                    }

                    break;
                }
                default: DBG_Assert(false);
            }

            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " LoadFloatingAction"; }
};

predicated_dependant_action
loadFloatingAction(SemanticInstruction* anInstruction,
                   eSize aSize,
                   boost::optional<eOperandCode> aBypass0,
                   boost::optional<eOperandCode> aBypass1)
{
    LoadFloatingAction* act = new LoadFloatingAction(anInstruction, aSize, aBypass0, aBypass1);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}

} // namespace nDecoder
