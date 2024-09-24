#ifndef FLEXUS_SLICES__DIRECTORY_MESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__DIRECTORY_MESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <iostream>

#define FLEXUS_DirectoryMessage_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

typedef Flexus::SharedTypes::PhysicalMemoryAddress DirectoryAddress;
namespace DirectoryCommand {
enum DirectoryCommand
{
    Get,
    Found,
    Set,
    // Saved,
    Lock,
    Acquired,
    Unlock,
    // Released,
    Squash
};
}
namespace {
const char* DirectoryCommandStr[] = { "GetEntry",
                                      "EntryRetrieved",
                                      "SetEntry",
                                      //"EntryCommitted",
                                      "LockRequest",
                                      "LockAcquired",
                                      "UnlockRequest",
                                      //"LockReleased",
                                      "SquashPending" };
}

struct DirectoryMessage : public boost::counted_base
{
    DirectoryMessage(DirectoryCommand::DirectoryCommand anOp)
      : op(anOp)
      , addr(0)
    {
    }
    DirectoryMessage(DirectoryCommand::DirectoryCommand anOp, DirectoryAddress anAddr)
      : op(anOp)
      , addr(anAddr)
    {
    }
    DirectoryMessage(const DirectoryMessage& oldMsg)
      : op(oldMsg.op)
      , addr(oldMsg.addr)
    {
    }

    DirectoryCommand::DirectoryCommand op;
    DirectoryAddress addr;
};

inline std::ostream&
operator<<(std::ostream& aStream, const DirectoryMessage& msg)
{
    aStream << "DirMsg: op=" << DirectoryCommandStr[msg.op] << " addr=" << &std::hex << msg.addr << &std::dec;
    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_COMMON_MEMORYMESSAGE_HPP_INCLUDED
