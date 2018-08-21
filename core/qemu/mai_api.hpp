#ifndef FLEXUS_QEMU_MAI_API_HPP_INCLUDED
#define FLEXUS_QEMU_MAI_API_HPP_INCLUDED

#include <boost/utility.hpp>
#include <core/target.hpp>
#include <core/flexus.hpp>
#include <core/exception.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/types.hpp>
using namespace Flexus::Core;

namespace Flexus {
namespace Qemu {


void onInterrupt (void * aPtr, void* anObj, long long aVector);

struct InstructionRaisesException : public QemuException {
  InstructionRaisesException () : QemuException("InstructionRaisesException") {}
};
struct UnresolvedDependenciesException : public QemuException {
  UnresolvedDependenciesException () : QemuException("UnresolvedDependenciesException") {}
};
struct SpeculativeException : public QemuException {
  SpeculativeException () : QemuException("SpeculativeException ") {}
};
struct StallingException : public QemuException {
  StallingException () : QemuException("StallingException ") {}
};
struct SyncException : public QemuException {
  SyncException() : QemuException("SyncException") {}
};

struct MemoryException : public QemuException {
  MemoryException() : QemuException("MemoryException") {}
};

} //namespace Qemu
} //namespace Flexus


namespace Flexus {
namespace Qemu {

class ProcessorMapper {
private:
  static ProcessorMapper * theMapper;

  int theNumVMs;
  std::vector<std::pair<int, int> > theProcMap;
  std::vector<int> theClientMap;
  std::vector<std::pair<int, bool> > theReverseMap;

  ProcessorMapper();

public:
  static int mapFlexusIndex2ProcNum(int index);
  static int mapClientNum2ProcNum(int index);
  static int mapProcNum2FlexusIndex(int index);
  static int mapFlexusIndex2VM(int index);

  static int numVMs();
  static int numClients();
  static int numProcessors();
};


using Flexus::SharedTypes::VirtualMemoryAddress;
using Flexus::SharedTypes::PhysicalMemoryAddress;

class BaseProcessorImpl {
protected:
  API::conf_object_t * theProcessor;
  int theProcessorNumber;
  int theQEMUProcessorNumber;

  void checkException() const {
  }
  BaseProcessorImpl(API::conf_object_t * aProcessor)
    : theProcessor(aProcessor)
    , theProcessorNumber(aProcessor ? ProcessorMapper::mapProcNum2FlexusIndex(API::QEMU_get_cpu_index(aProcessor)) : 0)
    , theQEMUProcessorNumber(aProcessor ? API::QEMU_get_cpu_index(aProcessor) : 0)
  {}

public:
  virtual ~BaseProcessorImpl() {}

  operator API::conf_object_t * () const {
    return theProcessor;
  }


  std::string disassemble(VirtualMemoryAddress const & anAddress) const {
      API::logical_address_t addr(anAddress);
      return API::QEMU_disassemble(*this, addr);
  }


  std::string dump_state() const {
      return API::QEMU_dump_state(*this);
  }

  std::string describeException(int anException) const {
      return "unknown_exception";
  }

  API::cycles_t cycleCount() const {
    //return API::SIM_cycle_count(*this);
    assert(false);
    return 0;
  }

  long long stepCount() const {
    //return API::SIM_step_count(*this);
    assert(false);
    return 0;
  }

  void enable() {
    assert(false);
    //API::SIM_enable_processor(*this);
  }

  void disable() {
    assert(false);
    //API::SIM_disable_processor(*this);
  }


  VirtualMemoryAddress getPC() const {
    return VirtualMemoryAddress( API::QEMU_get_program_counter(*this) );
  }

  uint64_t readXRegister(int anIndex) const {
   return API::QEMU_read_register(*this, API::GENERAL, anIndex);
}

  uint64_t readVRegister(int anIndex) const {
   return API::QEMU_read_register(*this, API::FLOATING_POINT, anIndex);
}

  uint32_t readPSTATE() const {
   return API::QEMU_read_pstate(*this);
}

  uint32_t readDCZID_EL0() const {
    return API::QEMU_read_DCZID_EL0(*this);
  }

  uint32_t readAARCH64() const {
    return API::QEMU_read_AARCH64(*this);
  }

   void readException(API::exception_t* exp) const {
   return API::QEMU_read_exception(*this, exp);
}

  uint64_t* readSCTLR() const {
   return API::QEMU_read_sctlr(*this);
}

uint64_t readHCREL2() const {
    return API::QEMU_read_hcr_el2(*this);
}

  uint32_t readFPCR() const {
   return API::QEMU_read_fpcr(*this);
}

  uint32_t readFPSR() const {
   return API::QEMU_read_fpsr(*this);
}

  uint64_t readPC() const {
   return API::QEMU_get_program_counter(*this);
}

  void writeRegister(int aRegister, unsigned long long aValue) const {
    assert(false);
    //return API::QEMU_write_register(*this, aRegister, aValue);
  }

  bits readVAddr(VirtualMemoryAddress anAddress, int aSize) const {
      API::logical_address_t addr(anAddress);
      API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Data, addr);
      return construct( API::QEMU_read_phys_memory(phy_addr, aSize), aSize);
  }

  bits readPAddr(PhysicalMemoryAddress anAddress, int aSize) const {
      API::physical_address_t phy_addr(anAddress);
      return construct(API::QEMU_read_phys_memory( phy_addr, aSize), aSize);
  }

  void writePAddr(PhysicalMemoryAddress anAddress, int aSize, unsigned long long aValue) const {
    assert(false);
  }

  virtual PhysicalMemoryAddress translate(VirtualMemoryAddress anAddress) const {
      API::logical_address_t addr(anAddress);
      API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Data, addr);
      return PhysicalMemoryAddress(phy_addr);
  }

