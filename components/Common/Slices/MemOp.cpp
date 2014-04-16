#include <iostream>

#include <core/types.hpp>

#include <components/Common/Slices/MemOp.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator <<( std::ostream & anOstream, eOperation op) {
  const char * map_tables[] = {
    "Load"
    , "AtomicPreload"
    , "RMW"
    , "CAS"
    , "StorePrefetch"
    , "Store"
    , "Invalidate"
    , "Downgrade"
    , "Probe"
    , "Return"
    , "LoadReply"
    , "AtomicPreloadReply"
    , "StoreReply"
    , "StorePrefetchReply"
    , "RMWReply"
    , "CASReply"
    , "DowngradeAck"
    , "InvAck"
    , "ProbeAck"
    , "ReturnReply"
    , "MEMBARMarker"
    , "LastOperation"
  };
  if (op >= kLastOperation) {
    anOstream << "Invalid Operation(" << static_cast<int>(op) << ")";
  } else {
    anOstream << map_tables[op];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, MemOp const & aMemOp) {
  anOstream
      << aMemOp.theOperation
      << "(" << aMemOp.theSize << ") "
      << aMemOp.theVAddr
      << "[" << aMemOp.theASI << "] "
      << aMemOp.thePAddr
      << " pc:" << aMemOp.thePC
      ;
  if (aMemOp.theReverseEndian) {
    anOstream << " {reverse-endian}";
  }
  if (aMemOp.theNonCacheable) {
    anOstream << " {non-cacheable}";
  }
  if (aMemOp.theSideEffect) {
    anOstream << " {side-effect}";
  }
  if (aMemOp.theNAW) {
    anOstream << " {non-allocate-write}";
  }
  anOstream << " =" << aMemOp.theValue;
  return anOstream;
}

} //SharedTypes
} //Flexus
