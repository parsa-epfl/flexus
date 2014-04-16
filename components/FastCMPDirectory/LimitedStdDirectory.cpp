#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/LimitedSharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <ext/hash_map>

#include <boost/tuple/tuple.hpp>

#include <list>

#include <components/Common/Util.hpp>
#include <components/Common/Serializers.hpp>
using nCommonUtil::log_base2;
using nCommonSerializers::StdDirEntrySerializer;

namespace nFastCMPDirectory {

class LimitedDirectoryEntry : public AbstractDirectoryEntry {
  LimitedDirectoryEntry(PhysicalMemoryAddress anAddr, SharingState aState, int32_t numPointers, int32_t granularity)
    : address(anAddr), sharers(numPointers, granularity), state(aState) {}

private:
  LimitedDirectoryEntry() : address(0), sharers(0, 0), state(ZeroSharers) {
    DBG_Assert(false, ( << "Private constructor - off limits."));
  }
  PhysicalMemoryAddress address;
  LimitedSharingVector sharers;
  SharingState   state;
  std::list<int>   pending_invalidates;

  friend class LimitedStdDirectory;

  inline void addSharer(int32_t index) {
    DBG_(Iface, ( << "addSharer(" << index << ") 0x" << std::hex << (uint64_t)address << " - " << sharers.getSharers() ));
    sharers.addSharer(index);
    if (sharers.countSharers() == 1) {
      state = OneSharer;
    } else {
      state = ManySharers;
    }
    DBG_(Iface, ( << "addSharer(" << index << ") 0x" << std::hex << (uint64_t)address << " - " << sharers.getSharers() ));
  }

  inline void makeExclusive(int32_t index) {
    DBG_Assert(sharers.isSharer(index), ( << "Core " << index << " is not a sharer " << sharers.getSharers() ) );
    sharers.setSharer(index);
    state = ExclSharer;
    DBG_(Iface, ( << "makeExclusive(" << index << ") 0x" << std::hex << (uint64_t)address << " - " << sharers.getSharers() ));
  }

  inline void removeSharer(int32_t index) {
    pending_invalidates.push_back(index);
    sharers.removeSharer(index);
    if (sharers.countSharers() == 0) {
      state = ZeroSharers;
    } else if (sharers.countSharers() == 1) {
      state = OneSharer;
    }
    DBG_(Iface, ( << "removeSharer(" << index << ") 0x" << std::hex << (uint64_t)address << " - " << sharers.getSharers() ));
  }

  inline void clearPendingInvals() {
    if (!pending_invalidates.empty()) {
      if (state != ManySharers) {
        pending_invalidates.clear();
      } else {
        sharers.removeAll(pending_invalidates);
        int32_t cnt = sharers.countSharers();
        if (cnt == 0) {
          state = ZeroSharers;
        } else if (cnt > 1) {
          state = ManySharers;
        } else {
          state = OneSharer;
        }
      }
    }
  }

  StdDirEntrySerializer getSerializer() {
    return StdDirEntrySerializer((uint64_t)address, sharers.getUInt64());
  }
  void operator=(const StdDirEntrySerializer & serializer) {
    address = PhysicalMemoryAddress(serializer.tag);
    sharers.setSharers(serializer.state);
    int32_t count = sharers.countSharers();
    if (count == 1) {
      state = ExclSharer;
    } else if (count == 0) {
      state = ZeroSharers;
    } else {
      state = ManySharers;
    }
  }

};

struct LimitedEntryWrapper : public AbstractDirectoryEntry {
  LimitedEntryWrapper(LimitedDirectoryEntry & e) : entry(e) {}
  LimitedDirectoryEntry & entry;
  LimitedDirectoryEntry * operator->() {
    return &entry;
  }
  LimitedDirectoryEntry & operator*() {
    return entry;
  }
};

typedef boost::intrusive_ptr<LimitedEntryWrapper> LimitedEntryWrapper_p;

class LimitedStdDirectory : public AbstractDirectory {
private:
  int32_t theNumSets;
  int32_t theAssociativity;

  int32_t theSetShift;
  int32_t theSetMask;
  bool theSkewSet;
  int32_t theSkewShift;

  int32_t theNumPointers;
  int32_t theGranularity;

  std::vector<std::vector<LimitedDirectoryEntry> > theDirectory;

  LimitedStdDirectory() : AbstractDirectory() {};

  void initialize() {

    DBG_Assert( theNumSets > 0 );
    DBG_Assert( theAssociativity > 0 );

    theDirectory.resize(theNumSets,
                        std::vector<LimitedDirectoryEntry>(theAssociativity,
                            LimitedDirectoryEntry(PhysicalMemoryAddress(0), ZeroSharers, theNumPointers, theGranularity)));

    theSetMask = theNumSets - 1;

    // UGLY: Hard-code for 64-bit blocks for now because I'm feeling lazy
    theSetShift = log_base2(theBlockSize);
    if (theSkewSet) {
      theSkewShift = 34 - log_base2(theNumSets);
    } else {
      theSkewShift = 0;
    }
  }

