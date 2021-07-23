#include <iostream>
#include <gtest/gtest.h>

// To connect to PushOutput port of a component, create two functions: available and manipulate, and register them in the JumpTable
// Available tells the component if its outgoing port is ready to send data or not
static bool avail1(Flexus::Core::index_t idx)
{
	std::cout << "Available called\n";
	return true;
}

// Manipulate sends the payload over
static void manip1(Flexus::Core::index_t idx, int &payload)
{
	std::cout << "Manipulate called with " << payload << "\n";
}

// To connect to PullInput port of a component, create two functions: available and manipulate, and register them in the JumpTable
// Available tells the component if its incoming port is ready to send data or not
static bool pull_avail1(Flexus::Core::index_t idx)
{
	std::cout << "Pull Available called\n";
	return true;
}

// Manipulate sends the payload over
static void pull_manip1(Flexus::Core::index_t idx, int &payload)
{
	std::cout << "Pull Manipulate called with " << payload << "\n";
	payload = 2;
}

// Create a new test
TEST(DummyTest1, DummyInstantiation)
{
	// Step 2: Create a configuration struct and specify the parameters
	DummyConfiguration_struct aCfg("The test config");
	aCfg.InitState = 0;

	// Step 3: Create a dummy JumpTable and register the ouput port functions
	DummyJumpTable aJumpTable;
	aJumpTable.wire_available_getState = avail1;
	aJumpTable.wire_manip_getState = manip1;
	aJumpTable.wire_available_pullStateIn = pull_avail1;
	aJumpTable.wire_manip_pullStateIn = pull_manip1;
	
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
	ASSERT_EQ( dut.isQuiesced(), true ) << "Quiesced returned false. Failed! ";
	
	// Sending some data to the component
	DummyInterface::setState setState_temp;
	int payload = 5;
	ASSERT_EQ( dut.available(setState_temp), true) << "setState not available. Failed!";
	dut.push(setState_temp, payload);
	
	// Driving the component
	DummyInterface::DummyDrive drive_temp;
	dut.drive(drive_temp);

	ASSERT_EQ( dut.getStateNonWire(), 2 ) << "Getting internal dummy component state failed, returned " << dut.getStateNonWire();
	
	DummyInterface::pullStateRet pullStateRet_tmp;
	ASSERT_EQ( dut.available(pullStateRet_tmp), true) << "pullStateRet not always available! Failed!";
	ASSERT_EQ( dut.pull(pullStateRet_tmp), 2) << "pullStateRet returned wrong value! Returned " << dut.pull(pullStateRet_tmp);
		
	// Finalizing the component state
	dut.finalize();
}
