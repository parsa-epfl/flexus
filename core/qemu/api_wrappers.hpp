#ifndef FLEXUS_CORE_QEMU_API_WRAPPERS_HPP__INCLUDED
#define FLEXUS_CORE_QEMU_API_WRAPPERS_HPP__INCLUDED

namespace Flexus {
namespace Qemu {

namespace API {
extern "C" {
#include <core/qemu/api.h>
}
} //namespace API

namespace APIFwd {

void QEMU_break_simulation(const char * aMessage);
void QEMU_write_configuration_to_file(const char * aFilename);

Qemu::API::conf_object_t * QEMU_get_processor(int aCPUNum);

int QEMU_get_processor_number(Qemu::API::conf_object_t  * aProcessor);

void QEMU_flush_all_caches();

} //namespace APIFwd

}  //End Namespace Qemu
} //namespace Flexus

#endif //FLEXUS_CORE_QEMU_API_WRAPPERS_HPP__INCLUDED

