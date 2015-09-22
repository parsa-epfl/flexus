#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/Utility.hpp>
#include <ext/hash_map>

#include <core/stats.hpp>

#include <tuple>
#include <list>

namespace nFastCMPDirectory {

struct CachedEntry {
  CachedEntry(int32_t blocksPerRegion) : sharers(blocksPerRegion, SharingVector()), state(blocksPerRegion, ZeroSharers) {}
  CachedEntry(std::vector<SharingVector> &v, std::vector<SharingState> &s) : sharers(v), state(s) {}
  std::vector<SharingVector> sharers;
  std::vector<SharingState> state;
};

class BetterCachingDirEntry : public AbstractDirectoryEntry {
  BetterCachingDirEntry(PhysicalMemoryAddress addr, int32_t blocksPerRegion, int32_t numCores)
    : region(addr), sharers(blocksPerRegion, SharingVector()), state(blocksPerRegion, ZeroSharers),
      cached_entries(numCores), is_rw_region(false), is_non_shared(true) {}

#if 0
  // We have a vector of pointers, if we need this function, we need to handle cache_entries properly
  BetterCachingDirEntry(const BetterCachingDirEntry & entry)
    : region(entry.region), sharers(entry.sharers), state(entry.state), cache_entries(entry.cache_entries), is_rw_region(entry.is_rw_entry) {}
#endif

public:
  ~BetterCachingDirEntry() {
    for (uint32_t i = 0; i < cached_entries.size(); i++) {
      if (cached_entries[i] != NULL) {
        delete cached_entries[i];
        cached_entries[i] = NULL;
      }
    }
  }

private:
  PhysicalMemoryAddress region;
  std::vector<SharingVector> sharers;
  std::vector<SharingState> state;

  // Each node can have a different cached copy
  // Store cached copies here to simplify things
  // We only cache regions that have no R/W blocks,
  // So no need to track state explicitly in the cached entries

  std::vector<CachedEntry *> cached_entries;

  bool is_rw_region;
  bool is_non_shared;

  friend class BetterCachingDir;

  inline void addSharer(int32_t index, int32_t offset) {
    sharers[offset].addSharer(index);
    if (sharers[offset].countSharers() == 1) {
      state[offset] = OneSharer;
    } else {
      state[offset] = ManySharers;
    }
  }

  inline void addCachedSharer(int32_t index, int32_t offset, int32_t cache) {
    DBG_Assert(cached_entries[cache] != NULL);
    cached_entries[cache]->sharers[offset].addSharer(index);
    if (cached_entries[cache]->sharers[offset].countSharers() == 1) {
      cached_entries[cache]->state[offset] = OneSharer;
    } else {
      cached_entries[cache]->state[offset] = ManySharers;
    }
  }

  inline void makeExclusive(int32_t index, int32_t offset) {
    DBG_Assert(sharers[offset].isSharer(index), ( << "Region " << region << " Block " << offset << " Core " << index << " is not a sharer " << sharers[offset].getSharers() ) );
    DBG_Assert(sharers[offset].countSharers() == 1, ( << "Region " << region << " Block " << offset << " has " << sharers[offset].countSharers() << " sharers (" << sharers[offset] << ") can't make " << index << " exclusive" ));
    state[offset] = ExclSharer;
  }

  inline void makeCachedExclusive(int32_t index, int32_t offset, int32_t cache) {
    DBG_Assert(cached_entries[cache] != NULL);
    DBG_Assert(cached_entries[cache]->sharers[offset].isSharer(index), ( << "Region " << region << " Block " << offset << " Core " << index << " is not a sharer " << cached_entries[cache]->sharers[offset].getSharers() ) );
    DBG_Assert(cached_entries[cache]->sharers[offset].countSharers() == 1, ( << "Region " << region << " Block " << offset << " cached_entry[" << cache << "] has " << cached_entries[cache]->sharers[offset].countSharers() << " sharers at offset " << offset ));
    cached_entries[cache]->state[offset] = ExclSharer;
  }

  inline void removeSharer(int32_t index, int32_t offset) {
    sharers[offset].removeSharer(index);
    if (sharers[offset].countSharers() == 0) {
      state[offset] = ZeroSharers;
    } else if (sharers[offset].countSharers() == 1) {
      state[offset] = OneSharer;
    }
  }

