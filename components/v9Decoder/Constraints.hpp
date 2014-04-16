#ifndef FLEXUS_v9DECODER_CONSTRAINTS_HPP_INCLUDED
#define FLEXUS_v9DECODER_CONSTRAINTS_HPP_INCLUDED

#include <boost/tuple/tuple.hpp>

#include <components/Common/Slices/MemOp.hpp>
#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"

namespace nv9Decoder {

struct SemanticInstruction;

boost::function<bool()> membarStoreLoadConstraint( SemanticInstruction * anInstruction );
boost::function<bool()> membarStoreStoreConstraint( SemanticInstruction * anInstruction );
boost::function<bool()> membarSyncConstraint( SemanticInstruction * anInstruction );
boost::function<bool()> loadMemoryConstraint( SemanticInstruction * anInstruction );
boost::function<bool()> storeQueueAvailableConstraint( SemanticInstruction * anInstruction );
boost::function<bool()> storeQueueEmptyConstraint( SemanticInstruction * anInstruction );

boost::function<void()> saveWillRaiseCondition( SemanticInstruction * anInstruction );
boost::function<void()> restoreWillRaiseCondition( SemanticInstruction * anInstruction );
boost::function<bool()> sideEffectStoreConstraint( SemanticInstruction * anInstruction );

} //nv9Decoder

#endif //FLEXUS_v9DECODER_CONSTRAINTS_HPP_INCLUDED
