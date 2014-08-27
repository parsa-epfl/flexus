#ifndef FLEXUS_SLICES__PERFECTPLACEMENTSLICE_HPP_INCLUDED
#define FLEXUS_SLICES__PERFECTPLACEMENTSLICE_HPP_INCLUDED

#ifdef FLEXUS_PerfectPlacementSlice_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::PerfectPlacementSlice data type"
#endif
#define FLEXUS_PerfectPlacementSlice_TYPE_PROVIDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <core/types.hpp>
#include <core/exception.hpp>

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>

namespace Flexus {
namespace SharedTypes {

using namespace Flexus::Core;
using boost::intrusive_ptr;

static MemoryMessage thePerfPlcDummyMemMsg(MemoryMessage::LoadReq);

struct PerfectPlacementSlice : public boost::counted_base { /*, public FastAlloc */
  typedef PhysicalMemoryAddress MemoryAddress;

  // enumerated message type
  enum PerfectPlacementSliceType {
    ProcessMsg,
    MakeBlockWritable
  };

  explicit PerfectPlacementSlice(PerfectPlacementSliceType aType)
    : theType(aType)
    , theAddress(0)
    , theMemoryMessage(thePerfPlcDummyMemMsg)
  {}
  explicit PerfectPlacementSlice(PerfectPlacementSliceType aType, MemoryAddress anAddress)
    : theType(aType)
    , theAddress(anAddress)
    , theMemoryMessage(thePerfPlcDummyMemMsg)
  {}
  explicit PerfectPlacementSlice(PerfectPlacementSliceType aType, MemoryAddress anAddress, MemoryMessage & aMemMsg)
    : theType(aType)
    , theAddress(anAddress)
    , theMemoryMessage(aMemMsg)
  {}

  static intrusive_ptr<PerfectPlacementSlice> newMakeBlockWritable(MemoryAddress anAddress) {
    intrusive_ptr<PerfectPlacementSlice> slice = new PerfectPlacementSlice(MakeBlockWritable, anAddress);
    return slice;
  }
  static intrusive_ptr<PerfectPlacementSlice> newProcessMsg(MemoryMessage & aMemMsg) {
    intrusive_ptr<PerfectPlacementSlice> slice = new PerfectPlacementSlice(ProcessMsg, aMemMsg.address(), aMemMsg);
    return slice;
  }

  const PerfectPlacementSliceType type() const {
    return theType;
  }
  const MemoryAddress address() const {
    return theAddress;
  }

  PerfectPlacementSliceType & type() {
    return theType;
  }
  MemoryAddress & address() {
    return theAddress;
  }
  MemoryMessage & memMsg() {
    return theMemoryMessage;
  }

private:
  PerfectPlacementSliceType theType;
  MemoryAddress theAddress;
  MemoryMessage & theMemoryMessage;
};

std::ostream & operator << (std::ostream & s, PerfectPlacementSlice const & aPerfPlcSlice);

} //End SharedTypes
} //End Flexus

#endif //FLEXUS_SLICES__PERFECTPLACEMENTSLICE_HPP_INCLUDED
