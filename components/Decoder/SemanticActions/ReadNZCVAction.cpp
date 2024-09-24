
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
#include <components/uArch/systemRegister.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

struct ReadNZCVAction : public BaseSemanticAction
{
    eOperandCode theOperandCode;
    eNZCV theBit;

    ReadNZCVAction(SemanticInstruction* anInstruction, eNZCV aBit, eOperandCode anOperandCode)
      : BaseSemanticAction(anInstruction, 1)
      , theOperandCode(anOperandCode)
      , theBit(aBit)
    {
    }

    void doEvaluate()
    {

        SEMANTICS_DBG(*this);
        bits nzcv_bit;

        switch (theBit) {
            case kN: nzcv_bit = theInstruction->core()->_PSTATE().N(); break;
            case kZ: nzcv_bit = theInstruction->core()->_PSTATE().Z(); break;
            case kC: nzcv_bit = theInstruction->core()->_PSTATE().C(); break;
            case kV: nzcv_bit = theInstruction->core()->_PSTATE().V(); break;
            default: DBG_Assert(false);
        }

        theInstruction->setOperand(theOperandCode, nzcv_bit);
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " ReadNZCVAction "; }
};

simple_action
readNZCVAction(SemanticInstruction* anInstruction, eNZCV aBit, eOperandCode anOperandCode)
{
    ReadNZCVAction* act = new ReadNZCVAction(anInstruction, aBit, anOperandCode);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

} // namespace nDecoder
