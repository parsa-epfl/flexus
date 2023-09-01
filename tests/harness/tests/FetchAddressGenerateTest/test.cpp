#include <components/FetchAddressGenerate_Boom/FetchAddressGenerateImpl.cpp>
#include<components/MTManager/MTManagerComponentImpl.cpp>
#include <core/flexus.hpp>
#include <gtest/gtest.h>
#include <DummyQemu.h>
// Create fixture for testing the DUT
class FetchAddressGenerateTestFixture : public testing::Test {
  
public:

  static void InitializeFetchAddressGenerateConfiguration(FetchAddressGenerateConfiguration_struct& aCfg, uint32_t MaxBPred,uint32_t Threads,bool EnableRAS, bool EnableTCE, bool EnableTrapRet, bool EnableBTBPrefill, bool MagicBTypeDetect ,bool PerfectBTB , int  BlocksOnBTBMiss, int InsnOnBTBMiss, uint32_t BTBSets, uint32_t BTBWays) {
    aCfg.MaxBPred = MaxBPred;
    aCfg.Threads = Threads;
    aCfg.EnableRAS = EnableRAS;
    aCfg.EnableTCE = EnableTCE;
    aCfg.EnableTrapRet = EnableTrapRet;

    aCfg.EnableBTBPrefill = EnableBTBPrefill;
    aCfg.MagicBTypeDetect = MagicBTypeDetect;
    aCfg.PerfectBTB = PerfectBTB;
    aCfg.BlocksOnBTBMiss = BlocksOnBTBMiss;
    aCfg.InsnOnBTBMiss = InsnOnBTBMiss;

    aCfg.BTBSets = BTBSets;
    aCfg.BTBWays = BTBWays;
	  std::cout << "FetchAddressGenerateConfiguration_struct defined\n";
  }

  static void InitializeJumpTable(FetchAddressGenerateJumpTable& aJumpTable) {
    aJumpTable.wire_available_BTBRequestOut = func_wire_available_BTBRequestOut;
    aJumpTable.wire_manip_BTBRequestOut = func_wire_manip_BTBRequestOut;

    aJumpTable.wire_available_uArchHalted = func_wire_available_uArchHalted;
    aJumpTable.wire_manip_uArchHalted = func_wire_manip_uArchHalted;

    aJumpTable.wire_available_PrefetchAddrOut = func_wire_available_PrefetchAddrOut;
    aJumpTable.wire_manip_PrefetchAddrOut = func_wire_manip_PrefetchAddrOut;

    aJumpTable.wire_available_RecordedMissOut = func_wire_available_RecordedMissOut;
    aJumpTable.wire_manip_RecordedMissOut = func_wire_manip_RecordedMissOut;

    aJumpTable.wire_available_FetchAddrOut = func_wire_available_FetchAddrOut;
    aJumpTable.wire_manip_FetchAddrOut = func_wire_manip_FetchAddrOut;


    aJumpTable.wire_available_AvailableFAQ = func_wire_available_AvailableFAQ;
    aJumpTable.wire_manip_AvailableFAQ = func_wire_manip_AvailableFAQ;
    std::cout << "FetchAddressGenerateJumpTable defined\n";
    }  
// WIRE CALLBACK FUNCTIONS
    static bool func_wire_available_FetchAddrOut(Flexus::Core::index_t anIndex) {
        // Replace with your implementation
        return true; // Return availability status
    }

    static void func_wire_manip_FetchAddrOut(Flexus::Core::index_t anIndex, FetchAddressGenerateInterface::FetchAddrOut::payload &aPayload) {
        // Replace with your implementation
        std::cout << "Manipulating FetchAddrOut for index " << anIndex << std::endl;
        // Modify the aPayload based on your logic
    }

    static bool func_wire_available_AvailableFAQ(Flexus::Core::index_t anIndex) {
        // Replace with your implementation
        return true; // Return availability status
    }

    static void func_wire_manip_AvailableFAQ(Flexus::Core::index_t anIndex, FetchAddressGenerateInterface::AvailableFAQ::payload &aPayload) {
        // Replace with your implementation
        std::cout << "Manipulating AvailableFAQ for index " << anIndex << std::endl;
        // Modify the aPayload based on your logic
    }

    static bool func_wire_available_uArchHalted(Flexus::Core::index_t anIndex) {
        // Replace with your implementation
        return true; // Return availability status
    }

    static void func_wire_manip_uArchHalted(Flexus::Core::index_t anIndex, FetchAddressGenerateInterface::uArchHalted::payload &aPayload) {
        // Replace with your implementation
        std::cout << "Manipulating uArchHalted for index " << anIndex << std::endl;
        // Modify the aPayload based on your logic
    }

// Added for Boomerang

    static bool func_wire_available_BTBRequestOut(Flexus::Core::index_t anIndex) {
        // Replace with your implementation
        return true; // Return availability status
    }

    static void func_wire_manip_BTBRequestOut(Flexus::Core::index_t anIndex, VirtualMemoryAddress &aPayload) {
        // Replace with your implementation
        std::cout << "Manipulating func_wire_manip_BTBRequestOut for index " << anIndex << std::endl;
        // Modify the aPayload based on your logic
    }

    static bool func_wire_available_PrefetchAddrOut(Flexus::Core::index_t anIndex) {
        // Replace with your implementation
        return true; // Return availability status
    }

    static void func_wire_manip_PrefetchAddrOut(Flexus::Core::index_t anIndex, boost::intrusive_ptr<FetchCommand> &aPayload) {
        // Replace with your implementation
        std::cout << "Manipulating func_wire_manip_PrefetchAddrOut for index " << anIndex << std::endl;
        // Modify the aPayload based on your logic
    }

    static bool func_wire_available_RecordedMissOut(Flexus::Core::index_t anIndex) {
        // Replace with your implementation
        return true; // Return availability status
    }

    static void func_wire_manip_RecordedMissOut(Flexus::Core::index_t anIndex, boost::intrusive_ptr<RecordedMisses> &aPayload) {
        // Replace with your implementation
        std::cout << "Manipulating func_wire_manip_RecordedMissOut for index " << anIndex << std::endl;
        // Modify the aPayload based on your logic
    }
protected:
  static void SetUpTestCase() {

    // Create Flexus base
    Flexus::Core::CreateFlexusObject();
    // Allocate memory for hooks_from_qemu
    Flexus::Qemu::API::QFLEX_TO_QEMU_API_t *hooks_from_qemu = new Flexus::Qemu::API::QFLEX_TO_QEMU_API_t;
    // Create a DummyQemu Object
    // initialize the DUMMYQemu
    DummyQemu dummyQemuObj(3);

    // Initialize hooks_from_qemu with the desired function pointer
    dummyQemuObj.initialize(3);
    hooks_from_qemu->QEMU_get_cpu_by_index = dummyQemuObj.DummyQEMU_get_cpu_by_index;
    hooks_from_qemu->QEMU_get_cpu_index = dummyQemuObj.DummyQEMU_get_cpu_index;
    hooks_from_qemu->QEMU_get_program_counter = dummyQemuObj.DummyQEMU_get_program_counter;
    QFLEX_API_set_Interface_Hooks(hooks_from_qemu);

  }

  static void TearDownTestCase() {

  }

  void SetUp() {
  }
  void TearDown() {
  }
};



#include "FetchAddressGenerateTests.h"
