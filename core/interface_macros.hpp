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
#ifndef FLEXUS_CORE__INTERFACE_MACROS_HPP_INCLUDED
#define FLEXUS_CORE__INTERFACE_MACROS_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#define FLEXUS_i_IT_LEN 5
#define FLEXUS_i_IT_Type 0
#define FLEXUS_i_IT_Name 1
#define FLEXUS_i_IT_NameStr 2
#define FLEXUS_i_IT_Payload 3
#define FLEXUS_i_IT_Width 4

#define FLEXUS_IFACE_PORT(Type, Payload, Name) ((Type, Name, #Name, Payload, 0)) /**/

#define FLEXUS_IFACE_PORT_ARRAY(Type, Payload, Name, Width)                                        \
  ((BOOST_PP_CAT(Type, Array), Name, #Name, Payload, Width)) /**/

#define FLEXUS_IFACE_DYNAMIC_PORT_ARRAY(Type, Payload, Name)                                       \
  ((BOOST_PP_CAT(Type, DynArray), Name, #Name, Payload, 0)) /**/

#define FLEXUS_IFACE_DRIVE(Name) ((Drive, Name, #Name, xx, 0)) /**/

#define FLEXUS_IFACE_PORT_SPEC(Name, Type, Payload, Array)                                         \
  struct Name {                                                                                    \
    typedef Payload payload;                                                                       \
    typedef Flexus::Core::aux_::Type port_type;                                                    \
    static const bool is_array = Array;                                                            \
  }; /**/

#define FLEXUS_IFACE_WIDTH(Name, Width)                                                            \
  static Flexus::Core::index_t width(configuration &cfg, Name const &) {                           \
    return Width;                                                                                  \
  } /**/

#define FLEXUS_IFACE_DYNWIDTH(Name)                                                                \
  static Flexus::Core::index_t width(configuration &cfg, Name const &); /**/

#define FLEXUS_IFACE_PUSH_IN_DECL(Name)                                                            \
  virtual void push(Name const &, Name::payload &) = 0;                                            \
  virtual bool available(Name const &) = 0; /**/

#define FLEXUS_IFACE_PUSH_IN_ARRAY_DECL(Name)                                                      \
  virtual void push(Name const &, Flexus::Core::index_t, Name::payload &) = 0;                     \
  virtual bool available(Name const &, Flexus::Core::index_t) = 0; /**/

#define FLEXUS_IFACE_PULL_OUT_DECL(Name)                                                           \
  virtual Name::payload pull(Name const &) = 0;                                                    \
  virtual bool available(Name const &) = 0; /**/

#define FLEXUS_IFACE_PULL_OUT_ARRAY_DECL(Name)                                                     \
  virtual Name::payload pull(Name const &, Flexus::Core::index_t) = 0;                             \
  virtual bool available(Name const &, Flexus::Core::index_t) = 0; /**/

#define FLEXUS_IFACE_DRIVE_SPEC(IfaceStr, Name, NameStr)                                           \
  struct Name {                                                                                    \
    static std::string name() {                                                                    \
      return IfaceStr "::" NameStr;                                                                \
    }                                                                                              \
  };                                                                                               \
  virtual void drive(Name const &) = 0; /**/

#define FLEXUS_IFACE_GENERATE_PushOutput(Iface, IfaceStr, Name, NameStr, Payload, Width)           \
  FLEXUS_IFACE_PORT_SPEC(Name, push, Payload, false) /**/

#define FLEXUS_IFACE_GENERATE_PushOutputArray(Iface, IfaceStr, Name, NameStr, Payload, Width)      \
  FLEXUS_IFACE_PORT_SPEC(Name, push, Payload, true)                                                \
  FLEXUS_IFACE_WIDTH(Name, Width) /**/

#define FLEXUS_IFACE_GENERATE_PushOutputDynArray(Iface, IfaceStr, Name, NameStr, Payload, Width)   \
  FLEXUS_IFACE_PORT_SPEC(Name, push, Payload, true)                                                \
  FLEXUS_IFACE_DYNWIDTH(Name) /**/

#define FLEXUS_IFACE_GENERATE_PushInput(Iface, IfaceStr, Name, NameStr, Payload, Width)            \
  FLEXUS_IFACE_PORT_SPEC(Name, push, Payload, false)                                               \
  FLEXUS_IFACE_PUSH_IN_DECL(Name) /**/

#define FLEXUS_IFACE_GENERATE_PushInputArray(Iface, IfaceStr, Name, NameStr, Payload, Width)       \
  FLEXUS_IFACE_PORT_SPEC(Name, push, Payload, true)                                                \
  FLEXUS_IFACE_PUSH_IN_ARRAY_DECL(Name, Width)                                                     \
  FLEXUS_IFACE_WIDTH(Name, Width) /**/

#define FLEXUS_IFACE_GENERATE_PushInputDynArray(Iface, IfaceStr, Name, NameStr, Payload, Width)    \
  FLEXUS_IFACE_PORT_SPEC(Name, push, Payload, true)                                                \
  FLEXUS_IFACE_PUSH_IN_ARRAY_DECL(Name)                                                            \
  FLEXUS_IFACE_DYNWIDTH(Name) /**/

#define FLEXUS_IFACE_GENERATE_PullOutput(Iface, IfaceStr, Name, NameStr, Payload, Width)           \
  FLEXUS_IFACE_PORT_SPEC(Name, pull, Payload, false)                                               \
  FLEXUS_IFACE_PULL_OUT_DECL(Name) /**/

#define FLEXUS_IFACE_GENERATE_PullOutputArray(Iface, IfaceStr, Name, NameStr, Payload, Width)      \
  FLEXUS_IFACE_PORT_SPEC(Name, pull, Payload, true)                                                \
  FLEXUS_IFACE_PULL_OUT_ARRAY_DECL(Name, Width)                                                    \
  FLEXUS_IFACE_WIDTH(Name, Width) /**/

#define FLEXUS_IFACE_GENERATE_PullOutputDynArray(Iface, IfaceStr, Name, NameStr, Payload, Width)   \
  FLEXUS_IFACE_PORT_SPEC(Name, pull, Payload, true)                                                \
  FLEXUS_IFACE_PULL_OUT_ARRAY_DECL(Name)                                                           \
  FLEXUS_IFACE_DYNWIDTH(Name) /**/

#define FLEXUS_IFACE_GENERATE_PullInput(Iface, IfaceStr, Name, NameStr, Payload, Width)            \
  FLEXUS_IFACE_PORT_SPEC(Name, pull, Payload, false) /**/

#define FLEXUS_IFACE_GENERATE_PullInputArray(Iface, IfaceStr, Name, NameStr, Payload, Width)       \
  FLEXUS_IFACE_PORT_SPEC(Name, pull, Payload, true)                                                \
  FLEXUS_IFACE_WIDTH(Name, Width) /**/

#define FLEXUS_IFACE_GENERATE_PullInputDynArray(Iface, IfaceStr, Name, NameStr, Payload, Width)    \
  FLEXUS_IFACE_PORT_SPEC(Name, pull, Payload, true)                                                \
  FLEXUS_IFACE_DYNWIDTH(Name) /**/

#define FLEXUS_IFACE_GENERATE_Drive(Iface, IfaceStr, Name, NameStr, Payload, Width)                \
  FLEXUS_IFACE_DRIVE_SPEC(IfaceStr, Name, NameStr) /**/

#define FLEXUS_IFACE_GENERATE(R, InterfaceName, ParameterTuple)                                    \
  BOOST_PP_CAT(FLEXUS_IFACE_GENERATE_,                                                             \
               BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Type, ParameterTuple))             \
  (InterfaceName, BOOST_PP_STRINGIZE(InterfaceName),                                               \
   BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Name, ParameterTuple),                         \
   BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_NameStr, ParameterTuple),                      \
   BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Payload, ParameterTuple),                      \
   BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Width, ParameterTuple)) /**/

