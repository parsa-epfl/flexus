#define FLEXUS_BEGIN_COMPONENT NetworkTest
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define NetworkTest_IMPLEMENTATION (<components/Network/NetworkTestImpl.hpp>)

#include "../Common/TransactionTracker.hpp"

#ifdef FLEXUS_ProtocolMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ProtocolMessage data type"
#endif

namespace nNetworkTest {

using boost::counted_base;

struct ProtocolMessage : public boost::counted_base {
  uint32_t value;
};

} // End namespace nNetworkTest

namespace Flexus {
namespace SharedTypes {

struct ProtocolMessageTag_t {} ProtocolMessageTag;

#define FLEXUS_ProtocolMessage_TYPE_PROVIDED
typedef nNetworkTest :: ProtocolMessage ProtocolMessage;

} // End namespace SharedTypes
} // End namespace Flexus

namespace nNetworkTest {

typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_NetworkTransport_Typemap,
std::pair <
Flexus::SharedTypes::ProtocolMessageTag_t
, Flexus::SharedTypes::ProtocolMessage
>
>::type NetworkTransport_Typemap;

#undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
#define FLEXUS_PREVIOUS_NetworkTransport_Typemap nNetworkTest::NetworkTransport_Typemap

} // End namespace NetworkTest

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NetworkTest
