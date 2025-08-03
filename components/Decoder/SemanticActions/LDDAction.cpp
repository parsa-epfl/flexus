
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

struct LDDAction : public PredicatedSemanticAction
{
    boost::optional<eOperandCode> theBypass0;
    boost::optional<eOperandCode> theBypass1;

    LDDAction(SemanticInstruction* anInstruction,
              boost::optional<eOperandCode> aBypass0,
              boost::optional<eOperandCode> aBypass1)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theBypass0(aBypass0)
      , theBypass1(aBypass1)
    {
    }

    void satisfy(int32_t anArg)
    {
        BaseSemanticAction::satisfy(anArg);
        SEMANTICS_DBG(*theInstruction << *this);
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
        SEMANTICS_DBG(*this);
        //    int32_t asi = theInstruction->operand< bits > (kOperand3);
        //    if (asi == 0x24 || asi == 0x34) {
        //      //Quad LDD
        //      bits value = core()->retrieveLoadValue(
        //      boost::intrusive_ptr<Instruction>(theInstruction) ); bits value1 =
        //      core()->retrieveExtendedLoadValue(
        //      boost::intrusive_ptr<Instruction>(theInstruction) );
        //      theInstruction->setOperand(kResult, value );
        //      theInstruction->setOperand(kResult1, value1 );

        //      if (theBypass0) {
        //        mapped_reg name = theInstruction->operand< mapped_reg >
        //        (*theBypass0); core()->bypass( name, value );
        //      }
        //      if (theBypass1) {
        //        mapped_reg name = theInstruction->operand< mapped_reg >
        //        (*theBypass1); core()->bypass( name, value1 );
        //      }
        //    } else {
        // Normal LDD
        bits value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
        theInstruction->setOperand(kResult, value >> 32);
        theInstruction->setOperand(kResult1, value & bits(0xFFFFFFFFULL));

        SEMANTICS_DBG(*this << " received load value=" << value);

        if (theBypass0) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
            core()->bypass(name, value >> 32);
        }
        if (theBypass1) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass1);
            core()->bypass(name, value & bits(0xFFFFFFFFULL));
        }
        //    }
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " LDDAction"; }
};

predicated_dependant_action
lddAction(SemanticInstruction* anInstruction,
          boost::optional<eOperandCode> aBypass0,
          boost::optional<eOperandCode> aBypass1)
{
    LDDAction* act = new LDDAction(anInstruction, aBypass0, aBypass1);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}

} // namespace nDecoder
