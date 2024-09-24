
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iomanip>
#include <iostream>
namespace ll = boost::lambda;

#include "../Conditions.hpp"
#include "../Effects.hpp"
#include "../Interactions.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "../Validations.hpp"

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

struct BranchCondAction : public BaseSemanticAction
{

    VirtualMemoryAddress theTarget;
    std::unique_ptr<Condition> theCondition;
    uint32_t theFeedbackCount;

    BranchCondAction(SemanticInstruction* anInstruction,
                     VirtualMemoryAddress aTarget,
                     std::unique_ptr<Condition>& aCondition,
                     size_t numOperands)
      : BaseSemanticAction(anInstruction, numOperands)
      , theTarget(aTarget)
      , theCondition(std::move(aCondition))
      , theFeedbackCount(0)
    {
        theInstruction->setExecuted(false);
    }

    void doEvaluate()
    {
        if (ready()) {
            if (theInstruction->hasPredecessorExecuted()) {

                std::vector<Operand> operands;
                for (int32_t i = 0; i < numOperands(); ++i) {
                    operands.push_back(op(eOperandCode(kOperand1 + i)));
                }

                if (theInstruction->hasOperand(kCondition)) { operands.push_back(theInstruction->operand(kCondition)); }

                boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
                feedback->thePC           = theInstruction->pc();
                feedback->theActualType   = kConditional;
                feedback->theActualTarget = theTarget;
                feedback->theBPState      = theInstruction->bpState();

                theCondition->setInstruction(theInstruction);

                bool result = theCondition->operator()(operands);

                if (result) {
                    // Taken
                    theInstruction->redirectPC(theTarget);
                    core()->applyToNext(theInstruction, branchInteraction(theTarget));
                    feedback->theActualDirection = kTaken;
                    DBG_(Iface, (<< "Branch taken! " << *theInstruction));
                } else {
                    theInstruction->redirectPC(theInstruction->pc() + 4);
                    core()->applyToNext(theInstruction, branchInteraction(theInstruction->pc() + 4));
                    feedback->theActualDirection = kNotTaken;
                    DBG_(Iface, (<< "Branch Not taken! " << *theInstruction));
                }
                theInstruction->setBranchFeedback(feedback);

                satisfyDependants();
                theInstruction->setExecuted(true);
            } else {
                DBG_(VVerb, (<< *this << " waiting for predecessor "));
                reschedule();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " BranchCC Action"; }

    Operand op(eOperandCode aCode) { return theInstruction->operand(aCode); }
};
dependant_action
branchCondAction(SemanticInstruction* anInstruction,
                 VirtualMemoryAddress aTarget,
                 std::unique_ptr<Condition> aCondition,
                 size_t numOperands)
{
    BranchCondAction* act = new BranchCondAction(anInstruction, aTarget, aCondition, numOperands);
    anInstruction->addNewComponent(act);

    return dependant_action(act, act->dependance());
}

struct BranchRegAction : public BaseSemanticAction
{

    VirtualMemoryAddress theTarget;
    eOperandCode theRegOperand;
    eBranchType theType;

    BranchRegAction(SemanticInstruction* anInstruction, eOperandCode aRegOperand, eBranchType aType)
      : BaseSemanticAction(anInstruction, 1)
      , theRegOperand(aRegOperand)
      , theType(aType)
    {
        theInstruction->setExecuted(false);
    }

    void doEvaluate()
    {
        if (ready()) {
            if (theInstruction->hasPredecessorExecuted()) {

                DBG_(Iface, (<< *this << " Branching to an address held in register " << theRegOperand));

                uint64_t target = boost::get<uint64_t>(theInstruction->operand<uint64_t>(kOperand1));

                theTarget = VirtualMemoryAddress(target);

                boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
                feedback->thePC           = theInstruction->pc();
                feedback->theActualType   = theType;
                feedback->theActualTarget = theTarget;
                feedback->theBPState      = theInstruction->bpState();
                theInstruction->setBranchFeedback(feedback);

                DBG_(
                  Iface,
                  (<< *this << " Checking for redirection PC= " << theInstruction->pc() << " target= " << theTarget));

                theInstruction->redirectPC(theTarget);
                core()->applyToNext(theInstruction, branchInteraction(theTarget));

                satisfyDependants();
                theInstruction->setExecuted(true);
            } else {
                DBG_(VVerb, (<< *this << " waiting for predecessor "));
                reschedule();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " BranchCC Action"; }
};

dependant_action
branchRegAction(SemanticInstruction* anInstruction, eOperandCode aRegOperand, eBranchType type)
{
    BranchRegAction* act = new BranchRegAction(anInstruction, aRegOperand, type);
    anInstruction->addNewComponent(act);
    return dependant_action(act, act->dependance());
}

struct BranchToCalcAddressAction : public BaseSemanticAction
{
    eOperandCode theTarget;
    uint32_t theFeedbackCount;

    BranchToCalcAddressAction(SemanticInstruction* anInstruction, eOperandCode aTarget)
      : BaseSemanticAction(anInstruction, 1)
      , theTarget(aTarget)
      , theFeedbackCount(0)
    {
        theInstruction->setExecuted(false);
    }

    void doEvaluate()
    {
        if (ready()) {
            if (theInstruction->hasPredecessorExecuted()) {

                // Feedback is taken care of by the updateUncoditional effect at
                // retirement
                uint64_t target = theInstruction->operand<uint64_t>(theTarget);
                VirtualMemoryAddress target_addr(target);
                DBG_(Iface, (<< *this << " branc to mapped_reg target: " << target_addr));

                theInstruction->redirectPC(target_addr);
                core()->applyToNext(theInstruction, branchInteraction(target_addr));

                satisfyDependants();
                theInstruction->setExecuted(true);
            } else {
                DBG_(VVerb, (<< *this << " waiting for predecessor "));
                reschedule();
            }
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " BranchToCalcAddress Action";
    }
};

dependant_action
branchToCalcAddressAction(SemanticInstruction* anInstruction)
{
    BranchToCalcAddressAction* act = new BranchToCalcAddressAction(anInstruction, kAddress);
    anInstruction->addNewComponent(act);

    return dependant_action(act, act->dependance());
}

} // namespace nDecoder