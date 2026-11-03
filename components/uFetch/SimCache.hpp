#ifndef FLEXUS_UFETCH_SIMCACHE
#define FLEXUS_UFETCH_SIMCACHE
#include "components/CommonQEMU/seq_map.hpp"
#include "core/checkpoint/json.hpp"
#include "core/debug/debug.hpp"

#include <fstream>
using json = nlohmann::json;

#define LOG2(x)                                                                                                        \
    ({                                                                                                                 \
        uint64_t _x = (x);                                                                                             \
        _x ? (63 - __builtin_clzl(_x)) : 0;                                                                            \
    })

// Keep provenance with the actual resident tag, so eviction/invalidation also
// removes its measurement state. These fields never affect replacement.
enum class FetchOrigin
{
    Demand,
    Prefetch
};

struct SimCacheLineInfo
{
    FetchOrigin origin = FetchOrigin::Demand;
    // First demand claim for this line lifetime, including one that joined its
    // outstanding fetch. Carrying that claim through fill prevents a late
    // prefetch from also becoming timely or over-fetch after a squash.
    bool used = false;
};

typedef flexus_boost_set_assoc<uint64_t, SimCacheLineInfo> SimCacheArray;
typedef SimCacheArray::iterator SimCacheIter;

struct SimCache
{

    SimCacheArray theCache;
    int32_t theCacheSize;
    int32_t theCacheAssoc;
    int32_t theCacheBlockShift;
    int32_t theBlockSize;
    std::string theName;

    void init(int32_t aCacheSize, int32_t aCacheAssoc, int32_t aBlockSize, const std::string& aName)
    {
        theCacheSize       = aCacheSize;
        theCacheAssoc      = aCacheAssoc;
        theBlockSize       = aBlockSize;
        theCacheBlockShift = LOG2(theBlockSize);
        theCache.init(theCacheSize / theBlockSize, theCacheAssoc, 0);
        theName = aName;
    }

    void loadState(std::string const& filename)
    {
        std::string ckpt_filename(filename);

        std::ifstream ifs(ckpt_filename.c_str(), std::ios::in);

        if (!ifs.good()) {
            DBG_(Dev, (<< "checkpoint file: " << ckpt_filename << " not found."));
            DBG_Assert(false, (<< "FILE NOT FOUND"));
        }

        json checkpoint;

        ifs >> checkpoint;
        uint32_t tag_shift = LOG2(theCache.sets());

        DBG_Assert((uint64_t)theCacheAssoc == checkpoint["associativity"]);
        DBG_Assert((uint64_t)theCache.sets() == checkpoint["tags"].size());

        for (std::size_t i{ 0 }; i < theCache.sets(); i++) {
            if (checkpoint["tags"].at(i).size() == 0) {
                continue;
            }

            for (uint32_t j = 0; j < checkpoint["tags"].at(i).size(); j++) {
                uint64_t tag  = checkpoint["tags"].at(i).at(j)["tag"];

                // Checkpoints contain tags, not prefetch provenance. Treat a
                // restored line as demand-origin, awaiting its first observed use.
                this->insert(((tag << tag_shift) | i) << theCacheBlockShift);
            }
        }

        ifs.close();
    }

    // Optional measurement output: zero is the existing return for both no
    // replacement and eviction of physical line zero. Preserve that behavior
    // while allowing next-line statistics to distinguish the two cases.
    uint64_t insert(uint64_t addr,
                    bool* replaced = nullptr,
                    SimCacheLineInfo incoming = {},
                    SimCacheLineInfo* evicted = nullptr)
    {
        if (replaced)
            *replaced = false;
        uint64_t ret_val  = 0;
        addr              = addr >> theCacheBlockShift;
        SimCacheIter iter = theCache.find(addr);
        if (iter != theCache.end()) {
            theCache.move_back(iter);
            // A duplicate reply does not begin a new residence or reset usage.
            return ret_val; // already present
        }
        if ((int)theCache.size() >= theCacheAssoc) {
            if (replaced)
                *replaced = true;
            if (evicted)
                *evicted = theCache.front();
            ret_val = theCache.front_key() << theCacheBlockShift;
            theCache.pop_front();
        }
        theCache.insert(std::make_pair(addr, incoming));
        return ret_val;
    }

    bool lookup(uint64_t addr)
    {
        addr              = addr >> theCacheBlockShift;
        SimCacheIter iter = theCache.find(addr);
        if (iter != theCache.end()) {
            theCache.move_back(iter);
            return true; // present
        }
        return false; // not present
    }

    // Prefetch filtering checks tags without a demand data access or LRU update.
    bool lookupTag(uint64_t addr)
    {
        return theCache.find(addr >> theCacheBlockShift) != theCache.end();
    }

    // Measurement-only access: finding metadata neither fetches data nor moves
    // the tag in LRU order. The pointer remains valid until the tag is removed.
    SimCacheLineInfo* lineInfo(uint64_t addr)
    {
        SimCacheIter iter = theCache.find(addr >> theCacheBlockShift);
        return iter == theCache.end() ? nullptr : &iter->second;
    }

    bool inval(uint64_t addr, SimCacheLineInfo* removed = nullptr)
    {
        addr              = addr >> theCacheBlockShift;
        SimCacheIter iter = theCache.find(addr);
        if (iter != theCache.end()) {
            if (removed)
                *removed = iter->second;
            theCache.erase(iter);
            return true; // invalidated
        }
        return false; // not present
    }
};

#endif
