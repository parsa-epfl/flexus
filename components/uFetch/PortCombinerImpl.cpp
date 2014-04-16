#include <components/uFetch/PortCombiner.hpp>

#define FLEXUS_BEGIN_COMPONENT PortCombiner
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nPortCombiner {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

class FLEXUS_COMPONENT(PortCombiner) {
  FLEXUS_COMPONENT_IMPL( PortCombiner );

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(PortCombiner)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

public:
  // Initialization
  void initialize() {
  }

  void finalize() {}

  bool isQuiesced() const {
    return true; //Mux is always quiesced
  }

  // Ports
  bool available( interface::ReplyIn const &) {
    return FLEXUS_CHANNEL( FetchMissOut ).available();
  }
  void push( interface::ReplyIn const &, MemoryTransport & aMemTransport) {
    FLEXUS_CHANNEL( FetchMissOut ) << aMemTransport;
  }

  bool available( interface::SnoopIn const &) {
    return FLEXUS_CHANNEL( FetchMissOut ).available();
  }
  void push( interface::SnoopIn const &, MemoryTransport & aMemTransport) {
    FLEXUS_CHANNEL( FetchMissOut ) << aMemTransport;
  }

};

} //End Namespace nPortCombiner

FLEXUS_COMPONENT_INSTANTIATOR( PortCombiner, nPortCombiner );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT PortCombiner
