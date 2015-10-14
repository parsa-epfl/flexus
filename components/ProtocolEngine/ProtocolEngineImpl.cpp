#include <components/ProtocolEngine/ProtocolEngine.hpp>

#define FLEXUS_BEGIN_COMPONENT ProtocolEngine
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <memory>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include "ServiceFlexus.hpp"
#include "ProtSharedTypes.hpp"
#include "he_exec_engine.hpp"
#include "re_exec_engine.hpp"
#include "protocol_engine.hpp"

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/Common/Transports/DirectoryTransport.hpp>
#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/DirectoryMessage.hpp>

#define DBG_DefineCategories ProtocolEngine
#define DBG_SetDefaultOps AddCat(ProtocolEngine)
#include DBG_Control()

namespace nProtocolEngine {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

class FLEXUS_COMPONENT(ProtocolEngine) {
  FLEXUS_COMPONENT_IMPL(ProtocolEngine);

private:
  typedef tProtocolEngine<tHEExecEngine> tHomeEngine;
  typedef tProtocolEngine<tREExecEngine> tRemoteEngine;

  std::shared_ptr<tHomeEngine>      theHomeEngine;        // the home engine
  std::shared_ptr<tRemoteEngine>    theRemoteEngine;      // the remote engine
  std::shared_ptr<ServiceFlexus>    theHESrvc;            // service provider for the home engine
  std::shared_ptr<ServiceFlexus>    theRESrvc;            // service provider for the remote engine

  struct ReceiveBuffer {
    ReceiveBuffer() : theTransport() {}
    boost::optional<NetworkTransport> theTransport;
    bool isEmpty() const {
      return !theTransport.is_initialized();
    }
    void insert(NetworkTransport & aTransport) {
      DBG_Assert(isEmpty());
      theTransport.reset(aTransport);
    }
    void remove() {
      DBG_Assert(!isEmpty());
      theTransport.reset();
    }
  };

  std::vector<ReceiveBuffer> theRecvBuffer;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(ProtocolEngine)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    bool quiesced = true;
    for (int32_t i = 0; i < cfg.VChannels && quiesced; ++i) {
      quiesced = theRecvBuffer[i].isEmpty();
    }
    if (quiesced && theHomeEngine) {
      quiesced = theHomeEngine->isQuiesced();
    }
    if (quiesced && theRemoteEngine) {
      quiesced = theRemoteEngine->isQuiesced();
    }
    return quiesced;
  }

  boost::intrusive_ptr<Flexus::SharedTypes::MemoryMap> theMemoryMap;

  // Initialization
  void initialize() {
    theRecvBuffer.resize(cfg.VChannels);

    Flexus::SharedTypes::node_id_t node_id = flexusIndex();
    Flexus::SharedTypes::node_id_t max_nodes = flexusWidth();
    theMemoryMap = Flexus::SharedTypes::MemoryMap::getMemoryMap(node_id);

    // first make the service providers
    theHESrvc = std::shared_ptr<ServiceFlexus>(new ServiceFlexus(node_id, max_nodes, theMemoryMap, cfg.CPI));

    // and pass them into the engines
    std::string HE_name = boost::padded_string_cast < 2, '0' > (flexusIndex()) + "-HE";
    theHomeEngine = std::shared_ptr<tHomeEngine>( new tHomeEngine( HE_name, *theHESrvc, cfg.TSRFSize));
    theHESrvc->setProtocolEngine(theHomeEngine);
    theHomeEngine->init();

    if (cfg.Remote) {
      std::string RE_name = boost::padded_string_cast < 2, '0' > (flexusIndex()) + "-RE";
      theRESrvc = std::shared_ptr<ServiceFlexus>(new ServiceFlexus(node_id, max_nodes, theMemoryMap, cfg.CPI));
      theRemoteEngine = std::shared_ptr<tRemoteEngine>(new tRemoteEngine(RE_name, *theRESrvc, cfg.TSRFSize));
      theRESrvc->setProtocolEngine(theRemoteEngine);
      theRemoteEngine->init();
    }

  }

