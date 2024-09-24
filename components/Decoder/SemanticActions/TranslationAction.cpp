
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

struct TranslationAction : public BaseSemanticAction
{

    TranslationAction(SemanticInstruction* anInstruction)
      : BaseSemanticAction(anInstruction, 1)
    {
    }

    void squash(int32_t anOperand)
    {
        if (!cancelled()) {
            DBG_(VVerb, (<< *this << " Squashing paddr."));
            core()->resolvePAddr(boost::intrusive_ptr<Instruction>(theInstruction), (PhysicalMemoryAddress)kUnresolved);
        }
        boost::intrusive_ptr<Instruction>(theInstruction)->setResolved(false);
        BaseSemanticAction::squash(anOperand);
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (ready()) {
            DBG_Assert(theInstruction->hasOperand(kAddress));
            VirtualMemoryAddress addr(theInstruction->operand<uint64_t>(kAddress));

            theInstruction->core()->translate(boost::intrusive_ptr<Instruction>(theInstruction));

            satisfyDependants();
        } else {
            reschedule();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " TranslationAction"; }
};

simple_action
translationAction(SemanticInstruction* anInstruction)
{
    TranslationAction* act = new TranslationAction(anInstruction);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

} // namespace nDecoder
