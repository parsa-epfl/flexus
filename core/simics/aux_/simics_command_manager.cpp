#include <string>
#include <vector>

#include <core/debug/debug.hpp>

#include <core/target.hpp>

#include <core/simics/configuration_api.hpp>
#include <core/simics/aux_/simics_command_manager.hpp>

namespace Flexus {
namespace Simics {
namespace aux_ {

API::set_error_t invoke_SetFn( void * aCommandManager, API::conf_object_t * /*ignored*/, API::attr_value_t * aCommandId, API::attr_value_t * /*ignored*/) {
  DBG_Assert( aCommandManager );
  DBG_Assert( aCommandId );
  DBG_Assert( aCommandId->kind == API::Sim_Val_Integer );
  DBG_( Verb, ( << "Call Gate: invoke " << aCommandId->u.integer ) );
  reinterpret_cast<SimicsCommandManager *>(aCommandManager)->invoke( aCommandId->u.integer);
  return API::Sim_Set_Ok;
}

API::attr_value_t invoke_GetFn( void * aCommandManager, API::conf_object_t * /*ignored*/, API::attr_value_t * /* ignored */) {
  DBG_Assert( aCommandManager );
  DBG_( Verb, ( << "Call Gate: retrieve invoke return value" ) );
  return reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theReturnValue;
}

API::set_error_t object_SetFn( void * aCommandManager, API::conf_object_t * /*ignored*/, API::attr_value_t * anObject, API::attr_value_t * /*ignored*/) {
  DBG_Assert( aCommandManager );
  DBG_Assert( anObject);
  DBG_Assert( anObject->kind == API::Sim_Val_Object);
  DBG_( Verb, ( << "Call Gate: object " << anObject->u.object->name ) );
  reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theObject = anObject->u.object;
  return API::Sim_Set_Ok;
}

API::set_error_t param1_SetFn( void * aCommandManager, API::conf_object_t * /*ignored*/, API::attr_value_t * aParameter, API::attr_value_t * /*ignored*/) {
  DBG_Assert( aCommandManager );
  DBG_Assert( aParameter );
  DBG_( Verb, ( << "Call Gate: Parameter 1" ) );
  reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter1 = *aParameter;

  if (aParameter->kind == API::Sim_Val_String) {
    DBG_(Verb, ( << "String in parameter 1 call gate") );
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter1.kind = API::Sim_Val_String;
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter1.u.string = strdup(aParameter->u.string);
  } else {
    DBG_(Verb, ( << "Parameter 1 " << aParameter->kind) );
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter1 = *aParameter;
  }

  return API::Sim_Set_Ok;
}

API::set_error_t param2_SetFn( void * aCommandManager, API::conf_object_t * /*ignored*/, API::attr_value_t * aParameter, API::attr_value_t * /*ignored*/) {
  DBG_Assert( aCommandManager );
  DBG_Assert( aParameter );
  DBG_( Verb, ( << "Call Gate: Parameter 2" ) );
  reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter2 = *aParameter;

  if (aParameter->kind == API::Sim_Val_String) {
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter2.kind = API::Sim_Val_String;
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter2.u.string = strdup(aParameter->u.string);
  } else {
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter2 = *aParameter;
  }

  return API::Sim_Set_Ok;
}

API::set_error_t param3_SetFn( void * aCommandManager, API::conf_object_t * /*ignored*/, API::attr_value_t * aParameter, API::attr_value_t * /*ignored*/) {
  DBG_Assert( aCommandManager );
  DBG_Assert( aParameter );
  DBG_( Verb, ( << "Call Gate: Parameter 3" ) );
  reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter3 = *aParameter;

  if (aParameter->kind == API::Sim_Val_String) {
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter3.kind = API::Sim_Val_String;
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter3.u.string = strdup(aParameter->u.string);
  } else {
    reinterpret_cast<SimicsCommandManager *>(aCommandManager)->theParameter3 = *aParameter;
  }

  return API::Sim_Set_Ok;
}

API::attr_value_t dummy_GetFn(void * /*ignored*/, API::conf_object_t * /*ignored*/, API::attr_value_t * /* ignored */) {
  API::attr_value_t ret_val;
  ret_val.kind = API::Sim_Val_Invalid;
  DBG_( Verb, ( << "Call Gate: dummy Get") );
  return ret_val;
}

API::conf_object_t * SimicsCommandManager_constructor(API::parse_object_t * aParseObject) {
  DBG_( Verb, ( << "SimicsCommandManager constructor.") );
  API::conf_object * object = new API::conf_object;
  APIFwd::SIM_object_constructor(object, aParseObject);
  return object;
}

int SimicsCommandManager_destructor(API::conf_object_t * anInstance) {
  delete anInstance;
  return 0;
}

SimicsCommandManager * SimicsCommandManager::get() {
  static SimicsCommandManager theManager;
  return &theManager;
}

SimicsCommandManager::SimicsCommandManager() {
  //Set up all the "closure" variables used for passing parameters through the gateway
  theObject = 0;
  theReturnValue.kind = API::Sim_Val_Invalid;
  theParameter1.kind = API::Sim_Val_Invalid;
  theParameter2.kind = API::Sim_Val_Invalid;
  theParameter3.kind = API::Sim_Val_Invalid;

  //Register the command gateway class
  API::class_data_t class_data;

  class_data.new_instance = &make_signature_from_free_fn<new_instance_t>::with<SimicsCommandManager_constructor>::trampoline;
  class_data.delete_instance = &make_signature_from_free_fn<delete_instance_t>::with<SimicsCommandManager_destructor>::trampoline;
  class_data.default_get_attr = 0;
  class_data.default_set_attr = 0;
  class_data.parent = 0;
  class_data.finalize_instance = 0;
#if SIM_VERSION < 1200
  //register_attr field was removed in Simics 1.2.x
  class_data.register_attr = 0;
#endif //SIM_VERSION < 1200
  class_data.parent = 0;
  class_data.description = "Simics command line interface gateway";
  class_data.kind = API::Sim_Class_Kind_Pseudo;

  theGatewayClass = RegisterClass_stub("FlexusCommandGateway", &class_data);

  //Register each call gate
  APIFwd::SIM_register_attribute(theGatewayClass, "invoke", invoke_GetFn, this, invoke_SetFn, this, API::Sim_Attr_Session, "Call gateway for commands");
  APIFwd::SIM_register_attribute(theGatewayClass, "object", dummy_GetFn, this, object_SetFn, this, API::Sim_Attr_Session, "object parameter for call gateway");
  APIFwd::SIM_register_attribute(theGatewayClass, "param1", dummy_GetFn, this, param1_SetFn, this, API::Sim_Attr_Session, "parameter 1 for call gateway");
  APIFwd::SIM_register_attribute(theGatewayClass, "param2", dummy_GetFn, this, param2_SetFn, this, API::Sim_Attr_Session, "parameter 2 for call gateway");
  APIFwd::SIM_register_attribute(theGatewayClass, "param3", dummy_GetFn, this, param3_SetFn, this, API::Sim_Attr_Session, "parameter 3 for call gateway");

  //Create the call gateway
  theGateway = NewObject_stub( theGatewayClass, "flexus_command_gateway", SimicsCommandManager_constructor );

  //Set up the variables in python so that command registration will work
  std::vector<std::string> commands;
  commands.push_back(std::string("@from cli import *\n"                                                   ));
  commands.push_back(std::string("@flexus_gw = SIM_get_object(\"flexus_command_gateway\")\n"));

  runPython(commands);
}

void SimicsCommandManager::invoke( int aCommandId ) {
  DBG_Assert(aCommandId >= 0);
  DBG_Assert(static_cast<unsigned>(aCommandId) < theCommands.size()); //static_cast to suppress warning
  DBG_( Verb, ( << "Invoke: " << aCommandId ) );

  theCommands[aCommandId] (theObject, theParameter1, theParameter2, theParameter3);
}

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker ) {
  DBG_( Verb, ( << "addCommand(): " << aNamespaceName << "." << aFrontendCommand) );

  //Add the command to the registry
  int command_id = theCommands.size();
  theCommands.push_back(anInvoker);

  //Assemble the python code which registers this command
  std::string command_id_str = boost::lexical_cast<std::string>(command_id);
  std::vector<std::string> commands;
  commands.push_back(std::string(
                       "@def flexus_command_" + command_id_str + "(obj):\n"
                       "\tSIM_set_attribute(flexus_gw, \"object\", obj)\n"
                       "\tSIM_set_attribute(flexus_gw, \"invoke\", " + command_id_str + ")\n"
                     ));
  commands.push_back(std::string("@new_command(\"" + aFrontendCommand + "\", flexus_command_" + command_id_str + ",[],type = \"Flexus Command\", namespace = \"" + aNamespaceName + "\", short = \"" + aCommandDescription + "\")\n"));

  runPython(commands);
}

std::string SimicsCommandManager::arg_class_to_string(int anArgClass) {
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

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int anArg1Class) {
  DBG_( Verb, ( << "addCommand(): " << aNamespaceName << "." << aFrontendCommand) );

  //Add the command to the registry
  int command_id = theCommands.size();
  theCommands.push_back( anInvoker );

  std::string arg1_class(arg_class_to_string(anArg1Class));

  //Assemble the python code which registers this command
  std::string command_id_str = boost::lexical_cast<std::string>(command_id);
  std::vector<std::string> commands;
  commands.push_back(std::string(
                       "@def flexus_command_" + command_id_str + "(obj, str):\n"
                       "\tSIM_set_attribute(flexus_gw, \"object\", obj)\n"
                       "\tSIM_set_attribute(flexus_gw, \"param1\", str)\n"
                       "\tSIM_set_attribute(flexus_gw, \"invoke\", " + command_id_str + ")\n"
                     ));
  commands.push_back(std::string("@new_command(\"" + aFrontendCommand + "\", flexus_command_" + command_id_str + ",[arg(" + arg1_class + ", \"" + anArg1Name + "\") ],type = \"Flexus Command\", namespace = \"" + aNamespaceName + "\", short = \"" + aCommandDescription + "\")\n"));

  runPython(commands);
}

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int anArg1Class, std::string const & anArg2Name,  int anArg2Class ) {
  DBG_( Verb, ( << "addCommand(): " << aNamespaceName << "." << aFrontendCommand) );

  //Add the command to the registry
  int command_id = theCommands.size();
  theCommands.push_back( anInvoker );

  std::string arg1_class(arg_class_to_string(anArg1Class));
  std::string arg2_class(arg_class_to_string(anArg2Class));

  //Assemble the python code which registers this command
  std::string command_id_str = boost::lexical_cast<std::string>(command_id);
  std::vector<std::string> commands;
  commands.push_back(std::string(
                       "@def flexus_command_" + command_id_str + "(obj, str1, str2):\n"
                       "\tSIM_set_attribute(flexus_gw, \"object\", obj)\n"
                       "\tSIM_set_attribute(flexus_gw, \"param1\", str1)\n"
                       "\tSIM_set_attribute(flexus_gw, \"param2\", str2)\n"
                       "\tSIM_set_attribute(flexus_gw, \"invoke\", " + command_id_str + ")\n"
                     ));
  commands.push_back(std::string("@new_command(\"" + aFrontendCommand + "\", flexus_command_" + command_id_str + ",[arg(" + arg1_class + ", \"" + anArg1Name + "\"), arg(" + arg2_class + ", \"" + anArg2Name + "\") ],type = \"Flexus Command\", namespace = \"" + aNamespaceName + "\", short = \"" + aCommandDescription + "\")\n"));

  runPython(commands);
}

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int anArg1Class, std::string const & anArg2Name,  int anArg2Class, std::string const & anArg3Name,  int anArg3Class  ) {
  DBG_( Verb, ( << "addCommand(): " << aNamespaceName << "." << aFrontendCommand) );

  //Add the command to the registry
  int command_id = theCommands.size();
  theCommands.push_back( anInvoker );

  std::string arg1_class(arg_class_to_string(anArg1Class));
  std::string arg2_class(arg_class_to_string(anArg2Class));
  std::string arg3_class(arg_class_to_string(anArg3Class));

  //Assemble the python code which registers this command
  std::string command_id_str = boost::lexical_cast<std::string>(command_id);
  std::vector<std::string> commands;
  commands.push_back(std::string(
                       "@def flexus_command_" + command_id_str + "(obj, str1, str2, str3):\n"
                       "\tSIM_set_attribute(flexus_gw, \"object\", obj)\n"
                       "\tSIM_set_attribute(flexus_gw, \"param1\", str1)\n"
                       "\tSIM_set_attribute(flexus_gw, \"param2\", str2)\n"
                       "\tSIM_set_attribute(flexus_gw, \"param3\", str3)\n"
                       "\tSIM_set_attribute(flexus_gw, \"invoke\", " + command_id_str + ")\n"
                     ));
  commands.push_back(std::string("@new_command(\"" + aFrontendCommand + "\", flexus_command_" + command_id_str + ",[arg(" + arg1_class + ", \"" + anArg1Name + "\"), arg(" + arg2_class + ", \"" + anArg2Name + "\"), arg(" + arg3_class + ", \"" + anArg3Name + "\") ],type = \"Flexus Command\", namespace = \"" + aNamespaceName + "\", short = \"" + aCommandDescription + "\")\n"));

  runPython(commands);
}

} //namespace aux_
} //namespace Simics
} //namespace Flexus
