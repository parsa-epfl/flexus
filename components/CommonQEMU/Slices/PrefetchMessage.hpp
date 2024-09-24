#ifndef FLEXUS_SLICES__PREFETCHMESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__PREFETCHMESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

struct PrefetchMessage : public boost::counted_base
{
    typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

    // enumerated message type
    enum PrefetchMessageType
    {
        // Requests made by Prefetch controller
        PrefetchReq,
        // Tells the prefetch buffer to perform a prefetch.
        DiscardReq,
        // Tells the prefetch buffer to discard a line, it if is present.
        WatchReq,
        // Tells the prefetch buffer to monitor a cache line, and report if
        // any requests arrive for it.

        // Responses/Notifications from Prefetch Buffer
        PrefetchComplete,
        // Indicates that a prefetch operation has been completed.
        PrefetchRedundant,
        // Indicates that a prefetch operation was rejected by the memory
        // heirarchy.

        LineHit,
        // This indicates that a hit has occurred on a prefetched line
        LineHit_Partial,
        // This indicates that a hit has occurred on a prefetched line
        LineReplaced,
        // This indicates that a line was removed by replacement
        LineRemoved,
        // This indicates that a line was removed for some reason other
        // than replacement.

        WatchPresent,
        // This indicates that a watched line was found when the cache heirarchy
        // was probed
        WatchRedundant,
        // This indicates that a watched line was found in the prefetch buffer
        WatchRequested,
        // This indicates that a watched line was found when the cache heirarchy
        // was probed
        WatchRemoved,
        // This indicates that a watched line is no longer tracked because of a
        // snoop or a write to the line
        WatchReplaced
        // This indicates that a watched line was dropped from the watch list
        // because of a replacement
    };

    const PrefetchMessageType type() const { return theType; }
    const MemoryAddress address() const { return theAddress; }

    PrefetchMessageType& type() { return theType; }
    MemoryAddress& address() { return theAddress; }

    const int64_t streamID() const { return theStreamID; }
    int64_t& streamID() { return theStreamID; }

    explicit PrefetchMessage(PrefetchMessageType aType, MemoryAddress anAddress, int64_t aStreamId = 0)
      : theType(aType)
      , theAddress(anAddress)
      , theStreamID(aStreamId)
    {
    }

    friend std::ostream& operator<<(std::ostream& s, PrefetchMessage const& aMsg);

  private:
    PrefetchMessageType theType;
    MemoryAddress theAddress;
    int64_t theStreamID;
};

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__PREFETCHMESSAGE_HPP_INCLUDED
