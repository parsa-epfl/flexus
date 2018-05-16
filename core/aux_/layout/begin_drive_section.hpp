#ifndef FLEXUS__CORE_TEST

#ifdef FLEXUS__LAYOUT_DRIVES_ORDERED
#error "Wiring.cpp may contain only one drive order section"
#endif //FLEXUS__LAYOUT_DRIVES_ORDERED

#ifndef FLEXUS__LAYOUT_COMPONENTS_WIRED
#error "The drive order section of wiring.cpp must follow the component wiring section"
#endif //FLEXUS__LAYOUT_COMPONENTS_WIRED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif //FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_DRIVE_SECTION

#endif //FLEXUS__CORE_TEST

#include <core/drive.hpp>

#ifdef FLEXUS__CORE_TEST
namespace FLEXUS__CORE_TEST {
#else //FLEXUS__CORE_TEST
namespace Flexus {
#endif //FLEXUS__CORE_TEST
namespace Wiring {

#define DRIVE( Handle, Drive )  \
  Flexus::Core::DriveHandle< Handle, Handle::iface::Drive >

typedef mpl::vector <
