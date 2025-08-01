#include <cstdint>
#include <tuple>
#include <optional>
#include <array>

template<uint32_t N_BLK>
std::tuple<uint64_t, uint64_t> get_base_offset(uint64_t addr) {
    uint32_t n_trailing_zeros = __builtin_ctzll(N_BLK);
    uint64_t base = addr >> n_trailing_zeros;
    uint64_t offset = addr & ((1ULL << n_trailing_zeros) - 1);
    return std::make_tuple(base, offset);
}

template<uint32_t N_BLK, uint32_t PHT_SETS, bool ROT>
uint64_t build_key(uint64_t pc, uint64_t offset) {
    const uint64_t pc_width = 16;
    uint32_t off_width = __builtin_ctzll(N_BLK);
    uint32_t index_len = __builtin_ctzll(PHT_SETS);

    if (ROT) {
        pc = pc & ((1ULL << (pc_width + off_width)) - 1);
        return pc;
    } else {
        pc = pc & ((1ULL << pc_width) - 1);
        offset = offset & ((1ULL << off_width) - 1);
        uint64_t key = (pc << off_width) | offset;
        return key;
    }
}

template<uint32_t N_BLK>
class AccTableEntry {
    uint64_t tag;
    uint64_t pc;
    uint64_t offset;
    bool access_pattern[N_BLK];
    bool read_pattern[N_BLK];
    uint64_t ts;
    bool valid;

public:
    AccTableEntry() {
        tag = 0;
        pc = 0;
        offset = 0;
        for (uint32_t i = 0; i < N_BLK; ++i) {
            access_pattern[i] = false;
            read_pattern[i] = false;
        }
        ts = 0;
        valid = false;
    }

    AccTableEntry& operator=(const AccTableEntry& other) {
        tag = other.tag;
        pc = other.pc;
        offset = other.offset;
        for (uint32_t i = 0; i < N_BLK; ++i) {
            access_pattern[i] = other.access_pattern[i];
            read_pattern[i] = other.read_pattern[i];
        }
        ts = other.ts;
        valid = other.valid;
        return *this;
    }

    void reset() {
        *this = AccTableEntry();
    }
};

template<uint32_t N_ACC, uint32_t N_BLK>
class AccTable{
    std::array<AccTableEntry<N_BLK>, N_ACC> entries;

public:
    AccTable() {
        for (uint32_t i = 0; i < N_ACC; ++i) {
            entries[i].reset();
        }
    }

    std::optional<uint32_t> poke(uint64_t addr) {
        uint64_t base, offset;
        std::tie(base, offset) = get_base_offset<N_BLK>(addr);
        for (int32_t i = 0; i < N_ACC; ++i) {
            if (entries[i].valid && entries[i].tag == base) {
                return i;   // Found a match
            }
        }
        return std::nullopt; // No match found
    }

    std::optional<AccTableEntry<N_BLK>> insert(AccTableEntry<N_BLK>& entry) {
        uint64_t lru_idx = 0, lru_ts = std::numeric_limits<uint64_t>::max();
        for (int32_t i = 0; i < N_ACC; ++i) {
            if (!entries[i].valid) {
                entries[i] = entry; // Insert in the first empty slot
                return std::nullopt;
            }
            if (entries[i].ts < lru_ts) {
                lru_ts = entries[i].ts;
                lru_idx = i;
            }
        }
        AccTableEntry<N_BLK> old_entry = entries[lru_idx];
        entries[lru_idx] = entry;
        return old_entry; // Return the evicted entry
    }

    bool poke_and_update(uint64_t addr, bool is_store, uint64_t ts) {
        uint64_t base, offset;
        std::tie(base, offset) = get_base_offset<N_BLK>(addr);
        auto idx = poke(addr);
        if(idx) {
            entries[*idx].ts = ts;
            if (!entries[*idx].access_pattern[offset]) {
                entries[*idx].access_pattern[offset] = true;
                entries[*idx].read_pattern[offset] = !is_store;
            }
        } else {
            return false;
        }
    }