  inline void removeCachedSharer(int32_t index, int32_t offset, int32_t cache) {
    DBG_Assert(cached_entries[cache] != NULL);
    cached_entries[cache]->sharers[offset].removeSharer(index);
    if (cached_entries[cache]->sharers[offset].countSharers() == 0) {
      cached_entries[cache]->state[offset] = ZeroSharers;
    } else if (cached_entries[cache]->sharers[offset].countSharers() == 1) {
      cached_entries[cache]->state[offset] = OneSharer;
    }
  }

  inline void cacheEntry(int32_t index) {
    if (cached_entries[index] == NULL) {
      cached_entries[index] = new CachedEntry(sharers, state);
    } else {
      cached_entries[index]->sharers = sharers;
      cached_entries[index]->state = state;
    }
  }

  inline void updateCachedEntry(int32_t cache, int32_t offset) {
    DBG_Assert(cached_entries[cache] != NULL);
    cached_entries[cache]->sharers[offset] = sharers[offset];
    cached_entries[cache]->state[offset] = state[offset];
  }

};

typedef boost::intrusive_ptr<BetterCachingDirEntry> BetterCachingEntry_p;

class BetterCachingDir : public AbstractDirectory {
private:
  struct AddrHash {
    std::size_t operator()(const PhysicalMemoryAddress & addr) const {
      return addr >> 10;
    }
  };
  typedef __gnu_cxx::hash_map<PhysicalMemoryAddress, BetterCachingEntry_p, AddrHash> inf_directory_t;
  inf_directory_t theDirectory;

  BetterCachingDir() : AbstractDirectory(), theRegionSize(1024), theBlockSize(64), theBlockScoutSnoopResponse(false) {};

  int32_t theBlocksPerRegion;
  int32_t theBlockShift;
  int32_t theRegionShift;
  uint64_t theRegionMask;
  uint64_t theBlockOffsetMask;

  uint64_t theRegionSize;
  uint64_t theBlockSize;

  bool theBlockScoutSnoopResponse;

  SharingVector current_cached_sharers;
  bool requires_directory;
  bool two_part_snoop_procedure;
  int32_t local_node;
  int32_t last_local_snoop;
  int32_t final_dir_loc;

  Flexus::Stat::StatCounter * theUncachedDirEntry_stat;
  Flexus::Stat::StatCounter * theFoundCachedDirEntry_stat;
  Flexus::Stat::StatCounter * theCacheDirEntry_stat;
  Flexus::Stat::StatCounter * theDontCacheDirEntry_stat;
  Flexus::Stat::StatCounter * theDirEntryAlreadyCached_stat;

  Flexus::Stat::StatCounter * theNonSharedCachedEntry_stat;
  Flexus::Stat::StatCounter * theSharedCachedEntry_stat;
  Flexus::Stat::StatCounter * theUselessCachedEntry_stat;
  Flexus::Stat::StatCounter * theTwoPartProcedure_stat;
  Flexus::Stat::StatCounter * theRecallCachedEntries_stat;

  inline int32_t getBlockOffset(PhysicalMemoryAddress address) {
    return (int)((address >> theBlockShift) & theBlockOffsetMask);
  }

  inline PhysicalMemoryAddress getRegion(PhysicalMemoryAddress address) {
    return PhysicalMemoryAddress(address & theRegionMask);
  }

  inline void setRegionSize(const std::string & arg) {
    theRegionSize = std::strtoul(arg.c_str(), NULL, 0);
  }

  inline void setBlockSize(const std::string & arg) {
    theBlockSize = std::strtoul(arg.c_str(), NULL, 0);
  }

