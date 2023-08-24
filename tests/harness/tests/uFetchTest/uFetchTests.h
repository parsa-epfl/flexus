#include <gtest/gtest.h>

#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/seq_map.hpp>

#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
// Test fixture for the uFetch component

/*  
Manual references:
D5.3 VMSAv8-64 translation table format descriptors
D5.2.5 Translation tables and the translation process

TESTS TO WRITE:
* 1. TLB Miss to Page Walk access -> Verify that Page Walk PortOutput has a request pending with the right VA
* 2. TLB Push to TLB Hit both dTLB and iTLB -> TLBReqIn push new entry, send Request, assert RequestResponse PortOutput has request pending with right VA
* 3. TLB Miss to Page Fault -> Cannot do the walk, check in manual what does a Page Table entry get inited to -> BitMap for invalid? Address == NULL / -1?
*	a. Page Fault level 0, 1, 2, 3 => 4 tests
*	b. Succesful level 1, 2, 3 walks => 3 tests
* 4. TLB Miss to Page Walk full walk, match intermediate addresses -> Stages 1, 2, 3, 4 assertions on cache access
* 5. TLB Miss, Page Walk, TLB Fill, to TLB Hit
* 6. Consumption of traces

*/

TranslationPtr uFetchTestFixture::payload = nullptr;


// TLBHitTest

TEST_F(uFetchTestFixture, Trace_PageAccessTest) {

    uFetchConfiguration_struct aCfg("uFetchTester config");
	InitializeuFetchConfiguration(aCfg, 32, 2, 10, 64, true, true, false, 65536, 4, 4, 1, false ,false, true);


    uFetchJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "uFetch index and widths defined\n";
	// Step 5: Instantiate the DUT
	nuFetch::uFetchComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "uFetchComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "uFetchComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nuFetch;

	// WRITE TESTS HERE

	// Translation* rawPtr = new nuFetch::Translation(); // Create raw pointer
	// TranslationPtr tlbRequest(rawPtr);
	// InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);

    // // // Perform the TLB lookup
    // dut.push(uFetchInterface::TLBReqIn(), 1, tlbRequest);
	// std::cout << "dut TLBReqIn push done\n";


    // ASSERT_TRUE(tlbRequest->theDone);
	// ASSERT_EQ(tlbRequest->thePaddr, 0x1000);
	// std::cout << "Page Translation Completed successfully\n";
    // Finalizing the component state
	dut.finalize();
}