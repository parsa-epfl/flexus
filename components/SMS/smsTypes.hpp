#include "boost/optional.hpp"
#include "core/checkpoint/json.hpp"
using json = nlohmann::json;

struct AccTableEntry
{
    uint64_t tag;
    uint64_t pc;
    uint64_t offset;
    std::vector<bool> access_pattern;
    std::vector<bool> read_pattern;
    uint64_t ts;
    bool valid;
    uint32_t N_BLK;

    AccTableEntry(uint32_t n_blk)
      : tag(0), pc(0), offset(0), access_pattern(n_blk, false), read_pattern(n_blk, false), ts(0), valid(false), N_BLK(n_blk) {}
    AccTableEntry& operator=(const AccTableEntry& other);
    void reset();
};

struct AccTable
{
    std::vector<AccTableEntry> entries;
    uint32_t N_ACC;
    uint32_t N_BLK;

    AccTable(uint32_t n_acc, uint32_t n_blk)
      : entries(n_acc, AccTableEntry(n_blk)), N_ACC(n_acc), N_BLK(n_blk) {}

    boost::optional<uint32_t> poke(uint64_t addr);
    boost::optional<AccTableEntry> insert(AccTableEntry& entry);
    bool poke_and_update(uint64_t addr, bool is_store, uint64_t ts);
    boost::optional<AccTableEntry> evict(uint64_t addr);
};

struct FilterTableEntry
{
    uint64_t tag;
    uint64_t pc;
    uint64_t offset;
    bool is_read;
    bool valid;
    uint64_t ts;

    FilterTableEntry()
      : tag(0), pc(0), offset(0), is_read(false), valid(false), ts(0) {}

    FilterTableEntry& operator=(const FilterTableEntry& other);
    void reset();
};

struct FilterTable
{
    std::vector<FilterTableEntry> entries;
    uint32_t N_FILTER;
    uint32_t N_BLK;

    FilterTable(uint32_t n_filter, uint32_t n_blk)
      : entries(n_filter, FilterTableEntry()), N_FILTER(n_filter), N_BLK(n_blk) {}

    boost::optional<uint32_t> poke(uint64_t addr);
    void insert(FilterTableEntry& entry);
    boost::optional<AccTableEntry> poke_and_update(uint64_t addr, uint64_t pc, bool is_store, uint64_t ts);
    void evict(uint64_t addr);
};

struct AGT
{
    AccTable acc_table;
    FilterTable filter_table;

    AGT()
      : acc_table(0, 0), filter_table(0, 0) {}

    AGT(uint32_t n_acc, uint32_t n_filter, uint32_t n_blk)
      : acc_table(n_acc, n_blk), filter_table(n_filter, n_blk) {}

    boost::optional<AccTableEntry> record(uint64_t addr, uint64_t pc, bool is_store, uint64_t ts);
    boost::optional<AccTableEntry> evict(uint64_t addr);
};

enum PatternType {
    tAccess,
    tRead,
    tWrite
};

struct PHTEntry
{
    uint64_t tag;
    std::vector<uint8_t> access_pattern;
    std::vector<uint8_t> read_pattern;
    std::vector<uint8_t> write_pattern;
    uint64_t ts;
    bool valid;
    uint32_t N_BLK;
    bool ROT; // Rotate access pattern
    bool SEP_RDWR; // Separate read/write patterns
    bool SAT_CNT; // Saturating counter for access patterns

    PHTEntry(uint32_t n_blk, bool rot, bool sep_rdwr, bool sat_cnt)
      : tag(0), access_pattern(n_blk, 1), read_pattern(n_blk, 1), write_pattern(n_blk, 1), ts(0), valid(false), N_BLK(n_blk), ROT(rot), SEP_RDWR(sep_rdwr), SAT_CNT(sat_cnt) {}

    void set(uint64_t aTag, AccTableEntry& entry);
    void update_pattern(std::vector<bool>& pattern, PatternType type);
    void update(AccTableEntry& entry);
    std::vector<bool> get_bitvec(bool is_read);
};

struct PHTSet
{
    std::vector<PHTEntry> entries;
    uint32_t PHT_WAYS;
    uint32_t N_BLK;
    bool ROT;
    bool SEP_RDWR;
    bool SAT_CNT;
    bool PERFECT;

    PHTSet(uint32_t pht_ways, uint32_t n_blk, bool rot, bool sep_rdwr, bool sat_cnt, bool perfect)
      : entries(pht_ways, PHTEntry(n_blk, rot, sep_rdwr, sat_cnt)), PHT_WAYS(pht_ways), N_BLK(n_blk), ROT(rot), SEP_RDWR(sep_rdwr), SAT_CNT(sat_cnt), PERFECT(perfect) {}

    boost::optional<std::vector<bool>> lookup(uint64_t tag, bool is_read, uint64_t ts);
    void insert(uint64_t tag, AccTableEntry& entry);
};

struct PHT
{
    uint32_t theIndex;
    std::vector<PHTSet> sets;
    uint32_t PHT_SETS;
    uint32_t N_BLK;
    bool ROT;

    PHT()
      : sets(), PHT_SETS(0), N_BLK(0), ROT(false) {}

    PHT(uint32_t theIndex, uint32_t pht_sets, uint32_t pht_ways, uint32_t n_blk, bool rot, bool sep_rdwr, bool sat_cnt, bool perfect)
      : theIndex(theIndex), sets(pht_sets, PHTSet(pht_ways, n_blk, rot, sep_rdwr, sat_cnt, perfect)), PHT_SETS(pht_sets), N_BLK(n_blk), ROT(rot) {}

    boost::optional<std::vector<uint64_t>> lookup(uint64_t pc, uint64_t addr, bool is_read, uint64_t ts);
    void insert(AccTableEntry& entry);
    uint64_t loadState(std::string const& aDirName);  // Returns the latest time
    void saveState(std::string const& aDirName);
};