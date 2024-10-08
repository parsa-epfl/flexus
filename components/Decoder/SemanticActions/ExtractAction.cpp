
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
#include "RegisterValueExtractor.hpp"

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

struct ExtractAction : public PredicatedSemanticAction
{
    eOperandCode theOperandCode1, theOperandCode2, theOperandCode3;
    bool the64;

    ExtractAction(SemanticInstruction* anInstruction,
                  eOperandCode anOperandCode1,
                  eOperandCode anOperandCode2,
                  eOperandCode anOperandCode3,
                  bool a64)
      : PredicatedSemanticAction(anInstruction, 3, true)
      , theOperandCode1(anOperandCode1)
      , theOperandCode2(anOperandCode2)
      , theOperandCode3(anOperandCode3)
      , the64(a64)
    {
        theInstruction->setExecuted(false);
    }

    void doEvaluate()
    {

        if (ready()) {
            if (theInstruction->hasPredecessorExecuted()) {
                uint64_t src  = boost::get<uint64_t>(theInstruction->operand(theOperandCode1));
                uint64_t src2 = boost::get<uint64_t>(theInstruction->operand(theOperandCode2));
                uint64_t imm  = (uint64_t)boost::get<uint64_t>(theInstruction->operand(theOperandCode3));

                std::unique_ptr<Operation> op = operation(the64 ? kCONCAT64_ : kCONCAT32_);
                std::vector<Operand> operands = { src, src2 };
                bits res                      = boost::get<bits>(op->operator()(operands));
                res >>= imm;
                theInstruction->setOperand(kResult, (uint64_t)res);
                satisfyDependants();
                theInstruction->setExecuted(true);
            } else {
                DBG_(VVerb, (<< *this << " waiting for predecessor "));
                reschedule();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " ExtractAction "; }
};

predicated_action
extractAction(SemanticInstruction* anInstruction,
              std::vector<std::list<InternalDependance>>& opDeps,
              eOperandCode anOperandCode1,
              eOperandCode anOperandCode2,
              eOperandCode anOperandCode3,
              bool a64)
{
    ExtractAction* act = new ExtractAction(anInstruction, anOperandCode1, anOperandCode2, anOperandCode3, a64);
    anInstruction->addNewComponent(act);

    for (uint32_t i = 0; i < opDeps.size(); ++i) {
        opDeps[i].push_back(act->dependance(i));
    }

    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
