#ifndef FLEXUS_SLICES__PREFETCHCOMMAND_HPP_INCLUDED
#define FLEXUS_SLICES__PREFETCHCOMMAND_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <list>

namespace Flexus {
namespace SharedTypes {

class PrefetchCommand : public boost::counted_base
{
    typedef PhysicalMemoryAddress MemoryAddress;

  public:
    enum PrefetchCommandType
    {
        // This command tells the PrefetchListener to prefetch the specified
        // list of addresses.
        ePrefetchAddressList,

        ePrefetchRequestMoreAddresses

    };

  private:
    PrefetchCommandType theType;
    std::list<MemoryAddress> theAddressList;
    unsigned theSource;
    int32_t theQueue;
    int64_t theLocation;
    int64_t theTag;

  public:
    const PrefetchCommandType type() const { return theType; }
    PrefetchCommandType& type() { return theType; }

    std::list<MemoryAddress>& addressList() { return theAddressList; }

    std::list<MemoryAddress> const& addressList() const { return theAddressList; }

    unsigned& source() { return theSource; }

    int32_t queue() const { return theQueue; }

    int32_t& queue() { return theQueue; }

    unsigned source() const { return theSource; }

    int64_t& location() { return theLocation; }

    int64_t location() const { return theLocation; }

    int64_t& tag() { return theTag; }

    int64_t tag() const { return theTag; }

    explicit PrefetchCommand(PrefetchCommandType aType)
      : theType(aType)
      , theSource(0)
      , theQueue(0)
      , theLocation(-1)
      , theTag(-1)
    {
    }

    friend std::ostream& operator<<(std::ostream& s, PrefetchCommand const& aMsg);
};

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__PREFETCHCOMMAND_HPP_INCLUDED
