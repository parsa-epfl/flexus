#ifndef __FASTCMPCACHE_COHERENCE_STATES_HPP__
#define __FASTCMPCACHE_COHERENCE_STATES_HPP__

namespace nFastCMPCache {

// Define states
typedef uint16_t CoherenceState_t;

static const CoherenceState_t kMigratory = 0x0F;
static const CoherenceState_t kModified  = 0x07;
static const CoherenceState_t kOwned     = 0x05;
static const CoherenceState_t kExclusive = 0x03;
static const CoherenceState_t kShared    = 0x01;
static const CoherenceState_t kInvalid   = 0x00;

static const CoherenceState_t kValid_Bit     = 0x1;
static const CoherenceState_t kExclusive_Bit = 0x2;
static const CoherenceState_t kDirty_Bit     = 0x4;
static const CoherenceState_t kMigrate_Bit   = 0x8;

inline std::string
Short2State(CoherenceState_t s)
{
    switch (s) {
        case kMigratory: return "Migratory"; break;
        case kModified: return "Modified"; break;
        case kOwned: return "Owned"; break;
        case kExclusive: return "Exclusive"; break;
        case kShared: return "Shared"; break;
        case kInvalid: return "Invalid"; break;
        default: return "Unknown"; break;
    }
}
inline std::string
state2String(CoherenceState_t s)
{
    return Short2State(s);
}

inline bool
isValid(CoherenceState_t s)
{
    return ((s & kValid_Bit) != 0);
}
}; // namespace nFastCMPCache

#endif
