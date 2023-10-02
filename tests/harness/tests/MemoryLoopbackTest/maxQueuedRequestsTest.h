// File: MemoryLoopbackTest.cpp

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <utility>
#include <boost/range/combine.hpp>
#include <stdio.h>
// Test whether MemoryLoopback correctly handles the maximum number of queued requests
TEST_F(MemoryLoopbackTestFixture, MaximumQueuedRequests) {
    // ... (Previous test setup code)

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

    // Create dummy variables to call push and drive
    MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
    MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;

    // Generate test vectorsaCfg.MaxRequests
    std::vector<MemoryTransport*> requests; // Use pointers here

    // ... (Previous test vector generation code)

    // Send the requests and check the queued requests count
    int numQueuedRequests = 100;
    for (MemoryTransport* requestPtr : requests) { // Use pointers here
        // Assert that LoopbackIn is ready to receive some data
        ASSERT_EQ(dut.available(LoopbackIn_tmp), true) << "LoopbackIn is not ready to receive data. Failed!";

        // Send the request (use the pointer directly)
        dut.push(LoopbackIn_tmp, *requestPtr);

        // Increment the queued requests count after each push
        ++numQueuedRequests;

        // Drive the dut
        dut.drive(LoopbackDrive_tmp);

        // Ensure that the number of queued requests does not exceed the maximum allowed
        ASSERT_LE(numQueuedRequests, aCfg.MaxRequests) << "The number of queued requests exceeds the maximum allowed. Failed!";
        // std::cout<<"aCfg.MaxRequests"<<aCfg.MaxRequests<<"\n";
        // std::cout<<"numQueuedRequests"<<numQueuedRequests<<"\n";
        EXPECT_TRUE(false) << "diagnostic message"<<aCfg.MaxRequests;
        EXPECT_TRUE(false) << "numQueuedRequests"<<numQueuedRequests;

        }

    // Clean up the allocated MemoryTransport objects
    for (MemoryTransport* requestPtr : requests) {
        delete requestPtr;
    }
}