  inline void setBlockScoutSnoops(const std::string & arg) {
    if (arg == "true" || arg == "True" || arg == "TRUE" || arg == "1") {
      theBlockScoutSnoopResponse = true;
    } else {
      theBlockScoutSnoopResponse = false;
    }
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    BetterCachingEntry_p my_entry(dynamic_cast<BetterCachingDirEntry *>(dir_entry.get()));
    if (my_entry == NULL) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index, getBlockOffset(address));
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    BetterCachingEntry_p my_entry(dynamic_cast<BetterCachingDirEntry *>(dir_entry.get()));
    if (my_entry == NULL) {
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
    BetterCachingEntry_p my_entry(dynamic_cast<BetterCachingDirEntry *>(dir_entry.get()));
    if (my_entry == NULL) {
      return;
    }
    my_entry->removeSharer(index, getBlockOffset(address));
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    BetterCachingEntry_p my_entry(dynamic_cast<BetterCachingDirEntry *>(dir_entry.get()));
    if (my_entry == NULL) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index, getBlockOffset(address));
  }

  BetterCachingEntry_p findOrCreateEntry(PhysicalMemoryAddress addr) {
    inf_directory_t::iterator iter;
    bool success;
    boost::tie(iter, success) = theDirectory.insert( std::make_pair<PhysicalMemoryAddress, BetterCachingDirEntry *>(getRegion(addr), NULL) );
    if (success) {
      iter->second = BetterCachingEntry_p(new BetterCachingDirEntry(getRegion(addr), theBlocksPerRegion, theNumCores));
    }
    return iter->second;
  }

  BetterCachingEntry_p findEntry(PhysicalMemoryAddress addr) {
    inf_directory_t::iterator iter = theDirectory.find(getRegion(addr));
    if (iter == theDirectory.end()) {
      return NULL;
    }
    return iter->second;
  }

#if 0
  void compareDirectoryEntries(BetterCachingDirEntry * cur_entry, BetterCachingDirEntry * cached_entry) {
    bool exact_match = true;
    for (int32_t offset = 0; offset < theBlocksPerRegion; offset++) {
      if (cur_entry->sharers[offset] != cached_entry->sharers[offset]) {
        // Check if any new sharers have been added;
        if ((cur_entry->sharers[offset] & cached_entry->sharers[offset]) != cur_entry->sharers[offset]) {
          (*theCachedNewSharers_stat)++;
          return;
        }
        exact_match = false;
      }
    }
    if (exact_match) {
      (*theCachedRegionMatch_stat)++;
    } else {
      (*theCachedFewerSharers_stat)++;
    }
  }
#endif

  bool recallCachedEntries(BetterCachingEntry_p entry, int32_t index, int32_t dir_loc, std::list<TopologyMessage> &msgs) {
    std::list<int> cache_list;
    // Need to recall sharers
    for (int32_t i = 0; i < theNumCores; i++) {
      if (entry->cached_entries[i] != NULL) {
        // Collect information from this sharer
        for (int32_t block = 0; block < theBlocksPerRegion; block++) {
          if ( entry->cached_entries[i]->sharers[block].isSharer(i) ) {
            entry->sharers[block].addSharer(i);
          } else if (!entry->cached_entries[i]->sharers[block].isSharer(i)) {
            entry->sharers[block].removeSharer(i);
          }
        }
        delete entry->cached_entries[i];
        entry->cached_entries[i] = NULL;

        if (i != index) {
          cache_list.push_back(i);
        }
      }
    }
    // Update the state of each block
    for (int32_t block = 0; block < theBlocksPerRegion; block++) {
      int32_t count = entry->sharers[block].countSharers();
      if (count == 0) {
        entry->state[block] = ZeroSharers;
      } else if (count == 1) {
        entry->state[block] = ExclSharer; // Be conservative
      } else if (count > 1) {
        entry->state[block] = ManySharers;
      }
    }
    // Record a multicast message to perform the recall
    if (cache_list.size() > 0) {
      msgs.push_back(TopologyMessage(dir_loc, cache_list));
      return true;
    } else {
      return false;
    }
  }

