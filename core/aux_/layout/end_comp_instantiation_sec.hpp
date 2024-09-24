#ifndef FLEXUS__LAYOUT_COMPONENT_INSTATIATION_SECTION
#error "End of component instantiation section without preceding begin."
#endif // FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED

#undef FLEXUS__LAYOUT_COMPONENT_INSTATIATION_SECTION
#undef FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED

#undef SCALE_WITH_SYSTEM_WIDTH
#undef FIXED
#undef MULTIPLY
#undef DIVIDE

} // End Wiring namespace
} // End Flexus namespace