    std::optional<AccTableEntry<N_BLK>> evict(uint64_t addr) {
        auto idx = poke(addr);
        if (idx) {
            entries[*idx].reset();
            return entries[*idx];
        }
        return std::nullopt; // No entry found
    }
};

class FilterTableEntry {
    uint64_t tag;
    uint64_t pc;
    uint64_t offset;
    bool is_read;
    bool valid;
    uint64_t ts;

public:
    FilterTableEntry() {
        tag = 0;
        pc = 0;
        offset = 0;
        is_read = false;
        valid = false;
        ts = 0;
    }

    FilterTableEntry& operator=(const FilterTableEntry& other) {
        tag = other.tag;
        pc = other.pc;
        offset = other.offset;
        is_read = other.is_read;
        valid = other.valid;
        ts = other.ts;
        return *this;
    }

    void reset() {
        *this = FilterTableEntry();
    }
};

template<uint32_t N_FILTER, uint32_t N_BLK>
class FilterTable{
    std::array<FilterTableEntry, N_FILTER> entries;

public:
    FilterTable() {
        for (uint32_t i = 0; i < N_FILTER; ++i) {
            entries[i].reset();
        }
    }

    std::optional<uint32_t> poke(uint64_t addr) {
        uint64_t base, offset;
        std::tie(base, offset) = get_base_offset<N_BLK>(addr);
        for (int32_t i = 0; i < N_FILTER; ++i) {
            if (entries[i].valid && entries[i].tag == base) {
                return i; // Found a match
            }
        }
        return std::nullopt; // No match found
    }

    void insert(FilterTableEntry& entry) {
        uint64_t lru_idx = 0, lru_ts = std::numeric_limits<uint64_t>::max();
        for (int32_t i = 0; i < N_FILTER; ++i) {
            if (!entries[i].valid) {
                entries[i] = entry; // Insert in the first empty slot
                return;
            }
            if (entries[i].ts < lru_ts) {
                lru_ts = entries[i].ts;
                lru_idx = i;
            }
        }
        entries[lru_idx] = entry; // Replace the LRU entry
    }

    std::optional<AccTableEntry<N_BLK>> poke_and_update(uint64_t addr, uint64_t pc, bool is_store, uint64_t ts) {
        uint64_t base, offset;
        std::tie(base, offset) = get_base_offset<N_BLK>(addr);
        auto idx = poke(addr);
        if (idx) {
            if (entries[*idx].offset == offset) {
                entries[*idx].pc = pc;
                entries[*idx].ts = ts;
                entries[*idx].is_read = !is_store;
                return std::nullopt; // No eviction
            } else {
                AccTableEntry<N_BLK> new_entry;
                new_entry.tag = entries[*idx].tag;
                new_entry.pc = entries[*idx].pc;
                new_entry.offset = entries[*idx].offset;
                new_entry.access_pattern[entries[*idx].offset] = true;
                new_entry.access_pattern[offset] = true;
                new_entry.read_pattern[entries[*idx].offset] = entries[*idx].is_read;
                new_entry.read_pattern[offset] = !is_store;
                new_entry.ts = ts;
                new_entry.valid = true;
                entries[*idx].reset();
                return new_entry;
            }
        } else {
            FilterTableEntry entry;
            entry.tag = base;
            entry.pc = pc;
            entry.offset = offset;
            entry.is_read = !is_store;
            entry.ts = ts;
            entry.valid = true;
            insert(entry);
            return std::nullopt; // No eviction
        }
    }

    void evict(uint64_t addr) {
        auto idx = poke(addr);
        if (idx) {
            entries[*idx].reset();
        }
    }
};

template<uint32_t N_ACC, uint32_t N_FILTER, uint32_t N_BLK>
class AGT {
    AccTable<N_ACC, N_BLK> acc_table;
    FilterTable<N_FILTER, N_BLK> filter_table;

public:
    AGT() {}

    std::optional<AccTableEntry<N_BLK>> record(uint64_t addr, uint64_t pc, bool is_store, uint64_t ts) {
        bool update = acc_table.poke_and_update(addr, is_store, ts);
        if (update) {
            return std::nullopt; // No eviction
        } else {
            auto res = filter_table.poke_and_update(addr, pc, is_store, ts);
            if (res) {
                acc_table.insert(*res);
            } else {
                return std::nullopt; // No eviction
            }
        }
    }

