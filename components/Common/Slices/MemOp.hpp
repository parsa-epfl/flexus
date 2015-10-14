#ifndef FLEXUS_SLICES__MEMOP_HPP_INCLUDED
#define FLEXUS_SLICES__MEMOP_HPP_INCLUDED

#include <functional>
#include <boost/dynamic_bitset.hpp>

#include <components/Common/Slices/AbstractInstruction.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

namespace Flexus {
namespace SharedTypes {

enum eOperation { //Sorted by priority for requesting memory ports
  kLoad
  , kAtomicPreload
  , kRMW
  , kCAS
  , kStorePrefetch
  , kStore
  , kInvalidate
  , kDowngrade
  , kProbe
  , kReturnReq
  , kLoadReply
  , kAtomicPreloadReply
  , kStoreReply
  , kStorePrefetchReply
  , kRMWReply
  , kCASReply
  , kDowngradeAck
  , kInvAck
  , kProbeAck
  , kReturnReply
  , kMEMBARMarker
  , kINVALID_OPERATION
  , kLastOperation
};
std::ostream & operator << ( std::ostream & anOstream, eOperation op);

enum eSize {
  kByte = 1
  , kHalfWord = 2
  , kWord = 4
  , kDoubleWord = 8
};

struct MemOp : boost::counted_base {
  eOperation theOperation;
  eSize theSize;
  VirtualMemoryAddress theVAddr;
  int32_t theASI;
  PhysicalMemoryAddress thePAddr;
  VirtualMemoryAddress thePC;
  uint64_t theValue;
  uint64_t theExtendedValue;
  bool theReverseEndian;
  bool theNonCacheable;
  bool theSideEffect;
  bool theAtomic;
  bool theNAW;
  boost::intrusive_ptr< TransactionTracker > theTracker;
  MemOp( )
    : theOperation( kINVALID_OPERATION )
    , theSize( kWord )
    , theVAddr( VirtualMemoryAddress(0) )
    , theASI( 0x80 )
    , thePAddr( PhysicalMemoryAddress(0) )
    , thePC (VirtualMemoryAddress(0) )
    , theValue( 0 )
    , theExtendedValue ( 0 )
    , theReverseEndian(false)
    , theNonCacheable(false)
    , theSideEffect(false)
    , theAtomic(false)
    , theNAW(false)
  {}
  MemOp( MemOp const & anOther)
    : theOperation( anOther.theOperation )
    , theSize( anOther.theSize)
    , theVAddr( anOther.theVAddr)
    , theASI( anOther.theASI )
    , thePAddr( anOther.thePAddr)
    , thePC( anOther.thePC)
    , theValue( anOther.theValue)
    , theExtendedValue( anOther.theExtendedValue)
    , theReverseEndian(anOther.theReverseEndian)
    , theNonCacheable(anOther.theNonCacheable)
    , theSideEffect(anOther.theSideEffect)
    , theAtomic(anOther.theAtomic)
    , theNAW(anOther.theNAW)
    , theTracker( anOther.theTracker )
  {}

};

std::ostream & operator << ( std::ostream & anOstream, MemOp const & aMemOp);

} //SharedTypes
} //Flexus

#endif //FLEXUS_SLICES__MEMOP_HPP_INCLUDED