  inline int32_t get_set(PhysicalMemoryAddress addr) {
    if (theSkewSet) {
      return (((uint64_t)addr >> theSetShift) ^ ((uint64_t)addr >> theSkewShift)) & theSetMask;
    } else {
      return ((uint64_t)addr >> theSetShift) & theSetMask;
    }
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    LimitedDirectoryEntry * my_entry(&(dynamic_cast<LimitedEntryWrapper *>(dir_entry.get())->entry));
    if (my_entry == NULL) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index);
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //LimitedDirectoryEntry *my_entry = dynamic_cast<LimitedDirectoryEntry*>(dir_entry);
    LimitedDirectoryEntry * my_entry(&(dynamic_cast<LimitedEntryWrapper *>(dir_entry.get())->entry));
    DBG_Assert(my_entry != NULL);
    my_entry->addSharer(index);
    my_entry->makeExclusive(index);
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // We track precise sharers, so a failed snoop means we remove a sharer
    removeSharer(index, dir_entry, address);
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //LimitedDirectoryEntry *my_entry = dynamic_cast<LimitedDirectoryEntry*>(dir_entry);
    LimitedDirectoryEntry * my_entry(&(dynamic_cast<LimitedEntryWrapper *>(dir_entry.get())->entry));
    if (my_entry == NULL) {
      return;
    }
    my_entry->removeSharer(index);
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //LimitedDirectoryEntry *my_entry = dynamic_cast<LimitedDirectoryEntry*>(dir_entry);
    LimitedDirectoryEntry * my_entry(&(dynamic_cast<LimitedEntryWrapper *>(dir_entry.get())->entry));
    if (my_entry == NULL) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index);
  }

  virtual void processRequestResponse(int32_t index, const MMType & request, MMType & response, AbstractEntry_p dir_entry, PhysicalMemoryAddress address, bool off_chip) {

    AbstractDirectory::processRequestResponse(index, request, response, dir_entry, address, off_chip);

    //LimitedDirectoryEntry *my_entry = dynamic_cast<LimitedDirectoryEntry*>(dir_entry);
    LimitedDirectoryEntry * my_entry(&(dynamic_cast<LimitedEntryWrapper *>(dir_entry.get())->entry));
    if (my_entry == NULL) {
      return;
    }
    my_entry->clearPendingInvals();
  }

  LimitedDirectoryEntry * findOrCreateEntry(PhysicalMemoryAddress addr, bool make_lru = true) {
    int32_t min_way = -1;
    int32_t set = get_set(addr);
    for (int32_t way = 0; way < theAssociativity; way++) {
      if (theDirectory[set][way].address == addr) {
        if (make_lru) {
          std::vector<LimitedDirectoryEntry>::iterator iter = theDirectory[set].begin();
          std::rotate(iter, iter + way, iter + way + 1);
          way = 0;
        }
        return &(theDirectory[set][way]);
      } else if (theDirectory[set][way].state == ZeroSharers) {
        min_way = way;
      }
    }

    if (min_way == -1) {
      min_way = theAssociativity - 1;
    }
    if (make_lru) {
      std::vector<LimitedDirectoryEntry>::iterator iter = theDirectory[set].begin();
      std::rotate(iter, iter + min_way, iter + min_way + 1);
      min_way = 0;
    }

    return &(theDirectory[set][min_way]);
  }

public:
  virtual boost::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) {

    LimitedDirectoryEntry * entry = findOrCreateEntry(address, !MemoryMessage::isEvictType(req_type));
    DBG_Assert(entry != NULL);
    if (entry->address != address) {
      // We're evicting an existing entry
      xtra_actions.push_back(boost::lambda::bind(theInvalidateAction, entry->address, entry->sharers));
      entry->address = address;
      entry->sharers.clear();
      entry->state = ZeroSharers;
    }

    // We have everything, now we just need to determine WHERE the directory is, and add the appropriate messages
    int32_t dir_loc = address2DirLocation(address, index);
    msgs.push_back(TopologyMessage(index, dir_loc));

    LimitedEntryWrapper_p wrapper(new LimitedEntryWrapper(*entry));

    return boost::tie(entry->sharers, entry->state, dir_loc, wrapper);
  }

  void saveState( std::ostream & s, const std::string & aDirName ) {
    boost::archive::binary_oarchive oa(s);
    StdDirEntrySerializer serializer;
    for (int32_t set = 0; set < theNumSets; set++) {
      for (int32_t way = 0; way < theAssociativity; way++) {
        serializer = theDirectory[set][way].getSerializer();
        oa << serializer;
      }
    }
  }

  bool loadState( std::istream & s, const std::string & aDirName ) {
    boost::archive::binary_iarchive ia(s);
    StdDirEntrySerializer serializer;
    for (int32_t set = 0; set < theNumSets; set++) {
      for (int32_t way = 0; way < theAssociativity; way++) {
        ia >> serializer;
        theDirectory[set][way] = serializer;
      }
    }
    return true;
  }

  static AbstractDirectory * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    LimitedStdDirectory * directory = new LimitedStdDirectory();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (strcasecmp(iter->first.c_str(), "sets") == 0) {
        directory->theNumSets = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "assoc") == 0) {
        directory->theAssociativity = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "num_pointers") == 0) {
        directory->theNumPointers = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "granularity") == 0) {
        directory->theGranularity = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "skew") == 0) {
        if (strcasecmp(iter->second.c_str(), "true") == 0) {
          directory->theSkewSet = true;
        } else {
          directory->theSkewSet = false;
        }
      }
    }

    directory->initialize();

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(LimitedStdDirectory, "Limited");

}; // namespace
