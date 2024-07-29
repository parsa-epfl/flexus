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
{     replacementQueue.erase(std::remove(replacementQueue.begin(), replacementQueue.end(), index),
                           replacementQueue.end());
    replacementQueue.insert(replacementQueue.end(), index);
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
    // TODO: Replacement is not needed if there is an invalid entry
    uint32_t replacementQueueIndex = replacementQueue[0];
    blocks[replacementQueueIndex]  = btbEntry;
    updateReplacementQueue(replacementQueueIndex);
}

// [MADHUR] Invalidate entry if present
void
BTBSet::invalidate(VirtualMemoryAddress anAddress)
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].thePC == anAddress && blocks[i].valid) {
            // Entry to be invalidated is present
            blocks[i].valid = false; // Invalidate
            // TODO: Update replacementQueue for invalidation
        }
    }
}

void
BTBSet::invalidateAll()
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        blocks[i].valid = false;
    }
}