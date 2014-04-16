#include <vector>
#include <string>
#include <iostream>

#include <core/target.hpp>
#include <core/simics/api_wrappers.hpp>

namespace Flexus {
namespace Simics {

void runPython(std::vector<std::string> & aCommandVector) {
  //Ignore runPython calls in standalone simulators
  return;
}

namespace aux_ {
API::conf_class_t * RegisterClass_stub(std::string const & name, API::class_data_t * class_data) {
  return 0;
}

API::conf_object_t * NewObject_stub(API::conf_class_t * aClass, std::string const & aName, API::conf_object_t * (&constructor)(API::parse_object_t *) ) {
  return constructor(0);
}

} //aux_

} //Simics
} //Flexus
