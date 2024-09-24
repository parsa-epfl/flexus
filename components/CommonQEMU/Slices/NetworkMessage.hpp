#ifndef FLEXUS_SLICES__NETWORKMESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__NETWORKMESSAGE_HPP_INCLUDED

#ifdef FLEXUS_NetworkMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::NetworkMessage data type"
#endif
#define FLEXUS_NetworkMessage_TYPE_PROVIDED

#include <core/boost_extensions/intrusive_ptr.hpp>

namespace Flexus {
namespace SharedTypes {

struct NetworkMessage : public boost::counted_base
{
    int32_t src;  // source node
    int32_t dest; // destination node
    int32_t vc;   // virtual channel
    int32_t size; // message (i.e. payload) size
    int32_t src_port;
    int32_t dst_port;

    NetworkMessage()
      : src(-1)
      , dest(-1)
      , vc(-1)
      , size(-1)
      , src_port(-1)
      , dst_port(-1)
    {
    }
};

inline std::ostream&
operator<<(std::ostream& os, const NetworkMessage& msg)
{
    os << "Src: " << msg.src << ", Dest: " << msg.dest << ", VC: " << msg.vc << ", Size: " << msg.size;
    return os;
}

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__NETWORKMESSAGE_HPP_INCLUDED
