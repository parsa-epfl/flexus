
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>

#include "bbv.hpp"

#include <boost/lexical_cast.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

int32_t done_count = 0;

namespace nuArch {

static const int32_t kCountThreshold = 50000;

typedef std::map<uint64_t, int> bbindex_t;
bbindex_t theBBIndex;
int32_t theNextBBIndex = 0;
bool theMapChanged     = false;

struct BBVTrackerImpl : public BBVTracker
{
    int32_t theIndex;
    int32_t theDumpNo;
    int32_t theCountSinceDump;

    PhysicalMemoryAddress theLastPC;
    bool theLastWasBranch;
    bool theLastWasBranchDelay;

    std::map<uint64_t, long> theBBV;

    BBVTrackerImpl(int32_t anIndex)
      : theIndex(anIndex)
      , theDumpNo(0)
      , theCountSinceDump(0)
      , theLastPC(PhysicalMemoryAddress(0))
      , theLastWasBranch(false)
      , theLastWasBranchDelay(false)
    {
    }

    virtual ~BBVTrackerImpl() {}

    virtual void commitInsn(PhysicalMemoryAddress aPC, bool isBranch)
    {
        // Only process if we just processed a branch delay, or we had a
        // discontinuous fetch.

        bool same_basic_block = ((aPC == theLastPC + 4 && !theLastWasBranchDelay) || aPC == 0);
        theLastPC             = aPC;
        theLastWasBranchDelay = theLastWasBranch;
        theLastWasBranch      = isBranch;

        if (!same_basic_block) {
            // We have a new basic block. See if it has an index
            ++theBBV[aPC];
        }

        // See if it is time to dump stats
        ++theCountSinceDump;
        if (theCountSinceDump >= kCountThreshold) {
            dump();
            theCountSinceDump = 0;
            theBBV.clear();
        }
    }

    void dump()
    {
        if (theDumpNo > 100) return; // Don't bother dumping past 100
        // Write out and then clear the current BBV vector
        std::string name("bbv.cpu");
        name += boost::padded_string_cast<2, '0'>(theIndex);
        name += ".";
        name += boost::padded_string_cast<3, '0'>(theDumpNo);
        name += ".out";
        std::ofstream bbv(name.c_str());

        boost::archive::binary_oarchive oa(bbv);

        oa << const_cast<std::map<uint64_t, long> const&>(theBBV);

        // close archive
        bbv.close();
        ++theDumpNo;
        if (theDumpNo == 51) {
            ++done_count;
            if (done_count == 16) {
                DBG_Assert(false,
                           (<< "Halting simulation - all CPUs have executed "
                               "2.5M instructions"));
            }
        }
    }
};

BBVTracker*
BBVTracker::createBBVTracker(int32_t aCPUIndex)
{
    return new BBVTrackerImpl(aCPUIndex);
}

} // namespace nuArch
