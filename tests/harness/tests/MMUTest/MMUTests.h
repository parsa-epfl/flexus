#include <gtest/gtest.h>

#include <components/CommonQEMU/Translation.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

#include <components/MMU/MMUUtil.cpp>
#include <components/MMU/ARMTranslationGranules.cpp>

#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
// Test fixture for the MMU component

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

TranslationPtr MMUTestFixture::payload = nullptr;


// TLBHitTest

TEST_F(MMUTestFixture, Trace_PageAccessTest) {

    MMUConfiguration_struct aCfg("MMUTester config");
	InitializeMMUConfiguration(aCfg, 1, 64, 64, false);


    MMUJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "MMU index and widths defined\n";
	// Step 5: Instantiate the DUT
	nMMU::MMUComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "MMUComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "MMUComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nMMU;

	Translation* rawPtr = new nMMU::Translation(); // Create raw pointer
	TranslationPtr tlbRequest(rawPtr);
	InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);

    // // Perform the TLB lookup
    dut.push(MMUInterface::TLBReqIn(), 1, tlbRequest);
	std::cout << "dut TLBReqIn push done\n";


    ASSERT_TRUE(tlbRequest->theDone);
	ASSERT_EQ(tlbRequest->thePaddr, 0x1000);
	std::cout << "Page Translation Completed successfully\n";
    // Finalizing the component state
	dut.finalize();
}

// TLBMissTest

TEST_F(MMUTestFixture, Trace_TLBMissTest) {
 
    MMUConfiguration_struct aCfg("MMUTester config");
	InitializeMMUConfiguration(aCfg, 1, 16, 16, false);

    MMUJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "MMU index and widths defined\n";
	// Step 5: Instantiate the DUT
	nMMU::MMUComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "MMUComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "MMUComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nMMU;

	// Translation* rawPtr = new nMMU::Translation(); // Create raw pointer
	// TranslationPtr tlbRequest(rawPtr);
	// InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);


	const int numRequests = 17;
    std::unique_ptr<nMMU::Translation> rawPtrs[numRequests];
    TranslationPtr tlbRequests[numRequests];

    // for (int i = 0; i < numRequests; ++i) {
    //     rawPtrs[i] = std::make_unique<nMMU::Translation>();
    //     tlbRequests[i] = TranslationPtr(rawPtrs[i].get());
    //     InitializeTLBRequest(tlbRequests[i], 0x80000000+(i*4096), 0x00001000+(i*4096));
    // }

for (int i = 0; i < numRequests; ++i) {
    rawPtrs[i] = std::make_unique<nMMU::Translation>();
    tlbRequests[i] = TranslationPtr(rawPtrs[i].release()); // Use release() to transfer ownership
    InitializeTLBRequest(tlbRequests[i], 0x80000000+(i*4096), 0x00001000+(i*4096));
}

// Running Trace to Warmup InstrTLB
	MMUInterface::MMUDrive drive_temp;
    for (int i = 0; i < numRequests; ++i) {
	    dut.push(MMUInterface::TLBReqIn(), 1, tlbRequests[i]);
    }
// Running Timing to do the test
	// int value = 1;
	// dut.push(MMUInterface::ResyncIn(), 1, value);

    PhysicalMemoryAddress expectedResult = PhysicalMemoryAddress(0x00001000); 
    // for (int i = 0; i < numRequests; ++i) {
    // dut.push(MMUInterface::iRequestIn(), 1, tlbRequests[i]);
	// std::cout << i << ". dut iRequestIn push done\n";
	// }
	// dut.drive(drive_temp);
	// std::cout << "TLBMissTest dut driven\n";

    EXPECT_NE(expectedResult, tlbRequests[0]->thePaddr);
	std::cout << "TLBMissTest Assertion made\n";

    // Finalizing the component state
	dut.finalize();

}

// TLBPageFaultTest

TEST_F(MMUTestFixture, Timing_PageAccessTest) {
 

    MMUConfiguration_struct aCfg("MMUTester config");
	InitializeMMUConfiguration(aCfg, 1, 64, 64, false);


    MMUJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "MMU index and widths defined\n";
	// Step 5: Instantiate the DUT
	nMMU::MMUComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "MMUComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "MMUComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nMMU;

	Translation* rawPtr = new nMMU::Translation(); // Create raw pointer
	TranslationPtr tlbRequest(rawPtr);
	InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);
	MMUInterface::MMUDrive drive_temp;

	// Resync the MMU
	int value = 1;
	dut.push(MMUInterface::ResyncIn(), 1, value);


    // // Perform the TLB lookup
    // dut.push(MMUInterface::TLBReqIn(), 1, tlbRequest);
	// std::cout << "dut TLBReqIn push done\n";

	for(int i = 0; i<11; i++){
		// Manual Reference: D5.3.1 VMSAv8-64 translation table level -1, level 0, level 1, and level 2 descriptor formats
		tlbRequest->rawTTEValue = 0x1023;
		dut.push(MMUInterface::iRequestIn(), 1, tlbRequest);
		std::cout << "dut iRequestIn push done\n";
		dut.drive(drive_temp);
		std::cout << "dut Drive push done\n";
		tlbRequest = getPayloadp();

	}
	std::cout << "PageAccess dut driven\n";

    ASSERT_EQ(true, tlbRequest->theDone);
	std::cout << "Page Tables Translated Successfully \n";
	ASSERT_EQ(tlbRequest->thePaddr, 0x1000);
	std::cout << "Page Translation Completed with correct address\n";
	dut.finalize();

}

