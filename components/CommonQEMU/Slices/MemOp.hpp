#ifndef FLEXUS_SLICES__MEMOP_HPP_INCLUDED
#define FLEXUS_SLICES__MEMOP_HPP_INCLUDED

#include <boost/dynamic_bitset.hpp>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <functional>

namespace Flexus {
namespace SharedTypes {

enum eOperation
{ // Sorted by priority for requesting memory ports
    kPageWalkRequest,
    kLoad,
    kLDP,
    kAtomicPreload,
    kRMW,
    kCAS,
    kCASP,
    kStorePrefetch,
    kStore,
    kInvalidate,
    kDowngrade,
    kProbe,
    kReturnReq,
    kLoadReply,
    kAtomicPreloadReply,
    kStoreReply,
    kStorePrefetchReply,
    kRMWReply,
    kCASReply,
    kDowngradeAck,
    kInvAck,
    kProbeAck,
    kReturnReply,
    kMEMBARMarker,
    kINVALID_OPERATION,
    kLastOperation
};
std::ostream&
operator<<(std::ostream& anOstream, eOperation op);

enum eSize
{
    kByte       = 1,
    kHalfWord   = 2,
    kWord       = 4,
    kDoubleWord = 8,
    kQuadWord   = 16,
    kIllegalSize
};

std::ostream&
operator<<(std::ostream& anOstream, eSize op);

eSize
dbSize(uint32_t s);

struct MemOp : boost::counted_base
{
    eOperation theOperation;
    eSize theSize;
    VirtualMemoryAddress theVAddr;
    PhysicalMemoryAddress thePAddr;
    VirtualMemoryAddress thePC;
    bits theValue;
    bits theExtendedValue;
    bool theReverseEndian;
    bool theNonCacheable;
    bool theSideEffect;
    bool theAtomic;
    bool theNAW;
    boost::intrusive_ptr<TransactionTracker> theTracker;
    boost::intrusive_ptr<AbstractInstruction> theInstruction;

    MemOp()
      : theOperation(kINVALID_OPERATION)
      , theSize(kWord)
      , theVAddr(VirtualMemoryAddress(0))
      , thePAddr(PhysicalMemoryAddress(0))
      , thePC(VirtualMemoryAddress(0))
      , theValue(0)
      , theExtendedValue(0)
      , theReverseEndian(false)
      , theNonCacheable(false)
      , theSideEffect(false)
      , theAtomic(false)
      , theNAW(false)
    {
    }
    MemOp(MemOp const& anOther)
      : theOperation(anOther.theOperation)
      , theSize(anOther.theSize)
      , theVAddr(anOther.theVAddr)
      , thePAddr(anOther.thePAddr)
      , thePC(anOther.thePC)
      , theValue(anOther.theValue)
      , theExtendedValue(anOther.theExtendedValue)
      , theReverseEndian(anOther.theReverseEndian)
      , theNonCacheable(anOther.theNonCacheable)
      , theSideEffect(anOther.theSideEffect)
      , theAtomic(anOther.theAtomic)
      , theNAW(anOther.theNAW)
      , theTracker(anOther.theTracker)
      , theInstruction(anOther.theInstruction)
    {
    }
};

std::ostream&
operator<<(std::ostream& anOstream, MemOp const& aMemOp);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__MEMOP_HPP_INCLUDED
