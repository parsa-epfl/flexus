#include <iostream>
#include <cstring>
#include <cstdlib>

#include <core/target.hpp>
#include <core/simics/api_wrappers.hpp>

namespace Flexus {
namespace Simics {
namespace APIFwd {

Simics::API::conf_class_t * SIM_register_class(const char * name, Simics::API::class_data_t * class_data) {
  std::cout << "Error, called unemulated Simics::API method: " << __FUNCTION__ << std::endl;
  return 0;
}

int32_t SIM_register_attribute(
  Simics::API::conf_class_t * class_struct
  , const char * attr_name
  , Simics::API::get_attr_t get_attr
  , Simics::API::lang_void * get_attr_data
  , Simics::API::set_attr_t set_attr
  , Simics::API::lang_void * set_attr_data
  , Simics::API::attr_attr_t attr
  , const char * doc
) {
  //Does nothing for standalone targets
  return 0;
}

void SIM_object_constructor(Simics::API::conf_object_t * conf_obj, Simics::API::parse_object_t * parse_obj) {
  //Does nothing for standalone targets
}

Simics::API::conf_object_t * SIM_new_object(Simics::API::conf_class_t * conf_class, const char * instance_name) {
  Simics::API::conf_object_t * ret_val = new Simics::API::conf_object_t;
  ret_val->name = strdup(instance_name);
  ret_val->class_data = 0;
  ret_val->queue = 0;
  ret_val->object_id = 0;
  return ret_val;
}

void SIM_break_simulation(const char * aMessage) {
  std::cout << "Break requested by debugger.";
  std::abort();
}

void SIM_write_configuration_to_file(const char * aFilename) {
  //Does nothing for standalone targets
}

} //namespace APIFwd
} //End Namespace Simics
} //namespace Flexus

