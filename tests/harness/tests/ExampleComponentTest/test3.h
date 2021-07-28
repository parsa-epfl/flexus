#include <iostream>
#include <gtest/gtest.h>

// To connect to PushOutput port of a component, create two functions: available and manipulate, and register them in the JumpTable
// Available tells the component if its outgoing port is ready to send data or not
bool push_avail_dyn3_called = false;
static bool push_avail_dyn3(Flexus::Core::index_t idx)
{
	push_avail_dyn3_called = true;
	return true;
}

// Manipulate sends the payload over
int push_manip_dyn3_data = -1;
static void push_manip_dyn3(Flexus::Core::index_t idx, int &payload)
{
	push_manip_dyn3_data = payload;
}

// To connect to PullInput port of a component, create two functions: available and manipulate, and register them in the JumpTable
// Available tells the component if its incoming port is ready to send data or not
bool pull_avail_dyn3_called = false;
static bool pull_avail_dyn3(Flexus::Core::index_t idx)
{
	pull_avail_dyn3_called = true;
	return true;
}

// Manipulate sends the payload over
static void pull_manip_dyn3(Flexus::Core::index_t idx, int &payload)
{
	payload = idx * 100;
}

// Create a new test
TEST(DynamicPortTest, DynamicPortTest)
{
	// Step 2: Create a configuration struct and specify the parameters
	DummyConfiguration_struct aCfg("The test config");
	aCfg.InitState = 0;

	// Step 3: Create a dummy JumpTable and register the ouput port functions
	DummyJumpTable aJumpTable;
	aJumpTable.wire_available_getStateDyn = push_avail_dyn3;
	aJumpTable.wire_manip_getStateDyn = push_manip_dyn3;
	aJumpTable.wire_available_pullStateInDyn = pull_avail_dyn3;
	aJumpTable.wire_manip_pullStateInDyn = pull_manip_dyn3;
	
	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 0;
	Flexus::Core::index_t aWidth =  0;

	// Step 5: Instantiate the DUT
	nDummy::DummyComponent dut(
	        aCfg,				// Specifies initial config
		aJumpTable,			// Registers callbacks for output ports
		anIndex,
		aWidth
		);

	// Initializing the component
	dut.initialize();

	// Test setStateDyn
	DummyInterface::setStateDyn setStateDyn_temp;
	ASSERT_EQ(dut.available(setStateDyn_temp, 1), false) << "setStateDyn returned true instead of false. Failed!";	
	ASSERT_EQ(dut.available(setStateDyn_temp, 2), true) << "setStateDyn returned flase instead of true. Failed!";	
	ASSERT_EQ(dut.available(setStateDyn_temp, 0), true) << "setStateDyn returned flase instead of true. Failed!";	
	ASSERT_EQ(DummyInterface::width(aCfg, setStateDyn_temp), 4) << "setStateDyn width was not set up correctly";
	int payload = 5;
	dut.push(setStateDyn_temp, 3, payload);
	ASSERT_EQ(dut.getStateNonWire(), 15) << "setStateDyn failed to set state correctly";
	
	// Drive Component	
	DummyInterface::DummyDrive drive_temp;
	dut.drive(drive_temp);

	// Test getStateDyn
	ASSERT_EQ(push_avail_dyn3_called, true) << "getStateDyn available not called";	
	ASSERT_EQ(push_manip_dyn3_data, 20) << "getStateDyn returned wrong data";	
	
	// Test pullStateInDyn
	ASSERT_EQ(pull_avail_dyn3_called, true) << "pullStateIneDyn available not called";	
	ASSERT_EQ(dut.getStateNonWire(), 300)  << "pullStateIn got wrong data, internal state was updated to" << dut.getStateNonWire();	
	
	// Test pullStateRetDyn
	DummyInterface::pullStateRetDyn pullStateRetDyn_temp;
	ASSERT_EQ(dut.available(pullStateRetDyn_temp, 1), true) << "pullStateRetDyn returned false instead of true. Failed!";	
	ASSERT_EQ(dut.pull(pullStateRetDyn_temp, 3), 3000) << "pullStateRetDyn returned incorrect value, returned " << dut.pull(pullStateRetDyn_temp, 3);
}
