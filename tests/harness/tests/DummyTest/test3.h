#include <iostream>
#include <gtest/gtest.h>

// Create a new test
TEST(DynamicPortTest, DynamicPortTest)
{
	// Step 2: Create a configuration struct and specify the parameters
	DummyConfiguration_struct aCfg("The test config");
	aCfg.InitState = 0;

	// Step 3: Create a dummy JumpTable and register the ouput port functions
	DummyJumpTable aJumpTable;
	
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

	// Test setStateDyn
	DummyInterface::setStateDyn setStateDyn_temp;
	ASSERT_EQ(dut.available(setStateDyn_temp, 1), false) << "setStateDyn returned true instead of false. Failed!";	
	ASSERT_EQ(dut.available(setStateDyn_temp, 2), true) << "setStateDyn returned flase instead of true. Failed!";	
	ASSERT_EQ(dut.available(setStateDyn_temp, 0), true) << "setStateDyn returned flase instead of true. Failed!";	
	
	int payload = 5;
	dut.push(setStateDyn_temp, 3, payload);
	ASSERT_EQ( dut.getStateNonWire(), 15 ) << "setStateDyn failed to set state correctly";
}

