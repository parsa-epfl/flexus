#ifndef FLEXUS_CORE_DEBUG_AUX__DECLARE_CATEGORIES_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_AUX__DECLARE_CATEGORIES_HPP_INCLUDED
// These macros are only defined the first time this file is included

#define DBG__internal_PROCESS_DECLARE_CATEGORIES(...)                                                                  \
    BOOST_PP_VA_FOR_EACH(DBG__internal_DECLARE_CATEGORY, __VA_ARGS__) /**/

#define DBG__internal_DECLARE_CATEGORY(cat)                                                                            \
    namespace DBG_Cats {                                                                                               \
    extern bool BOOST_PP_CAT(cat, _debug_enabled);                                                                     \
    extern Flexus::Dbg::Category cat;                                                                                  \
    } /**/

#endif // FLEXUS_CORE_DEBUG_AUX__DECLARE_CATEGORIES_HPP_INCLUDED

DBG__internal_PROCESS_DECLARE_CATEGORIES(DBG_DeclareCategories)
