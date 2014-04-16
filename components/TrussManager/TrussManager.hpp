#define FLEXUS_BEGIN_COMPONENT TrussManager
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define TrussManager_IMPLEMENTATION (<components/TrussManager/TrussManagerImpl.hpp>)

#ifdef FLEXUS_TrussManager_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::TrussManager data type"
#endif

#define FLEXUS_TrussManager_TYPE_PROVIDED

namespace nTrussManager {

typedef Flexus::Core::index_t node_id_t;
typedef uint64_t CycleCount;

struct TrussManager : public boost::counted_base {
  static boost::intrusive_ptr<TrussManager> getTrussManager();
  static boost::intrusive_ptr<TrussManager> getTrussManager(node_id_t aRequestingNode);

  virtual bool isMasterNode(node_id_t aNode) = 0;
  virtual bool isSlaveNode(node_id_t aNode) = 0;
  virtual int32_t getSlaveIndex(node_id_t aNode) = 0;
  virtual node_id_t getMasterIndex(node_id_t aNode) = 0;
  virtual node_id_t getProcIndex(node_id_t aNode) = 0;
  virtual node_id_t getSlaveNode(node_id_t aNode, int32_t slave_idx) = 0;
  virtual int32_t getFixedDelay() = 0;

};

struct TrussMessage : public boost::counted_base, public FastAlloc {
  TrussMessage() : theTimestamp(0) {}
  TrussMessage(CycleCount cycle, boost::intrusive_ptr<Flexus::SharedTypes::ProtocolMessage> msg)
    : theTimestamp(cycle), theProtocolMessage(msg) {}
  TrussMessage(CycleCount cycle, boost::intrusive_ptr<Flexus::SharedTypes::MemoryMessage> msg)
    : theTimestamp(cycle), theMemoryMessage(msg) {}

  CycleCount theTimestamp;
  boost::intrusive_ptr<Flexus::SharedTypes::ProtocolMessage> theProtocolMessage;
  boost::intrusive_ptr<Flexus::SharedTypes::MemoryMessage> theMemoryMessage;
};

} //nTrussManager

namespace Flexus {
namespace SharedTypes {

using nTrussManager::TrussManager;
using nTrussManager::node_id_t;

struct TrussMessageTag_t {} TrussMessageTag;
#define FLEXUS_TrussMessage_TYPE_PROVIDED
typedef nTrussManager::TrussMessage TrussMessage;

} //End SharedTypes
} //End Flexus

namespace nTrussManager {

//Add TrussMessage to NetworkTransport
//=================================

typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_NetworkTransport_Typemap,
std::pair <
Flexus::SharedTypes::TrussMessageTag_t
, Flexus::SharedTypes::TrussMessage
>
>::type NetworkTransport_Typemap;

#undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
#define FLEXUS_PREVIOUS_NetworkTransport_Typemap nTrussManager::NetworkTransport_Typemap

} //nTrussManager

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TrussManager
