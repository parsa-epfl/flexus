
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

struct BitFieldAction : public PredicatedSemanticAction
{
    eOperandCode theOperandCode1, theOperandCode2;
    uint64_t theS, theR;
    uint64_t thewmask, thetmask;
    bool theExtend, the64;

    BitFieldAction(SemanticInstruction* anInstruction,
                   eOperandCode anOperandCode1,
                   eOperandCode anOperandCode2,
                   uint64_t imms,
                   uint64_t immr,
                   uint64_t wmask,
                   uint64_t tmask,
                   bool anExtend,
                   bool a64)
      : PredicatedSemanticAction(anInstruction, 2, true)
      , theOperandCode1(anOperandCode1)
      , theOperandCode2(anOperandCode2)
      , theS(imms)
      , theR(immr)
      , thewmask(wmask)
      , thetmask(tmask)
      , theExtend(anExtend)
      , the64(a64)
    {
        theInstruction->setExecuted(false);
    }

    void doEvaluate()
    {

        if (ready()) {
            if (theInstruction->hasPredecessorExecuted()) {

                uint64_t src = boost::get<uint64_t>(theInstruction->operand(theOperandCode1));
                uint64_t dst = boost::get<uint64_t>(theInstruction->operand(theOperandCode2));

                std::unique_ptr<Operation> ror = operation(kROR_);
                std::vector<Operand> operands  = { src, theR, (the64 ? (uint64_t)64 : (uint64_t)32) };
                uint64_t res                   = boost::get<uint64_t>(ror->operator()(operands));

                // perform bitfield move on low bits
                uint64_t bot = (dst & ~thewmask) | (res & thewmask);
                // determine extension bits (sign, zero or dest register)
                uint64_t top_mask = the64 ? (uint64_t)-1 : 0xFFFFFFFF;
                uint64_t top      = theExtend ? ((src & ((uint64_t)1 << theS)) ? top_mask : 0) : dst;
                // combine extension bits and result bits
                uint64_t result = ((top & ~thetmask) | (bot & thetmask));
                theInstruction->setOperand(kResult, result);

                satisfyDependants();
                theInstruction->setExecuted(true);
            } else {
                DBG_(VVerb, (<< *this << " waiting for predecessor "));
                reschedule();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " BitFieldAction "; }
};

predicated_action
bitFieldAction(SemanticInstruction* anInstruction,
               std::vector<std::list<InternalDependance>>& opDeps,
               eOperandCode anOperandCode1,
               eOperandCode anOperandCode2,
               uint64_t imms,
               uint64_t immr,
               uint64_t wmask,
               uint64_t tmask,
               bool anExtend,
               bool a64)
{
    BitFieldAction* act =
      new BitFieldAction(anInstruction, anOperandCode1, anOperandCode2, imms, immr, wmask, tmask, anExtend, a64);
    anInstruction->addNewComponent(act);

    for (uint32_t i = 0; i < opDeps.size(); ++i) {
        opDeps[i].push_back(act->dependance(i));
    }

    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
