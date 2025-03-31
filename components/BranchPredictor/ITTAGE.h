#ifndef FLEXUS_ITTAGE
#define FLEXUS_ITTAGE

#include <cstdint>
#include <math.h>
#include <assert.h>
#include <components/uFetch/uFetchTypes.hpp>

using namespace Flexus;
using namespace SharedTypes;

class ITTAGE
{
private:
    struct ITTAGE_entry {
        uint64_t tag; // (partial) tag
        uint64_t target; // target
        uint64_t confidence; // confidence for indirect branches
        uint64_t rrpv; // replacement information, useful bit for ITTAGE
    };

    // parameters
    static const int MIN_HIST = 4;
    static const int MAX_HIST = 260;
    static const int ITTAGE_NTAB = 4;
    static const int ITTAGE_TAG_NBIT = 13;
    static const int ITTAGE_IDX_NBIT = 12;
    static const int ITTAGE_IDX_SIZE = 1 << ITTAGE_IDX_NBIT;

    ITTAGE_entry ittage_set[ITTAGE_NTAB][ITTAGE_IDX_SIZE];

    // branch history
    uint64_t btb_ghr[16];
    
    // ITTAGE
    int ittage_hlen[ITTAGE_NTAB];
    int64_t ittage_tick;
    int64_t ittage_hit_bank;
    int64_t ittage_alt_diff;
    int64_t ittage_use_alt;
    uint64_t ittage_hit_pred;
    uint64_t ittage_alt_pred;
    uint64_t ittage_hash_idx[ITTAGE_NTAB];
    uint64_t ittage_hash_tag[ITTAGE_NTAB];

public:
   ITTAGE();

    void     checkpointHistory(BPredState& aBPState) const;
    void     restore_history(const BPredState& aBPState);

    uint64_t predict(uint64_t ip);

    uint64_t update_target(uint64_t target, bool mispred, const BPredState& aBPState);
    void     update_history(uint64_t target);
};
#endif

