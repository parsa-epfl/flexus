
#include "Constraints.hpp"

#include "SemanticInstruction.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using nuArch::kSC;
using nuArch::kTSO;

bool
checkStoreQueueAvailable(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    if (anInstruction->core()->sbFull()) { return false; }
    return true;
}

std::function<bool()>
storeQueueAvailableConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkStoreQueueAvailable(anInstruction);
    };
}

bool
checkMembarStoreStoreConstraint(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    return anInstruction->core()->mayRetire_MEMBARStSt();
}

std::function<bool()>
membarStoreStoreConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkMembarStoreStoreConstraint(anInstruction);
    };
}

bool
checkMembarStoreLoadConstraint(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    return anInstruction->core()->mayRetire_MEMBARStLd();
}

std::function<bool()>
membarStoreLoadConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkMembarStoreLoadConstraint(anInstruction);
    };
}

bool
checkMembarSyncConstraint(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    return anInstruction->core()->mayRetire_MEMBARSync();
}

std::function<bool()>
membarSyncConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkMembarSyncConstraint(anInstruction);
    };
}

bool
checkMemoryConstraint(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    switch (anInstruction->core()->consistencyModel()) {
        case kSC:
            if (!anInstruction->core()->speculativeConsistency()) {
                // Under nonspeculative SC, a load instruction may only retire when no
                // stores are outstanding.
                if (!anInstruction->core()->sbEmpty()) { return false; }
            }
            break;
        case kTSO:
        case kRMO:
            // Under TSO and RMO, a load may always retire when it reaches the
            // head of the re-order buffer.
            break;
        default:
            DBG_Assert(false,
                       (<< "Load Memory Instruction does not support consistency model "
                        << anInstruction->core()->consistencyModel()));
    }
    return true;
}

std::function<bool()>
loadMemoryConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkMemoryConstraint(anInstruction);
    };
}

bool
checkStoreQueueEmpty(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    return anInstruction->core()->sbEmpty();
}

std::function<bool()>
storeQueueEmptyConstraint(SemanticInstruction* anInstruction)
{
    return []() {
        return checkStoreQueueEmpty;
    };
}

bool
checkSideEffectStoreConstraint(SemanticInstruction* anInstruction)
{
    if (!anInstruction->core()) { return false; }
    return anInstruction->core()->checkStoreRetirement(boost::intrusive_ptr<nuArch::Instruction>(anInstruction));
}

std::function<bool()>
sideEffectStoreConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkSideEffectStoreConstraint(anInstruction);
    };
}

bool
checkpaddrResolutionConstraint(SemanticInstruction* anInstruction)
{
    return anInstruction->isResolved();
}
std::function<bool()>
paddrResolutionConstraint(SemanticInstruction* anInstruction)
{
    return [anInstruction]() {
        return checkpaddrResolutionConstraint(anInstruction);
    };
}

} // namespace nDecoder
