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

typedef flexus_boost_set_assoc<uint64_t, int> SimCacheArray;
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

                this->insert(((tag << tag_shift) | i) << theCacheBlockShift);
            }
        }

        ifs.close();
    }

    uint64_t insert(uint64_t addr)
    {
        uint64_t ret_val  = 0;
        addr              = addr >> theCacheBlockShift;
        SimCacheIter iter = theCache.find(addr);
        if (iter != theCache.end()) {
            theCache.move_back(iter);
            return ret_val; // already present
        }
        if ((int)theCache.size() >= theCacheAssoc) {
            ret_val = theCache.front_key() << theCacheBlockShift;
            theCache.pop_front();
        }
        theCache.insert(std::make_pair(addr, 0));
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

    bool inval(uint64_t addr)
    {
        addr              = addr >> theCacheBlockShift;
        SimCacheIter iter = theCache.find(addr);
        if (iter != theCache.end()) {
            theCache.erase(iter);
            return true; // invalidated
        }
        return false; // not present
    }
};

#endif
