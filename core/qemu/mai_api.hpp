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
};

#if FLEXUS_TARGET_IS(v9)
class v9ProcessorImpl :  public BaseProcessorImpl {
private:
  //static const int kReg_npc = 33;
  static const int kReg_npc = 102;//NOOSHIN
  mutable API::sparc_v9_interface_t * theSparcAPI;
  mutable API::conf_object_t * theMMU;
  mutable API::mmu_interface_t * theMMUAPI;
  int thePendingInterrupt;
  bool theInterruptsConnected;

  void initialize();
  void handleInterrupt( long long aVector);

public:
  explicit v9ProcessorImpl(API::conf_object_t * aProcessor)
    : BaseProcessorImpl(aProcessor)
    , thePendingInterrupt( API::QEMU_PE_No_Exception )
    , theInterruptsConnected(false) {
    theSparcAPI = 0;
    theMMUAPI = 0;
  }

  void initializeMMUs();
#if FLEXUS_TARGET_IS(v9)
  VirtualMemoryAddress getNPC() const {
    uint64_t reg_content;
    API::QEMU_read_register(*this, kReg_npc, NULL, &reg_content);
    return VirtualMemoryAddress( reg_content );
  }
#endif

  API::sparc_v9_interface_t * sparc() const {
    if (theSparcAPI == 0) {
      assert(false); //FIXME
      //theSparcAPI = reinterpret_cast<API::sparc_v9_interface *>( API::SIM_get_interface(*this, "sparc_v9"));
      //DBG_Assert( theSparcAPI );
    }
    return theSparcAPI;
  }

  API::mmu_interface_t * mmu() const {
    if (theMMUAPI == 0) {
      //Obtain the MMU object
      API::attr_value_t mmu_obj;
      assert(false);	//FIXME
      //mmu_obj = API::SIM_get_attribute(*this, "mmu");
      theMMU = mmu_obj.u.object;
      DBG_Assert( theMMU );
      //theMMUAPI = reinterpret_cast<API::mmu_interface_t *>( API::SIM_get_interface(theMMU, "mmu"));
      DBG_Assert( theMMUAPI );
    }
    return theMMUAPI;
  }

//FIXME: The following functions for SPARC should be implemented in QEMU
  unsigned long long readF(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 5);
  }

  void writeF(int aRegister, unsigned long long aValue) const {
    assert(false);
    //sparc()->write_fp_register_x(*this, aRegister, aValue);
  }

  unsigned long long readG(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 0);
  }

  unsigned long long readAG(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 1);
  }

  unsigned long long readSP(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 1);
  }

  unsigned long long readMG(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 3);
  }

  unsigned long long readIG(int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aRegister, 2);
  }

  unsigned long long readWindowed(int aWindow, int aRegister) const {
    return API::QEMU_read_register_by_type(*this, aWindow*16+aRegister-8, 4);
  }

  unsigned long long interruptRead(VirtualMemoryAddress anAddress, int anASI) const;

  long long getSystemTickInterval() const {
/*
    double freq = 1.0, stick = 1.0;
    API::attr_value_t freq_attr, stick_attr;
    freq_attr = API::SIM_get_attribute(*this, "freq_mhz");
    if ( freq_attr.kind == API::Sim_Val_Integer ) {
      freq = freq_attr.u.integer;
    } else if ( freq_attr.kind == API::Sim_Val_Floating) {
      freq = freq_attr.u.floating;
    } else {
      DBG_Assert(false);
    }
    stick_attr = API::SIM_get_attribute(*this, "system_tick_frequency");
    if ( stick_attr.kind == API::Sim_Val_Integer ) {
      stick = stick_attr.u.integer;
    } else if ( stick_attr.kind == API::Sim_Val_Floating) {
      stick = stick_attr.u.floating;
    } else {
      DBG_Assert(false);
    }
    DBG_Assert( freq != 0 );
    DBG_Assert( stick != 0 );
    long long interval = static_cast<long long>(freq / stick);
    DBG_Assert( interval > 0);
    return interval;
*/
    return (long long)API::QEMU_get_tick_frequency(*this);	//ALEX: FIXME - replaced "interval" with frequency.
								//Not sure what this is supposed to be.
  }

  //Public MMU API
  void translate(Translation & aTranslation, bool aTakeException) const;
  long /*opcode*/ fetchInstruction(Translation & aTranslation, bool aTakeTrap);

  unsigned long long readVAddr(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  unsigned long long readVAddrXendian(Translation & aTranslation, int aSize) const;

  MMU::mmu_t getMMU();
  void ckptMMU();
  void releaseMMUCkpt();
  void rollbackMMUCkpts(int n);
  void resyncMMU();
  bool validateMMU(MMU::mmu_t * m = NULL);
  void dumpMMU(MMU::mmu_t * m = NULL);
  void initializeASIInfo();

  unsigned long long mmuRead(VirtualMemoryAddress anAddress, int anASI);
  void mmuWrite(VirtualMemoryAddress anAddress, int anASI, unsigned long long aValue);

  //QemuImpl MMU API
  PhysicalMemoryAddress translateInstruction_QemuImpl(VirtualMemoryAddress anAddress) const;
  long fetchInstruction_QemuImpl(VirtualMemoryAddress const & anAddress);

  std::tuple < PhysicalMemoryAddress, bool/*cacheable*/, bool/*side-effect*/ > translateTSB_QemuImpl(VirtualMemoryAddress anAddress, int anASI) const;
  unsigned long long readVAddr_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  unsigned long long readVAddrXendian_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  void translate_QemuImpl(  API::v9_memory_transaction_t & xact, VirtualMemoryAddress anAddress, int anASI ) const;

  //MMUImpl
  void translate_MMUImpl( Translation & aTranslation, bool aTakeTrap) const;
  long /*opcode*/ fetchInstruction_MMUImpl(Translation & aTranslation, bool aTakeTrap);
  unsigned long long readVAddr_MMUImpl(Translation & aTranslation, int aSize) const;
  unsigned long long readVAddrXendian_MMUImpl(Translation & aTranslation, int aSize) const;

  int advance(bool anAcceptInterrupt);
  int getPendingException() const;
  int getPendingInterrupt() const;

  void breakSimulation() {
    //API::SIM_break_cycle( *this, 0);
    API::QEMU_break_simulation("");
  }
};
#endif //FLEXUS_TARGET_IS(v9)