  void processLocalSnoopResponse(int32_t index, const MMType & type, AbstractEntry_p dir_entry, PhysicalMemoryAddress address, int32_t node) {
    BetterCachingDirEntry * my_entry = dynamic_cast<BetterCachingDirEntry *>(dir_entry.get());
    int32_t offset = getBlockOffset(address);
    switch (type) {
      case MemoryMessage::Invalidate:
      case MemoryMessage::ReturnNAck:
        recordSnoopMiss();
        if (my_entry == NULL) {
          return;
        }
        my_entry->removeCachedSharer(index, offset, node);
        break;
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvUpdateAck:
        recordSnoopHit();
        if (my_entry == NULL) {
          return;
        }
        my_entry->removeCachedSharer(index, offset, node);
        break;
      case MemoryMessage::ReturnReply:
        recordSnoopHit();
        if (my_entry == NULL) {
          return;
        }
        // If the snooped node has a cached directory, we have to make sure it doesn't think it has an exclusive copy
        if (my_entry->cached_entries[index] != NULL && my_entry->cached_entries[index]->state[offset] == ExclSharer) {
          my_entry->cached_entries[index]->state[offset] = ManySharers;
        }
        break;
      case MemoryMessage::DowngradeAck:
      case MemoryMessage::DownUpdateAck:
        DBG_Assert(false);
        recordSnoopHit();
        if (my_entry == NULL) {
          return;
        }
        my_entry->removeCachedSharer(index, offset, node);
        break;
      default:
        DBG_Assert( false, ( << "Unknown Snoop Response '" << type << "'" ));
        break;
    }
    if (theBlockScoutSnoopResponse) {
      if (my_entry == NULL || my_entry->cached_entries[node] == NULL) {
        return;
      }
      // Update this entry with information about all the blocks present at the given node
      RegionScoutMessage probe(RegionScoutMessage::eBlockScoutProbe, PhysicalMemoryAddress(address));
      thePorts.sendRegionProbe(probe, index);
      uint32_t mask = 1;
      for (int32_t block = 0; block < theBlocksPerRegion; block++, mask <<= 1) {
        if ((probe.blocks() & mask) != 0) {
          my_entry->cached_entries[node]->sharers[block].addSharer(index);
          if (my_entry->cached_entries[node]->sharers[block].countSharers() > 0) {
            my_entry->cached_entries[node]->state[block] = ManySharers;
          }
        } else {
          my_entry->cached_entries[node]->sharers[block].removeSharer(index);
          if (my_entry->cached_entries[node]->sharers[block].countSharers() == 0) {
            my_entry->cached_entries[node]->state[block] = ZeroSharers;
          }
        }
      }
    }
  }

  void processDirectorySnoopResponse(int32_t index, const MMType & type, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    switch (type) {
      case MemoryMessage::Invalidate:
      case MemoryMessage::ReturnNAck:
        recordSnoopMiss();
        failedSnoop(index, dir_entry, address);
        break;
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvUpdateAck:
        recordSnoopHit();
        removeSharer(index, dir_entry, address);
        break;
      case MemoryMessage::ReturnReply: {
        recordSnoopHit();
        // This is really the only special case.
        // We need to make sure the snooped node doesn't think it has an Excl Copy
        BetterCachingDirEntry * my_entry = dynamic_cast<BetterCachingDirEntry *>(dir_entry.get());
        if (my_entry == NULL) {
          return;
        }
        int32_t offset = getBlockOffset(address);
        if (my_entry->cached_entries[index] != NULL && my_entry->cached_entries[index]->state[offset] == ExclSharer) {
          my_entry->cached_entries[index]->state[offset] = ManySharers;
        }
        break;
      }
      case MemoryMessage::DowngradeAck:
      case MemoryMessage::DownUpdateAck:
        recordSnoopHit();
// This doesn't look right to me. Downgrade transitions to shared, it doesn't remove a sharer
        DBG_Assert(false);
        removeSharer(index, dir_entry, address);
        break;
      default:
        DBG_Assert( false, ( << "Unknown Snoop Response '" << type << "'" ));
        break;
    }
  }

  void processLocalRequestResponse(int32_t index, const MMType & request, MMType & response, BetterCachingEntry_p my_entry, PhysicalMemoryAddress address) {
    switch (request) {
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictDirty:
        my_entry->removeCachedSharer(index, getBlockOffset(address), index);
        return;
      case MemoryMessage::UpgradeReq:
        DBG_Assert( response == MemoryMessage::UpgradeReply );
        my_entry->makeCachedExclusive(index, getBlockOffset(address), index);
        return;
      default:
        break;
    }
    switch (response) {
      case MemoryMessage::MissReply:
        my_entry->addCachedSharer(index, getBlockOffset(address), index);
        return;
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::MissReplyDirty:
        my_entry->addCachedSharer(index, getBlockOffset(address), index);
        my_entry->makeCachedExclusive(index, getBlockOffset(address), index);
        return;
      default:
        break;
    }
    DBG_Assert( false, ( << "Uknown Request+Response combination: " << request << "-" << response ));
  }

public:

