#ifndef FASTCMPDIRECTORY_ABSTRACTDIRECTORY_HPP
#define FASTCMPDIRECTORY_ABSTRACTDIRECTORY_HPP

#include <core/stats.hpp>
#include <core/types.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/Common/Slices/RegionScoutMessage.hpp>

#include <components/FastCMPDirectory/AbstractFactory.hpp>
#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/Utility.hpp>

#include <tuple>
#include <list>

namespace nFastCMPDirectory {

using namespace Flexus::SharedTypes;

struct AbstractDirectoryStats {
  Flexus::Stat::StatCounter theSnoopMisses;
  Flexus::Stat::StatCounter theSnoopHits;

  AbstractDirectoryStats(const std::string & aName) :
    theSnoopMisses(aName + "-Snoop:Misses")
    , theSnoopHits(aName + "-Snoop:Hits") {
  }
};

class AbstractDirectoryEntry : public boost::counted_base {
public:
  virtual ~AbstractDirectoryEntry() {}
};

typedef boost::intrusive_ptr<AbstractDirectoryEntry> AbstractEntry_p;

class AbstractDirectory {
protected:
  AbstractTopology * theTopology;
  AbstractDirectoryStats * theStats;
  int32_t theNumCores;
  int32_t theBlockSize;

  int32_t theTileShift;
  uint64_t theTileMask;
  uint64_t theDirectoryInterleaving;

  enum DirectoryLocation_t {
    Distributed,
    AtMemController,
    LocalDirectory
  } theDirectoryLocation;

  struct Port_Fn_t {
    boost::function<void (RegionScoutMessage &, int)> sendRegionProbe;
    boost::function<void (boost::function<void(void)>)> scheduleDelayedAction;
  } thePorts;

  boost::function<void (PhysicalMemoryAddress, SharingVector)> theInvalidateAction;

  inline int32_t address2Tile(PhysicalMemoryAddress addr) {
    return (( addr >> theTileShift ) & theTileMask);
  }

  inline int32_t address2DirLocation(PhysicalMemoryAddress address, int32_t requester_index) {
    int32_t index = 0;

    // Directory is either:
    //  a) distributed at each tile
    //  b) distributed at each memory controller
    if (theDirectoryLocation == AtMemController) {
      index = theTopology->memoryToIndex(address);
    } else if (theDirectoryLocation == Distributed) {
      index = address2Tile(address);
    } else if (theDirectoryLocation == LocalDirectory) {
      index = requester_index;
    }
    return index;
  }

  inline void setLocation(std::string & loc) {
    if (loc == "Distributed") {
      theDirectoryLocation = Distributed;
    } else if (loc == "AtMemController") {
      theDirectoryLocation = AtMemController;
    } else if (loc == "Local") {
      theDirectoryLocation = LocalDirectory;
    }
  }

  inline void setInterleaving(std::string & arg) {
    theDirectoryInterleaving = strtoul(arg.c_str(), NULL, 0);
  }

  inline void recordSnoopMiss() {
    theStats->theSnoopMisses++;
  }

  inline void recordSnoopHit() {
    theStats->theSnoopHits++;
  }

  virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;
  virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;
  virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;
  // Don't assume that a failed snoop means we're removing an actual sharer.
  virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;
  virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;

public:

  typedef MemoryMessage::MemoryMessageType    MMType;

  AbstractDirectory() :  theDirectoryInterleaving(64), theDirectoryLocation(Distributed) {};
  virtual ~AbstractDirectory() {}

  virtual std::tuple<SharingVector, SharingState, int, AbstractEntry_p>
  lookup(int, PhysicalMemoryAddress, MMType, std::list<TopologyMessage> &msgs, std::list<boost::function<void(void)> > &xtra_actions) = 0;

