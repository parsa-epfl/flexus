#include <algorithm>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <components/CommonQEMU/Serializers.hpp>
#include <components/FastCMPCache/AbstractDirectory.hpp>
#include <components/FastCMPCache/AbstractProtocol.hpp>
#include <components/FastCMPCache/SharingVector.hpp>
#include <core/debug/debug.hpp>
#include <list>
#include <unordered_map>
// using nCommonSerializers::StdDirEntrySerializer;
using nCommonSerializers::StdDirEntryExtendedSerializer;

#include <components/CommonQEMU/Util.hpp>
using nCommonUtil::log_base2;

#include <core/checkpoint/json.hpp>
using json = nlohmann::json;
#define MAX_NUM_SHARERS 128

namespace nFastCMPCache {

class InfiniteDirectoryEntry : public AbstractDirectoryEntry
{
    InfiniteDirectoryEntry(InfiniteDirectoryEntry& entry)
      : address(entry.address)
      , sharers(entry.sharers)
      , state(entry.state)
    {
    }
    InfiniteDirectoryEntry(PhysicalMemoryAddress addr)
      : address(addr)
      , state(ZeroSharers)
    {
    }

    // InfiniteDirectoryEntry(const StdDirEntrySerializer & serializer) :
    // address(serializer.tag) {
    InfiniteDirectoryEntry(const StdDirEntryExtendedSerializer& serializer)
      : address(serializer.tag)
    {
        sharers.setSharers(serializer.state);

        int32_t count = sharers.countSharers();
        if (count == 1) {
            state = OneSharer;
        } else if (count == 0) {
            state = ZeroSharers;
        } else {
            state = ManySharers;
        }
    }

    InfiniteDirectoryEntry()
      : address(0)
      , state(ZeroSharers)
    {
    }

    InfiniteDirectoryEntry(PhysicalMemoryAddress addr, std::bitset<MAX_NUM_SHARERS> state)
      : address(addr)
    {
        sharers.setSharers(state);
        int32_t count = sharers.countSharers();
        if (count == 1) {
            state = OneSharer;
        } else if (count == 0) {
            state = ZeroSharers;
        } else {
            state = ManySharers;
        }
    }

  private:
    PhysicalMemoryAddress address;
    SharingVector sharers;
    SharingState state;

    friend class InfiniteDirectory;
    friend class HasZeroSharers;

    inline void addSharer(int32_t index)
    {
        sharers.addSharer(index);
        if (sharers.countSharers() == 1) {
            state = OneSharer;
        } else {
            state = ManySharers;
        }
    }

    inline void makeExclusive(int32_t index)
    {
        DBG_Assert(
          sharers.isSharer(index),
          (<< "Core " << index << " is not a sharer " << sharers.getSharers() << " for block " << std::hex << address));
        DBG_Assert(sharers.countSharers() == 1);
        state = OneSharer;
    }

    inline void removeSharer(int32_t index)
    {
        sharers.removeSharer(index);
        if (sharers.countSharers() == 0) {
            state = ZeroSharers;
        } else if (sharers.countSharers() == 1) {
            state = OneSharer;
        }
    }

    // StdDirEntrySerializer getSerializer() const {
    //  return StdDirEntrySerializer((uint64_t)address, sharers.getUInt64());
    //}

    StdDirEntryExtendedSerializer getSerializer() const
    {
        return StdDirEntryExtendedSerializer((uint64_t)address, sharers.getSharers());
    }

    // InfiniteDirectoryEntry & operator=(const StdDirEntrySerializer &
    // serializer) {
    InfiniteDirectoryEntry& operator=(const StdDirEntryExtendedSerializer& serializer)
    {
        address = PhysicalMemoryAddress(serializer.tag);
        sharers.setSharers(serializer.state);
        int32_t count = sharers.countSharers();
        if (count == 1) {
            state = OneSharer;
        } else if (count == 0) {
            state = ZeroSharers;
        } else {
            state = ManySharers;
        }
        return *this;
    }
};

typedef boost::intrusive_ptr<InfiniteDirectoryEntry> InfiniteDirectoryEntry_p;

struct HasZeroSharers
{
    bool operator()(const std::pair<const PhysicalMemoryAddress, InfiniteDirectoryEntry_p>& entry) const
    {
        return (entry.second->state == ZeroSharers);
    }
};

class InfiniteDirectory : public AbstractDirectory
{
  private:
    struct AddrHash
    {
        std::size_t operator()(const PhysicalMemoryAddress& addr) const { return addr >> 6; }
    };
    typedef std::unordered_map<PhysicalMemoryAddress, InfiniteDirectoryEntry_p, AddrHash> inf_directory_t;
    inf_directory_t theDirectory;

    InfiniteDirectory()
      : AbstractDirectory() {};

  protected:
    virtual void addSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address)
    {
        InfiniteDirectoryEntry* my_entry = dynamic_cast<InfiniteDirectoryEntry*>(dir_entry.get());
        if (my_entry == nullptr) { my_entry = findOrCreateEntry(address); }
        my_entry->addSharer(index);
    }

    virtual void addExclusiveSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address)
    {
        InfiniteDirectoryEntry* my_entry = dynamic_cast<InfiniteDirectoryEntry*>(dir_entry.get());
        if (my_entry == nullptr) { my_entry = findOrCreateEntry(address); }
        my_entry->addSharer(index);
        my_entry->makeExclusive(index);
    }

    virtual void failedSnoop(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address)
    {
        // We track precise sharers, so a failed snoop means we remove a sharer
        removeSharer(index, dir_entry, address);
    }

    virtual void removeSharer(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address)
    {
        InfiniteDirectoryEntry* my_entry = dynamic_cast<InfiniteDirectoryEntry*>(dir_entry.get());
        if (my_entry == nullptr) { return; }
        my_entry->removeSharer(index);
    }

    virtual void makeSharerExclusive(int32_t index, AbstractEntry_p dir_entry, PhysicalMemoryAddress address)
    {
        InfiniteDirectoryEntry* my_entry = dynamic_cast<InfiniteDirectoryEntry*>(dir_entry.get());
        if (my_entry == nullptr) { return; }
        // Make it exclusive
        my_entry->makeExclusive(index);
    }

    InfiniteDirectoryEntry* findOrCreateEntry(PhysicalMemoryAddress addr)
    {
        inf_directory_t::iterator iter;
        bool success;
        std::tie(iter, success) = theDirectory.insert(std::make_pair(addr, InfiniteDirectoryEntry_p()));
        if (success) { iter->second = new InfiniteDirectoryEntry(addr); }
        return iter->second.get();
    }

    InfiniteDirectoryEntry_p findEntry(PhysicalMemoryAddress addr)
    {
        inf_directory_t::iterator iter = theDirectory.find(addr);
        if (iter == theDirectory.end()) { return nullptr; }
        return iter->second;
    }

  public:
    virtual std::tuple<SharingVector, SharingState, AbstractEntry_p> lookup(
      int32_t index,
      PhysicalMemoryAddress address,
      MMType req_type,
      std::list<std::function<void(void)>>& xtra_actions)
    {

        InfiniteDirectoryEntry_p entry = findEntry(address);
        SharingVector sharers;
        SharingState state = ZeroSharers;
        if (entry != nullptr) {
            sharers = entry->sharers;
            state   = entry->state;
        }

        return std::tie(sharers, state, entry);
    }

    virtual std::tuple<SharingVector, SharingState, AbstractEntry_p, bool> snoopLookup(int32_t index,
                                                                                       PhysicalMemoryAddress address,
                                                                                       MMType req_type)
    {

        InfiniteDirectoryEntry_p entry = findEntry(address);
        bool valid                     = false;
        SharingVector sharers;
        SharingState state = ZeroSharers;
        if (entry != nullptr) {
            valid   = true;
            sharers = entry->sharers;
            state   = entry->state;
        }

        return std::tie(sharers, state, entry, valid);
    }

    virtual void processRequestResponse(int32_t index,
                                        const MMType& request,
                                        MMType& response,
                                        AbstractEntry_p dir_entry,
                                        PhysicalMemoryAddress address,
                                        bool off_chip)
    {
        // First, do default behaviour
        AbstractDirectory::processRequestResponse(index, request, response, dir_entry, address, off_chip);

        if (!MemoryMessage::isEvictType(request)) { return; }

        InfiniteDirectoryEntry* my_entry = dynamic_cast<InfiniteDirectoryEntry*>(dir_entry.get());
        if (my_entry == nullptr) { return; }
        if (my_entry->state == ZeroSharers) { theDirectory.erase(address); }
    }

    void saveState(std::ostream& s, const std::string& aDirName)
    {

        json checkpoint;
        checkpoint = json::array();

        uint32_t count = (uint32_t)theDirectory.size();
        DBG_(Dev, (<< "Saving " << count << " directory entries."));

        inf_directory_t::iterator iter = theDirectory.begin();
        uint64_t i                     = 0;
        for (; iter != theDirectory.end(); iter++) {

            uint64_t dirAddress = iter->second->address;

            checkpoint[i++] = { { "tag", dirAddress }, { "sharers", iter->second->sharers.getSharers().to_string() } };

            DBG_(Trace, (<< "Directory saving block: " << dirAddress));
        }

        s << std::setw(4) << checkpoint << std::endl;
    }

    bool loadState(std::istream& s, const std::string& aDirName)
    {

        json checkpoint;
        s >> checkpoint;

        uint32_t dirSize = checkpoint["entries"].size();

        // empty the directory
        theDirectory.clear();

        DBG_(Trace, (<< "Directory loading " << dirSize << " entries."));

        // add elements to the directory
        for (uint32_t count = 0; count < dirSize; count++) {

            DBG_(Trace, (<< "Directory loading block " << count));

            uint64_t address = checkpoint["entries"].at(count)["tag"];
            std::bitset<MAX_NUM_SHARERS> state(checkpoint.at(count)["sharers"].get<std::string>());

            if (state != ZeroSharers) {
                theDirectory.insert(std::pair<PhysicalMemoryAddress, InfiniteDirectoryEntry*>(
                  PhysicalMemoryAddress(address),
                  new InfiniteDirectoryEntry(PhysicalMemoryAddress(address), state)));
            }
        }

        return true;
    }

    static AbstractDirectory* createInstance(std::list<std::pair<std::string, std::string>>& args)
    {
        InfiniteDirectory* directory = new InfiniteDirectory();

        directory->parseGenericOptions(args);

        std::list<std::pair<std::string, std::string>>::iterator iter = args.begin();
        for (; iter != args.end(); iter++) {
            // Parse any specifc options we want
        }

        return directory;
    }

    static const std::string name;
};

REGISTER_DIRECTORY_TYPE(InfiniteDirectory, "Infinite");

}; // namespace nFastCMPCache
