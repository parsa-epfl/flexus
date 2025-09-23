
#include "coreModelImpl.hpp"

namespace nuArch {

void
CoreImpl::create(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    if (anAction->isREAD())
        anAction->connectBypass();

    if (theInOrderExecute) {
        if (anAction->isWB())
            theRescheduledWBActions.push(anAction);
        else if (anAction->isREAD())
            theRescheduledRDActions.push(anAction);
        else
            theRescheduledActions.push(anAction);
    }
    else
        theRescheduledActions.push(anAction);
}
void
CoreImpl::reschedule(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    if (theInOrderExecute) {
        if (anAction->isWB())
            theRescheduledWBActions.push(anAction);
        else if (anAction->isREAD())
            theRescheduledRDActions.push(anAction);
        else
            theRescheduledActions.push(anAction);
    }
    else
        theRescheduledActions.push(anAction);
}

bool
ActionOrder::operator()(boost::intrusive_ptr<SemanticAction> const& l,
                        boost::intrusive_ptr<SemanticAction> const& r) const
{
    return l->instructionNo() > r->instructionNo();
}

} // namespace nuArch
