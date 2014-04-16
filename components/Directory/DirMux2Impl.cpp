#include <components/Directory/DirMux2.hpp>

#define FLEXUS_BEGIN_COMPONENT DirMux2
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories DirMux
#define DBG_SetDefaultOps AddCat(DirMux)
#include DBG_Control()

#include <components/Common/Slices/Mux.hpp>

namespace nDirMux2 {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

class FLEXUS_COMPONENT(DirMux2) {
  FLEXUS_COMPONENT_IMPL(DirMux2);

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DirMux2)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return true; //DirMux is always quiesced.
  }

  // Initialization
  void initialize() {
  }

  // Ports

  bool available(interface::TopIn1 const &) {
    return FLEXUS_CHANNEL(BottomOut).available();
  }
  void push(interface::TopIn1 const &, DirectoryTransport & aDirTransport) {
    // add the arbitration slice to the transport so it can be
    // directed correctly on the way back
    intrusive_ptr<Mux> arb( new Mux(1) );
    aDirTransport.set(DirMux2ArbTag, arb);
    FLEXUS_CHANNEL( BottomOut) << aDirTransport;
  }

  bool available(interface::TopIn2 const &) {
    return FLEXUS_CHANNEL(BottomOut).available();
  }
  void push(interface::TopIn2 const &, DirectoryTransport & aDirTransport) {
    // add the arbitration slice to the transport so it can be
    // directed correctly on the way back
    intrusive_ptr<Mux> arb( new Mux(2) );
    aDirTransport.set(DirMux2ArbTag, arb);
    FLEXUS_CHANNEL( BottomOut) << aDirTransport;
  }

  bool available(interface::BottomIn const &) {
    return FLEXUS_CHANNEL(TopOut1).available() && FLEXUS_CHANNEL(TopOut2).available();
  }
  void push(interface::BottomIn const &, DirectoryTransport & aDirTransport) {
    switch (aDirTransport[DirMux2ArbTag]->source) {
      case 1:
        FLEXUS_CHANNEL( TopOut1) << aDirTransport;
        break;
      case 2:
        FLEXUS_CHANNEL( TopOut2) << aDirTransport;
        break;
      default:
        DBG_Assert(false, ( << "Invalid source"));
    }
  }

};

} //End Namespace nDirMux2

FLEXUS_COMPONENT_INSTANTIATOR( DirMux2, nDirMux2 );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DirMux2

#define DBG_Reset
#include DBG_Control()
