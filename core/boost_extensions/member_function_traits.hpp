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
