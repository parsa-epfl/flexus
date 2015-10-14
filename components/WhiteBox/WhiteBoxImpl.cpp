#include <components/WhiteBox/WhiteBox.hpp>

#define FLEXUS_BEGIN_COMPONENT WhiteBox
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <list>

#include <components/WhiteBox/WhiteBoxIface.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/simics/mai_api.hpp>

namespace nWhiteBox {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using namespace Flexus::Simics;

namespace API = Flexus::Simics::API;

static const uint64_t  KernelVaddr_cpu_list = 0x104652c8;

#if FLEXUS_TARGET_IS(v9)
std::ostream & operator<< (std::ostream & anOstream, CPUState & entry) {
  anOstream
      << std::hex
      << " trap:" << entry.theTrap
      << " thread:0x" << entry.theThread
      ;
  std::list<uint64_t>::reverse_iterator i;
  for (i = entry.theBackTrace.rbegin(); i != entry.theBackTrace.rend(); ++i)
    anOstream << " 0x" << *i;
  anOstream << std::dec;
  return anOstream;
}
#endif

struct WhiteBoxImpl : WhiteBox {
  API::symtable_interface_t * theSymTable;

  std::vector< PhysicalMemoryAddress > theThreadTs;
  std::vector< PhysicalMemoryAddress > theIdleThreadTs;

  WhiteBoxImpl() {
#if FLEXUS_TARGET_IS(v9)
    API::SIM_load_module("symtable");
    API::conf_class_t * symtable_class = API::SIM_get_class("symtable");
    API::conf_object_t * symtable_obj = API::SIM_new_object(symtable_class, "flexus_symtable");
    theSymTable = reinterpret_cast<API::symtable_interface_t *>(API::SIM_get_class_interface(symtable_class, "symtable"));

    for (int32_t i = 0; i < API::SIM_number_processors(); ++i) {
      API::conf_object_t * ctx = API::SIM_get_attribute(Simics::APIFwd::SIM_get_processor(i), "current_context").u.object;
      API::attr_value_t val;
      val.kind = API::Sim_Val_Object;
      val.u.object = symtable_obj;
      API::SIM_set_attribute(ctx, "symtable", &val);
    }
    API::attr_value_t arch = API::SIM_get_attribute( Simics::APIFwd::SIM_get_processor(0), "architecture");
    API::SIM_set_attribute( symtable_obj, "arch", &arch);

    DBG_(Dev, ( << "symtable loaded" ) );
#endif

  }

  uint64_t get_thread_t( int32_t aCPU ) {
#if FLEXUS_TARGET_IS(v9)
    DBG_Assert( aCPU < API::SIM_number_processors());
    if (theThreadTs.size() == 0 ) {
      theThreadTs.resize( API::SIM_number_processors() );
      VirtualMemoryAddress cpu_ptr( KernelVaddr_cpu_list );  //The virtual address of cpu_list in our kernel
      Processor cpu0 = Processor::getProcessor(0);
      cpu_ptr = VirtualMemoryAddress(cpu0->readVAddr(cpu_ptr, 4 /*NUCLEUS*/, 8));

      DBG_(Tmp,
           ( << "initializing thread_t's, num procs=" << std::dec << API::SIM_number_processors()
             << "  sys width=" << Flexus::Core::ComponentManager::getComponentManager().systemWidth()
           )
          );
      uint32_t t1 = static_cast<uint32_t> (API::SIM_number_processors());
      uint32_t t2 = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
      // DBG_(Tmp, ( << "  t1=" << std::dec << t1 << "  t2=" << t2 ) );
      for (uint32_t i = 0; (i < t1 ) && (i < t2); i++) {
        // DBG_(Tmp, ( << " trying i=" << std::dec << i ) );

        //Try each translation on each CPU until it works
        bool ok = false;

        for (uint32_t j = 0; (j < t1) && (j < t2); j++) {
          //DBG_(Tmp, ( << "   trying j=" << std::dec << j ) );
          Processor cpu = Processor::getProcessor(j);
          int32_t cpu_num = cpu->readVAddr(VirtualMemoryAddress(cpu_ptr), 4/*NUCLEUS*/, 4); //CPU number at offset 0
          if (cpu_num == (static_cast<int> ( i )) ) {
            bool ignored1, ignored2;
            PhysicalMemoryAddress paddr;
            std::tie( paddr, ignored1, ignored2) = cpu->translateTSB_SimicsImpl(VirtualMemoryAddress(cpu_ptr + 0x10), 4 /*NUCLEUS*/ );
            theThreadTs[i] = paddr;
            DBG_(Dev, ( << "CPU[" << i << "] thread_t point at paddr: " <<  std::hex << theThreadTs[i] << std::dec ) );
            cpu_ptr = VirtualMemoryAddress(cpu->readVAddr(VirtualMemoryAddress(cpu_ptr + 0x48), 4 /*NUCLEUS*/, 8)); //next CPU pointer at offset 0x48
            ok = true;
            //break;
            j = 1000000;
          }
        }

        if (!ok) {
          DBG_(Crit, ( << "No CPU was able to translate the virtual address of the cpu struct for cpu " << i ) );
          return 0;
        }

      }
    }
    uint64_t thread_t_ptr = Processor::getProcessor(0)->readPAddr(theThreadTs[aCPU], 8); //thread_t pointer at offset 0x10
    DBG_(Verb, ( << "Thread_t: " << std::hex << thread_t_ptr << std::dec ) );

    return thread_t_ptr;
#else
    return 0;
#endif
  };

