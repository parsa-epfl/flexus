
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

struct ExtendAction : public PredicatedSemanticAction
{
    eOperandCode theRegisterCode;
    bool the64;
    std::unique_ptr<Operation> theExtendOperation;

    ExtendAction(SemanticInstruction* anInstruction,
                 eOperandCode aRegisterCode,
                 std::unique_ptr<Operation>& anExtendOperation,
                 bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theRegisterCode(aRegisterCode)
      , the64(is64)
      , theExtendOperation(std::move(anExtendOperation))
    {
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (!signalled()) {
            SEMANTICS_DBG("Signalling");

            Operand aValue = theInstruction->operand(theRegisterCode);

            aValue = theExtendOperation->operator()({ aValue });

            bits val = boost::get<bits>(aValue);

            if (!the64) { val &= 0xffffffff; }

            theInstruction->setOperand(theRegisterCode, val);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ExtendRegisterAction " << theRegisterCode;
    }
};

predicated_action
extendAction(SemanticInstruction* anInstruction,
             eOperandCode aRegisterCode,
             std::unique_ptr<Operation>& anExtendOp,
             std::vector<std::list<InternalDependance>>& opDeps,
             bool is64)
{

    ExtendAction* act = new ExtendAction(anInstruction, aRegisterCode, anExtendOp, is64);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < opDeps.size(); ++i) {
        opDeps[i].push_back(act->dependance(i));
    }
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
