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
#include "core/qemu/qemu_interface.h"
}

namespace Flexus {

namespace Core {
void Break() {
	// tell Qemu to pause simulation.
}
void CreateFlexusObject();
void PrepareFlexusObject();
}

namespace Qemu {

using namespace Flexus::Core;
namespace Qemu = Flexus::Qemu;


void CreateFlexus() {
  CreateFlexusObject();
  Flexus::Core::index_t systemWidth = 1;
  Flexus::Core::ComponentManager::getComponentManager().instantiateComponents(systemWidth);
  ConfigurationManager::getConfigurationManager().processCommandLineConfiguration(0, 0);
}

void PrepareFlexus() {
  PrepareFlexusObject();
  CallOnConfigReady(CreateFlexus);
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
  cerr << "Flexus Qemu simulator - Built as " << Flexus::theSimulatorName << endl << endl;
}

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

  DBG_(Iface, ( << "Flexus Initialized." ));
}

extern "C" void qemuflex_fini(void) {
  delete theFlexusFactory;
}
