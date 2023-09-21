#include <components/FastCache/FastCacheImpl.cpp>
#include <gtest/gtest.h>
#include <core/qemu/qflex-api.h>
// #include <hw/core/cpu.h>
#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/mai_api.hpp>
#include <DummyQemu.hpp>

class FastCacheTestFixture : public testing::Test {
public:
  static TranslationPtr payload;

// Initialization Functions

  static void InitializeFastCacheConfiguration(FastCacheConfiguration_struct& aCfg,
  int MTWidth, int Size, int Associativity, int BlockSize,
  bool CleanEvictions, Flexus::SharedTypes::tFillLevel CacheLevel, bool NotifyReads, bool NotifyWrites,
  bool TraceTracker , int RegionSize , int RTAssoc, int RTSize,
  std::string RTReplPolicy, int ERBSize, bool StdArray, bool BlockScout,
  bool SkewBlockSet, std::string Protocol, bool UsingTraces, bool TextFlexpoints,
  bool GZipFlexpoints, bool DowngradeLRU, bool SnoopLRU ) {
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

    std::cout << "FastCacheConfiguration_struct defined\n";

  }

  static void InitializeTLBRequest(Flexus::SharedTypes::TranslationPtr& tlbRequest, unsigned int Vaddr,unsigned int Paddr){
    // Create an instance of the TLB request and set its attributes
	tlbRequest->theVaddr = VirtualMemoryAddress(Vaddr);
	tlbRequest->thePaddr = PhysicalMemoryAddress(Paddr);
	tlbRequest->theTLBtype = Flexus::SharedTypes::Translation::kINST;
  tlbRequest->isInstr();
	std::cout <<"Vaddr"<< tlbRequest->theVaddr << " Paddr"<< tlbRequest->thePaddr<< " TLB instance made\n";
  }

  static void InitializeJumpTable(FastCacheJumpTable& aJumpTable) {
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

       	std::cout << "FastCacheJumpTable defined\n";

    }
// Wire Callback Functions
    static bool func_wire_available_RequestOut(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_RequestOut called\n";
        return true;
    }

    static void func_wire_manip_RequestOut(Flexus::Core::index_t idx, MemoryMessage& p) {
        std::cout << "wire_manip_RequestOut called \n";
    }

    static bool func_wire_available_SnoopOutI(Flexus::Core::index_t idx) {
        std::cout << "wire_available_SnoopOutI called\n";
        return true;
    }

    static void func_wire_manip_SnoopOutI(Flexus::Core::index_t idx, MemoryMessage& p) {
        std::cout << "wire_manip_SnoopOutI called with DTR\n";
    }

    static bool func_wire_available_SnoopOutD(Flexus::Core::index_t idx) {
        std::cout << "wire_available_SnoopOutD called\n";
        return true;
    }

    static void func_wire_manip_SnoopOutD(Flexus::Core::index_t idx, MemoryMessage& p) {
        std::cout << "wire_manip_SnoopOutD \n";
	}

    static bool func_wire_available_Reads(Flexus::Core::index_t idx) {
        std::cout << "wire_available_Reads called\n";
        return true;
    }

    static void func_wire_manip_Reads(Flexus::Core::index_t idx, MemoryMessage& p) {
        std::cout << "wire_available_Reads called\n";
    }

    static bool func_wire_available_Writes(Flexus::Core::index_t idx) {
        std::cout << "wire_available_Writes called with RO\n";
        return true;
    }

    static void func_wire_manip_Writes(Flexus::Core::index_t idx, MemoryMessage& p) {
        std::cout << "wire_manip_Writes called\n";
    }

    static bool func_wire_available_RegionNotify(Flexus::Core::index_t idx) {
        std::cout << "wire_available_RegionNotify called\n";
        return true;
    }

    static void func_wire_manip_RegionNotify(Flexus::Core::index_t idx, RegionScoutMessage& p) {
        std::cout << "func_wire_manip_RegionNotify called with DTR\n";
    }


    // static void setPayloadp(TranslationPtr& p){
    //     uFetchTestFixture::payload = p;
    // }

    // static TranslationPtr getPayloadp(){
    //   return uFetchTestFixture::payload;
    // }

protected:
  static void SetUpTestCase() {
    // Create Flexus base
    Flexus::Core::CreateFlexusObject();

    // Allocate memory for hooks_from_qemu
    Flexus::Qemu::API::QFLEX_TO_QEMU_API_t *hooks_from_qemu = new Flexus::Qemu::API::QFLEX_TO_QEMU_API_t;

    DummyQemu dummyQemuObj(3);

    // Initialize hooks_from_qemu with the desired function pointer
    dummyQemuObj.initialize(3);
    hooks_from_qemu->QEMU_get_cpu_by_index = dummyQemuObj.DummyQEMU_get_cpu_by_index;
    hooks_from_qemu->QEMU_get_cpu_index = dummyQemuObj.DummyQEMU_get_cpu_index;
    hooks_from_qemu->QEMU_logical_to_physical = dummyQemuObj.DummyQEMU_logical_to_physical;
    hooks_from_qemu->QEMU_read_sctlr = dummyQemuObj.DummyQEMU_read_sctlr;
    hooks_from_qemu->QEMU_read_register = dummyQemuObj.DummyQEMU_read_register;
    hooks_from_qemu->QEMU_read_phys_memory = dummyQemuObj.DummyQEMU_read_phys_memory;
    printf("hooks_from_qemu: %p\n", hooks_from_qemu);
    QFLEX_API_set_Interface_Hooks(hooks_from_qemu);
  }

  static void TearDownTestCase() {
    // Deallocate memory for qemu_cpus
    // free(qemu_cpus);
    // // Deallocate memory for cpu_states
    // free(cpu_states);
    // std::cout << "TearDownTestCase called\n";

  }

  void SetUp() {
  }
  void TearDown() {
  }
};


#include "FastCacheTests.h"