#define FLEXUS_IFACE_JUMPTABLE_Impl(Name)                                                          \
  bool (*BOOST_PP_CAT(wire_available_, Name))(Flexus::Core::index_t anIndex);                      \
  void (*BOOST_PP_CAT(wire_manip_, Name))(Flexus::Core::index_t anIndex,                           \
                                          iface::Name::payload & aPayload); /**/

#define FLEXUS_IFACE_JUMPTABLE_PullInput(Name) FLEXUS_IFACE_JUMPTABLE_Impl(Name)          /**/
#define FLEXUS_IFACE_JUMPTABLE_PullInputArray(Name) FLEXUS_IFACE_JUMPTABLE_Impl(Name)     /**/
#define FLEXUS_IFACE_JUMPTABLE_PullInputDynArray(Name) FLEXUS_IFACE_JUMPTABLE_Impl(Name)  /**/
#define FLEXUS_IFACE_JUMPTABLE_PushOutput(Name) FLEXUS_IFACE_JUMPTABLE_Impl(Name)         /**/
#define FLEXUS_IFACE_JUMPTABLE_PushOutputArray(Name) FLEXUS_IFACE_JUMPTABLE_Impl(Name)    /**/
#define FLEXUS_IFACE_JUMPTABLE_PushOutputDynArray(Name) FLEXUS_IFACE_JUMPTABLE_Impl(Name) /**/