  uint64_t get_idle_thread_t( int32_t aCPU ) {
#if FLEXUS_TARGET_IS(v9)
    DBG_Assert( aCPU < API::SIM_number_processors());
    if (theIdleThreadTs.size() == 0 ) {
      theIdleThreadTs.resize( API::SIM_number_processors() );
      VirtualMemoryAddress cpu_ptr( KernelVaddr_cpu_list );  //The virtual address of cpu_list in our kernel
      Processor cpu0 = Processor::getProcessor(0);
      cpu_ptr = VirtualMemoryAddress(cpu0->readVAddr(cpu_ptr, 4 /*NUCLEUS*/, 8));
      DBG_(Tmp,
           ( << "initializing idle_thread_t's, num procs=" << std::dec << API::SIM_number_processors()
             << "  sys width=" << Flexus::Core::ComponentManager::getComponentManager().systemWidth()
           )
          );
      uint32_t t1 = static_cast<uint32_t> (API::SIM_number_processors());
      uint32_t t2 = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
      //DBG_(Tmp, ( << "  t1=" << std::dec << t1 << "  t2=" << t2 ) );
      for (uint32_t i = 0; (i < t1 ) && (i < t2); i++) {
        //DBG_(Tmp, ( << " trying i=" << std::dec << i ) );
        //Try each translation on each CPU until it works
        bool ok = false;
        for (uint32_t j = 0; (j < t1) && (j < t2); j++) {
          //DBG_(Tmp, ( << "   trying j=" << std::dec << j ) );
          Processor cpu = Processor::getProcessor(j);
          int32_t cpu_num = cpu->readVAddr(VirtualMemoryAddress(cpu_ptr), 4/*NUCLEUS*/, 4); //CPU number at offset 0
          if (cpu_num == (static_cast<int> ( i )) ) {
            bool ignored1, ignored2;
            PhysicalMemoryAddress paddr;
            std::tie( paddr, ignored1, ignored2) = cpu->translateTSB_SimicsImpl(VirtualMemoryAddress(cpu_ptr + 0x18), 4 /*NUCLEUS*/ );
            theIdleThreadTs[i] = paddr;
            DBG_(Dev, ( << "CPU[" << i << "] idle_thread_t point at paddr: " <<  std::hex << theIdleThreadTs[i] << std::dec ) );
            cpu_ptr = VirtualMemoryAddress(cpu->readVAddr(VirtualMemoryAddress(cpu_ptr + 0x48), 4 /*NUCLEUS*/, 8)); //next CPU pointer at offset 0x48
            ok = true;
            //break;
            j = 1000000;
          }
        }
        if (!ok) {
          DBG_(Crit, ( << "No CPU was able to translate the virtual address of the cpu struct for cpu " << i ) );
          return 0;
        }
      }
    }
    uint64_t idle_thread_t_ptr = Processor::getProcessor(0)->readPAddr(theIdleThreadTs[aCPU], 8); //thread_t pointer at offset 0x10
    DBG_(Verb, ( << "IdleThread_t: " << std::hex << idle_thread_t_ptr << std::dec ) );

    return idle_thread_t_ptr;
#else
    return 0;
#endif
  };

  int32_t getPendingTrap( int32_t aCPU ) {
#if FLEXUS_TARGET_IS(v9)
    static const int32_t kTL = 46;
    static const int32_t kTT1 = 78;

    Processor cpu = Processor::getProcessor(aCPU);
    uint32_t tl = cpu->readRegister( kTL );
    if (tl == 0) {
      return 0;
    }
    uint32_t tt = cpu->readRegister( kTT1 + tl - 1);
    return tt;
#else
    return 0;
#endif
  }

