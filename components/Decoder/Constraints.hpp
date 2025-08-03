
#ifndef FLEXUS_DECODER_CONSTRAINTS_HPP_INCLUDED
#define FLEXUS_DECODER_CONSTRAINTS_HPP_INCLUDED

#include <functional>

namespace nDecoder {

struct SemanticInstruction;

std::function<bool()>
membarStoreLoadConstraint(SemanticInstruction* anInstruction);
std::function<bool()>
membarStoreStoreConstraint(SemanticInstruction* anInstruction);
std::function<bool()>
membarSyncConstraint(SemanticInstruction* anInstruction);
std::function<bool()>
loadMemoryConstraint(SemanticInstruction* anInstruction);
std::function<bool()>
storeQueueAvailableConstraint(SemanticInstruction* anInstruction);
std::function<bool()>
storeQueueEmptyConstraint(SemanticInstruction* anInstruction);

std::function<void()>
saveWillRaiseCondition(SemanticInstruction* anInstruction);
std::function<void()>
restoreWillRaiseCondition(SemanticInstruction* anInstruction);
std::function<bool()>
sideEffectStoreConstraint(SemanticInstruction* anInstruction);
std::function<bool()>
paddrResolutionConstraint(SemanticInstruction* anInstruction);

} // namespace nDecoder

#endif // FLEXUS_DECODER_CONSTRAINTS_HPP_INCLUDED