#define FLEXUS_IFACE_JUMPTABLE_PushInput(x)          /**/
#define FLEXUS_IFACE_JUMPTABLE_PushInputArray(x)     /**/
#define FLEXUS_IFACE_JUMPTABLE_PushInputDynArray(x)  /**/
#define FLEXUS_IFACE_JUMPTABLE_PullOutput(x)         /**/
#define FLEXUS_IFACE_JUMPTABLE_PullOutputArray(x)    /**/
#define FLEXUS_IFACE_JUMPTABLE_PullOutputDynArray(x) /**/
#define FLEXUS_IFACE_JUMPTABLE_Drive(x)              /**/

#define FLEXUS_IFACE_JUMPTABLE(R, x, ParameterTuple)                                               \
  BOOST_PP_CAT(FLEXUS_IFACE_JUMPTABLE_,                                                            \
               BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Type, ParameterTuple))             \
  (BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Name, ParameterTuple)) /**/

#define FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name) BOOST_PP_CAT(wire_available_, Name) = 0; /**/

#define FLEXUS_IFACE_JUMPTABLE_INIT_PullInput(Name) FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name)      /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PullInputArray(Name) FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name) /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PullInputDynArray(Name)                                        \
  FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name)                                                    /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PushOutput(Name) FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name) /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PushOutputArray(Name)                                          \
  FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name) /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PushOutputDynArray(Name)                                       \
  FLEXUS_IFACE_JUMPTABLE_INIT_Impl(Name) /**/

#define FLEXUS_IFACE_JUMPTABLE_INIT_PushInput(x)          /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PushInputArray(x)     /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PushInputDynArray(x)  /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PullOutput(x)         /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PullOutputArray(x)    /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_PullOutputDynArray(x) /**/
#define FLEXUS_IFACE_JUMPTABLE_INIT_Drive(x)              /**/

#define FLEXUS_IFACE_JUMPTABLE_INIT(R, x, ParameterTuple)                                          \
  BOOST_PP_CAT(FLEXUS_IFACE_JUMPTABLE_INIT_,                                                       \
               BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Type, ParameterTuple))             \
  (BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Name, ParameterTuple)) /**/

#define FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr)                                           \
  if (BOOST_PP_CAT(wire_available_, Name) == 0) {                                                  \
    DBG_(Crit, (<< anInstance << " port " NameStr " is not wired"));                               \
  } /**/

#define FLEXUS_IFACE_JUMPTABLE_CHECK_PullInput(Name, NameStr)                                      \
  FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr) /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PullInputArray(Name, NameStr)                                 \
  FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr) /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PullInputDynArray(Name, NameStr)                              \
  FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr) /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PushOutput(Name, NameStr)                                     \
  FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr) /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PushOutputArray(Name, NameStr)                                \
  FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr) /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PushOutputDynArray(Name, NameStr)                             \
  FLEXUS_IFACE_JUMPTABLE_CHECK_Impl(Name, NameStr) /**/

#define FLEXUS_IFACE_JUMPTABLE_CHECK_PushInput(x, y)          /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PushInputArray(x, y)     /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PushInputDynArray(x, y)  /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PullOutput(x, y)         /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PullOutputArray(x, y)    /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_PullOutputDynArray(x, y) /**/
#define FLEXUS_IFACE_JUMPTABLE_CHECK_Drive(x, y)              /**/

#define FLEXUS_IFACE_JUMPTABLE_CHECK(R, x, ParameterTuple)                                         \
  BOOST_PP_CAT(FLEXUS_IFACE_JUMPTABLE_CHECK_,                                                      \
               BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Type, ParameterTuple))             \
  (BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Name, ParameterTuple),                         \
   BOOST_PP_STRINGIZE(                                                                             \
       BOOST_PP_TUPLE_ELEM(FLEXUS_i_IT_LEN, FLEXUS_i_IT_Name, ParameterTuple))) /**/

