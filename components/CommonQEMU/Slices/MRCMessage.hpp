#ifndef FLEXUS_SLICES__MRCMESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__MRCMESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

class MRCMessage : public boost::counted_base {

public:
  enum MRCMessageType {
    //Sent by Consort to inform MRCReflector that it has added a block to
    //its buffer
    kAppend,

    //Sent by MRCReflector to request a stream on behalf of a node which
    //experienced a read miss
    kRequestStream

  };

private:
  MRCMessageType theType;
  uint32_t theNode;
  PhysicalMemoryAddress theAddress;
  int64_t theLocation;
  int64_t  theTag;

public:
  const MRCMessageType type() const {
    return theType;
  }

  uint32_t node() const {
    return theNode;
  }

  PhysicalMemoryAddress address() const {
    return theAddress;
  }

  int64_t location() const {
    return theLocation;
  }

  int64_t  tag() const {
    return theTag;
  }

  MRCMessage(MRCMessageType aType, uint32_t aNode, PhysicalMemoryAddress anAddress, int64_t aLocation, int64_t  aTag = -1)
    : theType(aType)
    , theNode(aNode)
    , theAddress(anAddress)
    , theLocation(aLocation)
    , theTag(aTag)
  {}

  friend std::ostream & operator << (std::ostream & s, MRCMessage const & aMsg);
};

} //SharedTypes
} //Scaffold

#endif  // FLEXUS_SLICES__MRCMESSAGE_HPP_INCLUDED
