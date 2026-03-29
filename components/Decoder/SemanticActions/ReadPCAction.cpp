
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

static const bits kAM = 0x8;

struct ReadPCAction : public PredicatedSemanticAction
{
    eOperandCode theResult;

    ReadPCAction(SemanticInstruction* anInstruction, eOperandCode aResult)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theResult(aResult)
    {
        setReady(0, true);
    }

    void doEvaluate()
    {
        //    bits pstate = theInstruction->core()->getPSTATE() ;
        //    if (pstate & kAM ) {
        //      //Need to mask upper 32 bits when AM is set
        //      theInstruction->setOperand(theResult,
        //      static_cast<bits>(theInstruction->pc()) & 0xFFFFFFFFULL);

        //    } else {
        uint64_t pc = theInstruction->pc();
        theInstruction->setOperand(theResult, (bits)pc);
        //    }
        DBG_(VVerb, (<< *this << " read PC"));
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " Read PC store in " << theResult;
    }
};

predicated_action
readPCAction(SemanticInstruction* anInstruction)
{
    ReadPCAction* act = new ReadPCAction(anInstruction, kResult);
    anInstruction->addNewComponent(act);

    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