  virtual int32_t processSnoopResponse(int32_t index, const MMType & type, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    AbstractDirectory::processSnoopResponse(index, type, dir_entry, address);

    // If these are snoops from a local node, then the directory should only update the cached copy
    // Otherwise, only update the directory
    if (current_cached_sharers.isSharer(index)) {
      processLocalSnoopResponse(index, type, dir_entry, address, local_node);
      if (two_part_snoop_procedure) {
        processDirectorySnoopResponse(index, type, dir_entry, address);
      }
    } else {
      // We need our own version to make sure that cached directories are updated at the nodes being snooped as well
      processDirectorySnoopResponse(index, type, dir_entry, address);
    }

    // now we have to decide if this snoop will be the last one from the local node before doing snoops from the directory
    if (index == last_local_snoop) {
      return final_dir_loc;
    }

    return -1;
  }

  // We need to handle the case where all the snoops from the local node fail and we move to the directory
  virtual std::list<int> orderSnoops(AbstractEntry_p dir_entry, SharingVector sharers, int32_t src, bool optimize3hop, int32_t final_dest, bool multicast) {
    if (two_part_snoop_procedure) {
      SharingVector additional_sharers = current_cached_sharers;
      additional_sharers.flip();
      additional_sharers &= sharers;

      std::list<int> local, from_dir;

      // First, order snoops from local node
      local = theTopology->orderSnoops(current_cached_sharers, src, optimize3hop, final_dest, multicast);

      // Record last entry in local list
      last_local_snoop = local.back();

      // Now add snoops from directory
      from_dir = theTopology->orderSnoops(additional_sharers, final_dir_loc, optimize3hop, final_dest, multicast);

      // Splice the two lists together
      local.splice(local.end(), from_dir);

      return local;

    } else {
      return theTopology->orderSnoops(sharers, src, optimize3hop, final_dest, multicast);
    }
  }

