#ifndef FLEXUS_CORE_QEMU_API_WRAPPERS_HPP__INCLUDED
#define FLEXUS_CORE_QEMU_API_WRAPPERS_HPP__INCLUDED

#include <stdbool.h>
#include <stdint.h>

namespace Flexus {
namespace Qemu {
namespace API {
#include <core/qemu/api.h>

void
QEMU_write_configuration_to_file(const char* aFilename);
} // namespace API
} // namespace Qemu
} // namespace Flexus

#endif // FLEXUS_CORE_QEMU_API_WRAPPERS_HPP__INCLUDED
