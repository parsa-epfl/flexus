#ifndef FLEXUS_SLICES__TAGLESS_DIR_MSG_HPP_INCLUDED
#define FLEXUS_SLICES__TAGLESS_DIR_MSG_HPP_INCLUDED

#include <components/CommonQEMU/GlobalHasher.hpp>
#include <components/CommonQEMU/Util.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/component.hpp>
#include <core/types.hpp>
#include <iostream>

#ifdef FLEXUS_TaglessDirMsg_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::TaglessDirMsg data type"
#endif
#define FLEXUS_TaglessDirMsg_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

typedef Flexus::SharedTypes::PhysicalMemoryAddress DestinationAddress;

struct TaglessDirMsg : public boost::counted_base
{

    TaglessDirMsg() {}

    TaglessDirMsg(const TaglessDirMsg& msg)
      : theConflictFreeBuckets(msg.theConflictFreeBuckets)
    {
    }

    TaglessDirMsg(const TaglessDirMsg* msg)
      : theConflictFreeBuckets(msg->theConflictFreeBuckets)
    {
    }

    TaglessDirMsg(boost::intrusive_ptr<TaglessDirMsg> msg)
      : theConflictFreeBuckets(msg->theConflictFreeBuckets)
    {
    }

    std::map<int, std::set<int>> theConflictFreeBuckets;

    void addConflictFreeSet(int32_t core, std::set<int>& buckets)
    {
        theConflictFreeBuckets.insert(std::make_pair(core, buckets));
    }

    const std::map<int, std::set<int>>& getConflictFreeBuckets() const { return theConflictFreeBuckets; }

    void merge(boost::intrusive_ptr<TaglessDirMsg> msg)
    {
        theConflictFreeBuckets.insert(msg->theConflictFreeBuckets.begin(), msg->theConflictFreeBuckets.end());
    }

    int32_t messageSize()
    {
        int32_t map_entries = theConflictFreeBuckets.size();

        // If there's zero or one set of buckets, assume we have room in the control
        // packet.
        if (map_entries < 2) { return 0; }

        int32_t core_id_bits =
          nCommonUtil::log_base2(Flexus::Core::ComponentManager::getComponentManager().systemWidth());
        int32_t num_hashes = nGlobalHasher::GlobalHasher::theHasher().numHashes();

        // Total size = # of entries (max = num cores - 1 -> need core_id_bits)
        //    + num_entries * entry size
        // Entry size = core_id + bucket bitmap
        int32_t total_bits = core_id_bits + map_entries * (core_id_bits + num_hashes);

        // Convert to number of bytes (round up)
        return ((total_bits + 7) / 8);
    }
};

typedef boost::intrusive_ptr<TaglessDirMsg> TaglessDirMsg_p;

inline std::ostream&
operator<<(std::ostream& aStream, const TaglessDirMsg& msg)
{

    aStream << "TaglessDirMsg: Conflict Free Buckets = ";
    std::map<int, std::set<int>>::const_iterator map_iter = msg.theConflictFreeBuckets.begin();
    for (; map_iter != msg.theConflictFreeBuckets.end(); map_iter++) {
        aStream << "Core " << map_iter->first << " = {";
        std::set<int>::iterator set_iter = map_iter->second.begin();
        bool needs_comma                 = false;
        for (; set_iter != map_iter->second.end(); set_iter++) {
            if (needs_comma) { aStream << ", "; }
            aStream << (*set_iter);
        }
        aStream << "},";
    }

    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__TAGLESS_DIR_MSG_HPP_INCLUDED
