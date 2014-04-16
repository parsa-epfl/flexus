#include <vector>
#include <string>

#include <core/target.hpp>
#include <core/simics/api_wrappers.hpp>

#include <core/simics/configuration_api.hpp>

namespace Flexus {
namespace Simics {

void runPython(std::vector<std::string> & aCommandVector) {
  API::conf_object_t * python = API::SIM_get_object("python");

  std::vector<std::string>::iterator iter = aCommandVector.begin();
  while (iter != aCommandVector.end()) {
    AttributeValue python_string(iter->c_str());
    API::attr_value_t python_val = python_string;
    API::SIM_set_attribute(python, "execute-string", &python_val);
    ++iter;
  }
}

namespace aux_ {
API::conf_class_t * RegisterClass_stub(std::string const & name, API::class_data_t * class_data) {
  API::conf_class_t * ret_val;
  if ((ret_val = APIFwd::SIM_register_class(name.c_str(), class_data)) == 0) {
    throw SimicsException(string("Unable to register class with Simics:") + name);
  }
  return ret_val;
}

API::conf_object_t * NewObject_stub(API::conf_class_t * aClass, std::string const & aName, API::conf_object_t * (&constructor)(API::parse_object_t *) ) {
  return APIFwd::SIM_new_object(aClass, aName.c_str());
}

} //aux_

} //Simics
} //Flexus

