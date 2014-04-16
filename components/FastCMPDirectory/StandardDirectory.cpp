#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/debug/debug.hpp>

#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/BlockDirectoryEntry.hpp>
#include <ext/hash_map>

#include <boost/tuple/tuple.hpp>

#include <list>

#include <components/Common/Util.hpp>
#include <components/Common/Serializers.hpp>
using nCommonUtil::log_base2;
using nCommonSerializers::StdDirEntrySerializer;

namespace nFastCMPDirectory {

typedef BlockDirectoryEntry StandardDirectoryEntry;

class StandardDirectory : public AbstractDirectory {
private:
  int32_t theNumSets;
  int32_t theAssociativity;

  bool theSkewSet;
  bool theSpecialInterleaving;
  int32_t theSetShift;
  int32_t theSetMask;
  int32_t theSkewShift;

  enum ReplPolicy {
    LRURepl,
    MinSharers
  } theReplPolicy;

  std::vector<std::vector<StandardDirectoryEntry> > theDirectory;

  StandardDirectory() : AbstractDirectory(), theReplPolicy(LRURepl) {};

  virtual void initialize(const std::string & aName) {

    AbstractDirectory::initialize(aName);

    DBG_Assert( theNumSets > 0 );
    DBG_Assert( theAssociativity > 0 );

    theDirectory.resize(theNumSets, std::vector<StandardDirectoryEntry>(theAssociativity, StandardDirectoryEntry()));

    theSetMask = theNumSets - 1;

    theSetShift = log_base2(theBlockSize);

    // The Physical address has 34 usable bits (33,32 = vm index)
    theSkewShift = 34 - log_base2(theNumSets);

    DBG_(Dev, ( << "Initialized Directory with " << theNumSets << " x " << theAssociativity << "-way sets" ));
    DBG_(Dev, ( << "\tSetMask = " << std::hex << theSetMask << ", SetShift = " << std::dec << theSetShift << ", SkewShift = " << theSkewShift << ", SkewSet = " << std::boolalpha << theSkewSet ));
  }

  inline int32_t get_set(PhysicalMemoryAddress addr) {

    if (theSpecialInterleaving) {
      uint64_t index = addr >> theSetShift;
      uint64_t bank = (index & 3) | ((index & 4) << 1) | ((index & 64) >> 4) | ((index & 0x180) >> 3);
      uint64_t set = (index & ~0x1FF) | ((index & 0x38) << 3);
      if (theSkewSet) {
        return ((set ^ ((addr >> theSkewShift) & ~0x3F)) | bank) & theSetMask;
      } else {
        return (set | bank) & theSetMask;
      }
    }

    if (theSkewSet) {
      return (((uint64_t)addr >> theSetShift) ^ ((uint64_t)addr >> theSkewShift)) & theSetMask;
    } else {
      return ((uint64_t)addr >> theSetShift) & theSetMask;
    }
  }

protected:
  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //StandardDirectoryEntry *my_entry = dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == NULL) {
      my_entry = findOrCreateEntry(address);
    }
    my_entry->addSharer(index);
  }

  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //StandardDirectoryEntry *my_entry = dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    DBG_Assert(my_entry != NULL);
    my_entry->addSharer(index);
    my_entry->makeExclusive(index);
  }

  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    // We track precise sharers, so a failed snoop means we remove a sharer
    removeSharer(index, dir_entry, address);
  }

  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //StandardDirectoryEntry *my_entry = dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == NULL) {
      return;
    }
    my_entry->removeSharer(index);
  }

  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
    //StandardDirectoryEntry *my_entry = dynamic_cast<StandardDirectoryEntry*>(dir_entry);
    StandardDirectoryEntry * my_entry(&(dynamic_cast<BlockEntryWrapper *>(dir_entry.get())->block));
    if (my_entry == NULL) {
      return;
    }
    // Make it exclusive
    my_entry->makeExclusive(index);
  }

  StandardDirectoryEntry * findOrCreateEntry(PhysicalMemoryAddress addr, bool make_lru = true) {
    int32_t min_sharers = INT_MAX;
    int32_t min_way = -1;
    int32_t set = get_set(addr);
    for (int32_t way = 0; way < theAssociativity; way++) {
      if (theDirectory[set][way].tag() == addr) {
        if (make_lru) {
          std::vector<StandardDirectoryEntry>::iterator iter = theDirectory[set].begin();
          std::rotate(iter, iter + way, iter + way + 1);
          way = 0;
        }
        return &(theDirectory[set][way]);
      } else if (theReplPolicy == LRURepl && theDirectory[set][way].state() == ZeroSharers) {
        min_way = way;
      } else if (theReplPolicy == MinSharers && theDirectory[set][way].sharers().countSharers() < min_sharers) {
        min_way = way;
        min_sharers = theDirectory[set][way].sharers().countSharers();
      }
    }

    if (min_way == -1) {
      min_way = theAssociativity - 1;
    }
    if (make_lru) {
      std::vector<StandardDirectoryEntry>::iterator iter = theDirectory[set].begin();
      std::rotate(iter, iter + min_way, iter + min_way + 1);
      min_way = 0;
    }

    return &(theDirectory[set][min_way]);
  }

public:
  virtual boost::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int32_t index, PhysicalMemoryAddress address, MMType req_type, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) {

    StandardDirectoryEntry * entry = findOrCreateEntry(address, !MemoryMessage::isEvictType(req_type));
    DBG_Assert(entry != NULL);
    if (entry->tag() != address) {
      // We're evicting an existing entry
      xtra_actions.push_back(boost::lambda::bind(theInvalidateAction, entry->tag(), entry->sharers()));
      entry->reset(address);
    }

    // We have everything, now we just need to determine WHERE the directory is, and add the appropriate messages
    int32_t dir_loc = address2DirLocation(address, index);
    msgs.push_back(TopologyMessage(index, dir_loc));

    BlockEntryWrapper_p wrapper(new BlockEntryWrapper(*entry));

    return boost::tie(entry->sharers(), entry->state(), dir_loc, wrapper);
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
    StandardDirectory * directory = new StandardDirectory();

    directory->parseGenericOptions(args);

    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (strcasecmp(iter->first.c_str(), "sets") == 0) {
        directory->theNumSets = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "assoc") == 0) {
        directory->theAssociativity = strtol(iter->second.c_str(), NULL, 0);
      } else if (strcasecmp(iter->first.c_str(), "repl") == 0) {
        if (strcasecmp(iter->second.c_str(), "lru") == 0) {
          directory->theReplPolicy = LRURepl;
        } else if ( strcasecmp(iter->second.c_str(), "min_sharers") == 0) {
          directory->theReplPolicy = MinSharers;
        } else {
          DBG_Assert( false, ( << "Unknown Replacement policty: '" << iter->second << "'" ));
        }
      } else if (strcasecmp(iter->first.c_str(), "skew") == 0) {
        if (strcasecmp(iter->second.c_str(), "true") == 0) {
          directory->theSkewSet = true;
        } else {
          directory->theSkewSet = false;
        }
      } else if (strcasecmp(iter->first.c_str(), "special") == 0) {
        if (strcasecmp(iter->second.c_str(), "true") == 0) {
          directory->theSpecialInterleaving = true;
        } else {
          directory->theSpecialInterleaving = false;
        }
      }
    }

    return directory;

  }

  static const std::string name;

};

REGISTER_DIRECTORY_TYPE(StandardDirectory, "Standard");

}; // namespace
