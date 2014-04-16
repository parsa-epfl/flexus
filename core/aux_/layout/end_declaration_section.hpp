#ifndef FLEXUS__LAYOUT_DECLARATION_SECTION
#error "End of declaration section without preceding begin."
#endif //FLEXUS_LAYOUT_COMPONENT_DECLARATION_SECTION

#ifndef FLEXUS__COMPONENT_CHAIN_PREVIOUS
#error "Last component forget to set the FLEXUS_COMPONENT_CHAIN_PREVIOUS macro"
#endif //FLEXUS__COMPONENT_CHAIN_PREVIOUS

#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#undef FLEXUS__LAYOUT_DECLARATION_SECTION
#undef FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_COMPONENTS_DECLARED

#undef FLEXUS__PREVIOUS_COMP_DECL
#undef FLEXUS__COMP_DECL

#undef FLEXUS_END_COMPONENT

