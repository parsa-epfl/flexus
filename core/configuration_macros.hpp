#ifndef FLEXUS_COMP_CFG_TEMPLATE_HPP_INCLUDED
#define FLEXUS_COMP_CFG_TEMPLATE_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#define FLEXUS_i_PARAMETER_tag 1
#define FLEXUS_i_POLICY_tag 2

#define FLEXUS_i_PT_LEN 8
#define FLEXUS_i_PT_Name 0
#define FLEXUS_i_PT_NameStr 1
#define FLEXUS_i_PT_Type 2
#define FLEXUS_i_PT_Descr 3
#define FLEXUS_i_PT_Switch 4
#define FLEXUS_i_PT_Default 5
#define FLEXUS_i_PT_DefaultStr 6
#define FLEXUS_i_PT_Dynamic 7

#define FLEXUS_PARAMETER(Name, Type, Descr, Switch, Default)                                       \
  ((Name, #Name, Type, Descr, Switch, Default, #Default, true)) /**/

#define FLEXUS_GENERATE_VAR(R, IGNORED, ParameterTuple)                                            \
  BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Type, ParameterTuple)                           \
  BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple);                          \
  struct BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple),      \
                      _param) {}; /**/

#define FLEXUS_GENERATE_SPECIALIZATION(R, TemplateName, ParameterTuple)                            \
  template <>                                                                                      \
  struct BOOST_PP_CAT(TemplateName, _struct)::get<                                                 \
      BOOST_PP_CAT(TemplateName, _struct)::BOOST_PP_CAT(                                           \
          BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple), _param)> {       \
    static std::string name() {                                                                    \
      return BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_NameStr, ParameterTuple);            \
    }                                                                                              \
    static std::string description() {                                                             \
      return BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Descr, ParameterTuple);              \
    }                                                                                              \
    static std::string cmd_line_switch() {                                                         \
      return BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Switch, ParameterTuple);             \
    }                                                                                              \
    typedef BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Type, ParameterTuple) type;           \
    static type defaultValue() {                                                                   \
      return BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Default, ParameterTuple);            \
    }                                                                                              \
    static type BOOST_PP_CAT(TemplateName, _struct)::*member() {                                   \
      return &BOOST_PP_CAT(TemplateName, _struct)::BOOST_PP_TUPLE_ELEM(                            \
          FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple);                                      \
    }                                                                                              \
  }; /**/

#define FLEXUS_GENERATE_INITIALIZER(R, IGNORED, ParameterTuple)                                    \
  , BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple)(theCfg_) /**/

#define FLEXUS_GENERATE_PARAMETER(R, TemplateName, ParameterTuple)                                 \
  Flexus::Core::aux_::DynamicParameter<                                                            \
      BOOST_PP_CAT(TemplateName, _struct),                                                         \
      BOOST_PP_CAT(TemplateName, _struct)::BOOST_PP_CAT(                                           \
          BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple), _param)>         \
      BOOST_PP_TUPLE_ELEM(FLEXUS_i_PT_LEN, FLEXUS_i_PT_Name, ParameterTuple); /**/

#define FLEXUS_DECLARE_COMPONENT_PARAMETERS(TemplateName, ParameterList)                           \
  struct BOOST_PP_CAT(TemplateName, Configuration_struct) {                                        \
    std::string theConfigName_;                                                                    \
    BOOST_PP_CAT(TemplateName, Configuration_struct)                                               \
    (std::string const &aConfigName) : theConfigName_(aConfigName) {                               \
    }                                                                                              \
    std::string const &name() const {                                                              \
      return theConfigName_;                                                                       \
    }                                                                                              \
    template <class Parameter> struct get;                                                         \
    BOOST_PP_SEQ_FOR_EACH(FLEXUS_GENERATE_VAR, x, ParameterList)                                   \
  };                                                                                               \
  BOOST_PP_SEQ_FOR_EACH(FLEXUS_GENERATE_SPECIALIZATION, BOOST_PP_CAT(TemplateName, Configuration), \
                        ParameterList)                                                             \
  struct BOOST_PP_CAT(TemplateName, Configuration) {                                               \
    typedef BOOST_PP_CAT(TemplateName, Configuration_struct) cfg_struct_;                          \
    cfg_struct_ theCfg_;                                                                           \
    std::string name() {                                                                           \
      return theCfg_.theConfigName_;                                                               \
    }                                                                                              \
    BOOST_PP_CAT(TemplateName, Configuration_struct) & cfg() {                                     \
      return theCfg_;                                                                              \
    }                                                                                              \
    BOOST_PP_CAT(TemplateName, Configuration)                                                      \
    (const std::string &aConfigName)                                                               \
        : theCfg_(aConfigName)                                                                     \
              BOOST_PP_SEQ_FOR_EACH(FLEXUS_GENERATE_INITIALIZER, x, ParameterList) {               \
    }                                                                                              \
    BOOST_PP_SEQ_FOR_EACH(FLEXUS_GENERATE_PARAMETER, BOOST_PP_CAT(TemplateName, Configuration),    \
                          ParameterList)                                                           \
  }; /**/

#define FLEXUS_DECLARE_COMPONENT_NO_PARAMETERS(TemplateName)                                       \
  struct BOOST_PP_CAT(TemplateName, Configuration_struct) {                                        \
    std::string theConfigName_;                                                                    \
    BOOST_PP_CAT(TemplateName, Configuration_struct)                                               \
    (std::string const &aConfigName) : theConfigName_(aConfigName) {                               \
    }                                                                                              \
    std::string const &name() const {                                                              \
      return theConfigName_;                                                                       \
    }                                                                                              \
  };                                                                                               \
  struct BOOST_PP_CAT(TemplateName, Configuration) {                                               \
    typedef BOOST_PP_CAT(TemplateName, Configuration_struct) cfg_struct_;                          \
    cfg_struct_ theCfg_;                                                                           \
    std::string name() {                                                                           \
      return theCfg_.theConfigName_;                                                               \
    }                                                                                              \
    BOOST_PP_CAT(TemplateName, Configuration)                                                      \
    (const std::string &aConfigName) : theCfg_(aConfigName) {                                      \
    }                                                                                              \
    cfg_struct_ &cfg() {                                                                           \
      return theCfg_;                                                                              \
    }                                                                                              \
  } /**/

#endif // FLEXUS_COMP_CFG_TEMPLATE_HPP_INCLUDED
