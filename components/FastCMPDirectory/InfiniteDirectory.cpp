#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/Utility.hpp>
#include <ext/hash_map>

#include <boost/tuple/tuple.hpp>
#include <boost/lambda/lambda.hpp>

#include <list>
#include <algorithm>

#include <components/Common/Serializers.hpp>
using nCommonSerializers::StdDirEntrySerializer;

using namespace boost::lambda;

namespace nFastCMPDirectory {

class InfiniteDirectoryEntry : public AbstractDirectoryEntry {
  InfiniteDirectoryEntry(InfiniteDirectoryEntry & entry) : address(entry.address), sharers(entry.sharers), state(entry.state) {}
  InfiniteDirectoryEntry(PhysicalMemoryAddress addr) : address(addr), state(ZeroSharers) {}

  InfiniteDirectoryEntry(const StdDirEntrySerializer & serializer) : address(serializer.tag) {
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

  InfiniteDirectoryEntry() : address(0), state(ZeroSharers) {}

private:
  PhysicalMemoryAddress address;
  SharingVector sharers;
  SharingState state;

  friend class InfiniteDirectory;
  friend class HasZeroSharers;

  inline void addSharer(int32_t index) {
    sharers.addSharer(index);
    if (sharers.countSharers() == 1) {
      state = OneSharer;
    } else {
      state = ManySharers;
    }
  }

  inline void makeExclusive(int32_t index) {
    DBG_Assert(sharers.isSharer(index), ( << "Core " << index << " is not a sharer " << sharers.getSharers() << " for block " << std::hex << address ) );
    DBG_Assert(sharers.countSharers() == 1);
    state = ExclSharer;
  }

  inline void removeSharer(int32_t index) {
    sharers.removeSharer(index);
    if (sharers.countSharers() == 0) {
      state = ZeroSharers;
    } else if (sharers.countSharers() == 1) {
      state = OneSharer;
    }
  }

  StdDirEntrySerializer getSerializer() const {
    return StdDirEntrySerializer((uint64_t)address, sharers.getUInt64());
  }

  InfiniteDirectoryEntry & operator=(const StdDirEntrySerializer & serializer) {
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
    return *this;
  }
};

typedef boost::intrusive_ptr<InfiniteDirectoryEntry> InfiniteDirectoryEntry_p;

struct HasZeroSharers {
  bool operator()(const std::pair<const PhysicalMemoryAddress, InfiniteDirectoryEntry_p> &entry) const {
    return (entry.second->state == ZeroSharers);
  }
};

class InfiniteDirectory : public AbstractDirectory {
private:
  struct AddrHash {
    std::size_t operator()(const PhysicalMemoryAddress & addr) const {
      return addr >> 6;
    }
  };
  typedef __gnu_cxx::hash_map<PhysicalMemoryAddress, InfiniteDirectoryEntry_p, AddrHash> inf_directory_t;
  inf_directory_t theDirectory;

  InfiniteDirectory() : AbstractDirectory() {};

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteDirectoryEntry * my_entry = dynamic_cast<InfiniteDirectoryEntry *>(dir_entry.get());
    if (my_entry == NULL) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index);
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteDirectoryEntry * my_entry = dynamic_cast<InfiniteDirectoryEntry *>(dir_entry.get());
    if (my_entry == NULL) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index);
    my_entry->makeExclusive(index);
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // We track precise sharers, so a failed snoop means we remove a sharer
    removeSharer(index, dir_entry, address);
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteDirectoryEntry * my_entry = dynamic_cast<InfiniteDirectoryEntry *>(dir_entry.get());
    if (my_entry == NULL) {
      return;
    }
    my_entry->removeSharer(index);
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    InfiniteDirectoryEntry * my_entry = dynamic_cast<InfiniteDirectoryEntry *>(dir_entry.get());
    if (my_entry == NULL) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index);
  }

  InfiniteDirectoryEntry * findOrCreateEntry(PhysicalMemoryAddress addr) {
    inf_directory_t::iterator iter;
    bool success;
    std::tie(iter, success) = theDirectory.insert( std::make_pair(addr, InfiniteDirectoryEntry_p()) );
    if (success) {
      iter->second = new InfiniteDirectoryEntry(addr);
    }
    return iter->second.get();
  }

  InfiniteDirectoryEntry_p findEntry(PhysicalMemoryAddress addr) {
    inf_directory_t::iterator iter = theDirectory.find(addr);
    if (iter == theDirectory.end()) {
      return NULL;
    }
    return iter->second;
  }

