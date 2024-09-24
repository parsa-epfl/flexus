#ifndef FLEXUS_CORE_DEBUG_AUX__NEW_CATEGORIES_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_AUX__NEW_CATEGORIES_HPP_INCLUDED
// These macros are only defined the first time this file is included

#define DBG__internal_PROCESS_NEW_CATEGORIES(...) BOOST_PP_VA_FOR_EACH(DBG__internal_NEW_CATEGORY, __VA_ARGS__) /**/

#define DBG__internal_NEW_CATEGORY(cat)                                                                                \
    namespace DBG_Cats {                                                                                               \
    bool BOOST_PP_CAT(cat, _debug_enabled) = true;                                                                     \
    Flexus::Dbg::Category cat(#cat, &BOOST_PP_CAT(cat, _debug_enabled));                                               \
    } /**/

#endif // FLEXUS_CORE_DEBUG_AUX__NEW_CATEGORIES_HPP_INCLUDED

DBG__internal_PROCESS_NEW_CATEGORIES(DBG_DefineCategories)
