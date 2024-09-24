#ifndef FLEXUS_SLICES__CMOB_MESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__CMOB_MESSAGE_HPP_INCLUDED

#include <bitset>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <iostream>

#ifdef FLEXUS_CMOBMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::CMOBMessage data type"
#endif
#define FLEXUS_CMOBMessage_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

struct CMOBLine
{
    static const int32_t kLineSize = 12;
    MemoryAddress theAddresses[kLineSize];
    std::bitset<kLineSize> theWasHit;
};

inline std::ostream&
operator<<(std::ostream& aStream, const CMOBLine& line)
{
    for (int32_t i = 0; i < 12; ++i) {
        aStream << std::hex << line.theAddresses[i] << (line.theWasHit[i] ? "(hit) " : "(nohit) ");
    }
    return aStream;
}

namespace CMOBCommand {
enum CMOBCommand
{
    eRead,
    eWrite,
    eInit
};
}
namespace {
char* CMOBCommandStr[] = { "eRead", "eWrite", "eInit" };
}

struct CMOBMessage : public boost::counted_base
{
    CMOBCommand::CMOBCommand theCommand;
    CMOBLine theLine;
    int64_t theCMOBOffset;
    uint32_t theCMOBId;
    int64_t theRequestTag;
};

inline std::ostream&
operator<<(std::ostream& aStream, const CMOBMessage& msg)
{
    aStream << "CMOB[" << msg.theCMOBId << "] #" << msg.theRequestTag << " " << CMOBCommandStr[msg.theCommand] << " @"
            << msg.theCMOBOffset << " " << msg.theLine;
    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_COMMON_CMOBMESSAGE_HPP_INCLUDED
