#ifndef FLEXUS_IOTLB
#define FLEXUS_IOTLB

#include "IOTLBSet.hpp"
#include "core/checkpoint/json.hpp"
#include "core/types.hpp"

#include <vector>

using json = nlohmann::json;
using namespace Flexus::SharedTypes;

class IOTLB
{
private:
	uint16_t pageSize;				// Different instances of IOTLB can have different page sizes
									// However each IOTLB can have only one page size
									// This will hwlp in implementing a Split TLB in the future		
									// (2^pageSize) gives the actual size of the page used (pageSize = 12 => 4KB pages)				
	std::vector<IOTLBSet> theIOTLB; // Array of Sets make up a cache
	uint64_t theIndexMask;

public:
	uint32_t theIOTLBSets;
	uint32_t theIOTLBAssoc;

private:
	IOTLB() = default;

private:
	/* The IOVA is assumed to be page-aligned so the index into the Set Associative structure is */
	/* INDEX_MASK anded with the IOVA >> pageSize */
	uint32_t index(VirtualMemoryAddress aVaddr);

	VirtualMemoryAddress iovaPFN(VirtualMemoryAddress aVaddr);
	PhysicalMemoryAddress iopaPFN(PhysicalMemoryAddress aPaddr);

public:
	IOTLB(uint16_t pageSize, int32_t IOTLBSets, int32_t IOTLBAssoc);

	// Whether IOTLB contains the translation
	bool contains(uint16_t BDF, VirtualMemoryAddress aVaddr);

	// Get the hit IOTLB entry's IOPFN
	PhysicalMemoryAddress access(uint16_t BDF, VirtualMemoryAddress aVaddr);

	// Change or add a new translation. In case there is a hit, just modify. In case of a miss, add a new entry
	bool update(uint16_t BDF, VirtualMemoryAddress aVaddr, PhysicalMemoryAddress aPaddr);

	void invalidate();												// Invalidate the entire IOTLB
	void invalidate(uint16_t BDF);									// Invalidate all the entries belonging to a device
	void invalidate(uint16_t BDF, VirtualMemoryAddress aVaddr);		// Invlaidate a specific entry

	void printValidIOTLBEntries();
};

#endif