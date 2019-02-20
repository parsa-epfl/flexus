#ifndef FLEXUS_CORE_DEBUG_DEBUG_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_DEBUG_HPP_INCLUDED

#include <sstream>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/less_equal.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <core/debug/debugger.hpp>

#include <core/debug/aux_/state.hpp>
#include <core/debug/aux_/operations.hpp>
#include <core/debug/aux_/process.hpp>

#define DBG_Control()     <core/debug/control.hpp>



#define DBG_(Sev, operations) \
  BOOST_PP_CAT(DBG__Undefined_Severity_Level__,Sev) ( BOOST_PP_CAT(DBG__internal_,Sev) ) ( DBG_internal_Sev_to_int(Sev) , operations)   /**/

#define DBG_Assert(condition, ... ) \
  DBG__internal_Assert( DBG_internal_Sev_to_int(Crit) , condition, __VA_ARGS__ )   /**/

#define DBG_AssertSev(Sev, condition, ... ) \
  BOOST_PP_CAT(DBG__Undefined_Severity_Level__,Sev) ( DBG__internal_Assert ) ( DBG_internal_Sev_to_int(Sev), condition,  __VA_ARGS__ )   /**/

#define DBG__Undefined_Severity_Level__None(Macro) Macro
#define DBG__Undefined_Severity_Level__Tmp(Macro) Macro
#define DBG__Undefined_Severity_Level__Crit(Macro) Macro
#define DBG__Undefined_Severity_Level__Dev(Macro) Macro
#define DBG__Undefined_Severity_Level__Trace(Macro) Macro
#define DBG__Undefined_Severity_Level__Iface(Macro) Macro
#define DBG__Undefined_Severity_Level__Verb(Macro) Macro
#define DBG__Undefined_Severity_Level__VVerb(Macro) Macro
#define DBG__Undefined_Severity_Level__Inv(Macro) Macro

#define DBG__Undefined_Severity_Level__none(Macro) Macro
#define DBG__Undefined_Severity_Level__tmp(Macro) Macro
#define DBG__Undefined_Severity_Level__crit(Macro) Macro
#define DBG__Undefined_Severity_Level__dev(Macro) Macro
#define DBG__Undefined_Severity_Level__trace(Macro) Macro
#define DBG__Undefined_Severity_Level__iface(Macro) Macro
#define DBG__Undefined_Severity_Level__verb(Macro) Macro
#define DBG__Undefined_Severity_Level__vverb(Macro) Macro
#define DBG__Undefined_Severity_Level__inv(Macro) Macro

#define DBG_internal_Sev_to_int(Sev)          \
    BOOST_PP_CAT(DBG_internal_Sev_to_int_,Sev)  /**/

//These defines must match what is set in the tSeverity enumeration in core/debug/severity.hpp
#define DBG_internal_Sev_to_int_None  8
#define DBG_internal_Sev_to_int_Tmp   7
#define DBG_internal_Sev_to_int_Crit  6
#define DBG_internal_Sev_to_int_Dev   5
#define DBG_internal_Sev_to_int_Trace 4
#define DBG_internal_Sev_to_int_Iface 3
#define DBG_internal_Sev_to_int_Verb  2
#define DBG_internal_Sev_to_int_VVerb 1
#define DBG_internal_Sev_to_int_Inv   0

#define DBG_internal_Sev_to_int_none  8
#define DBG_internal_Sev_to_int_tmp   7
#define DBG_internal_Sev_to_int_crit  6
#define DBG_internal_Sev_to_int_dev   5
#define DBG_internal_Sev_to_int_trace 4
#define DBG_internal_Sev_to_int_iface 3
#define DBG_internal_Sev_to_int_verb  2
#define DBG_internal_Sev_to_int_vverb 1
#define DBG_internal_Sev_to_int_inv   0

//No default ops
#define DBG__internal_DEFAULT_OPS

namespace DBG_Cats {
extern Flexus::Dbg::Category Core;
extern bool Core_debug_enabled;
extern Flexus::Dbg::Category Assert;
extern bool Assert_debug_enabled;
extern Flexus::Dbg::Category Stats;
extern bool Stats_debug_enabled;
}

#ifndef SELECTED_DEBUG
#warning "SELECTED_DEBUG not passed in on command line.  Defaulting to Iface"
#define DBG_SetCompileTimeMinSev Iface
#else
#define DBG_SetCompileTimeMinSev SELECTED_DEBUG
#endif
#define DBG_SetCoreDebugging true
#define DBG_SetAssertions true
#include DBG_Control()


#define RESET   "\e[0m"
#define BLACK   "\e[1;30m"
#define RED     "\e[1;31m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\e[1;33m"
#define BLUE    "\e[1;34m"
#define PURPLE  "\e[1;35m"
#define CYAN    "\e[1;36m"
#define WHITE   "\e[1;37m"

#define BG_BLACK   "\e[40m"
#define BG_RED     "\e[41m"
#define BG_GREEN   "\e[42m"
#define BG_YELLOW  "\e[43m"
#define BG_BLUE    "\e[44m"
#define BG_PURPLE  "\e[45m"
#define BG_CYAN    "\e[46m"
#define BG_WHITE   "\e[47m"


#define __TRACE_W(COLOR, WORD) \
    do { \
    DBG_(Dev,(<< COLOR <<__func__ << " " << WORD << RESET)); \
    } while(0)

#define __TRACE_W2(COLOR, WORD) \
    do { \
    DBG_(Dev,(<< COLOR <<__func__ << " " << WORD << RESET)); \
    } while(0)

#define __TRACE(COLOR) \
    do { \
    DBG_(Dev,(<< COLOR <<__func__ << RESET)); \
    } while(0)

#define __TRACE2(COLOR) \
    do { \
    DBG_(Dev,(<< COLOR <<__func__ << RESET)); \
    } while(0)

#define DECODER_TRACE \
    __TRACE2 (RED)
#define SEMANTICS_TRACE \
    __TRACE (CYAN)
#define DISPATCH_TRACE \
    __TRACE (YELLOW)
#define CORE_TRACE \
    __TRACE (GREEN)


#define SEMANTICS_DBG(WORD) \
    __TRACE_W (CYAN, WORD)
#define DECODER_DBG(WORD) \
    __TRACE_W2 (RED, WORD)
#define DISPATCH_DBG(WORD) \
    __TRACE_W (YELLOW, WORD)
#define CORE_DBG(WORD) \
    __TRACE_W (GREEN, WORD)
#define FETCH_DBG(WORD) \
    __TRACE_W (BLUE, WORD)
#define AGU_DBG(WORD) \
    __TRACE_W (PURPLE, WORD)
#define FLEXUS_DBG(WORD) \
    __TRACE_W (WHITE, WORD)


#endif //FLEXUS_CORE_DEBUG_DEBUG_HPP_INCLUDED

