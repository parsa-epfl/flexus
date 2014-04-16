#ifndef FLEXUS_TRANSPORTS__MEMORY_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__MEMORY_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/ExecuteState.hpp>
#include <components/Common/Slices/MemOp.hpp>
#include <components/Common/Slices/Mux.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>
#include <components/Common/Slices/DestinationMessage.hpp>
#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/TaglessDirMsg.hpp>

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_MemoryMessageTag
#define FLEXUS_TAG_MemoryMessageTag
struct MemoryMessageTag_t {};
namespace {
MemoryMessageTag_t MemoryMessageTag;
}
#endif //FLEXUS_TAG_MemoryMessageTag

#ifndef FLEXUS_TAG_ExecuteStateTag
#define FLEXUS_TAG_ExecuteStateTag
struct ExecuteStateTag_t {};
namespace {
ExecuteStateTag_t ExecuteStateTag;
}
#endif //FLEXUS_TAG_ExectueStateTag

#ifndef FLEXUS_TAG_uArchStateTag
#define FLEXUS_TAG_uArchStateTag
struct uArchStateTag_t {};
namespace {
uArchStateTag_t uArchStateTag;
}
#endif //FLEXUS_TAG_uArchStateTag

#ifndef FLEXUS_TAG_MuxTag
#define FLEXUS_TAG_MuxTag
struct MuxTag_t {};
namespace {
MuxTag_t MuxTag;
}
#endif //FLEXUS_TAG_MuxTag

#ifndef FLEXUS_TAG_BusTag
#define FLEXUS_TAG_BusTag
struct BusTag_t {};
namespace {
BusTag_t BusTag;
}
#endif //FLEXUS_TAG_BusTag

#ifndef FLEXUS_TAG_DirectoryEntryTag
#define FLEXUS_TAG_DirectoryEntryTag
struct DirectoryEntryTag_t {};
struct DirectoryEntry;
namespace {
DirectoryEntryTag_t DirectoryEntryTag;
}
#endif //FLEXUS_TAG_DirectoryEntryTag

#ifndef FLEXUS_TAG_TransactionTrackerTag
#define FLEXUS_TAG_TransactionTrackerTag
struct TransactionTrackerTag_t {};
struct TransactionTracker;
namespace {
TransactionTrackerTag_t TransactionTrackerTag;
}
#endif //FLEXUS_TAG_TransactionTrackerTag

#ifndef FLEXUS_TAG_DestinationTag
#define FLEXUS_TAG_DestinationTag
struct DestinationTag_t {};
namespace {
DestinationTag_t DestinationTag;
}
#endif //FLEXUS_TAG_DestinationTag

#ifndef FLEXUS_TAG_NetworkMessageTag
#define FLEXUS_TAG_NetworkMessageTag
struct NetworkMessageTag_t {};
namespace {
NetworkMessageTag_t NetworkMessageTag;
}
#endif //FLEXUS_TAG_NetworkMessageTag

#ifndef FLEXUS_TAG_TaglessDirMsgTag
#define FLEXUS_TAG_TaglessDirMsgTag
struct TaglessDirMsgTag_t {};
namespace {
TaglessDirMsgTag_t TaglessDirMsgTag;
}
#endif //FLEXUS_TAG_TaglessDirMsgTag

typedef Transport
< mpl::vector
< transport_entry< MemoryMessageTag_t, MemoryMessage >
, transport_entry< ExecuteStateTag_t, ExecuteState >
, transport_entry< uArchStateTag_t, MemOp >
, transport_entry< MuxTag_t, Mux >
, transport_entry< BusTag_t, Mux >
, transport_entry< DirectoryEntryTag_t, DirectoryEntry >
, transport_entry< TransactionTrackerTag_t, TransactionTracker >
, transport_entry< DestinationTag_t, DestinationMessage >
, transport_entry< NetworkMessageTag_t, NetworkMessage >
, transport_entry< TaglessDirMsgTag_t, TaglessDirMsg >
>
> MemoryTransport;

} //namespace SharedTypes
} //namespace Flexus

#endif //FLEXUS_TRANSPORTS__MEMORY_TRANSPORT_HPP_INCLUDED

