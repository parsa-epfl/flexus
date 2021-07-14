#include <iostream>
#include <components/Dummy/DummyImpl.cpp> // Step 1: Include the .cpp file of the DUT

int main()
{
	// Step 2: Create a configuration struct and specify the parameters
	DummyConfiguration_struct aCfg("The test config");
	aCfg.InitState = 0;

	// Step 3: Create a dummy JumpTable
	DummyJumpTable aJumpTable;

	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;

	// Step 5: Instantiate the DUT
	nDummy::DummyComponent dut(
	        aCfg,
		aJumpTable,
		anIndex,
		aWidth
		);

	dut.initialize();
	std::cout << "test" << std::endl;
	return -1;
}
