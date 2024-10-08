#ifndef FLEXUS_SLICES__Mux_HPP_INCLUDED
#define FLEXUS_SLICES__Mux_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#define FLEXUS_Mux_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {
struct Mux : public boost::counted_base
{
    explicit Mux(int32_t a)
      : source(a)
    {
    }
    int32_t source;
};

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__Mux_HPP_INCLUDED
