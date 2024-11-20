#ifndef FASTCMPCACHE_ABSTRACTDIRECTORY_HPP
#define FASTCMPCACHE_ABSTRACTDIRECTORY_HPP

#include <components/CommonQEMU/Slices/RegionScoutMessage.hpp>
#include <components/CommonQEMU/Util.hpp>
#include <components/FastCMPCache/AbstractFactory.hpp>
#include <components/FastCMPCache/AbstractProtocol.hpp>
#include <components/FastCMPCache/SharingVector.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/stats.hpp>
#include <core/types.hpp>
#include <functional>
#include <list>
#include <tuple>

namespace nFastCMPCache {

using namespace Flexus::SharedTypes;

using nCommonUtil::log_base2;

struct AbstractDirectoryStats
{
    Flexus::Stat::StatCounter theSnoopMisses;
    Flexus::Stat::StatCounter theSnoopHits;

    AbstractDirectoryStats(const std::string& aName)
      : theSnoopMisses(aName + "-Snoop:Misses")
      , theSnoopHits(aName + "-Snoop:Hits")
    {
    }
};

class AbstractDirectoryEntry : public boost::counted_base
{
  public:
    virtual ~AbstractDirectoryEntry() {}
};

typedef boost::intrusive_ptr<AbstractDirectoryEntry> AbstractEntry_p;

class AbstractDirectory
{
  protected:
    AbstractDirectoryStats* theStats;
    int32_t theNumCores;
    int32_t theNumCaches;
    int32_t theBlockSize;

    int32_t theTileShift;
    uint64_t theTileMask;
    uint64_t theDirectoryInterleaving;

    enum DirectoryLocation_t
    {
        Distributed,
        AtMemController,
        LocalDirectory
    } theDirectoryLocation;

    struct Port_Fn_t
    {
        std::function<void(RegionScoutMessage&, int)> sendRegionProbe;
        std::function<void(std::function<void(void)>)> scheduleDelayedAction;
    } thePorts;

    std::function<void(PhysicalMemoryAddress, SharingVector)> theInvalidateAction;

    inline void recordSnoopMiss() { theStats->theSnoopMisses++; }

    inline void recordSnoopHit() { theStats->theSnoopHits++; }

    virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr)          = 0;
    virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;
    virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr)       = 0;
    // Don't assume that a failed snoop means we're removing an actual sharer.
    virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr)         = 0;
    virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress addr) = 0;

  public:
    typedef MemoryMessage::MemoryMessageType MMType;

    AbstractDirectory()
      : theDirectoryInterleaving(64)
      , theDirectoryLocation(Distributed) {};
    virtual ~AbstractDirectory() {}

    virtual std::tuple<SharingVector, SharingState, AbstractEntry_p>
    lookup(int, PhysicalMemoryAddress, MMType, std::list<std::function<void(void)>>& xtra_actions) = 0;

    virtual std::tuple<SharingVector, SharingState, AbstractEntry_p, bool> snoopLookup(int,
                                                                                       PhysicalMemoryAddress,
                                                                                       MMType) = 0;

    virtual void processSnoopResponse(int32_t index,
                                      const MMType& type,
                                      AbstractEntry_p dir_entry,
                                      PhysicalMemoryAddress address)
    {
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
            case MemoryMessage::ReturnReplyDirty: recordSnoopHit(); break;
            case MemoryMessage::DowngradeAck:
            case MemoryMessage::DownUpdateAck: recordSnoopHit(); break;
            default: DBG_Assert(false, (<< "Unknown Snoop Response '" << type << "'")); break;
        }
    }

    virtual void processRequestResponse(int32_t index,
                                        const MMType& request,
                                        MMType& response,
                                        AbstractEntry_p dir_entry,
                                        PhysicalMemoryAddress address,
                                        bool off_chip,
                                        bool fromSMMU)
    {
        switch (request) {
            case MemoryMessage::EvictClean:
            case MemoryMessage::EvictWritable:
            case MemoryMessage::EvictDirty: removeSharer(index, dir_entry, address); return;    // TODO: IDK if SMMU would hit this case. For correctness, an SMMU write should perform this to invalidate from sharers.
            case MemoryMessage::UpgradeReq:
                DBG_Assert(response == MemoryMessage::UpgradeReply,
                           (<< " Received " << request << " in response to UpgradeReq."));
                makeSharerExclusive(index, dir_entry, address);    // No read or write from the SMMU should make a sharer exclusive!
                                                                        // A read should not change the state while a write should removeSharer
                                                                        // ! For SMMU tracing, this switch will never match
                return;
            default: break;
        }

        // If a Snoop fails for SMMU, it requests main memory for data
        // Response from main memory would add the requester to the list
        // of sharers. But for SMMU, as SMMU is not in the sharers list, 
        // we do not need to add anything to the sharer list!
        switch (response) {
            case MemoryMessage::MissReply:
            case MemoryMessage::FwdReplyOwned: if (!fromSMMU) addSharer(index, dir_entry, address); return;
            case MemoryMessage::MissReplyWritable:
            case MemoryMessage::MissReplyDirty: if (!fromSMMU) addExclusiveSharer(index, dir_entry, address); return;
            case MemoryMessage::NonAllocatingStoreReply:
                // Snoop replies will remove any previous sharers and we never add any
                // sharers so nothing new to do
                return;
            default: break;
        }
        DBG_Assert(false, (<< "Uknown Request+Response combination: " << request << "-" << response));
    }

    virtual void updateLRU(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address) {}

    virtual void initialize(const std::string& aName) { theStats = new AbstractDirectoryStats(aName); }

    virtual void parseGenericOptions(std::list<std::pair<std::string, std::string>>& arg_list)
    {
        std::list<std::pair<std::string, std::string>>::iterator iter = arg_list.begin();
        std::list<std::pair<std::string, std::string>>::iterator next = iter;
        next++;
        for (; iter != arg_list.end(); iter = next, next++) {
            // Add generic options here if necessary
        }
    }

    virtual void regionNotify(int32_t anIndex, RegionScoutMessage& aMessage) {}

    void setNumCores(int32_t n) { theNumCores = n; }
    void setNumCaches(int32_t n) { theNumCaches = n; }

    void setBlockSize(int32_t size) { theBlockSize = size; }

    void setPortOperations(std::function<void(RegionScoutMessage&, int)> regionProbe,
                           std::function<void(std::function<void(void)>)> delayedAction)
    {
        thePorts.sendRegionProbe       = regionProbe;
        thePorts.scheduleDelayedAction = delayedAction;
    }

    void setInvalidateAction(std::function<void(PhysicalMemoryAddress, SharingVector)> action)
    {
        theInvalidateAction = action;
    }

    virtual void saveState(std::ostream& s, const std::string& aDirName) { DBG_Assert(false); }

    virtual bool loadState(std::istream& s, const std::string& aDirName) { return false; }

    virtual void saveStateJSON(std::ostream& s, const std::string& aDirName) { DBG_Assert(false); }

    virtual bool loadStateJSON(std::istream& s, const std::string& aDirName) { return false; }

    virtual void finalize() {}

    // To work properly, every subclass should have the following two additional
    // public members:
    //   1. static std::string name
    //  2. static AbstractDirectory* createInstance(std::string args)
};

// Don't hate me for this
// But I'm going to use some C++ Trickery to make some things easier.
// Here's how this works. In theory, there are many types of Topologies that all
// inherit from AbstractDirectory We want to be able to easily create one of
// them, based on some flexus parameters Rather than having lots of parameters
// and a big ugly if statement to sort through them We use 1 String argument and
// a system of "Factories" Our AbstractFactory extracts the type of directory
// from the string This maps to a static factory object for a particular type of
// directory We then pass the remaining string argument to that static factory
// Yes, this is probably excessive with one or two directories considering it's
// only called at startup But if you want to compare a bunch of different
// directories, you'll be amazed how easy this makes things This could be made
// into a nice little templated thingy somewhere if someone wanted to take the
// time to do that

#define REGISTER_DIRECTORY_TYPE(type, n)                                                                               \
    const std::string type::name = n;                                                                                  \
    static ConcreteFactory<AbstractDirectory, type> type##_Factory
#define CREATE_DIRECTORY(args) AbstractFactory<AbstractDirectory>::createInstance(args)

/* To create and use an instance of an AbstractDirectory, you should ALWAYS do
 * the following:
 *
 *  AbstractDirectory *dir_ptr = CREATE_DIRECTORY(args);
 *  dir_ptr->setNumCores(num_cores);
 *  dir_ptr->setTopology(topology);
 *  dir_pte->setPortOperations(regionProbe);
 *  dir_ptr->initialize(std::string &stat_name);
 *
 * Any other information needed to initialize it should be specificed as part of
 * args the format of args is "<name>[:<directory specific options>]"
 */

}; // namespace nFastCMPCache

#endif // FASTCMPCACHE_ABSTRACTDIRECTORY_HPP
