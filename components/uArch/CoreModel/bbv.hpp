
#ifndef FLEXUS_uARCH_COREMODEL_BBV__INCLUDED
#define FLEXUS_uARCH_COREMODEL_BBV__INCLUDED

#include <iostream>
#include <vector>
#include "core/types.hpp"

namespace nuArch {
using Flexus::SharedTypes::PhysicalMemoryAddress;

struct BBVTracker
{
    static BBVTracker* createBBVTracker(int32_t aCPUIndex);
    static std::vector<BBVTracker*>& getAllTrackers();
    static void dumpAllBBV(std::ostream& anOstream);
    virtual void commitInsn(PhysicalMemoryAddress aPC, bool isBranch) = 0;
    virtual void dumpToStream(std::ostream& anOstream, int32_t aCoreId) = 0;
    virtual ~BBVTracker() {}
};

} // namespace nuArch

#endif // FLEXUS_uARCH_COREMODEL_BBV__INCLUDED
