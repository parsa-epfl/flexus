/*! \file simulator_name.hpp
    \brief Forward declaration of theSimulatorName

    extern declaration of theSimulatorName.  Used in the Standalone and
    Simics cores to be able to print out the name of the simulator.

    Revision History:
     - twenisch 12Jul02 Initial Revision
 */

#ifndef FLEXUS_SIMULATOR_NAME_HPP_INCLUDED
#define FLEXUS_SIMULATOR_NAME_HPP_INCLUDED

#include <string>

namespace Flexus {

extern std::string theSimulatorName;

} // namespace Flexus

#endif // FLEXUS_SIMULATOR_NAME_HPP_INCLUDED
