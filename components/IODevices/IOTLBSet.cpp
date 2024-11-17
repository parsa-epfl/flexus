#include "IOTLBSet.hpp"

#include "core/types.hpp"

using namespace Flexus::SharedTypes;

IOTLBSet::IOTLBSet()
  : blocks(64)
  , replacementQueue(64, 0)
{
}

IOTLBSet::IOTLBSet(uint32_t associativity)
  : blocks(associativity, IOTLBEntry())
  , replacementQueue(associativity)
{
    for (uint32_t i = 0; i < associativity; ++i) {
        replacementQueue[i] = i;
    }
}

void
IOTLBSet::updateReplacementQueue(uint32_t index)
{
    replacementQueue.erase(std::remove(replacementQueue.begin(), replacementQueue.end(), index),
                           replacementQueue.end());
    replacementQueue.insert(replacementQueue.end(), index);
}

bool
IOTLBSet::isHit(uint16_t ASID, VirtualMemoryAddress anIOVAPFN)
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].theASID == ASID &&
         blocks[i].theIOVAPFN == anIOVAPFN && blocks[i].valid) { return true; }
    }
    return false;
}

// [MADHUR] Return the Hit IOTLB entry
IOTLBEntry*
IOTLBSet::access(uint16_t ASID, VirtualMemoryAddress anIOVAPFN)
{
    for (uint32_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].theASID == ASID && blocks[i].theIOVAPFN == anIOVAPFN && blocks[i].valid) {
            updateReplacementQueue(i);
            return &blocks[i];
        }
    }

    return NULL;
}

// [MADHUR] Insert a new IOTLB Entry
void
IOTLBSet::insert(IOTLBEntry iotlbEntry)
{
    // TODO: Replacement is not needed if there is an invalid entry
    uint32_t replacementQueueIndex = replacementQueue[0];
    blocks[replacementQueueIndex]  = iotlbEntry;
    updateReplacementQueue(replacementQueueIndex);
}

// [MADHUR] Invalidate entry if present
void
IOTLBSet::invalidate(uint16_t ASID, VirtualMemoryAddress anIOVAPFN)
{
    for (auto& block : blocks) {
        if (block.theASID == ASID 
        && block.theIOVAPFN == anIOVAPFN && block.valid) block.valid = false;
    }
}

// [MADHUR] Invalidate all entries of a device
void
IOTLBSet::invalidate(uint16_t ASID)
{
    for (auto& block : blocks) {
        if (block.theASID == ASID) block.valid = false;
    }
}

// [MADHUR] Invalidate all the entries
void
IOTLBSet::invalidate()
{
    for (auto& block : blocks) {
        block.valid = false;
    }
}

void 
IOTLBSet::printValidEntries() {
    for (auto& block : blocks) {
        if (block.valid) {
            block.print();
        }
    }
}