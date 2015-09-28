#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/Utility.hpp>
#include <components/FastCMPDirectory/BreakdownStats.hpp>
#include <ext/hash_map>

#include <core/stats.hpp>

#include <tuple>

#include <list>

namespace nFastCMPDirectory {

class InfDirRegStatsEntry : public AbstractDirectoryEntry {
  InfDirRegStatsEntry(PhysicalMemoryAddress addr, int32_t blocksPerRegion)
    : region(addr), sharers(blocksPerRegion, SharingVector()), state(blocksPerRegion, ZeroSharers), is_rw_region(false) {}

  InfDirRegStatsEntry(const InfDirRegStatsEntry & entry)
    : region(entry.region), sharers(entry.sharers), state(entry.state), is_rw_region(false) {}

private:
  PhysicalMemoryAddress region;
  std::vector<SharingVector> sharers;
  std::vector<SharingState> state;
  bool is_rw_region;

  friend class InfDirRegStats;

  inline void addSharer(int32_t index, int32_t offset) {
    sharers[offset].addSharer(index);
    if (sharers[offset].countSharers() == 1) {
      state[offset] = OneSharer;
    } else {
      state[offset] = ManySharers;
    }
  }

  inline void makeExclusive(int32_t index, int32_t offset) {
    DBG_Assert(sharers[offset].isSharer(index), ( << "Core " << index << " is not a sharer " << sharers[offset].getSharers() ) );
    DBG_Assert(sharers[offset].countSharers() == 1);
    state[offset] = ExclSharer;
  }

  inline void removeSharer(int32_t index, int32_t offset) {
    sharers[offset].removeSharer(index);
    if (sharers[offset].countSharers() == 0) {
      state[offset] = ZeroSharers;
    } else if (sharers[offset].countSharers() == 1) {
      state[offset] = OneSharer;
    }
  }

};

typedef boost::intrusive_ptr<InfDirRegStatsEntry> InfDirRegStatsEntry_p;

class InfDirRegStats : public AbstractDirectory {
private:
  struct AddrHash {
    std::size_t operator()(const PhysicalMemoryAddress & addr) const {
      return addr >> 10;
    }
  };
  typedef __gnu_cxx::hash_map<PhysicalMemoryAddress, InfDirRegStatsEntry_p, AddrHash> inf_directory_t;
  inf_directory_t theDirectory;
  inf_directory_t * theDirectoryCache;

  InfDirRegStats() : AbstractDirectory() {};

  int32_t theBlocksPerRegion;
  int32_t theBlockShift;
  int32_t theRegionShift;
  uint64_t theRegionMask;
  uint64_t theBlockOffsetMask;

  uint64_t theRegionSize;
  uint64_t theBlockSize;

  BreakdownStats::SharingDimension s;
  BreakdownStats::HitDimension bh;
  BreakdownStats::HitDimension rh;

  BreakdownStats * theBreakdownStats;

  Flexus::Stat::StatCounter * theUncached_stat;
  Flexus::Stat::StatCounter * theCachedNewSharers_stat;
  Flexus::Stat::StatCounter * theCachedFewerSharers_stat;
  Flexus::Stat::StatCounter * theCachedRegionMatch_stat;
  Flexus::Stat::StatCounter * theCachedBlockUnchanged_stat;
  Flexus::Stat::StatCounter * theCachedBlockChanged_stat;
  Flexus::Stat::StatCounter * theCachedBlockFewerSharers_stat;
  Flexus::Stat::StatCounter * theCachedBlockNewSharers_stat;
  Flexus::Stat::StatCounter * theUncached_RO;
  Flexus::Stat::StatCounter * theCachedNewSharers_RO;
  Flexus::Stat::StatCounter * theCachedFewerSharers_RO;
  Flexus::Stat::StatCounter * theCachedRegionMatch_RO;
  Flexus::Stat::StatCounter * theCachedBlockUnchanged_RO;
  Flexus::Stat::StatCounter * theCachedBlockChanged_RO;
  Flexus::Stat::StatCounter * theCachedBlockFewerSharers_RO;
  Flexus::Stat::StatCounter * theCachedBlockNewSharers_RO;

