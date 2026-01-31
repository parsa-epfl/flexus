#ifndef FLEXUS_QEMU_AUX__COMMAND_MANAGER_HPP_INCLUDED
#define FLEXUS_QEMU_AUX__COMMAND_MANAGER_HPP_INCLUDED

#include <core/debug/debug.hpp>
#include <core/exception.hpp>
#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/trampoline.hpp>
#include <string>
namespace Flexus {
namespace Qemu {

struct BaseClassImpl;

namespace aux_ {

// Helper functions
API::conf_class_t*
RegisterClass_stub(std::string const& name);

class QemuCommandManager
{
  public:
    API::conf_object_t* theGateway; // Qemu class data structure for gateway

    //"Closure" variables used to pass parameters through the gateway
    API::conf_object_t* theObject;

    static QemuCommandManager* get();

    QemuCommandManager();
};

} // namespace aux_
} // namespace Qemu
} // namespace Flexus

#endif // FLEXUS_QEMU_AUX__COMMAND_MANAGER_HPP_INCLUDED
