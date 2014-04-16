#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>

#include <core/debug/debug.hpp>

#include <core/target.hpp>
#include <core/simulator_name.hpp>
#include <core/configuration.hpp>
#include <core/component.hpp>

#include <core/simics/simics_interface.hpp>

namespace Flexus {

namespace Core {
void Break() {
  Flexus::Simics::BreakSimulation("Simulation break caused by debugger.");  //Does nothing
}
void CreateFlexusObject();
void PrepareFlexusObject();
}

namespace Simics {

using namespace Flexus::Core;
namespace Simics = Flexus::Simics;

Simics::SimicsInterface_Obj theSimicsInterface;

Simics::SimicsInterfaceFactory * theSimicsInterfaceFactory;

void CreateFlexus() {
  CreateFlexusObject();

  theSimicsInterface = theSimicsInterfaceFactory->create("flexus-simics-interface");
  if (!theSimicsInterface) {
    throw SimicsException("Unable to create SimicsInterface object in Simics");
  }

  Flexus::Core::ComponentManager::getComponentManager().instantiateComponents( theSimicsInterface->getSystemWidth() );
  ConfigurationManager::getConfigurationManager().processCommandLineConfiguration(0, 0);
}

void PrepareFlexus() {
  PrepareFlexusObject();
  theSimicsInterfaceFactory = new Simics::SimicsInterfaceFactory ();

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
  cerr << "Flexus Simics simulator - Built as " << Flexus::theSimulatorName << endl << endl;
}

}

extern "C" void init_local(void) {
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
  Flexus::Simics::PrepareFlexus();

  DBG_(Iface, ( << "Flexus Initialized." ));
}

extern "C" void fini_local(void) {
  //Theoretically, we would delete Flexus here, but Simics currently does not call this function.
  //delete theFlexusFactory;
}
