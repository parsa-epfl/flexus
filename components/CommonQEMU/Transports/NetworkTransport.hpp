#ifndef FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED

#include <boost/mpl/vector.hpp>

#include <components/CommonQEMU/Slices/NetworkMessage.hpp>
#include <components/CommonQEMU/Slices/ProtocolMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <core/transport.hpp>

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_NetworkMessageTag
#define FLEXUS_TAG_NetworkMessageTag
struct NetworkMessageTag_t
{};
struct NetworkMessage;
namespace {
NetworkMessageTag_t NetworkMessageTag;
}
#endif // FLEXUS_TAG_NetworkMessageTag

#ifndef FLEXUS_TAG_ProtocolMessageTag
#define FLEXUS_TAG_ProtocolMessageTag
struct ProtocolMessageTag_t
{};
struct ProtocolMessage;
namespace {
ProtocolMessageTag_t ProtocolMessageTag;
}
#endif // FLEXUS_TAG_NetworkMessageTag

#ifndef FLEXUS_TAG_TransactionTrackerTag
#define FLEXUS_TAG_TransactionTrackerTag
struct TransactionTrackerTag_t
{};
struct TransactionTracker;
namespace {
TransactionTrackerTag_t TransactionTrackerTag;
}
#endif // FLEXUS_TAG_TransactionTrackerTag

typedef Transport<mpl::vector<transport_entry<NetworkMessageTag_t, NetworkMessage>,
                              transport_entry<ProtocolMessageTag_t, ProtocolMessage>,
                              transport_entry<TransactionTrackerTag_t, TransactionTracker>>>
  NetworkTransport;

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED
