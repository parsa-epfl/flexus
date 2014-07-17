#include <core/target.hpp>
#include <core/qemu/api_wrappers.hpp>

namespace Flexus {
namespace Qemu {
namespace API {
extern "C" {
#include <core/qemu/api.h>
}
} //namespace API

namespace APIFwd {

void QEMU_break_simulation(const char * aMessage) {
  return Qemu::API::QEMU_break_simulation(aMessage);
}

void QEMU_write_configuration_to_file(const char * aFilename) {
  /*Qemu::API::SIM_write_configuration_to_file(aFilename);*/
	// XXX: Does nothing.
}

Qemu::API::conf_object_t * QEMU_get_processor(int aCPUNum) {
	return Qemu::API::QEMU_get_cpu_by_index(aCPUNum);
}

int QEMU_get_processor_number(Qemu::API::conf_object_t  * aProcessor) {
	return Qemu::API::QEMU_get_processor_number(aProcessor);
}

void QEMU_flush_all_caches() {
	// XXX: Does nothing.
	return;
}

} //namespace APIFwd
} //End Namespace Qemu
} //namespace Flexus

