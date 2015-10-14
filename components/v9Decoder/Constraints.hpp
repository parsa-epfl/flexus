#ifndef FLEXUS_v9DECODER_CONSTRAINTS_HPP_INCLUDED
#define FLEXUS_v9DECODER_CONSTRAINTS_HPP_INCLUDED

#include <components/Common/Slices/MemOp.hpp>
#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"

namespace nv9Decoder {

struct SemanticInstruction;

std::function<bool()> membarStoreLoadConstraint( SemanticInstruction * anInstruction );
std::function<bool()> membarStoreStoreConstraint( SemanticInstruction * anInstruction );
std::function<bool()> membarSyncConstraint( SemanticInstruction * anInstruction );
std::function<bool()> loadMemoryConstraint( SemanticInstruction * anInstruction );
std::function<bool()> storeQueueAvailableConstraint( SemanticInstruction * anInstruction );
std::function<bool()> storeQueueEmptyConstraint( SemanticInstruction * anInstruction );

std::function<void()> saveWillRaiseCondition( SemanticInstruction * anInstruction );
std::function<void()> restoreWillRaiseCondition( SemanticInstruction * anInstruction );
std::function<bool()> sideEffectStoreConstraint( SemanticInstruction * anInstruction );

} //nv9Decoder

#endif //FLEXUS_v9DECODER_CONSTRAINTS_HPP_INCLUDED
