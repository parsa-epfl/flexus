#ifndef __REGION_DIR_ENTRY_HPP__
#define __REGION_DIR_ENTRY_HPP__

#include <components/Common/Serializers.hpp>
#include <components/FastCMPDirectory/BlockDirectoryEntry.hpp>

namespace nFastCMPDirectory {

using nCommonSerializers::RegionDirEntrySerializer;

class RegionDirEntry {
private:
  PhysicalMemoryAddress               theAddress;
  std::vector<BlockDirectoryEntry>    theBlocks;

  RegionDirEntry() {
    DBG_Assert(false);
  }
public:
  int32_t                                 owner;
  bool                                shared;

  RegionDirEntry(int32_t num_blocks)
    : theAddress(0), theBlocks(num_blocks, BlockDirectoryEntry()), owner(-1)
  {}
  virtual ~RegionDirEntry() {}
  const PhysicalMemoryAddress & tag() {
    return theAddress;
  }
  virtual void reset(PhysicalMemoryAddress anAddress) {
    theAddress = anAddress;
    std::for_each(theBlocks.begin(), theBlocks.end(), boost::lambda::bind(&BlockDirectoryEntry::clear, &boost::lambda::_1));
    owner = -1;
  }
  BlockDirectoryEntry & operator[](int32_t offset) {
    return theBlocks[offset];
  }
  void fillSerializer(RegionDirEntrySerializer & serializer) {
    serializer.tag = (uint64_t)theAddress;
    serializer.num_blocks = (uint8_t)theBlocks.size();
    serializer.owner = owner;
    serializer.state.clear();
    for (uint32_t i = 0; i < theBlocks.size(); i++) {
      serializer.state.push_back(theBlocks[i].sharers().getUInt64());
    }
  }
  void operator=(const RegionDirEntrySerializer & serializer) {
    theAddress = PhysicalMemoryAddress(serializer.tag);
    theBlocks.resize(serializer.num_blocks);
    owner = serializer.owner;
    shared = false;
    for (uint32_t i = 0; i < theBlocks.size(); i++) {
      theBlocks[i].setSharers(serializer.state[i]);
    }
  }
  RegionDirEntry & operator=(const RegionDirEntry & entry) {
    theAddress = entry.theAddress;
    owner = entry.owner;
    shared = entry.shared;
    for (uint32_t i = 0; i < theBlocks.size(); i++) {
      theBlocks[i].setSharers(entry.theBlocks[i].sharers());
    }
    return *this;
  }
};

};
#endif // ! __REGION_DIR_ENTRY_HPP__
