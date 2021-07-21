#include <iostream>
#include <components/Dummy/DummyImpl.cpp> // Step 1: Include the .cpp file of the DUT
#include <gtest/gtest.h>


// To connect to the outputs of a components, create two functions: available and manipulate, and register them in the JumpTable
// Available tell the component if its outgoing port is ready to send data or not
static bool avail(Flexus::Core::index_t idx)
{
	std::cout << "Available called\n";
	return true;
}

// Manipulate sends the payload over
static void manip(Flexus::Core::index_t idx, int &payload)
{
	std::cout << "Manipulate called with " << payload << "\n";
}

// Create a new test
TEST(DummyTest1, DummyInstantiation)
{
	// Step 2: Create a configuration struct and specify the parameters
	DummyConfiguration_struct aCfg("The test config");
	aCfg.InitState = 0;

	// Step 3: Create a dummy JumpTable and register the ouput port functions
	DummyJumpTable aJumpTable;
	aJumpTable.wire_available_getState = avail;
	aJumpTable.wire_manip_getState = manip;
	
	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;

	// Step 5: Instantiate the DUT
	nDummy::DummyComponent dut(
	        aCfg,				// Specifies initial config
		aJumpTable,			// Registers callbacks for output ports
		anIndex,
		aWidth
		);

	// Initializing the component
	dut.initialize();

	// Checking if the component is quiesced
	std::cout << "Quiesced: " << dut.isQuiesced() << std::endl;
	
	// Sending some data to the component
	DummyInterface::setState setState_temp;
	int payload = 5;
	std::cout << "Available: " << dut.available(setState_temp)<< std::endl;
	dut.push(setState_temp, payload);
	
	// Driving the component
	DummyInterface::DummyDrive drive_temp;
	dut.drive(drive_temp);
	
	// Finalizing the component state
	dut.finalize();
}


TEST(DummyTest2, DummyInstantiation)
{
	// Step 2: Create a configuration struct and specify the parameters
	DummyConfiguration_struct aCfg("The test config");
	aCfg.InitState = 0;

	// Step 3: Create a dummy JumpTable and register the ouput port functions
	DummyJumpTable aJumpTable;
	aJumpTable.wire_available_getState = avail;
	aJumpTable.wire_manip_getState = manip;
	
	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;

	// Step 5: Instantiate the DUT
	nDummy::DummyComponent dut(
	        aCfg,				// Specifies initial config
		aJumpTable,			// Registers callbacks for output ports
		anIndex,
		aWidth
		);

	// Initializing the component
	dut.initialize();

	// Checking if the component is quiesced
	std::cout << "Quiesced: " << dut.isQuiesced() << std::endl;
	
	// Sending some data to the component
	DummyInterface::setState setState_temp;
	int payload = 5;
	std::cout << "Available: " << dut.available(setState_temp)<< std::endl;
	dut.push(setState_temp, payload);
	
	// Driving the component
	DummyInterface::DummyDrive drive_temp;
	dut.drive(drive_temp);
	
	// Finalizing the component state
	dut.finalize();
}
