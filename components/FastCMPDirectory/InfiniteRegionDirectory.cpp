#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/Utility.hpp>
#include <unordered_map>

#include <boost/lambda/if.hpp>
#include <boost/lambda/casts.hpp>

#include <tuple>
#include <list>
#include <vector>

namespace nFastCMPDirectory {

using namespace boost::lambda;

class InfiniteRegionDirectoryEntry : public AbstractDirectoryEntry {
  InfiniteRegionDirectoryEntry(PhysicalMemoryAddress addr, int32_t blocksPerRegion)
    : address(addr), state(ZeroSharers), present(blocksPerRegion, false), shared(blocksPerRegion, false) {}

private:
  PhysicalMemoryAddress address;
  SharingVector  sharers;
  SharingState  state;
  std::vector<bool> present;
  std::vector<bool> shared;

  friend class InfiniteRegionDirectory;

  friend std::ostream & operator<<(std::ostream & os, const InfiniteRegionDirectoryEntry & entry);

  inline void addSharer(int32_t index, int32_t offset) {
    sharers.addSharer(index);
    if (sharers.countSharers() == 1) {
      state = OneSharer;
    } else {
      state = ManySharers;
    }
    if (present[offset]) {
      shared[offset] = true;
    } else {
      present[offset] = true;
    }
  }

  inline void makeExclusive(int32_t index, int32_t offset) {
    DBG_Assert(sharers.isSharer(index), ( << "Core " << index << " is not a sharer " << sharers.getSharers() ) );
    // We don't actually change the state because we don't know if it's exclusive or not
    shared[offset] = false;
  }

  inline void removeRegionSharer(int32_t index) {
    sharers.removeSharer(index);
    if (sharers.countSharers() == 0) {
      state = ZeroSharers;
      present.assign(present.size(), false);
      shared.assign(shared.size(), false);
    } else if (sharers.countSharers() == 1) {
      state = OneSharer;
      shared.assign(shared.size(), false);
    }
  }

  inline void removeBlockSharer(int32_t offset) {
    if (!shared[offset]) {
      present[offset] = false;
    }
  }

  inline void markBlockPresent(int32_t offset) {
    present[offset] = true;
  }

  inline bool isBlockPresent(int32_t offset) {
    return present[offset];
  }

  inline bool isBlockShared(int32_t offset) {
    return shared[offset];
  }

};

typedef boost::intrusive_ptr<InfiniteRegionDirectoryEntry> InfiniteRegionDirectoryEntry_p;

std::ostream & operator<<(std::ostream & os, const InfiniteRegionDirectoryEntry & entry) {
  os << "Region: 0x" << std::hex << (uint64_t)entry.address;
  os << ", State: " << entry.state;
  os << ", Sharers: " << entry.sharers;
  os << ", Present: ";
  for (uint32_t i = 0; i < entry.present.size(); i++) {
    os << (entry.present[i] ? 1 : 0);
  }
  os << ", Shared: ";
  for (uint32_t i = 0; i < entry.shared.size(); i++) {
    os << (entry.shared[i] ? 1 : 0);
  }
  return os;
}

class InfiniteRegionDirectory : public AbstractDirectory {
private:
  struct AddrHash {
    std::size_t operator()(const PhysicalMemoryAddress & addr) const {
      return (std::size_t)(addr >> 10);
    }
  };
  typedef std::unordered_map<PhysicalMemoryAddress, InfiniteRegionDirectoryEntry_p, AddrHash> inf_region_dir_t;
  inf_region_dir_t theDirectory;

  uint64_t theBlockSize;
  uint64_t theRegionSize;
  uint64_t theBlockOffsetMask;
  uint64_t theRegionMask;
  int32_t theRegionShift;
  int32_t theBlockShift;
  int32_t theBlocksPerRegion;

  bool theTrackSharedBlocks;

  InfiniteRegionDirectory() : AbstractDirectory() {};

  inline void setRegionSize(const std::string & arg) {
    theRegionSize = std::strtoul(arg.c_str(), nullptr, 0);
  }

  inline void setBlockSize(const std::string & arg) {
    theBlockSize = std::strtoul(arg.c_str(), nullptr, 0);
  }

  inline void setTrackShared(const std::string & arg) {
    if (arg == "true" || arg == "1" || arg == "True" || arg == "TRUE") {
      theTrackSharedBlocks = true;
    } else {
      theTrackSharedBlocks = false;
    }
  }

  inline PhysicalMemoryAddress getRegion(const PhysicalMemoryAddress & addr) {
    return PhysicalMemoryAddress(addr & theRegionMask);
  }

  inline int32_t getBlockOffset(const PhysicalMemoryAddress & addr) {
    return ((addr >> theBlockShift) & theBlockOffsetMask);
  }

