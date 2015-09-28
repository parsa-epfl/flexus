#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/BlockDirectoryEntry.hpp>
#include <components/FastCMPDirectory/RegionDirEntry.hpp>
#include <ext/hash_map>

#include <tuple>
#include <list>

#include <components/Common/Serializers.hpp>
#include <components/Common/Util.hpp>

using nCommonSerializers::RegionDirEntrySerializer;
using nCommonUtil::log_base2;

namespace nFastCMPDirectory {

class RegionDirectory : public AbstractDirectory {
private:
  int32_t theNumSets;
  int32_t theAssociativity;

  int32_t theRegionSize;
  int32_t theBlocksPerRegion;

  uint64_t theTagMask;

  int32_t theOffsetShift;
  int32_t theOffsetMask;

  int32_t theSetShift;
  int32_t theSetMask;
  bool theSkewSet;
  int32_t theSkewShift;

  std::vector<std::vector<RegionDirEntry> > theDirectory;

  RegionDirectory() : AbstractDirectory() {};

  virtual void initialize(const std::string & aName) {

    AbstractDirectory::initialize(aName);

    DBG_Assert( theNumSets > 0 );
    DBG_Assert( theAssociativity > 0 );
    DBG_Assert( theRegionSize > 0 );

    theBlocksPerRegion = theRegionSize / theBlockSize;
    DBG_Assert( (theBlockSize * theBlocksPerRegion) == theRegionSize );

    DBG_(Dev, ( << "Initializing Directory: " << theNumSets << " sets, " << theAssociativity << " ways, " << theBlocksPerRegion << " x " << theBlockSize << "-byte blocks per region."));
    theDirectory.resize(theNumSets,
                        std::vector<RegionDirEntry>( theAssociativity,
                            RegionDirEntry(theBlocksPerRegion) ));

    theSetMask = theNumSets - 1;

    // UGLY: Hard-code for 64-bit blocks for now because I'm feeling lazy
    theSetShift = log_base2(theRegionSize);

    theOffsetShift = log_base2(theBlockSize);
    theOffsetMask = theBlocksPerRegion - 1;
    theTagMask = ~(((uint64_t)theRegionSize) - 1);

    theSkewShift = 34 - log_base2(theNumSets);
  }

  inline int32_t get_set(PhysicalMemoryAddress addr) {
    if (theSkewSet) {
      return ( ((uint64_t)addr >> theSetShift) ^ ( (uint64_t)addr >> theSkewShift) ) & theSetMask;
    } else {
      return ((uint64_t)addr >> theSetShift) & theSetMask;
    }
  }
  inline int32_t get_offset(PhysicalMemoryAddress addr) {
    return ((uint64_t)addr >> theOffsetShift) & theOffsetMask;
  }
  inline PhysicalMemoryAddress get_tag(PhysicalMemoryAddress addr) {
    return PhysicalMemoryAddress((uint64_t)addr & theTagMask);
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //BlockDirectoryEntry *my_entry = dynamic_cast<BlockDirectoryEntry*>(dir_entry);
    BlockDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));