public:
  virtual boost::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) {

    InfiniteDirectoryEntry_p entry = findEntry(address);
    SharingVector sharers;
    SharingState  state = ZeroSharers;
    if (entry != NULL) {
      sharers = entry->sharers;
      state = entry->state;
    }

    // We have everything, now we just need to determine WHERE the directory is, and add the appropriate messages
    int32_t dir_loc = address2DirLocation(address, index);
    msgs.push_back(TopologyMessage(index, dir_loc));

    return boost::tie(sharers, state, dir_loc, entry);
  }

  virtual void processRequestResponse(int32_t index, const MMType & request, MMType & response,
                                      AbstractEntry_p dir_entry, PhysicalMemoryAddress address, bool off_chip) {
    // First, do default behaviour
    AbstractDirectory::processRequestResponse(index, request, response, dir_entry, address, off_chip);

    if (!MemoryMessage::isEvictType(request)) {
      return;
    }

    InfiniteDirectoryEntry * my_entry = dynamic_cast<InfiniteDirectoryEntry *>(dir_entry.get());
    if (my_entry == NULL) {
      return;
    }
    if (my_entry->state == ZeroSharers) {
      theDirectory.erase(address);
    }
  }

  void saveState( std::ostream & s, const std::string & aDirName ) {
    boost::archive::binary_oarchive oa(s);
#if 0
    inf_directory_t::iterator iter = theDirectory.begin();
    inf_directory_t::iterator prev = theDirectory.begin();
    uint32_t count = 0;
    for (; iter != theDirectory.end(); prev = iter, iter++) {
      if (iter->second->state == ZeroSharers) {
        theDirectory.erase(iter);
        if (count == 0) {
          iter = theDirectory.begin();
        } else {
          iter = prev;
        }
      } else {
        count++;
      }
    }
    //uint32_t count = (uint32_t)theDirectory.size();
    oa << count;
    DBG_(Dev, ( << "Saving " << count << " directory entries." ));
    StdDirEntrySerializer serializer;
    for (; iter != theDirectory.end(); iter++) {
      serializer = it->second->getSerializer();
      DBG_(Trace, ( << "Directory saving block: " << serializer ));
      oa << serializer;
    }
    std::vector<std::pair<PhysicalMemoryAddress, InfiniteDirectoryEntry_p> > dir_vector;
    std::vector<std::pair<PhysicalMemoryAddress, InfiniteDirectoryEntry_p> >::iterator it, last_it;
    last_it = std::remove_copy_if(theDirectory.begin(), theDirectory.end(), dir_vector.begin(), HasZeroSharers() );

    it = dir_vector.begin();
    uint32_t count = last_it - it;
    DBG_(Dev, ( << "Saving " << count << " directory entries." ));
    StdDirEntrySerializer serializer;
    for (; it != last_it; it++) {
      serializer = it->second->getSerializer();
      DBG_(Trace, ( << "Directory saving block: " << serializer ));
      oa << serializer;
    }

#endif

    uint32_t count = (uint32_t)theDirectory.size();
    oa << count;
    DBG_(Dev, ( << "Saving " << count << " directory entries." ));
    StdDirEntrySerializer serializer;
    inf_directory_t::iterator iter = theDirectory.begin();
    for (; iter != theDirectory.end(); iter++) {
      serializer = iter->second->getSerializer();
      DBG_(Trace, ( << "Directory saving block: " << serializer ));
      oa << serializer;
    }
  }

  bool loadState( std::istream & s, const std::string & aDirName ) {
    boost::archive::binary_iarchive ia(s);
    int32_t count;
    ia >> count;
    StdDirEntrySerializer serializer;
    DBG_(Dev, ( << "Directory loading " << count << " entries." ));
    for (; count > 0; count--) {
      ia >> serializer;
      DBG_(Trace, ( << "Directory loading block " << serializer));
      InfiniteDirectoryEntry entry(serializer);
      if (entry.state != ZeroSharers) {
        theDirectory.insert( std::pair<PhysicalMemoryAddress, InfiniteDirectoryEntry *>(PhysicalMemoryAddress(serializer.tag), new InfiniteDirectoryEntry(serializer)));
      }
    }
    return true;
  }

  static AbstractDirectory * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    InfiniteDirectory * directory = new InfiniteDirectory();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      // Parse any specifc options we want
    }

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(InfiniteDirectory, "Infinite");

}; // namespace
