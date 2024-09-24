#include <components/CommonQEMU/Slices/MRCMessage.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <iostream>
#include <list>

namespace Flexus {
namespace SharedTypes {

std::ostream&
operator<<(std::ostream& s, MRCMessage const& aMsg)
{
    char const* types[] = { "Append", "RequestStream" };
    if (aMsg.tag() == -1) {
        s << "MRC " << types[aMsg.type()] << "#*";
    } else {
        s << "MRC " << types[aMsg.type()] << "#" << aMsg.tag() / 32 << "[" << (aMsg.tag() & 15) << "]"
          << ((aMsg.tag() & 16) ? "b" : "a");
    }
    s << " @" << aMsg.address() << "<" << aMsg.location() << ">";
    s << " by [" << boost::padded_string_cast<2, '0'>(aMsg.node()) << "]";
    return s;
}

} // namespace SharedTypes
} // namespace Flexus
