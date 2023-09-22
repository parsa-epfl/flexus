#include <components/uFetch_Boom/uFetchImpl.cpp>
#include <gtest/gtest.h>
#include <core/qemu/qflex-api.h>
// #include <hw/core/cpu.h>
#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/mai_api.hpp>
#include <DummyQemu.hpp>

class uFetchTestFixture : public testing::Test {
public:
  static TranslationPtr payload;

// Initialization Functions

  static void InitializeuFetchConfiguration(uFetchConfiguration_struct& aCfg, uint32_t FAQSize, uint32_t MaxFetchLines,
   uint32_t MaxFetchInstructions, uint64_t ICacheLineSize, bool PerfectICache,bool PrefetchEnabled, bool FDIPEnabled, bool RecMissEnabled, bool CleanEvict,int Size,
   int Associativity, uint32_t MissQueueSize, uint32_t OutstandingFDIPMisses, uint32_t Threads, bool SendAcks, bool UseReplyChannel, bool  EvictOnSnoop) {
    aCfg.FAQSize = FAQSize;
    aCfg.MaxFetchLines = MaxFetchLines;
    aCfg.MaxFetchInstructions = MaxFetchInstructions;
    aCfg.ICacheLineSize = ICacheLineSize;

    aCfg.PerfectICache = PerfectICache;
    aCfg.PrefetchEnabled = PrefetchEnabled;
    aCfg.FDIPEnabled = FDIPEnabled;
    aCfg.RecMissEnabled = RecMissEnabled;

    aCfg.CleanEvict = CleanEvict;
    aCfg.Size = Size;
    aCfg.Associativity = Associativity;
    aCfg.MissQueueSize = MissQueueSize;

    aCfg.OutstandingFDIPMisses = OutstandingFDIPMisses;
    aCfg.Threads = Threads;
    aCfg.SendAcks = SendAcks;
    aCfg.UseReplyChannel = UseReplyChannel;

    aCfg.EvictOnSnoop = EvictOnSnoop;

    std::cout << "uFetchConfiguration_struct defined\n";

  }

  static void InitializeRequest(Flexus::SharedTypes::TranslationPtr& tlbRequest, unsigned int Vaddr,unsigned int Paddr){
    // Create an instance of the TLB request and set its attributes
	tlbRequest->theVaddr = VirtualMemoryAddress(Vaddr);
	tlbRequest->thePaddr = PhysicalMemoryAddress(Paddr);
	tlbRequest->theTLBtype = Flexus::SharedTypes::Translation::kINST;
  tlbRequest->isInstr();
	std::cout <<"Vaddr"<< tlbRequest->theVaddr << " Paddr"<< tlbRequest->thePaddr<< " TLB instance made\n";
  }

  static void InitializeJumpTable(uFetchJumpTable& aJumpTable) {
        aJumpTable.wire_available_iTranslationOut = func_wire_available_iTranslationOut;
        aJumpTable.wire_manip_iTranslationOut = func_wire_manip_iTranslationOut;

        aJumpTable.wire_available_FetchBundleOut = func_wire_available_FetchBundleOut;
        aJumpTable.wire_manip_FetchBundleOut = func_wire_manip_FetchBundleOut;

        aJumpTable.wire_available_InstructionFetchSeen = func_wire_available_InstructionFetchSeen;
        aJumpTable.wire_manip_InstructionFetchSeen = func_wire_manip_InstructionFetchSeen;

        aJumpTable.wire_available_AvailableFIQ = func_wire_available_AvailableFIQ;
        aJumpTable.wire_manip_AvailableFIQ = func_wire_manip_AvailableFIQ;

        aJumpTable.wire_available_FetchReplyOut = func_wire_available_FetchReplyOut;
        aJumpTable.wire_manip_FetchReplyOut = func_wire_manip_FetchReplyOut;

        aJumpTable.wire_available_FetchSnoopOut = func_wire_available_FetchSnoopOut;
        aJumpTable.wire_manip_FetchSnoopOut = func_wire_manip_FetchSnoopOut;

        aJumpTable.wire_available_FetchMissOut = func_wire_available_FetchMissOut;
        aJumpTable.wire_manip_FetchMissOut = func_wire_manip_FetchMissOut;

        aJumpTable.wire_available_ClockTickSeen = func_wire_available_ClockTickSeen;
        aJumpTable.wire_manip_ClockTickSeen = func_wire_manip_ClockTickSeen;

       	std::cout << "uFetchJumpTable defined\n";

    }
// Wire Callback Functions
    static bool func_wire_available_AvailableFIQ(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_AvailableFIQ called\n";
        return true;
    }

