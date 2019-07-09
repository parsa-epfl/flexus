#include <core/debug/debug.hpp>

#if defined(DBG_NewCategories)
#error "DBG_NewCategories has been replaced with DBG_DefineCategories"
#endif

#if defined(DBG_SetCompileTimeMinSev)

#undef DBG__internal_tmp_MinimumSeverity
#define DBG__internal_tmp_MinimumSeverity                                                          \
  BOOST_PP_CAT(DBG__Undefined_Severity_Level__, DBG_SetCompileTimeMinSev)                          \
  (DBG_internal_Sev_to_int)(DBG_SetCompileTimeMinSev) /**/

#include <core/debug/aux_/clear_macros.hpp>

#include <core/debug/aux_/crit_macros.hpp>
#include <core/debug/aux_/dev_macros.hpp>
#include <core/debug/aux_/iface_macros.hpp>
#include <core/debug/aux_/inv_macros.hpp>
#include <core/debug/aux_/tmp_macros.hpp>
#include <core/debug/aux_/trace_macros.hpp>
#include <core/debug/aux_/verb_macros.hpp>
#include <core/debug/aux_/vverb_macros.hpp>

#undef DBG_SetCompileTimeMinSev

#endif // DBG_SetCompileTimeMinSev

#if defined(DBG_SetInitialRuntimeMinSev)
#ifdef DBG__internal_GlobalRunTimeMinSev_SET
#error "Runtime global minimum severity may only be set once"
#else
#define DBG__internal_GlobalRunTimeMinSev_SET
#endif

#include <core/debug/aux_/runtime_sev.hpp>

#undef DBG_SetInitialRuntimeMinSev

#endif // DBG_SetGlobalRunTimeMinSev

#if defined(DBG_DefineCategories)

#include <core/debug/aux_/new_categories.hpp>
#undef DBG_DefineCategories

#endif // DBG_NewCategories

#if defined(DBG_DeclareCategories)

#include <core/debug/aux_/declare_new_categories.hpp>
#undef DBG_DeclareCategories

#endif // DBG_DeclareCategories

#if defined(DBG_SetAssertions)

#include <core/debug/aux_/assert_macros.hpp>
#undef DBG_SetAssertions

#endif // DBG_SetMinimumSeverity

#if defined(DBG_SetDefaultOps)

#include <core/debug/aux_/default_ops.hpp>

#endif // DBG_SetDefaultOps

#if defined(DBG_Reset)

#undef DBG_Reset
#include <core/debug/aux_/reset.hpp>

#endif // DBG_SetDefaultOps
