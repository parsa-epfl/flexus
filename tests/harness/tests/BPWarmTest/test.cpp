#include <components/BPWarm/BPWarmImpl.cpp>
#include <core/flexus.hpp>
#include <gtest/gtest.h>

// Create fixture for testing the DUT
class BPWarmTestFixture : public testing::Test {
  
public:

  static void InitializeBPWarmConfiguration(BPWarmConfiguration_struct& aCfg, int Cores, int UnresolvedBranches, int RunaheadDistance, uint32_t BTBSets, uint32_t BTBWays) {
    aCfg.Cores = Cores;
    aCfg.BTBSets = BTBSets;
    aCfg.UnresolvedBranches = UnresolvedBranches;
    aCfg.RunaheadDistance = RunaheadDistance;
    aCfg.BTBWays = BTBWays;

	  std::cout << "BPWarmConfiguration_struct defined\n";
  }

  static void InitializeJumpTable(BPWarmJumpTable& aJumpTable) {
    // aJumpTable.wire_available_InsnOut = func_wire_available_InsnOut;
    // aJumpTable.wire_manip_InsnOut = func_wire_manip_InsnOut;

    std::cout << "BPWarmJumpTable defined\n";
    }  
// WIRE CALLBACK FUNCTIONS
    // static bool func_wire_available_InsnOut(Flexus::Core::index_t anIndex) {
    //     std::cout << "func_wire_available_InsnOut called\n";
    //     return true;
    // }

    // static void func_wire_manip_InsnOut(Flexus::Core::index_t anIndex, BPWarmJumpTable::iface::InsnOut::payload & aPayload) {
    //     std::cout << "func_wire_manip_InsnOut\n";
    // }
protected:
  static void SetUpTestCase() {

    // Create Flexus base
    Flexus::Core::CreateFlexusObject();


  }

  static void TearDownTestCase() {

  }

  void SetUp() {
  }
  void TearDown() {
  }
};



#include "BPWarmTests.h"