  inline int32_t isRegionPresent(int32_t index, PhysicalMemoryAddress & addr) {
    RegionScoutMessage probe(RegionScoutMessage::eRegionProbe, PhysicalMemoryAddress(addr));
    thePorts.sendRegionProbe(probe, index);
    return (probe.type() == RegionScoutMessage::eRegionHitReply);
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteRegionDirectoryEntry * my_entry = dynamic_cast<InfiniteRegionDirectoryEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index, getBlockOffset(address));
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteRegionDirectoryEntry * my_entry = dynamic_cast<InfiniteRegionDirectoryEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index, getBlockOffset(address));
    my_entry->makeExclusive(index, getBlockOffset(address));
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteRegionDirectoryEntry * my_entry = dynamic_cast<InfiniteRegionDirectoryEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      return;
    }
    // If there's only 1 sharer then we can safely remove the sharer
    if (my_entry->state == OneSharer || my_entry->state == ExclSharer) {
      if (theTrackSharedBlocks) {
        my_entry->removeBlockSharer(getBlockOffset(address));
      }
    }
    if (!isRegionPresent(index, my_entry->address)) {
      my_entry->removeRegionSharer(index);
    }
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteRegionDirectoryEntry * my_entry = dynamic_cast<InfiniteRegionDirectoryEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      return;
    }
    // We only actually remove the sharer if they don't have ANY blocks from this region
    // To do this, we send a region probe.
    // (This reduces the amount of meta-data we need to store here.)
    // (In a real system, this information would be communicated in the actual request)
    // IMPORTANT: remove the block before removing the region
    //   if we remove a non-shared block, we mark it not present
    //   when we remove a region sharer, we might mark all blocks non-shared if there is only one remaining sharer
    //   thus, if we remove the region first, we might mark a block non present when we shouldn't
    if (theTrackSharedBlocks) {
      my_entry->removeBlockSharer(getBlockOffset(address));
    }
    if (!isRegionPresent(index, my_entry->address)) {
      my_entry->removeRegionSharer(index);
    }
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteRegionDirectoryEntry * my_entry = dynamic_cast<InfiniteRegionDirectoryEntry *>(dir_entry.get());
    if (my_entry == nullptr) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index, getBlockOffset(address));
  }

  InfiniteRegionDirectoryEntry * findOrCreateEntry(PhysicalMemoryAddress addr) {
    inf_region_dir_t::iterator iter;
    bool success;
    std::tie(iter, success) = theDirectory.insert( std::make_pair<PhysicalMemoryAddress, InfiniteRegionDirectoryEntry_p>(getRegion(addr), InfiniteRegionDirectoryEntry_p()) );
    if (success) {
      iter->second = new InfiniteRegionDirectoryEntry(getRegion(addr), theBlocksPerRegion);
    }
    return iter->second.get();
  }

  InfiniteRegionDirectoryEntry_p findEntry(PhysicalMemoryAddress addr) {
    inf_region_dir_t::iterator iter = theDirectory.find(getRegion(addr));
    if (iter == theDirectory.end()) {
      return nullptr;
    }
    return iter->second;
  }

public:
  virtual std::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<std::function<void(void)> > &xtra_actions) {

    InfiniteRegionDirectoryEntry_p entry = findEntry(address);
    SharingVector sharers;
    SharingState  state = ZeroSharers;
    if (entry != nullptr && entry->isBlockPresent(getBlockOffset(address))) {
      sharers = entry->sharers;
      state = entry->state;
      if (theTrackSharedBlocks && !entry->isBlockShared(getBlockOffset(address))) {
        state = OneSharer;
      }
    }
#if 0
    if (entry == nullptr) {
      DBG_(Dev, ( << "No Dir entry for region 0x" << std::hex << (uint64_t)getRegion(address) ));
    } else {
      DBG_(Dev, ( << "Dir entry: " << (*entry) ));
    }
#endif

    // We have everything, now we just need to determine WHERE the directory is, and add the appropriate messages
    int32_t dir_loc = address2DirLocation(address, index);
    msgs.push_back(TopologyMessage(index, dir_loc));

    return std::tuple(sharers, state, dir_loc, entry);
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
  }

  static AbstractDirectory * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    InfiniteRegionDirectory * directory = new InfiniteRegionDirectory();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (iter->first == "RegionSize") {
        directory->setRegionSize(iter->second);
      } else if (iter->first == "BlockSize") {
        directory->setBlockSize(iter->second);
      } else if (iter->first == "TrackShared") {
        directory->setTrackShared(iter->second);
      }
    }

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(InfiniteRegionDirectory, "InfiniteRegion");

}; // namespace
