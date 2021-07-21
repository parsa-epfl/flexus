#include <iostream>
#include <components/MemoryLoopback/MemoryLoopbackImpl.cpp> // Step 1: Include the .cpp file of the DUT
#include <gtest/gtest.h>

// Create a new test
TEST(MemoryLoopback, MemoryLoopbackInstantiation)
{
	// Step 2: Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");

	// Step 3: Create a dummy JumpTable and register the ouput port functions
	MemoryLoopbackJumpTable aJumpTable;
	
	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;

	// Step 5: Instantiate the DUT
	nMemoryLoopback::MemoryLoopbackComponent dut(
	        aCfg,				// Specifies initial config
		aJumpTable,			// Registers callbacks for output ports
		anIndex,
		aWidth
		);
}
