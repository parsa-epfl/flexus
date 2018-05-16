#ifdef FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED
#error "Wiring.cpp may contain only one component instantiation section"
#endif //FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif //FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_COMPONENT_INSTATIATION_SECTION

#ifdef FLEXUS__CORE_TEST
namespace FLEXUS__CORE_TEST {
#else //FLEXUS__CORE_TEST
namespace Flexus {
#endif //FLEXUS__CORE_TEST
namespace Wiring {

using Flexus::Core::ComponentInstance;
using Flexus::Core::ComponentHandle;

#define SCALE_WITH_SYSTEM_WIDTH true
#define FIXED false
#define MULTIPLY true
#define DIVIDE   false

