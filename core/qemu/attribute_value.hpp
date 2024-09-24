#ifndef FLEXUS_QEMU_ATTRIBUTE_VALUE_HPP_INCLUDED
#define FLEXUS_QEMU_ATTRIBUTE_VALUE_HPP_INCLUDED

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/for_each.hpp>
#include <core/debug/debug.hpp>
#include <string>
namespace mpl = boost::mpl;

#include <core/exception.hpp>
#include <core/qemu/api_wrappers.hpp>

namespace Flexus {
namespace Qemu {

namespace aux_ {
// Forward declare AttributeFriend for the friend declaration in AttributeValue
class AttributeFriend;
struct Object
{};
struct BuiltIn
{};
struct construct_tag
{};
} // namespace aux_

struct Nil
{};

} // namespace Qemu
} // namespace Flexus

#endif // FLEXUS_SIMICS_ATTRIBUTE_VALUE_HPP_INCLUDED
