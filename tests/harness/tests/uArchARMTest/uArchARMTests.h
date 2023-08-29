#include <gtest/gtest.h>

#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/MTManager/MTManager.hpp>


// Test fixture for the uArchARM component


TEST_F(uArchARMTestFixture, uArchARMTest) {

    uArchARMConfiguration_struct aCfg("uArchARMTest config");
	InitializeuArchARMConfiguration(aCfg,2556,256,false, false, 8,4,1,16,false,0,64,false,true,false,false,false,0,0,false,false, false, false, 0 , 0 , false,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 );

    uArchARMJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "uArchARM index and widths defined\n";
	// Step 5: Instantiate the DUT
	nuArchARM::uArchARMComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "uArchARMComponent instantiated\n";



	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nuArchARM;

    // Initialize the component
    dut.initialize();
	std::cout << "uArchARMComponent initialized\n";

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
//     // Declare and initialize input
//     boost::intrusive_ptr<BranchFeedback> dummyFeedback(new BranchFeedback());
// // Pushes to the uArchARM
//     dut.push(uArchARMInterface::BranchFeedbackIn(), 0, dummyFeedback);
//     std::cout<<"dut push done\n";

// //Drive The uArchARM
// 	uArchARMInterface::uArchARMDrive drive_temp;
// 	dut.drive(drive_temp);

// TESTS: Perform assertions based on the expected behavior

    // ASSERT_EQ(expectedTraceInModernResult, pcAndTypeAndAnnulPair);

    // Finalize the component
    dut.finalize();
}
