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
#ifndef BOOST_TT_MBR_FUNCTION_TRAITS_HPP_INCLUDED
#define BOOST_TT_MBR_FUNCTION_TRAITS_HPP_INCLUDED

#include <boost/config.hpp>

namespace boost {

namespace detail {

template <typename MemberFunction> struct member_function_traits_helper;

template <typename R, typename T> struct member_function_traits_helper<R (T::*)(void)> {
  BOOST_STATIC_CONSTANT(int, arity = 0);
  typedef T class_type;
  typedef R result_type;
};

template <typename R, typename T, typename T1> struct member_function_traits_helper<R (T::*)(T1)> {
  BOOST_STATIC_CONSTANT(int, arity = 1);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
};

template <typename R, typename T, typename T1>
struct member_function_traits_helper<R (T::*)(T1) const> {
  BOOST_STATIC_CONSTANT(int, arity = 1);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
};

template <typename R, typename T, typename T1, typename T2>
struct member_function_traits_helper<R (T::*)(T1, T2)> {
  BOOST_STATIC_CONSTANT(int, arity = 2);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
};

template <typename R, typename T, typename T1, typename T2>
struct member_function_traits_helper<R (T::*)(T1, T2) const> {
  BOOST_STATIC_CONSTANT(int, arity = 2);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3>
struct member_function_traits_helper<R (T::*)(T1, T2, T3)> {
  BOOST_STATIC_CONSTANT(int, arity = 3);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3>
struct member_function_traits_helper<R (T::*)(T1, T2, T3) const> {
  BOOST_STATIC_CONSTANT(int, arity = 3);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4)> {
  BOOST_STATIC_CONSTANT(int, arity = 4);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4) const> {
  BOOST_STATIC_CONSTANT(int, arity = 4);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5)> {
  BOOST_STATIC_CONSTANT(int, arity = 5);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5) const> {
  BOOST_STATIC_CONSTANT(int, arity = 5);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6)> {
  BOOST_STATIC_CONSTANT(int, arity = 6);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6) const> {
  BOOST_STATIC_CONSTANT(int, arity = 6);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7)> {
  BOOST_STATIC_CONSTANT(int, arity = 7);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7) const> {
  BOOST_STATIC_CONSTANT(int, arity = 7);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7, T8)> {
  BOOST_STATIC_CONSTANT(int, arity = 8);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
  typedef T8 arg8_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7, T8) const> {
  BOOST_STATIC_CONSTANT(int, arity = 8);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
  typedef T8 arg8_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9)> {
  BOOST_STATIC_CONSTANT(int, arity = 9);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
  typedef T8 arg8_type;
  typedef T9 arg9_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9) const> {
  BOOST_STATIC_CONSTANT(int, arity = 9);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
  typedef T8 arg8_type;
  typedef T9 arg9_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)> {
  BOOST_STATIC_CONSTANT(int, arity = 10);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
  typedef T8 arg8_type;
  typedef T9 arg9_type;
  typedef T10 arg10_type;
};

template <typename R, typename T, typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
struct member_function_traits_helper<R (T::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) const> {
  BOOST_STATIC_CONSTANT(int, arity = 10);
  typedef R result_type;
  typedef T class_type;
  typedef T1 arg1_type;
  typedef T2 arg2_type;
  typedef T3 arg3_type;
  typedef T4 arg4_type;
  typedef T5 arg5_type;
  typedef T6 arg6_type;
  typedef T7 arg7_type;
  typedef T8 arg8_type;
  typedef T9 arg9_type;
  typedef T10 arg10_type;
};

} // end namespace detail

template <typename MemberFunction>
struct member_function_traits : public detail::member_function_traits_helper<MemberFunction> {};

} // namespace boost

#endif // BOOST_TT_MBR_FUNCTION_TRAITS_HPP_INCLUDED
