#include <components/FastCache/FastCacheImpl.cpp>
#include <gtest/gtest.h>
#include <core/types.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include "FastCacheFixture.hpp"
#include "DummyQemu.hpp"
#include <core/stats.hpp>
#include "StatStreamParser.h"

#include <iterator>
#include <memory>
#include <ostream>
#include <vector>


/**
 * The test file include the implemention then the test bench in that order,
 * WITH the implementation imported for the exact reason that so far a
 * linking error could not be avoided "any" other way.
 */


// Test fixture for the FastCache component

FastCacheTestFixture::FastCacheTestFixture() {
}

void FastCacheTestFixture::SetUp() {
  // Create Flexus base
  Flexus::Core::CreateFlexusObject();

  // Allocate memory for hooks_from_qemu
  auto hooks_from_qemu = std::unique_ptr<Flexus::Qemu::API::QFLEX_TO_QEMU_API_t>(
      new Flexus::Qemu::API::QFLEX_TO_QEMU_API_t);

  DummyQemu dummyQemuObj(3);

  // Initialize hooks_from_qemu with the desired function pointer
  hooks_from_qemu->QEMU_get_cpu_by_index = dummyQemuObj.DummyQEMU_get_cpu_by_index;
  hooks_from_qemu->QEMU_get_cpu_index = dummyQemuObj.DummyQEMU_get_cpu_index;
  hooks_from_qemu->QEMU_logical_to_physical = dummyQemuObj.DummyQEMU_logical_to_physical;
  hooks_from_qemu->QEMU_read_sctlr = dummyQemuObj.DummyQEMU_read_sctlr;
  hooks_from_qemu->QEMU_read_register = dummyQemuObj.DummyQEMU_read_register;
  hooks_from_qemu->QEMU_read_phys_memory = dummyQemuObj.DummyQEMU_read_phys_memory;
  QFLEX_API_set_Interface_Hooks(hooks_from_qemu.get());
}

void FastCacheTestFixture::TearDown() {
}

void FastCacheTestFixture::InitializeFastCacheConfiguration(
    FastCacheConfiguration_struct &aCfg, int MTWidth, int Size, int Associativity, int BlockSize,
    bool CleanEvictions, Flexus::SharedTypes::tFillLevel CacheLevel, bool NotifyReads,
    bool NotifyWrites, bool TraceTracker, int RegionSize, int RTAssoc, int RTSize,
    std::string RTReplPolicy, int ERBSize, bool StdArray, bool BlockScout, bool SkewBlockSet,
    std::string Protocol, bool UsingTraces, bool TextFlexpoints, bool GZipFlexpoints,
    bool DowngradeLRU, bool SnoopLRU) {

  aCfg.MTWidth = MTWidth;
  aCfg.Size = Size;
  aCfg.Associativity = Associativity;
  aCfg.BlockSize = BlockSize;

  aCfg.CleanEvictions = CleanEvictions;
  aCfg.CacheLevel = CacheLevel;
  aCfg.NotifyReads = NotifyReads;
  aCfg.NotifyWrites = NotifyWrites;

  aCfg.TraceTracker = TraceTracker;
  aCfg.RegionSize = RegionSize;
  aCfg.RTAssoc = RTAssoc;
  aCfg.RTSize = RTSize;

  aCfg.RTReplPolicy = RTReplPolicy;
  aCfg.ERBSize = ERBSize;
  aCfg.StdArray = StdArray;
  aCfg.BlockScout = BlockScout;

  aCfg.SkewBlockSet = SkewBlockSet;
  aCfg.Protocol = Protocol;
  aCfg.UsingTraces = UsingTraces;
  aCfg.TextFlexpoints = TextFlexpoints;

  aCfg.GZipFlexpoints = GZipFlexpoints;
  aCfg.DowngradeLRU = DowngradeLRU;
  aCfg.SnoopLRU = SnoopLRU;
}

void FastCacheTestFixture::InitializeTLBRequest(Flexus::SharedTypes::TranslationPtr &tlbRequest,
                                                unsigned int Vaddr, unsigned int Paddr) {
  tlbRequest->theVaddr = Flexus::SharedTypes::VirtualMemoryAddress(Vaddr);
  tlbRequest->thePaddr = Flexus::SharedTypes::PhysicalMemoryAddress(Paddr);
  tlbRequest->theTLBtype = Flexus::SharedTypes::Translation::kINST;
  tlbRequest->isInstr();
}
void FastCacheTestFixture::InitializeJumpTable(FastCacheJumpTable &aJumpTable) {
  aJumpTable.wire_available_RequestOut = func_wire_available_RequestOut;
  aJumpTable.wire_manip_RequestOut = func_wire_manip_RequestOut;

  aJumpTable.wire_available_SnoopOutI = func_wire_available_SnoopOutI;
  aJumpTable.wire_manip_SnoopOutI = func_wire_manip_SnoopOutI;

  aJumpTable.wire_available_SnoopOutD = func_wire_available_SnoopOutD;
  aJumpTable.wire_manip_SnoopOutD = func_wire_manip_SnoopOutD;

  aJumpTable.wire_available_Reads = func_wire_available_Reads;
  aJumpTable.wire_manip_Reads = func_wire_manip_Reads;

  aJumpTable.wire_available_Writes = func_wire_available_Writes;
  aJumpTable.wire_manip_Writes = func_wire_manip_Writes;

  aJumpTable.wire_available_RegionNotify = func_wire_available_RegionNotify;
  aJumpTable.wire_manip_RegionNotify = func_wire_manip_RegionNotify;
}

