#include <components/uArchARM/uArchImpl.cpp>
#include <gtest/gtest.h>
#include <core/qemu/qflex-api.h>
// #include <hw/core/cpu.h>
#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>
#include <DummyQemu.hpp>

class uArchARMTestFixture : public testing::Test {
public:
  static TranslationPtr payload;

// Initialization Functions

  static void InitializeuArchARMConfiguration(uArchARMConfiguration_struct& aCfg,
  uint32_t ROBSize, uint32_t SBSize, bool NAWBypassSB, bool NAWWaitAtSync,
  uint32_t RetireWidth, uint32_t MemoryPorts, uint32_t SnoopPorts, uint32_t StorePrefetches,
  bool PrefetchEarly, uint32_t ConsistencyModel, uint32_t CoherenceUnit, bool BreakOnResynchronize,
  bool SpinControl, bool SpeculativeOrder, bool SpeculateOnAtomicValue, bool SpeculateOnAtomicValuePerfect,
  int SpeculativeCheckpoints, int CheckpointThreshold, bool EarlySGP, bool TrackParallelAccesses,
  bool InOrderMemory, bool InOrderExecute, uint32_t OnChipLatency, uint32_t OffChipLatency,
  bool Multithread, uint32_t NumIntAlu, uint32_t IntAluOpLatency, uint32_t IntAluOpPipelineResetTime,
  uint32_t NumIntMult, uint32_t IntMultOpLatency, uint32_t IntMultOpPipelineResetTime, uint32_t IntDivOpLatency,
  uint32_t IntDivOpPipelineResetTime, uint32_t NumFpAlu, uint32_t FpAddOpLatency, uint32_t FpAddOpPipelineResetTime,
  uint32_t FpCmpOpLatency, uint32_t FpCmpOpPipelineResetTime, uint32_t FpCvtOpLatency, uint32_t FpCvtOpPipelineResetTime,
  uint32_t NumFpMult, uint32_t FpMultOpLatency, uint32_t FpMultOpPipelineResetTime, uint32_t FpDivOpLatency,
  uint32_t FpDivOpPipelineResetTime, uint32_t FpSqrtOpLatency, uint32_t FpSqrtOpPipelineResetTime) {
    aCfg.ROBSize = ROBSize;
    aCfg.SBSize = SBSize;
    aCfg.NAWBypassSB = NAWBypassSB;
    aCfg.NAWWaitAtSync = NAWWaitAtSync;

    aCfg.RetireWidth = RetireWidth;
    aCfg.MemoryPorts = MemoryPorts;
    aCfg.SnoopPorts = SnoopPorts;
    aCfg.StorePrefetches = StorePrefetches;

    aCfg.PrefetchEarly = PrefetchEarly;
    aCfg.ConsistencyModel = ConsistencyModel;
    aCfg.CoherenceUnit = CoherenceUnit;
    aCfg.BreakOnResynchronize = BreakOnResynchronize;

    aCfg.SpinControl = SpinControl;
    aCfg.SpeculativeOrder = SpeculativeOrder;
    aCfg.SpeculateOnAtomicValue = SpeculateOnAtomicValue;
    aCfg.SpeculateOnAtomicValuePerfect = SpeculateOnAtomicValuePerfect;

    aCfg.SpeculativeCheckpoints = SpeculativeCheckpoints;
    aCfg.CheckpointThreshold = CheckpointThreshold;
    aCfg.EarlySGP = EarlySGP;
    aCfg.TrackParallelAccesses = TrackParallelAccesses;

    aCfg.InOrderMemory = InOrderMemory;
    aCfg.InOrderExecute = InOrderExecute;
    aCfg.OnChipLatency = OnChipLatency;
    aCfg.OffChipLatency = OffChipLatency;


    aCfg.Multithread = Multithread;
    aCfg.NumIntAlu = NumIntAlu;
    aCfg.IntAluOpLatency = IntAluOpLatency;
    aCfg.IntAluOpPipelineResetTime = IntAluOpPipelineResetTime;

    aCfg.NumIntMult = NumIntMult;
    aCfg.IntMultOpLatency = IntMultOpLatency;
    aCfg.IntMultOpPipelineResetTime = IntMultOpPipelineResetTime;
    aCfg.IntDivOpLatency = IntDivOpLatency;

    aCfg.IntDivOpPipelineResetTime = IntDivOpPipelineResetTime;
    aCfg.NumFpAlu = NumFpAlu;
    aCfg.FpAddOpLatency = FpAddOpLatency;
    aCfg.FpAddOpPipelineResetTime = FpAddOpPipelineResetTime;

    aCfg.FpCmpOpLatency = FpCmpOpLatency;
    aCfg.FpCmpOpPipelineResetTime = FpCmpOpPipelineResetTime;
    aCfg.FpCvtOpLatency = FpCvtOpLatency;
    aCfg.FpCvtOpPipelineResetTime = FpCvtOpPipelineResetTime;

    aCfg.NumFpMult = NumFpMult;
    aCfg.FpMultOpLatency = FpMultOpLatency;
    aCfg.FpMultOpPipelineResetTime = FpMultOpPipelineResetTime;
    aCfg.FpDivOpLatency = FpDivOpLatency;

    aCfg.FpDivOpPipelineResetTime = FpDivOpPipelineResetTime;
    aCfg.FpSqrtOpLatency = FpSqrtOpLatency;
    aCfg.FpSqrtOpPipelineResetTime = FpSqrtOpPipelineResetTime;
    std::cout << "uArchARMConfiguration_struct defined\n";
  }

