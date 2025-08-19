#include "SMS.hpp"
#include "core/debug/debug.hpp"

std::tuple<uint64_t, uint64_t> get_base_offset(uint64_t addr, uint32_t N_BLK) {
    uint32_t n_trailing_zeros = __builtin_ctzll(N_BLK);
    uint64_t base = addr >> n_trailing_zeros;
    uint64_t offset = addr & ((1ULL << n_trailing_zeros) - 1);
    return std::make_tuple(base, offset);
}

uint64_t build_key(uint64_t pc, uint64_t offset, uint32_t N_BLK, uint32_t PHT_SETS, bool ROT) {
    const uint64_t pc_width = 16;
    uint32_t off_width = __builtin_ctzll(N_BLK);
    uint32_t index_len = __builtin_ctzll(PHT_SETS);
    DBG_Assert(pc_width + off_width > index_len);

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

uint64_t get_address(uint64_t base, uint64_t offset, uint32_t N_BLK) {
    return (base << __builtin_ctzll(N_BLK)) | offset;
}

AccTableEntry& AccTableEntry::operator=(const AccTableEntry& other) {
    if (this != &other) {
        tag = other.tag;
        pc = other.pc;
        offset = other.offset;
        access_pattern = other.access_pattern;
        read_pattern = other.read_pattern;
        ts = other.ts;
        valid = other.valid;
    }
    return *this;
}

void AccTableEntry::reset() {
    *this = AccTableEntry(N_BLK);
}

boost::optional<uint32_t> AccTable::poke(uint64_t addr) {
    uint64_t base, offset;
    std::tie(base, offset) = get_base_offset(addr, N_BLK);
    for (uint32_t i = 0; i < N_ACC; ++i) {
        if (entries[i].valid && entries[i].tag == base) {
            return i;   // Found a match
        }
    }
    return boost::none; // No match found
}

boost::optional<AccTableEntry> AccTable::insert(AccTableEntry& entry) {
    uint64_t lru_idx = 0, lru_ts = std::numeric_limits<uint64_t>::max();
    for (uint32_t i = 0; i < N_ACC; ++i) {
        if (!entries[i].valid) {
            entries[i] = entry; // Insert in the first empty slot
            return boost::none;
        }
        if (entries[i].ts < lru_ts) {
            lru_ts = entries[i].ts;
            lru_idx = i;
        }
    }
    AccTableEntry old_entry = entries[lru_idx];
    entries[lru_idx] = entry;
    return old_entry; // Return the evicted entry
}

bool AccTable::poke_and_update(uint64_t addr, bool is_store, uint64_t ts) {
    uint64_t base, offset;
    std::tie(base, offset) = get_base_offset(addr, N_BLK);
    auto idx = poke(addr);
    if(idx) {
        entries[*idx].ts = ts;
        if (!entries[*idx].access_pattern[offset]) {
            entries[*idx].access_pattern[offset] = true;
            entries[*idx].read_pattern[offset] = !is_store;
        }
        return true;
    } else {
        return false;
    }
}

boost::optional<AccTableEntry> AccTable::evict(uint64_t addr) {
    auto idx = poke(addr);
    if (idx) {
        entries[*idx].reset();
        return entries[*idx];
    }
    return boost::none; // No entry found
}

FilterTableEntry& FilterTableEntry::operator=(const FilterTableEntry& other) {
    tag = other.tag;
    pc = other.pc;
    offset = other.offset;
    is_read = other.is_read;
    valid = other.valid;
    ts = other.ts;
    return *this;
}

void FilterTableEntry::reset() {
    *this = FilterTableEntry();
}

boost::optional<uint32_t> FilterTable::poke(uint64_t addr) {
    uint64_t base, offset;
    std::tie(base, offset) = get_base_offset(addr, N_BLK);
    for (uint32_t i = 0; i < N_FILTER; ++i) {
        if (entries[i].valid && entries[i].tag == base) {
            return i; // Found a match
        }
    }
    return boost::none; // No match found
}

void FilterTable::insert(FilterTableEntry& entry) {
    uint64_t lru_idx = 0, lru_ts = std::numeric_limits<uint64_t>::max();
    for (uint32_t i = 0; i < N_FILTER; ++i) {
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

boost::optional<AccTableEntry> FilterTable::poke_and_update(uint64_t addr, uint64_t pc, bool is_store, uint64_t ts) {
    uint64_t base, offset;
    std::tie(base, offset) = get_base_offset(addr, N_BLK);
    auto idx = poke(addr);
    if (idx) {
        if (entries[*idx].offset == offset) {
            entries[*idx].pc = pc;
            entries[*idx].ts = ts;
            entries[*idx].is_read = !is_store;
            return boost::none; // No eviction
        } else {
            AccTableEntry new_entry = AccTableEntry(N_BLK);
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
        return boost::none; // No eviction
    }
}

void FilterTable::evict(uint64_t addr) {
    auto idx = poke(addr);
    if (idx) {
        entries[*idx].reset();
    }
}

boost::optional<AccTableEntry> AGT::record(uint64_t addr, uint64_t pc, bool is_store, uint64_t ts) {
    bool update = acc_table.poke_and_update(addr, is_store, ts);
    if (update) {
        return boost::none; // No eviction
    } else {
        auto res = filter_table.poke_and_update(addr, pc, is_store, ts);
        if (res) {
            return acc_table.insert(*res);
        } else {
            return boost::none; // No eviction
        }
    }
}

boost::optional<AccTableEntry> AGT::evict(uint64_t addr) {
    filter_table.evict(addr);
    return acc_table.evict(addr);
}

void PHTEntry::set(uint64_t aTag, AccTableEntry& entry) {
    tag = aTag;
    ts = entry.ts;
    valid = true;
    if (SEP_RDWR) {
        for (uint32_t i = 0; i < N_BLK; ++i) {
            read_pattern[i] = (entry.access_pattern[i]) ? (entry.read_pattern[i] ? 2 : 1) : 1;
            write_pattern[i] = (entry.access_pattern[i]) ? (entry.read_pattern[i] ? 1 : 2) : 1;
        } 
    } else {
        for (uint32_t i = 0; i < N_BLK; ++i) {
            access_pattern[i] = (entry.access_pattern[i]) ? 2 : 1;
        }
    }
    if (ROT) {
        if (SEP_RDWR) {
            std::rotate(read_pattern.begin(), read_pattern.begin() + entry.offset, read_pattern.end());
            std::rotate(write_pattern.begin(), write_pattern.begin() + entry.offset, write_pattern.end());
        } else {
            std::rotate(access_pattern.begin(), access_pattern.begin() + entry.offset, access_pattern.end());
        }
    }
}

void PHTEntry::update_pattern(std::vector<bool>& pattern, PatternType type) {
    for(uint32_t i = 0; i < N_BLK; ++i) {
        uint8_t& p = (type == PatternType::tAccess) ? access_pattern[i] :
                                        (type == PatternType::tRead) ? read_pattern[i] : write_pattern[i];

        if (SAT_CNT) {
            p = pattern[i] ? std::min<uint8_t>(p + 1, 3) : std::max<uint8_t>(p - 1, 0);
        } else {
            p = (pattern[i]) ? 2 : 1;
        }
    }
}

void PHTEntry::update(AccTableEntry& entry) {
    if (SEP_RDWR) {
        std::vector<bool> read_pattern = entry.read_pattern;
        std::vector<bool> write_pattern(N_BLK, false);
        for(uint32_t i = 0; i < N_BLK; ++i) {
            write_pattern[i] = entry.access_pattern[i] && !entry.read_pattern[i];
        }
        if (ROT) {
            std::rotate(read_pattern.begin(), read_pattern.begin() + entry.offset, read_pattern.end());
            std::rotate(write_pattern.begin(), write_pattern.begin() + entry.offset, write_pattern.end());
        }
        update_pattern(read_pattern, PatternType::tRead);
        update_pattern(write_pattern, PatternType::tWrite);
    } else {
        std::vector<bool> access_pattern = entry.access_pattern;
        if (ROT) {
            std::rotate(access_pattern.begin(), access_pattern.begin() + entry.offset, access_pattern.end());
        }
        update_pattern(access_pattern, PatternType::tAccess);
    }
    ts = entry.ts;
}

std::vector<bool> PHTEntry::get_bitvec(bool is_read) {
    std::vector<bool> bitvec(N_BLK, false);
    if (SEP_RDWR) {
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

boost::optional<std::vector<bool>> PHTSet::lookup(uint64_t tag, bool is_read, uint64_t ts) {
    for (uint32_t i = 0; i < PHT_WAYS; ++i) {
        if (entries[i].valid && entries[i].tag == tag) {
            entries[i].ts = ts; // Update timestamp
            return entries[i].get_bitvec(is_read);
        }
    }
    return boost::none; // No match found
}

void PHTSet::insert(uint64_t tag, AccTableEntry& entry) {
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
    if (empty) {
        entries[empty_idx].set(tag, entry);
    } else {
        if (PERFECT) {
            auto new_entry = PHTEntry(N_BLK, ROT, SEP_RDWR, SAT_CNT);
            new_entry.set(tag, entry);
            entries.push_back(new_entry);
        } else {
            entries[lru_idx].set(tag, entry); // Replace the LRU entry
        }
    }
}

boost::optional<std::vector<uint64_t>> PHT::lookup(uint64_t pc, uint64_t addr, bool is_read, uint64_t ts) {
    uint64_t base, offset;
    std::tie(base, offset) = get_base_offset(addr, N_BLK);
    uint64_t key = build_key(pc, offset, N_BLK, PHT_SETS, ROT);
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
                addrs.push_back(get_address(base, i, N_BLK));
            }
        }
        return addrs;
    } else {
        return boost::none; // No match found
    }
}

void PHT::insert(AccTableEntry& entry) {
    uint64_t key = build_key(entry.pc, entry.offset, N_BLK, PHT_SETS, ROT);
    uint32_t set_idx = key % PHT_SETS;
    uint64_t tag = key >> __builtin_ctzll(PHT_SETS);
    sets[set_idx].insert(tag, entry);
}