#ifndef FLEXUS__LAYOUT_COMPONENT_CONFIGURATION_SECTION
#error "End of component configuration section without preceding begin."
#endif // FLEXUS__LAYOUT_COMPONENT_CONFIGURATION_SECTION

#undef FLEXUS__LAYOUT_COMPONENT_CONFIGURATION_SECTION
#undef FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_COMPONENTS_CONFIGURED

#undef CREATE_CONFIGURATION

} // End namespace Wiring
} // End namespace Flexus
