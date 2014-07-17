#ifndef FLEXUS_QEMU_MAI_API_HPP_INCLUDED
#define FLEXUS_QEMU_MAI_API_HPP_INCLUDED

#include <boost/utility.hpp>

#include <core/flexus.hpp>
#include <core/exception.hpp>
#include <core/types.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/qemu/hap_api.hpp>
#if FLEXUS_TARGET_IS(v9)
#include <core/qemu/sparcmmu.hpp>
#endif

namespace Flexus {
namespace Qemu {
namespace API {
extern "C" {
#include <core/qemu/api.h>
} //extern "C"
} //namespace API
} //namespace Qemu
} //namespace Flexus

namespace Flexus {
namespace Qemu {
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
  //Note: do not access these variables directly when writing member functions.
  //Instead, use *this whenever you need theProcessor or theProcessorAsConfObject.
  //That way, the conversion operators will get called, and they will fill in the pointers
  //if their current value is not cached.
  API::conf_object_t * theProcessor;
  int theProcessorNumber;
  int theQemuProcessorNumber;

  void checkException() const {
    API::sim_exception_t anException = API::QEMU_clear_exception();
    checkException( anException ) ;
  }

  void checkException(API::sim_exception_t anException) const {
    if (anException != API::SimExc_No_Exception) {
      if (anException == API::SimExc_Memory) {
        throw MemoryException();
      } else {
        throw QemuException(API::QEMU_last_error());
      }
    }
  }

  BaseProcessorImpl(API::conf_object_t * aProcessor)
    : theProcessor(aProcessor)
    , theProcessorNumber(aProcessor ? ProcessorMapper::mapProcNum2FlexusIndex(APIFwd::QEMU_get_processor_number(aProcessor)) : 0)
    , theQemuProcessorNumber(aProcessor ? APIFwd::QEMU_get_processor_number(aProcessor) : 0)
  {}

public:
  virtual ~BaseProcessorImpl() {}

  operator API::conf_object_t * () const {
    return theProcessor;
  }

  /*
        #if FLEXUS_TARGET_IS(x86)
          operator API::processor_t * () {
            return API::SIM_conf_object_to_processor(theProcessor);
          }
        #endif
  */
  std::string disassemble(VirtualMemoryAddress const & anAddress) const {
    API::tuple_int_string_t * tuple;
    tuple = API::QEMU_disassemble(*this, (API::logical_address_t)anAddress, 1/* address is logical*/);
    API::sim_exception_t anException = API::QEMU_clear_exception();
    if (anException == API::SimExc_No_Exception) {
      if (strcmp("jmpl [%i7 + 8], %g0",tuple->string)==0) return std::string("ret (jmpl [%i7 + 8], %g0)");
      if (strcmp("jmpl [%o7 + 8], %g0",tuple->string)==0) return std::string("retl (jmpl [%o7 + 8], %g0)");
      return std::string(tuple->string);
    } else {
      return "disassembly unavailable";
    }
  }

  std::string disassemble(PhysicalMemoryAddress const & anAddress) const {
    API::tuple_int_string_t * tuple;
    tuple = API::QEMU_disassemble(*this, (API::physical_address_t)anAddress, 0/* address is physical*/);
    API::sim_exception_t anException = API::QEMU_clear_exception();
    if (anException == API::SimExc_No_Exception) {
      if (strcmp("jmpl [%i7 + 8], %g0",tuple->string)==0) return std::string("ret (jmpl [%i7 + 8], %g0)");
      if (strcmp("jmpl [%o7 + 8], %g0",tuple->string)==0) return std::string("retl (jmpl [%o7 + 8], %g0)");
      return std::string(tuple->string);
    } else {
      return "disassembly unavailable";
    }
  }

  std::string describeException(int anException) const {
    char const * descr = API::QEMU_get_exception_name(*this, anException);
    API::sim_exception_t except = API::QEMU_clear_exception();
    if (except == API::SimExc_No_Exception) {
      return std::string(descr);
    } else {
      return "unknown_exception";
    }
  }

  API::cycles_t cycleCount() const {
    return API::QEMU_cycle_count(*this);
  }

  long long stepCount() const {
    return API::QEMU_step_count(*this);
  }

  void enable() {
    API::QEMU_enable_processor(*this);
  }

  void disable() {
    API::QEMU_disable_processor(*this);
  }

  VirtualMemoryAddress getPC() const {
    return VirtualMemoryAddress( API::QEMU_get_program_counter(*this) );
  }

