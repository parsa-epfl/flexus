#ifndef FLEXUS__CORE_TEST

#ifdef FLEXUS__LAYOUT_COMPONENTS_WIRED
#error "Wiring.cpp may contain only one component wiring section"
#endif // FLEXUS__LAYOUT_COMPONENTS_WIRED

#ifndef FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED
#error "The component wiring section of wiring.cpp must follow the component instantiation section"
#endif // FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif // FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_COMPONENT_WIRING_SECTION

#endif // FLEXUS__CORE_TEST

#ifdef FLEXUS__CORE_TEST
#define nFLEXUS FLEXUS__CORE_TEST
#else
#define nFLEXUS Flexus
#endif // FLEXUS__CORE_TEST

namespace nFLEXUS {
namespace Wiring {

#define WIRE(FromInstance, FromPort, ToInstance, ToPort)                                           \
  BOOST_PP_CAT(FromInstance, _instance).theJumpTable.BOOST_PP_CAT(wire_available_, FromPort) =     \
      &resolve_channel<ToInstance, ToInstance::iface::ToPort,                                      \
                       ToInstance::iface::ToPort::port_type,                                       \
                       ToInstance::iface::ToPort::is_array>::invoke_available;                     \
  BOOST_PP_CAT(FromInstance, _instance).theJumpTable.BOOST_PP_CAT(wire_manip_, FromPort) =         \
      &resolve_channel<ToInstance, ToInstance::iface::ToPort,                                      \
                       ToInstance::iface::ToPort::port_type,                                       \
                       ToInstance::iface::ToPort::is_array>::invoke_manip; /**/

void connectWiring() {
