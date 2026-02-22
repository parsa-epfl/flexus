
#include "bbv.hpp"

#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>

namespace nuArch {

static const int32_t kCountThreshold = 50000;

struct BBVTrackerImpl : public BBVTracker
{
    int32_t theIndex;
    int32_t theDumpNo;
    int32_t theCountSinceDump;

    PhysicalMemoryAddress theLastPC;
    bool theLastWasBranch;

    std::map<uint64_t, long> theBBV;

    BBVTrackerImpl(int32_t anIndex)
      : theIndex(anIndex)
      , theDumpNo(0)
      , theCountSinceDump(0)
      , theLastPC(PhysicalMemoryAddress(0))
      , theLastWasBranch(false)
    {
        getAllTrackers().push_back(this);
    }

    virtual ~BBVTrackerImpl() {}

    virtual void commitInsn(PhysicalMemoryAddress aPC, bool isBranch)
    {
        uint64_t current_pc = static_cast<uint64_t>(aPC);
        uint64_t last_pc = static_cast<uint64_t>(theLastPC);
        bool same_basic_block = (current_pc == last_pc + 4 && !theLastWasBranch);

        if (!same_basic_block) {
            ++theBBV[current_pc];
        }

        theLastPC        = aPC;
        theLastWasBranch = isBranch;

        ++theCountSinceDump;
        if (theCountSinceDump >= kCountThreshold) {
            theCountSinceDump = 0;
            theBBV.clear();
        }
    }

    virtual void dumpToStream(std::ostream& anOstream, int32_t aCoreId) override
    {
        anOstream << "  {" << std::endl;
        anOstream << "    \"core\": " << aCoreId << "," << std::endl;
        anOstream << "    \"bbv\": {" << std::endl;
        bool first = true;
        for (auto const& kv : theBBV) {
            if (!first) {
                anOstream << "," << std::endl;
            }
            anOstream << "      \"0x" << std::hex << kv.first << "\": " << std::dec << kv.second;
            first = false;
        }
        anOstream << std::endl << "    }" << std::endl;
        anOstream << "  }" << std::endl;
    }
};

std::vector<BBVTracker*>& 
BBVTracker::getAllTrackers() 
{
    static std::vector<BBVTracker*> trackers;
    return trackers;
}

void 
BBVTracker::dumpAllBBV(std::ostream& anOstream)
{
    auto& trackers = getAllTrackers();
    if (trackers.empty()) {
        return;
    }
    
    anOstream << "[" << std::endl;
    for (size_t i = 0; i < trackers.size(); ++i) {
        trackers[i]->dumpToStream(anOstream, static_cast<int32_t>(i));
        if (i < trackers.size() - 1) {
            anOstream << "," << std::endl;
        }
    }
    anOstream << std::endl << "]" << std::endl;
}

BBVTracker*
BBVTracker::createBBVTracker(int32_t aCPUIndex)
{
    return new BBVTrackerImpl(aCPUIndex);
}

} // namespace nuArch
