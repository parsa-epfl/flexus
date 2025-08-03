#include <components/CommonQEMU/Slices/PredictorMessage.hpp>
#include <iostream>
#include <list>

namespace Flexus {
namespace SharedTypes {

std::ostream&
operator<<(std::ostream& anOstream, PredictorMessage::tPredictorMessageType aType)
{
    const char* const name[4] = { "eFlush", "eWrite", "eReadPredicted", "eReadNonPredicted" };
    anOstream << name[aType];
    return anOstream;
}

std::ostream&
operator<<(std::ostream& aStream, PredictorMessage const& anEntry)
{
    aStream << "PredictorMessage: type=" << anEntry.type() << " addr=" << &std::hex << anEntry.address() << &std::dec
            << " node=" << anEntry.node();

    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus
