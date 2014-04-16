#define FLEXUS_BEGIN_COMPONENT SimicsMemory
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include "../Common/MemoryMessage.hpp"

#ifdef FLEXUS_MemoryMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::MemoryMessage data type"
#endif

namespace Flexus {
namespace SharedTypes {

struct MemoryMessageTag_t {} MemoryMessageTag;

#define FLEXUS_MemoryMessage_TYPE_PROVIDED
typedef MemoryCommon :: MemoryMessage MemoryMessage;

} //End SharedTypes

namespace SimicsMemory {

typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_MemoryTransport_Typemap,
std::pair <
Flexus::SharedTypes::MemoryMessageTag_t
,   Flexus::SharedTypes::MemoryMessage
>
>::type MemoryTransport_Typemap;

#undef FLEXUS_PREVIOUS_MemoryTransport_Typemap
#define FLEXUS_PREVIOUS_MemoryTransport_Typemap Flexus::SimicsMemory::MemoryTransport_Typemap

} //End SimicsMemory
} //End Flexus

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SimicsMemory
