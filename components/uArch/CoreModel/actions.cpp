
#include "coreModelImpl.hpp"

namespace nuArch {

void
CoreImpl::create(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    if (anAction->isWB() && theInOrderExecute)
        theRescheduledWBActions.push(anAction);
    else
        theRescheduledActions.push(anAction);
}
void
CoreImpl::reschedule(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    if (anAction->isWB() && theInOrderExecute)
        theRescheduledWBActions.push(anAction);
    else
        theRescheduledActions.push(anAction);
}
void
CoreImpl::recreate(boost::intrusive_ptr<SemanticAction> anAction)
{
    CORE_DBG(*anAction);
    theActiveActions.push(anAction);
}

bool
ActionOrder::operator()(boost::intrusive_ptr<SemanticAction> const& l,
                        boost::intrusive_ptr<SemanticAction> const& r) const
{
    return l->instructionNo() > r->instructionNo();
}

} // namespace nuArch