  static void InitializeRequest(Flexus::SharedTypes::TranslationPtr& tlbRequest, unsigned int Vaddr,unsigned int Paddr){
    // Create an instance of the TLB request and set its attributes
	tlbRequest->theVaddr = VirtualMemoryAddress(Vaddr);
	tlbRequest->thePaddr = PhysicalMemoryAddress(Paddr);
	tlbRequest->theTLBtype = Flexus::SharedTypes::Translation::kINST;
  tlbRequest->isInstr();
	std::cout <<"Vaddr"<< tlbRequest->theVaddr << " Paddr"<< tlbRequest->thePaddr<< " TLB instance made\n";
  }

  static void InitializeJumpTable(uArchARMJumpTable& aJumpTable) {
        aJumpTable.wire_available_SquashOut = func_wire_available_SquashOut;
        aJumpTable.wire_manip_SquashOut = func_wire_manip_SquashOut;

        aJumpTable.wire_available_ResyncOut = func_wire_available_ResyncOut;
        aJumpTable.wire_manip_ResyncOut = func_wire_manip_ResyncOut;

        aJumpTable.wire_available_ChangeCPUState = func_wire_available_ChangeCPUState;
        aJumpTable.wire_manip_ChangeCPUState = func_wire_manip_ChangeCPUState;

        aJumpTable.wire_available_RedirectOut = func_wire_available_RedirectOut;
        aJumpTable.wire_manip_RedirectOut = func_wire_manip_RedirectOut;

        aJumpTable.wire_available_BranchFeedbackOut = func_wire_available_BranchFeedbackOut;
        aJumpTable.wire_manip_BranchFeedbackOut = func_wire_manip_BranchFeedbackOut;

        aJumpTable.wire_available_StoreForwardingHitSeen = func_wire_available_StoreForwardingHitSeen;
        aJumpTable.wire_manip_StoreForwardingHitSeen = func_wire_manip_StoreForwardingHitSeen;

        aJumpTable.wire_available_dTranslationOut = func_wire_available_dTranslationOut;
        aJumpTable.wire_manip_dTranslationOut = func_wire_manip_dTranslationOut;

        aJumpTable.wire_available_MemoryOut_Request = func_wire_available_MemoryOut_Request;
        aJumpTable.wire_manip_MemoryOut_Request = func_wire_manip_MemoryOut_Request;

        aJumpTable.wire_available_MemoryOut_Snoop = func_wire_available_MemoryOut_Snoop;
        aJumpTable.wire_manip_MemoryOut_Snoop = func_wire_manip_MemoryOut_Snoop;


       	std::cout << "uArchARMJumpTable defined\n";

    }
// Wire Callback Functions
    static bool func_wire_available_SquashOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_SquashOut called\n";
        return true;
    }

    static void func_wire_manip_SquashOut(Flexus::Core::index_t idx, eSquashCause& p) {
        std::cout << "wire_manip_SquashOut called \n";
    }

    static bool func_wire_available_ResyncOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_ResyncOut called\n";
        return true;
    }

    static void func_wire_manip_ResyncOut(Flexus::Core::index_t idx, int& p) {
        std::cout << "wire_manip_ResyncOut called with DTR\n";
    }

    static bool func_wire_available_ChangeCPUState(Flexus::Core::index_t idx) {
        std::cout << "wire_available_ChangeCPUState called\n";
        return true;
    }

    static void func_wire_manip_ChangeCPUState(Flexus::Core::index_t idx, CPUState& p) {
        std::cout << "wire_manip_ChangeCPUState \n";
	}

    static bool func_wire_available_RedirectOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_RedirectOut called\n";
        return true;
    }

    static void func_wire_manip_RedirectOut(Flexus::Core::index_t idx, Flexus::SharedTypes::VirtualMemoryAddress& p) {
        std::cout << "wire_manip_RedirectOut called with RO\n";
    }

    static bool func_wire_available_BranchFeedbackOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_BranchFeedbackOut called\n";
        return true;
    }

    static void func_wire_manip_BranchFeedbackOut(Flexus::Core::index_t idx, boost::intrusive_ptr<BranchFeedback>& p) {
        std::cout << "wire_manip_BranchFeedbackOut called with ITR\n";
    }
        // std::cout << p->thePaddr  << "\n";
        // setPayloadp(p);

    static bool func_wire_available_StoreForwardingHitSeen(Flexus::Core::index_t idx) {
        std::cout << "wire_available_StoreForwardingHitSeen called\n";
        return true;
    }

    static void func_wire_manip_StoreForwardingHitSeen(Flexus::Core::index_t idx, bool& p) {
        std::cout << "wire_manip_StoreForwardingHitSeen called with DTR\n";
    }

    static bool func_wire_available_dTranslationOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_dTranslationOut called\n";
        return true;
    }

    static void func_wire_manip_dTranslationOut(Flexus::Core::index_t idx, TranslationPtr& p) {
        std::cout << "wire_manip_dTranslationOut called with MRO\n";
		// Assertion to check the payload value is one of the permissible values

        // std::cout << p->thePaddr  << "\n";
        // setPayloadp(p);
	}

    static bool func_wire_available_MemoryOut_Request(Flexus::Core::index_t idx) {
        std::cout << "wire_available_MemoryOut_Request called\n";
        return true;
    }

    static void func_wire_manip_MemoryOut_Request(Flexus::Core::index_t idx, MemoryTransport& p) {
        std::cout << "wire_manip_MemoryOut_Request \n";
    }

    static bool func_wire_available_MemoryOut_Snoop(Flexus::Core::index_t idx) {
        std::cout << "wire_available_MemoryOut_Snoop called\n";
        return true;
    }

    static void func_wire_manip_MemoryOut_Snoop(Flexus::Core::index_t idx, MemoryTransport& p) {
        std::cout << "wire_manip_MemoryOut_Snoop \n";
    }

    // static void setPayloadp(TranslationPtr& p){
    //     uArchARMTestFixture::payload = p;
    // }

    // static TranslationPtr getPayloadp(){
    //   return uArchARMTestFixture::payload;
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


#include "uArchARMTests.h"
