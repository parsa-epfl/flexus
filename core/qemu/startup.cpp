#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>

#include <core/debug/debug.hpp>

#include <core/target.hpp>
#include <core/simulator_name.hpp>
#include <core/configuration.hpp>
#include <core/component.hpp>
#include <core/qemu/qemu.h>

#define QEMUFLEX_FLEXUS_INTERNAL
namespace Flexus{
namespace Qemu{
namespace API{
#include <core/qemu/api.h>
} // API
} // Qemu
} // Flexus

// For debug purposes
#include <iostream>

namespace Flexus {

namespace Core {
void Break() {
	// QEMU: halt simulation and print message
}
void CreateFlexusObject();
void PrepareFlexusObject();
void initFlexus();
void deinitFlexus();
void callQMP(Flexus::Qemu::API::qmp_flexus_cmd_t aCMD, const char* args);
void startTimingFlexus();
void handleInterruptFlexus(void* anObj,long long aVector);
}

namespace Qemu {

using namespace Flexus::Core;
namespace Qemu = Flexus::Qemu;

void CreateFlexus() {
  CreateFlexusObject();

  Flexus::Core::index_t systemWidth = Qemu::API::QEMU_get_num_cpus();
  Flexus::Core::ComponentManager::getComponentManager()
                                .instantiateComponents(systemWidth);
 
  ConfigurationManager::getConfigurationManager()
                        .processCommandLineConfiguration(0, 0);
   //not sure if this is correct or where it should be. 
 std::cerr<<"systemWidth: " <<systemWidth << std::endl;
}

void PrepareFlexus() {
  PrepareFlexusObject();
  Qemu::API::QEMU_insert_callback(QEMUFLEX_GENERIC_CALLBACK, Qemu::API::QEMU_config_ready, nullptr, (void*)&CreateFlexus);


}

extern "C" void qmp_call(Flexus::Qemu::API::qmp_flexus_cmd_t aCMD, const char* anArgs) {
    callQMP(aCMD, anArgs);
}

extern "C" void flexus_init(void) {
    initFlexus();
}

extern "C" void flexus_deinit(void) {
    deinitFlexus();
}

extern "C" void start_timing_sim(void) {
    startTimingFlexus();
}

extern "C" void handle_interruptTwo(void * anObj, long long aVector){
    std::cerr << "in the second handle interrupt\n"<<std::endl;
    //onInterrupt(0 , anObj, aVector);
    handleInterruptFlexus(anObj, aVector);
}

} //end namespace Core
} //end namespace Flexus

namespace {

using std::cerr;
using std::endl;

void print_copyright() {
  cerr << "\nFlexus (C) 2006-2010 The SimFlex Project" << endl;
  cerr << "Eric Chung, Michael Ferdman, Brian Gold, Nikos Hardavellas, Jangwook Kim," << endl;
  cerr << "Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi," << endl;
  cerr << "Evangelos Vlachos, Thomas Wenisch, Roland Wunderlich" << endl;
  cerr << "Anastassia Ailamaki, Babak Falsafi and James C. Hoe." << endl << endl;
  cerr << "Flexus Simics simulator - Built as " << Flexus::theSimulatorName << endl << endl;
}



}

extern "C" void qmpcall(Flexus::Qemu::API::qmp_flexus_cmd_t aCMD, const char* anArgs ){
    Flexus::Qemu::qmp_call(aCMD, anArgs);
}


extern "C" void flexInit(){
    Flexus::Qemu::flexus_init();
}

extern "C" void flexDeinit(){
    Flexus::Qemu::flexus_deinit();
}

extern "C" void startTiming(){
    Flexus::Qemu::start_timing_sim();
}

extern "C" void handleInterrupt(void* anObj, long long aVector){
    cerr << "Flexus: in handle interrupt\n"<<endl;
    if (Flexus::Qemu::handle_interruptTwo != NULL){
      cerr << "Flexus: handle_interrupt is not null " << std::hex << (void *) Flexus::Qemu::handle_interruptTwo << "\n"<<endl;
      Flexus::Qemu::handle_interruptTwo(anObj, aVector);
      //cerr << "Flexus: flexInit is not null " << std::hex << (void *) Flexus::Qemu::flexus_init << "\n"<<endl;
      //Flexus::Qemu::flexus_init();
      //exit(-1);
    }else{
      cerr << "Flexus: handle_interrupt is null\n"<<endl;
    }
}

extern "C" void qflex_init(Flexus::Qemu::API::QFLEX_API_Interface_Hooks_t* hooks) {
  Flexus::Qemu::API::QFLEX_API_set_interface_hooks( hooks );
  std::cerr << "Entered init_local\n";

  print_copyright();

  if (getenv("WAITFORSIGCONT")) {
    std::cerr << "Waiting for SIGCONT..." << std::endl;
    std::cerr << "Attach gdb with the following command and 'c' from the gdb prompt:" << std::endl;
    std::cerr << "  gdb - " << getpid() << std::endl;
    raise(SIGSTOP);
  }

  DBG_(Dev, ( << "Initializing Flexus." ));

  //Do all the stuff we need to get Simics to know we are here
  Flexus::Qemu::PrepareFlexus();

  DBG_(Iface, ( << "Flexus Initialized." ));
}

extern "C" void qflex_quit(void) {
    flexDeinit();
}
