#include <gtest/gtest.h>

#include <components/CommonQEMU/BranchPredictor.hpp>
#include <components/MTManager/MTManager.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
// Test fixture for the FAG component


// TLBHitTest

TEST_F(FetchAddressGenerateTestFixture, FetchAddressGenerateTest) {

    FetchAddressGenerateConfiguration_struct aCfg("FetchAddressGenerateTest config");
	InitializeFetchAddressGenerateConfiguration(aCfg,2,1,false, false, false, false, false, false, 0, 1,512,4);

    FetchAddressGenerateJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "FAG index and widths defined\n";
	// Step 5: Instantiate the DUT
	nFetchAddressGenerate::FetchAddressGenerateComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "FetchAddressGenerateComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "FetchAddressGenerateComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nFetchAddressGenerate;

    // Initialize the component
    dut.initialize();

// Input Initializations
/*
struct BranchFeedback : boost::counted_base {
  VirtualMemoryAddress thePC;
  eBranchType theActualType;
  eDirection theActualDirection;
  VirtualMemoryAddress theActualTarget;
  boost::intrusive_ptr<BPredState> theBPState;
};
*/
    // Declare and initialize input
    boost::intrusive_ptr<BranchFeedback> dummyFeedback(new BranchFeedback());
// Pushes to the FAG
    dut.push(FetchAddressGenerateInterface::BranchFeedbackIn(), 0, dummyFeedback);
    std::cout<<"dut push done\n";

//Drive The FAG
	FetchAddressGenerateInterface::FAGDrive drive_temp;
	dut.drive(drive_temp);

// TESTS: Perform assertions based on the expected behavior

    // ASSERT_EQ(expectedTraceInModernResult, pcAndTypeAndAnnulPair);

    // Finalize the component
    dut.finalize();
}
