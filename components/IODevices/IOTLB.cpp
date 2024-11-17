#include "IOTLB.hpp"

#include <cstdint>

IOTLB::IOTLB(uint16_t pageSize, int32_t IOTLBSets, int32_t IOTLBAssoc)
  : pageSize(pageSize)
  , theIOTLBSets(IOTLBSets)
  , theIOTLBAssoc(IOTLBAssoc)
{
	// IOTLBSize must be a power of 2
	DBG_Assert(((theIOTLBSets - 1) & (theIOTLBSets)) == 0);
	theIOTLB.resize(theIOTLBSets, IOTLBSet(theIOTLBAssoc));

	theIndexMask = theIOTLBSets - 1;    // As theIOTLBSets is a power of 2, subtracting 1 will directly give us the mask
}


uint32_t
IOTLB::index(VirtualMemoryAddress aVaddr)
{
	// Shift address by pageSize
	return (((uint64_t)aVaddr) >> pageSize) & theIndexMask;
}

VirtualMemoryAddress 
IOTLB::iovaPFN(VirtualMemoryAddress aVaddr) {
	return VirtualMemoryAddress( ((uint64_t)aVaddr) >> pageSize ); 
}

PhysicalMemoryAddress 
IOTLB::iopaPFN(PhysicalMemoryAddress aPaddr) {
	return PhysicalMemoryAddress( ((uint64_t)aPaddr) >> pageSize ); 
}

// Whether the IOTLB contains target for an IOVA
bool
IOTLB::contains(uint16_t ASID, VirtualMemoryAddress aVaddr)
{
	int32_t ind = index(aVaddr);
	return theIOTLB[ind].isHit(ASID, iovaPFN(aVaddr));
}

PhysicalMemoryAddress 
IOTLB::access(uint16_t ASID, VirtualMemoryAddress aVaddr) {
	int32_t ind = index(aVaddr);

	PhysicalMemoryAddress IOPAPFN = theIOTLB[ind].access(ASID, iovaPFN(aVaddr))->thePAPFN;

	return PhysicalMemoryAddress( (((uint64_t)IOPAPFN) << pageSize) | (((uint64_t)aVaddr) & ((1UL << pageSize) - 1)) );
}

// Update or add a new entry to the BTB
bool
IOTLB::update(uint16_t ASID, VirtualMemoryAddress aVaddr, PhysicalMemoryAddress aPaddr)
{
	int32_t ind = index(aVaddr);
	bool isHit  = theIOTLB[ind].isHit(ASID, iovaPFN(aVaddr));

	if (isHit) {    // Change in the mapping
		IOTLBEntry* iotlbEntry = theIOTLB[ind].access(ASID, iovaPFN(aVaddr)); // [MADHUR] Access will also update the replacement queue

		iotlbEntry->thePAPFN = iopaPFN(aPaddr);

		return false; // not a new entry
	} else {   // Adding a new entry
		
		theIOTLB[ind].insert(IOTLBEntry(ASID, iovaPFN(aVaddr), iopaPFN(aPaddr))); // [MADHUR] Inserting a new entry

		return true;
	}
}

void 
IOTLB::invalidate() {
	for (auto& iotlbSet : theIOTLB) {
		iotlbSet.invalidate();
	}
}

void 
IOTLB::invalidate(uint16_t ASID) {
	for (auto& iotlbSet : theIOTLB) {
		iotlbSet.invalidate(ASID);
	}
}

void 
IOTLB::invalidate(uint16_t ASID, VirtualMemoryAddress aVaddr) {
	for (auto& iotlbSet : theIOTLB) {
		iotlbSet.invalidate(ASID, iovaPFN(aVaddr));
	}
}

void 
IOTLB::printValidIOTLBEntries() {
	for (auto iotlbSet : theIOTLB) {
		iotlbSet.printValidEntries();
	}
}