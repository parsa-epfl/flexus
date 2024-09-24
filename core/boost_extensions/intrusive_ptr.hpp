#ifndef FLEXUS_CORE_BOOST_EXTENSIONS_INTRUSIVE_PTR_HPP_INCLUDED
#define FLEXUS_CORE_BOOST_EXTENSIONS_INTRUSIVE_PTR_HPP_INCLUDED

#include <boost/lambda/lambda.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <stdint.h>

namespace boost {

struct counted_base
{
    mutable int32_t theRefCount;

    counted_base()
      : theRefCount(0)
    {
    }
    virtual ~counted_base() {}
};

template<class T>
void
intrusive_ptr_add_ref(T* p)
{
    static_cast<boost::counted_base const*>(p)->theRefCount++;
}

template<class T>
void
intrusive_ptr_release(T* p)
{
    if (--static_cast<boost::counted_base const*>(p)->theRefCount == 0) { delete p; }
}

} // namespace boost

namespace boost {
namespace serialization {
template<class Archive, class T>
void
save(Archive& ar, ::boost::intrusive_ptr<T> const& ptr, uint32_t version)
{
    T* t = ptr.get();
    ar & t;
}

template<class Archive, class T>
void
load(Archive& ar, ::boost::intrusive_ptr<T>& ptr, uint32_t version)
{
    T* t;
    ar & t;
    ptr = boost::intrusive_ptr<T>(t);
}

template<class Archive, class T>
inline void
serialize(Archive& ar, ::boost::intrusive_ptr<T>& t, const uint32_t file_version)
{
    split_free(ar, t, file_version);
}
} // namespace serialization
} // namespace boost

namespace boost {
namespace lambda {
namespace detail {

template<class A>
struct contentsof_type<boost::intrusive_ptr<A>>
{
    typedef A& type;
};

} // namespace detail
} // namespace lambda
} // namespace boost

#endif // FLEXUS_CORE_BOOST_EXTENSIONS_INTRUSIVE_PTR_HPP_INCLUDED
