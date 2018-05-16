#ifndef FLEXUS__CORE_TEST

#ifndef FLEXUS__LAYOUT_COMPONENT_WIRING_SECTION
#error "End of component wiring section without preceding begin."
#endif //FLEXUS__LAYOUT_COMPONENT_WIRING_SECTION

#undef FLEXUS__LAYOUT_COMPONENT_WIRING_SECTION
#undef FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_COMPONENTS_WIRED

#endif //FLEXUS__CORE_TEST

} //end connectWiring

#undef WIRE

} //End Wiring namespace
} //End nFlexus namespace

#undef nFLEXUS

