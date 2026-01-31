#ifndef FLEXUS_TRANSPORTS__DIRECTORY_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__DIRECTORY_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_DirectoryMessageTag
#define FLEXUS_TAG_DirectoryMessageTag
struct DirectoryMessageTag_t
{};
struct DirectoryMessage;
namespace {
DirectoryMessageTag_t DirectoryMessageTag;
}
#endif // FLEXUS_TAG_DirectoryMessageTag

#ifndef FLEXUS_TAG_DirectoryEntryTag
#define FLEXUS_TAG_DirectoryEntryTag
struct DirectoryEntryTag_t
{};
struct DirectoryEntry;
namespace {
DirectoryEntryTag_t DirectoryEntryTag;
}
#endif // FLEXUS_TAG_DirectoryEntryTag

#ifndef FLEXUS_TAG_DirMux2ArbTag
#define FLEXUS_TAG_DirMux2ArbTag
struct DirMux2ArbTag_t
{};
struct Mux;
namespace {
DirMux2ArbTag_t DirMux2ArbTag;
}
#endif // FLEXUS_TAG_DirMux2ArbTag

typedef Transport<mpl::vector<transport_entry<DirectoryMessageTag_t, DirectoryMessage>,
                              transport_entry<DirectoryEntryTag_t, DirectoryEntry>,
                              transport_entry<DirMux2ArbTag_t, Mux>>>
  DirectoryTransport;

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_TRANSPORTS__DIRECTORY_TRANSPORT_HPP_INCLUDED