//Test Number 3a of the TODO
TEST_F(MMUTestFixture, Timing_TLBMissToInvalidBitPageFaultTest) {

    MMUConfiguration_struct aCfg("MMUTester config");
	InitializeMMUConfiguration(aCfg, 1, 64, 64, false);


    MMUJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "MMU index and widths defined\n";
	// Step 5: Instantiate the DUT
	nMMU::MMUComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "MMUComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "MMUComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nMMU;

	Translation* rawPtr = new nMMU::Translation(); // Create raw pointer
	TranslationPtr tlbRequest(rawPtr);
	InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);
	MMUInterface::MMUDrive drive_temp;

// 3.a. Page Fault level 1
	// Resync the MMU
	int value = 1;
	dut.push(MMUInterface::ResyncIn(), 1, value);


    // // Perform the TLB lookup
    // dut.push(MMUInterface::TLBReqIn(), 1, tlbRequest);
	// std::cout << "dut TLBReqIn push done\n";

	for(int i = 0; i<11; i++){
		// Manual Reference: D5.3.1 VMSAv8-64 translation table level -1, level 0, level 1, and level 2 descriptor formats
		tlbRequest->rawTTEValue = 0x1023;
		if(tlbRequest->theCurrentTranslationLevel ==1) {tlbRequest->rawTTEValue = 0x0000000000000002;}
		if(tlbRequest->thePaddr == 0xffffffffffffffff){break;}
		dut.push(MMUInterface::iRequestIn(), 1, tlbRequest);
		std::cout << "dut iRequestIn push done\n";
		dut.drive(drive_temp);
		std::cout << "dut Drive push done\n";
		tlbRequest = getPayloadp();

	}
	std::cout << "dut driven\n";

    ASSERT_EQ(true, (tlbRequest->thePageFault) & (tlbRequest->theCurrentTranslationLevel==1));
	std::cout<<"*** Page fault occurred as expected at Level 1 *** \n" ;
	
// 3.b. Page Fault level 2
	InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);
	MMUInterface::MMUDrive drive_temp2;
	tlbRequest->theDone = false;
	tlbRequest->thePageFault = false;

	// Resync the MMU
	dut.push(MMUInterface::ResyncIn(), 1, value);


    // // Perform the TLB lookup
    // dut.push(MMUInterface::TLBReqIn(), 1, tlbRequest);
	// std::cout << "dut TLBReqIn push done\n";

	for(int i = 0; i<11; i++){
		// Manual Reference: D5.3.1 VMSAv8-64 translation table level -1, level 0, level 1, and level 2 descriptor formats
		tlbRequest->rawTTEValue = 0x1023;
		if(tlbRequest->theCurrentTranslationLevel ==2) {tlbRequest->rawTTEValue = 0x0000000000000002;}
		if(tlbRequest->thePaddr == 0xffffffffffffffff){break;}
		dut.push(MMUInterface::iRequestIn(), 1, tlbRequest);
		std::cout << "dut iRequestIn push done\n";
		dut.drive(drive_temp2);
		std::cout << "dut Drive push done\n";
		tlbRequest = getPayloadp();

	}
	std::cout << "dut driven\n";
    ASSERT_EQ(true, (tlbRequest->thePageFault) & (tlbRequest->theCurrentTranslationLevel==2));
	std::cout<<"*** Page fault occurred as expected at Level 2 ***  \n" ;

// 3.c. Page Fault level 3

	InitializeTLBRequest(tlbRequest, 0x80000000, 0x00001000);
	MMUInterface::MMUDrive drive_temp3;
	tlbRequest->theDone = false;
	tlbRequest->thePageFault = false;

	// Resync the MMU
	dut.push(MMUInterface::ResyncIn(), 1, value);


    // // Perform the TLB lookup
    // dut.push(MMUInterface::TLBReqIn(), 1, tlbRequest);
	// std::cout << "dut TLBReqIn push done\n";

	for(int i = 0; i<11; i++){
		// Manual Reference: D5.3.1 VMSAv8-64 translation table level -1, level 0, level 1, and level 2 descriptor formats
		tlbRequest->rawTTEValue = 0x1023;
		if(tlbRequest->theCurrentTranslationLevel ==3) {tlbRequest->rawTTEValue = 0x0000000000000002;}
		if(tlbRequest->thePaddr == 0xffffffffffffffff){break;}
		dut.push(MMUInterface::iRequestIn(), 1, tlbRequest);
		std::cout << "dut iRequestIn push done\n";
		dut.drive(drive_temp3);
		std::cout << "dut Drive push done\n";
		tlbRequest = getPayloadp();

	}
	std::cout << "dut driven\n";

    ASSERT_EQ(true, (tlbRequest->thePageFault) & (tlbRequest->theCurrentTranslationLevel==3));
	std::cout<<"*** Page fault occurred as expected at Level 3 *** \n" ;


	dut.finalize();
}