  inline int32_t getBlockOffset(PhysicalMemoryAddress address) {
    return (int)((address >> theBlockShift) & theBlockOffsetMask);
  }

  inline PhysicalMemoryAddress getRegion(PhysicalMemoryAddress address) {
    return PhysicalMemoryAddress(address & theRegionMask);
  }

  inline void setRegionSize(const std::string & arg) {
    theRegionSize = std::strtoul(arg.c_str(), nullptr, 0);
  }

  inline void setBlockSize(const std::string & arg) {
    theBlockSize = std::strtoul(arg.c_str(), nullptr, 0);
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfDirRegStatsEntry * my_entry = dynamic_cast<InfDirRegStatsEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index, getBlockOffset(address));
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfDirRegStatsEntry * my_entry = dynamic_cast<InfDirRegStatsEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index, getBlockOffset(address));
    my_entry->makeExclusive(index, getBlockOffset(address));
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // We track precise sharers, so a failed snoop means we remove a sharer
    removeSharer(index, dir_entry, address);
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfDirRegStatsEntry * my_entry = dynamic_cast<InfDirRegStatsEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      return;
    }
    my_entry->removeSharer(index, getBlockOffset(address));
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfDirRegStatsEntry * my_entry = dynamic_cast<InfDirRegStatsEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index, getBlockOffset(address));
  }

  InfDirRegStatsEntry * findOrCreateEntry(PhysicalMemoryAddress addr) {
    inf_directory_t::iterator iter;
    bool success;
    std::tie(iter, success) = theDirectory.insert( std::make_pair<PhysicalMemoryAddress, InfDirRegStatsEntry_p>(getRegion(addr), InfDirRegStatsEntry_p()) );
    if (success) {
      iter->second = new InfDirRegStatsEntry(getRegion(addr), theBlocksPerRegion);
    }
    return iter->second.get();
  }

  InfDirRegStatsEntry_p findEntry(PhysicalMemoryAddress addr) {
    inf_directory_t::iterator iter = theDirectory.find(getRegion(addr));
    if (iter == theDirectory.end()) {
      return nullptr;
    }
    return iter->second;
  }

  void compareDirectoryEntries(InfDirRegStatsEntry_p cur_entry, InfDirRegStatsEntry_p cached_entry, BreakdownStats::HitDimension & region_hit) {
    bool exact_match = true;
    for (int32_t offset = 0; offset < theBlocksPerRegion; offset++) {
      if (cur_entry->sharers[offset] != cached_entry->sharers[offset]) {
        // Check if any new sharers have been added;
        if ((cur_entry->sharers[offset] & cached_entry->sharers[offset]) != cur_entry->sharers[offset]) {
          region_hit = BreakdownStats::HitMore;
          return;
        }
        exact_match = false;
      }
    }
    if (exact_match) {
      region_hit = BreakdownStats::HitMatch;
    } else {
      region_hit = BreakdownStats::HitFewer;
    }
  }

