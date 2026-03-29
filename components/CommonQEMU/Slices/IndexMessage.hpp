#ifndef FLEXUS_SLICES__INDEX_MESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__INDEX_MESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <iostream>
#include <list>

#define FLEXUS_IndexMessage_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

namespace IndexCommand {
enum IndexCommand
{
    eLookup,
    eInsert,
    eUpdate,
    eMatchReply,
    eNoMatchReply,
    eNoUpdateReply,
};

namespace {
char* IndexCommandStr[] = { "eLookup", "eInsert", "eUpdate", "eMatchReply", "eNoMatchReply", "eNoUpdateReply" };
}

inline std::ostream&
operator<<(std::ostream& aStream, IndexCommand cmd)
{
    if (cmd <= eNoMatchReply) { aStream << IndexCommandStr[cmd]; }
    return aStream;
}
} // namespace IndexCommand

struct IndexMessage : public boost::counted_base
{
    IndexCommand::IndexCommand theCommand;
    MemoryAddress theAddress;
    int32_t theTMSc;
    int32_t theCMOB;
    int64_t theCMOBOffset;
    uint64_t theStartTime;
    std::list<MemoryAddress> thePrefix;
};

inline std::ostream&
operator<<(std::ostream& aStream, const IndexMessage& msg)
{
    aStream << msg.theCommand << " TMSc[" << msg.theTMSc << "] " << msg.theAddress << " -> " << msg.theCMOB << " @"
            << msg.theCMOBOffset;
    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_COMMON_INDEXMESSAGE_HPP_INCLUDED
