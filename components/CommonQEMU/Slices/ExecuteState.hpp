#ifndef FLEXUS_SLICES__ExecuteState_HPP_INCLUDED
#define FLEXUS_SLICES__ExecuteState_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#ifdef FLEXUS_ExecuteState_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ExecuteState data type"
#endif
#define FLEXUS_ExecuteState_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

struct ExecuteState : public boost::counted_base
{
  protected:
    // Only may be created by Execute component
    ExecuteState() {}
};

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__ExecuteState_HPP_INCLUDED
