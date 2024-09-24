#ifndef FLEXUS_SLICES__FillLevel_HPP_INCLUDED
#define FLEXUS_SLICES__FillLevel_HPP_INCLUDED

#include <ostream>
#include <string>

namespace Flexus {
namespace SharedTypes {

enum tFillLevel
{
    eUnknown,
    eL1,
    eL2,
    eL3,
    eLocalMem,
    eRemoteMem,
    ePrefetchBuffer,
    ePeerL1Cache,
    eL2Prefetcher,
    eL1I,
    eCore,
    ePeerL2,
    eDirectory,
    NumFillLevels
};

std::ostream&
operator<<(std::ostream& anOstream, tFillLevel aType);

std::string
fillLevelName(tFillLevel aType);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__FillLevel_HPP_INCLUDED