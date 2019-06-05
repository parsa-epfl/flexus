#include <components/Cache/IDCacheMux.hpp>

#define FLEXUS_BEGIN_COMPONENT IDCacheMux
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nIDCacheMux {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

class FLEXUS_COMPONENT(IDCacheMux) {
  FLEXUS_COMPONENT_IMPL( IDCacheMux );

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(IDCacheMux)
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
  // From the instruction cache
  bool available( interface::TopInI const &) {
    return FLEXUS_CHANNEL( BottomOut).available();
  }
  void push( interface::TopInI const &, MemoryTransport & aMemTransport) {
    intrusive_ptr<Mux> arb( new Mux('i') );
    aMemTransport.set(MuxTag, arb);
    FLEXUS_CHANNEL( BottomOut) << aMemTransport;
  }

  // From the data cache
  bool available( interface::TopInD const &) {
    return FLEXUS_CHANNEL( BottomOut).available();
  }
  void push( interface::TopInD const &, MemoryTransport & aMemTransport) {
    intrusive_ptr<Mux> arb( new Mux('d') );
    aMemTransport.set(MuxTag, arb);
    FLEXUS_CHANNEL( BottomOut) << aMemTransport;
  }

  bool available( interface::BottomIn const &) {
    return FLEXUS_CHANNEL(TopOutI).available() && FLEXUS_CHANNEL(TopOutD).available();
  }
  void push( interface::BottomIn const &, MemoryTransport & aMemTransport) {
    if (aMemTransport[MuxTag] == 0) {
      // slice doesn't exist in the transport - forward to the data cache
      FLEXUS_CHANNEL(TopOutD) << aMemTransport;
    } else {
      switch (aMemTransport[MuxTag]->source) {
        case 'i':
          FLEXUS_CHANNEL(TopOutI) << aMemTransport;
          break;
        case 'd':
          FLEXUS_CHANNEL(TopOutD) << aMemTransport;
          break;
        default:
          DBG_Assert(false, ( << "Invalid source"));
      }
    }
  }

  bool available( interface::TopIn_SnoopI const &) {
    return FLEXUS_CHANNEL( BottomOut_Snoop).available();
  }
  void push( interface::TopIn_SnoopI const &, MemoryTransport & aMemTransport) {
    intrusive_ptr<Mux> arb( new Mux('i') );
    aMemTransport.set(MuxTag, arb);
    aMemTransport[MemoryMessageTag]->dstream() = false;
    FLEXUS_CHANNEL( BottomOut_Snoop) << aMemTransport;
  }

  // From the data cache
  bool available( interface::TopIn_SnoopD const &) {
    return FLEXUS_CHANNEL( BottomOut_Snoop).available();
  }
  void push( interface::TopIn_SnoopD const &, MemoryTransport & aMemTransport) {
    intrusive_ptr<Mux> arb( new Mux('d') );
    aMemTransport.set(MuxTag, arb);
    aMemTransport[MemoryMessageTag]->dstream() = true;
    FLEXUS_CHANNEL( BottomOut_Snoop) << aMemTransport;
  }
};

} //End Namespace nIDCacheMux

FLEXUS_COMPONENT_INSTANTIATOR( IDCacheMux, nIDCacheMux );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT IDCacheMux