    std::optional<AccTableEntry<N_BLK>> evict(uint64_t addr) {
        filter_table.evict(addr);
        return acc_table.evict(addr);
    }
};

template<uint32_t N_BLK, bool ROT, bool SEP_RDWR, bool SAT_CNT>
class PHTEntry{
    uint64_t tag;
    std::array<uint8_t, N_BLK> access_pattern;
    std::array<uint8_t, N_BLK> write_pattern;
    std::array<uint8_t, N_BLK> read_pattern;
    uint64_t ts;
    bool valid;

public:
    PHTEntry() {
        tag = 0;
        for (uint32_t i = 0; i < N_BLK; ++i) {
            access_pattern[i] = 1;
            write_pattern[i] = 1;
            read_pattern[i] = 1;
        }
        ts = 0;
        valid = false;
    }

    void set(uint64_t aTag, AccTableEntry<N_BLK>& entry) {
        tag = aTag;
        ts = entry.ts;
        valid = true;
        if SEP_RDWR {
           for (uint32_t i = 0; i < N_BLK; ++i) {
                read_pattern[i] = (entry.access_pattern[i]) ? (entry.read_pattern[i] ? 2 : 1) : 1;
                write_pattern[i] = (entry.access_pattern[i]) ? (entry.read_pattern[i] ? 1 : 2) : 1;
           } 
        } else {
            for (uint32_t i = 0; i < N_BLK; ++i) {
                access_pattern[i] = (entry.access_pattern[i]) ? 2 : 1;
            }
        }
        if ROT {
            if SEP_RDWR {
                std::rotate(read_pattern.begin(), read_pattern.begin() + entry.offset, read_pattern.end());
                std::rotate(write_pattern.begin(), write_pattern.begin() + entry.offset, write_pattern.end());
            } else {
                std::rotate(access_pattern.begin(), access_pattern.begin() + entry.offset, access_pattern.end());
            }
        }
    }

    enum PatternType {
        access,
        read,
        write
    };

    void update_pattern(std::array<bool, N_BLK>& pattern, PatternType type) {
        for(uint32_t i = 0; i < N_BLK; ++i) {
            uint8_t& p = (type == PatternType::access) ? access_pattern[i] :
                                            (type == PatternType::read) ? read_pattern[i] : write_pattern[i];

            if SAT_CNT {
                p = pattern[i] ? std::min<uint8_t>(p + 1, 3) : std::max<uint8_t>(p - 1, 0);
            } else {
                p = (pattern[i]) ? 2 : 1;
            }
        }
    }

    void update(AccTableEntry<N_BLK>& entry) {
        if SEP_RDWR {
            std::array<bool, N_BLK> read_pattern = entry.read_pattern;
            std::array<bool, N_BLK> write_pattern;
            for(uint32 i = 0; i < N_BLK; ++i) {
                write_pattern[i] = entry.access_pattern[i] && !entry.read_pattern[i];
            }
            if ROT {
                std::rotate(read_pattern.begin(), read_pattern.begin() + entry.offset, read_pattern.end());
                std::rotate(write_pattern.begin(), write_pattern.begin() + entry.offset, write_pattern.end());
            }
            update_pattern(read_pattern, PatternType::read);
            update_pattern(write_pattern, PatternType::write);
        } else {
            std::array<bool, N_BLK> access_pattern = entry.access_pattern;
            if ROT {
                std::rotate(access_pattern.begin(), access_pattern.begin() + entry.offset, access_pattern.end());
            }
            update_pattern(access_pattern, PatternType::access);
        }
        ts = entry.ts;
    }

    std::array<bool, N_BLK> get_bitvec(bool is_read) {
        std::array<bool, N_BLK> bitvec;
        if SEP_RDWR {
            for (uint32_t i = 0; i < N_BLK; ++i) {
                bitvec[i] = is_read ? (read_pattern[i] >= 2) : (write_pattern[i] >= 2);
            }
        } else {
            for (uint32_t i = 0; i < N_BLK; ++i) {
                bitvec[i] = access_pattern[i] >= 2;
            }
        }
        return bitvec;
    }
};