  std::string getExecName(int32_t aCPU, uint64_t thread_t_ptr) {
#if FLEXUS_TARGET_IS(v9)
    Processor cpu = Processor::getProcessor(aCPU);
    uint64_t proc_t_ptr = cpu->readVAddr(VirtualMemoryAddress(thread_t_ptr + 0x130), 4 /*NUCLEUS*/, 8); //thread_t pointer at offset 0x130
    std::string str = "";

    if (proc_t_ptr != 0) {
      for (int32_t i = 0; i < 256; i++) {
        char ch = cpu->readVAddr(VirtualMemoryAddress(proc_t_ptr + 0x510 + i), 4, 1);
        if (ch == 0 || ch == ' ') break;
        str.push_back(ch);
      }
    }

    if (str == "") str = "unknown";
    return str;
#else
    return "unknown";
#endif
  }

  //Returns 0 if the PID cannot be determined
  int64_t getPID( int32_t aCPU) {
#if FLEXUS_TARGET_IS(v9)
    VirtualMemoryAddress cpu_ptr( KernelVaddr_cpu_list );  //The virtual address of cpu_list in our kernel
    Processor cpu0 = Processor::getProcessor(0);
    cpu_ptr = VirtualMemoryAddress(cpu0->readVAddr(cpu_ptr, 4 /*NUCLEUS*/, 8));
    for (int32_t i = 0; i < aCPU; ++i) {
      Processor cpu = Processor::getProcessor(i);
      int32_t cpu_num = cpu->readVAddr(VirtualMemoryAddress(cpu_ptr), 4/*NUCLEUS*/, 4); //CPU number at offset 0
      DBG_Assert(cpu_num == i);
      cpu_ptr = VirtualMemoryAddress(cpu->readVAddr(VirtualMemoryAddress(cpu_ptr + 0x48), 4 /*NUCLEUS*/, 8)); //next CPU pointer at offset 0x48
    }
    Processor cpu = Processor::getProcessor(aCPU);
    int32_t cpu_num = cpu->readVAddr(VirtualMemoryAddress(cpu_ptr), 4 /*NUCLEUS*/, 4); //CPU number at offset 0
    DBG_Assert(cpu_num == aCPU );
    uint64_t thread_t_ptr = cpu->readVAddr(VirtualMemoryAddress(cpu_ptr + 0x10), 4 /*NUCLEUS*/, 8); //thread_t pointer at offset 0x10
    uint64_t proc_t_ptr = cpu->readVAddr(VirtualMemoryAddress(thread_t_ptr + 0x130), 4 /*NUCLEUS*/, 8); //thread_t pointer at offset 0x130
    uint64_t pid_t_ptr = cpu->readVAddr(VirtualMemoryAddress(proc_t_ptr + 0xb0), 4 /*NUCLEUS*/, 8); //pid_t pointer at offset 0xb0
    int32_t pid = cpu->readVAddr(VirtualMemoryAddress(pid_t_ptr + 0x4), 4 /*NUCLEUS*/, 4); //pid_t pointer at offset 0xb0
    return pid;
#else
    return 0;
#endif
  }

  void printPIDS( ) {
#if FLEXUS_TARGET_IS(v9)
    for (int32_t i = 0; i < API::SIM_number_processors(); ++i) {
      int32_t td = get_thread_t(i);
      int32_t pid = getPID(i);
      if (pid == 0) {
        std::cout << "CPU[" << i << "] thread_t(" << td << ")" << std::endl;
      } else {
        std::cout << "CPU[" << i << "] thread_t(" << td  << ") pid(" << pid << ")" << std::endl;
      }
    }
#endif
  }

  uint64_t pc(int32_t aCPU) {
    return Processor::getProcessor(aCPU)->getPC();
  }

  void fillBackTrace(int32_t aCPU, std::list<uint64_t> & aTrace) {
#if FLEXUS_TARGET_IS(v9)
    //DBG_(Dev, ( << "Backtrace" ) );
    API::attr_value_t trace = (*theSymTable->stack_trace)(Simics::APIFwd::SIM_get_processor(aCPU), 256);
    DBG_Assert(trace.kind == API::Sim_Val_List);
    for (int32_t i = 0; i < trace.u.list.size; ++i) {
      DBG_Assert(trace.u.list.vector[i].kind == API::Sim_Val_List );
      DBG_Assert( trace.u.list.vector[i].u.list.size >= 1);
      DBG_Assert( trace.u.list.vector[i].u.list.vector[0].kind == API::Sim_Val_Integer);
      aTrace.push_back( trace.u.list.vector[i].u.list.vector[0].u.integer );
      //DBG_(Dev, ( << i << ": " << std::hex << aTrace.back() << std::dec ) );
    }
    API::SIM_free_attribute(trace);
#endif
  }

