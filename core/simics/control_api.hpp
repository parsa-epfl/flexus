#ifndef FLEXUS_SIMICS_CONTROL_API_HPP_INCLUDED
#define FLEXUS_SIMICS_CONTROL_API_HPP_INCLUDED

#include <core/simics/api_wrappers.hpp>

namespace Flexus {
namespace Simics {

namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#include FLEXUS_SIMICS_API_HEADER(event)
}
}

inline void BreakSimulation(char const * aMessage) {
  APIFwd::SIM_break_simulation(aMessage);
}

inline void WriteCheckpoint(char const * aFilename) {
  APIFwd::SIM_write_configuration_to_file(aFilename);
}

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_SIMICS_HAP_API_HPP_INCLUDED

