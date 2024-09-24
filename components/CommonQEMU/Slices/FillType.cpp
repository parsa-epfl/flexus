
#include <components/CommonQEMU/Slices/FillType.hpp>
#include <list>
#include <ostream>

namespace Flexus {
namespace SharedTypes {

std::ostream&
operator<<(std::ostream& anOstream, tFillType aType)
{
    const char* const name[] = { "eCold",  "eReplacement", "eCoherence",   "eDGP",          "eDMA",         "ePrefetch",
                                 "eFetch", "eNAW",         "eDataPrivate", "eDataSharedRO", "eDataSharedRW" };
    anOstream << name[aType];
    return anOstream;
}

} // namespace SharedTypes
} // namespace Flexus