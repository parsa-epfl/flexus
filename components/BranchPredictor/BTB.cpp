#include "BTB.hpp"

#include <cstdint>

BTB::BTB(int32_t aBTBSets, int32_t aBTBAssoc)
  : theBTBSets(aBTBSets)
  , theBTBAssoc(aBTBAssoc)
{
    // aBTBSize must be a power of 2
    DBG_Assert(((aBTBSets - 1) & (aBTBSets)) == 0);
    theBTB.resize(aBTBSets, BTBSet(aBTBAssoc));

    theIndexMask = aBTBSets - 1; // ! Shouldn't it be log2(aBTBSets) ?
}

/* The PC is assumed to be word aligned so the index into the Set Associative structure is */
/* INDEX_MASK anded with the PC >> 2 */
uint32_t
BTB::index(VirtualMemoryAddress anAddress)
{
    // Shift address by 2, since we assume word-aligned PCs
    return (anAddress >> 2) & theIndexMask;
}

// Whether the BTB contains target for anAddress
bool
BTB::contains(VirtualMemoryAddress anAddress)
{
    int32_t ind = index(anAddress);
    return theBTB[ind].isHit(anAddress);
}

// The kind of branch corresponding to the address: conditional, direct, indirect, return, etc
eBranchType
BTB::type(VirtualMemoryAddress anAddress)
{
    int32_t ind = index(anAddress);
    bool isHit  = theBTB[ind].isHit(anAddress);

    if (!isHit) {
        return kNonBranch;
    } else {
        return theBTB[ind].access(anAddress)->theBranchType;
    }
}

// Target address of the branch
boost::optional<VirtualMemoryAddress>
BTB::target(VirtualMemoryAddress anAddress)
{
    int32_t ind = index(anAddress);
    bool isHit  = theBTB[ind].isHit(anAddress);

    if (!isHit) {
        return boost::none;
    } else {
        return theBTB[ind].access(anAddress)->theTarget;
    }
}

// Update or add a new entry to the BTB
bool
BTB::update(VirtualMemoryAddress aPC, eBranchType aType, VirtualMemoryAddress aTarget)
{
    int32_t ind = index(aPC);
    bool isHit  = theBTB[ind].isHit(aPC);

    if (isHit) {
        if (aType == kNonBranch) {
            theBTB[ind].invalidate(aPC); // [MADHUR] Mispredict
        } else {
            BTBEntry* btbEntry = theBTB[ind].access(aPC); // [MADHUR] Access will also update the replacement queue

            btbEntry->theBranchType = aType;

            if (aTarget) {
                DBG_(Verb, (<< "BTB setting target for " << aPC << " to " << aTarget));

                btbEntry->theTarget = aTarget;
            }
        }

        return false; // not a new entry
    } else if (aType != kNonBranch) {
        DBG_(Verb, (<< "BTB adding new branch for " << aPC << " to " << aTarget));

        theBTB[ind].insert(BTBEntry(aPC, aType, aTarget)); // [MADHUR] Inserting a new entry

        return true;
    }
    return false; // not a new entry
}

bool
BTB::update(BranchFeedback const& aFeedback)
{
    return update(aFeedback.thePC, aFeedback.theActualType, aFeedback.theActualTarget);
}

json
BTB::saveState() const
{

    json checkpoint;

    for (size_t i = 0; i < theBTBSets; i++) {

        checkpoint.emplace_back(json::array());

        auto block = theBTB[i].blocks.begin();
        auto end   = theBTB[i].blocks.end();

        size_t j = 0;
        for (; block != end; block++, j++) {
            uint8_t type = 15;
            switch (block->theBranchType) {
                case kNonBranch: type = 0; break;
                case kConditional: type = 1; break;
                case kUnconditional: type = 2; break;
                case kCall: type = 3; break;
                case kIndirectReg: type = 4; break;
                case kIndirectCall: type = 5; break;
                case kReturn: type = 6; break;
                default: DBG_Assert(false, (<< "Don't know how to save branch type")); break;
            }
            if (block->valid) {
                checkpoint[i][j] = { { "PC", (uint64_t)block->thePC },
                                     { "target", (uint64_t)block->theTarget },
                                     { "type", (uint8_t)type } };
            }
        }
    }

    return checkpoint;
}

void
BTB::loadState(json checkpoint)
{

    for (size_t set = 0; set < (size_t)theBTBSets; set++) {

        size_t blockSize = checkpoint.at(set).size();

        theBTB[set].invalidateAll();

        for (size_t block = 0; block < blockSize; block++) {

            enum eBranchType type = kLastBranchType;
            uint64_t aPC          = checkpoint.at(set).at(block)["PC"];
            uint64_t aTarget      = checkpoint.at(set).at(block)["target"];
            uint8_t aType         = checkpoint.at(set).at(block)["type"];

            switch (aType) {
                case 0: type = kNonBranch; break;
                case 1: type = kConditional; break;
                case 2: type = kUnconditional; break;
                case 3: type = kCall; break;
                case 4: type = kIndirectReg; break;
                case 5: type = kIndirectCall; break;
                case 6: type = kReturn; break;
                default: DBG_Assert(false, (<< "Don't know how to load type: " << aType)); break;
            }

            theBTB[set].insert(BTBEntry(VirtualMemoryAddress(aPC), type, VirtualMemoryAddress(aTarget)));
        }
    }
};
