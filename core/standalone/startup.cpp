// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block   
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