  unsigned long long readRegister(int aRegister) const {
    return API::QEMU_read_register(*this, aRegister);
  }

  void writeRegister(int aRegister, unsigned long long aValue) const {
    return API::QEMU_write_register(*this, aRegister, aValue);
  }

  unsigned long long readVAddr(VirtualMemoryAddress anAddress, int aSize) const {
    try {
      API::logical_address_t addr(anAddress);
      API::physical_address_t phy_addr = API::QEMU_logical_to_physical(*this, API::QEMU_DI_Data, addr);
      checkException();

      unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, phy_addr, aSize);
      checkException();

      return value;
    } catch (MemoryException & anError ) {
      return 0;
    }
  }

  unsigned long long readPAddr(PhysicalMemoryAddress anAddress, int aSize) const {
    try {
      API::physical_address_t phy_addr(anAddress);
      unsigned long long value = Qemu::API::QEMU_read_phys_memory( *this, phy_addr, aSize);
      checkException();

      return value;
    } catch (MemoryException & anError ) {
      return 0;
    }
  }
 
 //added by jevdjic 
 unsigned long long getMemorySizeInMB(){ //memory size in MB
  try{
    Qemu::API::conf_object_t *memory = API::SIM_get_object("server_memory_image");
    if (memory==0) memory = API::SIM_get_object("memory_image");
    if(memory==0) memory = API::SIM_get_object("memory0_image"); 
    if(memory==0) return 0;
    Qemu::API:: attr_value_t size = API::SIM_get_attribute(memory, "size");
    return size.u.integer/(1024*1024);	
  }catch(MemoryException &abError){
    return 0;
  }
 }
 //end jevdjic

  void writePAddr(PhysicalMemoryAddress anAddress, int aSize, unsigned long long aValue) const {
    try {
      API::physical_address_t phy_addr(anAddress);
      Qemu::API::QEMU_write_phys_memory( *this, phy_addr, aValue, aSize);
      checkException();
    } catch (MemoryException & anError ) {
      DBG_( Crit, ( << "Unable to write " << aValue << " to  " << anAddress << "[" << aSize << "]" ) );
    }
  }

  virtual PhysicalMemoryAddress 
	  translate(VirtualMemoryAddress anAddress) const {
    try {
      API::logical_address_t addr(anAddress);
      API::physical_address_t phy_addr = 
		  API::QEMU_logical_to_physical(*this, API::QEMU_DI_Data, addr);
      checkException();
      return PhysicalMemoryAddress(phy_addr);
    } catch (MemoryException & anError ) {
      return PhysicalMemoryAddress(0);
    }
  }

  virtual PhysicalMemoryAddress 
	  translateInstruction(VirtualMemoryAddress anAddress) const {
    try {
      API::logical_address_t addr(anAddress);
      API::physical_address_t phy_addr = 
		  API::QEMU_logical_to_physical(*this, API::QEMU_DI_Instruction, addr);
      checkException();

      return PhysicalMemoryAddress(phy_addr);
    } catch (MemoryException & anError ) {
      return PhysicalMemoryAddress(0);
    }
  }

  int id() const {
    return theProcessorNumber;
  }
  int qemuId() const {
    return theQemuProcessorNumber;
  }

  bool mai_mode() const {
    return false;
  }

};

#if FLEXUS_TARGET_IS(v9)
struct Translation {
  VirtualMemoryAddress theVaddr;
  int theASI;
  int theTL;
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
  bool isMMU();
  bool isInterrupt();
  bool isTranslating();
};

class v9ProcessorImpl :  public BaseProcessorImpl {
private:
  static const int kReg_npc = 33;
  //mutable API::sparc_v9_interface * theSparcAPI;
  //mutable API::conf_object_t * theMMU;
  //mutable API::mmu_interface_t * theMMUAPI;
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

  VirtualMemoryAddress getNPC() const {
    return VirtualMemoryAddress( API::QEMU_read_register(*this, kReg_npc) );
  }

 /* API::sparc_v9_interface * sparc() const {
    if (theSparcAPI == 0) {
      theSparcAPI = reinterpret_cast<API::sparc_v9_interface *>( API::SIM_get_interface(*this, "sparc_v9"));
      DBG_Assert( theSparcAPI );
    }
    return theSparcAPI;
  }

  API::mmu_interface_t * mmu() const {
    if (theMMUAPI == 0) {
      //Obtain the MMU object
      API::attr_value_t mmu_obj;
      mmu_obj = API::SIM_get_attribute(*this, "mmu");
      theMMU = mmu_obj.u.object;
      DBG_Assert( theMMU );
      theMMUAPI = reinterpret_cast<API::mmu_interface_t *>( API::SIM_get_interface(theMMU, "mmu"));
      DBG_Assert( theMMUAPI );
    }
    return theMMUAPI;
  }*/

