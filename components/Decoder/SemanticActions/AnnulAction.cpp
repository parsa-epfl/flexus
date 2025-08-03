
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

struct AnnulAction : public PredicatedSemanticAction
{
    eOperandCode theRegisterCode;
    eOperandCode theOutputCode;
    bool theMOVConnected;
    bool theTriggered;

    AnnulAction(SemanticInstruction* anInstruction, eOperandCode aRegisterCode, eOperandCode anOutputCode)
      : PredicatedSemanticAction(anInstruction, 1, false)
      , theRegisterCode(aRegisterCode)
      , theOutputCode(anOutputCode)
      , theMOVConnected(false)
    {
    }

    void doEvaluate()
    {
        if (thePredicate) {
            mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
            eResourceStatus status;
            if (!theMOVConnected) {
                status = core()->requestRegister(name, theInstruction->makeInstructionDependance(dependance(0)));
                // theInstruction->addRetirementEffect( disconnectRegister(
                // theInstruction, theRegisterCode ) ); theInstruction->addSquashEffect(
                // disconnectRegister( theInstruction, theRegisterCode ) );
                theMOVConnected = true;
            } else {
                status = core()->requestRegister(name);
                DBG_(VVerb,
                     (<< *this << " scheduled. " << theRegisterCode << "(" << name << ")"
                      << " is " << status));
            }
            if (kReady == status) { setReady(0, true); }
            if (ready()) {
                Operand aValue = core()->readRegister(name);
                theInstruction->setOperand(theOutputCode, aValue);
                DBG_(VVerb,
                     (<< *this << " read " << theRegisterCode << "(" << name << ") = " << aValue << " written to "
                      << theOutputCode));
                theInstruction->setExecuted(true);
                satisfyDependants();
            }
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " Annul " << theRegisterCode << " store in " << theOutputCode;
    }
};

predicated_action
annulAction(SemanticInstruction* anInstruction)
{
    AnnulAction* act = new AnnulAction(anInstruction, kPPD, kResult);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

predicated_action
annulRD1Action(SemanticInstruction* anInstruction)
{
    AnnulAction* act = new AnnulAction(anInstruction, kPPD1, kResult1);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

predicated_action
annulXTRA(SemanticInstruction* anInstruction)
{
    AnnulAction* act = new AnnulAction(anInstruction, kXTRAppd, kXTRAout);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

predicated_action
floatingAnnulAction(SemanticInstruction* anInstruction, int32_t anIndex)
{
    AnnulAction* act =
      new AnnulAction(anInstruction, eOperandCode(kPPFD0 + anIndex), eOperandCode(kfResult0 + anIndex));
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
