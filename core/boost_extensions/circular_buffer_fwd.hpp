#if !defined(BOOST_CIRCULAR_BUFFER_FWD_HPP)
#define BOOST_CIRCULAR_BUFFER_FWD_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1200
#pragma once
#endif

#include <boost/config.hpp>
#if !defined(BOOST_NO_STD_ALLOCATOR)
#include <memory>
#else
#include <vector>
#endif

namespace boost {

#if !defined(BOOST_NO_STD_ALLOCATOR)
#define BOOST_CB_DEFAULT_ALLOCATOR(T) std::allocator<T>
#else
#define BOOST_CB_DEFAULT_ALLOCATOR(T) BOOST_DEDUCED_TYPENAME std::vector<T>::allocator_type
#endif

template <class T, class Alloc = BOOST_CB_DEFAULT_ALLOCATOR(T)> class circular_buffer;

template <class T, class Alloc = BOOST_CB_DEFAULT_ALLOCATOR(T)>
class circular_buffer_space_optimized;

#undef BOOST_CB_DEFAULT_ALLOCATOR

} // namespace boost

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_FWD_HPP)
