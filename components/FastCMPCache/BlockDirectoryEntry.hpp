#ifndef __BLOCK_DIRECTORY_ENTRY_HPP__
#define __BLOCK_DIRECTORY_ENTRY_HPP__

#include <components/CommonQEMU/Serializers.hpp>

namespace nFastCMPCache {

using nCommonSerializers::StdDirEntrySerializer;

class BlockDirectoryEntry : public AbstractDirectoryEntry {
private:
  PhysicalMemoryAddress theAddress;
  SharingVector theSharers;
  SharingState theState;

public:
  BlockDirectoryEntry(PhysicalMemoryAddress addr) : theAddress(addr), theState(ZeroSharers) {}
  BlockDirectoryEntry() : theAddress(0), theState(ZeroSharers) {}
  virtual ~BlockDirectoryEntry() {}

  const SharingVector & sharers() const {
    return theSharers;
  }
  const SharingState & state() const {
    return theState;
  }
  const PhysicalMemoryAddress & tag() const {
    return theAddress;
  }

  inline void addSharer(int32_t index) {
    theSharers.addSharer(index);
    if (theSharers.countSharers() == 1) {
      theState = OneSharer;
    } else {
      theState = ManySharers;
    }
  }

  inline void makeExclusive(int32_t index) {
    DBG_Assert(theSharers.isSharer(index), ( << "Core " << index << " is not a sharer " << theSharers.getSharers() ) );
    DBG_Assert(theSharers.countSharers() == 1, ( << "Cannot make xclusive, sharers = " << theSharers ));
    theState = OneSharer;
  }

  inline void removeSharer(int32_t index) {
    theSharers.removeSharer(index);
    if (theSharers.countSharers() == 0) {
      theState = ZeroSharers;
    } else if (theSharers.countSharers() == 1) {
      theState = OneSharer;
    }
  }
  void setSharers(const SharingVector & new_sharers) {
    theSharers = new_sharers;
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }
  void setSharers(uint64_t new_sharers) {
    theSharers.setSharers(new_sharers);
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }

  inline void clear() {
    theSharers.clear();
    theState = ZeroSharers;
  }

  inline void reset(PhysicalMemoryAddress addr) {
    theAddress = addr;
    theSharers.clear();
    theState = ZeroSharers;
  }

  BlockDirectoryEntry & operator&=(const BlockDirectoryEntry & entry) {
    theSharers &= entry.theSharers;
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
    return *this;
  }

  StdDirEntrySerializer getSerializer() {
    return StdDirEntrySerializer((uint64_t)theAddress, theSharers.getUInt64());
  }

  void operator=(const StdDirEntrySerializer & serializer) {
    theAddress = PhysicalMemoryAddress(serializer.tag);
    theSharers.setSharers(serializer.state);
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }

};

typedef boost::intrusive_ptr<BlockDirectoryEntry> BlockDirectoryEntry_p;

struct BlockEntryWrapper : public AbstractDirectoryEntry {
  BlockEntryWrapper(BlockDirectoryEntry & block) : block(block) {}
  BlockDirectoryEntry & block;

  BlockDirectoryEntry * operator->() const {
    return &block;
  }
  BlockDirectoryEntry & operator*() const {
    return block;
  }
};

typedef boost::intrusive_ptr<BlockEntryWrapper> BlockEntryWrapper_p;

};

#endif //! __BLOCK_DIRECTORY_ENTRY_HPP__