    DBG_Assert(my_entry != nullptr);
    my_entry->addSharer(index);
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //BlockDirectoryEntry *my_entry = dynamic_cast<BlockDirectoryEntry*>(dir_entry);
    BlockDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    DBG_Assert(my_entry != nullptr);
    my_entry->addSharer(index);
    my_entry->makeExclusive(index);
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // We track precise sharers, so a failed snoop means we remove a sharer
    removeSharer(index, dir_entry, address);
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //BlockDirectoryEntry *my_entry = dynamic_cast<BlockDirectoryEntry*>(dir_entry);
    BlockDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == nullptr) {
      return;
    }
    my_entry->removeSharer(index);
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //BlockDirectoryEntry *my_entry = dynamic_cast<BlockDirectoryEntry*>(dir_entry);
    BlockDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == nullptr) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index);
  }

  RegionDirEntry * findOrCreateEntry(PhysicalMemoryAddress tag) {
    int32_t min_blocks = theBlocksPerRegion + 1;
    int32_t min_sharers = INT_MAX;
    int32_t min_way = -1;
    int32_t set = get_set(tag);
    for (int32_t way = 0; way < theAssociativity; way++) {
      if (theDirectory[set][way].tag() == tag) {
        DBG_(Iface, ( << "Found entry in way " << way ));
        return &(theDirectory[set][way]);
      } else if (min_blocks > 0) {
        int32_t bcount = 0;
        int32_t max_sharers = 0;
        for (int32_t i = 0; i < theBlocksPerRegion; i++) {
          if (theDirectory[set][way][i].state() != ZeroSharers) {
            bcount++;
            int32_t cnt = theDirectory[set][way][i].sharers().countSharers();
            if (cnt > max_sharers) {
              max_sharers = cnt;
            }
          }
        }
        if (bcount < min_blocks) {
          min_way = way;
          min_blocks = bcount;
          min_sharers = max_sharers;
        } else if (bcount == min_blocks && max_sharers < min_sharers) {
          min_way = way;
          min_blocks = bcount;
          min_sharers = max_sharers;
        }
      }
      DBG_(Iface, ( << "Did not match way " << way << ", tag=" << std::hex << theDirectory[set][way].tag() << " != " << tag));
    }

    DBG_Assert(min_way >= 0 && min_way < theAssociativity, ( << "invalid way " << min_way ));
    DBG_(Iface, ( << "Entry not Found, selecting victim way " << min_way << " with tag: " << std::hex << theDirectory[set][min_way].tag() ));
    return &(theDirectory[set][min_way]);
  }

public:
  virtual std::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) {

    PhysicalMemoryAddress tag(get_tag(address));
    RegionDirEntry * entry = findOrCreateEntry(tag);
    DBG_Assert(entry != nullptr);
    if (entry->tag() != tag) {
      // We're evicting an existing entry
      PhysicalMemoryAddress addr(entry->tag());
      DBG_(Iface, ( << "Directory Evicting Region: " << addr ));
      for (int32_t i = 0; i < theBlocksPerRegion; i++, addr += theBlockSize) {
        if ((*entry)[i].state() != ZeroSharers) {
          DBG_(Iface, ( << "Directory Evicting Block: " << addr ));
          xtra_actions.push_back([this, addr, entry](){ return this->theInvalidateAction(addr,(*entry)[i].sharers()); });
        }
      }
      entry->reset(tag);
    }

    // We have everything, now we just need to determine WHERE the directory is, and add the appropriate messages
    int32_t dir_loc = address2DirLocation(address, index);
    msgs.push_back(TopologyMessage(index, dir_loc));

    int32_t offset = get_offset(address);
    BlockEntryWrapper_p block( new BlockEntryWrapper((*entry)[offset]));
    DBG_(Iface, ( << "Lookup: tag=" << std::hex << tag << ", set=" << get_set(tag) << ", offset=" << offset << ", block=" << block ));
    return std::make_tuple((*block)->sharers(), (*block)->state(), dir_loc, block);
  }

  void saveState( std::ostream & s, const std::string & aDirName ) {
    boost::archive::binary_oarchive oa(s);
    RegionDirEntrySerializer serializer(0, theBlocksPerRegion, -1);
    for (int32_t set = 0; set < theNumSets; set++) {
      for (int32_t way = 0; way < theAssociativity; way++) {
        theDirectory[set][way].fillSerializer(serializer);
        oa << serializer;
      }
    }
  }

  bool loadState( std::istream & s, const std::string & aDirName ) {
    boost::archive::binary_iarchive ia(s);
    RegionDirEntrySerializer serializer(0, theBlocksPerRegion, -1);
    for (int32_t set = 0; set < theNumSets; set++) {
      for (int32_t way = 0; way < theAssociativity; way++) {
        ia >> serializer;
        theDirectory[set][way] = serializer;
      }
    }
    return true;
  }

  static AbstractDirectory * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    RegionDirectory * directory = new RegionDirectory();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (strcasecmp(iter->first.c_str(), "sets") == 0) {
        directory->theNumSets = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "assoc") == 0) {
        directory->theAssociativity = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "rsize") == 0) {
        directory->theRegionSize = strtol(iter->second.c_str(), nullptr, 0);
      } else if (strcasecmp(iter->first.c_str(), "skew") == 0) {
        if (strcasecmp(iter->second.c_str(), "true") == 0) {
          directory->theSkewSet = true;
        } else {
          directory->theSkewSet = false;
        }
      }
    }

    // directory->initialize();
    // Initialization called after other parameters have been set

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(RegionDirectory, "Region");

}; // namespace
