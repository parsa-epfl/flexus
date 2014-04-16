#define FLEXUS_BEGIN_COMPONENT Network
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define Network_IMPLEMENTATION (<components/Network/NetworkImpl.hpp>)

#ifdef FLEXUS_NetworkMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::NetworkMessage data type"
#endif

#include "../Common/TransactionTracker.hpp"

#include <core/fast_alloc.hpp>

namespace nNetwork {

using boost::counted_base;

struct NetworkMessage : public boost::counted_base, FastAlloc {
  int32_t src;     // source node
  int32_t dest;    // destination node
  int32_t vc;      // virtual channel
  int32_t size;    // message (i.e. payload) size
  int32_t src_port;
  int32_t dst_port;
};

} // End namespace nNetwork

namespace Flexus {
namespace SharedTypes {

struct NetworkMessageTag_t {} NetworkMessageTag;

#define FLEXUS_NetworkMessage_TYPE_PROVIDED
typedef nNetwork :: NetworkMessage NetworkMessage;

#ifndef FLEXUS_TransactionTracker_TYPE_PROVIDED
struct TransactionTrackerTag_t {} TransactionTrackerTag;
#define FLEXUS_TransactionTracker_TYPE_PROVIDED
typedef nTransactionTracker :: TransactionTracker TransactionTracker;
#endif //FLEXUS_TransactionTracker_TYPE_PROVIDED

} // End namespace SharedTypes
} // End namespace Flexus

namespace nNetwork {

typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_NetworkTransport_Typemap,
std::pair <
Flexus::SharedTypes::NetworkMessageTag_t
, Flexus::SharedTypes::NetworkMessage
>
>::type temp_Typemap;

typedef boost::mpl::push_front <
temp_Typemap,
std::pair <
Flexus::SharedTypes::TransactionTrackerTag_t
, Flexus::SharedTypes::TransactionTracker
>
>::type NetworkTransport_Typemap;

#undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
#define FLEXUS_PREVIOUS_NetworkTransport_Typemap nNetwork::NetworkTransport_Typemap

} // End namespace nNetwork

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Network
