#ifndef FLEXUS_BTB_SET
#define FLEXUS_BTB_SET

#include "BTBEntry.hpp"

#include <vector>

/* [MADHUR]
 * replacementQueue implements replacement policy for each set
 * Head of the queue (replacementQueue[0]) is supposed to be replaced
 * after modification or lookup of the set, the replacement queue needs
 * to be updated to update the life time of each entry
 */
class BTBSet
{
  public:
    std::vector<BTBEntry> blocks; // Array of blocks make up a set
  private:
    std::vector<uint32_t> replacementQueue;
    void updateReplacementQueue(uint32_t index); // [MADHUR] Right now, it is LRU
                                                 //
  public:
    BTBSet();

    BTBSet(uint32_t associativity);

    // [MADHUR] Whether BTB entry corresponding to anAddress is present
    bool isHit(VirtualMemoryAddress anAddress);

    // [MADHUR] Return the Hit BTB entry
    BTBEntry* access(VirtualMemoryAddress anAddress);

    // [MADHUR] Insert a new BTB Entry
    void insert(BTBEntry btbEntry);

    // [MADHUR] Invalidate entry if present
    void invalidate(VirtualMemoryAddress anAddress);

    void invalidateAll();
};

#endif