bool FastCacheTestFixture::func_wire_available_RequestOut(Flexus::Core::index_t idx) {
  return true;
}
void FastCacheTestFixture::func_wire_manip_RequestOut(Flexus::Core::index_t idx, MemoryMessage &p) {
  p.type() = MemoryMessage::MissReplyWritable;
}
bool FastCacheTestFixture::func_wire_available_SnoopOutI(Flexus::Core::index_t idx) {
  return true;
}
void FastCacheTestFixture::func_wire_manip_SnoopOutI(Flexus::Core::index_t idx, MemoryMessage &p) {
}
bool FastCacheTestFixture::func_wire_available_SnoopOutD(Flexus::Core::index_t idx) {
  return true;
}
void FastCacheTestFixture::func_wire_manip_SnoopOutD(Flexus::Core::index_t idx, MemoryMessage &p) {
}
bool FastCacheTestFixture::func_wire_available_Reads(Flexus::Core::index_t idx) {
  return true;
}
void FastCacheTestFixture::func_wire_manip_Reads(Flexus::Core::index_t idx, MemoryMessage &p) {
}
bool FastCacheTestFixture::func_wire_available_Writes(Flexus::Core::index_t idx) {
  return true;
}
void FastCacheTestFixture::func_wire_manip_Writes(Flexus::Core::index_t idx, MemoryMessage &p) {
}
bool FastCacheTestFixture::func_wire_available_RegionNotify(Flexus::Core::index_t idx) {
  return true;
}
void FastCacheTestFixture::func_wire_manip_RegionNotify(Flexus::Core::index_t idx,
                                                        RegionScoutMessage &p) {
}




// @see
// https://github.com/google/googletest/blob/main/docs/primer.md#test-fixtures-using-the-same-data-configuration-for-multiple-tests-same-data-multiple-tests
//  TEST 1. First Access always miss
TEST_F(FastCacheTestFixture, FastCacheTest) {

  FastCacheConfiguration_struct aCfg("FastCacheTest config");
  InitializeFastCacheConfiguration(aCfg, 1, 65536, 2, 64, false, Flexus::SharedTypes::eUnknown,
                                   false, false, false, 1024, 16, 8192, "SetLRU", 8, false, false,
                                   false, "InclusiveMESI", true, false, true, false, false);

  FastCacheJumpTable aJumpTable;
  InitializeJumpTable(aJumpTable);

  // Step 4: Specify the Index and Width
  Flexus::Core::index_t anIndex = 1;
  Flexus::Core::index_t aWidth  = 1;

  std::cout << "FastCache index and widths defined\n";
  // Step 5: Instantiate the DUT

  nFastCache::FastCacheComponent dut(aCfg, aJumpTable, anIndex, aWidth);
  std::cout << "FastCacheComponent instantiated\n";

  using namespace Flexus::Core;
//  using namespace Flexus::SharedTypes;
  // using namespace nFastCache;

  // Initialize the component

  Flexus::Stat::getStatManager()->initialize();
  dut.initialize();

  std::cout << "FastCacheComponent initialized\n";

  MemoryMessage mock_mess(MemoryMessage::MemoryMessageType::LoadReq);
  mock_mess.address() = PhysicalMemoryAddress(0xc0ffee);
  mock_mess.pc() = VirtualMemoryAddress(0x00dead);
  mock_mess.coreIdx() = 0xc0ffee;

  dut.push(FastCacheInterface::FetchRequestIn(), 0, mock_mess);
  dut.finalize();

  std::stringstream ss;
  Flexus::Stat::getStatManager()->printMeasurement("all", ss);

  StatStreamHelper helper(ss);

  int value;
  bool ret_val = helper.get_entry_by_name("sys-FastCacheTest config-Misses:Read:Invalid", value);

  EXPECT_EQ(ret_val, true);
  ASSERT_EQ(value, 1);
}

//  TEST 2. Fill first, then HIT
//  TEST 3. FILL sov, fill associative, and observe eviction
//  TEST 4. Verify total cache size
//  TEST 5. LLC Sharing
//  TEST 6. Coherence Test