  virtual std::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) {

    // Reset state information
    requires_directory = true;
    two_part_snoop_procedure = false;
    current_cached_sharers.clear();
    local_node = index;

    int32_t dir_loc = address2DirLocation(address, index);

    BetterCachingEntry_p entry = findEntry(address);
    SharingVector sharers;
    SharingState  state = ZeroSharers;
    if (entry != NULL) {
      DBG_(Iface, Addr(address) ( << "Found directory entry" ));
      int32_t offset = getBlockOffset(address);
      sharers = entry->sharers[offset];
      state = entry->state[offset];

      // Check Cached copy
      if (entry->cached_entries[index] != NULL) {
        (*theFoundCachedDirEntry_stat)++;
        dir_loc = index;
        DBG_(Iface, Addr(address) ( << "Found cached entry (index = " << index << ", offset = " << offset ));
        {
          DBG_(Iface, Addr(address) ( << "    Cached Entry at " << entry->cached_entries[index] ));
          DBG_(Iface, Addr(address) ( << "    state " << SharingStatePrinter(entry->cached_entries[index]->state[offset]) << " sharers[" << offset << "] = " << entry->cached_entries[index]->sharers[offset] ));
        }
        sharers = entry->cached_entries[index]->sharers[offset];
        state = entry->cached_entries[index]->state[offset];

        /* Read/Fetch:
         *   - do we have a valid sharer?
         *   - if yes, then we're good to go
         *   - if not, we need to remember which sharers we do have
         *   - then update the directory
         *
         * Write/Upgrade:
         *  - Is this a Non-Shared Region?
         *      Yes: go directly to the memory controller, no need to access main directory, but mark as shared_rw
         *      No: go to directory, recall sharers, mark as shared_rw
         */
        if (req_type == MemoryMessage::ReadReq || req_type == MemoryMessage::FetchReq) {
          bool valid_sharer = false;
          RegionScoutMessage probe(RegionScoutMessage::eBlockProbe, PhysicalMemoryAddress(address));
          for (int32_t i = 0; i < theNumCores; i++) {
            // Skip sharers that we don't know about
            if (!sharers.isSharer(i)) continue;
            probe.setType(RegionScoutMessage::eBlockProbe);
            thePorts.sendRegionProbe(probe, i);
            if (probe.type() == RegionScoutMessage::eBlockHitReply) {
              DBG_(Iface, Addr(address) ( << "Core " << i << " is a valid sharer" ));
              valid_sharer = true;
              break;
            }
          }
          if (!valid_sharer) {
            DBG_(Iface, Addr(address) ( << "No valid sharers found" ));
            // Update the directory about which blocks we have
            if (entry->is_non_shared) {
              DBG_(Iface, Addr(address) ( << "Region is Non-Shared" ));
              entry->sharers = entry->cached_entries[index]->sharers;
              entry->state = entry->cached_entries[index]->state;
            } else {
              DBG_(Iface, Addr(address) ( << "Region is Shared, updating main directory entry" ));
              for (int32_t block = 0; block < theBlocksPerRegion; block++) {
                if ( entry->cached_entries[index]->sharers[block].isSharer(index) ) {
                  entry->sharers[block].addSharer(index);
                  if (entry->sharers[block].countSharers() == 1) {
                    entry->state[block] = ExclSharer;
                  } else {
                    entry->state[block] = ManySharers;
                  }
                } else if (!entry->cached_entries[index]->sharers[block].isSharer(index)) {
                  entry->sharers[block].removeSharer(index);
                  if (entry->sharers[block].countSharers() == 0) {
                    entry->state[block] = ZeroSharers;
                  }
                }
              }

            }

            sharers = entry->sharers[offset];
            state = entry->state[offset];

            final_dir_loc = address2DirLocation(address, index);

            if (!entry->is_non_shared) {
              bool found_potential_sharers = false;
              // We also need to snoop any nodes that have cached entries
              for (int32_t i = 0; i < theNumCores; i++) {
                if (i != index && entry->cached_entries[i] != NULL) {
                  sharers.addSharer(i);
                  found_potential_sharers = true;
                }
              }
              if (found_potential_sharers && state == ZeroSharers) {
                state = ManySharers;
                entry->state[offset] = ManySharers;
              }
              entry->sharers[offset] = sharers;
            }

            // If we don't know about any other sharers
            if (current_cached_sharers.countSharers() == 0 || (current_cached_sharers.countSharers() == 1 && current_cached_sharers.isSharer(index))) {
              if (entry->is_non_shared) {
                requires_directory = false;
                (*theNonSharedCachedEntry_stat)++;
              } else {
                dir_loc = final_dir_loc;
                (*theUselessCachedEntry_stat)++;
              }
            } else {
              DBG_(Iface, Addr(address) ( << "Remembering current cached sharers" ));
              current_cached_sharers = entry->cached_entries[index]->sharers[offset];
              // It's important that the current node isn't somehow listed as a sharer
              // This might cause problems
              current_cached_sharers.removeSharer(index);

              two_part_snoop_procedure = true;
              (*theTwoPartProcedure_stat)++;
            }
          } else {
            DBG_(Iface, Addr(address) ( << "Found at least one valid sharer returning:" ));
            DBG_(Iface, Addr(address) ( << "    state " << SharingStatePrinter(state) << " sharers: " << sharers ));
            requires_directory = false;
            current_cached_sharers = sharers;
            (*theSharedCachedEntry_stat)++;
          }
        } else if (req_type == MemoryMessage::WriteReq || req_type == MemoryMessage::UpgradeReq) {
          DBG_(Iface, Addr(address) ( << "Write/Upgrade request, marking region r/w" ));
          entry->is_rw_region = true;

          if (!entry->is_non_shared) {
            // We need to snoop any nodes that have cached entries
#if 0
            bool found_potential_sharers = false;
            for (int32_t i = 0; i < theNumCores; i++) {
              if (i != index && entry->cached_entries[i] != NULL) {
                sharers.addSharer(i);
                found_potential_sharers = true;
              }
            }
            if (found_potential_sharers && state != ManySharers) {
              state = ManySharers;
            }
#endif
            DBG_(Iface, Addr(address) ( << "Shared region, recalling sharers" ));
            dir_loc = address2DirLocation(address, index);
            recallCachedEntries(entry, index, dir_loc, msgs);
            (*theRecallCachedEntries_stat)++;

            // Need to update the state we return AFTER doing the recall
            sharers = entry->sharers[offset];
            state = entry->state[offset];
          } else {
            DBG_(Iface, Addr(address) ( << "Non-Shared region, no special action" ));
            requires_directory = false;
            (*theNonSharedCachedEntry_stat)++;
          }
        }
      } else {
        DBG_(Iface, Addr(address) ( << "No Cached Entry found." ));
        (*theUncachedDirEntry_stat)++;

        // If we're writing to a Rd-only region, need to collect cached entries
        if (!entry->is_rw_region && (req_type == MemoryMessage::WriteReq || req_type == MemoryMessage::UpgradeReq)) {
          DBG_(Iface, Addr(address) ( << "Writing/Upgrading in Read-only region" ));
          //Need to recall sharers
          entry->is_rw_region = true;
          dir_loc = address2DirLocation(address, index);
          if (recallCachedEntries(entry, index, dir_loc, msgs)) {
            (*theRecallCachedEntries_stat)++;
            // If we recalled entries, we also need to update our response
            sharers = entry->sharers[offset];
            state = entry->state[offset];
          }
        } else if (entry->is_non_shared) {
          DBG_(Iface, Addr(address) ( << "Accessing Non-Shared region without cached entry" ));
          // If this is a non-shared region, then we need should do a directory update,
          // but don't actually recall the cached entry
          for (int32_t core = 0; core < theNumCores; core++) {
            if (entry->cached_entries[core] != NULL) {
              DBG_(Iface, Addr(address) ( << "Getting Directory info from solo node " << core ));
              entry->state = entry->cached_entries[core]->state;
              entry->sharers = entry->cached_entries[core]->sharers;
              // Make sure we use the right state
              sharers = entry->sharers[offset];
              state = entry->state[offset];
              entry->is_non_shared = false;
              // Add two messages
              msgs.push_back(TopologyMessage(dir_loc, core));
              msgs.push_back(TopologyMessage(core, dir_loc));

              // Need to recall if this is a R/W region
              if (entry->is_rw_region) {
                delete entry->cached_entries[core];
                entry->cached_entries[core] = NULL;
                (*theRecallCachedEntries_stat)++;
              }
            }
          }
        } else if (!entry->is_rw_region) {
          // Read/Fetch to shared rd-only region
          // Any node with a cached entry is a potential sharer

          DBG_(Iface, Addr(address) ( << "Accessing Shared region without cached entry" ));

          // If the block is exclusive, we have the right state
          // Otherwise, consider all caching nodes as sharers
          if (state != ExclSharer) {
            DBG_(Iface, Addr(address) ( << "Adding caching nodes as sharers" ));
            for (int32_t i = 0; i < theNumCores; i++) {
              if (i != index && entry->cached_entries[i] != NULL) {
                sharers.addSharer(i);
              }
            }
            int32_t count = sharers.countSharers();
            if (count == 1) {
              state = OneSharer;
            } else if (count > 1) {
              state = ManySharers;
            }

            // Update the state in the directory
            entry->sharers[offset] = sharers;
            entry->state[offset] = state;
          }
        }
      }

      DBG_(Iface, Addr(address) ( << "After lookup, main dir says (" << SharingStatePrinter(entry->state[offset]) << "/" << entry->sharers[offset]
                                  << "), return (" << SharingStatePrinter(state) << "/" << sharers << ")" ));
    } else {
      (*theUncachedDirEntry_stat)++;
    }

    // Initial message from requestor to directory
    msgs.push_front(TopologyMessage(index, dir_loc));

    return boost::tie(sharers, state, dir_loc, entry);
  }

  virtual void processRequestResponse(int32_t index, const MMType & request, MMType & response,
                                      AbstractEntry_p dir_entry, PhysicalMemoryAddress address, bool off_chip ) {
    // If we accessed the directory, make it aware of the changes
    if (requires_directory) {
      DBG_(Iface, Addr(address) ( << "Request required main directory, updating with response" ));
      AbstractDirectory::processRequestResponse(index, request, response, dir_entry, address, off_chip);
    }

    BetterCachingEntry_p my_entry(dynamic_cast<BetterCachingDirEntry *>(dir_entry.get()));
    if (my_entry == NULL) {
      my_entry = findEntry(address);
      if (my_entry == NULL) {
        DBG_(Iface, Addr(address) ( << "No Directory entry after processing response" ));
        return;
      }
    }

    // Is the directory already cached?
    if (my_entry->cached_entries[index] != NULL) {
      int32_t offset = getBlockOffset(address);
      // we should make sure that the cached entry matches the directory entry for this block
      if (requires_directory) {
        DBG_(Iface, Addr(address) ( << "Found Cached Entry, updating based on main directory update" ));
        my_entry->updateCachedEntry(index, offset);
      } else {
        DBG_(Iface, Addr(address) ( << "Found Cached Entry, processing local changes" ));
        processLocalRequestResponse(index, request, response, my_entry, address);
      }
      (*theDirEntryAlreadyCached_stat)++;
      DBG_(Iface, Addr(address) ( << "After processing response, main dir says (" << SharingStatePrinter(my_entry->state[offset]) << "/" << my_entry->sharers[offset]
                                  << "), cached entry says (" << SharingStatePrinter(my_entry->cached_entries[index]->state[offset]) << my_entry->cached_entries[index]->sharers[offset]
                                  << ")" ));
      return;
    }

    // Sanity check, make sure we know that we had to access the main directory
    DBG_Assert(requires_directory);

    // Should we be caching this region at this node?
    // Scenarios:
    //   1. It's Non-Shared (ie. we're the only sharer)
    //  2. It's Shared Read-Only

    // Check for other sharers
    my_entry->is_non_shared = true;
    for (int32_t offset = 0; offset < theBlocksPerRegion; offset++) {
      int32_t count = my_entry->sharers[offset].countSharers();
      if (count > 1 || (count == 1 && !my_entry->sharers[offset].isSharer(index))) {
        my_entry->is_non_shared = false;
        break;
      }
    }
    if (my_entry->is_non_shared) {
      for (int32_t i = 0; i < theNumCores; i++) {
        if (my_entry->cached_entries[i] != NULL) {
          my_entry->is_non_shared = false;
        }
      }
    }

    if (my_entry->is_non_shared || !my_entry->is_rw_region) {
      int32_t offset = getBlockOffset(address);
      DBG_(Iface, Addr(address) ( << "Caching Directory Entry at node " << index ));
      my_entry->cacheEntry(index);
      (*theCacheDirEntry_stat)++;
      DBG_(Iface, Addr(address) ( << "After processing response, main dir says (" << SharingStatePrinter(my_entry->state[offset]) << "/" << my_entry->sharers[offset]
                                  << "), cached entry says (" << SharingStatePrinter(my_entry->cached_entries[index]->state[offset]) << my_entry->cached_entries[index]->sharers[offset]
                                  << ")" ));
    } else {
      (*theDontCacheDirEntry_stat)++;
      int32_t offset = getBlockOffset(address);
      DBG_(Iface, Addr(address) ( << "After processing response, main dir says (" << SharingStatePrinter(my_entry->state[offset]) << "/" << my_entry->sharers[offset]
                                  << "), Entry NOT cached" ));
    }
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
    theUncachedDirEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Miss:Uncached");
    theFoundCachedDirEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit");
    theCacheDirEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Allocate");
    theDontCacheDirEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Skip");
    theDirEntryAlreadyCached_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Retain");

    theNonSharedCachedEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:NonShared");
    theSharedCachedEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Shared");
    theUselessCachedEntry_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:Useless");
    theTwoPartProcedure_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Hit:TwoPart");
    theRecallCachedEntries_stat = new Flexus::Stat::StatCounter(aName + "-Cache:Recall");
  }

  static AbstractDirectory * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    BetterCachingDir * directory = new BetterCachingDir();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (iter->first == "RegionSize") {
        directory->setRegionSize(iter->second);
      } else if (iter->first == "BlockSize") {
        directory->setBlockSize(iter->second);
      } else if (iter->first == "BlockScoutSnoops") {
        directory->setBlockScoutSnoops(iter->second);
      }
    }

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(BetterCachingDir, "BetterCaching");

}; // namespace
