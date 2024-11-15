#ifndef FLEXUS_IOTLB_SET
#define FLEXUS_IOTLB_SET

#include "IOTLBEntry.hpp"

#include <vector>

/* [MADHUR]
 * replacementQueue implements replacement policy for each set
 * Head of the queue (replacementQueue[0]) is supposed to be replaced
 * after modification or lookup of the set, the replacement queue needs
 * to be updated to update the life time of each entry
 */
class IOTLBSet
{
public:
	std::vector<IOTLBEntry> blocks; // Array of blocks make up a set

private:
	std::vector<uint32_t> replacementQueue;
	void updateReplacementQueue(uint32_t index); // [MADHUR] Right now, it is LRU

public:

	IOTLBSet();
	IOTLBSet(uint32_t associativity);

	// [MADHUR] Whether IOTLB entry corresponding to anIOVAPFN is present
	bool isHit(uint16_t BDF, VirtualMemoryAddress anIOVAPFN);

	// [MADHUR] Return the Hit IOTLB entry
	IOTLBEntry * access(uint16_t BDF, VirtualMemoryAddress anIOVAPFN);

	// [MADHUR] Insert a new IOTLB Entry
	void insert(IOTLBEntry iotlbEntry);

	// [MADHUR] Invalidate entry if present
	void invalidate(uint16_t BDF, VirtualMemoryAddress anIOVAPFN);
	void invalidate(uint16_t BDF);	// Invalidate the entries belonging to a device
	void invalidate();	// Invalidate the entire set

	void printValidEntries();

};

#endif