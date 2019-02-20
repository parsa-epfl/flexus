#include "coreModelImpl.hpp"

namespace nuArch {

void CoreImpl::create( boost::intrusive_ptr< SemanticAction > anAction) {
  theRescheduledActions.push( anAction );
}
void CoreImpl::reschedule( boost::intrusive_ptr< SemanticAction > anAction) {
  theRescheduledActions.push( anAction );
}

bool ActionOrder::operator()(boost::intrusive_ptr< SemanticAction > const & l, boost::intrusive_ptr< SemanticAction > const & r) const {
  return l->instructionNo() > r->instructionNo();
}

} //nuArch
