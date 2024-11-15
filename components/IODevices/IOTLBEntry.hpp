#ifndef FLEXUS_IOTLB_ENTRY
#define FLEXUS_IOTLB_ENTRY

#include <stdint.h>
#include <core/qemu/mai_api.hpp>
#include "core/debug/debug.hpp"

using namespace Flexus::SharedTypes;

class IOTLBEntry
{
public:

	bool valid;
	uint16_t theBDF;
	VirtualMemoryAddress theIOVAPFN;	// This is same as an IOVA
	PhysicalMemoryAddress thePAPFN;

	IOTLBEntry(uint16_t theBDF, VirtualMemoryAddress anIOVAPFN, PhysicalMemoryAddress aPAPFN)
		: valid(true)
		, theBDF(theBDF)
		, theIOVAPFN(anIOVAPFN)
		, thePAPFN(aPAPFN)
	{
	}

	IOTLBEntry()
		: valid(false)
	{
	}

	void print() {
		DBG_(Crit, (	<< "IOTLB Entry: IOVAPFN(0x" << std::hex << (uint64_t)theIOVAPFN
					 	<< ")\tPAPFN(" << (uint64_t)thePAPFN
						<< ")"
					));
	}
};

#endif