public:
  virtual std::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) {

    s = BreakdownStats::NonShared;
    bh = BreakdownStats::Miss;
    rh = BreakdownStats::Miss;

    InfDirRegStatsEntry_p entry = findEntry(address);
    SharingVector sharers;
    SharingState  state = ZeroSharers;
    bool is_shared = false;
    if (entry != nullptr) {
      int32_t offset = getBlockOffset(address);
      sharers = entry->sharers[offset];
      state = entry->state[offset];

      bool read_only = !entry->is_rw_region;
      for (int32_t block = 0; block < theBlocksPerRegion; block++) {
        if (entry->state[offset] == ExclSharer) {
          read_only = false;
          break;
        }
      }
      SharingVector test;
      test.addSharer(index);
      test.flip();
      for (int32_t block = 0; block < theBlocksPerRegion; block++) {
        if ((entry->sharers[block] & test).any()) {
          is_shared = true;
        }
      }
      if (is_shared) {
        if (read_only) {
          s = BreakdownStats::SharedRO;
        } else {
          s = BreakdownStats::SharedRW;
        }
      } else {
        s = BreakdownStats::NonShared;
      }

      // Check if this state matches what we have in the directory cache
      inf_directory_t::iterator iter = theDirectoryCache[index].find(getRegion(address));
      if (iter != theDirectoryCache[index].end()) {
        if ((sharers == iter->second->sharers[offset]) && (state == iter->second->state[offset])) {
          bh = BreakdownStats::HitMatch;
        } else {
          (*theCachedBlockChanged_stat)++;
          if ((sharers & iter->second->sharers[offset]) != sharers) {
            bh = BreakdownStats::HitMore;
          } else {
            bh = BreakdownStats::HitFewer;
          }
        }

        // compare entire region
        compareDirectoryEntries(entry, iter->second, rh);
      }
    }

    // We have everything, now we just need to determine WHERE the directory is, and add the appropriate messages
    int32_t dir_loc = address2DirLocation(address, index);
    msgs.push_back(TopologyMessage(index, dir_loc));

    return std::make_tuple(sharers, state, dir_loc, entry);
  }

  virtual void processRequestResponse(int32_t index, const MMType & request, MMType & response,
                                      AbstractEntry_p dir_entry, PhysicalMemoryAddress address, bool off_chip) {
    // First, do default behaviour
    AbstractDirectory::processRequestResponse(index, request, response, dir_entry, address, off_chip);

    InfDirRegStatsEntry_p my_entry = dynamic_cast<InfDirRegStatsEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      my_entry = findEntry(address);
      if (my_entry == nullptr) {
        return;
      }
    }

    if (request == MemoryMessage::WriteReq || request == MemoryMessage::UpgradeReq) {
      my_entry->is_rw_region = true;
    }

    theBreakdownStats->incrementStat(request, off_chip, s, bh, rh);

    // Now, store a copy of this entry in a per-core cache
    inf_directory_t::iterator iter;
    bool success;
    std::tie(iter, success) = theDirectoryCache[index].insert( std::make_pair<PhysicalMemoryAddress, InfDirRegStatsEntry_p>(getRegion(address), InfDirRegStatsEntry_p() ) );
#if 0
    if (!success) {
      // Delete the old entry
      delete iter->second;
    }
#endif
    iter->second = new InfDirRegStatsEntry(*my_entry);

  }

  virtual void initialize(const std::string & aName) {

    DBG_Assert( theRegionSize > theBlockSize);
    DBG_Assert( ((theRegionSize - 1) & theRegionSize) == 0 );
    DBG_Assert( ((theBlockSize - 1) & theBlockSize) == 0 );

    theRegionShift  = log_base2(theRegionSize);
    theRegionMask  = ~(theRegionSize - 1);
    theBlockShift  = log_base2(theBlockSize);
    theBlockOffsetMask = (theRegionSize - 1) >> theBlockShift;
    theBlocksPerRegion = theRegionSize / theBlockSize;

    AbstractDirectory::initialize(aName);

    // Initialize stats
    theUncached_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Miss");
    theCachedNewSharers_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Region:NewSharers");
    theCachedFewerSharers_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Region:FewerSharers");
    theCachedRegionMatch_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Region:Match");
    theCachedBlockUnchanged_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Block:Unchanged");
    theCachedBlockChanged_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Block:Changed");
    theCachedBlockFewerSharers_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Block:Changed:FewerSharers");
    theCachedBlockNewSharers_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Block:Changed:NewSharers");

    theUncached_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Miss");
    theCachedNewSharers_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Region:NewSharers");
    theCachedFewerSharers_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Region:FewerSharers");
    theCachedRegionMatch_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Region:Match");
    theCachedBlockUnchanged_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Block:Unchanged");
    theCachedBlockChanged_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Block:Changed");
    theCachedBlockFewerSharers_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Block:Changed:FewerSharers");
    theCachedBlockNewSharers_RO = new Flexus::Stat::StatCounter(aName + "-SharedRO:Cache:Hit:Block:Changed:NewSharers");

    theDirectoryCache = new inf_directory_t[theNumCores];

    theBreakdownStats = new BreakdownStats(aName);
  }

  static AbstractDirectory * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    InfDirRegStats * directory = new InfDirRegStats();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (iter->first == "RegionSize") {
        directory->setRegionSize(iter->second);
      } else if (iter->first == "BlockSize") {
        directory->setBlockSize(iter->second);
      }
    }

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(InfDirRegStats, "InfDirRegStats");

}; // namespace
