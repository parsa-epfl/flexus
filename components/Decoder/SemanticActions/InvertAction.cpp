
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

struct InvertAction : public PredicatedSemanticAction
{
    eOperandCode theRegisterCode;
    bool the64;

    InvertAction(SemanticInstruction* anInstruction, eOperandCode aRegisterCode, bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theRegisterCode(aRegisterCode)
      , the64(is64)
    {
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (!signalled()) {
            SEMANTICS_DBG("Signalling");

            mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
            Operand aValue  = core()->readRegister(name);

            bits val = boost::get<bits>(aValue);
            val      = ~val;

            if (!the64) { val &= 0xffffffff; }

            theInstruction->setOperand(theRegisterCode, val);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " InvertRegisterAction " << theRegisterCode;
    }
};

predicated_action
invertAction(SemanticInstruction* anInstruction, eOperandCode aRegisterCode, bool is64)
{

    InvertAction* act = new InvertAction(anInstruction, aRegisterCode, is64);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
