#ifdef FLEXUS__LAYOUT_COMPONENTS_CONFIGURED
#error "Wiring.cpp may contain only one component configuration section"
#endif //FLEXUS__LAYOUT_COMPONENTS_CONFIGURED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif //FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_COMPONENT_CONFIGURATION_SECTION

#include <core/flexus.hpp>

#ifdef FLEXUS__CORE_TEST
namespace FLEXUS__CORE_TEST {
#else //FLEXUS__CORE_TEST
namespace Flexus {
#endif //FLEXUS__CORE_TEST
namespace Wiring {

//Bring in the tools we need from the Configuration component
using namespace Flexus::Core;

#define CREATE_CONFIGURATION( Component, String, Name )  \
    BOOST_PP_CAT(Component,Configuration) Name(String);

Flexus::Core::index_t getSystemWidth() {
  return ComponentManager::getComponentManager().systemWidth();
}

using Flexus::Core::theFlexus;
