#include <iostream>
#include <cstdlib>

#include <core/simulator_name.hpp>
#include <core/metaprogram.hpp>
#include <core/drive_reference.hpp>
#include <core/configuration.hpp>
#include <core/target.hpp>
#include <core/flexus.hpp>
#include <core/configuration_macros.hpp>
#include <core/component.hpp>

using std::cout;
using std::cerr;
using std::endl;

using namespace Flexus::Core;
using Flexus::Wiring::theDrive;

namespace Simics = Flexus::Simics;

namespace Flexus {
namespace Core {
void Break() {
  std::exit(0);
} //Does nothing
void CreateFlexusObject();
void PrepareFlexusObject();

}
}

namespace {

void print_copyright() {
  cerr << "\nFlexus (C) 2005 The SimFlex Project" << endl;
  cerr << "Brian Gold, Nikos Hardavellas, Jangwook Kim, Jared Smolens," << endl;
  cerr << "Stephen Somogyi, Thomas Wenisch, Babak Falsafi and James C. Hoe." << endl << endl;
  cerr << "Standalone Flexus simulator - Built as " << Flexus::theSimulatorName << endl << endl;
}
}

FLEXUS_DECLARE_COMPONENT_PARAMETERS(Core,
                                    FLEXUS_PARAMETER( MaxCycles, uint64_t, "Max Cycles", "max", 0 )
                                    FLEXUS_PARAMETER( SystemWidth, uint32_t, "System Width", "width", 1 )
                                   );

int32_t main(int32_t argc, char ** argv) {
  print_copyright();

  CoreConfiguration CoreCfg("core");

  try {
    //Initialize components
    Flexus::Core::PrepareFlexusObject();
    Flexus::Core::CreateFlexusObject();

    //Read in the configuration
    ConfigurationManager::getConfigurationManager().processCommandLineConfiguration(argc, argv);

    Flexus::Core::ComponentManager::getComponentManager().instantiateComponents( CoreCfg.cfg().SystemWidth );
    Flexus::Core::theFlexus->initializeComponents();
  } catch (std::exception & anException) {
    cout << "Exception initializing components: " << anException.what() << endl;
  }

  try {
    for (uint64_t i = 0; CoreCfg.cfg().MaxCycles == 0 || i < CoreCfg.cfg().MaxCycles; ++i ) {
      //Call the simulator cycle function
      theFlexus->doCycle();
    }
  } catch (std::exception & anException) {
    cout << "Exception in doCycle: " << anException.what() << endl;
  }

  return 0;
}