template<uint32_t PHT_WAYS, uint32_t N_BLK, bool ROT, bool SEP_RDWR, bool SAT_CNT, bool PERFECT>
class PHTSet {
    std::vector<PHTEntry<N_BLK, ROT, SEP_RDWR, SAT_CNT>> entries;

public:
    PHTSet(): entries(PHT_WAYS) {
        for (uint32_t i = 0; i < PHT_WAYS; ++i) {
            entries[i] = PHTEntry<N_BLK, ROT, SEP_RDWR, SAT_CNT>();
        }
    }

    std::optional<std::array<bool, N_BLK>> lookup(uint64_t tag, bool is_read, uint64_t ts) {
        for (uint32_t i = 0; i < PHT_WAYS; ++i) {
            if (entries[i].valid && entries[i].tag == tag) {
                entries[i].ts = ts; // Update timestamp
                return entries[i].get_bitvec(is_read);
            }
        }
        return std::nullopt; // No match found
    }

    void insert(uint64_t tag, AccTableEntry<N_BLK>& entry) {
        bool empty = false;
        uint32_t empty_idx = 0, lru_idx = 0;
        uint64_t lru_ts = std::numeric_limits<uint64_t>::max();

        for(uint32_t i = 0; i < PHT_WAYS; ++i) {
            if (entries[i].valid && entries[i].tag == tag) {
                entries[i].update(entry);
                return; // Updated existing entry
            }
            if (!entries[i].valid && !empty) {
                empty = true;
                empty_idx = i; // Found an empty slot
            }
            if (entries[i].ts < lru_ts) {
                lru_ts = entries[i].ts;
                lru_idx = i; // Found the LRU entry
            }
        }
        if empty {
            entries[empty_idx].set(tag, entry);
        } else {
            if PERFECT {
                auto new_entry = PHTEntry<N_BLK, ROT, SEP_RDWR, SAT_CNT>();
                new_entry.set(tag, entry);
                entries.push_back(new_entry);
            } else {
                entries[lru_idx].set(tag, entry); // Replace the LRU entry
            }
        }
    }
};

template<uint32_t PHT_SETS, uint32_t PHT_WAYS, uint32_t N_BLK, bool ROT, bool SEP_RDWR, bool SAT_CNT, bool PERFECT>
class PHT {
    std::array<PHTSet<PHT_WAYS, N_BLK, ROT, SEP_RDWR, SAT_CNT, PERFECT>, PHT_SETS> sets;

public:
    PHT() {
        for (uint32_t i = 0; i < PHT_SETS; ++i) {
            sets[i] = PHTSet<PHT_WAYS, N_BLK, ROT, SEP_RDWR, SAT_CNT, PERFECT>();
        }
    }

    std::optional<std::array<uint64_t, N_BLK>> lookup(uint64_t pc, uint64_t addr, bool is_read, uint64_t ts) {
        uint64_t base, offset;
        std::tie(base, offset) = get_base_offset<N_BLK>(addr);
        uint64_t key = build_key<N_BLK, PHT_SETS, ROT>(pc, offset);
        uint32_t set_idx = key % PHT_SETS;
        uint64_t tag = key >> __builtin_ctzll(PHT_SETS);

        auto res = sets[set_idx].lookup(tag, is_read, ts);
        if (res) {
            if (ROT) {
                std::rotate(res->begin(), res->begin() - offset, res->end());
            }
            std::vector<uint64_t> addrs;
            for (uint32_t i = 0; i < N_BLK; ++i) {
                if ((*res)[i]) {
                    addrs.push_back(base + i);
                }
            }
            return addrs;
        } else {
            return std::nullopt; // No match found
        }
    }

    void insert(uint64_t tag, AccTableEntry<N_BLK>& entry) {
        uint64_t key = build_key<N_BLK, PHT_SETS, ROT>(entry.pc, entry.offset);
        uint32_t set_idx = key % PHT_SETS;
        uint64_t tag = key >> __builtin_ctzll(PHT_SETS);
        sets[set_idx].insert(tag, entry);
    }
};