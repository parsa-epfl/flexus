#define FLEXUS_BEGIN_COMPONENT TrussMemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define TrussMemoryLoopback_IMPLEMENTATION (<components/TrussMemoryLoopback/TrussMemoryLoopbackImpl.hpp>)

#include "../Common/MemoryMessage.hpp"
#include "../Common/TransactionTracker.hpp"

#ifdef FLEXUS_MemoryMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::MemoryMessage data type"
#endif

#ifdef FLEXUS_TransactionTracker_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::TransactionTracker data type"
#endif

namespace Flexus {
namespace SharedTypes {

struct MemoryMessageTag_t {} MemoryMessageTag;

#define FLEXUS_MemoryMessage_TYPE_PROVIDED
typedef MemoryCommon :: MemoryMessage MemoryMessage;

struct TransactionTrackerTag_t {} TransactionTrackerTag;
#define FLEXUS_TransactionTracker_TYPE_PROVIDED
typedef nTransactionTracker :: TransactionTracker TransactionTracker;

} //End SharedTypes
} //End Flexus

namespace nTrussMemoryLoopback {

typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_MemoryTransport_Typemap,
std::pair <
Flexus::SharedTypes::MemoryMessageTag_t
,   Flexus::SharedTypes::MemoryMessage
>
>::type added_memory_message;

typedef boost::mpl::push_front <
added_memory_message,
std::pair <
Flexus::SharedTypes::TransactionTrackerTag_t
,   Flexus::SharedTypes::TransactionTracker
>
>::type MemoryTransport_Typemap;

#undef FLEXUS_PREVIOUS_MemoryTransport_Typemap
#define FLEXUS_PREVIOUS_MemoryTransport_Typemap nTrussMemoryLoopback::MemoryTransport_Typemap

typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_NetworkTransport_Typemap,
std::pair <
Flexus::SharedTypes::TransactionTrackerTag_t
, Flexus::SharedTypes::TransactionTracker
>
>::type NetworkTransport_Typemap;

#undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
#define FLEXUS_PREVIOUS_NetworkTransport_Typemap nTrussMemoryLoopback::NetworkTransport_Typemap

} //End nTrussMemoryLoopback

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TrussMemoryLoopback
