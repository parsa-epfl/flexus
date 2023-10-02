#include <gtest/gtest.h>

#include <components/CommonQEMU/BranchPredictor.hpp>
#include <components/CommonQEMU/Transports/InstructionTransport.hpp>
#include <components/uFetch/uFetchTypes.hpp>


#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
// Test fixture for the BPWarm component


// TLBHitTest

TEST_F(BPWarmTestFixture, BPWarmTest) {

    BPWarmConfiguration_struct aCfg("BPWarmTests config");
	InitializeBPWarmConfiguration(aCfg,1,0,3,512,4);


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

	std::cout << "BPWarmComponent initialized\n";

	using namespace Flexus;
	using namespace Core;
	using namespace SharedTypes;
	using namespace nBPWarm;

    // Initialize the component
    dut.initialize();

// Input Initializations

    FILE *inputFile;
    inputFile = fopen("/home/ninadjangle/qflex-8.0/BPWarmTrace.txt", "r");
    if (inputFile == NULL) {
        assert(0);
    }
    char line[100]; // Use a char array to store the line
    while (fgets(line, sizeof(line), inputFile) != NULL) {        uint64_t PC, branchTarget; // Use uint64_t for PC and branchTarget
        char opType[100]; // Use a char array to store opType
        int resolveDir, predDir;
        eBranchType optype;
        // Parse the input line
        sscanf(line, "v:%lx %s %d %d v:%lx", &PC, opType, &predDir, &resolveDir, &branchTarget);
        // printf("%s \n",opType);
            if(strcmp(opType ,  "NonBranch") == 0){
                optype = kNonBranch;    
            } else if (strcmp(opType , "Conditional")==0){
                optype = kConditional;
            } else if (strcmp(opType , "Unconditional")==0){
                optype = kUnconditional;
            } else if (!strcmp(opType, "Call")){
                optype = kCall;
            } else if (!strcmp(opType, "IndirectRegister")){
                optype = kIndirectReg;
            } else if (!strcmp(opType, "IndirectCall")){
                optype = kIndirectCall;
            } else if (!strcmp(opType,"Return")){
                optype = kReturn;
            } else{
                optype = kLastBranchType;
            }
 			MemoryMessage theMemoryMessage(MemoryMessage::LoadReq);
            // printf("opType: %s\n", opType);

			theMemoryMessage.pc() = VirtualMemoryAddress(PC);
			theMemoryMessage.branchType() = optype;
			theMemoryMessage.targetpc() = VirtualMemoryAddress(branchTarget);

// Pushes to the BPWarm
    // Initialize the instruction as needed
	dut.push(BPWarmInterface::ITraceInModern(),0,theMemoryMessage);
    // dut.push(BPWarmInterface::ITraceInModern(), 0, pcAndTypeAndAnnulPair);
    std::cout<<"dut push done\n";

// TESTS: Perform assertions based on the expected behavior

    // Example assertion for the expected behavior of InsnOut port
    // EXPECT_TRUE(dut.available(BPWarmInterface::InsnOut())); // Check if the port is available
    // Assert that the expected trace input has been processed correctly
    // Finalize the component
}
    fclose(inputFile);
    dut.finalize();

}