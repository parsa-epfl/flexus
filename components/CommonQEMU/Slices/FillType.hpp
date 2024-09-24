#ifndef FLEXUS_SLICES__FillType_HPP_INCLUDED
#define FLEXUS_SLICES__FillType_HPP_INCLUDED

#include <ostream>

namespace Flexus {
namespace SharedTypes {

enum tFillType
{
    eCold,
    eReplacement,
    eCoherence,
    eDGP,
    eDMA,
    ePrefetch,
    eFetch,
    eNAW,
    eDataPrivate,
    eDataSharedRO,
    eDataSharedRW,
};

std::ostream&
operator<<(std::ostream& anOstream, tFillType aType);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__FillType_HPP_INCLUDED