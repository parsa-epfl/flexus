#ifndef FLEXUS_SLICES__REGIONSCOUTMESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__REGIONSCOUTMESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <list>

namespace Flexus {
namespace SharedTypes {

class RegionScoutMessage : public boost::counted_base
{
    typedef PhysicalMemoryAddress MemoryAddress;

  public:
    enum RegionScoutMessageType
    {
        eRegionProbe,
        eRegionStateProbe,

        eRegionMissReply,
        eRegionHitReply,

        eRegionIsShared,
        eRegionNonShared,

        eRegionGlobalMiss,
        eRegionPartialMiss,

        eRegionProbeOwner,
        eRegionOwnerReply,
        eRegionSetOwner,

        eRegionEvict,

        eBlockProbe,
        eBlockScoutProbe,
        eBlockMissReply,
        eBlockHitReply,

        eSetTagProbe,
        eRVASetTagProbe,
        eSetTagReply,
        eRVASetTagReply

    };

  private:
    RegionScoutMessageType theType;
    MemoryAddress theRegion;
    int32_t theOwner;
    int32_t theCount;
    uint32_t theBlocks;
    bool theShared;

    std::list<MemoryAddress> theTagList;

  public:
    const RegionScoutMessageType type() const { return theType; }
    void setType(const RegionScoutMessageType& type) { theType = type; }

    const MemoryAddress region() const { return theRegion; }
    void setRegion(const MemoryAddress& region) { theRegion = region; }

    const int32_t owner() const { return theOwner; }
    void setOwner(const int32_t& owner) { theOwner = owner; }

    const bool shared() const { return theShared; }
    void setShared(const bool& shared) { theShared = shared; }

    const int32_t count() const { return theCount; }
    void setCount(const int32_t& count) { theCount = count; }

    const uint32_t blocks() const { return theBlocks; }
    void setBlocks(const uint32_t& blocks) { theBlocks = blocks; }

    void addTag(const MemoryAddress& addr) { theTagList.push_back(addr); }
    std::list<MemoryAddress>& getTags() { return theTagList; }

    explicit RegionScoutMessage(RegionScoutMessageType aType)
      : theType(aType)
      , theRegion(0)
      , theOwner(-1)
      , theBlocks(0)
      , theShared(true)
    {
    }

    explicit RegionScoutMessage(RegionScoutMessageType aType, const MemoryAddress& aRegion)
      : theType(aType)
      , theRegion(aRegion)
      , theOwner(-1)
      , theBlocks(0)
      , theShared(true)
    {
    }

    explicit RegionScoutMessage(RegionScoutMessageType aType, const MemoryAddress& aRegion, int32_t anOwner)
      : theType(aType)
      , theRegion(aRegion)
      , theOwner(anOwner)
      , theBlocks(0)
    {
    }

    friend std::ostream& operator<<(std::ostream& s, RegionScoutMessage const& aMsg);
};

std::ostream&
operator<<(std::ostream& s, RegionScoutMessage::RegionScoutMessageType const& aType);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__REGIONSCOUTMESSAGE_HPP_INCLUDED
