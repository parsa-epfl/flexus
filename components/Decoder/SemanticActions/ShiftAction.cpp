
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

struct ShiftRegisterAction : public PredicatedSemanticAction
{
    eOperandCode theRegisterCode;
    bool the64;
    std::unique_ptr<Operation> theShiftOperation;
    uint64_t theShiftAmount;

    ShiftRegisterAction(SemanticInstruction* anInstruction,
                        eOperandCode aRegisterCode,
                        std::unique_ptr<Operation>& aShiftOperation,
                        uint64_t aShiftAmount,
                        bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theRegisterCode(aRegisterCode)
      , the64(is64)
      , theShiftOperation(std::move(aShiftOperation))
      , theShiftAmount(aShiftAmount)
    {
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (ready()) {
            Operand aValue = theInstruction->operand(theRegisterCode);

            aValue = theShiftOperation->operator()({ aValue, theShiftAmount });

            uint64_t val = boost::get<uint64_t>(aValue);

            if (!the64) { val &= 0xffffffff; }

            theInstruction->setOperand(theRegisterCode, val);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ShiftRegisterAction " << theRegisterCode;
    }
};

predicated_action
shiftAction(SemanticInstruction* anInstruction,
            eOperandCode aRegisterCode,
            std::unique_ptr<Operation>& aShiftOp,
            uint64_t aShiftAmount,
            bool is64)
{
    ShiftRegisterAction* act = new ShiftRegisterAction(anInstruction, aRegisterCode, aShiftOp, aShiftAmount, is64);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
