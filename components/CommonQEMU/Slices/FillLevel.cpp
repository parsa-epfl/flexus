#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <iostream>
#include <list>

namespace Flexus {
namespace SharedTypes {

std::string
fillLevelName(tFillLevel aType)
{
    const char* const name[] = { "eUnknown",        "eL1",          "eL2",           "eL3",  "eLocalMem", "eRemoteMem",
                                 "ePrefetchBuffer", "ePeerL1Cache", "eL2Prefetcher", "eL1I", "eCore",     "ePeerL2",
                                 "eDirectory" };
    return name[aType];
}

std::ostream&
operator<<(std::ostream& anOstream, tFillLevel aType)
{
    anOstream << fillLevelName(aType);
    return anOstream;
}

} // namespace SharedTypes
} // namespace Flexus