#define FLEXUS_COMPONENT_INTERFACE(ComponentName, ParameterList)                                   \
  struct BOOST_PP_CAT(ComponentName, JumpTable);                                                   \
  struct BOOST_PP_CAT(ComponentName, Interface) : public Flexus::Core::ComponentInterface {        \
    typedef BOOST_PP_CAT(ComponentName, Configuration_struct) configuration;                       \
    typedef BOOST_PP_CAT(ComponentName, JumpTable) jump_table;                                     \
    static BOOST_PP_CAT(ComponentName, Interface) *                                                \
        instantiate(configuration &, jump_table &, Flexus::Core::index_t, Flexus::Core::index_t);  \
    BOOST_PP_SEQ_FOR_EACH(FLEXUS_IFACE_GENERATE, BOOST_PP_CAT(ComponentName, Interface),           \
                          ParameterList)                                                           \
  };                                                                                               \
  struct BOOST_PP_CAT(ComponentName, JumpTable) {                                                  \
    typedef BOOST_PP_CAT(ComponentName, Interface) iface;                                          \
    BOOST_PP_SEQ_FOR_EACH(FLEXUS_IFACE_JUMPTABLE, x, ParameterList)                                \
    BOOST_PP_CAT(ComponentName, JumpTable)() {                                                     \
      BOOST_PP_SEQ_FOR_EACH(FLEXUS_IFACE_JUMPTABLE_INIT, x, ParameterList)                         \
    }                                                                                              \
    void check(std::string const &anInstance) {                                                    \
      BOOST_PP_SEQ_FOR_EACH(FLEXUS_IFACE_JUMPTABLE_CHECK, x, ParameterList)                        \
    }                                                                                              \
  } /**/

#define FLEXUS_COMPONENT_EMPTY_INTERFACE(ComponentName)                                            \
  struct BOOST_PP_CAT(ComponentName, JumpTable) {                                                  \
    void check(std::string const &anInstance) {                                                    \
    }                                                                                              \
  };                                                                                               \
  struct BOOST_PP_CAT(ComponentName, Interface) : public Flexus::Core::ComponentInterface {        \
    typedef BOOST_PP_CAT(ComponentName, Configuration_struct) configuration;                       \
    typedef BOOST_PP_CAT(ComponentName, JumpTable) jump_table;                                     \
    static BOOST_PP_CAT(ComponentName, Interface) *                                                \
        instantiate(configuration &, jump_table &, Flexus::Core::index_t, Flexus::Core::index_t);  \
  } /**/

/*

Example expansion:

COMPONENT_INTERFACE(
  PORT(  PushOutput, DirectoryTransport, DirReplyOut )
  PORT(  PushInput,  DirectoryTransport, DirRequestIn )
  DRIVE( DirectoryDrive )
);

struct DirectoryJumpTable;
struct DirectoryInterface : public Flexus::Core::ComponentInterface {
  struct DirReplyOut {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };

  struct DirRequestIn {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };
  virtual void push(DirRequestIn const &, DirRequestIn::payload &) = 0;
  virtual bool available(DirRequestIn const &) = 0;

  struct DirectoryDrive { static std::string name() { return
"DirectoryComponentInterface::DirectoryDrive"; } }; virtual void
drive(DirectoryDrive const & ) = 0;

  static DirectoryInterface * instantiate(DirectoryConfiguration::cfg_struct_ &
aCfg, DirectoryJumpTable & aJumpTable, Flexus::Core::index_t anIndex,
Flexus::Core::index_t aWidth); typedef DirectoryConfiguration_struct
configuration; typedef DirectoryJumpTable jump_table;
};

struct DirectoryJumpTable {
  typedef DirectoryInterface iface;
  bool (*wire_available_DirReplyOut)(Flexus::Core::index_t anIndex);
  void (*wire_manip_DirReplyOut)(Flexus::Core::index_t anIndex,
iface::DirReplyOut::payload & aPayload); DirectoryJumpTable() {
    wire_available_DirReplyOut = 0;
    wire_manip_DirReplyOut = 0;
  }
  void check(std::string const & anInstance) {
    if ( wire_available_DirReplyOut == 0 ) { DBG_(Crit, (<< anInstance << " port
" "DirReplyOut" " is not wired" ) ); }
  }
};

*/

#endif // FLEXUS_CORE__INTERFACE_MACROS_HPP_INCLUDED