  virtual int32_t processSnoopResponse(int32_t index, const MMType & type, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {
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
      case MemoryMessage::ReturnReply:
      case MemoryMessage::ReturnReplyDirty:
        recordSnoopHit();
        break;
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
    return -1;
  }

  virtual void processRequestResponse(int32_t index, const MMType & request, MMType & response, AbstractEntry_p dir_entry, PhysicalMemoryAddress address, bool off_chip) {
    switch (request) {
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictDirty:
        removeSharer(index, dir_entry, address);
        return;
      case MemoryMessage::UpgradeReq:
        DBG_Assert( response == MemoryMessage::UpgradeReply );
        makeSharerExclusive(index, dir_entry, address);
        return;
      default:
        break;
    }
    switch (response) {
      case MemoryMessage::MissReply:
      case MemoryMessage::FwdReplyOwned:
        addSharer(index, dir_entry, address);
        return;
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::MissReplyDirty:
        addExclusiveSharer(index, dir_entry, address);
        return;
      default:
        break;
    }
    DBG_Assert( false, ( << "Uknown Request+Response combination: " << request << "-" << response ));
  }

  virtual std::list<int> orderSnoops(AbstractEntry_p dir_entry, SharingVector sharers, int32_t src, bool optimize3hop, int32_t final_dest, bool multicast) {
    return theTopology->orderSnoops(sharers, src, optimize3hop, final_dest, multicast);
  }

  virtual void initialize(const std::string & aName) {
    theStats = new AbstractDirectoryStats(aName);

    DBG_Assert( (theDirectoryInterleaving & (theDirectoryInterleaving - 1)) == 0);  // Make sure it's a power of 2

    theTileShift = log_base2(theDirectoryInterleaving);
    theTileMask = theNumCores - 1;

    DBG_( Dev, ( << "Directory: TileShift = " << theTileShift << ", TileMask = " << theTileMask ));

  }

  virtual void parseGenericOptions(std::list<std::pair<std::string, std::string> > &arg_list) {
    std::list< std::pair<std::string, std::string> >::iterator iter = arg_list.begin();
    std::list< std::pair<std::string, std::string> >::iterator next = iter;
    next++;
    for (; iter != arg_list.end(); iter = next, next++) {
      if (iter->first == "Loc") {
        setLocation(iter->second);
        arg_list.erase(iter);
      } else if (iter->first == "Interleaving") {
        setInterleaving(iter->second);
        arg_list.erase(iter);
      }
    }
  }

  virtual void regionNotify(int32_t anIndex, RegionScoutMessage & aMessage) { }

  void setNumCores(int32_t n) {
    theNumCores = n;
  }

  void setTopology(AbstractTopology * top) {
    theTopology = top;
  }

  void setBlockSize(int32_t size) {
    DBG_(Dev, ( << "Setting Block size to " << size ));
    theBlockSize = size;
  }

  void setPortOperations( boost::function<void (RegionScoutMessage &, int)> regionProbe,
                          boost::function<void(boost::function<void(void)>)> delayedAction) {
    thePorts.sendRegionProbe = regionProbe;
    thePorts.scheduleDelayedAction = delayedAction;
  }

  void setInvalidateAction(boost::function<void (PhysicalMemoryAddress, SharingVector)> action) {
    theInvalidateAction = action;
  }

  virtual void saveState( std::ostream & s, const std::string & aDirName ) {
    DBG_Assert( false );
  }

  virtual bool loadState( std::istream & s, const std::string & aDirName ) {
    return false;
  }

  virtual void finalize() { }

// To work properly, every subclass should have the following two additional public members:
//   1. static std::string name
//  2. static AbstractDirectory* createInstance(std::string args)

};

// Don't hate me for this
// But I'm going to use some C++ Trickery to make some things easier.
// Here's how this works. In theory, there are many types of Topologies that all inherit from AbstractDirectory
// We want to be able to easily create one of them, based on some flexus parameters
// Rather than having lots of parameters and a big ugly if statement to sort through them
// We use 1 String argument and a system of "Factories"
// Our AbstractFactory extracts the type of directory from the string
// This maps to a static factory object for a particular type of directory
// We then pass the remaining string argument to that static factory
// Yes, this is probably excessive with one or two directories considering it's only called at startup
// But if you want to compare a bunch of different directories, you'll be amazed how easy this makes things
// This could be made into a nice little templated thingy somewhere if someone wanted to take the time to do that

#define REGISTER_DIRECTORY_TYPE(type,n) const std::string type::name = n; static ConcreteFactory<AbstractDirectory,type> type ## _Factory
#define CREATE_DIRECTORY(args)   AbstractFactory<AbstractDirectory>::createInstance(args)

/* To create and use an instance of an AbstractDirectory, you should ALWAYS do the following:
 *
 *  AbstractDirectory *dir_ptr = CREATE_DIRECTORY(args);
 *  dir_ptr->setNumCores(num_cores);
 *  dir_ptr->setTopology(topology);
 *  dir_pte->setPortOperations(regionProbe);
 *  dir_ptr->initialize(std::string &stat_name);
 *
 * Any other information needed to initialize it should be specificed as part of args
 * the format of args is "<name>[:<directory specific options>]"
 */

}; // namespace

#endif // FASTCMPDIRECTORY_ABSTRACTDIRECTORY_HPP

