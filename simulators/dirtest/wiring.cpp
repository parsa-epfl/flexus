
#define FLEXUS_WIRING_FILE
#include <core/simulator_layout.hpp>


//This section contains the name of the simulator
#include <core/simulator_name.hpp>
namespace Flexus {
//Simulator Name
std::string theSimulatorName = "DirTest";
}


#include FLEXUS_BEGIN_DECLARATION_SECTION()

#include <components/Directory/Directory.hpp>
#include <components/Directory/DirTest.hpp>

#include FLEXUS_END_DECLARATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION()

CREATE_CONFIGURATION( Directory, "directory", theDirCfg );
CREATE_CONFIGURATION( DirTest, "dir_test", theDirTestCfg );

//You may optionally initialize configuration parameters from within this
//function.  This initialization occur before the command line is processed,
//so they will be overridden from the command line.
//
//Return value indicates whether simulation should abort if any parameters
//are left at their default values;
bool initializeParameters() {
  theDirCfg.Latency.initialize(5);

  return false; //Abort simulation if parameters are not initialized
}

#include FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION()
//All component Instances are created here.  This section
//also creates handles for each component

FLEXUS_INSTANTIATE_COMPONENT_ARRAY( Directory, theDirCfg, theDirectory, SCALE_WITH_SYSTEM_WIDTH, MULTIPLY, 1);
FLEXUS_INSTANTIATE_COMPONENT( DirTest, theDirTestCfg, theDirTest);

#include FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()


#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

WIRE( theDirTest, RequestOut, theDirectory, DirRequestIn )
WIRE( theDirectory, DirReplyOut, theDirTest, ResponseIn )

#include FLEXUS_END_COMPONENT_WIRING_SECTION()

#include FLEXUS_BEGIN_DRIVE_ORDER_SECTION()

DRIVE( theDirTest, DirTestDrive )
, DRIVE( theDirectory, DirectoryDrive )

#include FLEXUS_END_DRIVE_ORDER_SECTION()
