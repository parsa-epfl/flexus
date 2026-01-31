#include <core/debug/debug.hpp>
#include <core/qemu/aux_/qemu_command_manager.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/target.hpp>
#include <string>
#include <vector>
namespace Flexus {
namespace Qemu {
namespace aux_ {

API::conf_object_t*
QemuCommandManager_constructor()
{
    DBG_(Verb, (<< "QemuCommandManager constructor."));
    API::conf_object* object = new API::conf_object;
    return object;
}

int
QemuCommandManager_destructor(API::conf_object_t* anInstance)
{
    delete anInstance;
    return 0;
}

QemuCommandManager*
QemuCommandManager::get()
{
    static QemuCommandManager theManager;
    return &theManager;
}

QemuCommandManager::QemuCommandManager()
{
    // Set up all the "closure" variables used for passing parameters
    // through the gateway (just one!)
    theObject = 0;
}

} // namespace aux_
} // namespace Qemu
} // namespace Flexus
