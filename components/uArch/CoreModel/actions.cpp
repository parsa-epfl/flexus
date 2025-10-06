
#include "coreModelImpl.hpp"

namespace nuArch {

void
CoreImpl::create(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    if (anAction->isREAD())
        anAction->connectBypass();

    if (anAction->isWB())
        theRescheduledWBActions.push(anAction);
    else if (anAction->isREAD())
        theRescheduledRDActions.push(anAction);
    else {
        DBG_(VVerb, (<< "Update Free EUs for " << *anAction));
        updateFreeEUs(anAction->getEU());
        theRescheduledActions[0].push(anAction);
    }
}
void
CoreImpl::reschedule(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    uint32_t idx = anAction->getExeStageIdx();
    if (idx >= numExeStages)
        idx = numExeStages - 1;
    if (anAction->isWB())
        theRescheduledWBActions.push(anAction);
    else if (anAction->isREAD())
        theRescheduledRDActions.push(anAction);
    else
        theRescheduledActions[idx].push(anAction);
}

void
CoreImpl::resetFreeEUs()
{
    if (!theInOrderExecute)
        return;
    theFreeALU = numALU;
    theFreeMUL = numMUL;
    theFreeAGU = numAGU;

    action_list_t tmp;
    while(!theRescheduledActions[0].empty()) {
        updateFreeEUs(theRescheduledActions[0].top()->getEU());
        tmp.push(theRescheduledActions[0].top());
        theRescheduledActions[0].pop();
    }
    std::swap(theRescheduledActions[0], tmp);
}

bool
ActionOrder::operator()(boost::intrusive_ptr<SemanticAction> const& l,
                        boost::intrusive_ptr<SemanticAction> const& r) const
{
    return l->instructionNo() > r->instructionNo();
}

} // namespace nuArch
