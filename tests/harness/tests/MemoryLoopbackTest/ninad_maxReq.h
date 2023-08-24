#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <boost/range/combine.hpp>

// Define callback function for LoopbackOut available
// bool LoopbackOut_avail_message_type(Flexus::Core::index_t)
// {
//     return true;        // Always available
// }

// Define callback function for LoopbackOut manipulate
// Flexus::SharedTypes::MemoryMessage::MemoryMessageType type;    // Received type
// uint64_t addr;                    // Received address

// void LoopbackOut_manip_message_type(Flexus::Core::index_t, Flexus::SharedTypes::MemoryTransport &aMemoryTransport)
// {
//     type = aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->type();
//     addr = uint64_t(aMemoryTransport[Flexus::SharedTypes::MemoryMessageTag]->address());
// } 

// Create a new test
// This tests whether MemoryLoopback responds correctly when the maximum number of requests is reached
TEST_F(MemoryLoopbackTestFixture, MaxRequestsReached)
{
    // Create a configuration struct and specify the parameters
    MemoryLoopbackConfiguration_struct aCfg("The test config");
    aCfg.Delay = 1;
    aCfg.MaxRequests = 10; // Set a smaller value for easier testing
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
        aCfg,                // Specifies initial config
        aJumpTable,            // Registers callbacks for output ports
        anIndex,
        aWidth
    );

    dut.initialize();

    using namespace Flexus;
    using namespace Core;
    using namespace SharedTypes;

    // Create dummy variables to call push and drive
    MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
    MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;    

    // Generate test vectors
    // This is a vector of requests to be sent
    std::vector<MemoryMessage::MemoryMessageType> requests = {
        MemoryMessage::LoadReq,
        MemoryMessage::StoreReq,
        MemoryMessage::FetchReq,
        MemoryMessage::CmpxReq,
        MemoryMessage::ReadReq,
        MemoryMessage::WriteReq,
        MemoryMessage::UpgradeReq,
        MemoryMessage::StorePrefetchReq,
        MemoryMessage::PrefetchReadAllocReq,
        MemoryMessage::NonAllocatingStoreReq
    };

    // Send the maximum number of requests to the DUT
    for (int i = 0; i < aCfg.MaxRequests; ++i) {
        // Assert that LoopbackIn is ready to receive some data
        ASSERT_EQ(dut.available(LoopbackIn_tmp), true) << "LoopbackIn is not ready to receive data. Failed!";
        
        // Generate the messages to be sent
        using boost::intrusive_ptr;
        MemoryTransport aMemoryTransport;
        intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(requests[i % requests.size()], PhysicalMemoryAddress(i));
        aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);
        
        // Send the message
        dut.push(LoopbackIn_tmp, aMemoryTransport);
        
        // Drive the dut    
        dut.drive(LoopbackDrive_tmp);
    }

    // Verify that LoopbackOut was called for each request
    ASSERT_EQ(aCfg.MaxRequests, addr + 1) << "LoopbackOut was not called for every request. Failed!";
}
