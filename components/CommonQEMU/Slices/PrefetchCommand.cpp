#include <components/CommonQEMU/Slices/PrefetchCommand.hpp>
#include <iostream>
#include <list>

namespace Flexus {
namespace SharedTypes {

std::ostream&
operator<<(std::ostream& s, PrefetchCommand const& aMsg)
{
    char const* prefetch_types[] = { "Prefetch Address List", "Prefetch More Addresses" };
    if (aMsg.tag() == -1) {
        s << prefetch_types[aMsg.type()] << "#* ";
    } else {
        s << prefetch_types[aMsg.type()] << "#" << aMsg.tag() / 32 << "[" << (aMsg.tag() & 15) << "]"
          << ((aMsg.tag() & 16) ? "b" : "a") << " ";
    }
    s << "from " << aMsg.source() << " <" << aMsg.location() << "> ";
    if (aMsg.queue() != -1) { s << "queue " << aMsg.queue() << " "; }
    s << "{" << &std::hex;
    std::list<PhysicalMemoryAddress>::const_iterator iter = aMsg.addressList().begin();
    std::list<PhysicalMemoryAddress>::const_iterator end  = aMsg.addressList().end();
    while (iter != end) {
        s << *iter;
        ++iter;
        if (iter != end) { s << ", "; }
    }
    s << "}" << &std::dec;
    return s;
}

} // namespace SharedTypes
} // namespace Flexus
