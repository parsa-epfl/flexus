#include "BTBSet.hpp"

#include "core/types.hpp"

#include <cstdint>

using namespace Flexus::SharedTypes;

BTBSet::BTBSet()
  : blocks(64)
  , replacementQueue(64, 0)
{
}

BTBSet::BTBSet(uint32_t associativity)
  : blocks(associativity, BTBEntry())
  , replacementQueue(associativity)
{
    for (uint32_t i = 0; i < associativity; ++i) {
        replacementQueue[i] = i;
    }
}

void
BTBSet::updateReplacementQueue(uint32_t index)
{
    for (auto i = replacementQueue.begin(); i != replacementQueue.end(); ++i)
        if (*i == index) {
            replacementQueue.erase(i);
            break;
        }

    replacementQueue.push_back(index);
}

bool
BTBSet::isHit(VirtualMemoryAddress anAddress)
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].thePC == anAddress && blocks[i].valid) { return true; }
    }
    return false;
}

// [MADHUR] Return the Hit BTB entry
BTBEntry*
BTBSet::access(VirtualMemoryAddress anAddress)
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].thePC == anAddress && blocks[i].valid) {
            updateReplacementQueue(i);
            return &blocks[i];
        }
    }

    return NULL;
}

// [MADHUR] Insert a new BTB Entry
void
BTBSet::insert(BTBEntry btbEntry)
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].valid) {
            blocks[i] = btbEntry;
            updateReplacementQueue(i);
            return;
        }
    }

    uint32_t index = replacementQueue.front();
    blocks[index] = btbEntry;
    updateReplacementQueue(index);
}

// [MADHUR] Invalidate entry if present
void
BTBSet::invalidate(VirtualMemoryAddress anAddress)
{
    for (auto& block : blocks) {
        if (block.thePC == anAddress && block.valid) block.valid = false;
    }
}

void
BTBSet::invalidateAll()
{

    for (auto& block : blocks) {
        block.valid = false;
    }
}
