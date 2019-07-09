#ifndef FLEXUS_TARGET
#error "Wiring.cpp must contain a flexus target section before the declaration section"
#endif

#ifdef FLEXUS__LAYOUT_COMPONENTS_DECLARED
#error "Wiring.cpp may contain only one declaration section"
#endif // FLEXUS__LAYOUT_COMPONENTS_DECLARED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif // FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_DECLARATION_SECTION

#define FLEXUS_END_COMPONENT NIL

#define FLEXUS__COMPONENT_CHAIN_CURRENT FLEXUS_BEGIN_COMPONENT
#define FLEXUS__COMPONENT_CHAIN_PREVIOUS FLEXUS_END_COMPONENT

#define FLEXUS__PREVIOUS_COMP_DECL()                                                               \
  BOOST_PP_CAT(FLEXUS__COMP_DECL_, FLEXUS__COMPONENT_CHAIN_PREVIOUS)
#define FLEXUS__COMP_DECL() BOOST_PP_CAT(FLEXUS__COMP_DECL_, FLEXUS__COMPONENT_CHAIN_CURRENT)

#define FLEXUS__PREVIOUS_IMPLEMENTATION_LIST()                                                     \
  BOOST_PP_CAT(FLEXUS__IMPLEMENTATION_LIST_, FLEXUS__COMPONENT_CHAIN_PREVIOUS)
#define FLEXUS__IMPLEMENTATION_LIST()                                                              \
  BOOST_PP_CAT(FLEXUS__IMPLEMENTATION_LIST_, FLEXUS__COMPONENT_CHAIN_CURRENT)
