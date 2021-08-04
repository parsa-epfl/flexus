#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <gtest/gtest.h>

// Define callback function for LoopbackOut available
bool LoopbackOut_avail_max_req(Flexus::Core::index_t)
{
	return true;		// Always available
}

// Define callback function for LoopbackOut manipulate
bool calledPushMaxReq = false;	// Stores whether manipulate was called or not
bool gotLoadReplyMaxReq = false;	// Stores whether manipulate was called with correct message or not
void LoopbackOut_manip_max_req(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
{
	if(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type() == Flexus::SharedTypes::MemoryMessage::LoadReply &&
	   uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address()) == 1000)
		gotLoadReplyMaxReq = true;
	calledPushMaxReq = true;
} 

// Create a new test
// This tests whether MemoryLoopback respects the MaxRequests parameter
TEST_F(MemoryLoopbackTestFixture, MaxRequestsConfiguration)
{
	// Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");
	aCfg.Delay = 1;
	aCfg.MaxRequests = 2;
	aCfg.UseFetchReply = false;

	// Create the jump table and register callbacks
	MemoryLoopbackJumpTable aJumpTable;
	aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail_max_req;	
	aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip_max_req;	
	
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

	// Assert that LoopbackIn is ready to receive some data
	MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;	
	ASSERT_EQ( dut.available(LoopbackIn_tmp), true ) << "LoopbackIn is not ready to receive 1st data. Failed!";
	
	// Generate the messages to be sent
	using boost::intrusive_ptr;
	MemoryTransport aMemoryTransport, anotherMemoryTransport;
	intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(MemoryMessage::LoadReq, PhysicalMemoryAddress(1000)); 
	intrusive_ptr<MemoryMessage> anotherMemoryMessage = new MemoryMessage(MemoryMessage::LoadReq, PhysicalMemoryAddress(1000)); 
	aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
	anotherMemoryTransport.set(MemoryMessageTag, anotherMemoryMessage);
	
	// Send the message
	dut.push(LoopbackIn_tmp, aMemoryTransport);
	
	// Assert that LoopbackIn is still ready to receive data
	ASSERT_EQ( dut.available(LoopbackIn_tmp), true ) << "LoopbackIn is not ready to receive 2nd data. Failed!";
	
	// Send the message
	dut.push(LoopbackIn_tmp, anotherMemoryTransport);
	
	// Assert that LoopbackIn is not ready to receive data
	ASSERT_EQ( dut.available(LoopbackIn_tmp), false ) << "LoopbackIn is ready to receive 3rd data. Failed!";
	
	// Drive the dut	
	MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
	dut.drive(LoopbackDrive_tmp);
	
	// Assert that LoopbackIn is again ready to receive data
	ASSERT_EQ( dut.available(LoopbackIn_tmp), true ) << "LoopbackIn is not ready to receive data again. Failed!";
}
