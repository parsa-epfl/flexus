#include <string>
#include <vector>

#include <core/qemu/api_wrappers.hpp>
#include <core/target.hpp>

#include <core/qemu/configuration_api.hpp>

namespace Flexus {
namespace Qemu {

namespace aux_ {
API::conf_class_t *RegisterClass_stub(std::string const &name, API::class_data_t *class_data) {
  return new API::conf_class_t;
}

API::conf_object_t *NewObject_stub(API::conf_class_t *aClass, std::string const &aName) {
  return new API::conf_object_t;
}

} // namespace aux_

} // namespace Qemu
} // namespace Flexus
