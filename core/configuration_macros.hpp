//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
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
