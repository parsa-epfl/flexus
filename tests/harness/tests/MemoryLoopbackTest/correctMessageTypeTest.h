#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <utility>
#include <boost/range/combine.hpp>

// Define callback function for LoopbackOut available
bool LoopbackOut_avail_message_type(Flexus::Core::index_t)
{
	return true;		// Always available
}

// Define callback function for LoopbackOut manipulate
Flexus::SharedTypes::MemoryMessage::MemoryMessageType type;	// Received type
uint64_t addr;					// Received address
void LoopbackOut_manip_message_type(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
{
	type = aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type();
	addr = uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address());
} 

// Create a new test
TEST_F(MemoryLoopbackTestFixture, CorrectMessageType)
{
	// Create a configuration struct and specify the parameters
	MemoryLoopbackConfiguration_struct aCfg("The test config");
	aCfg.Delay = 1;
	aCfg.MaxRequests = 64;
	aCfg.UseFetchReply = false;

	// Create the jump table and register callbacks
	MemoryLoopbackJumpTable aJumpTable;
	aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail_message_type;	
	aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip_message_type;	
	
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


	// Create dummy variablesto call push and drive
	MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
	MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;	

	// Generate test vectors
	std::vector<std::pair<MemoryMessage::MemoryMessageType, uint64_t>> trigger = 
	{
		std::make_pair(MemoryMessage::LoadReq,			1),
		std::make_pair(MemoryMessage::FetchReq, 		2),
		std::make_pair(MemoryMessage::StoreReq, 		3),
		std::make_pair(MemoryMessage::StorePrefetchReq,		4),
		std::make_pair(MemoryMessage::CmpxReq, 			5),
		std::make_pair(MemoryMessage::ReadReq, 			6),
		std::make_pair(MemoryMessage::WriteReq, 		7),
		std::make_pair(MemoryMessage::WriteAllocate,	 	8),
		std::make_pair(MemoryMessage::NonAllocatingStoreReq, 	9),
		std::make_pair(MemoryMessage::UpgradeReq, 		10),
		std::make_pair(MemoryMessage::UpgradeAllocate, 		11),
		std::make_pair(MemoryMessage::PrefetchReadAllocReq,	12),
		std::make_pair(MemoryMessage::PrefetchReadNoAllocReq,	13),
		std::make_pair(MemoryMessage::StreamFetch, 		14)
	};	

	// Generate expected output
	std::vector<std::pair<MemoryMessage::MemoryMessageType, uint64_t>> expected =
	{
		std::make_pair(MemoryMessage::LoadReply, 		1),
		std::make_pair(MemoryMessage::MissReplyWritable, 	2),
		std::make_pair(MemoryMessage::StoreReply, 		3),
		std::make_pair(MemoryMessage::StorePrefetchReply,	4),
		std::make_pair(MemoryMessage::CmpxReply,		5),
		std::make_pair(MemoryMessage::MissReplyWritable,	6),
		std::make_pair(MemoryMessage::MissReplyWritable,	7),
		std::make_pair(MemoryMessage::MissReplyWritable, 	8),
		std::make_pair(MemoryMessage::NonAllocatingStoreReply, 	9),
		std::make_pair(MemoryMessage::UpgradeReply, 		10),
		std::make_pair(MemoryMessage::UpgradeReply, 		11),
		std::make_pair(MemoryMessage::PrefetchWritableReply,	12),
		std::make_pair(MemoryMessage::PrefetchWritableReply,	13),
		std::make_pair(MemoryMessage::StreamFetchWritableReply,	14)
	};	

	for(auto testCase : boost::combine(trigger, expected))
	{
		
		std::pair<MemoryMessage::MemoryMessageType, uint64_t> trig, exp;
		boost::tie(trig, exp) = testCase;

		// Assert that LoopbackIn is ready to receive some data
		ASSERT_EQ( dut.available(LoopbackIn_tmp), true ) << "LoopbackIn is not ready to receive data. Failed!";
		
		// Generate the messages to be sent
		using boost::intrusive_ptr;
		MemoryTransport aMemoryTransport;
		intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(trig.first, PhysicalMemoryAddress(trig.second)); 
		aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
		
		// Send the message
		dut.push(LoopbackIn_tmp, aMemoryTransport);
		
		// Drive the dut	
		dut.drive(LoopbackDrive_tmp);
		
		// Assert that LoopbackIn is not ready to receive data
		ASSERT_EQ( exp.first, type ) << "Got wring message type. Failed!";
		ASSERT_EQ( exp.second, addr ) << "Got wrong address from DUT. Failed! Got " << addr << " Expected " << exp.second;
	}	
}
