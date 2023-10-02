#include <components/MMU/MMUImpl.cpp>
#include <gtest/gtest.h>
#include <core/qemu/qflex-api.h>
// #include <hw/core/cpu.h>
#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/mai_api.hpp>
#include <DummyQemu.h>

class MMUTestFixture : public testing::Test {
public: 
  static TranslationPtr payload;

// Initialization Functions

  static void InitializeMMUConfiguration(MMUConfiguration_struct& aCfg, int cores, int iTLBSize, int dTLBSize, bool perfectTLB) {
    aCfg.Cores = cores;
    aCfg.iTLBSize = iTLBSize;
    aCfg.dTLBSize = dTLBSize;
    aCfg.PerfectTLB = perfectTLB;
	  std::cout << "MMUConfiguration_struct defined\n";

  }

  static void InitializeTLBRequest(Flexus::SharedTypes::TranslationPtr& tlbRequest, unsigned int Vaddr,unsigned int Paddr){
    // Create an instance of the TLB request and set its attributes
	tlbRequest->theVaddr = VirtualMemoryAddress(Vaddr); 
	tlbRequest->thePaddr = PhysicalMemoryAddress(Paddr); 
	tlbRequest->theTLBtype = Flexus::SharedTypes::Translation::kINST;
  tlbRequest->isInstr();
	std::cout <<"Vaddr"<< tlbRequest->theVaddr << " Paddr"<< tlbRequest->thePaddr<< " TLB instance made\n";
  }

  static void InitializeJumpTable(MMUJumpTable& aJumpTable) {
        aJumpTable.wire_available_iTranslationReply = func_wire_available_iTranslationReply;
        aJumpTable.wire_manip_iTranslationReply = func_wire_manip_iTranslationReply;

        aJumpTable.wire_available_dTranslationReply = func_wire_available_dTranslationReply;
        aJumpTable.wire_manip_dTranslationReply = func_wire_manip_dTranslationReply;

        aJumpTable.wire_available_MemoryRequestOut = func_wire_available_MemoryRequestOut;
        aJumpTable.wire_manip_MemoryRequestOut = func_wire_manip_MemoryRequestOut;

        aJumpTable.wire_available_ResyncOut = func_wire_available_ResyncOut;
        aJumpTable.wire_manip_ResyncOut = func_wire_manip_ResyncOut;
       	std::cout << "MMUJumpTable defined\n";

    }
// Wire Callback Functions
    static bool func_wire_available_iTranslationReply(Flexus::Core::index_t idx) {
        std::cout << "wire_available_iTranslationReply called\n";
        return true;
    }

    static void func_wire_manip_iTranslationReply(Flexus::Core::index_t idx, TranslationPtr& p) {
        std::cout << "wire_manip_iTranslationReply called with ITR\n";
    }

    static bool func_wire_available_dTranslationReply(Flexus::Core::index_t idx) {
        std::cout << "wire_available_dTranslationReply called\n";
        return true;
    }

    static void func_wire_manip_dTranslationReply(Flexus::Core::index_t idx, MMUInterface::dTranslationReply::payload& p) {
        std::cout << "wire_manip_dTranslationReply called with DTR\n";
    }

    static bool func_wire_available_MemoryRequestOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_MemoryRequestOut called\n";
        return true;
    }

    static void func_wire_manip_MemoryRequestOut(Flexus::Core::index_t idx, TranslationPtr& p) {
        std::cout << "wire_manip_MemoryRequestOut called with MRO\n";
		// Assertion to check the payload value is one of the permissible values
        
        std::cout << p->thePaddr  << "\n";
        setPayloadp(p);
    //     if (std::find(permissibleMemoryRequestPayloadValues.begin(), permissibleMemoryRequestPayloadValues.end(), p->thePaddr) != permissibleMemoryRequestPayloadValues.end()) {
    //     std::cout << "Payload value is within permissible values." << std::endl;
    // } else {
    //     std::cout << "Payload value is not within permissible values." << std::endl;
    //     assert(false);  // Assertion fails
    // }
	}

    static bool func_wire_available_ResyncOut(Flexus::Core::index_t idx) {
        std::cout << "wire_available_ResyncOut called\n";
        return true;
    }

    static void func_wire_manip_ResyncOut(Flexus::Core::index_t idx, MMUInterface::ResyncOut::payload& p) {
        std::cout << "wire_manip_ResyncOut called with RO\n";
    }
    static void setPayloadp(TranslationPtr& p){
        MMUTestFixture::payload = p;
    }

    static TranslationPtr getPayloadp(){
      return MMUTestFixture::payload;
    }

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


#include "MMUTests.h"    
