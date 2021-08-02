#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <gtest/gtest.h>
#include <iostream>
// Define callback function for LoopbackOut available
bool LoopbackOut_avail(Flexus::Core::index_t)
{
	return true;		// Always available
}

// Define callback function for LoopbackOut manipulate
static bool calledPush = false;	// Stores whether manipulate was called or not
static bool gotLoadReply = false;	// Stores whether manipulate was called with correct message or not
void LoopbackOut_manip(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
{
	if(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type() == Flexus::SharedTypes::MemoryMessage::LoadReply &&
	   uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address()) == 1000)
		gotLoadReply = true;
	calledPush = true;
} 

// Create a new test
TEST_F(MemoryLoopbackTestFixture, DelayConfiguration)
{
	// Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");
	aCfg.Delay = 2;
	aCfg.MaxRequests = 64;
	aCfg.UseFetchReply = false;

	// Create the jump table and register callbacks
	MemoryLoopbackJumpTable aJumpTable;
	aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail;	
	aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip;	
	
	// Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;

	// Instantiate the DUT
	nMemoryLoopback::MemoryLoopbackComponent dut(
	        aCfg,				// Specifies initial config
		aJumpTable,			// Registers callbacks for output ports
		anIndex,
		aWidth
		);


	dut.initialize();

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;

	// Generate the message to be sent
	using boost::intrusive_ptr;
	MemoryTransport aMemoryTransport;
	intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(MemoryMessage::LoadReq, PhysicalMemoryAddress(1000)); 
	aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
	
	// Send the message
	MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;	
	dut.push(LoopbackIn_tmp, aMemoryTransport); 
	
	// Drive the dut	
	MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
	dut.drive(LoopbackDrive_tmp);
	
	// Check that the reply didn't come early
	ASSERT_EQ( calledPush, false ) << "DUT replied called early. Failed!";
	
	// Advance simulation cycle
	theFlexus->doCycle();
	
	// Drive dut
	dut.drive(LoopbackDrive_tmp);
	
	// Check whether the DUT replied
	ASSERT_EQ( calledPush, true ) << "DUT didn't reply. Failed!";
	ASSERT_EQ( gotLoadReply, true) << "DUT replied with wring message. Failed!";

}