  // Ports
  bool available( interface::FromNic const &, index_t aVC) {
    // available IFF there's a spot in our single-entry buffer
    return theRecvBuffer[aVC].isEmpty();
  }
  void push( interface::FromNic const &, index_t aVC, NetworkTransport & transport) {
    // add transport to single-entry buffer
    DBG_Assert(theRecvBuffer[aVC].isEmpty());
    theRecvBuffer[aVC].insert(transport);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromDirectory);
  void push( interface::FromDirectory const &, DirectoryTransport & transport) {
    if (theHESrvc->isAddressLocal(transport[DirectoryMessageTag]->addr)) {
      DBG_(Iface, Comp(*this) ( << "Dir --> HE: " << *transport[DirectoryMessageTag] ) );
      theHESrvc->recvDirectory(transport);
    } else {
      throw FlexusException("Directory should not talk to remote engine");
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromCpu);
  void push( interface::FromCpu const &, MemoryTransport & transport) {
    if (transport[TransactionTrackerTag]) {
      transport[TransactionTrackerTag]->setInPE(true);
    }

    if (theHESrvc->isAddressLocal(transport[MemoryMessageTag]->address())) {
      DBG_(Iface, Comp(*this) ( << "CPU --> HE: " << *transport[MemoryMessageTag] ) );
      theHESrvc->recvCpu(transport);
    } else {
      DBG_Assert(cfg.Remote);
      DBG_(Iface, Comp(*this) ( << "CPU --> RE: " << *transport[MemoryMessageTag] ) );
      theRESrvc->recvCpu(transport);
    }
  }

  //Drive Interfaces
  void drive( interface::ProtocolEngineDrive const &) {
    nextClock();
  }

private:

  // main cycle function
  void nextClock() {

    // next advance the engines
    DBG_(VVerb, Comp(*this) ( << "Entered Protocol Engine" ) );

    // move messages from Nic receive buffers to input queues
    // CAVEAT: this code will move multiple VC entries in one clock cycle!
    for (int32_t ii = cfg.VChannels - 1; ii >= 0; ii--) {
      if (theRecvBuffer[ii].theTransport) {
        NetworkTransport tp = *(theRecvBuffer[ii].theTransport);
        if (theHESrvc->isAddressLocal(tp[ProtocolMessageTag]->address())) {
          DBG_( Iface, Comp(*this) ( << "HE queue size[vc:" << ii << "] " << theHESrvc->queueSize(ii + 2)));
          if (theHESrvc->queueSize(ii + 2) < cfg.TSRFSize + 1) { //Was ReceiveCapacity
            DBG_( Iface, Comp(*this) ( << "Nic --> HE: " << *tp[ProtocolMessageTag] ) );
            theHESrvc->recvNetwork(tp);
            theRecvBuffer[ii].remove();
          }  // else the entry in the RecvBuffer stays
        } else {
          DBG_( Iface, Comp(*this) ( << "RE queue size[vc:" << ii << "] " << theRESrvc->queueSize(ii + 2)));
          if (theRESrvc->queueSize(ii + 2) < cfg.TSRFSize + 1) { //Was ReceiveCapacity
            DBG_Assert(cfg.Remote);
            DBG_( Iface, Comp(*this) ( << "Nic --> RE: " << *tp[ProtocolMessageTag] ) );
            theRESrvc->recvNetwork(tp);
            theRecvBuffer[ii].remove();
          }  // else the entry in the RecvBuffer stays
        }
      }
    }

    theHomeEngine->do_cycle();
    if (cfg.Remote) {
      theRemoteEngine->do_cycle();
    }

    // look for messages to send
    DBG_(VVerb, Comp(*this) ( << "Sending output messages." ) );
    if (!theHESrvc->toDir.empty()) {
      if (FLEXUS_CHANNEL(ToDirectory).available()) {
        DBG_(Iface, Comp(*this) ( << "HE --> Dir: " << *theHESrvc->toDir.front()[DirectoryMessageTag] ) );
        FLEXUS_CHANNEL(ToDirectory) << theHESrvc->toDir.front();
        theHESrvc->toDir.erase(theHESrvc->toDir.begin());
      }
    }
    if (!theHESrvc->toNetwork.empty()) {
      if (FLEXUS_CHANNEL(ToNic).available()) {
        DBG_(Iface, Comp(*this) ( << "HE --> Nic: " << *theHESrvc->toNetwork.front()[ProtocolMessageTag]  ) );
        FLEXUS_CHANNEL(ToNic) << theHESrvc->toNetwork.front();
        theHESrvc->toNetwork.erase(theHESrvc->toNetwork.begin());
      }
    }
    if (!theHESrvc->toCpu.empty()) {
      if (FLEXUS_CHANNEL(ToCpu).available()) {
        DBG_(Iface, Comp(*this) ( << "HE --> CPU: " << * theHESrvc->toCpu.front()[MemoryMessageTag]  ) );
        if (theHESrvc->toCpu.front()[TransactionTrackerTag]) {
          theHESrvc->toCpu.front()[TransactionTrackerTag]->setInPE(false);
        }
        FLEXUS_CHANNEL(ToCpu) << theHESrvc->toCpu.front();
        theHESrvc->toCpu.erase(theHESrvc->toCpu.begin());
      }
    }
    /* CMU-ONLY-BLOCK-BEGIN */
    if (!theHESrvc->toPredictor.empty()) {
      if (FLEXUS_CHANNEL(ToPredictor).available()) {
        DBG_(Iface, Comp(*this) ( << "HE --> Pred: " << * theHESrvc->toPredictor.front()[PredictorMessageTag]) );
        FLEXUS_CHANNEL(ToPredictor) << theHESrvc->toPredictor.front();
        theHESrvc->toPredictor.erase(theHESrvc->toPredictor.begin());
      }
    }
    /* CMU-ONLY-BLOCK-END */

    if (cfg.Remote) {
      if (!theRESrvc->toDir.empty()) {
        throw FlexusException("Remote engine should not request from directory");
      }
      if (!theRESrvc->toNetwork.empty()) {
        if (FLEXUS_CHANNEL(ToNic).available()) {
          DBG_(Iface, Comp(*this) ( << "RE --> Nic: " << *theRESrvc->toNetwork.front()[ProtocolMessageTag]) );
          FLEXUS_CHANNEL(ToNic) << theRESrvc->toNetwork.front();
          theRESrvc->toNetwork.erase(theRESrvc->toNetwork.begin());
        }
      }
      if (!theRESrvc->toCpu.empty()) {
        if (FLEXUS_CHANNEL( ToCpu).available()) {
          DBG_(Iface, Comp(*this) ( << "RE --> CPU: " << * theRESrvc->toCpu.front()[MemoryMessageTag]  ) );
          if (theRESrvc->toCpu.front()[TransactionTrackerTag]) {
            theRESrvc->toCpu.front()[TransactionTrackerTag]->setInPE(false);
          }
          FLEXUS_CHANNEL(ToCpu) << theRESrvc->toCpu.front();
          theRESrvc->toCpu.erase(theRESrvc->toCpu.begin());
        }
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      if (!theRESrvc->toPredictor.empty()) {
        if (FLEXUS_CHANNEL(ToPredictor).available()) {
          DBG_(Iface, Comp(*this) ( << "RE --> Pred: " << * theRESrvc->toPredictor.front()[PredictorMessageTag]  ) );
          FLEXUS_CHANNEL(ToPredictor) << theRESrvc->toPredictor.front();
          theRESrvc->toPredictor.erase(theRESrvc->toPredictor.begin());
        }
      }
      /* CMU-ONLY-BLOCK-END */
    }

    //Simics::API::SIM_break_simulation("done with cycle");
  }  // nextClock()

};

} //End Namespace nProtocolEngine

FLEXUS_COMPONENT_INSTANTIATOR( ProtocolEngine, nProtocolEngine );
FLEXUS_PORT_ARRAY_WIDTH( ProtocolEngine, FromNic ) {
  return cfg.VChannels;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT ProtocolEngine

#define DBG_Reset
#include DBG_Control()
