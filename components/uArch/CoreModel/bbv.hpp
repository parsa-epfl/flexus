
#ifndef FLEXUS_uARCH_COREMODEL_BBV__INCLUDED
#define FLEXUS_uARCH_COREMODEL_BBV__INCLUDED

#include "core/types.hpp"

namespace nuArch {
using Flexus::SharedTypes::PhysicalMemoryAddress;

struct BBVTracker
{
    static BBVTracker* createBBVTracker(int32_t aCPUIndex);
    virtual void commitInsn(PhysicalMemoryAddress aPC, bool isBranch) = 0;
    virtual ~BBVTracker() {}
};

} // namespace nuArch

#endif // FLEXUS_uARCH_COREMODEL_BBV__INCLUDED
