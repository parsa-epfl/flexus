#ifndef FLEXUS_CORE_SIMICS_API_WRAPPERS_HPP__INCLUDED
#define FLEXUS_CORE_SIMICS_API_WRAPPERS_HPP__INCLUDED

namespace Flexus {
namespace Simics {

namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict
#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_ARCH_HEADER
}
} //namespace API

namespace APIFwd {

#if SIM_VERSION > 1300
typedef API::TARGET_MEM_TRANS  memory_transaction_t;
#else
typedef API::memory_transaction_t memory_transaction_t;
#endif

Simics::API::conf_class_t * SIM_register_class(const char * name, Simics::API::class_data_t * class_data);

int SIM_register_attribute(
  Simics::API::conf_class_t * class_struct
  , const char * attr_name
  , Simics::API::get_attr_t get_attr
  , Simics::API::lang_void * get_attr_data
  , Simics::API::set_attr_t set_attr
  , Simics::API::lang_void * set_attr_data
  , Simics::API::attr_attr_t attr
  , const char * doc
);

void SIM_object_constructor(Simics::API::conf_object_t * conf_obj, Simics::API::parse_object_t * parse_obj);

Simics::API::conf_object_t * SIM_new_object(Simics::API::conf_class_t * conf_class, const char * instance_name);

void SIM_break_simulation(const char * aMessage);
void SIM_write_configuration_to_file(const char * aFilename);

Simics::API::conf_object_t * SIM_get_processor(int aCPUNum);

int SIM_get_processor_number(Simics::API::conf_object_t  * aProcessor);

void SIM_flush_all_caches();

} //namespace APIFwd

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_CORE_SIMICS_API_WRAPPERS_HPP__INCLUDED