  unsigned long long readF(int aRegister) const {
    return sparc()->read_fp_register_x(*this, aRegister);
  }

  void writeF(int aRegister, unsigned long long aValue) const {
    sparc()->write_fp_register_x(*this, aRegister, aValue);
  }

  unsigned long long readG(int aRegister) const {
    return sparc()->read_global_register(*this, 0, aRegister);
  }

  unsigned long long readAG(int aRegister) const {
    return sparc()->read_global_register(*this, 1, aRegister);
  }

  unsigned long long readMG(int aRegister) const {
    return sparc()->read_global_register(*this, 2, aRegister);
  }

  unsigned long long readIG(int aRegister) const {
    return sparc()->read_global_register(*this, 3, aRegister);
  }

  unsigned long long readWindowed(int aWindow, int aRegister) const {
    return sparc()->read_window_register(*this, aWindow, aRegister);
  }

  unsigned long long interruptRead(VirtualMemoryAddress anAddress, int anASI) const;

  long long getSystemTickInterval() const {
    /*double freq = 1.0, stick = 1.0;
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
	  return 42;
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

  //SimicsImpl MMU API
  PhysicalMemoryAddress translateInstruction_QemuImpl(
					  VirtualMemoryAddress anAddress
					  ) const;
  long fetchInstruction_QemuImpl(VirtualMemoryAddress const & anAddress);

  boost::tuple < PhysicalMemoryAddress
					, bool/*cacheable*/
					, bool/*side-effect*/ > 
					translateTSB_QemuImpl(
							  VirtualMemoryAddress anAddress
							, int anASI
							) const;

  unsigned long long readVAddr_QemuImpl(
						VirtualMemoryAddress anAddress
					  , int anASI
					  , int aSize) const;

  unsigned long long readVAddrXendian_QemuImpl(
						VirtualMemoryAddress anAddress
					  , int anASI
					  , int aSize) const;

  void translate_QemuImpl(
						API::v9_memory_transaction_t & xact
					  , VirtualMemoryAddress anAddress
					  , int anASI ) const;

  //MMUImpl
  void translate_MMUImpl( Translation & aTranslation, bool aTakeTrap) const;
  long /*opcode*/ fetchInstruction_MMUImpl(Translation & aTranslation, bool aTakeTrap);
  unsigned long long readVAddr_MMUImpl(Translation & aTranslation, int aSize) const;
  unsigned long long readVAddrXendian_MMUImpl(Translation & aTranslation, int aSize) const;

  int advance(bool anAcceptInterrupt);
  int getPendingException() const;
  int getPendingInterrupt() const;

  void breakSimulation() {
//    API::SIM_break_cycle( *this, 0);
	QEMU_break_simulation("Micro-arch interface called simulation break.\n");
  }
};
#endif //FLEXUS_TARGET_IS(v9)

#if FLEXUS_TARGET_IS(x86)
class x86ProcessorImpl :  public BaseProcessorImpl {

public:
  explicit x86ProcessorImpl(API::conf_object_t * aProcessor)
    : BaseProcessorImpl(aProcessor) {
  }

  void breakSimulation() {
    API::QEMU_break_cycle( *this, 0);
  }

  bool validateMMU() {
    return true;
  }
  void dumpMMU() {}

};
#endif //FLEXUS_TARGET_IS(x86)

#if FLEXUS_TARGET_IS(v9)
#define PROCESSOR_IMPL v9ProcessorImpl
#elif FLEXUS_TARGET_IS(x86)
#define PROCESSOR_IMPL x86ProcessorImpl
#else
#error "Architecture does not support MAI"

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
    return Processor(APIFwd::QEMU_get_processor(ProcessorMapper::mapFlexusIndex2ProcNum(aProcessorNumber)));
  }

  static Processor current() {
    return Processor(API::QEMU_current_processor());
  }
};

#undef PROCESSOR_IMPL

#if FLEXUS_TARGET_IS(v9)
bool isTranslatingASI(int anASI);
#endif

unsigned long long endianFlip(unsigned long long val, int aSize);

} //end Namespace Qemu
} //end namespace Flexus

#endif //FLEXUS_QEMU_MAI_API_HPP_INCLUDED
#endif