  virtual PhysicalMemoryAddress translateInstruction(VirtualMemoryAddress anAddress) const {
      API::logical_address_t addr(anAddress);
      API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction, addr);

      return PhysicalMemoryAddress(phy_addr);

  }

  int id() const {
    return theProcessorNumber;
  }
  int QEMUId() const {
    return theQEMUProcessorNumber;
  }

  bool mai_mode() const {
    return true;
  }

};

struct Translation {
  VirtualMemoryAddress theVaddr;
  int thePSTATE;
  enum eTranslationType {
    eStore,
    eLoad,
    eFetch
  } theType;
  PhysicalMemoryAddress thePaddr;
  int theException;
  unsigned long long theTTEEntry;
  bool isCacheable();
  bool isSideEffect();
  bool isXEndian();
  bool isInterrupt();
  bool isTranslating();

  // Msutherl: additional stuff....
  uint8_t requiredTableLookups;
  uint8_t currentLookupLevel;
  bool isBR0;
  uint32_t granuleSize;
  std::shared_ptr<MMU::TTResolver> TTAddressResolver;
};

////////// Msutherl
// - from here down

using namespace MMU;
class armProcessorImpl :  public BaseProcessorImpl {
private:
  static const int kReg_npc = 102;
  mutable API::armInterface_t * theARMAPI;
  std::shared_ptr<mmu_t> theMMU;
  mutable API::mmu_interface_t* theMMUAPI;
  int thePendingInterrupt;
  bool theInterruptsConnected;
  bool mmuInitialized;

  void initialize();
  void handleInterrupt( long long aVector );

public:
  explicit armProcessorImpl(API::conf_object_t * aProcessor)
    : BaseProcessorImpl(aProcessor)
    , theARMAPI(nullptr)
    , theMMU(nullptr)
    , theMMUAPI(nullptr)
    , thePendingInterrupt( API::QEMU_PE_No_Exception )
    , theInterruptsConnected(false)
    , mmuInitialized(false) { }

  API::armInterface_t * arm() const {
    if (theARMAPI == 0) {
      assert(false); //FIXME
    }
    return theARMAPI;
  }

  // Msutherl: Added functions for MMU interactions.
  // - added smaller MMU interface (resolving walks + memory accesses resolved in Flexus components)
  void initializeMMU();
  std::shared_ptr<MMU::mmu_t> getMMUPointer();
  MMU::TTEDescriptor getNextTTDescriptor(Translation& aTr );
  void InitialTranslationSetup( Translation& aTr );
  void doTTEAccess( Translation& aTr );
private:
  void setupTTResolver( Translation& aTr, uint64_t rawTTDescriptor) ;

public:
  uint8_t getQEMUExceptionLevel( ) const { 
      return API::QEMU_get_current_el(*this);
  }

//FIXME: The following functions for SPARC should be implemented in QEMU
  unsigned long long readX(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 5);
  }

  void writeF(int aRegister, unsigned long long aValue) const {
    assert(false);
    //sparc()->write_fp_register_x(*this, aRegister, aValue);
  }

  unsigned long long readG(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 0);
  }

  unsigned long long interruptRead(VirtualMemoryAddress anAddress, int anASI) const;

  long long getSystemTickInterval() const {
    return (long long)API::QEMU_get_tick_frequency(*this);
  }

  // LEGACY MMU API, FIXME - REMOVE THIS WHEN DONE WITH NEW MMU
  void translate(Translation & aTranslation, bool aTakeException) const;
  long /*opcode*/ fetchInstruction(Translation & aTranslation, bool aTakeTrap);
    
    // Msutherl - june'18
    // - added smaller MMU interface (resolving walks + memory accesses resolved in Flexus components I/D TLBs)
  MMU::TTEDescriptor getNextTTDescriptor( mmu_t* mmu, PhysicalMemoryAddress pa );

  bits readVAddr(VirtualMemoryAddress anAddress, int aSize) const;
  bits readVAddrXendian(Translation & aTranslation, int aSize) const;

  //QemuImpl MMU API
  PhysicalMemoryAddress translateInstruction_QemuImpl(VirtualMemoryAddress anAddress) const;
  long fetchInstruction_QemuImpl(VirtualMemoryAddress const & anAddress);

  unsigned long long readVAddr_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  unsigned long long readVAddrXendian_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  void translate_QemuImpl(  API::arm_memory_transaction_t & xact, VirtualMemoryAddress anAddress, int anASI ) const;

  int advance(bool anAcceptInterrupt);
  int getPendingException() const;
  int getPendingInterrupt() const;

  void breakSimulation() {
    API::QEMU_break_simulation("");
  }
};


#define PROCESSOR_IMPL armProcessorImpl


class Processor : public BuiltInObject< PROCESSOR_IMPL > {
  typedef BuiltInObject< PROCESSOR_IMPL > base;
public:
  explicit Processor():
    base( 0 ) {}

  explicit Processor(API::conf_object_t * aProcessor):
    base( PROCESSOR_IMPL (aProcessor) ) {}

  operator API::conf_object_t * () {
    return theImpl;
  }

  static Processor getProcessor(int aProcessorNumber) {
    return Processor(API::QEMU_get_cpu_by_index(ProcessorMapper::mapFlexusIndex2ProcNum(aProcessorNumber)));
  }

  static Processor getProcessor(std::string const & aProcessorName) {
    return Processor(API::QEMU_get_object_by_name(aProcessorName.c_str()));
  }

  static Processor current() {
    assert(false);
    return getProcessor(0);
  }
};

#undef PROCESSOR_IMPL
unsigned long long endianFlip(unsigned long long val, int aSize);

} //end Namespace Qemu
} //end namespace Flexus

#endif //FLEXUS_QEMU_MAI_API_HPP_INCLUDED
