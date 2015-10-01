#ifndef CONFIG_QEMU

#include <string>
#include <vector>

#include <core/debug/debug.hpp>

#include <core/target.hpp>

#include <core/simics/configuration_api.hpp>
#include <core/simics/aux_/simics_command_manager.hpp>

namespace Flexus {
namespace Simics {
namespace aux_ {

SimicsCommandManager * SimicsCommandManager::get() {
  static SimicsCommandManager theManager;
  return &theManager;
}

SimicsCommandManager::SimicsCommandManager() {
}

void SimicsCommandManager::invoke( int32_t aCommandId ) { }

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker ) { }

std::string SimicsCommandManager::arg_class_to_string(int32_t anArgClass) {
  if ( anArgClass == SimicsCommandManager::string_class ) {
    return "str_t";
  } else if ( anArgClass == SimicsCommandManager::int_class ) {
    return "int_t";
  } else if ( anArgClass == SimicsCommandManager::flag_class ) {
    return "flag_t";
  } else if ( anArgClass == SimicsCommandManager::address_class ) {
    return "addr_t";
  } else if ( anArgClass == SimicsCommandManager::file_class ) {
    return "filename_t()";
  } else {
    return "str_t";
  }
}

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int32_t anArg1Class) { }

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int32_t anArg1Class, std::string const & anArg2Name,  int32_t anArg2Class ) { }

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int32_t anArg1Class, std::string const & anArg2Name,  int32_t anArg2Class, std::string const & anArg3Name,  int32_t anArg3Class  ) { }

} //namespace aux_
} //namespace Simics
} //namespace Flexus
#endif //CONFIG_QEMU
