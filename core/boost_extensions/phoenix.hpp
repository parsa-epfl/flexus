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
#ifndef FLEXUS_CORE_BOOST_EXTENSION_PHOENIX_HPP_INCLUDED
#define FLEXUS_CORE_BOOST_EXTENSION_PHOENIX_HPP_INCLUDED

#define PHOENIX_LIMIT 6

#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/spirit/include/phoenix1_casts.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
// #include <boost/spirit/phoenix/functions.hpp>
// #include <boost/spirit/phoenix/casts.hpp>
// #include <boost/spirit/phoenix/binders.hpp>
#include <boost/type_traits/remove_pointer.hpp>

namespace phoenix {
template <> struct rank<char const *> { static int32_t const value = 160; };

template <> struct rank<char const *const> { static int32_t const value = 160; };

struct new_0_impl {
  template <class Object> struct result { typedef Object type; };

  template <class Object> Object operator()(Object const ignored) const {
    return new typename boost::remove_pointer<Object>::type;
  }
};

struct new_1_impl {
  template <class Object, class Arg1> struct result { typedef Object type; };

  template <class Object, class Arg1> Object operator()(Object const ignored, Arg1 &arg1) const {
    return new typename boost::remove_pointer<Object>::type(arg1);
  }
};

struct new_2_impl {
  template <class Object, class Arg1, class Arg2> struct result { typedef Object type; };

  template <class Object, class Arg1, class Arg2>
  Object operator()(Object const ignored, Arg1 &arg1, Arg2 &arg2) const {
    return new typename boost::remove_pointer<Object>::type(arg1, arg2);
  }
};

struct new_3_impl {
  template <class Object, class Arg1, class Arg2, class Arg3> struct result {
    typedef Object type;
  };

  template <class Object, class Arg1, class Arg2, class Arg3>
  Object operator()(Object const ignored, Arg1 &arg1, Arg2 &arg2, Arg3 &arg3) const {
    return new typename boost::remove_pointer<Object>::type(arg1, arg2, arg3);
  }
};

phoenix::function<new_0_impl> new_0;
phoenix::function<new_1_impl> new_1;
phoenix::function<new_2_impl> new_2;
phoenix::function<new_3_impl> new_3;

} // namespace phoenix

#endif // FLEXUS_CORE_BOOST_EXTENSION_PHOENIX_HPP_INCLUDED
