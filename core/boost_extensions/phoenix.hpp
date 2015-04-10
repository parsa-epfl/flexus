#ifndef FLEXUS_CORE_BOOST_EXTENSION_PHOENIX_HPP_INCLUDED
#define FLEXUS_CORE_BOOST_EXTENSION_PHOENIX_HPP_INCLUDED

#define PHOENIX_LIMIT 6

#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/spirit/include/phoenix1_casts.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/type_traits/remove_pointer.hpp>

namespace phoenix {
template <> struct rank<char const *> {
  static int32_t const value = 160;
};

template <> struct rank<char const * const> {
  static int32_t const value = 160;
};

struct new_0_impl {
  template <class Object>
  struct result {
    typedef Object type;
  };

  template <class Object >
  Object operator()(Object const ignored) const {
    return new typename boost::remove_pointer<Object>::type ;
  }
};

struct new_1_impl {
  template <class Object, class Arg1 >
  struct result {
    typedef Object type;
  };

  template <class Object, class Arg1>
  Object operator()(Object const ignored, Arg1 & arg1) const {
    return new typename boost::remove_pointer<Object>::type (arg1) ;
  }
};

struct new_2_impl {
  template <class Object, class Arg1, class Arg2 >
  struct result {
    typedef Object type;
  };

  template <class Object, class Arg1, class Arg2 >
  Object operator()(Object const ignored, Arg1 & arg1, Arg2 & arg2) const {
    return new typename boost::remove_pointer<Object>::type (arg1, arg2) ;
  }
};

struct new_3_impl {
  template <class Object, class Arg1, class Arg2, class Arg3>
  struct result {
    typedef Object type;
  };

  template <class Object, class Arg1, class Arg2, class Arg3 >
  Object operator()(Object const ignored, Arg1 & arg1, Arg2 & arg2, Arg3 & arg3 ) const {
    return new typename boost::remove_pointer<Object>::type (arg1, arg2, arg3) ;
  }
};

phoenix::function<new_0_impl> new_0;
phoenix::function<new_1_impl> new_1;
phoenix::function<new_2_impl> new_2;
phoenix::function<new_3_impl> new_3;

} //phoenix

#endif //FLEXUS_CORE_BOOST_EXTENSION_PHOENIX_HPP_INCLUDED

