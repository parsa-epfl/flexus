#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <gtest/gtest.h>

// Define callback function for LoopbackOut available
bool LoopbackOut_avail_fetch_reply(Flexus::Core::index_t)
{
	return true;		// Always available
}

// Define callback function for LoopbackOut manipulate
bool calledPushFetchReply = false;	// Stores whether manipulate was called or not
bool gotFetchReply = false;	// Stores whether manipulate was called with correct message or not
void LoopbackOut_manip_fetch_reply(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
{
	if(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type() == Flexus::SharedTypes::MemoryMessage::FetchReply &&
	   uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address()) == 1000)
		gotFetchReply = true;
	calledPushFetchReply = true;
} 

// Create a new test
TEST_F(MemoryLoopbackTestFixture, UseFetchReplyConfiguration)
{
	// Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");
	aCfg.Delay = 1;
	aCfg.MaxRequests = 64;
	aCfg.UseFetchReply = true;

	// Create the jump table and register callbacks
	MemoryLoopbackJumpTable aJumpTable;
	aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail_fetch_reply;	
	aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip_fetch_reply;	
	
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
	MemoryTransport aMemoryTransport;
	intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(MemoryMessage::FetchReq, PhysicalMemoryAddress(1000)); 
	aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
	
	// Send the message
	dut.push(LoopbackIn_tmp, aMemoryTransport);
	
	// Drive the dut	
	MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
	dut.drive(LoopbackDrive_tmp);
	
	// Assert that LoopbackIn is not ready to receive data
	ASSERT_EQ( calledPushFetchReply, true  ) << "No data was sent over LoopbackOut. Failed!";
	ASSERT_EQ( gotFetchReply, true ) << "Got wrong message type from DUT. Failed!";
}

// Define callback function for LoopbackOut manipulate
void LoopbackOut_manip_miss_writable_reply(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
{
	if(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type() == Flexus::SharedTypes::MemoryMessage::MissReplyWritable &&
	   uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address()) == 1000)
		gotFetchReply = true;
	calledPushFetchReply = true;
} 

// Create a new test
TEST_F(MemoryLoopbackTestFixture, UseMissReplyWritableConfiguration)
{
  // Set output vars to false before test.
  calledPushFetchReply = false;
  gotFetchReply = false;

	// Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");
	aCfg.Delay = 1;
	aCfg.MaxRequests = 64;
	aCfg.UseFetchReply = false;

	// Create the jump table and register callbacks
	MemoryLoopbackJumpTable aJumpTable;
	aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail_fetch_reply;	
	aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip_miss_writable_reply;	
	
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
	MemoryTransport aMemoryTransport;
	intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(MemoryMessage::FetchReq, PhysicalMemoryAddress(1000)); 
	aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
	
	// Send the message
	dut.push(LoopbackIn_tmp, aMemoryTransport);
	
	// Drive the dut	
	MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
	dut.drive(LoopbackDrive_tmp);
	
	// Assert that LoopbackIn is not ready to receive data
	ASSERT_EQ( calledPushFetchReply, true  ) << "No data was sent over LoopbackOut. Failed!";
	ASSERT_EQ( gotFetchReply, true ) << "Got wrong message type from DUT. Failed!";
}
