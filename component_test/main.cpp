#include <iostream>
#include <components/MemoryLoopback/MemoryLoopbackImpl.cpp> // Step 1: Include the .cpp file of the DUT

int main()
{
//	using namespace Flexus;
//	using namespace Core;
//	using namespace SharedTypes;
//	using Flexus::SharedTypes::MemoryMap;
//	using boost::intrusive_ptr;
	// Step 2: Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");
	aCfg.Delay = 2;
	aCfg.MaxRequests = 1;
	aCfg.UseFetchReply = true;

	// Step 3: Create a dummy JumpTable
	MemoryLoopbackJumpTable aJumpTable;

	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;

	// Step 5: Instantiate the DUT
	nMemoryLoopback::MemoryLoopbackComponent dut(
	        aCfg,
		aJumpTable,
		anIndex,
		aWidth
		);

	std::cout << "test" << std::endl;
	return -1;
}
