#include <components/CommonQEMU/Slices/DirectoryEntry.hpp>

namespace Flexus {
namespace SharedTypes {

// for debug output
std::ostream&
operator<<(std::ostream& anOstream, tDirState const x)
{
    const char* const name[3] = { "DIR_STATE_INVALID", "DIR_STATE_SHARED", "DIR_STATE_MODIFIED" };
    DBG_Assert(x < static_cast<int>(sizeof(name)));
    anOstream << name[x];
    return anOstream;
}

std::ostream&
operator<<(std::ostream& aStream, DirectoryEntry const& anEntry)
{
    switch (anEntry.getState()) {
        case DIR_STATE_INVALID: aStream << anEntry.getState(); break;
        case DIR_STATE_SHARED: {
            bool first = true;
            aStream << anEntry.getState() << " Sharers{";
            for (int32_t i = 0; i < 16; ++i) {
                if (anEntry.theNodes & (1UL << i)) {
                    if (!first) { aStream << ","; }
                    first = false;
                    aStream << i;
                }
            }
            aStream << "}";
            break;
        }
        case DIR_STATE_MODIFIED: aStream << anEntry.getState() << " Owner=" << anEntry.theNodes; break;
    }
    aStream << ((anEntry.wasModified()) ? "  " : " !");
    aStream << "WasModified";
    bool first = true;
    aStream << anEntry.getState() << " PastSharers{";
    for (int32_t i = 0; i < 16; ++i) {
        if (anEntry.getPastReaders() & (1UL << i)) {
            if (!first) { aStream << ","; }
            first = false;
            aStream << i;
        }
    }
    aStream << "}";
    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus
