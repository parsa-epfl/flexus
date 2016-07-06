#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>

#include <core/debug/debug.hpp>

#include <core/target.hpp>
#include <core/simulator_name.hpp>
#include <core/configuration.hpp>
#include <core/component.hpp>
#include <boost/version.hpp>

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
void startTimingFlexus(); 
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
extern "C" void flexus_init(void) {
    initFlexus();
}
extern "C" void start_timing_sim(void) {
    startTimingFlexus(); 
}
} //end namespace Core
} //end namespace Flexus

namespace {

using std::cerr;
using std::endl;

void print_copyright() {
  cerr << "\nFlexus (C) 2006-2016 The QFlex Project" << endl;
  cerr << "Alexandros Daglis, Arash Pourhabibi, Damien Hilloulin," << endl;
  cerr << "Dmitrii Ustiugov, Eric Chung, Mario Drumond, Michael Ferdman," << endl;
  cerr << "Brian Gold, Nikos Hardavellas, Nooshin Mirzadeh, Jangwook Kim, Javier Picorel," << endl;
  cerr << "Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi," << endl;
  cerr << "Evangelos Vlachos, Thomas Wenisch, Roland Wunderlich" << endl;
  cerr << "Anastassia Ailamaki, Babak Falsafi and James C. Hoe." << endl << endl;
  cerr << "QFlex simulator - Built as " << Flexus::theSimulatorName << endl << endl;
}



}
extern "C" void flexInit(){
    Flexus::Qemu::flexus_init();
}

extern "C" void startTiming(){
    Flexus::Qemu::start_timing_sim();
}

extern "C" void qemuflex_init(Flexus::Qemu::API::QFLEX_API_Interface_Hooks_t* hooks) {
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
  DBG_(Dev, ( << "Compiled with Boost: " << BOOST_VERSION / 100000 << "."
              << BOOST_VERSION / 100 % 1000 << "." << BOOST_VERSION % 100 ));

  //Do all the stuff we need to get Simics to know we are here
  Flexus::Qemu::PrepareFlexus();

  DBG_(Iface, ( << "Flexus Initialized." ));
}

extern "C" void qemuflex_quit(void) {
  //Theoretically, we would delete Flexus here, but Qemu currently does not call this function.
  // delete theFlexusFactory;
}
