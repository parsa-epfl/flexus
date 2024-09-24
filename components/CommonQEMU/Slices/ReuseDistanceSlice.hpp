#ifndef FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED
#define FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED

#ifdef FLEXUS_ReuseDistanceSlice_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ReuseDistanceSlice data type"
#endif
#define FLEXUS_ReuseDistanceSlice_TYPE_PROVIDED

#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/exception.hpp>
#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

using namespace Flexus::Core;
using boost::intrusive_ptr;

static MemoryMessage theDummyMemMsg(MemoryMessage::LoadReq);

struct ReuseDistanceSlice : public boost::counted_base
{ /*, public FastAlloc */
    typedef PhysicalMemoryAddress MemoryAddress;

    // enumerated message type
    enum ReuseDistanceSliceType
    {
        ProcessMemMsg,
        GetMeanReuseDist_Data
    };

    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType)
      : theType(aType)
      , theAddress(0)
      , theMemoryMessage(theDummyMemMsg)
      , theReuseDist(0LL)
    {
    }
    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType, MemoryAddress anAddress)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(theDummyMemMsg)
      , theReuseDist(0LL)
    {
    }
    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType, MemoryAddress anAddress, MemoryMessage& aMemMsg)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(aMemMsg)
      , theReuseDist(0LL)
    {
    }

    static intrusive_ptr<ReuseDistanceSlice> newProcessMemMsg(MemoryMessage& aMemMsg)
    {
        intrusive_ptr<ReuseDistanceSlice> slice = new ReuseDistanceSlice(ProcessMemMsg, aMemMsg.address(), aMemMsg);
        return slice;
    }
    static intrusive_ptr<ReuseDistanceSlice> newMeanReuseDistData(MemoryAddress anAddress)
    {
        intrusive_ptr<ReuseDistanceSlice> slice = new ReuseDistanceSlice(GetMeanReuseDist_Data, anAddress);
        return slice;
    }

    const ReuseDistanceSliceType type() const { return theType; }
    const MemoryAddress address() const { return theAddress; }
    const int64_t meanReuseDist() const { return theReuseDist; }

    ReuseDistanceSliceType& type() { return theType; }
    MemoryAddress& address() { return theAddress; }
    MemoryMessage& memMsg() { return theMemoryMessage; }
    int64_t& meanReuseDist() { return theReuseDist; }

  private:
    ReuseDistanceSliceType theType;
    MemoryAddress theAddress;
    MemoryMessage& theMemoryMessage;
    int64_t theReuseDist;
};

std::ostream&
operator<<(std::ostream& s, ReuseDistanceSlice const& aReuseDistSlice);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED
