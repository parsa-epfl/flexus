#include <vector>
#include <string>

#include <core/target.hpp>
#include <core/qemu/api_wrappers.hpp>

#include <core/qemu/configuration_api.hpp>

namespace Flexus {
namespace Qemu {
namespace API {
extern "C" {
#include <core/qemu/api.h>
}
}

namespace API = Flexus::Qemu::API;

namespace aux_ {
API::conf_class_t * RegisterClass_stub(std::string const & name, 
		API::class_data_t * class_data) 
{
	return new conf_class_t;
}

API::conf_object_t * NewObject_stub(API::conf_class_t * aClass, 
		std::string const & aName) 
{
	return new conf_object_t;
}

} //aux_

} //Qemu
} //Flexus

