#ifndef FLEXUS_SIMICS_AUX__COMMAND_MANAGER_HPP_INCLUDED
#define FLEXUS_SIMICS_AUX__COMMAND_MANAGER_HPP_INCLUDED

#include <string>

#include <core/debug/debug.hpp>

#include <core/simics/trampoline.hpp>
#include <core/exception.hpp>
#include <core/simics/api_wrappers.hpp>

namespace Flexus {
namespace Simics {

struct BaseClassImpl;

typedef API::set_error_t (*SimicsSetFnT)(void *, API::conf_object_t *, API::attr_value_t *, API::attr_value_t *);
typedef API::attr_value_t (*SimicsGetFnT)(void *, API::conf_object_t *, API::attr_value_t *);

namespace aux_ {

//Helper functions
API::conf_class_t * RegisterClass_stub(std::string const & name, API::class_data_t * class_data);

class SimicsCommandManager {
public:
  static const int string_class = 0;
  static const int int_class = 1;
  static const int flag_class = 2;
  static const int address_class = 3;
  static const int file_class = 4;
  static const int ull_class = 5;

  API::conf_class_t * theGatewayClass; //Simics class data structure for gateway
  API::conf_object_t * theGateway; //Simics class data structure for gateway

  //"Closure" variables used to pass parameters through the gateway
  API::conf_object_t * theObject;
  API::attr_value_t theParameter1;
  API::attr_value_t theParameter2;
  API::attr_value_t theParameter3;
  API::attr_value_t theReturnValue;

  typedef boost::function< void ( API::conf_object_t *, API::attr_value_t const & , API::attr_value_t const & , API::attr_value_t const & ) > Invoker;

  //Each of these vectors contain all the commands with a particular signature
  std::vector< Invoker > theCommands;

  static SimicsCommandManager * get();

  void invoke( int aCommandId );

  SimicsCommandManager();

  //Add a command to the simics CLI
  void addCommand
  ( std::string const & aNamespaceName
    , std::string const & aFrontendCommand
    , std::string const & aCommandDescription
    , Invoker anInvoker
  );
  void addCommand
  ( std::string const & aNamespaceName
    , std::string const & aFrontendCommand
    , std::string const & aCommandDescription
    , Invoker anInvoker
    , std::string const & anArg1Name
    , int anArg1Class
  );
  void addCommand
  ( std::string const & aNamespaceName
    , std::string const & aFrontendCommand
    , std::string const & aCommandDescription
    , Invoker anInvoker
    , std::string const & anArg1Name
    , int anArg1Class
    , std::string const & anArg2Name
    , int anArg2Class
  );
  void addCommand
  ( std::string const & aNamespaceName
    , std::string const & aFrontendCommand
    , std::string const & aCommandDescription
    , Invoker anInvoker
    , std::string const & anArg1Name
    , int anArg1Class
    , std::string const & anArg2Name
    , int anArg2Class
    , std::string const & anArg3Name
    , int anArg3Class
  );

private:
  std::string arg_class_to_string(int anArgClass);

};

template <class ArgType>
struct SimicsCommandArgClass;

template <>
struct SimicsCommandArgClass<std::string> {
  static const int value = SimicsCommandManager::string_class;
};

template <>
struct SimicsCommandArgClass<std::string const &> {
  static const int value = SimicsCommandManager::string_class;
};

template <>
struct SimicsCommandArgClass<int> {
  static const int value = SimicsCommandManager::int_class;
};

template <>
struct SimicsCommandArgClass<bool> {
  static const int value = SimicsCommandManager::flag_class;
};

template <>
struct SimicsCommandArgClass<unsigned long long> {
  static const int value = SimicsCommandManager::ull_class;
};

} //aux_
} //namespace Simics
} //namespace Flexus

#endif //FLEXUS_SIMICS_AUX__COMMAND_MANAGER_HPP_INCLUDED
