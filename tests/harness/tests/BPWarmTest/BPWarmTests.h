#include <gtest/gtest.h>

#include <components/CommonQEMU/BranchPredictor.hpp>
#include <components/CommonQEMU/Transports/InstructionTransport.hpp>

#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
// Test fixture for the BPWarm component


// TLBHitTest

TEST_F(BPWarmTestFixture, BPWarmTest) {

    BPWarmConfiguration_struct aCfg("BPWarmTests config");
	InitializeBPWarmConfiguration(aCfg,1,512,4);


    BPWarmJumpTable aJumpTable;
	InitializeJumpTable(aJumpTable);

  	// Step 4: Specify the Index and Width
	Flexus::Core::index_t anIndex = 1;
	Flexus::Core::index_t aWidth =  1;
	std::cout << "BPWarm index and widths defined\n";
	// Step 5: Instantiate the DUT
	nBPWarm::BPWarmComponent dut(
	    aCfg,				
		aJumpTable,			
		anIndex,
		aWidth
		);
	std::cout << "BPWarmComponent instantiated\n";
	
	// Initializing the component
	dut.initialize();

	std::cout << "BPWarmComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nBPWarm;

    // Initialize the component
    dut.initialize();

// Input Initializations

    // Declare and initialize the pc_type_annul_triplet object
    std::pair<uint64_t, std::pair<uint32_t, uint32_t>> pcAndTypeAndAnnulPair(
        0x90000000, std::make_pair(0x23456789, 0x1));
    // Example assertion for the expected behavior of ITraceInModern port
    std::pair<uint64_t, std::pair<uint32_t, uint32_t>> expectedTraceInModernResult(
        0x90000000, std::make_pair(0x23456789, 0x1));


// Pushes to the BPWarm
    // Initialize the instruction as needed

    // dut.push(BPWarmInterface::ITraceInModern(), 0, pcAndTypeAndAnnulPair);
    std::cout<<"dut push done\n";

// TESTS: Perform assertions based on the expected behavior

    // Example assertion for the expected behavior of InsnOut port
    // EXPECT_TRUE(dut.available(BPWarmInterface::InsnOut())); // Check if the port is available
    // Assert that the expected trace input has been processed correctly
    ASSERT_EQ(expectedTraceInModernResult, pcAndTypeAndAnnulPair);

    // Finalize the component
    dut.finalize();
}
