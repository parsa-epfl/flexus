#ifndef FLEXUS_CORE_DEBUG_AUX__STATE_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_AUX__STATE_HPP_INCLUDED

#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/replace.hpp>

#define DBG__internal_State_Severity           0
#define DBG__internal_State_HasCondition       1
#define DBG__internal_State_Condition          2
#define DBG__internal_State_NoMessage          3
#define DBG__internal_State_HasCategories      4
#define DBG__internal_State_Categories         5
#define DBG__internal_State_CategoryCondition  6
#define DBG__internal_State_Assert             7
#define DBG__internal_State_SuppressDefaultOps 8
#define DBG__internal_State_HasComponent       9
#define DBG__internal_State_Component          10

#define DBG__internal_OPERATION_INITIAL_STATE(Sev)                                                                     \
    (/*Severity*/ Sev)(/* HasCondition */ 0)(/*Condition*/ true)(/*NoMessage*/ 1)(/*HasCategories*/ 0)(                \
      /*Categories*/)(/*CategoryCondition*/ true)(/*Assert*/ 0)(/*SuppressDefaultOps*/ 0)(/*HasComponent*/ 0)(         \
      /* Component */)

#define DBG__internal_State_GetSev(state)      BOOST_PP_SEQ_ELEM(DBG__internal_State_Severity, state)
#define DBG__internal_State_SetSev(state, sev) BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Severity, sev)

#define DBG__internal_State_GetHasCondition(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_HasCondition, state)
#define DBG__internal_State_SetHasCondition(state, has_cond)                                                           \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_HasCondition, has_cond)

#define DBG__internal_State_GetCondition(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_Condition, state)
#define DBG__internal_State_SetCondition(state, condition)                                                             \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Condition, condition)
#define DBG__internal_State_AddCondition(state, condition)                                                             \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Condition, DBG__internal_State_GetCondition(state) && condition)

#define DBG__internal_State_GetNoMessage(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_NoMessage, state)
#define DBG__internal_State_SetNoMessage(state, no_message)                                                            \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_NoMessage, no_message)

#define DBG__internal_State_GetHasCategories(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_HasCategories, state)
#define DBG__internal_State_SetHasCategories(state, has_cats)                                                          \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_HasCategories, has_cats)

#define DBG__internal_State_GetCategories(state)       BOOST_PP_SEQ_ELEM(DBG__internal_State_Categories, state)
#define DBG__internal_State_SetCategories(state, cats) BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Categories, cats)
#define DBG__internal_State_AddCategories(state, cats)                                                                 \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Categories, DBG__internal_State_GetCategories(state) | cats)

#define DBG__internal_State_GetCategoryCondition(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_CategoryCondition, state)
#define DBG__internal_State_SetCategoryCondition(state, cats)                                                          \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_CategoryCondition, cats)
#define DBG__internal_State_AddCategoryCondition(state, cats)                                                          \
    BOOST_PP_SEQ_REPLACE(state,                                                                                        \
                         DBG__internal_State_CategoryCondition,                                                        \
                         DBG__internal_State_GetCategoryCondition(state) || cats)

#define DBG__internal_State_GetAssert(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_Assert, state)
#define DBG__internal_State_SetAssert(state, is_assert)                                                                \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Assert, is_assert)

#define DBG__internal_State_GetSuppressDefaultOps(state)                                                               \
    BOOST_PP_SEQ_ELEM(DBG__internal_State_SuppressDefaultOps, state)
#define DBG__internal_State_SetSuppressDefaultOps(state, is_default)                                                   \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_SuppressDefaultOps, is_default)

#define DBG__internal_State_GetHasComponent(state) BOOST_PP_SEQ_ELEM(DBG__internal_State_HasComponent, state)
#define DBG__internal_State_SetHasComponent(state, has_comp)                                                           \
    BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_HasComponent, has_comp)

#define DBG__internal_State_GetComponent(state)       BOOST_PP_SEQ_ELEM(DBG__internal_State_Component, state)
#define DBG__internal_State_SetComponent(state, comp) BOOST_PP_SEQ_REPLACE(state, DBG__internal_State_Component, comp)

#endif // FLEXUS_CORE_DEBUG_AUX__STATE_HPP_INCLUDED