  bool getState(int32_t aCPU, CPUState & aState) {
#if FLEXUS_TARGET_IS(v9)
    aState.theThread = get_thread_t(aCPU);
    if (aState.theThread) {
      aState.theExecName = getExecName(aCPU, aState.theThread);
      aState.thePC = pc(aCPU);
      aState.theTrap = getPendingTrap(aCPU);
      fillBackTrace(aCPU, aState.theBackTrace);
      return true;
    }
#endif
    return false;
  }

};

class WhiteBox_SimicsObject_Impl  {
  boost::intrusive_ptr< WhiteBox > theWhiteBox;
public:
  WhiteBox_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) {}

  void setWhiteBox ( boost::intrusive_ptr<WhiteBox> aWhiteBox ) {
#if FLEXUS_TARGET_IS(v9)
    theWhiteBox = aWhiteBox;
#endif
  }

  void print_thread_t(int32_t aCPU) {
#if FLEXUS_TARGET_IS(v9)
    uint64_t thread_t = theWhiteBox->get_thread_t(aCPU);
    std::cout << std::hex << thread_t << std::dec << std::endl;
#endif
  }

  void testBackTrace(int32_t aCPU) {
#if FLEXUS_TARGET_IS(v9)
    CPUState foo;
    theWhiteBox->fillBackTrace(aCPU, foo.theBackTrace);
#endif
  }

  void printPIDS() {
    theWhiteBox->printPIDS();
  }
};

class WhiteBox_SimicsObject : public Simics::AddInObject <WhiteBox_SimicsObject_Impl> {

  typedef Simics::AddInObject<WhiteBox_SimicsObject_Impl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "WhiteBox";
  }
  static std::string classDescription() {
    return "WhiteBox object";
  }

  WhiteBox_SimicsObject() : base() { }
  WhiteBox_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  WhiteBox_SimicsObject(WhiteBox_SimicsObject_Impl * anImpl) : base(anImpl) {}

  template <class Class>
  static void defineClass(Class & aClass) {

#if FLEXUS_TARGET_IS(v9)
    aClass.addCommand
    ( & WhiteBox_SimicsObject_Impl::print_thread_t
      , "print-active-thread"
      , "Prints the virtual address of the thread_t structure for the active thread on a cpu"
      , "cpu"
    );

    aClass.addCommand
    ( & WhiteBox_SimicsObject_Impl::printPIDS
      , "print-active-pids"
      , "Prints the active pids on each CPU, where available"
    );

    aClass.addCommand
    ( & WhiteBox_SimicsObject_Impl::testBackTrace
      , "test-back-trace"
      , "Prints the back trace of a CPU"
      , "cpu"
    );
#endif
  }

};

Simics::Factory<WhiteBox_SimicsObject> theWhiteBoxFactory;

boost::intrusive_ptr<WhiteBox> theWhiteBox;

boost::intrusive_ptr<WhiteBox> WhiteBox::getWhiteBox() {
  return theWhiteBox;
}

class FLEXUS_COMPONENT(WhiteBox) {
  FLEXUS_COMPONENT_IMPL(WhiteBox);

  WhiteBox_SimicsObject theWhiteBoxObject;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(WhiteBox)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
#if FLEXUS_TARGET_IS(v9)
    DBG_(Dev, ( << "Creating WhiteBox" ) );
    theWhiteBox = new WhiteBoxImpl;
    theWhiteBoxObject = theWhiteBoxFactory.create("white-box");
    theWhiteBoxObject->setWhiteBox(theWhiteBox);
#endif
  }

  bool isQuiesced() const {
    return true; //MagicBreakComponent is always quiesced
  }

  void saveState(std::string const & aDirName) {
  }

  void loadState(std::string const & aDirName) {
  }

  void initialize() {
  }

  void finalize() {}

};
}//End namespace nMagicBreak

FLEXUS_COMPONENT_INSTANTIATOR( WhiteBox, nWhiteBox);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT WhiteBox

#define DBG_Reset
#include DBG_Control()
