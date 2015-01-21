#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>

#include <core/debug/debug.hpp>

#include <core/target.hpp>
#include <core/simulator_name.hpp>
#include <core/configuration.hpp>
#include <core/component.hpp>

extern "C" {
#include <core/qemu/api.h>
}

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
}

namespace Qemu {

using namespace Flexus::Core;
namespace Qemu = Flexus::Qemu;

void CreateFlexus() {
  CreateFlexusObject();

  Flexus::Core::index_t systemWidth = QEMU_get_num_cpus();
  Flexus::Core::ComponentManager::getComponentManager()
								.instantiateComponents(systemWidth);
 
  ConfigurationManager::getConfigurationManager()
						.processCommandLineConfiguration(0, 0);
   //not sure if this is correct or where it should be. 
 std::cerr<<"systemWidth: " <<systemWidth << std::endl;
}

void PrepareFlexus() {
  PrepareFlexusObject();
  QEMU_insert_callback(QEMUFLEX_GENERIC_CALLBACK, QEMU_config_ready, NULL, (void*)&CreateFlexus);
}
extern "C" void flexus_init(void) {
    initFlexus();
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
extern "C" void flexInit(){
    Flexus::Qemu::flexus_init();
}

extern "C" void qemuflex_init(void) {
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

extern "C" void qemuflex_quit(void) {
  //Theoretically, we would delete Flexus here, but Qemu currently does not call this function.
  // delete theFlexusFactory;
}