class arm_tte_t {
};

#if FLEXUS_TARGET_IS(ARM)
using namespace MMU;
class armProcessorImpl :  public BaseProcessorImpl {
private:
  static const int kReg_npc = 102;
  mutable API::armInterface_t * theARMAPI;
  int thePendingInterrupt;
  bool theInterruptsConnected;

  void initialize();
  void handleInterrupt( long long aVector);

public:
  explicit armProcessorImpl(API::conf_object_t * aProcessor)
    : BaseProcessorImpl(aProcessor)
    , thePendingInterrupt( API::QEMU_PE_No_Exception )
    , theInterruptsConnected(false) {
    theARMAPI = 0;
  }
  API::armInterface_t * arm() const {
    if (theARMAPI == 0) {
      assert(false);
    }
    return theARMAPI;
  }
  unsigned long long interruptRead(VirtualMemoryAddress anAddress, int anASI) const;

  long long getSystemTickInterval() const {
    return (long long)API::QEMU_get_tick_frequency(*this);
  }

  //Public MMU API
  void translate(Translation & aTranslation, bool aTakeException) const;
  long /*opcode*/ fetchInstruction(Translation & aTranslation, bool aTakeTrap);
    
    // Msutherl - june'18
    // - added smaller MMU interface (resolving walks + memory accesses resolved in Flexus components I/D TLBs)

    arm_tte_t getNextTTAddress( mmu_t* mmu, PhysicalMemoryAddress pa ); // FIXME: is the pte in vmem or pmem?

  bits readVAddr(VirtualMemoryAddress anAddress, int aSize) const;
  bits readVAddrXendian(Translation & aTranslation, int aSize) const;



  PhysicalMemoryAddress translateInstruction_QemuImpl(VirtualMemoryAddress anAddress) const;
  long fetchInstruction_QemuImpl(VirtualMemoryAddress const & anAddress);

  unsigned long long readVAddr_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  unsigned long long readVAddrXendian_QemuImpl(VirtualMemoryAddress anAddress, int anASI, int aSize) const;
  void translate_QemuImpl(  API::arm_memory_transaction_t & xact, VirtualMemoryAddress anAddress, int anASI ) const;

  int advance();
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
