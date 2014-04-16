#ifndef FLEXUS_SLICES__PREDICTORMESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__PREDICTORMESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

typedef Flexus::SharedTypes::PhysicalMemoryAddress tAddress;

class PredictorMessage : public boost::counted_base {
public:
  enum tPredictorMessageType {
    eFlush,
    eWrite,
    eReadPredicted,
    eReadNonPredicted,
  };
private:
  tPredictorMessageType theType;
  int32_t theNode;
  tAddress theAddress;

public:
  PredictorMessage(tPredictorMessageType aType, int32_t aNode, tAddress anAddress)
    : theType(aType)
    , theNode(aNode)
    , theAddress(anAddress)
  { }
  const tPredictorMessageType type() const {
    return theType;
  }
  int32_t node() const {
    return theNode;
  }
  tAddress address() const {
    return theAddress;
  }
};

std::ostream & operator << (std::ostream & anOstream, PredictorMessage::tPredictorMessageType aType);
std::ostream & operator << (std::ostream & aStream, PredictorMessage const & anEntry);

} //SharedTypes
} //Scaffold

#endif  // FLEXUS_SLICES__PREDICTORMESSAGE_HPP_INCLUDED