    static void func_wire_manip_AvailableFIQ(Flexus::Core::index_t idx, decode_status& p) {
        std::cout << "func_wire_manip_AvailableFIQ called \n";
    }

    static bool func_wire_available_InstructionFetchSeen(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_InstructionFetchSeen called\n";
        return true;
    }

    static void func_wire_manip_InstructionFetchSeen(Flexus::Core::index_t idx, bool& p) {
        std::cout << "func_wire_manip_InstructionFetchSeen called with DTR\n";
    }

    static bool func_wire_available_FetchBundleOut(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_FetchBundleOut called\n";
        return true;
    }

    static void func_wire_manip_FetchBundleOut(Flexus::Core::index_t idx, pFetchBundle& p) {
        std::cout << "func_wire_manip_FetchBundleOut \n";
	}

    static bool func_wire_available_iTranslationOut(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_iTranslationOut called\n";
        return true;
    }

    static void func_wire_manip_iTranslationOut(Flexus::Core::index_t idx, TranslationPtr& p) {
        std::cout << "func_wire_manip_iTranslationOut called with RO\n";
    }

    static bool func_wire_available_ClockTickSeen(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_ClockTickSeen called\n";
        return true;
    }

    static void func_wire_manip_ClockTickSeen(Flexus::Core::index_t idx, bool& p) {
        std::cout << "func_wire_manip_ClockTickSeen called with ITR\n";
    }
        // std::cout << p->thePaddr  << "\n";
        // setPayloadp(p);

    static bool func_wire_available_FetchMissOut(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_FetchMissOut called\n";
        return true;
    }

    static void func_wire_manip_FetchMissOut(Flexus::Core::index_t idx, MemoryTransport& p) {
        std::cout << "func_wire_manip_FetchMissOut called with DTR\n";
    }

    static bool func_wire_available_FetchSnoopOut(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_FetchSnoopOut called\n";
        return true;
    }

    static void func_wire_manip_FetchSnoopOut(Flexus::Core::index_t idx, MemoryTransport& p) {
        std::cout << "func_wire_manip_FetchSnoopOut called with MRO\n";
		// Assertion to check the payload value is one of the permissible values

        // std::cout << p->thePaddr  << "\n";
        // setPayloadp(p);
	}

    static bool func_wire_available_FetchReplyOut(Flexus::Core::index_t idx) {
        std::cout << "func_wire_available_FetchReplyOut called\n";
        return true;
    }

    static void func_wire_manip_FetchReplyOut(Flexus::Core::index_t idx, MemoryTransport& p) {
        std::cout << "func_wire_manip_FetchReplyOut \n";
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
    hooks_from_qemu->QEMU_get_cpu_by_index    = dummyQemuObj.DummyQEMU_get_cpu_by_index;
    hooks_from_qemu->QEMU_get_cpu_index       = dummyQemuObj.DummyQEMU_get_cpu_index;
    hooks_from_qemu->QEMU_logical_to_physical = dummyQemuObj.DummyQEMU_logical_to_physical;
    hooks_from_qemu->QEMU_read_sctlr          = dummyQemuObj.DummyQEMU_read_sctlr;
    hooks_from_qemu->QEMU_read_register       = dummyQemuObj.DummyQEMU_read_register;
    hooks_from_qemu->QEMU_read_phys_memory    = dummyQemuObj.DummyQEMU_read_phys_memory;
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


#include "uFetchTests.h"
